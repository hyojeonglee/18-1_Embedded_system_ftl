
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

unsigned char migration_buff[FLASH_PAGE_SIZE];
#if 0
uint32_t g_max_ec_in_hot_pool;
uint32_t g_max_ec_in_hot_no;
uint32_t g_min_ec_in_hot_pool;
uint32_t g_min_ec_in_hot_no;
uint32_t g_max_rec_in_hot_pool; /* unused ? */
uint32_t g_min_rec_in_hot_pool;
uint32_t g_min_rec_in_hot_no;

uint32_t g_max_ec_in_cold_pool; /* unused ? */
uint32_t g_min_ec_in_cold_pool;
uint32_t g_min_ec_in_cold_no;
uint32_t g_max_rec_in_cold_pool;
uint32_t g_max_rec_in_cold_no;
uint32_t g_min_rec_in_cold_pool; /* unused ? */
#endif

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
	/* TODO: 위 부분은 dual pool 실행 전에 확인하는 코드. 어디에 쓸 수 있을까? */
}

/* 2. cold pool adjustment */
uint32_t check_cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
#if 0
	find_max_rec_pool_block_info(ptr_ftl_context, COLD_POOL);
	find_min_rec_pool_block_info(ptr_ftl_context, HOT_POOL);
#endif
	if (g_max_rec_in_cold_pool.nr_erase_cnt - g_min_rec_in_hot_pool.nr_erase_cnt > WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

void cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_max_rec_block;

	ptr_max_rec_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[g_max_rec_in_cold_pool.no_block];
	// TODO: 위 블록을 hot pool의 맨 뒤(왼쪽)으로 이동
}

/* 3. hot pool adjustment */
uint32_t check_hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
#if 0
	find_max_ec_pool_block_info(ptr_ftl_context, HOT_POOL);
	find_min_ec_pool_block_info(ptr_ftl_context, HOT_POOL);
#endif
	if (g_max_ec_in_hot_pool.nr_erase_cnt - g_min_ec_in_hot_pool.nr_erase_cnt > 2 * WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
	
}

void hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context) {	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_min_ec_block;

	ptr_min_ec_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[g_min_ec_in_hot_pool.no_block];
	// TODO: 위 블록을 cold pool의 맨 뒤(왼쪽)으로 이동
}

/* 1. cold data migration */
uint32_t check_cold_data_migration(struct ftl_context_t *ptr_ftl_context){
#if 0
	find_max_ec_pool_block_info(ptr_ftl_context, HOT_POOL);
	find_min_ec_pool_block_info(ptr_ftl_context, COLD_POOL);
#endif

	if (g_max_ec_in_hot_pool.nr_erase_cnt - g_min_ec_in_cold_pool.nr_erase_cnt > WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

/* TODO: how to control dual_pool_block_info struct ? */
struct flash_block_t *get_min_max_ptr(struct ftl_context_t *ptr_ftl_context, dual_pool_block_info *pool_info){
}

struct flash_block_t *get_erase_blk_ptr(struct ftl_context_t *ptr_ftl_context, uint32_t target_bus, uint32_t target_chip, uint32_t target_block){
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
	struct vitrual_device_t* ptr_vdevice = ptr_ftl_context->ptr_vdevice;
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
}

/* 1. cold data migration */
void cold_data_migration(struct ftl_context_t* ptr_ftl_context){
	/* TODO: it is so waste */
//	uint32_t oldest_in_hot_pool = g_max_ec_in_hot_pool.no_block;
//	uint32_t youngest_in_cold_pool = g_min_ec_in_cold_pool.no_block;
//	uint32_t target;

	// copy valid pages in oldest to other block (where?)
	// clear oldest
	// copy valid pages in youngest to cleared block
	// clear youngest
	// change status variables (swap)
	// and initialize EEC of new cold block (include hot data)
	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_oldest_block_in_hot_pool;
	struct flash_block_t* ptr_youngest_block_in_cold_pool;
	struct flash_block_t* ptr_tgt_block; /* TODO: how to choose ? */

	ptr_oldest_block_in_hot_pool = &ptr_ssd->list_buses[g_max_ec_in_hot_pool.no_bus].list_chips[g_max_ec_in_hot_pool.no_chip].list_blocks[g_max_ec_in_hot_pool.no_block];
	ptr_youngest_block_in_cold_pool = &ptr_ssd->list_buses[g_min_ec_in_cold_pool.no_bus].list_chips[g_min_ec_in_cold_pool.no_chip].list_blocks[g_min_ec_in_cold_pool.no_block];
//	ptr_tgt_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[target]; /* TODO: how to choose target block for oldest data */
	
	block_copy(ptr_oldest_block_in_hot_pool, ptr_tgt_block, ptr_ftl_context);
	page_clean_in_block(ptr_oldest_block_in_hot_pool, ptr_ftl_context);
	block_copy(ptr_youngest_block_in_cold_pool, ptr_oldest_block_in_hot_pool, ptr_ftl_context);
	page_clean_in_block(ptr_youngest_block_in_cold_pool, ptr_ftl_context);
	// change status variables (swap)
	// and initialize EEC of new cold block (include hot data)
}

void insert_pool(struct ftl_context_t* ptr_ftl_context, struct flash_block_t* ptr_erase_block){
}

#if 0
uint32_t find_max_ec_pool_block_info(struct ftl_context_t* ptr_ftl_context, uint32_t pool){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_erase_block;
	int i;
	uint32_t max_ec_hot = 0;
	uint32_t max_ec_cold = 0;
	uint32_t max_ec_hot_block_no;
	uint32_t max_ec_cold_block_no;

	for (i = 0; i < ptr_ssd->nr_blocks_per_chip; i++) {
		ptr_erase_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[i];
		uint32_t curr_cnt = ptr_erase_block->nr_erase_cnt;
		if (ptr_erase_block->hot_cold_pool == HOT_POOL) {
			if (max_ec_hot < curr_cnt) {
				max_ec_hot = curr_cnt;
				max_ec_hot_block_no = ptr_erase_block->no_block;
			}
		}
		else if (ptr_erase_block->hot_cold_pool == COLD_POOL) {
			if (max_ec_cold < curr_cnt) {
				max_ec_cold = curr_cnt;
				max_ec_cold_block_no = ptr_erase_block->no_block;
			}
		}
		else {
			err(2, "NO SETTING for HOT or COLD POOL!");
		}
	}
	
	/* set global variable */
	g_max_ec_in_hot_pool = max_ec_hot;
	g_max_ec_in_cold_pool = max_ec_cold;
	
	if (pool == HOT_POOL) {
		/* find value and return block index */
		g_max_ec_in_hot_no = max_ec_hot_block_no;
		return max_ec_hot_block_no;
	}
	else if (pool == COLD_POOL) {
		/* find value and return block index */
		return max_ec_cold_block_no;
	}
}

uint32_t find_min_ec_pool_block_info(struct ftl_context_t* ptr_ftl_context, uint32_t pool){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_erase_block;
	int i;
	uint32_t min_ec_hot = 99999;
	uint32_t min_ec_cold = 99999;
	uint32_t min_ec_hot_block_no;
	uint32_t min_ec_cold_block_no;

	for (i = 0; i < ptr_ssd->nr_blocks_per_chip; i++) {
		ptr_erase_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[i];
		uint32_t curr_cnt = ptr_erase_block->nr_erase_cnt;
		if (ptr_erase_block->hot_cold_pool == HOT_POOL) {
			if (min_ec_hot < curr_cnt) {
				min_ec_hot = curr_cnt;
				min_ec_hot_block_no = ptr_erase_block->no_block;
			}
		}
		else if (ptr_erase_block->hot_cold_pool == COLD_POOL) {
			if (min_ec_cold < curr_cnt) {
				min_ec_cold = curr_cnt;
				min_ec_cold_block_no = ptr_erase_block->no_block;
			}
		}
		else {
			err(2, "NO SETTING for HOT or COLD POOL!");
		}
	}
	
	/* set global variable */
	g_min_ec_in_hot_pool = min_ec_hot;
	g_min_ec_in_cold_pool = min_ec_cold;
	
	if (pool == HOT_POOL) {
		/* find value and return block index */
		g_min_ec_in_hot_no = min_ec_hot_block_no;
		return min_ec_hot_block_no;
	}
	else if (pool == COLD_POOL) {
		/* find value and return block index */
		g_min_ec_in_cold_no = min_ec_cold_block_no;
		return min_ec_cold_block_no;
	}
}

uint32_t find_max_rec_pool_block_info(struct ftl_context_t* ptr_ftl_context, uint32_t pool){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_erase_block;
	int i;
	uint32_t max_rec_hot = 0;
	uint32_t max_rec_cold = 0;
	uint32_t max_rec_hot_block_no;
	uint32_t max_rec_cold_block_no;

	for (i = 0; i < ptr_ssd->nr_blocks_per_chip; i++) {
		ptr_erase_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[i];
		uint32_t curr_cnt = ptr_erase_block->nr_recent_erase_cnt;
		if (ptr_erase_block->hot_cold_pool == HOT_POOL) {
			if (max_rec_hot < curr_cnt) {
				max_rec_hot = curr_cnt;
				max_rec_hot_block_no = ptr_erase_block->no_block;
			}
		}
		else if (ptr_erase_block->hot_cold_pool == COLD_POOL) {
			if (max_rec_cold < curr_cnt) {
				max_rec_cold = curr_cnt;
				max_rec_cold_block_no = ptr_erase_block->no_block;
			}
		}
		else {
			err(2, "NO SETTING for HOT or COLD POOL!");
		}
	}
	
	/* set global variable */
	g_max_rec_in_hot_pool = max_rec_hot;
	g_max_rec_in_cold_pool = max_rec_cold;
	
	if (pool == HOT_POOL) {
		/* find value and return block index */
		return max_rec_hot_block_no;
	}
	else if (pool == COLD_POOL) {
		/* find value and return block index */
		g_max_rec_in_cold_no = max_rec_cold_block_no;
		return max_rec_cold_block_no;
	}
}

uint32_t find_min_rec_pool_block_info(struct ftl_context_t* ptr_ftl_context, uint32_t pool){	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_erase_block;
	int i;
	uint32_t min_rec_hot = 99999;
	uint32_t min_rec_cold = 99999;
	uint32_t min_rec_hot_block_no;
	uint32_t min_rec_cold_block_no;

	for (i = 0; i < ptr_ssd->nr_blocks_per_chip; i++) {
		ptr_erase_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[i];
		uint32_t curr_cnt = ptr_erase_block->nr_recent_erase_cnt;
		if (ptr_erase_block->hot_cold_pool == HOT_POOL) {
			if (min_rec_hot < curr_cnt) {
				min_rec_hot = curr_cnt;
				min_rec_hot_block_no = ptr_erase_block->no_block;
			}
		}
		else if (ptr_erase_block->hot_cold_pool == COLD_POOL) {
			if (min_rec_cold < curr_cnt) {
				min_rec_cold = curr_cnt;
				min_rec_cold_block_no = ptr_erase_block->no_block;
			}
		}
		else {
			err(2, "NO SETTING for HOT or COLD POOL!");
		}
	}
	
	/* set global variable */
	g_min_rec_in_hot_pool = min_rec_hot;
	g_min_rec_in_cold_pool = min_rec_cold;
	
	if (pool == HOT_POOL) {
		/* find value and return block index */
		return min_rec_hot_block_no;
	}
	else if (pool == COLD_POOL) {
		/* find value and return block index */
		return min_rec_cold_block_no;
	}
}
#endif

uint32_t update_max_min_nr_erase_cnt_in_pool( int pool, int type, int min_max, int bus, int chip, int block, uint32_t nr_erase_cnt){
	return 0;
}

void init_global_wear_leveling_metadata(void){
}

