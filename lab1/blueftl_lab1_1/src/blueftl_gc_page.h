#ifndef _BLUESSD_GC_PAGE
#define _BLUESSD_GC_PAGE

#include <linux/types.h>
#include "blueftl_ftl_base.h"

/* TODO: 그냥 NO 답 */
int32_t gc_page_trigger_gc_lab(
		struct ftl_context_t* ptr_ftl_context, 
		uint32_t gc_target_bus, 
		uint32_t gc_target_chip);

#endif
