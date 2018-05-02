#ifdef KERNEL_MODE

#include <linux/module.h>

#else

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#endif

#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_gc_page.h"
#include "blueftl_mapping_page.h"
#include "blueftl_util.h"
#include "blueftl_read_write_mgr.h"

/**
	page_mapping_get_mapped_physical_page_address:
		logical page를 주면 physical page를 리턴함
	page_mapping_get_free_physical_page_address:
		(write 직전) free page를 리턴함
	page_mapping_map_logical_to_physical:
		(write 직후) logical page와 physical page를 매핑시킴.
**/

struct ftl_base_t ftl_base_page_mapping = {
	.ftl_create_ftl_context = page_mapping_create_ftl_context,
	.ftl_destroy_ftl_context = page_mapping_destroy_ftl_context,
	.ftl_get_mapped_physical_page_address = page_mapping_get_mapped_physical_page_address, 
	.ftl_get_free_physical_page_address = page_mapping_get_free_physical_page_address,
	.ftl_map_logical_to_physical = page_mapping_map_logical_to_physical,
#if GC_ON
	.ftl_trigger_gc = gc_page_trigger_gc,
#else
	.ftl_trigger_gc = NULL,
#endif
	.ftl_trigger_merge = NULL,
	.ftl_trigger_wear_leveler = NULL,
};


/* create the page mapping table */
struct ftl_context_t* page_mapping_create_ftl_context(struct virtual_device_t* ptr_vdevice)
{
	uint32_t init_pg_loop = 0;

	struct ftl_context_t* ptr_ftl_context = NULL;
	struct flash_ssd_t* ptr_ssd = NULL;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = NULL;

	timer_init();
	srand(time(NULL));
	//printf("-S----%s \n", __func__);

	/* create the ftl context */
	/*if ((ptr_ftl_context = (struct ftl_context_t*)kmalloc (sizeof (struct ftl_context_t), GFP_ATOMIC)) == NULL) {*/
	if ((ptr_ftl_context = (struct ftl_context_t*)malloc (sizeof (struct ftl_context_t))) == NULL) {
		printf ("blueftl_mapping_page: the creation of the ftl context failed\n");
		goto error_alloc_ftl_context;
	}

	/* create the ssd context */
	if ((ptr_ftl_context->ptr_ssd = ssdmgmt_create_ssd (ptr_vdevice)) == NULL) {
		printf ("blueftl_mapping_page: the creation of the ftl context failed\n");
		goto error_create_ssd_context;
	}
	
	/* create the page mapping context */
	/*if ((ptr_ftl_context->ptr_mapping = (struct ftl_page_mapping_context_t *)kmalloc (sizeof (struct ftl_page_mapping_context_t), GFP_ATOMIC)) == NULL) {*/
	if ((ptr_ftl_context->ptr_mapping = (struct ftl_page_mapping_context_t *)malloc (sizeof (struct ftl_page_mapping_context_t))) == NULL) {
		printf ("blueftl_mapping_page: the creation of the ftl context failed\n");
		goto error_alloc_ftl_page_mapping_context;
	}

	/* set virtual device */
	ptr_ftl_context->ptr_vdevice = ptr_vdevice;

	ptr_ssd = ptr_ftl_context->ptr_ssd;
	ptr_pg_mapping = (struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;

	/* TODO: implement page-level FTL */

	ptr_pg_mapping->nr_pg_table_entries = ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus * ptr_ssd->nr_blocks_per_chip * ptr_ssd->nr_pages_per_block;
	printf(" %d %d %d %d = %d \n", ptr_ssd->nr_buses, ptr_ssd->nr_chips_per_bus, ptr_ssd->nr_blocks_per_chip, ptr_ssd->nr_pages_per_block, ptr_pg_mapping->nr_pg_table_entries);
	/*if ((ptr_pg_mapping->ptr_pg_table = (uint32_t*)kmalloc (ptr_pg_mapping->nr_pg_table_entries * sizeof (uint32_t), GFP_ATOMIC)) == NULL) {*/
	if ((ptr_pg_mapping->ptr_pg_table = (uint32_t*)malloc (ptr_pg_mapping->nr_pg_table_entries * sizeof (uint32_t))) == NULL) {
		printf ("blueftl_mapping_page: failed to allocate the memory for ptr_mapping_table\n");
		goto error_alloc_mapping_table;
	}

	/* initialize ptr_chunk_table */
	if((ptr_pg_mapping->ptr_chunk_table = (struct chunk_table_t *)malloc(ptr_pg_mapping->nr_pg_table_entries * sizeof(struct chunk_table_t *))) == NULL) {
		printf ("blueftl_mapping_page: failed to allocate the memory for ptr_chunk_table\n");
		goto error_alloc_ptr_chunk_table;
	}

	for (init_pg_loop = 0; init_pg_loop < ptr_pg_mapping->nr_pg_table_entries; init_pg_loop++) {
		ptr_pg_mapping->ptr_pg_table[init_pg_loop] = PAGE_TABLE_FREE;
		ptr_pg_mapping->ptr_chunk_table[init_pg_loop].valid_count = 0;
		ptr_pg_mapping->ptr_chunk_table[init_pg_loop].physical_page_len = 0;
		ptr_pg_mapping->ptr_chunk_table[init_pg_loop].is_compressed = false;
	}
	
	// int b,o;
	// for(b=0; b<1024; b++){ for(o=0; o<64; o++) printf("%d ", ptr_ssd->list_buses[0].list_chips[0].list_blocks[b].list_pages[o].page_status);}
	gc_page_trigger_init(ptr_ftl_context);
	/* TODO: end */
	
	return ptr_ftl_context;

error_alloc_ptr_chunk_table:
	free (((struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping)->ptr_pg_table);

error_alloc_mapping_table:
	/*kfree (ptr_ftl_context->ptr_mapping);*/
	free (ptr_ftl_context->ptr_mapping);

error_alloc_ftl_page_mapping_context:
	ssdmgmt_destroy_ssd (ptr_ssd);

error_create_ssd_context:
	/*kfree (ptr_ftl_context);*/
	free (ptr_ftl_context);

error_alloc_ftl_context:
	return NULL;
}

/* destroy the page mapping table */
void page_mapping_destroy_ftl_context (struct ftl_context_t* ptr_ftl_context)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
    //mskim

	//printf("-S----%s \n", __func__);
	/* TODO: implement page-level FTL */

	if (ptr_pg_mapping->ptr_chunk_table != NULL){
		free(ptr_pg_mapping->ptr_chunk_table);
	}

	if (ptr_pg_mapping->ptr_pg_table != NULL) {
		/*kfree (ptr_pg_mapping->ptr_pg_table);*/
		free (ptr_pg_mapping->ptr_pg_table);
	}
	/* TODO: end */

	/* destroy the page mapping context */
	if (ptr_pg_mapping != NULL)
		/*kfree (ptr_pg_mapping);*/
		free (ptr_pg_mapping);

	/* destroy the ssd context */
	ssdmgmt_destroy_ssd (ptr_ssd);

	/* destory the ftl context */
	if (ptr_ftl_context != NULL)
		/*kfree (ptr_ftl_context);*/
		free (ptr_ftl_context);
	//printf("-E----%s \n\n", __func__);
}

/* get a physical page number that was mapped to a logical page number before */
int32_t page_mapping_get_mapped_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	uint32_t physical_page_address;
    /* mskim: pageoffset <= logical_page_address*/

	int32_t ret = -1;
	//printf("-S----%s \n", __func__);

	/* obtain the physical page address using the page mapping table */
	physical_page_address = ptr_pg_mapping->ptr_pg_table[logical_page_address];

	if (physical_page_address == PAGE_TABLE_FREE) {
		*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
		ret = -1;
	} else {
		struct flash_page_t* ptr_flash_page = NULL;

		ftl_convert_to_ssd_layout (physical_page_address, ptr_bus, ptr_chip, ptr_block, ptr_page);
		
		ptr_flash_page = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_blocks[*ptr_block].list_pages[*ptr_page];
		if (ptr_flash_page->page_status == PAGE_STATUS_FREE) {
			/* the logical page must be mapped to the corresponding physical page */
			*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
			printf("physical null\n");
			ret = -1;
		} else {
			ret = 0;
		}
	}

	//printf("-E----%s||%d \n\n", __func__, ret);
	return ret;
}

struct flash_block_t *_current_block = (struct flash_block_t *)-1;
uint32_t _page_offset;

int32_t find_page_in_block(struct flash_ssd_t* ptr_ssd, uint32_t nr_pages, uint32_t *pbus, uint32_t *pchip, uint32_t *pblock){
	
	uint32_t i;
	// 현재 블록의 빈 페이지를 찾음
	if(_current_block->is_reserved_block == 0){
		for(_page_offset=0; _page_offset < ptr_ssd->nr_pages_per_block; _page_offset++){
			
			// FREE PAGE를 찾음 
			if(_current_block->list_pages[_page_offset].page_status==PAGE_STATUS_FREE){
				
				// FREE PAGE를 시작으로 nr_pages 길이만큼 free page가 존재하면 return 0
				// nr_page 길이만큼 free page를 찾다가 free page가 아닌 페이지가 나오면 retrun -1
				for(i=1; i<nr_pages; i++){
					if(_current_block->list_pages[_page_offset+i].page_status!=PAGE_STATUS_FREE){
						continue;
					}
				}
				return 0;
			}
		}
	}


	(*pblock)++;
	if((*pblock) >= ptr_ssd->nr_blocks_per_chip){
		(*pchip)++;
		(*pblock)=0;
		if((*pchip) >= ptr_ssd->nr_chips_per_bus){
			(*pbus)++;
			(*pchip)=0;
			if((*pbus) >= ptr_ssd->nr_buses) (*pbus)=0;
		}
	}
	_current_block = &(ptr_ssd->list_buses[*pbus].list_chips[*pchip].list_blocks[*pblock]);
	return -1;
}

int32_t find_page(struct flash_ssd_t* ptr_ssd, uint32_t nr_pages){
	if(_current_block == (struct flash_block_t *)-1){
		_current_block = ssdmgmt_get_free_block(ptr_ssd, 0, 0);
		_page_offset=0;
		return 0;
	}
	uint32_t org_bus = _current_block->no_bus;
	uint32_t org_chip = _current_block->no_chip;
	uint32_t org_block = _current_block->no_block;
	uint32_t bus = org_bus;
	uint32_t chip = org_chip;
	uint32_t block = org_block;


	// if(find_page_in_block(ptr_ssd, nr_pages, &bus, &chip, &block)==0) return 0;
	do{
		if(find_page_in_block(ptr_ssd, nr_pages, &bus, &chip, &block)==0) return 0;
	}while(!(org_bus==bus && org_chip==chip && org_block==block));
	// printf("*\n");
	return -1;
}

/* get a free physical page address */
int32_t page_mapping_get_free_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t nr_pages,
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page)
{
	//printf("-S----%s \n", __func__);

	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;

	if(find_page(ptr_ssd, nr_pages)==-1) { goto need_gc; }
	*ptr_bus = _current_block->no_bus;
	*ptr_chip = _current_block->no_chip;
	*ptr_block = _current_block->no_block;
	*ptr_page = _page_offset;
	
	//printf("-E----%s \n\n", __func__);

	return 0;


need_gc:
	// printf("ngc");
	return -1;
}

void clear_prev_ppa(
	struct ftl_context_t *ptr_ftl_context, 
	uint32_t logical_page_address,
	uint32_t bus,
	uint32_t chip,
	uint32_t block,
	uint32_t page
	)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	struct chunk_table_t *ptr_chunk_table = ((struct ftl_page_mapping_context_t*)ptr_ftl_context)->ptr_chunk_table;
	struct flash_block_t *old_block;
	uint32_t rbus, rchip, rblock, rpage;
	uint32_t new_ppa, old_ppa, i;

	old_ppa = ptr_pg_mapping->ptr_pg_table[logical_page_address];
	new_ppa = ftl_convert_to_physical_page_address (bus, chip, block, page);
	
	/* 해당 lpa가 update인지 확인, invalidation이 필요한지 확인 */
	if ((old_ppa!=-1) &&(new_ppa != old_ppa)) {
		/* update이면 invalidation 함
			1. 청크 테이블에 valid 카운트 감소
			2. valid가 0이 되면, 압축여부에 관계없이 동작함 .
				1. 압축된 모든 페이지 invalidation, lpa 클리어
				2. 블록에 invalid 페이지 증가, valid page 감소
		*/
		ptr_chunk_table[old_ppa].valid_count--;
		
		//valid한 page가 없을 때, 압축된 모든 페이지를 invalidation
		if(ptr_chunk_table[old_ppa].valid_count==0){
			ftl_convert_to_ssd_layout(old_ppa, &rbus, &rchip, &rblock, &rpage);
			old_block = &(ptr_ssd->list_buses[rbus].list_chips[rchip].list_blocks[rblock]);
			for(i=0; i<ptr_chunk_table[old_ppa].physical_page_len; i++){
				old_block->list_pages[rpage+i].page_status = PAGE_STATUS_INVALID;
				old_block->list_pages[rpage+i].no_logical_page_addr = -1;
				old_block->nr_valid_pages--;
				old_block->nr_invalid_pages++;
			}
			old_block->last_modified_time = timer_get_timestamp_in_us();
		}
	}
}

int32_t mapping_logical_to_physical(
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address,
	uint32_t bus,
	uint32_t chip,
	uint32_t block,
	uint32_t page
	)
{
	struct flash_block_t *new_block;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	uint32_t ppa = ftl_convert_to_physical_page_address(bus, chip, block, page);
	/* ppa에 해당하는 정보 등록. */
	new_block = &(ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block]);
	
	uint32_t prev_valid=new_block->nr_valid_pages;
	uint32_t prev_invalid=new_block->nr_invalid_pages;

	if(new_block->list_pages[page].page_status == PAGE_STATUS_FREE){
		new_block->list_pages[page].page_status = PAGE_STATUS_VALID;
		new_block->list_pages[page].no_logical_page_addr = logical_page_address;
		new_block->nr_valid_pages++;
		new_block->nr_free_pages--;

		// 매핑 테이블 등록
		ptr_pg_mapping->ptr_pg_table[logical_page_address] = ppa;
	} else {
		return -1;
	}

	new_block->last_modified_time = timer_get_timestamp_in_us();
	if(prev_valid==0 && prev_invalid==0){
		ptr_ssd->list_buses[new_block->no_bus].list_chips[new_block->no_chip].nr_free_blocks--;
		ptr_ssd->list_buses[new_block->no_bus].list_chips[new_block->no_chip].nr_dirty_blocks++;
	}

	return 0;
}

/* map a logical page address to a physical page address */
int32_t page_mapping_map_logical_to_physical (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t *logical_page_address,
	uint32_t bus,
	uint32_t chip,
	uint32_t block,
	uint32_t page,
	uint32_t nr_pages,
	bool is_compressed)
{
	struct chunk_table_t *ptr_chunk_table = ((struct ftl_page_mapping_context_t*)ptr_ftl_context)->ptr_chunk_table;
	uint32_t ppa = ftl_convert_to_physical_page_address (bus, chip, block, page);
	uint32_t i;

	/**
		매핑에서 고려해야 할 것,
		0. 이전 페이지들의 invalidation 필요 여부
		1. 페이지: status, lpa --> 의미없어짐..
		2. 블록: free / valid / invalid 페이지 카운트
		3. 칩: free / dirty 블록 카운트 
		4. 매핑테이블: ppa
		5. 청크테이블: 압축여부, 페이지len, valid 카운트
	**/
	if(is_compressed){
		for(i=0; i<WRITE_BUFFER_LEN; i++){
			clear_prev_ppa(ptr_ftl_context, logical_page_address[i], bus, chip, block, page+i);
			if(mapping_logical_to_physical(ptr_ftl_context, logical_page_address[i], bus, chip, block, page+i) == -1){
				goto err;
			}
		}

		// 청크 테이블 등록
		for(i=0; i<nr_pages; i++){
			ptr_chunk_table[ppa+i].valid_count = WRITE_BUFFER_LEN;
			ptr_chunk_table[ppa+i].physical_page_len = nr_pages;
			ptr_chunk_table[ppa+i].is_compressed = 1;
		}
	} else {
		clear_prev_ppa(ptr_ftl_context, *logical_page_address, bus, chip, block, page);
		if(mapping_logical_to_physical(ptr_ftl_context, *logical_page_address, bus, chip, block, page) == -1){
			goto err;
		}
		ptr_chunk_table[ppa].valid_count = 1;
		ptr_chunk_table[ppa].physical_page_len = 1;
		ptr_chunk_table[ppa].is_compressed = 0;
	}

	//printf("-E----%s||%d \n", __func__,ret);
	return 0;

err:
	printf ("blueftl_mapping_block: the physical page address is already used (%u:%u,%u,%u,%u)\n", ppa, bus, chip, block, page);
	return -1;
}

