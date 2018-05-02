
/* This is just a sample blueftl_wearleveling_dualpool code */

// #include <linux/module.h> 
#include <stdio.h>
#include <stdlib.h>

#include "blueftl_ftl_base.h"
#include "blueftl_user_ftl_main.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_util.h"
#include "blueftl_gc_page.h"
#include "blueftl_wl_dual_pool.h"
#include "blueftl_mapping_page.h"

uint32_t dbgtime = 0;

dual_pool_block_info MAX_EC_HOT;
dual_pool_block_info MAX_REC_HOT;
dual_pool_block_info MAX_EC_COLD;
dual_pool_block_info MAX_REC_COLD;

dual_pool_block_info MIN_EC_COLD;   
dual_pool_block_info MIN_REC_COLD;
dual_pool_block_info MIN_EC_HOT;
dual_pool_block_info MIN_REC_HOT;

#define __DUALPOOL_DEBUG 0

#if __DUALPOOL_DEBUG
uint32_t g_space=0;
#endif

/*self-defined function erase block*/
uint32_t erase_block(struct ftl_context_t *ptr_ftl_context,struct flash_block_t *aim_block){
    struct virtual_device_t* ptr_vdevice=ptr_ftl_context->ptr_vdevice;
    struct flash_ssd_t *ptr_ssd = ptr_ftl_context->ptr_ssd;
    uint32_t i;

#if __DUALPOOL_DEBUG    
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - start \n", __func__);
    g_space++;
#endif

    blueftl_user_vdevice_block_erase (ptr_vdevice, aim_block->no_bus, aim_block->no_chip, aim_block->no_block);
    aim_block->nr_erase_cnt++;
    aim_block->nr_recent_erase_cnt++;
    //aim_block->is_reserved_block=0;     mskim
	aim_block->nr_valid_pages = 0;
	aim_block->nr_invalid_pages = 0;
	aim_block->nr_free_pages = ptr_ssd->nr_pages_per_block;
	aim_block->last_modified_time = timer_get_timestamp_in_us();
    for(i=0; i<ptr_ssd->nr_pages_per_block; i++){
        aim_block->list_pages[i].no_logical_page_addr=-1;
        aim_block->list_pages[i].page_status=1;
    }
    
#if __DUALPOOL_DEBUG
    g_space--;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - end\n", __func__);
#endif
    return 0;
}
        

/*self-defined function mapping between dual_pool_block_info and flash_block_t*/
void mapping_info_and_fbt(dual_pool_block_info *block_info, struct flash_block_t *ptr_block){
#if __DUALPOOL_DEBUG
    uint32_t i;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - start ", __func__);
    g_space++;
#endif

    if(ptr_block==NULL){
        block_info->no_bus=0;
        block_info->no_chip=0;
        block_info->no_block=0;
        block_info->nr_erase_cnt=0;
        block_info->nr_recent_erase_cnt=0;
    }
    else{
        block_info->no_bus=ptr_block->no_bus;
        block_info->no_chip=ptr_block->no_chip;
        block_info->no_block=ptr_block->no_block;
        block_info->nr_erase_cnt=ptr_block->nr_erase_cnt;
        block_info->nr_recent_erase_cnt=ptr_block->nr_recent_erase_cnt;
    }

#if __DUALPOOL_DEBUG
    g_space--;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - end", __func__);
#endif
}

void cold_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
    struct flash_ssd_t *ptr_ssd = ptr_ftl_context->ptr_ssd;

#if __DUALPOOL_DEBUG
    uint32_t i;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - start ", __func__);
    g_space++;
#endif

    if((MAX_REC_COLD.nr_recent_erase_cnt - MIN_REC_HOT.nr_recent_erase_cnt)>WEAR_LEVELING_THRESHOLD){
        if(timer_get_timestamp_in_sec() - dbgtime > 3) printf("cold pool adjustment\n");
        struct flash_block_t *goal;
        goal = &(ptr_ssd->list_buses[MAX_REC_COLD.no_bus].list_chips[MAX_REC_COLD.no_chip].list_blocks[MAX_REC_COLD.no_block]);
        goal->hot_cold_pool=HOT_POOL;
        // goal->head_or_tail_rec=TAIL;
        // goal->nr_recent_erase_cnt=0;
        update_max_min_nr_erase_cnt_in_pool(ptr_ftl_context);
    }
    // else printf("There has no check cold pool adjustment!\n");

#if __DUALPOOL_DEBUG
    g_space--;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - end", __func__);
#endif
}

void hot_pool_adjustment(struct ftl_context_t *ptr_ftl_context){
    struct flash_ssd_t *ptr_ssd = ptr_ftl_context->ptr_ssd;

#if __DUALPOOL_DEBUG
    uint32_t i;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - start ", __func__);
    g_space++;
#endif

    if((MAX_REC_HOT.nr_erase_cnt-MIN_REC_HOT.nr_erase_cnt)>2*WEAR_LEVELING_THRESHOLD){
        if(timer_get_timestamp_in_sec() - dbgtime > 3) printf("hot pool adjustment\n");
        struct flash_block_t *goal;
        goal =&(ptr_ssd->list_buses[MIN_REC_HOT.no_bus].list_chips[MIN_REC_HOT.no_chip].list_blocks[MIN_REC_HOT.no_block]);
        goal->hot_cold_pool=COLD_POOL;
        // goal->head_or_tail_ec=TAIL;
        // goal->nr_recent_erase_cnt=0;
        update_max_min_nr_erase_cnt_in_pool(ptr_ftl_context);
    }
    // else printf("There has no check hot pool adjustment!\n");

#if __DUALPOOL_DEBUG    
    g_space--;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - end", __func__);
#endif
}

void cold_data_migration(struct ftl_context_t *ptr_ftl_context){
    struct flash_ssd_t *ptr_ssd = ptr_ftl_context->ptr_ssd;
    struct flash_block_t *being_mig_hot_block=NULL;
    struct flash_block_t *being_mig_cold_block=NULL;
	struct flash_block_t *reserved = ((struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping)->reserved;

#if __DUALPOOL_DEBUG
    uint32_t i;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - start ", __func__);
    g_space++;
#endif


    if(MAX_EC_HOT.nr_erase_cnt - MIN_EC_COLD.nr_erase_cnt > WEAR_LEVELING_THRESHOLD){
        if(timer_get_timestamp_in_sec() - dbgtime > 3) printf("cold data migration\n");
        being_mig_hot_block=&(ptr_ssd->list_buses[MAX_EC_HOT.no_bus].list_chips[MAX_EC_HOT.no_chip].list_blocks[MAX_EC_HOT.no_block]);
        being_mig_cold_block=&(ptr_ssd->list_buses[MIN_EC_COLD.no_bus].list_chips[MIN_EC_COLD.no_chip].list_blocks[MIN_EC_COLD.no_block]);

        move_block(ptr_ftl_context, being_mig_hot_block, reserved);
        erase_block(ptr_ftl_context, being_mig_hot_block);

        move_block(ptr_ftl_context, being_mig_cold_block, being_mig_hot_block);
        erase_block(ptr_ftl_context, being_mig_cold_block);

        move_block(ptr_ftl_context, reserved, being_mig_cold_block);
        erase_block(ptr_ftl_context, reserved);

        being_mig_hot_block->hot_cold_pool=COLD_POOL;
        being_mig_cold_block->hot_cold_pool=HOT_POOL;

        
        being_mig_hot_block->nr_recent_erase_cnt=0;
        // 맞는지 확인
        // being-mig_cold_block->nr_recent_erase_cnt=0;

        update_max_min_nr_erase_cnt_in_pool(ptr_ftl_context);
    }

#if __DUALPOOL_DEBUG
    g_space--;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - end", __func__);
#endif    
}

uint32_t update_max_min_nr_erase_cnt_in_pool(struct ftl_context_t *ptr_ftl_context){
    struct flash_ssd_t *ptr_ssd = ptr_ftl_context->ptr_ssd;

    uint32_t nr_bus=ptr_ssd->nr_buses;
    uint32_t nr_chip= ptr_ssd->nr_chips_per_bus;
    uint32_t nr_block=ptr_ssd->nr_blocks_per_chip;
    uint32_t bus=0;
    uint32_t chip=0;
    uint32_t block=0;
    
    struct flash_block_t *ptr_block;

    struct flash_block_t *max_ec_hot = NULL;
    struct flash_block_t *max_rec_hot = NULL;
    struct flash_block_t *max_ec_cold = NULL;
    struct flash_block_t *max_rec_cold = NULL;
    struct flash_block_t *min_ec_cold = NULL;
    struct flash_block_t *min_rec_cold = NULL;
    struct flash_block_t *min_ec_hot = NULL;
    struct flash_block_t *min_rec_hot = NULL;

#if __DUALPOOL_DEBUG
    uint32_t i;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - start ", __func__);
    g_space++;
#endif

    for(;bus<nr_bus;bus++){
        for(;chip<nr_chip;chip++){
            for(;block<nr_block;block++){
                ptr_block=&(ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block]);
                if(ptr_block->hot_cold_pool == COLD_POOL){
                    if(max_ec_cold == NULL)
                        max_ec_cold=max_rec_cold=min_ec_cold=min_rec_cold = ptr_block;
                    if(max_ec_cold->nr_erase_cnt < ptr_block->nr_erase_cnt)
                        max_ec_cold = ptr_block;
                    if(max_rec_cold->nr_recent_erase_cnt < ptr_block->nr_recent_erase_cnt)
                        max_rec_cold = ptr_block;
                    if(min_ec_cold->nr_erase_cnt >= ptr_block->nr_erase_cnt)
                        min_ec_cold = ptr_block;
                    if(min_rec_cold->nr_recent_erase_cnt >= ptr_block->nr_recent_erase_cnt)
                        min_rec_cold = ptr_block;
                }
                else if(ptr_block->hot_cold_pool == HOT_POOL){
                    if(max_ec_hot == NULL)
                        max_ec_hot=max_rec_hot=min_ec_hot=min_rec_hot = ptr_block;
                    if(max_ec_hot->nr_erase_cnt < ptr_block->nr_erase_cnt)
                        max_ec_hot = ptr_block;
                    if(max_rec_hot->nr_recent_erase_cnt < ptr_block->nr_recent_erase_cnt)
                        max_rec_hot = ptr_block;
                    if(min_ec_hot->nr_erase_cnt >= ptr_block->nr_erase_cnt)
                        min_ec_hot = ptr_block;
                    if(min_rec_hot->nr_recent_erase_cnt >= ptr_block->nr_recent_erase_cnt)
                        min_rec_hot = ptr_block;
                } 

                else{
                    printf("There involve an error in initializing cold-hot pool");
                    exit(1);
                }
            }
        }
    }

    mapping_info_and_fbt(&MAX_EC_HOT, max_ec_hot);
    mapping_info_and_fbt(&MAX_REC_HOT, max_rec_hot);
    mapping_info_and_fbt(&MAX_EC_COLD, max_ec_cold);
    mapping_info_and_fbt(&MAX_REC_COLD, max_rec_cold);
    mapping_info_and_fbt(&MIN_EC_HOT, min_ec_hot);
    mapping_info_and_fbt(&MIN_REC_HOT, min_rec_hot);
    mapping_info_and_fbt(&MIN_EC_COLD, min_ec_cold);
    mapping_info_and_fbt(&MIN_REC_COLD, min_rec_cold);
    
    uint32_t curtime = timer_get_timestamp_in_sec();
    if(curtime - dbgtime > 3){
        printf("------------------\n");
        printf("MAX_EC_HOT  : %3d %3d\n", MAX_EC_HOT.nr_erase_cnt, MAX_EC_HOT.nr_recent_erase_cnt); 
        printf("MAX_REC_HOT : %3d %3d\n", MAX_REC_HOT.nr_erase_cnt, MAX_REC_HOT.nr_recent_erase_cnt); 
        printf("MAX_EC_COLD : %3d %3d\n", MAX_EC_COLD.nr_erase_cnt, MAX_EC_COLD.nr_recent_erase_cnt); 
        printf("MAX_REC_COLD: %3d %3d\n", MAX_REC_COLD.nr_erase_cnt, MAX_REC_COLD.nr_recent_erase_cnt); 
        printf("MIN_EC_HOT  : %3d %3d\n", MIN_EC_HOT.nr_erase_cnt, MIN_EC_HOT.nr_recent_erase_cnt); 
        printf("MIN_REC_HOT : %3d %3d\n", MIN_REC_HOT.nr_erase_cnt, MIN_REC_HOT.nr_recent_erase_cnt); 
        printf("MIN_EC_COLD : %3d %3d\n", MIN_EC_COLD.nr_erase_cnt, MIN_EC_COLD.nr_recent_erase_cnt); 
        printf("MIN_REC_COLD: %3d %3d\n", MIN_REC_COLD.nr_erase_cnt, MIN_REC_COLD.nr_recent_erase_cnt); 
        printf("------------------\n");

        // printf("HOT   EC    REC   \t\tCOLD  EC    REC   \n");
        // printf("MAX   %4d  %4d  \t\tMAX   %4d  %4d  \n", MAX_EC_HOT.nr_erase_cnt, MAX_EC_HOT.nr_recent_erase_cnt);
        // printf("MIN   %4d  %4d  \t\tMAX   %4d  %4d  \n");
        // printf("\n");

        dbgtime = curtime;
    }

#if __DUALPOOL_DEBUG
    g_space--;
    for(i=0; i<g_space; i++) printf(" ");
    printf("%s - end", __func__);
#endif
    return 0;
}
