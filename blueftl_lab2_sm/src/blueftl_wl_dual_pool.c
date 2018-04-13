
/* This is just a sample blueftl_wearleveling_dualpool code for lab 2*/

#include <linux/module.h> 
#include "blueftl_ftl_base.h"
#include "blueftl_user_ftl_main.h"
#include "blueftl_ssdmgmt.h"
#include "bluessd_user_vdevice.h"
#include "blueftl_mapping_page.h"
#include "blueftl_util.h"
#include "blueftl_wl_dual_pool.h" 

unsigned char migration_buff[FLASH_PAGE_SIZE];

bool check_cold_pool_adjustment(){
	if(g_max_rec_in_cold_pool.nr_erase_cnt - g_min_rec_in_hot_pool.nr_erase_cnt > WEAR_LEVELING_THRESHOLD) return true;
	else return false;
}

void cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
}

bool check_hot_pool_adjustment(){
	if(g_max_ec_in_hot_pool.nr_erase_cnt - g_min_ec_in_hot_pool.nr_erase_cnt > 2*WEAR_LEVELING_THRESHOLD) return true;
	else return false;
}

void hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context)
	}

bool check_cold_data_migration(){
	if(g_max_ec_in_hot_pool.nr_erase_cnt - g_min_ec_in_cold_pool.nr_erase_cnt > WEAR_LEVELING_THRESHOLD) return true;
	else return false;
}

struct flash_block_t *get_min_max_ptr(struct ftl_context_t *ptr_ftl_context, dual_pool_block_info *pool_info){
}

struct flash_block_t *get_erase_blk_ptr(struct ftl_context_t *ptr_ftl_context, uint32_t target_bus, uint32_t target_chip, uint32_t target_block){
}

bool block_copy(struct flash_block_t *ptr_src_block, struct flash_block_t *ptr_tgt_block, struct ftl_context_t *ptr_ftl_context){

	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;

	struct flash_block_t* ptr_src_block = ptr_src_block;
	struct flash_block_t* ptr_tgt_block = ptr_tgt_block;

	uint32_t i ;

	/* src의 모든 valid page를 찾아서 tgt에 copy */
	for(i= 0 ; i < ptr_ssd->nr_pages_per_block; i++){
		if(ptr_src_block->list.pages[i] == PAGE_STATUS_VALID)
			ptr_tgt_block
	}


}


bool page_clean_in_block(struct flash_block_t *ptr_block,  struct ftl_context_t *ptr_ftl_context){

	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_erase_block = ptr_block;

	struct flash_page_t* ptr_erase_page  = NULL;

	uint32_t i;

	for(i = 0; i < ptr_ssd->nr_pages_per_block; i++ ){
		if(ptr_erase_block->list.pages[i] != PAGE_STATUS_FREE){
			ptr_erase_block->list.pages[i]->page_status = PAGE_STATUS_FREE;
		}
	}
	ptr_erase_block->nr_valid_pages = 0;
	ptr_erase_block->nr_invalid_pages = 0;
	ptr_erase_block->nr_free_pages = ptr_ssd->nr_pages_per_block;

	ptr_erase_block->nr_erase_cnt++;
}

void cold_data_migration(struct ftl_context_t* ptr_ftl_context){
}

void insert_pool(struct ftl_context_t* ptr_ftl_context, struct flash_block_t* ptr_erase_block){
}

uint32_t find_max_ec_pool_block_info(struct ftl_context_t* ptr_ftl_context, uint32_t pool){
}

uint32_t find_min_ec_pool_block_info(struct ftl_context_t* ptr_ftl_context, uint32_t pool){
}

uint32_t find_max_rec_pool_block_info(struct ftl_context_t* ptr_ftl_context, uint32_t pool){
}

uint32_t find_min_rec_pool_block_info(struct ftl_context_t* ptr_ftl_context, uint32_t pool){
}

uint32_t update_max_min_nr_erase_cnt_in_pool( int pool, int type, int min_max, int bus, int chip, int block, uint32_t nr_erase_cnt){
}

void init_global_wear_leveling_metadata(void){
}

