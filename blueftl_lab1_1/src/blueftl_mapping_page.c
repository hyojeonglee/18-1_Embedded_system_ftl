// page mappping 관련 함수들 정의 
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
//#include "blueftl_gc_page.h"
#include "blueftl_util.h"

#endif

struct ftl_base_t ftl_base_page_mapping_lab = {
	.ftl_create_ftl_context = page_mapping_create_ftl_context,
	.ftl_destroy_ftl_context = page_mapping_destroy_ftl_context,
        .ftl_get_mapped_physical_page_address = page_mapping_get_mapped_physical_page_address,
	.ftl_get_free_physical_page_address = page_mapping_get_free_physical_page_address, 
	.ftl_map_logical_to_physical = page_mapping_map_logical_to_physical,
//	.ftl_trigger_gc = gc_page_trigger_gc_lab,
	.ftl_trigger_gc = NULL,
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
	struct ftl_page_mapping_context_t* ptr_page_mapping = NULL;

	/* create the ftl context */
	/* if ((ptr_ftl_context = (struct ftl_context_t*)kmalloc (sizeof ftl_context_t), GFP_ATOMIC)) == NULL */
	if ((ptr_ftl_context = (struct ftl_context_t*)malloc (sizeof (struct ftl_context_t))) == NULL) {	printf("blueftl_mapping_page: the creation of the ftl context failed\n");
	  goto error_alloc_ftl_context;
 }

	/* create the ssd context */
	if ((ptr_ftl_context->ptr_ssd = ssdmgmt_create_ssd (ptr_vdevice)) == NULL){
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
	ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus * ptr_ssd->nr_blocks_per_chip * ptr_ssd->nr_pages_per_block;
	
/* if ((ptr_page_mapping->ptr_pagetable = (uint32_t*)kmalloc(ptr_page_mapping->nr_pagetable_entries *sizeof(uint32_t), GFP_ATOMIC)) == NULL) */
	if((ptr_page_mapping->ptr_pagetable = (uint32_t*)malloc(ptr_page_mapping->nr_pagetable_entries*sizeof(uint32_t))) == NULL){
	printf("blueftl_mapping_page: faile to allocate the memory for ptr_mapping_table \n");
	goto error_alloc_mapping_table;
	}

	for(init_page_loop = 0; init_page_loop < ptr_page_mapping->nr_pagetable_entries; init_page_loop++){
		ptr_page_mapping->ptr_pagetable[init_page_loop] = PAGE_TABLE_FREE;
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
	struct ftl_page_mapping_context_t* ptr_page_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	/* TODO: implement page-leve FTL */
	if(ptr_page_mapping->ptr_pagetable != NULL) {
		/* kfree (ptr_page_mapping->ptr_pagetable); */
		free(ptr_page_mapping->ptr_pagetable); 
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
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_page_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	
	uint32_t pagelv_physical_page_address;

	int32_t ret = -1;
	
	/* obtain the physical page address using the page mapping table */
	pagelv_physical_page_address = ptr_page_mapping-> ptr_pagetable[logical_page_address];
	

	if(pagelv_physical_page_address == PAGE_TABLE_FREE){
		/* the requested logical page is not mapped to any physical page */
		*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
		 ret = -1;
	}
        else{ 
		//struct flash_block_t* ptr_erase_block = NULL;

		struct flash_page_t* ptr_erase_page = NULL;

		
		/* decoding the physical page address */
		ftl_convert_to_ssd_layout (pagelv_physical_page_address, ptr_bus, ptr_chip, ptr_block, ptr_page);

		ptr_erase_page = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_blocks[*ptr_block].list_pages[*ptr_page];

		if(ptr_erase_page->page_status == PAGE_STATUS_FREE){
			/* the logical page must be mapped to the corresponding physical page */
                  	*ptr_bus = *ptr_chip = *ptr_block= *ptr_page = -1;
			ret = -1;
		}

		else{
			ret = 0; 
	     	}	
	}	
             return ret;
}

/* get a free physical page address */
int32_t page_mapping_get_free_physical_page_address(
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t* ptr_bus,
	uint32_t* ptr_chip,
	uint32_t* ptr_block,
	uint32_t* ptr_page)
{
	struct flash_block_t* ptr_erase_block;
	struct flash_page_t* ptr_erase_page;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_page_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	uint32_t pagelv_physical_page_address;
	uint32_t pagelv_physical_block_address;
	uint32_t pagelv_page_offset;
 
        /* obtain the physical page address,the physical block address and the page offset 
           using the page mapping table */
        pagelv_physical_page_address = ptr_page_mapping->ptr_pagetable[logical_page_address];
	pagelv_physical_block_address = pagelv_physical_page_address / ptr_ssd->nr_pages_per_block;
	pagelv_page_offset = pagelv_physical_page_address % ptr_ssd->nr_pages_per_block;

	if(pagelv_physical_page_address != PAGE_TABLE_FREE) {
		/* encoding the ssd layout to a physical block address in pagelv */
		ftl_convert_to_ssd_layout (pagelv_physical_page_address, ptr_bus, ptr_chip, ptr_block, ptr_page);

		*ptr_page = pagelv_physical_page_address;

 		/* see if the logical page can be written to the physical block found */
		ptr_erase_block = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_blocks[*ptr_block];
		ptr_erase_page = &ptr_erase_block->list_pages[pagelv_page_offset];                

		if(ptr_erase_page->page_status == PAGE_STATUS_FREE) {
			uint32_t page_loop = 0;

			/* check the SOP restriction */
			for(page_loop = pagelv_page_offset; page_loop < ptr_ssd->nr_pages_per_block; page_loop++){
			     if(ptr_erase_block->list_pages[page_loop].page_status != PAGE_STATUS_FREE){
				goto need_gc;                                                         }
				}	

			/* OK, We can use this page for writing */
			*ptr_page = pagelv_page_offset;           } 
		else{
			/* The target page is already used, so it is required to perform GC */
				goto need_gc;
		    }
	}

	else{
             // 이 부분 잘 모르겠음
	     //struct flash_page_t* ptr_erase_page = NULL;
	     struct flash_block_t* ptr_erase_block = NULL;

	
	     /* get the free page from the page mapping table */
	     ptr_erase_block = ssdmgmt_get_free_block(ptr_ssd, 0, 0);
	
	       *ptr_bus = ptr_erase_block->no_bus;
               *ptr_chip = ptr_erase_block->no_chip;
               *ptr_block = ptr_erase_block->no_block;
	       *ptr_page = pagelv_page_offset;
            }

	return 0;

need_gc:
	return -1;
}

/* map a logical page address to a physical page address */
int32_t page_mapping_map_logical_to_physical (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address,
	uint32_t bus,
	uint32_t chip,
	uint32_t block,
	uint32_t page)
{
	struct flash_block_t* ptr_erase_block;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_page_mapping = (struct ftl_page_mapping_context_t*) ptr_ftl_context->ptr_mapping;

	uint32_t pagelv_physical_page_address;
      //uint32_t pagelv_physical_block_address;
	uint32_t pagelv_page_offset;

	int32_t ret = -1;

 	/* get the logical_page_address and the physical block address in pagelv */
        pagelv_physical_page_address = ptr_page_mapping -> ptr_pagetable[logical_page_address];
      //pagelv_physical_block_address = pagelv_physical_page_address / ptr_ssd->nr_pages_per_block;
	pagelv_page_offset = pagelv_physical_page_address % ptr_ssd->nr_pages_per_block;	

        
	if(pagelv_page_offset != page) {
                /* the page offset of the logical page address must be the same as the page address */
                printf ("blueftl_mapping_page: there is something wrong with page mapping (%u != %u)\n", pagelv_page_offset, page);
                return -1;
        }
	
	/* see if the given logical page is already mapped or not */
	if(pagelv_physical_page_address == PAGE_TABLE_FREE){
		/* encoding the ssd layout to a physical page address*/
	pagelv_physical_page_address = ftl_convert_to_physical_page_address (bus,chip,block,page);

		/* update the page mapping table */
		ptr_page_mapping->ptr_pagetable[logical_page_address] = pagelv_physical_page_address;
	}

	/* see if the given page is already written or not */
	ptr_erase_block = &ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block];
	if(ptr_erase_block->list_pages[pagelv_page_offset].page_status == PAGE_STATUS_FREE){
		/* make it valid */
		ptr_erase_block->list_pages[pagelv_page_offset].page_status = PAGE_STATUS_VALID;

		/* increase the number of valid pages while decreasing the number of free pages */
		ptr_erase_block->nr_valid_pages++;
		ptr_erase_block->nr_free_pages--;

		ret = 0;
	 }

	 else {
                /*hmmm... the physical page that corresponds to the logical page must be free */
                printf ("blueftl_mapping_page: the physical page address is already used (%u:%u,%u,%u,%u)\n",pagelv_physical_page_address, bus, chip, block, page);
                ret = -1;
        }
	
	return ret;
}
