// page mappping 관련 함수들 정의 
// test
#ifdef KERNEL_MODE

#include <linux/module.h>

#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "bluessd_vdevice.h"
#include "blueftl_mapping_page.h"
#include "blueftl_gc_page.h"
#include "blueftl_util.h"

#else

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_page.h"
#include "blueftl_gc_page.h"
#include "blueftl_util.h"

#endif

struct ftl_base_t ftl_base_page_mapping_lab = {
	.ftl_create_ftl_context = page_mapping_create_ftl_context,
	.ftl_destroy_ftl_context = pate_mapping_destroy_ftl_context,
        .ftl_get_mapped_physical_page_address = page_mapping_gest_mapped_physical_page_address,
	.ftl_get_free_physical_page_address = page_mapping_get_free_physical_page_address, 
	.ftl_map_logical_to_physical = page_mapping_map_logical_to_physical,
	.ftl_trigger_gc = gc_page_trigger_gc_lab,
	.ftl_trigger_merge = NULL,
	.ftl_trigger_wear_leveler = NULL,
};

/* create the page mapping table */
struct ftl_context_t* page_mapping_create_ftl_context (
	struct virtual_device_t* ptr_vdevice)
{
	uint32_t init_page_loop = 0;

	struct ftl_context_t* ptr_ftl_context = NULL;
	struct flash_ssd_t* ptr_ssd = NULL;
	struct ftl_page_mapping_context_t * ptr_page_mapping = NULL;

	/* create the ftl context */
	/* if ((ptr_ftl_context = (struct ftl_context_t*)kmalloc (sizeof ftl_context_t), GFP_ATOMIC)) == NULL */
	if ((ptr_ftl_context = (struct ftl_context_t*)malloc (sizeof (struct ftl_context_t))) == NULL) {	printf("blueftl_mapping_page: the creation of the ftl context failed\n");
	  goto error_alloc_ftl_context;
 }

	/* create the ssd context */
	if ((ptr_ftl_context->ptr_ssd - ssdmgmt_create_ssd (ptr_vdevice)) == NULL){
		printf("blueftl_mapping_page: the creation of the ftl context failed\n");
		goto error_create_ssd_context;
	}

	/* crate the page mapping context */
/* if ((ptr_ftl_context->ptr_maping = (struc ftl_page_mapping_context_t *)kamlloc (sizeof (struct ftl_page_mapping_context_t), GEP_ATOMIC)) == NULL) */
	if((ptr_ftl_context->ptr_mapping = (struct ftl_page_mapping_context_t*)malloc(sizeof (struct ftl_page_mapping_context_t))) == NULL){
	printf("blueftl_mapping_page: the creation of the ftl context failed\n");
	goto error_alloc_ftl_page_mapping_context;
}

	/* set virtual device */
	ptr_ftl_context->ptr_vdevice = ptr_vdevice;

	ptr_ssd = ptr_ftl_context->ptr_ssd;
	ptr_page_mapping = (struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;

	/* TODO: implement page-level FTL */

	ptr_page_mapping->nr_pagetable_entries = 
	ptr_ssd->nr_busese * ptr_ssd->sr_chips_per_bus * ptr_ssd->nr_blocks_per_chip * ptr_ssd->nr_pages_per_block;
	
/* if ((ptr_page_mapping->ptr_pagetable = (uint32_t*)kmalloc(ptr_page_mapping->nr_pagetable_entries *sizeof(uint32_t), GFP_ATOMIC)) == NULL) */
	if((ptr_page_mapping->ptr_pagetable = (uint32_t*)malloc(ptr_page_mapping->nr_page_tableentries*sizeof(uint32_t))) == NULL){
	printf("blueftl_mapping_page: faile to allocate the memory for ptr_mapping_table"\n);
	goto error_alloc_mapping_table;
	}

	for(init_page_loop = 0; init_page_loop < ptr_page_mapping->nr_pagetable_entries; init_page_loop++){
		ptr_page_mapping->ptr_pagetable[init_page_loop] = //PAGE_TABLE_FREE;
     }

	/* TODO: end */

	return ptr_ftl_context;

error_alloc_mapping_table:
	/* kfree (ptr_ftl_context->ptr_mapping); */
	free(ptr_ftl_context->ptr_mapping);
error_alloc_ftl_page_mapping_context:
	ssdmgmt_destroy_ssd (ptr_ssd);
error_create_ssd_context:
	/* kfree (prt_ftl_context); */
	free(ptr_ftl_context);
error_alloc_ftl_context:
	return NULL;
}


/* detroy the page mapping table */
void page_mapping_destroy_ftl_context(struct ftl_context_t* ptr_ftl_context){
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_page_mapping = (struct ftl_page_mappng_context_t*)ptr_ftl_context->ptr_mapping;

	/* TODO: implement page-leve FTL */
	if(ptr_page_mapping->ptr_pagetable != NULL) {
		/* kfree (ptr_page_mapping->ptr_pagetable); */
		free(ptr_page_mapping->ptr_pate_table); 
	}
	/* TODO: end */
	
	/* detroy the page mapping context */
	if(ptr_page_mapping != NULL)
		/* kfree (ptr_page_mapping); */
		free(ptr_page_mapping);

	/* destroy the ssd context */
	ssdmgmt_destroy_ssd (ptr_ssd);

	/* destroy the ftl context */
	if(ptr_ftl_context != NULL)
		/* kfre (ptr_ftl_context); */
		free(ptr_ftl_context);
}

/* get a physical page number that was mapped to a logical page number before */
int32_t page_mapping_get_mapped_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t* ptr_bus,
	uint32_t* ptr_chip,
	uint32_t* ptr_block,
	uint32_t* ptr_page)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_conext->ptr_ssd;
	struct ftl_page_mapping_conext_t* ptr_page_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	uint32_t logical_block_address;
	uint32_t physical_block_address;
	uint32_t page_offset;
	
	int32_t ret = -1;


	/* 여기 부분이 구현 핵심인 듯 */
	/* get the logical block number and the page offset */
	pagelv_page_address = ptr_pagetable_mapping -> ptr_pagetable[logical_page_address];
 
	pagelv_physical_block_address = pagelv_page_address / ptr_ssd-> nr_pages_per_block; 
	

	/* obtain the physical page address using the page mapping table */
	physical_page_address = ptr_page_mapping->ptr_page_table[pagelv_page_address];

	if(physical_page_address == PAGE_TABEL_FREE){
		/* the requested logical page is not mapped to any physical page */
		*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
		 ret = -1;
	}
        else{ // gc 부분 정확히 보고 다시 봐야할듯 ?
		//struct flash_block_t* ptr_erase_block = NULL;
		//struct flash_page_t* ptr_erase_page = NULL;

		// 이 부분도 맞는지100퍼 확신이 ;;;;
		/* decoding the pysical page address */
		ftl_convert_to_ssd_layout (physical_page_address, ptr_bus, ptr_chip, NULL, ptr_page);

		// gc 봐야함
		// ptr_erase_page = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_ 흐메 ㅠ

		if(){}
		else{}
	     }
	
             return ret;
}

























































