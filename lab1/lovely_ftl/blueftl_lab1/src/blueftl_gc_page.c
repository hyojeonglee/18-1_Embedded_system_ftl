#ifdef KERNEL_MODE

#include <linux/vmalloc.h>
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_page.h"
#include "blueftl_gc_page.h"
#include "blueftl_util.h"

#include <time.h> // for uisng time 

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blueftl_ftl_base.h"
#include "blueftl_mapping_page.h"
#include "blueftl_gc_page.h"
#include "blueftl_util.h"
#include "blueftl_user_vdevice.h"

#include <time.h> // for uisng time 

#endif

int32_t gc_page_trigger_gc_lab (
		struct ftl_context_t* ptr_ftl_context,
		uint32_t gc_target_bus,
		uint32_t gc_target_chip)
{
	/* TODO: choose gc policy by option random/greedy/costbenefit that in blueftl_ftl_base.h */
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct virtual_device_t* ptr_vdevice = ptr_ftl_context->ptr_vdevice;
		
	struct flash_block_t* ptr_victim_block = NULL;

	uint8_t* ptr_page_buff = NULL;
	uint8_t* ptr_block_buff = NULL;
	uint32_t loop_page = 0;

	int32_t ret = 0;

/*
	//TODO: Greedy Policy 
	uint32_t k;
	struct flash_block_t* ptr_erase_block;
	uint32_t min_valid_pg = 65;
	uint32_t tmp_target_block;

	for (k = 0; k < ptr_ssd->nr_blocks_per_chip; k++) {
		ptr_erase_block = &ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[k];
		// Point 
		uint32_t tmp_valid_pg = ptr_erase_block->nr_valid_pages;
		// TODO: is this right to use below conditions ? 
		if (tmp_valid_pg < min_valid_pg && ptr_erase_block->nr_free_pages != ptr_ssd->nr_pages_per_block) {
			min_valid_pg = tmp_valid_pg;
			tmp_target_block = k;

		}
	}
*/

	/* TODO: RANDOM POLICY */

	struct flash_block_t* ptr_erase_block;
	uint32_t tmp_target_block;
	uint32_t num_random;
	 
	srand((unsigned)time(NULL));

	while(1) {
		num_random = rand() % (ptr_ssd->nr_blocks_per_chip);

	    ptr_erase_block = &ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[num_random];

		if(ptr_erase_block->is_reserved_block == IS_RESERVED_FOR_GC && ptr_erase_block->nr_valid_pages != 64) {
			
			tmp_target_block = num_random;
			break;
																	 }
	}

	ptr_victim_block = &ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[tmp_target_block];

			printf("gc line 85 : %u,%u,%u,%u \n ",tmp_target_block, ptr_victim_block->nr_invalid_pages, ptr_victim_block->nr_valid_pages, ptr_victim_block->nr_free_pages);

	/* TODO: initialize buffer */
	if ((ptr_block_buff = (uint8_t*)malloc (ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size)) == NULL) {
		printf("blueftl_gc_page: the malloc for the buffer failed\n");
		return -1;
	}
	memset (ptr_block_buff, 0xFF, ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size);

	/* step 1. read all the valid pages from the target block */
	for (loop_page = 0; loop_page < ptr_ssd->nr_pages_per_block; loop_page++) {
		if (ptr_victim_block->list_pages[loop_page].page_status == 3) {
			ptr_page_buff = ptr_block_buff + (loop_page * ptr_vdevice->page_main_size);

			blueftl_user_vdevice_page_read (
					_ptr_vdevice,
					gc_target_bus, gc_target_chip, tmp_target_block, loop_page,
					ptr_vdevice->page_main_size,
					(char*)ptr_page_buff);

			perf_gc_inc_page_copies ();
		}
	}
	
	/* step 2. free victim block */
	blueftl_user_vdevice_block_erase (ptr_vdevice, gc_target_bus, gc_target_chip, tmp_target_block);
	ptr_victim_block->nr_erase_cnt++;	
	perf_gc_inc_blk_erasures ();

	ptr_victim_block->nr_invalid_pages = 0;
	ptr_victim_block->nr_free_pages = 0;
	ptr_victim_block->is_reserved_block = 0;

	/* step 3. write valid page to free block and update page table */
	for (loop_page = 0; loop_page < ptr_ssd->nr_pages_per_block; loop_page++) {
		if (ptr_victim_block->list_pages[loop_page].page_status == 3) {
			ptr_page_buff = ptr_block_buff + (loop_page * ptr_vdevice->page_main_size);

			blueftl_user_vdevice_page_write (
					ptr_vdevice,
					gc_target_bus, gc_target_chip, tmp_target_block, loop_page,
					ptr_vdevice->page_main_size,
					(char*)ptr_page_buff);
			
		} else {
			ptr_victim_block->list_pages[loop_page].page_status = 1;
			ptr_victim_block->nr_free_pages++;

		}
	}

			printf("gc line 136 : %u,%u,%u,%u \n ",tmp_target_block, ptr_victim_block->nr_invalid_pages, ptr_victim_block->nr_valid_pages, ptr_victim_block->nr_free_pages);

	return ret;
}

