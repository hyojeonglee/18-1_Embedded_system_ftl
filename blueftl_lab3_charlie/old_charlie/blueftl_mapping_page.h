#ifndef _BLUESSD_FTL_PAGE
#define _BLUESSD_FTL_PAGE

#define PAGE_TABLE_FREE	 0


extern struct ftl_base_t ftl_base_page_mapping;

struct ftl_chunk_table_entry_t {
	int32_t nr_valid_pages;
	int32_t nr_physical_pages;
	int32_t is_compressed;
};

struct ftl_chunk_table_context_t {
	uint32_t nr_chunk_table_entries;
	struct ftl_chunk_table_entry_t* ptr_chunks;
};

struct ftl_page_mapping_context_t {
	uint32_t nr_pg_table_entries;	/* the number of pages that belong to the page mapping table */
	/* TODO needs nr_blk_table_entries? */
	uint32_t* ptr_pg_table; /* for the page mapping */
};

/* create the page mapping table */
struct ftl_context_t* page_mapping_create_ftl_context (struct virtual_device_t* ptr_vdevice);

/* destroy the page mapping table */
void page_mapping_destroy_ftl_context (struct ftl_context_t* ptr_ftl_context);

int32_t page_mapping_update_chunk_table(
		struct ftl_context_t* ptr_ftl_context,
		uint32_t *ptr_bus,
		uint32_t *ptr_chip,
		uint32_t *ptr_block,
		uint32_t *ptr_page,
		uint32_t nr_valid_pages,
		uint32_t nr_physical_pages,
		uint32_t is_compressed);

/* get a physical page number that was mapped to a logical page number before */
int32_t page_mapping_get_mapped_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page);

/* get a free physical page address */
int32_t page_mapping_get_free_physical_page_address (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address,
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page);

/* map a logical page address to a physical page address */
int32_t page_mapping_map_logical_to_physical (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint32_t bus,
	uint32_t chip,
	uint32_t block,
	uint32_t page);

#endif
