#ifdef KERNEL_MODE

#include <linux/module.h>

#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "bluessd_vdevice.h"
#include "blueftl_mapping_block.h"
#include "blueftl_gc_block.h"
#include "blueftl_util.h"

#else

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_mapping_block.h"
#include "blueftl_gc_block.h"
#include "blueftl_util.h"

#endif


struct ftl_base_t ftl_base_block_mapping = {
	.ftl_create_ftl_context = block_mapping_create_ftl_context,
	.ftl_destroy_ftl_context = block_mapping_destroy_ftl_context,
	.ftl_get_mapped_physical_page_address = block_mapping_get_mapped_physical_page_address,
	.ftl_get_free_physical_page_address = block_mapping_get_free_physical_page_address,
	.ftl_map_logical_to_physical = block_mapping_map_logical_to_physical,
	.ftl_trigger_gc = NULL,
	.ftl_trigger_merge = gc_block_trigger_merge,
	.ftl_trigger_wear_leveler = NULL,
};

/* create the block mapping table */
struct ftl_context_t* block_mapping_create_ftl_context (
	struct virtual_device_t* ptr_vdevice)
{
	uint32_t init_blk_loop = 0;

	struct ftl_context_t* ptr_ftl_context = NULL;
	struct flash_ssd_t* ptr_ssd = NULL;
	struct ftl_block_mapping_context_t* ptr_blk_mapping = NULL;

	/* create the ftl context */
	/*if ((ptr_ftl_context = (struct ftl_context_t*)kmalloc (sizeof (struct ftl_context_t), GFP_ATOMIC)) == NULL) {*/
	if ((ptr_ftl_context = (struct ftl_context_t*)malloc (sizeof (struct ftl_context_t))) == NULL) {
		printf ("blueftl_mapping_block: the creation of the ftl context failed\n");
		goto error_alloc_ftl_context;
	}

	/* create the ssd context */
	if ((ptr_ftl_context->ptr_ssd = ssdmgmt_create_ssd (ptr_vdevice)) == NULL) {
		printf ("blueftl_mapping_block: the creation of the ftl context failed\n");
		goto error_create_ssd_context;
	}

	/* create the block mapping context */
	/*if ((ptr_ftl_context->ptr_mapping = (struct ftl_block_mapping_context_t *)kmalloc (sizeof (struct ftl_block_mapping_context_t), GFP_ATOMIC)) == NULL) {*/
	if ((ptr_ftl_context->ptr_mapping = (struct ftl_block_mapping_context_t *)malloc (sizeof (struct ftl_block_mapping_context_t))) == NULL) {
		printf ("blueftl_mapping_block: the creation of the ftl context failed\n");
		goto error_alloc_ftl_block_mapping_context;
	}

	/* set virtual device */
	ptr_ftl_context->ptr_vdevice = ptr_vdevice;

	ptr_ssd = ptr_ftl_context->ptr_ssd;
	ptr_blk_mapping = (struct ftl_block_mapping_context_t *)ptr_ftl_context->ptr_mapping;

	/* TODO: implement block-level FTL */

	ptr_blk_mapping->nr_blk_table_entries = 
		ptr_ssd->nr_buses * ptr_ssd->nr_chips_per_bus * ptr_ssd->nr_blocks_per_chip;

	/*if ((ptr_blk_mapping->ptr_blk_table = (uint32_t*)kmalloc (ptr_blk_mapping->nr_blk_table_entries * sizeof (uint32_t), GFP_ATOMIC)) == NULL) {*/
	if ((ptr_blk_mapping->ptr_blk_table = (uint32_t*)malloc (ptr_blk_mapping->nr_blk_table_entries * sizeof (uint32_t))) == NULL) {
		printf ("blueftl_mapping_page: failed to allocate the memory for ptr_mapping_table\n");
		goto error_alloc_mapping_table;
	}

	for (init_blk_loop = 0; init_blk_loop < ptr_blk_mapping->nr_blk_table_entries; init_blk_loop++) {
		ptr_blk_mapping->ptr_blk_table[init_blk_loop] = BLOCK_TABLE_FREE;
	}

	/* TODO: end */

	return ptr_ftl_context;


error_alloc_mapping_table:
	/*kfree (ptr_ftl_context->ptr_mapping);*/
	free (ptr_ftl_context->ptr_mapping);

error_alloc_ftl_block_mapping_context:
	ssdmgmt_destroy_ssd (ptr_ssd);

error_create_ssd_context:
	/*kfree (ptr_ftl_context);*/
	free (ptr_ftl_context);

error_alloc_ftl_context:
	return NULL;
}

/* destroy the block mapping table */
void block_mapping_destroy_ftl_context (struct ftl_context_t* ptr_ftl_context)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_block_mapping_context_t* ptr_blk_mapping = (struct ftl_block_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	/* TODO: implement block-level FTL */
	if (ptr_blk_mapping->ptr_blk_table != NULL) {
		/*kfree (ptr_blk_mapping->ptr_blk_table);*/
		free (ptr_blk_mapping->ptr_blk_table);
	}
	/* TODO: end */

	/* destroy the block mapping context */
	if (ptr_blk_mapping != NULL)
		/*kfree (ptr_blk_mapping);*/
		free (ptr_blk_mapping);

	/* destroy the ssd context */
	ssdmgmt_destroy_ssd (ptr_ssd);

	/* destory the ftl context */
	if (ptr_ftl_context != NULL)
		/*kfree (ptr_ftl_context);*/
		free (ptr_ftl_context);
}

/* get a physical page number that was mapped to a logical page number before */
int32_t block_mapping_get_mapped_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page)
{	
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_block_mapping_context_t* ptr_blk_mapping = (struct ftl_block_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	uint32_t logical_block_address;
	uint32_t physical_block_address;
	uint32_t page_offset;

	int32_t ret = -1;

	/* get the logical block number and the page offset */
	logical_block_address = logical_page_address / ptr_ssd->nr_pages_per_block;
	page_offset = logical_page_address % ptr_ssd->nr_pages_per_block;

	/* obtain the physical block address using the block mapping table */
	physical_block_address = ptr_blk_mapping->ptr_blk_table[logical_block_address];

	if (physical_block_address == BLOCK_TABLE_FREE) {
		/* the requested logical block is not mapped to any physical block */
		*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
		ret = -1;
	} else {
		struct flash_block_t* ptr_erase_block = NULL;
		
		/* decoding the physical block address */
		ftl_convert_to_ssd_layout (physical_block_address, ptr_bus, ptr_chip, ptr_block, NULL);
		
		ptr_erase_block = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_blocks[*ptr_block];
		if (ptr_erase_block->list_pages[page_offset].page_status == PAGE_STATUS_FREE) {
			/* the logical page must be mapped to the corresponding physical page */
			*ptr_bus = *ptr_chip = *ptr_block = *ptr_page = -1;
			ret = -1;
		} else {
			*ptr_page = page_offset;
			ret = 0;
		}
	}

	return ret;
}

/* get a free physical page address */
int32_t block_mapping_get_free_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address,
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page)
{
	struct flash_block_t* ptr_erase_block;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_block_mapping_context_t* ptr_blk_mapping = (struct ftl_block_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	uint32_t logical_block_address;
	uint32_t physical_block_address;
	uint32_t page_offset;

	/* get the logical block number and the page offset */
	logical_block_address = logical_page_address / ptr_ssd->nr_pages_per_block;
	page_offset = logical_page_address % ptr_ssd->nr_pages_per_block;

	/* obtain the physical block address using the block mapping table */
	physical_block_address = ptr_blk_mapping->ptr_blk_table[logical_block_address];

	if (physical_block_address != BLOCK_TABLE_FREE) {
		/* encoding the ssd layout to a phyical block address 
		   NOTE: page must be 0 in the block-level mapping */
		ftl_convert_to_ssd_layout (physical_block_address, ptr_bus, ptr_chip, ptr_block, NULL);
		*ptr_page = page_offset;

		/* see if the logical page can be written to the physical block found */
		ptr_erase_block = &ptr_ssd->list_buses[*ptr_bus].list_chips[*ptr_chip].list_blocks[*ptr_block];

		if (ptr_erase_block->list_pages[page_offset].page_status == PAGE_STATUS_FREE) {
			uint32_t page_loop = 0;

			/* check the SOP restriction */ 
			for (page_loop = page_offset; page_loop < ptr_ssd->nr_pages_per_block; page_loop++) {
				if (ptr_erase_block->list_pages[page_loop].page_status != PAGE_STATUS_FREE) {
					goto need_gc;
				}
			}

			/* ok. we can use this page for writing */
			*ptr_page = page_offset;
		} else {
			/* the target page is already used, so it is required to perform garbage collection */
			goto need_gc;
		}
	} else {
		struct flash_block_t* ptr_erase_block = NULL;

		/* get the free block from the block mapping table */
		ptr_erase_block = ssdmgmt_get_free_block (ptr_ssd, 0, 0);

		*ptr_bus = ptr_erase_block->no_bus;
		*ptr_chip = ptr_erase_block->no_chip;
		*ptr_block = ptr_erase_block->no_block;
		*ptr_page = page_offset;
	}

	return 0;

need_gc:
	return -1;
}

/* map a logical page address to a physical page address */
int32_t block_mapping_map_logical_to_physical (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t bus,
	uint32_t chip,
	uint32_t block,
	uint32_t page)
{
	struct flash_block_t* ptr_erase_block;
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct ftl_block_mapping_context_t* ptr_blk_mapping = (struct ftl_block_mapping_context_t*)ptr_ftl_context->ptr_mapping;

	uint32_t logical_block_address;
	uint32_t physical_block_address;
	uint32_t page_offset;

	int32_t ret = -1;

	/* get the logical block number and the page offset */
	logical_block_address = logical_page_address / ptr_ssd->nr_pages_per_block;
	page_offset = logical_page_address % ptr_ssd->nr_pages_per_block;

	/* get the physical block address using the block mapping table */
	physical_block_address = ptr_blk_mapping->ptr_blk_table[logical_block_address];

	if (page_offset != page) {
		/* the page offset of the logical page address must be the same as the page address */
		printf ("blueftl_mapping_block: there is something wrong with block mapping (%u != %u)\n", page_offset, page);
		return -1;
	}

	/* see if the given logical block is alreay mapped or not */
	if (physical_block_address == BLOCK_TABLE_FREE) {
		/* encoding the ssd layout to a phyical block address 
		   NOTE: the address of the page must be 0 in the block-level mapping */
		physical_block_address = ftl_convert_to_physical_page_address (bus, chip, block, 0);

		/* update the block mapping table */
		ptr_blk_mapping->ptr_blk_table[logical_block_address] = physical_block_address;
	} 

	/* see if the given page is already written or not */
	ptr_erase_block = &ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block];
	if (ptr_erase_block->list_pages[page_offset].page_status == PAGE_STATUS_FREE) {
		/* make it valid */
		ptr_erase_block->list_pages[page_offset].page_status = PAGE_STATUS_VALID;

		/* increase the number of valid pages while decreasing the number of free pages */
		ptr_erase_block->nr_valid_pages++;
		ptr_erase_block->nr_free_pages--;

		ret = 0;
	} else {
		/*hmmm... the physical page that corresponds to the logical page must be free */
		printf ("blueftl_mapping_block: the physical page address is already used (%u:%u,%u,%u,%u)\n", 
			physical_block_address, bus, chip, block, page);
		ret = -1;
	}

	return ret;
}

