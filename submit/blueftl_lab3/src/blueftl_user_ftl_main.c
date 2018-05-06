#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "blueftl_user.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_ftl_base.h"
#include "blueftl_char.h"
#include "blueftl_mapping_page.h"
#include "lzrw3.h"
#include "blueftl_util.h"

#define NR_PAGES	64

struct ftl_base_t _ftl_base;
struct ftl_context_t* _ptr_ftl_context = NULL;

extern int8_t _is_debugging_mode;

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

int find_data_in_chk (struct write_buffer_t* w_buf, uint32_t lpa){
	int i;

	for (i = 0; i < CHUNK_SIZE; i++) {
		if (w_buf->t_lpa[i] == lpa)
			return i;
		//else
		//	return -1;
	}
	return -1;
}

/* for lab 3 */
int32_t blueftl_user_write_chk(
		uint32_t lpa_curr,
		uint32_t lpa_begin,
		uint8_t* ptr_buffer,
		struct write_buffer_t* w_buf)
{
	uint8_t* final_buf;
	uint32_t bus, chip, block, page;
	uint8_t* ptr_lba_buff = ptr_buffer + 
		((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);
	uint8_t is_merge_needed = 0;

	if (w_buf->page_cnt < CHUNK_SIZE) {
		int is_new = 0;
		int i = 0;
		for (i = 0; i < w_buf->page_cnt; i++) {
			if (w_buf->t_lpa[i] == lpa_curr) {
				memcpy(w_buf->t_buf + i*_ptr_vdevice->page_main_size, ptr_lba_buff , _ptr_vdevice->page_main_size);
				is_new = 1;
			}
		}
		if (is_new != 1) {
			w_buf->t_lpa[w_buf->page_cnt] = lpa_curr;
			memcpy(w_buf->t_buf + w_buf->page_cnt*_ptr_vdevice->page_main_size, ptr_lba_buff , _ptr_vdevice->page_main_size); 

			w_buf->page_cnt++;
		}
	}

	if (w_buf->page_cnt == CHUNK_SIZE) {
		int flag_no_page = 0;
		UWORD compressed_size = 0;  
		int i;
		struct ftl_page_mapping_context_t* ptr_pg_mapping = _ptr_ftl_context->ptr_mapping;
		uint32_t t_bus, t_chip, t_block, t_page;
		int is_need_comp = 1;
		struct c_header tmp_ch;
		uint8_t* tmp_wbh;
		uint8_t* ptr_comp_buff;

		for(i = 0; i < w_buf->page_cnt; i++) {
			if (ptr_pg_mapping->ptr_pg_table[w_buf->t_lpa[i]] != PAGE_TABLE_FREE) {
				_ftl_base.ftl_get_mapped_physical_page_address (_ptr_ftl_context, w_buf->t_lpa[i], &bus, &chip, &block, &page);
				_ptr_ftl_context->chk_table[block * NR_PAGES + page].valid_page_cnt--;
				if (_ptr_ftl_context->chk_table[block * NR_PAGES + page].valid_page_cnt == 0) {
					_ftl_base.ftl_get_mapped_physical_page_address (_ptr_ftl_context, w_buf->t_lpa[i], &bus, &chip, &block, &page);

					struct flash_block_t* ptr_erase_block;
					ptr_erase_block = &_ptr_ftl_context->ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block];
					ptr_erase_block->list_pages[page].page_status = PAGE_STATUS_INVALID;
					ptr_erase_block->nr_valid_pages--;
					ptr_erase_block->nr_invalid_pages++;

					_ptr_ftl_context->chk_table[block * NR_PAGES + page].valid_page_cnt = 0;
					_ptr_ftl_context->chk_table[block * NR_PAGES + page].physical_page_cnt = 0;
				}

				ptr_pg_mapping->ptr_pg_table[w_buf->t_lpa[i]] = -1;
			}
		}

		if ((final_buf = (uint8_t*)malloc(sizeof(struct c_header) +  _ptr_vdevice->page_main_size * CHUNK_SIZE)) == NULL) {
			return -1;
		}


		for (i = 0; i < CHUNK_SIZE ; i++) {
			tmp_ch.lpa[i] = w_buf->t_lpa[i];
			tmp_ch.offset[i] = i;
		}
		tmp_wbh = (uint8_t*)malloc(sizeof(struct c_header) +  _ptr_vdevice->page_main_size * CHUNK_SIZE);
		memcpy(tmp_wbh, &tmp_ch, sizeof(struct c_header));
		memcpy(tmp_wbh + sizeof(struct c_header), w_buf->t_buf, _ptr_vdevice->page_main_size * CHUNK_SIZE);

		compressed_size = compress(tmp_wbh, _ptr_vdevice->page_main_size * CHUNK_SIZE + sizeof(struct c_header) , final_buf);


		if (compressed_size >= _ptr_vdevice->page_main_size * CHUNK_SIZE) {
			is_need_comp = 0;
			flag_no_page = CHUNK_SIZE;
		} else {
			is_need_comp = 1;
			flag_no_page = 1 + compressed_size/_ptr_vdevice->page_main_size; 
		}
		free(tmp_wbh);

		for (i = 0; i < CHUNK_SIZE ; i++) {
			if (i < flag_no_page) {
				if (is_need_comp == 1)
					ptr_comp_buff = final_buf + i * _ptr_vdevice->page_main_size;
				else if (is_need_comp == 0)
					ptr_comp_buff = w_buf->t_buf + i * _ptr_vdevice->page_main_size;

				if (_ftl_base.ftl_get_free_physical_page_address (_ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == -1) {
					if (_ftl_base.ftl_trigger_gc != NULL) {
						if (_ftl_base.ftl_trigger_gc (_ptr_ftl_context, bus, chip) == -1) {
							printf ("bluessd: oops! garbage collection failed.\n");
							return -1;
						}

						if (_ftl_base.ftl_get_free_physical_page_address (_ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == -1) {
							printf ("bluessd: there is not sufficient space in flash memory.\n");
							return -1;
						}
					} else if (_ftl_base.ftl_trigger_merge != NULL) {
						is_merge_needed = 1;
					} else {
						printf ("bluessd: garbage collection is not registered\n");
						return -1;
					}
				}

				blueftl_user_vdevice_page_write(
						_ptr_vdevice, 
						bus, chip, block, page,
						_ptr_vdevice->page_main_size, 
						(char*)ptr_comp_buff);

				struct flash_block_t* ptr_erase_block;
				ptr_erase_block = &_ptr_ftl_context->ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block];
				ptr_erase_block->list_pages[page].page_status = PAGE_STATUS_VALID;
				ptr_erase_block->nr_free_pages--;
				ptr_erase_block->nr_valid_pages++;

				if (i == 0) {
					t_bus = bus;
					t_chip = chip;
					t_block = block;
					t_page = page;
					_ftl_base.ftl_map_logical_to_physical (_ptr_ftl_context, w_buf->t_lpa[i], 
							bus, chip, block, page,
							2, -1, -1, -1);
				} else {
					_ftl_base.ftl_map_logical_to_physical (_ptr_ftl_context, w_buf->t_lpa[i], 
							bus, chip, block, page,
							t_bus, t_chip, t_block, t_page);
				}

				if (is_need_comp == 1) {
					_ptr_ftl_context->chk_table[block * NR_PAGES + page + i].is_compressed = 1;
				} else {
					_ptr_ftl_context->chk_table[block * NR_PAGES + page + i].is_compressed = 0;
				}

				_ptr_ftl_context->chk_table[block * NR_PAGES + page + i].physical_page_cnt = flag_no_page;
				_ptr_ftl_context->chk_table[block * NR_PAGES + page + i].valid_page_cnt = CHUNK_SIZE;
			} else {
				_ftl_base.ftl_map_logical_to_physical (_ptr_ftl_context, w_buf->t_lpa[i], 
						bus, chip, block, page,
						t_bus, t_chip, t_block, t_page);
			} 
			w_buf->t_lpa[i] = -1;
		}
		w_buf->page_cnt = 0;

		memset(w_buf->t_buf, 0xFF, CHUNK_SIZE * _ptr_vdevice->page_main_size);  
		/* by hj */
		free(final_buf);
	}

	return 0;
}

int32_t blueftl_user_read_chk (
		uint32_t lpa_curr,
		uint32_t lpa_begin,
		uint8_t* ptr_buffer,
		struct write_buffer_t* w_buf)
{
	uint32_t bus, chip, page, block;
	uint8_t* ptr_lba_buff = ptr_buffer + 
		((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);

	if (_ftl_base.ftl_get_mapped_physical_page_address (
				_ptr_ftl_context, lpa_curr, &bus, &chip, &block, &page) == 0) {
		int count = find_data_in_chk (w_buf, lpa_curr);
		if (count != -1) {
			memcpy(ptr_lba_buff, w_buf->t_buf + count * _ptr_vdevice->page_main_size, _ptr_vdevice->page_main_size);
		} else {
			uint32_t is_compressed = _ptr_ftl_context->chk_table[page].is_compressed;
			uint8_t* prev_dec = (uint8_t*)malloc(_ptr_vdevice->page_main_size * CHUNK_SIZE);
			uint8_t* after_dec = (uint8_t* )malloc( sizeof(struct c_header) + _ptr_vdevice->page_main_size * CHUNK_SIZE);
			memset(after_dec, 0xFF, (sizeof(struct c_header) + _ptr_vdevice->page_main_size * CHUNK_SIZE));
			memset(prev_dec, 0xFF, (_ptr_vdevice->page_main_size * CHUNK_SIZE));

			if (is_compressed) {
				int page_cnt = _ptr_ftl_context->chk_table[page].physical_page_cnt;
				int i;
				struct c_header* temp_header = (struct c_header *)malloc(sizeof(struct c_header));
				for (i = 0; i < page_cnt; i++) {
					blueftl_user_vdevice_page_read (
							_ptr_vdevice,
							bus, chip, block, page, 
							_ptr_vdevice->page_main_size, 
							(char*)prev_dec + i * _ptr_vdevice->page_main_size);
				}
				decompress(prev_dec, sizeof(prev_dec) , after_dec);


				memcpy(temp_header, after_dec, sizeof(struct c_header));

				for (i = 0; i < CHUNK_SIZE; i++) {
					if (temp_header->lpa[i] == lpa_curr)
						memcpy(ptr_lba_buff,  after_dec + sizeof(struct c_header) + temp_header->offset[i]*_ptr_vdevice->page_main_size, _ptr_vdevice->page_main_size);
				}
			} else {
				blueftl_user_vdevice_page_read (
						_ptr_vdevice,
						bus, chip, block, page, 
						_ptr_vdevice->page_main_size, 
						(char*)ptr_lba_buff);
			}
			free(prev_dec);
			free(after_dec);
		}
	} else {
		/* the requested logical page is not mapped to any physical page */
		/* simply ignore */
	}
	// free(prev_dec);
	// free(after_dec);
	return 0;
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
	struct write_buffer_t* w_buf = _ptr_ftl_context->write_buf;
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
				blueftl_user_read_chk(lpa_curr, lpa_begin, ptr_buffer, w_buf);	
			}
			break;

		case NETLINK_WRITE:
			for (lpa_curr = lpa_begin; lpa_curr < lpa_end; lpa_curr++) {
				ret = blueftl_user_write_chk(lpa_curr, lpa_begin, ptr_buffer, w_buf);	
			}
			break;

		default:
			printf ("bluessd: invalid I/O type (%u)\n", req_dir);
			ret = -1;
			goto failed;
	}
	ret = 0;

failed:
	/* blueftl_user_vdevice_req_done (_ptr_vdevice); */
	return ret;
}
