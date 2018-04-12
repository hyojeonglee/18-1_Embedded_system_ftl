#ifndef _BLUESSD_BLKMGMT
#define _BLUESSD_BLKMGMT

#include <stdint.h>

#include "blueftl_user_vdevice.h"

#define PAGE_STATUS_FREE		1
#define PAGE_STATUS_INVALID		2
#define PAGE_STATUS_VALID		3

struct flash_page_t {
	uint32_t no_logical_page_addr;
	int32_t page_status;	/* 1 : free, 2 : invalid, 3: valid */
};

struct flash_block_t {
	/* block id */
	uint32_t no_block;
	uint32_t no_chip;
	uint32_t no_bus;

	/* block status */
	uint32_t nr_valid_pages;
	uint32_t nr_invalid_pages;
	uint32_t nr_free_pages;

	/* the number of erasure operations */
	uint32_t nr_erase_cnt;

	/* for bad block management */
	uint32_t is_bad_block;

	/* for garbage collection */
	uint32_t last_modified_time;

	/* is reserved block for gc? */
	uint32_t is_reserved_block;

	/* for keeping the oob data (NOTE: it must be removed in real implementation) */
	struct flash_page_t* list_pages;

	/* for dual-pool algorithm, recent erasure counts */
	uint32_t nr_recent_erase_cnt;

	/* for dual-pool algorithm, head, tail flag of erasure count list*/
	uint32_t head_or_tail_ec;

	/* for dual-pool algorithm, head, tail flag of recent erasure count list */
	uint32_t head_or_tail_rec;

	/* for dual-pool algorithm, flag hot cold pool */
	uint32_t hot_cold_pool;
};

struct flash_chip_t {
	/* the number of free blocks in a chip */
	uint32_t nr_free_blocks;

	/* the number of dirty blocks in a chip */
	uint32_t nr_dirty_blocks;

	/* block array */
	struct flash_block_t* list_blocks;
};

struct flash_bus_t {
	/* chip array */
	struct flash_chip_t* list_chips;
};

struct flash_ssd_t {
	/* ssd information */
	uint32_t nr_buses;
	uint32_t nr_chips_per_bus;
	uint32_t nr_blocks_per_chip;
	uint32_t nr_pages_per_block;

	/* bus array */
	struct flash_bus_t* list_buses;
};

/* create the ftl context */
struct flash_ssd_t* ssdmgmt_create_ssd (struct virtual_device_t* ptr_vdevice);

/* destory the ftl context */
void ssdmgmt_destroy_ssd (struct flash_ssd_t* ptr_ssd);

/* get the free block */
struct flash_block_t* ssdmgmt_get_free_block (struct flash_ssd_t* ptr_ssd, uint32_t target_bus, uint32_t target_chip);

#endif
