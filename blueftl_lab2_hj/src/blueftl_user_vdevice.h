#ifndef _BLUEFTL_CHAR_USER_H
#define _BLUEFTL_CHAR_USER_H

#include <stdint.h>

#include "blueftl_user.h"

extern struct virtual_device_t* _ptr_vdevice;

/* The following information must be collected according to NAND flash chips */
#define	NR_BUSES				1
#define	NR_CHIPS_PER_BUS		1
#define NR_BLOCKS_PER_CHIP		1024
#define	NR_PAGES_PER_BLOCK		64
#define FLASH_PAGE_SIZE			2048
#define FLASH_PAGE_OOB_SIZE     64

/* The options for debugging */
#define OFFSET_CHIP				0
#define OFFSET_BUS				0
#define	MAX_PAGES_PER_BLOCK 	64

/* The type of the SSD */
#define SSD_TYPE_RAMDRIVE		1
#define SSD_TYPE_BLUESSD_XUPV2	2
#define SSD_TYPE_BLUESSD_ML605	3

struct virtual_device_t {
	/* character device */
	int32_t blueftl_char_h;

	/* the information about the SSD organization */
	uint32_t nr_buses;
	uint32_t nr_chips_per_bus;
	uint32_t nr_blocks_per_chip;
	uint32_t nr_pages_per_block;
	uint32_t page_main_size;
	uint32_t page_oob_size;
	uint32_t device_capacity;
};

struct virtual_device_t* blueftl_user_vdevice_open (
	struct ssd_params_t* ptr_ssd_params);

void blueftl_user_vdevice_close (
	struct virtual_device_t* ptr_vdevice);

int32_t blueftl_user_vdevice_page_read (
	struct virtual_device_t* ptr_vdevice,
	int32_t bus, 
	int32_t chip, 
	int32_t block, 
	int32_t page, 
	int32_t page_length, 
	char* ptr_page_data);

int32_t blueftl_user_vdevice_page_write (
	struct virtual_device_t* ptr_vdevice,
	int32_t bus, 
	int32_t chip, 
	int32_t block, 
	int32_t page, 
	int32_t page_length, 
	char* ptr_page_data);

int32_t blueftl_user_vdevice_block_erase (
	struct virtual_device_t* ptr_vdevice,
	int32_t bus, 
	int32_t chip, 
	int32_t block);

int32_t blueftl_user_vdevice_req_done (
	struct virtual_device_t* ptr_vdevice);

int32_t blueftl_user_vdevice_char_open (
	struct virtual_device_t* ptr_vdevice);

void blueftl_user_vdevice_char_close (
	struct virtual_device_t* ptr_vdevice);

int32_t blueftl_user_vdevice_set_timeout (
	struct virtual_device_t* ptr_vdevice,
	long timeout_ms);

#endif
