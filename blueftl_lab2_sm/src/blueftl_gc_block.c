#ifdef KERNEL_MODE

#include <linux/vmalloc.h>
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_block.h"
#include "blueftl_gc_block.h"
#include "blueftl_util.h"

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blueftl_ftl_base.h"
#include "blueftl_mapping_block.h"
#include "blueftl_gc_block.h"
#include "blueftl_util.h"
#include "blueftl_user_vdevice.h"

#endif


int32_t gc_block_trigger_merge (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint8_t* ptr_new_data_buff, 
	uint32_t merge_bus, 
	uint32_t merge_chip, 
	uint32_t merge_block)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct virtual_device_t* ptr_vdevice = ptr_ftl_context->ptr_vdevice;
	struct flash_block_t* ptr_victim_block = NULL;

	uint8_t* ptr_page_buff = NULL;
	uint32_t new_page_offset = 0;
	uint32_t loop_page = 0;
	int32_t ret = 0;

	uint8_t* ptr_block_buff = NULL;

	new_page_offset = logical_page_address % ptr_ssd->nr_pages_per_block;

	/* get the victim block information */
	if ((ptr_victim_block = &(ptr_ssd->list_buses[merge_bus].list_chips[merge_chip].list_blocks[merge_block])) == NULL) {
		printf ("blueftl_gc_block: oops! 'ptr_victim_block' is NULL\n");
		return -1;
	}

	/* allocate the memory for the block merge */
	if ((ptr_block_buff = (uint8_t*)malloc (ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size)) == NULL) {
		printf ("blueftl_gc_block: the memory allocation for the merge buffer failed (%u)\n",
				ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size);
		return -1;
	}
	memset (ptr_block_buff, 0xFF, ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size);

	/* MERGE-STEP1: read all the valid pages from the target block */
	for (loop_page = 0; loop_page < ptr_ssd->nr_pages_per_block; loop_page++) {
		if (ptr_victim_block->list_pages[loop_page].page_status == 3 && loop_page != new_page_offset) {
			/* read the valid page data from the flash memory */
			ptr_page_buff = ptr_block_buff + (loop_page * ptr_vdevice->page_main_size);

			blueftl_user_vdevice_page_read (
				_ptr_vdevice,
				merge_bus, merge_chip, merge_block, loop_page, 
				ptr_vdevice->page_main_size,
				(char*)ptr_page_buff);

			perf_gc_inc_page_copies ();
		}
	}

	/* MERGE-STEP2: merge the existing data with the new data */
	ptr_page_buff = ptr_block_buff + (new_page_offset * ptr_vdevice->page_main_size);
	memcpy (ptr_page_buff, ptr_new_data_buff, ptr_vdevice->page_main_size);

	/* MERGE-STEP3: erase the target block */
	/*bluessd_erase_wait_for_completion (ptr_vdevice, merge_bus, merge_chip, merge_block);*/
	blueftl_user_vdevice_block_erase (ptr_vdevice, merge_bus, merge_chip, merge_block);
	ptr_victim_block->nr_erase_cnt++;
	perf_gc_inc_blk_erasures ();

	/* MERGE-STEP4: copy the data in the buffer to the block again */
	for (loop_page = 0; loop_page < ptr_ssd->nr_pages_per_block; loop_page++) {
		/* flush the data to the block only when
		   (1) the corresponding page has valid data before
		   (2) there are new data from the host */
		if (ptr_victim_block->list_pages[loop_page].page_status == 3 || loop_page == new_page_offset) {
			ptr_page_buff = ptr_block_buff + (loop_page * ptr_vdevice->page_main_size);

			blueftl_user_vdevice_page_write ( 
					ptr_vdevice,
					merge_bus, merge_chip, merge_block, loop_page, 
					ptr_vdevice->page_main_size, 
					(char*)ptr_page_buff);

			/* are there new data from the host? */
			if (ptr_victim_block->list_pages[loop_page].page_status == 1) {
				/* if so, 'loop_page' must be the same as 'new_page_offset' */
				if (loop_page != new_page_offset) {
					printf ("blueftl_gc_block: 'loop_page' != 'new_page_offset'\n");
					ret = -1;
					goto gc_exit;
				}
				
				/* increase the number of valid pages in the block */
				ptr_victim_block->nr_valid_pages++;
				ptr_victim_block->nr_free_pages--;

				/* make the page status valid */
				ptr_victim_block->list_pages[loop_page].page_status = 3;
			}
		}
	}

gc_exit:

	/* free the block buffer */
	if (ptr_block_buff != NULL) {
		free (ptr_block_buff);
	}

	return ret;
}
