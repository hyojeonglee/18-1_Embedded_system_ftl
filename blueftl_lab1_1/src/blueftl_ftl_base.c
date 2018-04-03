#include <linux/types.h>
#include "blueftl_ftl_base.h"

void ftl_convert_to_ssd_layout (
	uint32_t physical_page_address, 
	uint32_t* ptr_bus, 
	uint32_t* ptr_chip, 
	uint32_t* ptr_block, 
	uint32_t* ptr_page)
{
	if (ptr_bus != NULL)
		*ptr_bus = (uint32_t)((physical_page_address >> 0) & 0x00000003);

	if (ptr_chip != NULL)
		*ptr_chip = (uint32_t)((physical_page_address >> 2) & 0x00000007);

	if (ptr_page != NULL)
		*ptr_page = (uint32_t)((physical_page_address >> 5) & 0x0000007F);

	if (ptr_block != NULL)
		*ptr_block = (uint32_t)((physical_page_address >> 12) & 0xFFFFFFFF);
}

uint32_t ftl_convert_to_physical_page_address (
	uint32_t bus,
	uint32_t chip, 
	uint32_t block, 
	uint32_t page)
{
	uint32_t physical_page_address = 0;

	physical_page_address = bus | chip << 2 | page << 5 | block << 12;

	return physical_page_address;
}

