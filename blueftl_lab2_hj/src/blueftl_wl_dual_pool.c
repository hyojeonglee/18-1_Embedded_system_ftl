
/* This is just a sample blueftl_wearleveling_dualpool code for lab 2*/

#include <linux/module.h> 
#include <err.h>
#include "blueftl_ftl_base.h"
#include "blueftl_user_ftl_main.h"
#include "blueftl_ssdmgmt.h"
#include "bluessd_user_vdevice.h"
#include "blueftl_mapping_page.h"
#include "blueftl_util.h"
#include "blueftl_wl_dual_pool.h" 

unsigned char migration_buff[FLASH_PAGE_SIZE];

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

void check_max_min_nr_erase_cnt(struct ftl_context_t *ptr_ftl_context){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_erase_block;
	int i;
	uint32_t max_erase_cnt = 0;
	uint32_t min_erase_cnt = 99999;

	for (i = 0; i < ptr_ssd->nr_blocks_per_chip; i++) {
		ptr_erase_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[k]; /* size of bus and chip: 1 */
		uint32_t curr_cnt = ptr_erase_block->nr_erase_cnt;

		if (max_erase_cnt < curr_cnt)
			max_erase_cnt = curr_cnt;
		if (min_erase_cnt > curr_cnt)
			min_erase_cnt = curr_cnt;
	}
	/* TODO: 위 부분은 dual pool 실행 전에 확인하는 코드. 어디에 쓸 수 있을까? */
}

/* 2. cold pool adjustment */
bool check_cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
	find_max_rec_pool_block_info(ptr_ftl_context, COLD_POOL);
	find_min_rec_pool_block_info(ptr_ftl_context, HOT_POOL);

	if (g_max_rec_in_cold_pool - g_min_rec_in_hot_pool > WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

void cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_max_rec_block;

	ptr_max_rec_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[g_max_rec_in_cold_no];
	// TODO: 위 블록을 hot pool의 맨 뒤(왼쪽)으로 이동
}

/* 3. hot pool adjustment */
bool check_hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
	find_max_ec_pool_block_info(ptr_ftl_context, HOT_POOL);
	find_min_ec_pool_block_info(ptr_ftl_context, HOT_POOL);

	if (g_max_ec_in_hot_pool - g_min_ec_in_hot_pool > 2 * WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
	
}

void hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context) {	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_min_ec_block;

	ptr_min_ec_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[g_min_ec_in_hot_no];
	// TODO: 위 블록을 cold pool의 맨 뒤(왼쪽)으로 이동
}

/* 1. cold data migration */
bool check_cold_data_migration(struct ftl_context_t *ptr_ftl_context){
	find_max_ec_pool_block_info(ptr_ftl_context, HOT_POOL);
	find_min_ec_pool_block_info(ptr_ftl_context, COLD_POOL);

	if (g_max_ec_in_hot_pool - g_min_ec_in_cold_pool > WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

/* TODO: how to control dual_pool_block_info struct ? */
struct flash_block_t *get_min_max_ptr(struct ftl_context_t *ptr_ftl_context, dual_pool_block_info *pool_info){
}

struct flash_block_t *get_erase_blk_ptr(struct ftl_context_t *ptr_ftl_context, uint32_t target_bus, uint32_t target_chip, uint32_t target_block){
}

bool block_copy(struct flash_block_t *ptr_src_block, struct flash_block_t *ptr_tgt_block, struct ftl_context_t *ptr_ftl_context){
}

bool page_clean_in_block(struct flash_block_t *ptr_block,  struct ftl_context_t *ptr_ftl_context){
}

/* 1. cold data migration */
void cold_data_migration(struct ftl_context_t* ptr_ftl_context){
	/* TODO: it is so waste */
	uint32_t oldest_in_hot_pool = g_max_ec_pool_block_no;
	uint32_t youngest_in_cold_pool = g_min_ec_pool_block_no;
	uint32_t target;

	// copy valid pages in oldest to other block (where?)
	// clear oldest
	// copy valid pages in youngest to cleared block
	// clear youngest
	// change status variables (swap)
	// and initialize EEC of new cold block (include hot data)
	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_oldest_block;
	struct flash block_t* ptr_youngest_block;
	struct flash block_t* ptr_tgt_block; /* TODO: how to choose ? */

	ptr_oldest_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[oldest_in_hot_pool];
	ptr_youngest_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[youngest_in_cold_pool];
	ptr_tgt_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[target]; /* TODO: how to choose target block for oldest data */
	
	block_copy(ptr_oldest_block, ptr_tgt_block, ptr_ftl_context);
	page_clean_in_block(ptr_oldest_block, ptr_ftl_context);
	block_copy(ptr_youngest_block, ptr_oldest_block, ptr_ftl_context);
	page_clean_in_block(ptr_youngest_block, ptr_ftl_context);
	// change status variables (swap)
	// and initialize EEC of new cold block (include hot data)
}

void insert_pool(struct ftl_context_t* ptr_ftl_context, struct flash_block_t* ptr_erase_block){
}

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

uint32_t update_max_min_nr_erase_cnt_in_pool( int pool, int type, int min_max, int bus, int chip, int block, uint32_t nr_erase_cnt){
}

void init_global_wear_leveling_metadata(void){
}

