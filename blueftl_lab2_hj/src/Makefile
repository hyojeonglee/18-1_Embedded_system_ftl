CFLAGS=-g -Wall
CC=gcc
LIBS=-lm -lpthread
INCLUDES=
OBJS=blueftl_user_vdevice.o blueftl_user_netlink.o blueftl_user_ftl_main.o blueftl_ftl_base.o blueftl_gc_page.o blueftl_mapping_page.o blueftl_ssdmgmt.o blueftl_util.o blueftl_wl_dual_pool.o
SRCS=blueftl_user_ftl_main.c blueftl_ftl_base.c blueftl_gc_page.c blueftl_mapping_page.c blueftl_wl_dual_pool.c blueftl_util.c

# The variable $@ has the value of the target. In this case $@ = psort
blueftl_user: blueftl_user.o ${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ blueftl_user.o ${OBJS} ${LIBS}

clean:
	rm blueftl_user_ftl_main.o blueftl_ftl_base.o blueftl_gc_page.o blueftl_mapping_page.o blueftl_wl_dual_pool.o blueftl_util.o blueftl_user
