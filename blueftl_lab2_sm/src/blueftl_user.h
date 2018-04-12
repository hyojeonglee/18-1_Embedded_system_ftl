#ifndef _BLUEFTL_USER_H
#define _BLUEFTL_USER_H

#include <stdint.h>

struct ssd_params_t {
	/* ssd paramters */
	uint32_t nr_buses;
	uint32_t nr_chips_per_bus;
	uint32_t nr_blocks_per_chip;
	uint32_t nr_pages_per_block;
	uint32_t page_main_size;
	uint32_t page_oob_size;
	uint32_t ssd_type;

	/* ftl parameters */
	uint32_t mapping_policy;
	uint32_t gc_policy;
	uint32_t wl_policy;
};

#endif
