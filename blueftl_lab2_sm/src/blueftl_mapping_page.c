#ifdef KERNEL_MODE

#include <linux/module.h>

#else

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#endif

#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_page.h"
#include "blueftl_gc_page.h"
#include "blueftl_util.h"


struct ftl_base_t ftl_base_page_mapping = {
	.ftl_create_ftl_context = page_mapping_create_ftl_context,
	.ftl_destroy_ftl_context = page_mapping_destroy_ftl_context,
	.ftl_get_mapped_physical_page_address = page_mapping_get_mapped_physical_page_address,
	.ftl_get_free_physical_page_address = page_mapping_get_free_physical_page_address,
	.ftl_map_logical_to_physical = page_mapping_map_logical_to_physical,
	.ftl_trigger_gc = gc_page_trigger_gc_lab, // for test greedy policy
	.ftl_trigger_merge = NULL,
	.ftl_trigger_wear_leveler = NULL,
};

/* create the page mapping table */
struct ftl_context_t* page_mapping_create_ftl_context (
	struct virtual_device_t* ptr_vdevice)
{
	uint32_t init_pg_loop = 0;

	struct ftl_context_t* ptr_ftl_context = NULL;
	struct flash_ssd_t* ptr_ssd = NULL;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = NULL;

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
	/*if ((ptr_ftl_context->ptr_mapping = (struct ftl_block_mapping_context_t *)kmalloc (sizeof (struct ftl_block_mapping_context_t), GFP_ATOMIC)) == NULL) {*/
	if ((ptr_ftl_context->ptr_mapping = (struct ftl_page_mapping_context_t *)malloc (sizeof (struct ftl_page_mapping_context_t))) == NULL) {
		printf ("blueftl_mapping_page: the creation of the ftl context failed\n");
		goto error_alloc_ftl_page_mapping_context;
	}

	/* set virtual device */
	ptr_ftl_context->ptr_vdevice = ptr_vdevice;

	
	// initialize latest info
	ptr_ftl_context->latest_chip = -1;
	ptr_ftl_context->latest_bus = -1;
	ptr_ftl_context->latest_block = -1;


	ptr_ssd = ptr_ftl_context->ptr_ssd;
	ptr_pg_mapping = (struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;

	/* TODO: implement block-level > page-level FTL */

	
	ptr_pg_mapping->nr_pg_table_entries = 
		ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus * ptr_ssd->nr_blocks_per_chip * ptr_ssd->nr_pages_per_block; 

	printf(" %d %d %d %d = %d \n", ptr_ssd->nr_buses, ptr_sssd->nr0-chips_per_bus, ptr_ssd->nr_blocks_per_chip, ptr_ssd->nr_pages_per_block, ptr_pg_mapping->nr_pg_table_entries);

	/*if ((ptr_blk_mapping->ptr_blk_table = (uint32_t*)kmalloc (ptr_blk_mapping->nr_blk_table_entries * sizeof (uint32_t), GFP_ATOMIC)) == NULL) {*/
	if ((ptr_pg_mapping->ptr_pg_table = (uint32_t*)malloc (ptr_pg_mapping->nr_pg_table_entries * sizeof (uint32_t))) == NULL) {
		printf ("blueftl_mapping_page: failed to allocate the memory for ptr_mapping_table\n");
		goto error_alloc_mapping_table;
	}

	/* Initialize */
	for (init_pg_loop = 0; init_pg_loop < ptr_pg_mapping->nr_pg_table_entries; init_pg_loop++) {
		ptr_pg_mapping->ptr_pg_table[init_pg_loop] = PAGE_TABLE_FREE;
	}

	gc_page_trigger_init(ptr_ftl_context);

	/* TODO: end */

	return ptr_ftl_context;


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

	/* TODO: implement block-level > page-level FTL */
	if (ptr_pg_mapping->ptr_pg_table != NULL) {
		/*kfree (ptr_blk_mapping->ptr_blk_table);*/
		free (ptr_pg_mapping->ptr_pg_table);
	}
	/* TODO: end */

	/* destroy the block mapping context */
	if (ptr_pg_mapping != NULL)
		/*kfree (ptr_blk_mapping);*/
		free (ptr_pg_mapping);

	/* destroy the ssd context */
	ssdmgmt_destroy_ssd (ptr_ssd);

	/* destory the ftl context */
	if (ptr_ftl_context != NULL)
		/*kfree (ptr_ftl_context);*/
		free (ptr_ftl_context);
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

	int32_t ret = -1;

	/* Get physical page addr and block addr */
	physical_page_address = ptr_pg_mapping->ptr_pg_table[logical_page_address];


	if (physical_page_address == PAGE_TABLE_FREE) {
		/* the requested logical page is not mapped to any physical page */
		*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
		ret = -1;
	} else {
		struct flash_page_t* ptr_erase_page = NULL;
		
		/* decoding the physical page address */
		ftl_convert_to_ssd_layout (physical_page_address, ptr_bus, ptr_chip, ptr_block, ptr_page);
		
		ptr_erase_page = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_blocks[*ptr_block].list_pages[*ptr_page];
		if (ptr_erase_page->page_status == PAGE_STATUS_FREE) {
			/* the logical page must be mapped to the corresponding physical page */
			*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
			ret = -1;
		} else {
			ret = 0;
		}
	}

	return ret;
}


struct flash_block_t* temp_block = NULL;
uint32_t offset;

/* Find the free page in the given block and return the whether it is found */
int32_t get_free_page_in_block(struct flash_ssd_t* ptr_ssd, uint32_t* bus, uint32_t* chip, uint32_t* block){
	
	/* The block is not reserved for GC */
	if(temp_block->is_reserved_block == NOT_RESERVED){
		for(offset = 0; offset < ptr_ssd->nr_pages_per_block; offset++){
			if(temp_block->list_pages[offset].page_status == PAGE_STATUS_FREE){
				return 0;
			}
		}
		
	}

	*block++;
	if(*block >= ptr_ssd->nr_blocks_per_chip){
		*chip++; 
		*block = 0; 
		
		if(*chip >= ptr_ssd->nr_chips_per_bus){
			*bus++;
			*chip = 0;

			if(*bus >= ptr_ssd->nr_buses){
				*bus =0;
			}

		}
	}

	temp_block = &(ptr_ssd->list_buses[*bus].list_chips[*chip].list_blocks[*block];
			return -1;
}

/* get the free page and return the whether it is found */
int32_t get_free_page(struct flash_ssd_t* ptr_ssd){
	
	if(temp_block == NULL){
		temp_block = ssdmgmt_get_free_block(ptr_ssd, 0, 0);
		offset = 0;

		return 0;
	}

	uint32_t cmp_bus = temp_block->no_bus;
	uint32_t cmp_chip = temp_block->no_chip;
	uint32_t cmp_block = temp_block->no_block;

	uint32_t bus = cmp_bus;
	uint32_t chip = cmp_chip;
	uint32_t block = cmp_block;

	if(get_free_page_in_block(ptr_ssd, &bus, &chip, &block) == 0) return 0;

	do{
		if(get_free_page_in_block(ptr_ssd, &bus, &chip, &block) == 0) return 0;
	}while(!(cmp_bus ==bus && cmp_chip == chip && cmp_block == block));

	return -1;

}


/* get a free physical page address */
int32_t page_mapping_get_free_physcial_page_address(
		struct ftl_context_t* ptr_ftl_context, 
		uint32_t logical_page_address, 
		uint32_t* ptr_bus,
		uint32_t* ptr_chip,
		uint32_t* ptr_block,
		uint32_t* ptr_page)
{
	struct falsh_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;

	if(get_free_page(ptr_ssd) == -1) goto need_gc;

	*ptr_bus = temp_block->no_bus;
	*ptr_chip = temp_block->no_chip;
	*ptr_block = temp_block->no_block;
	*ptr_page = offset;

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
	struct flash_block_t* ptr_prev_block, ptr_curr_block;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	
	uint32_t prev_ppa, curr_ppa;
	uint32_t prev_bus, prev_chip, prev_page;

	int32_t ret = -1;

	/* get the previou ppa in the page mapping table */
	prev_ppa = ptr_pg_mapping->ptr_pg_table[logical_page_address];

	ptr_curr_block = &(ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block]);
	curr_ppa = ftl_convert_to_physical_page_address(bus, chip, block, page);

	/* For manage the previous */
	if((prev_ppa != -1) && (prev_ppa != curr_ppa)){
		
		ftl_convert_to_ssd_layout(prev_ppa, &prev_bus, &prev_chip, &prev_block, &prev_page);

		ptr_prev_block = &(ptr_ssd->list_buses[prev_bus].list_chips[prev_chip].list_blocks[prev_block]);

		if(ptr_prev_block->list_pages[prev_page].page_status == PAGE_STATUS_VALID){
			ptr_prev_block->list_pages[prev_page].page_status == PAGE_STATUS_INVALID;
			ptr_prev_block->list_pages[prev_page].no_logical_page_addr = -1;
			ptr_prev_block->nr_valid_pages--;
			ptr_prev_block->nr_invalid_pages++;
		}

	}

	/* For mange the current */
	uint32_t curr_valid = ptr_curr_block->nr_valid_pages;
	uint32_t curr_invalid = ptr_curr_block->nr_invalid_pages;
	
	/* update the current ppa in the page mapping table */
	ptr_pg_mapping->ptr_pg_table[logical_page_address] = curr_ppa;

	if(ptr_curr_block->list_pages[page].page_status == PAGE_STATUS_FREE{
			ptr_curr_block->list_pages[page].page_status == PAGE_STATUS_VALID;
			ptr_curr_block->list_pages[page].no_logical_page_addr = logical_page_address;
			ptr_curr_block->nr_valid_pages++;
			ptr_curr_block->nr_free_pages--;
		      //ptr_curr_block->last_modified_time = timer_get_timestamp_in_us();
		        } 

			/* when the current block is free block */
			if(curr_valid == 0 && curr_invalid == 0){
				ptr_ssd->list_buses[ptr_curr_block->no_bus].list_chips[ptr_curr_block->no_chip].nr_free_blocks--;
				ptr_ssd->list_buses[ptr_curr_block->no_bus].list_chips[ptr_curr_block->no_chip].nr_dirty_blocks++;
			
			}
			ret = 0;
	}
	else{
		printf("blueftl_mapping_block: the physical page address is already used (%u: %u, %u, %u, %u)\n", curr_ppa, bus, chip, block, page);
		
		ret = -1;


	}
	
        return ret;
}
			












}

