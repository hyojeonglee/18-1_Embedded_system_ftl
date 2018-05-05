#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h> //  our new library 
#include <unistd.h>
#include <math.h>

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

#if 0
struct wr_buff_t {
	uint32_t arr_lpa[WRITE_BUFFER_LEN];
	uint8_t buff[FLASH_PAGE_SIZE * WRITE_BUFFER_LEN];
};

struct wr_buff_t _struct_write_buff; 
#endif

uint8_t *_write_buff;
uint8_t *_compressed_buff;
uint32_t _nr_buff_pages;


void serialize();
void deserialize();

int32_t get_free_page(struct ftl_context_t* ptr_ftl_context, 
		uint32_t nr_pages,
		uint32_t *ptr_bus,
		uint32_t *ptr_chip,
		uint32_t *ptr_block,
		uint32_t *ptr_page);

int32_t get_free_pages(struct ftl_base_t* _ftl_base,
		struct ftl_context_t* ptr_ftl_context, 
		uint32_t nr_pages,
		uint32_t *ptr_bus,
		uint32_t *ptr_chip,
		uint32_t *ptr_block,
		uint32_t *ptr_page);

void clean_buff(){
	_struct_write_buff.arr_lpa[0] = -1;
	_struct_write_buff.arr_lpa[1] = -1;
	_struct_write_buff.arr_lpa[2] = -1;
	_struct_write_buff.arr_lpa[3] = -1;
	_nr_buff_pages = 0;
	memset(_struct_write_buff.buff, 0xFF, 4 * FLASH_PAGE_SIZE);
}

int32_t is_page_in_wb(struct wr_buff_t *wb, uint32_t lpa)
{
	int i = 0;
	for(i = 0; i < 4; i++) {
		if(wb->arr_lpa[i] == lpa) {
			return i;
		}
	}
	return -1;
}

uint32_t blueftl_read_write_mgr_init(){   
	clean_buff();

	_write_buff = (uint8_t *)malloc(sizeof(struct wr_buff_t));
	_compressed_buff = (uint8_t *)malloc(8*FLASH_PAGE_SIZE*sizeof(uint8_t));
	return _write_buff || _compressed_buff; 
}

void blueftl_read_write_mgr_close(){
	free(_write_buff);
	free(_compressed_buff);
}

void blueftl_page_read(
		struct ftl_base_t *ftl_base, 
		struct ftl_context_t *ptr_ftl_context, 
		uint32_t lpa_curr, 
		uint8_t *ptr_lba_buff
		){

	uint32_t bus, chip, page, block;
	uint32_t is_comp = 0;
	uint32_t nr_pages; // # of physical pages in a compressed chunk
	uint8_t *decompressed_buff = NULL; //to store decompressed data
	uint8_t *decompressing_buff = NULL;// to store raw data before decompressing
		struct wr_buff_t *write_buff = NULL;
	int32_t loop_page = 0;
	int32_t cnt;

	if (ftl_base->ftl_get_mapped_physical_page_address(ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == 0)
	{
		/*add by charlie*/
		/*page is in the write buff, not commit yet*/
		if((cnt = is_page_in_wb(&_struct_write_buff, lpa_curr)) != -1) {
			memcpy(ptr_lba_buff, _struct_write_buff.buff + cnt * _ptr_vdevice->page_main_size, _ptr_vdevice->page_main_size);	
		}else {
			/*decompress the page and read page from SSD*/
			struct chunk_table_t *ptr_chunk_table = ((struct ftl_page_mapping_context_t*)ptr_ftl_context)->ptr_chunk_table;
			decompressing_buff = (uint8_t *)malloc(_ptr_vdevice->page_main_size * 4);
			memset(decompressing_buff, 0xFF, _ptr_vdevice->page_main_size * 4);
			is_comp = ptr_chunk_table[block * 64 + page].is_compressed;
			nr_pages = ptr_chunk_table[block * 64 + page]. physical_page_len;
			for(loop_page = 0; loop_page < nr_pages; loop_page++) {
				blueftl_user_vdevice_page_read (
						_ptr_vdevice, 
						bus, chip, block, page, 
						_ptr_vdevice->page_main_size,
						(char*)decompressing_buff + loop_page * _ptr_vdevice->page_main_size);
			}
			if(is_comp) { //data compressed
				decompress(decompressing_buff, sizeof(decompressing_buff), decompressed_buff);
				write_buff = (struct wr_buff_t *)decompressed_buff;
				if((cnt = is_page_in_wb(write_buff, lpa_curr)) != -1) {
					memcpy(ptr_lba_buff, write_buff->buff + cnt * _ptr_vdevice->page_main_size, _ptr_vdevice->page_main_size);	
				}else {
					printf("wanted page are not found\n");
					return -1;
				}

			}else { // not compressed
				memcpy(ptr_lba_buff, decompressing_buff, _ptr_vdevice->page_main_size);
				free(decompressing_buff);
				free(decompressed_buff);
			}
		}// end of if; to check the page whether it's in wb

	//	blueftl_user_vdevice_page_read (_ptr_vdevice, bus, chip, block, page, _ptr_vdevice->page_main_size, (char*)ptr_lba_buff);

	} else {
		/* the requested logical page is not mapped to any physical page */
		/* simply ignore */
	}
}

uint32_t blueftl_page_write(
		struct ftl_base_t *ftl_base, 
		struct ftl_context_t *ptr_ftl_context, 
		uint32_t lpa_curr, 
		uint8_t *ptr_lba_buff
		){
	uint32_t i;

	/*
	   (1) write buff count 증가
	   (2) lpa 저장
	   (3) write 대상 데이터를 버퍼에 복사
	   */
	// case of update need to be taken into consideration
	_nr_buff_pages++; // (1)
	_struct_write_buff.arr_lpa[_nr_buff_pages-1] = lpa_curr; // (3)
	memcpy(&_struct_write_buff.buff[_nr_buff_pages-1], ptr_lba_buff, FLASH_PAGE_SIZE); // (5)

	/* buff가 꽉찼으므로 compression 후 write */
	if(_nr_buff_pages == WRITE_BUFFER_LEN){
		printf("going to compress\n");

		/*
		   압축이 필요한가 - 압축이 필요할 때
		   1. 압축
		   2. 압축된 페이지 사이즈에 맞게 ftl 페이지 요청
		   3. physical write
		   4. ftl 등록
		   압축이 필요한가 - 압축이 필요하지 않을 때
		   각 4개의 페이지에 대해
		   1. ftl 페이지요청
		   2. physical write
		   3. ftl 등록 
170419: 압축 write를 먼저 구현하자. 압축필요여부는 나중에.

QUESTION
Write를 했다 -> buffer에 들어가 있음
buffer에서 꺼내 압축 하여 physical write를 하기 전에 read요청이 들어오면 어떻게 할것인가.
=>  buffer에 있는 데이터를 그냥 꺼내라

문제점 page_mapping_get_free_physical_page_address
현재 block 내에서 연속된 페이지를 찾는데 block fragmentation이 생김. 
그런 빈 자리를 싱글 페이지가 들어가도록 유도해야 할듯.!!

*/
		/* 1. serialize 후 압축 */ 
		serialize();
		/*put header and buffer together*/
		UWORD compressed_size = compress(_write_buff, sizeof(struct wr_buff_t), _compressed_buff);
		uint32_t bus, chip, block, page;

		if( compressed_size >= FLASH_PAGE_SIZE * WRITE_BUFFER_LEN // 압축했는데 4페이지보다 크거나
				|| compressed_size == -1 // 압축의 리턴값이 -1(실패)일 때 압축하지 않고 physical write
		  ){  
			// 압축 하지 않고 write
			for(i=0; i< WRITE_BUFFER_LEN; i++){
				/* 2. 페이지 요청 */
				if(get_free_pages(ftl_base, ptr_ftl_context, 1, &bus, &chip, &block, &page)==-1){
					goto err;
				}

				/* 3. physical write */
				blueftl_user_vdevice_page_write(
						_ptr_vdevice,
						bus, chip, block, page,
						FLASH_PAGE_SIZE,
						(char *)_struct_write_buff.buff[FLASH_PAGE_SIZE*i]
						);

				/* 4. ftl 등록 */
				if(ftl_base->ftl_map_logical_to_physical(
							ptr_ftl_context, &_struct_write_buff.arr_lpa[i], bus, chip, block, page, 1, 0) == -1)
				{
					printf ("bluessd: map_logical_to_physical failed\n");
					goto err;
				}
			}
			printf("finish non-compredded write ");
		} else { // 압축된 것을 write
			// 압축된 페이지 계산
			uint32_t nr_compress_pages = compressed_size / FLASH_PAGE_SIZE + compressed_size % FLASH_PAGE_SIZE?1:0;
			uint32_t bus, chip, block, page;
			/* 2. 압축된 페이지 사이즈에 맞게 ftl 페이지 요청 */
			/*     if(get_free_pages(ftl_base, ptr_ftl_context, nr_compress_pages, &bus, &chip, &block, &page)==-1){
				   goto err;
				   }
				   printf("get nr %d pages succeed \n", nr_compress_pages);*/

			/* 3. physical write */
			uint32_t *ptr_compress_page;
			for(i = 0; i < nr_compress_pages; i++){
				if(get_free_pages(ftl_base, ptr_ftl_context, 1, &bus, &chip, &block, &page) == -1) {
					goto err;
				}
				ptr_compress_page = _compressed_buff + i * FLASH_PAGE_SIZE;
				blueftl_user_vdevice_page_write(
						_ptr_vdevice,
						bus, chip, block, page,
						FLASH_PAGE_SIZE,
						(char *)ptr_compress_page
						);
				/*3.3 mapping*/	
				/* 4. ftl 등록 */
				if(ftl_base->ftl_map_logical_to_physical(
							ptr_ftl_context, _struct_write_buff.arr_lpa, bus, chip, block, page, nr_compress_pages, 1) == -1)
				{
					printf ("bluessd: map_logical_to_physical failed\n");
					goto err;
				}
			} // end of all nr_compressed_pages

		}
		printf("map succeed\n"); 

		clean_buff();

	}
	return 0;

err:
	return -1;
}

int32_t get_free_pages(struct ftl_base_t* _ftl_base,
		struct ftl_context_t* ptr_ftl_context, 
		uint32_t nr_pages,
		uint32_t *ptr_bus,
		uint32_t *ptr_chip,
		uint32_t *ptr_block,
		uint32_t *ptr_page)
{
	if(_ftl_base->ftl_get_free_physical_page_address(
				ptr_ftl_context, nr_pages, ptr_bus, ptr_chip, ptr_block, ptr_page) == -1
	  ){
		/* 구현:: 공간 없음. GC 요청, page_mapping_get_free_physical_page_address 재시도, 안되면 에러*/
		if (_ftl_base->ftl_trigger_gc != NULL) {

			/* GC */
			if(_ftl_base->ftl_trigger_gc(ptr_ftl_context, *ptr_bus, *ptr_chip) == -1) {
				printf("bluessd: oops! garbage collection failed.\n");
				goto err;
			}

			/* 페이지요청 재시도 */
			if(_ftl_base->ftl_get_free_physical_page_address(
						ptr_ftl_context, nr_pages, ptr_bus, ptr_chip, ptr_block, ptr_page) == -1
			  ){
				printf("bluessd: there is not sufficient space in flash memory.\n");
				goto err;
			}
		} else {
			printf ("bluessd: garbage collection is not registered\n");
			goto err;
		}
	}
	return 0;

err:
	return -1;
}


void serialize(){
	uint8_t *ptr_buf_pos = _write_buff;

	memcpy(ptr_buf_pos, &_struct_write_buff.arr_lpa[0], sizeof(uint32_t));
	ptr_buf_pos += sizeof(uint32_t);
	memcpy(ptr_buf_pos, &_struct_write_buff.arr_lpa[1], sizeof(uint32_t));
	ptr_buf_pos += sizeof(uint32_t);
	memcpy(ptr_buf_pos, &_struct_write_buff.arr_lpa[2], sizeof(uint32_t));
	ptr_buf_pos += sizeof(uint32_t);
	memcpy(ptr_buf_pos, &_struct_write_buff.arr_lpa[3], sizeof(uint32_t));
	ptr_buf_pos += sizeof(uint32_t);

	memcpy(ptr_buf_pos, &_struct_write_buff.buff[FLASH_PAGE_SIZE*0], FLASH_PAGE_SIZE);
	ptr_buf_pos += FLASH_PAGE_SIZE;
	memcpy(ptr_buf_pos, &_struct_write_buff.buff[FLASH_PAGE_SIZE*1], FLASH_PAGE_SIZE);
	ptr_buf_pos += FLASH_PAGE_SIZE;
	memcpy(ptr_buf_pos, &_struct_write_buff.buff[FLASH_PAGE_SIZE*2], FLASH_PAGE_SIZE);
	ptr_buf_pos += FLASH_PAGE_SIZE;
	memcpy(ptr_buf_pos, &_struct_write_buff.buff[FLASH_PAGE_SIZE*3], FLASH_PAGE_SIZE);

}

void deserialize(){
	uint8_t *ptr_buf_pos = _write_buff;

	memcpy(&_struct_write_buff.arr_lpa[0], ptr_buf_pos, sizeof(uint32_t));
	ptr_buf_pos += sizeof(uint32_t);
	memcpy(&_struct_write_buff.arr_lpa[1], ptr_buf_pos, sizeof(uint32_t));
	ptr_buf_pos += sizeof(uint32_t);
	memcpy(&_struct_write_buff.arr_lpa[2], ptr_buf_pos, sizeof(uint32_t));
	ptr_buf_pos += sizeof(uint32_t);
	memcpy(&_struct_write_buff.arr_lpa[3], ptr_buf_pos, sizeof(uint32_t));
	ptr_buf_pos += sizeof(uint32_t);

	memcpy(&_struct_write_buff.buff[FLASH_PAGE_SIZE*0], ptr_buf_pos, FLASH_PAGE_SIZE);
	ptr_buf_pos += FLASH_PAGE_SIZE;
	memcpy(&_struct_write_buff.buff[FLASH_PAGE_SIZE*1], ptr_buf_pos, FLASH_PAGE_SIZE);
	ptr_buf_pos += FLASH_PAGE_SIZE;
	memcpy(&_struct_write_buff.buff[FLASH_PAGE_SIZE*2], ptr_buf_pos, FLASH_PAGE_SIZE);
	ptr_buf_pos += FLASH_PAGE_SIZE;
	memcpy(&_struct_write_buff.buff[FLASH_PAGE_SIZE*3], ptr_buf_pos, FLASH_PAGE_SIZE);

}
