#ifndef _BLUESSD_FTL_BLOCK
#define _BLUESSD_FTL_BLOCK

#define BLOCK_TABLE_FREE		-1

extern struct ftl_base_t ftl_base_block_mapping;

struct ftl_block_mapping_context_t {
	uint32_t nr_blk_table_entries;	/* the number of blocks that belong to the block mapping table */
	uint32_t* ptr_blk_table; /* for the block mapping */
};

/* create the block mapping table */
struct ftl_context_t* block_mapping_create_ftl_context (struct virtual_device_t* ptr_vdevice);

/* destroy the block mapping table */
void block_mapping_destroy_ftl_context (struct ftl_context_t* ptr_ftl_context);

/* get a physical page number that was mapped to a logical page number before */
int32_t block_mapping_get_mapped_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page);

/* get a free physical page address */
int32_t block_mapping_get_free_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address,
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page);

/* map a logical page address to a physical page address */
int32_t block_mapping_map_logical_to_physical (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t bus,
	uint32_t chip,
	uint32_t block,
	uint32_t page);

#endif
