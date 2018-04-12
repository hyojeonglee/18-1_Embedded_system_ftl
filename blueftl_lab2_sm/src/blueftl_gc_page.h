#ifndef _BLUESSD_GC_PAGE
#define _BLUESSD_GC_PAGE

#include <linux/types.h>
#include "blueftl_ftl_base.h"

/* TODO: 아래에서 세번째까지 파라미터도 필요할까? */
int32_t gc_page_trigger_gc_lab (
		struct ftl_context_t* ptr_ftl_context,
		uint32_t gc_target_bus,
		uint32_t gc_target_chip);

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
