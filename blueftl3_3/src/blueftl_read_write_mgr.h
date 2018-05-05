#ifndef _BLUESSD_FTL_RW_MGR
#define _BLUESSD_FTL_RW_MGR

#define WRITE_BUFFER_LEN 4

uint32_t blueftl_read_write_mgr_init();
void blueftl_read_write_mgr_close();

void blueftl_page_read(
    struct ftl_base_t *ftl_base, 
    struct ftl_context_t *ptr_ftl_context, 
    uint32_t lpa_curr, 
    uint8_t *ptr_lba_buff
);

uint32_t blueftl_page_write(
    struct ftl_base_t *ftl_base, 
    struct ftl_context_t *ptr_ftl_context, 
    uint32_t lpa_curr, 
    uint8_t *ptr_lba_buff
);

#endif


struct wr_buff_t {
    uint32_t arr_lpa[WRITE_BUFFER_LEN];
    uint8_t buff[FLASH_PAGE_SIZE * WRITE_BUFFER_LEN];
};

struct wr_buff_t _struct_write_buff; 

uint8_t *_write_buff;
uint8_t *_compressed_buff;
uint32_t _nr_buff_pages;


