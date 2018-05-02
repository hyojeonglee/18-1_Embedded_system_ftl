#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h> //  our new library 
#include <unistd.h>
#include <math.h>
////
#include "blueftl_user.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_util.h"
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_page.h"
#include "blueftl_mapping_block.h"
#include "blueftl_char.h"
#include "blueftl_wl_dual_pool.h"
#include "blueftl_gc_page.h"
#include "blueftl_read_write_mgr.h"
#include "lzrw3.h"
struct ftl_base_t _ftl_base;
struct ftl_context_t* _ptr_ftl_context = NULL;

extern int8_t _is_debugging_mode;

struct sigaction act_new;
struct sigaction act_old;

void printall(int sig){ // can be called asynchronously
	struct flash_ssd_t *ptr_ssd = _ptr_ftl_context->ptr_ssd;
	uint32_t bus=0;
	uint32_t chip=0;
	uint32_t block=0;
	uint32_t max_count=0;
	// double d[1024];

#if WL_ON
	printf("DUALPOOL THRESH: %d\n", WEAR_LEVELING_THRESHOLD);
#else
	printf("NO WEAR LEVELING: %d\n", WL_ON);
#endif

	signal(SIGTSTP, SIG_IGN);
	for(bus=0; bus<ptr_ssd->nr_buses; bus++){
		for(chip=0; chip<ptr_ssd->nr_chips_per_bus; chip++){
			printf("bus,chip:: %d, %d\n", bus, chip);
			for(block=0; block<ptr_ssd->nr_blocks_per_chip; block++){
				// if(block%30==0) printf("\n");
				if(max_count < ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block].nr_erase_cnt){
					max_count = ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block].nr_erase_cnt;
				}
				// d[block] = (double)(ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block].nr_erase_cnt);
				printf("%d ", ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block].nr_erase_cnt);
			}
			printf("\n");
		}
		printf("-----------------------");
	}
	printf("max ec: %d\n", max_count);
	// printf("standard deviation: %lf\n", sqrt(sd(d)));
	// kill(getpid(), SIGINT);
	// exit(0);
}

int32_t blueftl_user_ftl_create (struct ssd_params_t* ptr_ssd_params)
{
	/* open the character device driver for blueSSD */
	if ((_ptr_vdevice = blueftl_user_vdevice_open (ptr_ssd_params)) == NULL) {
		return -1;
	}
	
	act_new.sa_handler = printall; // 시그널 핸들러 지정
	sigemptyset(&act_new.sa_mask);      // 시그널 처리 중 블록될 시그널은 없음
	sigaction(SIGTSTP, &act_new, &act_old); 

	/* map the block mapping functions to _ftl_base */
	// _ftl_base = ftl_base_block_mapping;
	_ftl_base = ftl_base_page_mapping; //* mskim: page mapping */
	

	/* initialize the user-level FTL */
	if ((_ptr_ftl_context = _ftl_base.ftl_create_ftl_context (_ptr_vdevice)) == NULL) {
		printf ("blueftl_user_ftl_main: error occurs when creating a ftl context\n");
		blueftl_user_vdevice_close (_ptr_vdevice);
		return -1;
	}

	if(blueftl_read_write_mgr_init(_ptr_vdevice->page_main_size) == NULL){
		printf("blueftl_read_write_mgr_init: the creation of the write page buff failed");
		return -1;
	}


	printf("blueftl user create end\n");
	return 0;
}

void blueftl_user_ftl_destroy (struct virtual_device_t* _ptr_vdevice)
{
	/* close the character device driver */
	blueftl_user_vdevice_close (_ptr_vdevice);

	/* destroy the user-level FTL */
	_ftl_base.ftl_destroy_ftl_context (_ptr_ftl_context);
	
	blueftl_read_write_mgr_close();
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
				uint8_t* ptr_lba_buff = ptr_buffer + ((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);
				
				blueftl_page_read(_ftl_base, _ptr_ftl_context, lpa_curr, ptr_lba_buff);
			}
			break;

		case NETLINK_WRITE:
			for (lpa_curr = lpa_begin; lpa_curr < lpa_end; lpa_curr++) {
				uint8_t* ptr_lba_buff = ptr_buffer + ((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);
				
				if((ret=blueftl_page_write(_ftl_base, _ptr_ftl_context, lpa_curr, ptr_lba_buff)) == -1){
					goto failed;
				}
			}
			break;

		default:
			printf ("bluessd: invalid I/O type (%u)\n", req_dir);
			ret = -1;
			goto failed;
	}

	ret = 0;

failed:
	blueftl_user_vdevice_req_done (_ptr_vdevice);
	return ret;
}
