#ifdef KERNEL_MODE

/* ADD */
#include <time.h>
#include <limits.h>
#include <linux/vmalloc.h>

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#endif

#include "blueftl_ftl_base.h"
#include "blueftl_mapping_page.h"
#include "blueftl_gc_page.h"
#include "blueftl_util.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_wl_dual_pool.h"  // for wear-leveling 


void gc_page_trigger_init(struct ftl_context_t* ptr_ftl_context){
		
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;

	if(ptr_ssd->list_buses[0].list_chips[0].nr_free_blocks > 0){
		((struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping)->reserved = ssdmgmt_get_free_block(ptr_ssd, 0, 0);

		ptr_ssd->list_buses[0].list_chips[0].nr_free_blocks--;
		ptr_ssd->list_buses[0].list_chips[0].nr_dirty_blocks++;
	}
		((struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping)->reserved->is_reserved_block = 1;
}


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

	/* TODO: Greedy Policy */
	
	uint32_t k;
	struct flash_block_t* ptr_erase_block;
	uint32_t min_valid_pg = 65;
	uint32_t tmp_target_block;

	for (k = 0; k < ptr_ssd->nr_blocks_per_chip; k++) {
		ptr_erase_block = &ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[k];
		uint32_t tmp_valid_pg = ptr_erase_block->nr_valid_pages;
		if (tmp_valid_pg < min_valid_pg && ptr_erase_block->nr_free_pages != ptr_ssd->nr_pages_per_block) {
			min_valid_pg = tmp_valid_pg;
			tmp_target_block = k;
		}
	}
	

	/* TODO: Random Policy */


	/* TODO: cost benefit */


	/* Choose victim ! */
	ptr_victim_block = &ptr_ssd->list_buses[gc_target_bus].list_chips[gc_target_chip].list_blocks[tmp_target_block];

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
    /* call the wear-leveling function */


/* If you want execute wear-leving */

#if WEAR_LEVELING
 			update_max_min_nr_erase_cnt_in_pool(ptr_ftl_context);
 			cold_data_migration(ptr_ftl_context);
 			cold_pool_adjustment(ptr_ftl_context);
 			hot_pool_adjustment(ptr_ftl_context);
#endif

	return ret;
}

void block_copy(struct ftl_context_t* context, struct flash_block_t* src_block, struct flash_block_t* dest_block){
	
	/*
	 * move the data from source block to buffer
	 * copy the data from buffer to dest
	 * update the mapping table
	 */

	struct flash_ssd_t* ptr_ssd = context->ptr_ssd;
	char* buff = (char*)malloc(context->ptr_vdevice->page_main_size * ptr_ssd->nr_pages_per_block);
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)context->ptr_mapping;
	
	uint32_t i, j;
	uint32_t count = 0;


	for(i = 0, j = 0; i < ptr_ssd->nr_pages_per_block; i++){
		if(src_block->list_pages[i].page_status == PAGE_STATUS_INVALID || 
		   src_block->list_pages[i].page_status == PAGE_STATUS_FREE){
			continue;
		} /* To get source valid pages */

		while(dest_block->list_pages[j].page_status != PAGE_STATUS_FREE){j++;}

		blueftl_user_vdevice_page_read(
				context->ptr_vdevice,
				src_block->no_bus,
				src_block->no_chip,
				src_block->no_block,
				i,
				context->ptr_vdevice->page_main_size,
				buff);

		blueftl_user_vdevice_page_write(
				context->ptr_vdevice,
				dest_block->no_bus,
				dest_block->no_chip,
				dest_block->no_block,
				j,
				context->ptr_vdevice->page_main_size,
				buff);

		dest_block->list_pages[j].page_status = PAGE_STATUS_VALID;
		dest_block->nr_free_pages--;
		dest_block->nr_valid_pages++;
		dest_block->list_pages[j].no_logical_page_addr = src_block->list_pages[i].no_logical_page_addr;
		
		/* update the page table information */
		ptr_pg_mapping->ptr_pg_table[src_block->list_pages[i].no_logical_page_addr] = 
			ftl_convert_to_physical_page_address(dest_block->no_bus, dest_block->no_chip, dest_block->no_block, j);

		count++;

	}
	free(buff);
}
