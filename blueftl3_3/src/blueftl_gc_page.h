#ifndef _BLUESSD_GC_PAGE
#define _BLUESSD_GC_PAGE

#include <linux/types.h>
#include "blueftl_ftl_base.h"

#define WL_ON 0

int32_t gc_page_trigger_gc(
	struct ftl_context_t *ptr_ftl_context, 
	int32_t gc_target_bus, 
	int32_t gc_target_chip);

void gc_page_trigger_init(struct ftl_context_t *ptr_ftl_context);

void move_block(struct ftl_context_t *context, struct flash_block_t *src, struct flash_block_t *dest);

#endif
