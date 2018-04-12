#ifndef _BLUESSD_GC_BLOCK
#define _BLUESSD_GC_BLOCK

#include <linux/types.h>
#include "blueftl_ftl_base.h"

int32_t gc_block_trigger_merge (
	struct ftl_context_t* ptr_ftl_context, 
	uint32_t logical_page_address, 
	uint8_t* ptr_lba_buffer, 
	uint32_t merge_bus, 
	uint32_t merge_chip, 
	uint32_t merge_block);

#endif
