
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
}

void cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
}

bool check_hot_pool_adjustment(){
}

void hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context)
}

bool check_cold_data_migration(){
}

struct flash_block_t *get_min_max_ptr(struct ftl_context_t *ptr_ftl_context, dual_pool_block_info *pool_info){
}

struct flash_block_t *get_erase_blk_ptr(struct ftl_context_t *ptr_ftl_context, uint32_t target_bus, uint32_t target_chip, uint32_t target_block){
}

bool block_copy(struct flash_block_t *ptr_src_block, struct flash_block_t *ptr_tgt_block, struct ftl_context_t *ptr_ftl_context){
}

bool page_clean_in_block(struct flash_block_t *ptr_block,  struct ftl_context_t *ptr_ftl_context){
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

