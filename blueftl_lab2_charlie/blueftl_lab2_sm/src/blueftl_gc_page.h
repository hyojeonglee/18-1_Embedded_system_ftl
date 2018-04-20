#ifndef _BLUESSD_GC_PAGE
#define _BLUESSD_GC_PAGE

#include <linux/types.h>
#include "blueftl_ftl_base.h"

/* for excecuting wear leveling */
#define WEAR_LEVELING 1

/* TODO: 아래에서 세번째까지 파라미터도 필요할까? */
int32_t gc_page_trigger_gc_lab (
		struct ftl_context_t* ptr_ftl_context,
		uint32_t gc_target_bus,
		uint32_t gc_target_chip);

/* intialize the condition for gc page */
void gc_page_triiger_init(struct ftl_context_t* ptr_ftl_context);

/* move the data from source bloek to destination block */
void block_copy(struct ftl_context_t* context, struct flash_block_t* src_block, struct flash_block_t* dest_block);

/*
int32_t gc_block_trigger_merge (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint8_t* ptr_lba_buffer, 
	uint32_t merge_bus, 
	uint32_t merge_chip, 
	uint32_t merge_block);
*/

#endif
