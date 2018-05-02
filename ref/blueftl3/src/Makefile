CFLAGS=-g -Wall
CC=gcc
LIBS=-lm -lpthread
INCLUDES=
OBJS=blueftl_user_vdevice.o blueftl_user_netlink.o blueftl_user_ftl_main.o blueftl_ftl_base.o blueftl_gc_page.o blueftl_mapping_page.o blueftl_gc_block.o blueftl_mapping_block.o  blueftl_ssdmgmt.o blueftl_util.o blueftl_wl_dual_pool.o blueftl_read_write_mgr.o lzrw3.o
SRCS=blueftl_user_ftl_main.c blueftl_ftl_base.c blueftl_gc_page.c blueftl_mapping_page.c blueftl_gc_block.c blueftl_mapping_block.c blueftl_wl_dual_pool.c blueftl_read_write_mgr.c lzrw3.c

# The variable $@ has the value of the target. In this case $@ = psort
blueftl_user: blueftl_user.o ${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ blueftl_user.o ${OBJS} ${LIBS}

clean:
	rm blueftl_user_ftl_main.o blueftl_ftl_base.o blueftl_gc_page.o blueftl_mapping_page.o blueftl_gc_block.o blueftl_mapping_block.o blueftl_wl_dual_pool.o blueftl_user
