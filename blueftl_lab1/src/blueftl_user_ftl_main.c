#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "blueftl_user.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_util.h"
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_block.h"
#include "blueftl_mapping_page.h"
#include "blueftl_gc_page.h"
#include "blueftl_char.h"


struct ftl_base_t _ftl_base;
struct ftl_context_t* _ptr_ftl_context = NULL;

extern int8_t _is_debugging_mode;

int32_t blueftl_user_ftl_create (struct ssd_params_t* ptr_ssd_params)
{
	/* open the character device driver for blueSSD */
	if ((_ptr_vdevice = blueftl_user_vdevice_open (ptr_ssd_params)) == NULL) {
		return -1;
	}

	/* map the block mapping functions to _ftl_base
	 *
	 * _ftl_base = ftl_base_block_mapping;
	 *
	 */

	/* TODO: if policy is 1 or 2, then choose mapping policy
	 *
	 * ref: blueftl_ftl_base.h
	 *
	 */

	/* TODO block to page mapping */
	_ftl_base = ftl_base_page_mapping;

	/* initialize the user-level FTL */
	if ((_ptr_ftl_context = _ftl_base.ftl_create_ftl_context (_ptr_vdevice)) == NULL) {
		printf ("blueftl_user_ftl_main: error occurs when creating a ftl context\n");
		blueftl_user_vdevice_close (_ptr_vdevice);
		return -1;
	}

	return 0;
}

void blueftl_user_ftl_destroy (struct virtual_device_t* _ptr_vdevice)
{
	/* close the character device driver */
	blueftl_user_vdevice_close (_ptr_vdevice);

	/* destroy the user-level FTL */
	_ftl_base.ftl_destroy_ftl_context (_ptr_ftl_context);
}

/* ftl entry point */
int32_t blueftl_user_ftl_main (
	uint8_t req_dir, 
	uint32_t lba_begin, 
	uint32_t lba_end, 
	uint8_t* ptr_buffer)
{
	uint32_t lpa_begin;
	uint32_t lpa_end;
	uint32_t lpa_curr;
	int32_t ret;

	if (_ptr_vdevice == NULL || _ptr_vdevice->blueftl_char_h == -1) {
		printf ("blueftl_user_ftl_main: the character device driver is not initialized\n");
		return -1;
	}

	lpa_begin = lba_begin / _ptr_vdevice->page_main_size;
	lpa_end = lpa_begin + (lba_end / _ptr_vdevice->page_main_size);

	switch (req_dir) {
		case NETLINK_READA:
		case NETLINK_READ:
			for (lpa_curr = lpa_begin; lpa_curr < lpa_end; lpa_curr++) {
				/* find a physical page address corresponding to a given lpa */
				uint32_t bus, chip, page, block;
				uint8_t* ptr_lba_buff = ptr_buffer + 
					((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);

				if (_ftl_base.ftl_get_mapped_physical_page_address (
						_ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == 0) {
					blueftl_user_vdevice_page_read (
						_ptr_vdevice,
						bus, chip, block, page, 
						_ptr_vdevice->page_main_size, 
					   (char*)ptr_lba_buff);
				} else {
					/* the requested logical page is not mapped to any physical page */
					/* simply ignore */
				}
			}
			break;

		case NETLINK_WRITE:
			for (lpa_curr = lpa_begin; lpa_curr < lpa_end; lpa_curr++) {
				uint32_t bus, chip, block, page;
				uint8_t* ptr_lba_buff = ptr_buffer + 
					((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);
				uint8_t is_merge_needed = 0;
				
				/* get the new physical page address from the FTL */
				if (_ftl_base.ftl_get_free_physical_page_address (
						_ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == -1) {
					/* there are no free pages; do garbage collection */
					if (_ftl_base.ftl_trigger_gc != NULL) {
						/* trigger gc */
						if (_ftl_base.ftl_trigger_gc (_ptr_ftl_context, bus, chip) == -1) {
							printf ("bluessd: oops! garbage collection failed.\n");
							ret = -1;
							goto failed;
						}

						/* garbage collection has been finished; chooses the new free page */
						if (_ftl_base.ftl_get_free_physical_page_address (
								_ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == -1) {
							printf ("bluessd: there is not sufficient space in flash memory.\n");
							ret = -1;
							goto failed;
						}

						/* ok. we obtain new free space */
					} else if (_ftl_base.ftl_trigger_merge != NULL) {
						/* the FTL does not support gc,
						   so it is necessary to merge the new data with the existing data */
						is_merge_needed = 1;
					} else {
						printf ("bluessd: garbage collection is not registered\n");
						ret = -1;
						goto failed;
					}
				}

				if (is_merge_needed == 0) {
					/* perform a page write */
					blueftl_user_vdevice_page_write (
						_ptr_vdevice, 
						bus, chip, block, page, 
						_ptr_vdevice->page_main_size, 
						(char*)ptr_lba_buff);

					/* map the logical address to the new physical address */
					if (_ftl_base.ftl_map_logical_to_physical (
							_ptr_ftl_context, lpa_curr, bus, chip, block, page) == -1) {
						printf ("bluessd: map_logical_to_physical failed\n");
						ret = -1;
						goto failed;
					}
				} else {
					/* trigger merge with new data */
					if (_ftl_base.ftl_trigger_merge (
							_ptr_ftl_context, lpa_curr, ptr_lba_buff, bus, chip, block) == -1) {
						printf ("bluessd: block merge failed\n");
						ret = -1;
						goto failed;
					}
				}
			}
			break;

		default:
			printf ("bluessd: invalid I/O type (%u)\n", req_dir);
			ret = -1;
			goto failed;
	}

	ret = 0;

failed:
	blueftl_user_vdevice_req_done (_ptr_vdevice);
	return ret;
}
