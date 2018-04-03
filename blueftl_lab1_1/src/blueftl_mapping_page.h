/*
 * =====================================================================================
 *
 *       Filename:  blueftl_mapping_page.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018년 04월 02일 21시 09분 07초
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (SEOG minN KWON), 
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef _BLUESSD_FTL_PAGE
#define _BLUESSE_FTL_PAGE

#define PAGE_TABLE_FREE

exten struct ftl_base_t ftl_base_page_mapping_lab;

struct ftl_page_mapping_context_t {
	uint32_t nr_pagetable_entries; /* the number of pages that belong to the page mappping table  */
	uint32_t* ptr_pagetable; /* for the page mapping  */
};

/* create the page mapping table  */
struct ftl_context_t* page_mapping_create_ftl_context (struct virtual_device_t* ptr_device);

/*  destroy the page mapping table  */
void page_mapping_destroy_ftl_context (struct ftl_context_t* ptr_ftl_context);

/* get a physical page number that was mapped to a logical page number before  */
// int32_t page_mapping_get_mapped_physical_page_address();

/*get a free physical page addres */
// int32_t page_mapping_get_free_physical_page_address();

/* map a logical page address to a physical page address */
// int32_t page_mapping_map_logical_to_physical();

#endif

