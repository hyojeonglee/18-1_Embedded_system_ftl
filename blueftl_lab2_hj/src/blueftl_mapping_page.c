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

struct ftl_base_t ftl_base_page_mapping = {
	.ftl_create_ftl_context = page_mapping_create_ftl_context,
	.ftl_destroy_ftl_context = page_mapping_destroy_ftl_context,
	.ftl_get_mapped_physical_page_address = page_mapping_get_mapped_physical_page_address,
	.ftl_get_free_physical_page_address = page_mapping_get_free_physical_page_address,
	.ftl_map_logical_to_physical = page_mapping_map_logical_to_physical,
	.ftl_trigger_gc = gc_page_trigger_gc_lab, // for test greedy policy
	//.ftl_trigger_gc = gc_page_trigger_option, // with gc option
	// .ftl_trigger_gc = NULL, // for test
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

	/* TODO is this right? */
	ptr_pg_mapping->nr_pg_table_entries = 
		ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus * ptr_ssd->nr_blocks_per_chip * ptr_ssd->nr_pages_per_block; // TODO modify ? */

	/*if ((ptr_blk_mapping->ptr_blk_table = (uint32_t*)kmalloc (ptr_blk_mapping->nr_blk_table_entries * sizeof (uint32_t), GFP_ATOMIC)) == NULL) {*/
	if ((ptr_pg_mapping->ptr_pg_table = (uint32_t*)malloc (ptr_pg_mapping->nr_pg_table_entries * sizeof (uint32_t))) == NULL) {
		printf ("blueftl_mapping_page: failed to allocate the memory for ptr_mapping_table\n");
		goto error_alloc_mapping_table;
	}

	/* Initialize */
	for (init_pg_loop = 0; init_pg_loop < ptr_pg_mapping->nr_pg_table_entries; init_pg_loop++) {
		ptr_pg_mapping->ptr_pg_table[init_pg_loop] = PAGE_TABLE_FREE;
	}

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

/* get a free physical page address */
int32_t page_mapping_get_free_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address,
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;
	struct flash_block_t* ptr_erase_block;
	struct flash_page_t* ptr_erase_page;

	uint32_t physical_page_address;

	physical_page_address = ptr_pg_mapping->ptr_pg_table[logical_page_address];
	
	uint32_t i, j, m, n;

	if (physical_page_address != PAGE_TABLE_FREE) {
		/* already mapped, so find another physical page that not in used */
		
		ftl_convert_to_ssd_layout (physical_page_address, ptr_bus, ptr_chip, ptr_block, ptr_page);	
		
		// for test
		// printf("[get free: 220] logic %d was already mapped with bus %d chip %d block %d page %d\n", logical_page_address, *ptr_bus, *ptr_chip, *ptr_block, *ptr_page);
		
		/* Change curr physical page status to INVALID */
		ptr_erase_block = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_blocks[*ptr_block];
		ptr_erase_page = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_blocks[*ptr_block].list_pages[*ptr_page];
		if(ptr_erase_page->page_status != 2){
			ptr_erase_page->page_status = 2;
			ptr_erase_block->nr_valid_pages--;
			ptr_erase_block->nr_invalid_pages++;
		}

		int nr_all_used_block = 0;
		int found_free_page = 0;
		int is_all_used;

		for (i = 0 ; i < ptr_ssd->nr_buses; i++) {
			for (j = 0; j < ptr_ssd->nr_chips_per_bus; j++) {
				for (m = 0; m < ptr_ssd->nr_blocks_per_chip; m++) {
					ptr_erase_block = &ptr_ssd->list_buses[i].list_chips[j].list_blocks[m];
					is_all_used = 0;
					for (n = 0; n < ptr_ssd->nr_pages_per_block; n++) {
						if (ptr_erase_block->list_pages[n].page_status == PAGE_STATUS_FREE) {
							if (found_free_page == 0) {
								*ptr_bus = i;
								*ptr_chip = j;
								*ptr_block = m;
								*ptr_page = n;
							}
							
							is_all_used = 0;
							found_free_page = 1;
						} else {
							is_all_used = 1;
						}
					}
					if (is_all_used == 1)
						nr_all_used_block++;
				}
				if (found_free_page == 1) {
					return 0;
				}
				else {
					return -1;
				}
			}
		}
	} else {
		uint32_t loop_page = 0;

		if (physical_page_address == 0) {
			if(ptr_ftl_context->latest_block == -1 || ptr_ssd->list_buses[ptr_ftl_context->latest_bus].list_chips[ptr_ftl_context->latest_chip].list_blocks[ptr_ftl_context->latest_block].nr_free_pages == 0){
		
				ptr_erase_block = ssdmgmt_get_free_block (ptr_ssd, 0, 0);

				int nr_all_used_block = 0;
				int is_all_used;
				int found_free_page = 0;
				if (ptr_erase_block == NULL) {
					for (i = 0 ; i < ptr_ssd->nr_buses; i++) {
						for (j = 0; j < ptr_ssd->nr_chips_per_bus; j++) {
							for (m = 0; m < ptr_ssd->nr_blocks_per_chip; m++) {
								ptr_erase_block = &ptr_ssd->list_buses[i].list_chips[j].list_blocks[m];
								is_all_used = 0;
								for (n = 0; n < ptr_ssd->nr_pages_per_block; n++) {
									if (ptr_erase_block->list_pages[n].page_status == PAGE_STATUS_FREE) {
										if (found_free_page == 0) {
											*ptr_bus = i;
											*ptr_chip = j;
											*ptr_block = m;
											*ptr_page = n;
										}
								
										is_all_used = 0;
										found_free_page = 1;
									} else {
										is_all_used = 1;
									}
								}
								if (is_all_used == 1)
								nr_all_used_block++;
							}
							if (found_free_page == 1) {
								return 0;
							}
							else {
								return -1;
							}
						}
					}
				}

				*ptr_bus = ptr_erase_block->no_bus;
				*ptr_chip = ptr_erase_block->no_chip;
				*ptr_block = ptr_erase_block->no_block;
				*ptr_page = 0;

				ptr_ftl_context->latest_bus = *ptr_bus;	
				ptr_ftl_context->latest_chip = *ptr_chip;	
				ptr_ftl_context->latest_block = *ptr_block;
				
			}else {
				ptr_erase_block = &ptr_ssd->list_buses[ptr_ftl_context->latest_bus].list_chips[ptr_ftl_context->latest_chip].list_blocks[ptr_ftl_context->latest_block];
				for (loop_page = 0; loop_page < ptr_ssd->nr_pages_per_block; loop_page++) {
					if (ptr_erase_block->list_pages[loop_page].page_status == PAGE_STATUS_FREE) {
						*ptr_bus = ptr_erase_block->no_bus;
						*ptr_chip = ptr_erase_block->no_chip;
						*ptr_block = ptr_erase_block->no_block;
						*ptr_page = loop_page;
						break;
					}
					else {
					}
				}
			}
			return 0;
		}
	}

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
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	uint32_t physical_page_address;
	
	int32_t ret = 0;

	physical_page_address = ptr_pg_mapping->ptr_pg_table[logical_page_address];

	/* see if the given logical page is alreay mapped or not */
	if (physical_page_address == PAGE_TABLE_FREE) {
		physical_page_address = ftl_convert_to_physical_page_address (bus, chip, block, page);

		/* update the page mapping table */
		ptr_pg_mapping->ptr_pg_table[logical_page_address] = physical_page_address;
	} 

	/* see if the given page is already written or not */

	ptr_erase_block = &ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block];
	if (ptr_erase_block->list_pages[page].page_status == PAGE_STATUS_FREE) {
		/* make it valid */
		ptr_erase_block->list_pages[page].page_status = PAGE_STATUS_VALID;

		/* increase the number of valid pages while decreasing the number of free pages */

		ptr_erase_block->nr_valid_pages++;
		ptr_erase_block->nr_free_pages--;

		physical_page_address = ftl_convert_to_physical_page_address (bus, chip, block, page);
		ptr_pg_mapping->ptr_pg_table[logical_page_address] = physical_page_address;

		ret = 0;
	} else {
		/*hmmm... the physical page that corresponds to the logical page must be free */
		printf ("blueftl_mapping_page: the physical page address is already used (%u:%u,%u,%u,%u)\n", 
			physical_page_address, bus, chip, block, page);
		ret = -1;
	}

	return ret;
}

