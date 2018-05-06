#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "blueftl_util.h"
#include "blueftl_user.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_block.h"
#include "blueftl_char.h"
#include "blueftl_mapping_page.h"
#include "lzrw3.h"


struct ftl_base_t _ftl_base;
struct ftl_context_t* _ptr_ftl_context = NULL;

extern int8_t _is_debugging_mode;

struct compression_header{
    uint32_t logical_page_address[4];
    uint8_t offset[4];
};

int chk_data_in_wb(struct write_buffer_t* wb, uint32_t lpa){
    int i=0;
    for(i=0; i< CHUNK_SIZE; i++){
        if(wb->t_lpa[i] == lpa ){
            return i;
        }
    }
    return -1;
}

int32_t blueftl_user_ftl_create (struct ssd_params_t* ptr_ssd_params)
{
    /* open the character device driver for blueSSD */
    if ((_ptr_vdevice = blueftl_user_vdevice_open (ptr_ssd_params)) == NULL) {
        return -1;
    }

    /* map the block mapping functions to _ftl_base */
    _ftl_base = ftl_base_page_mapping;

    /* initialize the user-level FTL */
    if ((_ptr_ftl_context = _ftl_base.ftl_create_ftl_context (_ptr_vdevice)) == NULL) {
        printf ("blueftl_user_ftl_main: error occurs when creating a ftl context\n");
        blueftl_user_vdevice_close (_ptr_vdevice);
        return -1;
    }
    return 0;
}

void blueftl_user_ftl_destroy (struct virtual_device_t* _ptr_vdevice)
{
    /* close the character device driver */
    blueftl_user_vdevice_close (_ptr_vdevice);

    /* destroy the user-level FTL */
    _ftl_base.ftl_destroy_ftl_context (_ptr_ftl_context);
}

/* ftl entry point */
int32_t blueftl_user_ftl_main (
        uint8_t req_dir,
        uint32_t lba_begin,
        uint32_t lba_end,
        uint8_t* ptr_buffer)
{
    uint32_t lpa_begin;
    uint32_t lpa_end;
    uint32_t lpa_curr;
    int32_t ret;

    struct write_buffer_t* wb = _ptr_ftl_context->write_buf;


    if (_ptr_vdevice == NULL || _ptr_vdevice->blueftl_char_h == -1) {
        printf ("blueftl_user_ftl_main: the character device driver is not initialized\n");
        return -1;
    }

    lpa_begin = lba_begin / _ptr_vdevice->page_main_size;
    lpa_end = lpa_begin + (lba_end / _ptr_vdevice->page_main_size);

    switch (req_dir) {
        case NETLINK_READA:
        case NETLINK_READ:

            for (lpa_curr = lpa_begin; lpa_curr < lpa_end; lpa_curr++) {
                /* find a physical page address corresponding to a given lpa */
                uint32_t bus, chip, page, block;
                uint8_t* ptr_lba_buff = ptr_buffer + 
                    ((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);

                if (_ftl_base.ftl_get_mapped_physical_page_address (
                            _ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == 0) {
                    //read from write buffer if the data is located in write buffer
                    int cnt = chk_data_in_wb(wb, lpa_curr);
                    if(cnt != -1 ){
                        //printf("[rocky][%s::%d] read from wb, lpa: %u\n", __FUNCTION__, __LINE__, lpa_curr);
                        // hit!
                        memcpy(ptr_lba_buff, wb->ptr_wb_buff + cnt*_ptr_vdevice->page_main_size, _ptr_vdevice->page_main_size );
                    }
                    else{
                        //printf("[rocky][%s::%d] read form nand, lpa: %u\n", __FUNCTION__, __LINE__, lpa_curr);

                        // 여기서 압축 풀어서 해당 페이지만 ptr_lba_buff로 넣어주기
                        uint32_t is_comp = _ptr_ftl_context->chk_table[page].is_compressed;
                        uint8_t* ptr_before_decomp = (uint8_t* )malloc( _ptr_vdevice->page_main_size * CHUNK_SIZE);
                        memset(ptr_before_decomp, 0xFF, (_ptr_vdevice->page_main_size * CHUNK_SIZE));

                        uint8_t* ptr_after_decomp = (uint8_t* )malloc( sizeof(struct compression_header) + _ptr_vdevice->page_main_size * CHUNK_SIZE);
                        memset(ptr_after_decomp, 0xFF, (sizeof(struct compression_header) + _ptr_vdevice->page_main_size * CHUNK_SIZE));

                        if(is_comp){

                            int num_pages = _ptr_ftl_context->chk_table[page].physical_page_cnt;
                            int i=0;
                            for(i=0; i<num_pages; i++){
                                blueftl_user_vdevice_page_read (
                                        _ptr_vdevice,
                                        bus, chip, block, page, 
                                        _ptr_vdevice->page_main_size, 
                                        (char*)ptr_before_decomp + i * _ptr_vdevice->page_main_size);
                            }
                            
                            decompress(ptr_before_decomp, sizeof(ptr_before_decomp) , ptr_after_decomp);
                            struct compression_header* temp_header = (struct compression_header *)malloc(sizeof(struct compression_header));
                            //temp_header = ptr_after_decomp;                          
                            memcpy(temp_header, ptr_after_decomp, sizeof(struct compression_header));

                            for(i = 0; i < 4; i++){
                                if(temp_header->logical_page_address[i] == lpa_curr){
                                    memcpy(ptr_lba_buff,  ptr_after_decomp + sizeof(struct compression_header) + temp_header->offset[i]*_ptr_vdevice->page_main_size, _ptr_vdevice->page_main_size);
                                }
                            }
                              
                        }
                        else{
                            blueftl_user_vdevice_page_read (
                                    _ptr_vdevice,
                                    bus, chip, block, page, 
                                    _ptr_vdevice->page_main_size, 
                                    (char*)ptr_lba_buff);
                        }

                        free(ptr_before_decomp);
                        free(ptr_after_decomp);
                    }

                } else {
                    /* the requested logical page is not mapped to any physical page */
                    /* simply ignore */
                }
            }
            break;

        case NETLINK_WRITE:
            for (lpa_curr = lpa_begin; lpa_curr < lpa_end; lpa_curr++) {
                uint32_t bus, chip, block, page;
                uint8_t* ptr_lba_buff = ptr_buffer + 
                    ((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);
                uint8_t is_merge_needed = 0;
                uint8_t* ptr_commit_buff;


		if(wb->wb_page_cnt < CHUNK_SIZE){
			//skim: check if the lpa_curr exist in the wb
			int i;
			int update = 0;
			for(i=0; i < wb->wb_page_cnt; i++){
				if(wb->t_lpa[i] == lpa_curr){
					memcpy(wb->ptr_wb_buff + i*_ptr_vdevice->page_main_size, ptr_lba_buff , _ptr_vdevice->page_main_size);
					update = 1;
				}
			}
			if(update != 1){
				wb->t_lpa[wb->wb_page_cnt] = lpa_curr;
				memcpy(wb->ptr_wb_buff + wb->wb_page_cnt*_ptr_vdevice->page_main_size, ptr_lba_buff , _ptr_vdevice->page_main_size); 
				wb->wb_page_cnt++;
			}

		}
		if( wb->wb_page_cnt == CHUNK_SIZE){ // 버퍼가 다 차면
			//printf("[rocky][%s::%d] commit!\n", __FUNCTION__, __LINE__);
			//skim: first update the valid page count and invalidate existing mapping
			int i=0;
			//_ptr_ftl_context->chk_table[page].no_physical_pages = no_pages_to_be_write;
			struct ftl_page_mapping_context_t* ptr_pg_mapping = _ptr_ftl_context->ptr_mapping;

			for(i = 0; i < wb->wb_page_cnt; i++){
				if(ptr_pg_mapping->ptr_pg_table[wb->t_lpa[i]] != PAGE_TABLE_FREE){
				
					//reduce valid counter for the chunk
					_ftl_base.ftl_get_mapped_physical_page_address (
						_ptr_ftl_context, wb->t_lpa[i], &bus, &chip, &block, &page);
	
					int num_pages_prev = _ptr_ftl_context->chk_table[block*64 + page].physical_page_cnt;
					int num_valid_prev = _ptr_ftl_context->chk_table[block*64 + page].valid_page_cnt;
					int j;
							
				//	for(j=0; j < num_pages_prev; j++){
						_ptr_ftl_context->chk_table[block*64 + page].valid_page_cnt--;
				//	}
					//free the chunk and the page
					if(_ptr_ftl_context->chk_table[block*64 + page].valid_page_cnt == 0){
						//printf("no more valid, %d\n", num_pages_prev);
				//		for(j=0; j < num_pages_prev; j++){
							_ftl_base.ftl_get_mapped_physical_page_address (
									_ptr_ftl_context, wb->t_lpa[i], &bus, &chip, &block, &page);
						
                            				//printf("[rocky][%s::%d] get_mapped %u, %u, %u, %u\n", __FUNCTION__, __LINE__, bus, chip, block, page);
	
							struct flash_block_t* ptr_temp_block;
							ptr_temp_block = &_ptr_ftl_context->ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block];
							ptr_temp_block->list_pages[page].page_status = PAGE_STATUS_INVALID;
							ptr_temp_block->nr_valid_pages--;
							ptr_temp_block->nr_invalid_pages++;
							_ptr_ftl_context->chk_table[block*64 + page].valid_page_cnt = 0;
							_ptr_ftl_context->chk_table[block*64 + page].physical_page_cnt = 0;
				//		}
					}
					ptr_pg_mapping->ptr_pg_table[wb->t_lpa[i]] = -1;
				}
			}

			uint32_t wb_bus, wb_chip, wb_block, wb_page;
                    int need_compression = 1;
                    UWORD comp_res_size = 0;  
                    int no_pages_to_be_write =0;

                    /* compression 사이즈 계산 및 compression */
                    if(( ptr_commit_buff = (uint8_t*)malloc(sizeof(struct compression_header) +  _ptr_vdevice->page_main_size * CHUNK_SIZE)) == NULL ){
                        //printf("rocky: commit buffer allocation for the write_buffer_t failed\n");
                        return -1;
                    }
                    //add header
                    struct compression_header ch;
                    for(i = 0; i < 4 ; i++){
                        ch.logical_page_address[i] = wb->t_lpa[i];
                        ch.offset[i] = i;
                        //printf("AT WRITE ch_lpa %u, wb_lpa: %u\n", ch.logical_page_address[i], wb->wb_lpa[i]);
                    }

                    uint8_t* temp_wb_header;
                    temp_wb_header = (uint8_t*)malloc(sizeof(struct compression_header) +  _ptr_vdevice->page_main_size * CHUNK_SIZE);
                    memcpy(temp_wb_header, &ch, sizeof(struct compression_header));
                    memcpy(temp_wb_header + sizeof(struct compression_header), wb->ptr_wb_buff, _ptr_vdevice->page_main_size * CHUNK_SIZE);

                    comp_res_size = compress(temp_wb_header, _ptr_vdevice->page_main_size * CHUNK_SIZE + sizeof(struct compression_header) , ptr_commit_buff);
                    free(temp_wb_header);

                    if(comp_res_size >= _ptr_vdevice->page_main_size * CHUNK_SIZE){
                        need_compression = 0;
                        no_pages_to_be_write = 4;

                    }else{
                        need_compression = 1;
                        no_pages_to_be_write = (comp_res_size / _ptr_vdevice->page_main_size) + 1 ; 
                    }


                    uint8_t* ptr_comp_buff;
                    /* write buffer commit */
                    for(i=0; i < CHUNK_SIZE ; i++ ){
                        //printf("[rocky][%s::%d] \n", __FUNCTION__, __LINE__);
                        if( i < no_pages_to_be_write){ // i = 0,1,2,3
                            if(need_compression == 1)
                                ptr_comp_buff = ptr_commit_buff + i * _ptr_vdevice->page_main_size;
                            else if(need_compression == 0)
                                ptr_comp_buff = wb->ptr_wb_buff + i * _ptr_vdevice->page_main_size;

                           // printf("[rocky][%s::%d] \n", __FUNCTION__, __LINE__);

                            /* get the new physical page address from the FTL */
                            if (_ftl_base.ftl_get_free_physical_page_address (
                                        _ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == -1) {
                                /* there are no free pages; do garbage collection */
                                if (_ftl_base.ftl_trigger_gc != NULL) {
                                    /* trigger gc */
                                    if (_ftl_base.ftl_trigger_gc (_ptr_ftl_context, bus, chip) == -1) {
                                        printf ("bluessd: oops! garbage collection failed.\n");
                                        ret = -1;
                                        goto failed;
                                    }

                                    /* garbage collection has been finished; chooses the new free page */
                                    if (_ftl_base.ftl_get_free_physical_page_address (
                                                _ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == -1) {
					printf ("bluessd: there is not sufficient space in flash memory.\n");
                                        ret = -1;
                                        goto failed;
                                    }

                                    /* ok. we obtain new free space */
                                } else if (_ftl_base.ftl_trigger_merge != NULL) {
                                    /* the FTL does not support gc,
                                       so it is necessary to merge the new data with the existing data */
                                    is_merge_needed = 1;
                                } else {
                                    printf ("bluessd: garbage collection is not registered\n");
                                    ret = -1;
                                    goto failed;
                                }
                            }// end: get new physical page

                            //printf("[rocky][%s::%d] write to nand %u, %u, %u, %u\n", __FUNCTION__, __LINE__, bus, chip, block, page);

                            /* write to nand */
							printf("write to nand %u, %u, %u, %u\n", bus, chip, block, page);
                            blueftl_user_vdevice_page_write(
                                    _ptr_vdevice, 
                                    bus, chip, block, page,
                                    _ptr_vdevice->page_main_size, 
                                    (char*)ptr_comp_buff);
							printf("write to nand finished \n");
			    /* make page valid */
			    struct flash_block_t* ptr_temp_block;
			    ptr_temp_block = &_ptr_ftl_context->ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block];
			    ptr_temp_block->list_pages[page].page_status = PAGE_STATUS_VALID;
			    ptr_temp_block->nr_free_pages--;
			    ptr_temp_block->nr_valid_pages++;
				printf("ptr_temp_block nr_page_free %u\n",ptr_temp_block->nr_free_pages);
                            /* mapping */
                            if(i == 0){ // wb 의 첫 번쨰 page mapping
                                wb_bus = bus;
                                wb_chip = chip;
                                wb_block = block;
                                wb_page = page;

                               // printf("[rocky][%s::%d] \n", __FUNCTION__, __LINE__);
                                _ftl_base.ftl_map_logical_to_physical (_ptr_ftl_context, wb->t_lpa[i], 
                                        bus, chip, block, page,
                                        2, -1, -1, -1);
                            }
                            else{ // 나머지 mapping
                                //printf("[rocky][%s::%d] \n", __FUNCTION__, __LINE__);
                                _ftl_base.ftl_map_logical_to_physical (_ptr_ftl_context, wb->t_lpa[i], 
                                        bus, chip, block, page,
                                        wb_bus, wb_chip, wb_block, wb_page);
                            }

                            /* chunk table */
                            if(need_compression == 1 ){
                                _ptr_ftl_context->chk_table[block*64 + page + i].is_compressed = 1;
                            }
                            else{
                                _ptr_ftl_context->chk_table[block*64 + page + i].is_compressed = 0;
                            }
                            _ptr_ftl_context->chk_table[block*64 + page + i].physical_page_cnt = no_pages_to_be_write;
                            _ptr_ftl_context->chk_table[block*64 + page + i].valid_page_cnt = CHUNK_SIZE;

                        }
                        else{
                            _ftl_base.ftl_map_logical_to_physical (_ptr_ftl_context, wb->t_lpa[i], 
                                    bus, chip, block, page,
                                    wb_bus, wb_chip, wb_block, wb_page);
                        } 

                        wb->t_lpa[i] = -1;
                    }// end for in CHUNK_SIZE

                    wb->wb_page_cnt = 0;
                    memset(wb->ptr_wb_buff, 0xFF, CHUNK_SIZE * _ptr_vdevice->page_main_size);  
                    free(ptr_commit_buff);
                }// end commit


            }// end for all lba
            break;

        default:
            printf ("bluessd: invalid I/O type (%u)\n", req_dir);
            ret = -1;
            goto failed;
    }

    ret = 0;

failed:
    /*blueftl_user_vdevice_req_done (_ptr_vdevice);*/
    return ret;
}
