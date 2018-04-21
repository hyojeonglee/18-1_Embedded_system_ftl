
/* This is just a sample blueftl_wearleveling_dualpool code for lab 2*/

//#include <linux/module.h> 
#include <err.h>
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
	printf("max_erase_cnt %d  min_erase_cnt %d\n",max_erase_cnt, min_erase_cnt);
	/* TODO: 위 부분은 dual pool 실행 전에 확인하는 코드. 어디에 쓸 수 있을까? */
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
	// TODO: 위 블록을 hot pool의 맨 뒤(왼쪽)으로 이동
}

/* 3. hot pool adjustment */
uint32_t check_hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
	if (g_max_rec_in_hot_pool.nr_erase_cnt - g_min_rec_in_hot_pool.nr_erase_cnt > 2 * WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

void hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context) {	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_min_ec_block;

	uint32_t no_bus = g_min_rec_in_hot_pool.no_bus;
	uint32_t no_chip = g_min_rec_in_hot_pool.no_chip;
	uint32_t no_block = g_min_rec_in_hot_pool.no_block;


	ptr_min_ec_block = &ptr_ssd->list_buses[no_bus].list_chips[no_chip].list_blocks[no_block];
	ptr_min_ec_block->hot_cold_pool = COLD_POOL;
	// TODO: 위 블록을 cold pool의 맨 뒤(왼쪽)으로 이동
}

/* 1. cold data migration */
uint32_t check_cold_data_migration(struct ftl_context_t *ptr_ftl_context){
	if (g_max_ec_in_hot_pool.nr_erase_cnt - g_min_ec_in_cold_pool.nr_erase_cnt > WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

/*
struct flash_block_t *get_min_max_ptr(struct ftl_context_t *ptr_ftl_context, dual_pool_block_info *pool_info){
}
*/

/*
struct flash_block_t *get_erase_blk_ptr(struct ftl_context_t *ptr_ftl_context, uint32_t target_bus, uint32_t target_chip, uint32_t target_block){
}
*/

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

	return 0;
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
   //ptr_erase_block->last_modified_time = timer_get_timestamp_in_us();

	for(i = 0; i < ptr_ssd->nr_pages_per_block; i++ ){
		ptr_erase_block->list_pages[i].no_logical_page_addr = -1;
		ptr_erase_block->list_pages[i].page_status = PAGE_STATUS_FREE;
	}

	return 0;
}

/* 1. cold data migration */
void cold_data_migration(struct ftl_context_t* ptr_ftl_context){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct virtual_device_t* ptr_vdevice = ptr_ftl_context->ptr_vdevice;
	struct flash_block_t* ptr_oldest_block_in_hot_pool;
	struct flash_block_t* ptr_youngest_block_in_cold_pool;

	ptr_oldest_block_in_hot_pool = &ptr_ssd->list_buses[g_max_ec_in_hot_pool.no_bus].list_chips[g_max_ec_in_hot_pool.no_chip].list_blocks[g_max_ec_in_hot_pool.no_block];
	ptr_youngest_block_in_cold_pool = &ptr_ssd->list_buses[g_min_ec_in_cold_pool.no_bus].list_chips[g_min_ec_in_cold_pool.no_chip].list_blocks[g_min_ec_in_cold_pool.no_block];
	
	/* New */
	// cold data to buff and clean youngest block
	// hot data to youngest block and clean oldest block
	// buff to oldest block
	// change status variables
	// and initialize EEC of new cold block (include hot data)
	uint8_t* ptr_page_buff = NULL;
	uint8_t* ptr_block_buff = NULL;
	uint8_t valid_page_check[ptr_ssd->nr_pages_per_block];
	int i;

	// cold data to buff and clean youngest block
	if ((ptr_block_buff = (uint8_t*)malloc (ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size)) == NULL) {
		printf("wl dual pool: the malloc for the buffer failed\n");
		return;
	}
	memset (ptr_block_buff, 0xFF, ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size);

	for (i = 0; i < ptr_ssd->nr_pages_per_block; i++) {
		if (ptr_youngest_block_in_cold_pool->list_pages[i].page_status == 3) {
			valid_page_check[i] = 1;
			ptr_page_buff = ptr_block_buff + (i * ptr_vdevice->page_main_size);

			blueftl_user_vdevice_page_read (
					_ptr_vdevice,
					0, 0, ptr_youngest_block_in_cold_pool->no_block, i,
					ptr_vdevice->page_main_size,
					(char*)ptr_page_buff);

			perf_wl_inc_page_copies ();
		}
		else {
			valid_page_check[i] = 0;
		}
	}
	page_clean_in_block(ptr_youngest_block_in_cold_pool, ptr_ftl_context);
	perf_wl_inc_blk_erasures ();

	// hot data to youngest block and clean oldest block
	block_copy(ptr_oldest_block_in_hot_pool, ptr_youngest_block_in_cold_pool, ptr_ftl_context);
	page_clean_in_block(ptr_oldest_block_in_hot_pool, ptr_ftl_context);
	perf_wl_inc_blk_erasures ();

	// buff to oldest block
	for (i = 0; i < ptr_ssd->nr_pages_per_block; i++) {
		if (valid_page_check[i] == 1) {
			ptr_page_buff = ptr_block_buff + (i * ptr_vdevice->page_main_size);

			blueftl_user_vdevice_page_write (
					ptr_vdevice,
					0, 0, ptr_oldest_block_in_hot_pool->no_block, i,
					ptr_vdevice->page_main_size,
					(char*)ptr_page_buff);
		}
		else {
			ptr_oldest_block_in_hot_pool->list_pages[i].page_status = 1;
			ptr_oldest_block_in_hot_pool->nr_free_pages++;
		}
	}
	// change status variables and initialize EEC of new cold block (include hot data)
	ptr_oldest_block_in_hot_pool->hot_cold_pool = COLD_POOL;
	ptr_oldest_block_in_hot_pool->nr_recent_erase_cnt = 0;
	ptr_youngest_block_in_cold_pool->hot_cold_pool = HOT_POOL;
}

void insert_pool(struct ftl_context_t* ptr_ftl_context, struct flash_block_t* ptr_erase_block){
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

