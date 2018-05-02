CFLAGS=-g -Wall
CC=gcc
LIBS=-lm -lpthread
INCLUDES=
OBJS=blueftl_user_vdevice.o blueftl_user_netlink.o blueftl_user_ftl_main.o blueftl_ftl_base.o blueftl_ssdmgmt.o blueftl_util.o blueftl_gc_page.o blueftl_mapping_page.o lzrw3.o blueftl_read_write_mgr.o
SRCS=blueftl_user_ftl_main.c blueftl_ftl_base.c blueftl_util.c blueftl_gc_page.c blueftl_mapping_page.c lzrw3.c blueftl_read_write_mgr.c

# The variable $@ has the value of the target. In this case $@ = psort
blueftl_user: blueftl_user.o ${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ blueftl_user.o ${OBJS} ${LIBS}

clean:
	rm blueftl_user_ftl_main.o blueftl_ftl_base.o blueftl_util.o blueftl_gc_page.o blueftl_mapping_page.o blueftl_user lzrw3.o blueftl_read_write_mgr.o
