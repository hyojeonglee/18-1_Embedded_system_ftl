#include "blueftl_ftl_base.h"
#include "blueftl_user_ftl_main.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_page.h"
#include "blueftl_util.h"
#include "blueftl_wl_dual_pool.h" 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

unsigned char migration_buff[FLASH_PAGE_SIZE];

dual_pool_block_info g_max_ec_in_hot_pool;
dual_pool_block_info g_min_ec_in_hot_pool;
dual_pool_block_info g_max_rec_in_hot_pool;
dual_pool_block_info g_min_rec_in_hot_pool;


dual_pool_block_info g_max_ec_in_cold_pool;
dual_pool_block_info g_min_ec_in_cold_pool;
dual_pool_block_info g_max_rec_in_cold_pool;
dual_pool_block_info g_min_rec_in_cold_pool;

void check_max_min_nr_erase_cnt(struct ftl_context_t *ptr_ftl_context){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_erase_block;
	int i;
	uint32_t max_erase_cnt = 0;
	uint32_t min_erase_cnt = 99999;

	for (i = 0; i < ptr_ssd->nr_blocks_per_chip; i++) {
		ptr_erase_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[i]; /* size of bus and chip: 1 */
		uint32_t curr_cnt = ptr_erase_block->nr_erase_cnt;

		if (max_erase_cnt < curr_cnt)
			max_erase_cnt = curr_cnt;
		if (min_erase_cnt > curr_cnt)
			min_erase_cnt = curr_cnt;
	}

	ptr_ftl_context->max_erase_cnt = max_erase_cnt;
	ptr_ftl_context->min_erase_cnt = min_erase_cnt;
	perf_wl_set_blk_max_erasures(max_erase_cnt);
	perf_wl_log_blk_erasures(ptr_ftl_context);
}

/* 2. cold pool adjustment */
uint32_t check_cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
	if (g_max_rec_in_cold_pool.nr_erase_cnt - g_min_rec_in_hot_pool.nr_erase_cnt > WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

void cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_max_rec_block;

	uint32_t no_bus = g_min_rec_in_cold_pool.no_bus;
	uint32_t no_chip = g_min_rec_in_cold_pool.no_chip;
	uint32_t no_block = g_min_rec_in_cold_pool.no_block;

	ptr_max_rec_block = &ptr_ssd->list_buses[no_bus].list_chips[no_chip].list_blocks[no_block];

	ptr_max_rec_block->hot_cold_pool = HOT_POOL;
}

/* 3. hot pool adjustment */
uint32_t check_hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
	if (g_max_ec_in_hot_pool.nr_erase_cnt - g_min_ec_in_hot_pool.nr_erase_cnt > 2 * WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

void hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context) {	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_min_ec_block;

	uint32_t no_bus = g_min_ec_in_hot_pool.no_bus;
	uint32_t no_chip = g_min_ec_in_hot_pool.no_chip;
	uint32_t no_block = g_min_ec_in_hot_pool.no_block;


	ptr_min_ec_block = &ptr_ssd->list_buses[no_bus].list_chips[no_chip].list_blocks[no_block];
	ptr_min_ec_block->hot_cold_pool = COLD_POOL;
}

/* 1. cold data migration */
uint32_t check_cold_data_migration(struct ftl_context_t *ptr_ftl_context){
	if (g_max_ec_in_hot_pool.nr_erase_cnt - g_min_ec_in_cold_pool.nr_erase_cnt > WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

uint32_t block_swap(struct flash_block_t *src_block, struct flash_block_t *dest_block, struct ftl_context_t *ptr_ftl_context)
{
/*
 *	swap the valid pages in hottest block and coldest block
 * */
	uint32_t loop_page = 0;
	uint32_t src_valid_page_cnt = 0;
	uint32_t dest_valid_page_cnt = 0;

	struct flash_ssd_t *ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct virtual_device_t* ptr_vdevice = ptr_ftl_context->ptr_vdevice;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	int32_t src_logical_page_addr[1024]={-1,};
	int32_t dest_logical_page_addr[1024]={-1,};

	char *src_block_buff = NULL;
	char *src_page_buff = NULL;
	char *dest_block_buff = NULL;
	char *dest_page_buff = NULL;

	if((src_block_buff = (char *)malloc(ptr_ftl_context->ptr_vdevice->page_main_size * ptr_ssd->nr_pages_per_block))== NULL) {
		printf("blueftl_wl_dual_pool: the malloc for the buffer failed\n");
		return -1;
	}

	memset(src_block_buff, 0xFF, ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size);

	if((dest_block_buff = (char *)malloc(ptr_ftl_context->ptr_vdevice->page_main_size * ptr_ssd->nr_pages_per_block))== NULL) {
		printf("blueftl_wl_dual_pool: the malloc for the buffer failed\n");
		return -1;
	}

	memset(dest_block_buff, 0xFF, ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size);

	/*step 1. read all the vailid page from src block*/
	for(loop_page = 0; loop_page < ptr_ssd->nr_pages_per_block; loop_page++) {
		if(src_block->list_pages[loop_page].page_status == PAGE_STATUS_VALID) {
			src_page_buff = src_block_buff + (loop_page * ptr_vdevice->page_main_size);
			blueftl_user_vdevice_page_read(
				ptr_ftl_context->ptr_vdevice,
				src_block->no_bus,
				src_block->no_chip,
				src_block->no_block,
				loop_page,
				ptr_ftl_context->ptr_vdevice->page_main_size,
				(char *)src_page_buff);
			perf_wl_inc_page_copies();
			src_logical_page_addr[src_valid_page_cnt]=src_block->list_pages[loop_page].no_logical_page_addr;
			src_valid_page_cnt++;
		}
	}

	/*step 2. free srouce block*/
	page_clean_in_block(src_block, ptr_ftl_context);
	perf_wl_inc_blk_erasures();

	/*step 3. read valid pages in dest block to another buffer*/

	for(loop_page = 0; loop_page < ptr_ssd->nr_pages_per_block; loop_page++) {
		if(dest_block->list_pages[loop_page].page_status == PAGE_STATUS_VALID) {
			dest_page_buff = dest_block_buff + (loop_page * ptr_vdevice->page_main_size);
			blueftl_user_vdevice_page_read(
				ptr_ftl_context->ptr_vdevice,
				dest_block->no_bus,
				dest_block->no_chip,
				dest_block->no_block,
				loop_page,
				ptr_ftl_context->ptr_vdevice->page_main_size,
				(char *)dest_page_buff);
			dest_logical_page_addr[dest_valid_page_cnt]=dest_block->list_pages[loop_page].no_logical_page_addr;
			dest_valid_page_cnt++;
			perf_wl_inc_page_copies();
		}
	}


	/*step 4. free dest block*/
	page_clean_in_block(dest_block, ptr_ftl_context);
	perf_wl_inc_blk_erasures();

	/*step 5 write contents in src_block_buffer to dest block*/
	for(loop_page = 0; loop_page < src_valid_page_cnt; loop_page++) {
			blueftl_user_vdevice_page_write (
					ptr_vdevice,
					dest_block->no_bus,
					dest_block->no_chip,
					dest_block->no_block,
					loop_page,
					ptr_ftl_context->ptr_vdevice->page_main_size,
					(char *)src_page_buff);
			dest_block->list_pages[loop_page].page_status = PAGE_STATUS_VALID;
			dest_block->nr_free_pages--;
			dest_block->nr_valid_pages++;
			ptr_pg_mapping->ptr_pg_table[src_logical_page_addr[loop_page]]= ftl_convert_to_physical_page_address(dest_block->no_bus, dest_block->no_chip, dest_block->no_block, loop_page);
	}

	/*step 6 write contents in dest_block_buffer to src block*/
	for(loop_page = 0; loop_page < dest_valid_page_cnt; loop_page++) {
			blueftl_user_vdevice_page_write (
					ptr_vdevice,
					src_block->no_bus,
					src_block->no_chip,
					src_block->no_block,
					loop_page,
					ptr_ftl_context->ptr_vdevice->page_main_size,
					(char *)dest_page_buff);
			src_block->list_pages[loop_page].page_status = PAGE_STATUS_VALID;
			src_block->nr_free_pages--;
			src_block->nr_valid_pages++;
			ptr_pg_mapping->ptr_pg_table[dest_logical_page_addr[loop_page]]= ftl_convert_to_physical_page_address(src_block->no_bus, src_block->no_chip, src_block->no_block, loop_page);
	}
}

uint32_t block_copy(struct flash_block_t *src_block, struct flash_block_t *dest_block, struct ftl_context_t *ptr_ftl_context){
/*
	 * move the data from source block to buffer
	 * copy the data from buffer to dest
	 * update the mapping table
	 */

	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	char* buff = (char*)malloc(ptr_ftl_context->ptr_vdevice->page_main_size * ptr_ssd->nr_pages_per_block);
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	
	uint32_t i, j;
	uint32_t count = 0;


	for(i = 0, j = 0; i < ptr_ssd->nr_pages_per_block; i++){
		if(src_block->list_pages[i].page_status == PAGE_STATUS_INVALID || 
		   src_block->list_pages[i].page_status == PAGE_STATUS_FREE){
			continue;
		} /* To get source valid pages */

		while(dest_block->list_pages[j].page_status != PAGE_STATUS_FREE){j++;}

		blueftl_user_vdevice_page_read(
				ptr_ftl_context->ptr_vdevice,
				src_block->no_bus,
				src_block->no_chip,
				src_block->no_block,
				i,
				ptr_ftl_context->ptr_vdevice->page_main_size,
				buff);

		blueftl_user_vdevice_page_write(
				ptr_ftl_context->ptr_vdevice,
				dest_block->no_bus,
				dest_block->no_chip,
				dest_block->no_block,
				j,
				ptr_ftl_context->ptr_vdevice->page_main_size,
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

uint32_t page_clean_in_block(struct flash_block_t *ptr_erase_block,  struct ftl_context_t *ptr_ftl_context){
	struct virtual_device_t* ptr_vdevice = ptr_ftl_context->ptr_vdevice;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;

	uint32_t i;

	blueftl_user_vdevice_block_erase(ptr_vdevice, ptr_erase_block->no_bus, ptr_erase_block->no_chip, ptr_erase_block->no_block);

	ptr_erase_block->nr_erase_cnt++;
	ptr_erase_block->nr_recent_erase_cnt++;

	ptr_erase_block->nr_valid_pages = 0;
	ptr_erase_block->nr_invalid_pages = 0;
	ptr_erase_block->nr_free_pages = ptr_ssd->nr_pages_per_block;

	for(i = 0; i < ptr_ssd->nr_pages_per_block; i++ ){
		ptr_erase_block->list_pages[i].no_logical_page_addr = -1;
		ptr_erase_block->list_pages[i].page_status = PAGE_STATUS_FREE;
	}
}

/* 1. cold data migration */
void cold_data_migration(struct ftl_context_t* ptr_ftl_context){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_oldest_block_in_hot_pool;
	struct flash_block_t* ptr_youngest_block_in_cold_pool;

	ptr_oldest_block_in_hot_pool = &ptr_ssd->list_buses[g_max_ec_in_hot_pool.no_bus].list_chips[g_max_ec_in_hot_pool.no_chip].list_blocks[g_max_ec_in_hot_pool.no_block];
	ptr_youngest_block_in_cold_pool = &ptr_ssd->list_buses[g_min_ec_in_cold_pool.no_bus].list_chips[g_min_ec_in_cold_pool.no_chip].list_blocks[g_min_ec_in_cold_pool.no_block];

	//swap valid pages of each block to another
	block_swap(ptr_oldest_block_in_hot_pool, ptr_youngest_block_in_cold_pool, ptr_ftl_context);
	
	ptr_oldest_block_in_hot_pool->hot_cold_pool = 0; //move to cold pool
	ptr_youngest_block_in_cold_pool->hot_cold_pool = 1;// move to hot pool
	ptr_oldest_block_in_hot_pool->nr_recent_erase_cnt = 0;//rest eec to be 0
}

void update_erase_cnt_in_each_pool(dual_pool_block_info *info, struct flash_block_t *ptr_target_block) {
	if(ptr_target_block == NULL){
		info->no_bus = 0;
		info->no_chip = 0;
		info->no_block = 0;
		info->nr_erase_cnt = 0;
	}else {
		info->no_bus = ptr_target_block->no_bus;
		info->no_chip = ptr_target_block->no_chip;
		info->no_block = ptr_target_block->no_block;
		info->nr_erase_cnt = ptr_target_block->nr_erase_cnt;
	}
}

uint32_t update_max_min_nr_erase_cnt_in_pool( struct ftl_context_t *ptr_ftl_context){
	struct flash_ssd_t *ptr_target_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t *ptr_target_block = NULL;

	uint32_t nr_bus = ptr_target_ssd->nr_buses;
	uint32_t nr_chip = ptr_target_ssd->nr_chips_per_bus;
	uint32_t nr_block = ptr_target_ssd->nr_blocks_per_chip;

	uint32_t i, j, k;

	struct flash_block_t *max_ec_in_hot = NULL;
	struct flash_block_t *min_ec_in_hot = NULL;
	struct flash_block_t *max_rec_in_hot = NULL;
	struct flash_block_t *min_rec_in_hot = NULL;

	struct flash_block_t *max_ec_in_cold = NULL;
	struct flash_block_t *min_ec_in_cold = NULL;
	struct flash_block_t *max_rec_in_cold = NULL;
	struct flash_block_t *min_rec_in_cold = NULL;

	uint32_t max_ec_value_in_hot = 0;
	uint32_t min_ec_value_in_hot = 99999;
	uint32_t max_rec_value_in_hot = 0;
	uint32_t min_rec_value_in_hot = 99999;

	uint32_t max_ec_value_in_cold = 0;
	uint32_t min_ec_value_in_cold = 99999;
	uint32_t max_rec_value_in_cold = 0;
	uint32_t min_rec_value_in_cold = 99999;

	for(i = 0; i < nr_bus; i++) {
		for(j = 0; j < nr_chip; j++) {
			for(k = 0; k < nr_block; k++) {
				ptr_target_block = &(ptr_target_ssd->list_buses[i].list_chips[j].list_blocks[k]);
				if(ptr_target_block->hot_cold_pool == HOT_POOL) {
					if(max_ec_value_in_hot < ptr_target_block->nr_erase_cnt){
						max_ec_value_in_hot = ptr_target_block->nr_erase_cnt;
						max_ec_in_hot = ptr_target_block;
					}	
					if(max_rec_value_in_hot < ptr_target_block->nr_erase_cnt){
						max_rec_value_in_hot = ptr_target_block->nr_erase_cnt;
						max_rec_in_hot = ptr_target_block;
					}
					if(min_ec_value_in_hot >= ptr_target_block->nr_erase_cnt) {
						min_ec_value_in_hot = ptr_target_block->nr_erase_cnt;
						min_ec_in_hot = ptr_target_block;
					}
					if(min_rec_value_in_hot >= ptr_target_block->nr_erase_cnt){
						min_rec_value_in_hot = ptr_target_block->nr_erase_cnt;
						min_rec_in_hot = ptr_target_block;
					}

				} else if(ptr_target_block->hot_cold_pool == COLD_POOL){
					if(max_ec_value_in_cold < ptr_target_block->nr_erase_cnt){
						max_ec_value_in_cold = ptr_target_block->nr_erase_cnt;
						max_ec_in_cold = ptr_target_block;
				//		printf("max_ec_count %d at block %d\n", max_ec_value_in_cold, ptr_target_block->no_block);
					}	
					if(max_rec_value_in_cold < ptr_target_block->nr_erase_cnt){
						max_rec_value_in_cold = ptr_target_block->nr_erase_cnt;
						max_rec_in_cold = ptr_target_block;
				//		printf("max_rec_count %d at block %d\n", max_rec_value_in_cold, ptr_target_block->no_block);
					}
					if(min_ec_value_in_cold >= ptr_target_block->nr_erase_cnt) {
						min_ec_value_in_cold = ptr_target_block->nr_erase_cnt;
						min_ec_in_cold = ptr_target_block;
				//		printf("min_ec_count %d at block %d\n", min_ec_value_in_cold, ptr_target_block->no_block);
					}
					if(min_rec_value_in_cold >= ptr_target_block->nr_erase_cnt){
						min_rec_value_in_cold = ptr_target_block->nr_erase_cnt;
						min_rec_in_cold = ptr_target_block;
				//		printf("min_rec_count %d at block %d\n", min_rec_value_in_cold, ptr_target_block->no_block);
					}
				}else {
					printf("error ");
					return 1;
				}
			} //end block for
		}//end chip for
	}// end bus for

	update_erase_cnt_in_each_pool(&g_max_ec_in_hot_pool, max_ec_in_hot);
	update_erase_cnt_in_each_pool(&g_max_rec_in_hot_pool, max_rec_in_hot);
	update_erase_cnt_in_each_pool(&g_min_ec_in_hot_pool, min_ec_in_hot);
	update_erase_cnt_in_each_pool(&g_min_rec_in_hot_pool, min_rec_in_hot);

	update_erase_cnt_in_each_pool(&g_max_ec_in_cold_pool, max_ec_in_cold);
	update_erase_cnt_in_each_pool(&g_max_rec_in_cold_pool, max_rec_in_cold);
	update_erase_cnt_in_each_pool(&g_min_ec_in_cold_pool, min_ec_in_cold);
	update_erase_cnt_in_each_pool(&g_min_rec_in_cold_pool, min_rec_in_cold);

	printf("g_max_ec_in_cold_pool %d, erase_cnt %d\n", g_max_ec_in_cold_pool.no_block, g_max_ec_in_cold_pool.nr_erase_cnt);
	printf("g_max_rec_in_cold_pool %d, erase_cnt %d\n", g_max_rec_in_cold_pool.no_block, g_max_rec_in_cold_pool.nr_erase_cnt);
	printf("g_min_ec_in_cold_pool %d, erase_cnt %d\n", g_min_ec_in_cold_pool.no_block, g_min_ec_in_cold_pool.nr_erase_cnt);
	printf("g_min_rec_in_cold_pool %d, erase_cnt %d\n", g_min_rec_in_cold_pool.no_block, g_min_rec_in_cold_pool.nr_erase_cnt);

	
	printf("g_max_ec_in_hot_pool %d, erase_cnt %d\n", g_max_ec_in_hot_pool.no_block, g_max_ec_in_hot_pool.nr_erase_cnt);
	printf("g_max_rec_in_hot_pool %d, erase_cnt %d\n", g_max_rec_in_hot_pool.no_block, g_max_rec_in_hot_pool.nr_erase_cnt);
	printf("g_min_ec_in_hot_pool %d, erase_cnt %d\n", g_min_ec_in_hot_pool.no_block, g_min_ec_in_hot_pool.nr_erase_cnt);
	printf("g_min_rec_in_hot_pool %d, erase_cnt %d\n", g_min_rec_in_hot_pool.no_block, g_min_rec_in_hot_pool.nr_erase_cnt);

	return 0;
}

void init_global_wear_leveling_metadata(void){
}

