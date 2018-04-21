
/* This is just a sample blueftl_wearleveling_dualpool code for lab 2*/

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "blueftl_ftl_base.h"
#include "blueftl_user_ftl_main.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
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
		ptr_erase_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[i]; /* size of bus and chip: 1 */
		uint32_t curr_cnt = ptr_erase_block->nr_erase_cnt;

		if (max_erase_cnt < curr_cnt)
			max_erase_cnt = curr_cnt;
		if (min_erase_cnt > curr_cnt)
			min_erase_cnt = curr_cnt;
	}
	/* TODO: 위 부분은 dual pool 실행 전에 확인하는 코드. 어디에 쓸 수 있을까? */
}

/* 2. cold pool adjustment */
uint32_t check_cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
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
uint32_t check_hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
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
uint32_t check_cold_data_migration(struct ftl_context_t *ptr_ftl_context){
	find_max_ec_pool_block_info(ptr_ftl_context, HOT_POOL);
	find_min_ec_pool_block_info(ptr_ftl_context, COLD_POOL);

	if (g_max_ec_in_hot_pool - g_min_ec_in_cold_pool > WEAR_LEVELING_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

/* TODO: how to control dual_pool_block_info struct ? */

uint32_t block_copy(struct flash_block_t *ptr_src_block, struct flash_block_t *ptr_tgt_block, struct ftl_context_t *ptr_ftl_context){
	
}

uint32_t page_clean_in_block(struct flash_block_t *ptr_block,  struct ftl_context_t *ptr_ftl_context){

}

/*
struct flash_block_t *get_tgt_block(struct ftl_context_t *ptr_ftl_context) {
	int i;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_erase_block;
	struct flash_block_t* ptr_tgt_block;

	for (i = 0; i < ptr_ssd->nr_blocks_per_chip; i++) {
		ptr_erase_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[i];
		if (ptr_erase_block->hot_cold_pool == HOT_POOL) {
			// DO SOMETHING
		}
	}

}
*/

/* 1. cold data migration */
void cold_data_migration(struct ftl_context_t* ptr_ftl_context){
	/* TODO: it is so waste */
	uint32_t oldest_in_hot_pool = g_max_ec_in_hot_no;
	uint32_t youngest_in_cold_pool = g_min_ec_in_cold_no;

	uint8_t* ptr_page_buff = NULL;
	uint8_t* ptr_block_buff = NULL;

	/* Original */
	// copy valid pages in oldest to other block (where?)
	// clear oldest
	// copy valid pages in youngest to cleared block
	// clear youngest
	// change status variables (swap)
	// and initialize EEC of new cold block (include hot data)
	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct virtual_device_t* ptr_vdevice = ptr_ftl_context->ptr_vdevice;
	struct flash_block_t* ptr_oldest_block;
	struct flash_block_t* ptr_youngest_block;
	struct flash_block_t* ptr_tgt_block; /* TODO: how to choose ? swap ! */

	int i;
	ptr_oldest_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[oldest_in_hot_pool];
	ptr_youngest_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[youngest_in_cold_pool];	

	/* Other */
	// cold data to buff and clean youngest block
	if ((ptr_block_buff = (uint8_t*)malloc (ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size)) == NULL) {
		printf("wl dual pool: the malloc for the buffer failed\n");
		return -1;
	}
	memset (ptr_block_buff, 0xFF, ptr_ssd->nr_pages_per_block * ptr_vdevice->page_main_size);
	
	for (i = 0; i < FLASH_PAGE_SIZE; i++) {
		if (ptr_youngest_block->list_pages[i].page_status == 3) {
			ptr_page_buff = ptr_block_buff + (i * ptr_vdevice->page_main_size);

			blueftl_user_vdevice_page_read (
					_ptr_vdevice,
					0, 0, ptr_youngest_block->no_block, i,
					ptr_vdevice->page_main_size,
					(char*)ptr_page_buff);
		
			perf_wl_inc_page_copies ();
		}
	}

	blueftl_user_vdevice_block_erase (ptr_vdevice, 0, 0, ptr_youngest_block->no_block);
	ptr_youngest_block->nr_erase_cnt++;
	perf_wl_inc_blk_erasures ();

	ptr_youngest_block->nr_invalid_pages = 0;
	ptr_youngest_block->nr_free_pages = 0;

	// hot data to youngest block and clean oldest block
	for (i = 0; i < ptr_ssd->nr_pages_per_block; i++) {
		if (ptr_oldest_block->list_pages[i].page_status == 3) {
			
		}
	}
	
	// buff to oldest block
	for (i = 0; i < ptr_ssd->nr_pages_per_block; i++) {
		
	}
	
	// change status variables
	
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

