/* This is just a sample blueftl_wearleveling_dualpool code for lab 2*/

//#include <linux/module.h> 

#include <stdio.h>
#include <stdlib.h>

#include "blueftl_ftl_base.h"
#include "blueftl_user_ftl_main.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_page.h"
#include "blueftl_gc_page.h"
#include "blueftl_util.h"
#include "blueftl_wl_dual_pool.h" 

#define TRUE 1
#define FALSE 0



/* For pool management */
dual_pool_block_info g_max_ec_in_hot_pool;
dual_pool_block_info g_min_ec_in_hot_pool;
dual_pool_block_info g_max_rec_in_hot_pool;
dual_pool_block_info g_min_rec_in_hot_pool;

dual_pool_block_info g_max_ec_in_cold_pool;
dual_pool_block_info g_min_ec_in_cold_pool;
dual_pool_block_info g_max_rec_in_cold_pool;
dual_pool_block_info g_min_rec_in_cold_pool;


/* erase all pages which are included in chosen block */
void page_clean_in_block(struct flash_block_t *ptr_erase_block, struct ftl_context_t *ptr_ftl_context){

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
		ptr_erase_block->list_pages[i].no_logical_page_addr = PAGE_TABLE_FREE;
		ptr_erase_block->list_pages[i].page_status = PAGE_STATUS_FREE;
		}
}


uint32_t check_cold_pool_adjustment(){
	if((g_max_rec_in_cold_pool.nr_recent_erase_cnt - g_min_rec_in_hot_pool.nr_recent_erase_cnt) > WEAR_LEVELING_THRESHOLD) return TRUE;
	else return FALSE;
}

void cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){

	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	
	if(check_cold_pool_adjustment() == TRUE){
		//if(timer_get_timestamp_in_sec() - dbgtime > 3) printf("COLD POOL adjustment\n");

	uint32_t no_bus = g_max_rec_in_cold_pool.no_bus;
	uint32_t no_chip = g_max_rec_in_cold_pool.no_chip;
	uint32_t no_block = g_max_rec_in_cold_pool.no_block;

	struct flash_block_t* target_block = &(ptr_ssd->list_buses[no_bus].list_chips[no_chip].list_blocks[no_block]);

	target_block->hot_cold_pool = HOT_POOL;

	/* Renew the status of each pool */
	update_max_min_nr_erase_cnt_in_pool(ptr_ftl_context);

	}
}

uint32_t check_hot_pool_adjustment(){
	/* TO DO: is it right g_rec.nr_erase_cnt??? vs g_ec.nr_recent_erase_cnt */
	if(g_max_rec_in_hot_pool.nr_erase_cnt - g_min_rec_in_hot_pool.nr_erase_cnt > WEAR_LEVELING_THRESHOLD*2) return TRUE;
	else return FALSE;
}

void hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context){

	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;

	if(check_hot_pool_adjustment() == TRUE){
		//if(timer_get_timestamp_in_sec() - dbgtime > 3) printf("HOT POOL adjustment\n");

	uint32_t no_bus = g_min_rec_in_hot_pool.no_bus;
	uint32_t no_chip = g_min_rec_in_hot_pool.no_chip;
	uint32_t no_block = g_min_rec_in_hot_pool.no_block;

	struct flash_block_t* target_block = &ptr_ssd->list_buses[no_bus].list_chips[no_chip].list_blocks[no_block];

	target_block->hot_cold_pool = COLD_POOL;

	/* Renew the status of each pool */
	update_max_min_nr_erase_cnt_in_pool(ptr_ftl_context);}
}


uint32_t check_cold_data_migration(){
	if(g_max_ec_in_hot_pool.nr_erase_cnt - g_min_ec_in_cold_pool.nr_erase_cnt > WEAR_LEVELING_THRESHOLD) return TRUE;
	else return FALSE;
}

void cold_data_migration(struct ftl_context_t* ptr_ftl_context){
	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* target_block_in_hot_pool = NULL;
	struct flash_block_t* target_block_in_cold_pool = NULL;
	struct flash_block_t* reserved = ((struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping)->reserved; 

	if(check_cold_data_migration() == TRUE){

		//if(timer_get_timestamp_in_sec() - dbgtime > 3) printf("COLD DATA MIAGRATION\n");

		target_block_in_hot_pool = &(ptr_ssd->list_buses[g_max_ec_in_hot_pool.no_bus].list_chips[g_max_ec_in_hot_pool.no_chip].list_blocks[g_max_ec_in_hot_pool.no_block]);

		target_block_in_cold_pool = &(ptr_ssd->list_buses[g_min_ec_in_cold_pool.no_bus].list_chips[g_min_ec_in_cold_pool.no_chip].list_blocks[g_min_ec_in_cold_pool.no_block]);

		block_copy(target_block_in_hot_pool, reserved, ptr_ftl_context);
		page_clean_in_block(target_block_in_hot_pool, ptr_ftl_context);

		block_copy(target_block_in_cold_pool, target_block_in_hot_pool, ptr_ftl_context);
		page_clean_in_block(target_block_in_cold_pool, ptr_ftl_context);

		block_copy(reserved, target_block_in_cold_pool, ptr_ftl_context);
		page_clean_in_block(reserved, ptr_ftl_context);

		/* For renew the status */
		target_block_in_hot_pool->hot_cold_pool = COLD_POOL;
		target_block_in_cold_pool->hot_cold_pool = HOT_POOL;

		/* reset the EEC of block moved to cold pool during cold migration */ 
		target_block_in_hot_pool->nr_recent_erase_cnt = 0;
	}
}


/* making a connection and saving information between dual_pool_block_info and flash_block_t */
void connection_pool_info_and_block(dual_pool_block_info* dual_pool_info, struct flash_block_t* ptr_block){
	if(ptr_block == NULL){
		dual_pool_info->no_bus = 0;
		dual_pool_info->no_chip = 0;
		dual_pool_info->no_block = 0;

		dual_pool_info->nr_erase_cnt = 0;
		dual_pool_info->nr_recent_erase_cnt = 0;
	}
	else{
		dual_pool_info->no_bus = ptr_block->no_bus;
		dual_pool_info->no_chip = ptr_block->no_chip;
		dual_pool_info->no_block = ptr_block->no_block;

		dual_pool_info->nr_erase_cnt = ptr_block->nr_erase_cnt;
		dual_pool_info->nr_recent_erase_cnt = ptr_block->nr_recent_erase_cnt;
	}
}


uint32_t update_max_min_nr_erase_cnt_in_pool(struct ftl_context_t* ptr_ftl_context){
	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct flash_block_t* ptr_block;

	uint32_t nr_bus = ptr_ssd->nr_buses;
	uint32_t nr_chip = ptr_ssd->nr_chips_per_bus;
	uint32_t nr_block = ptr_ssd->nr_blocks_per_chip;

	uint32_t bus, chip, block;

	struct flash_block_t* max_ec_block_hot = NULL;
	struct flash_block_t* min_ec_block_hot = NULL;

	struct flash_block_t* max_ec_block_cold = NULL;
	struct flash_block_t* min_ec_block_cold = NULL;

	struct flash_block_t* max_rec_block_hot = NULL;
	struct flash_block_t* min_rec_block_hot = NULL;

	struct flash_block_t* max_rec_block_cold = NULL;
	struct flash_block_t* min_rec_block_cold = NULL;

	for(bus = 0; bus < nr_bus; bus++){
		for(chip = 0; chip < nr_chip; chip++){
			for(block = 0; block < nr_block; block++){
				ptr_block = &(ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block]);

				if(ptr_block->hot_cold_pool == HOT_POOL){
					if(max_ec_block_hot == NULL){
					max_ec_block_hot = max_rec_block_hot = min_ec_block_hot = min_rec_block_hot = ptr_block;}
					if(max_ec_block_hot->nr_erase_cnt < ptr_block->nr_erase_cnt){max_ec_block_hot = ptr_block;}
					if(max_rec_block_hot->nr_recent_erase_cnt < ptr_block->nr_recent_erase_cnt){max_rec_block_hot = ptr_block;}
					if(min_ec_block_hot->nr_erase_cnt >= ptr_block->nr_erase_cnt){min_ec_block_hot = ptr_block;}
					if(min_rec_block_hot->nr_recent_erase_cnt >= ptr_block->nr_recent_erase_cnt){min_rec_block_hot = ptr_block;}
					}


				else if(ptr_block->hot_cold_pool == COLD_POOL){
					if(max_ec_block_cold == NULL){
					max_ec_block_cold = max_rec_block_cold = min_ec_block_cold = min_rec_block_cold = ptr_block;}
					if(max_ec_block_cold->nr_erase_cnt < ptr_block->nr_erase_cnt){max_ec_block_cold = ptr_block;}
					if(max_rec_block_cold->nr_recent_erase_cnt < ptr_block->nr_recent_erase_cnt){max_rec_block_cold = ptr_block;}
					if(min_ec_block_cold->nr_erase_cnt >= ptr_block->nr_erase_cnt){min_ec_block_cold = ptr_block;}
					if(min_rec_block_cold->nr_recent_erase_cnt >= ptr_block->nr_recent_erase_cnt){min_rec_block_cold = ptr_block;}
					}
				else{
					printf("There is an ERROR in constructing hot & cold pool");
					return -1; // Is it right?
				    }
				}
			}
		}

/* making a connection and saving information between dual_pool_block_info and flash_block_t */

	connection_pool_info_and_block(&g_max_ec_in_hot_pool, max_ec_block_hot);
	connection_pool_info_and_block(&g_max_rec_in_hot_pool, max_rec_block_hot);
	connection_pool_info_and_block(&g_min_ec_in_hot_pool, min_ec_block_hot);
	connection_pool_info_and_block(&g_min_rec_in_hot_pool, min_rec_block_hot);

	connection_pool_info_and_block(&g_max_ec_in_cold_pool, max_ec_block_cold);
	connection_pool_info_and_block(&g_max_rec_in_cold_pool, max_rec_block_cold);
	connection_pool_info_and_block(&g_min_ec_in_cold_pool, min_ec_block_cold);
	connection_pool_info_and_block(&g_min_rec_in_cold_pool, min_ec_block_cold);

	return 0;
}

