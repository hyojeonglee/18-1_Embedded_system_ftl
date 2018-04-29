#ifndef _BLUEFTL_CHAR_H
#define _BLUEFTL_CHAR_H

#include <stdint.h>
#include <linux/ioctl.h>

enum {
	NETLINK_READA = 0,
	NETLINK_READ = 1,
	NETLINK_WRITE = 2,
};

enum {
	IOCTL_TYPE_READ = 0,
	IOCTL_TYPE_WRITE = 1,
	IOCTL_TYPE_ERASE = 2,
};

struct blueftl_io_req {
	char op_type;	/* 0: read, 1: write, 2: erase */
	int bus;
	int chip;
	int block;		
	int page;
	uint8_t ptr_page_data[2048];
};

#define	BLUEFTL_CHAR_MAJOR_NUM			123

#define BLUEFTL_CHAR_IOCTL_BIO_DONE		_IOR(BLUEFTL_CHAR_MAJOR_NUM,  0, char)	/* Let kernel konw that the requested bio has done */
#define BLUEFTL_CHAR_IOCTL_REQ			_IOR(BLUEFTL_CHAR_MAJOR_NUM,  1, struct blueftl_io_req)	/* Normal flash I/O operations, e.g., read, write, and erase */
#define BLUEFTL_CHAR_IOCTL_TIMEOUT		_IOR(BLUEFTL_CHAR_MAJOR_NUM,  2, long)	/* the period during which we wait for responses from the user-level FTL */
#define BLUEFTL_CHAR_IOCTL_SEND_DATA	_IOR(BLUEFTL_CHAR_MAJOR_NUM,  3, char*)	

#endif
