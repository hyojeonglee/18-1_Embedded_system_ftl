# File Name: reset.sh
# Author: charlie_chen
# mail: charliecqc@dcslab.snu.ac.kr
# Created Time: 2018년 04월 24일 (화) 오후 01시 18분 48초
#########################################################################
#!/bin/bash
umount /media/blueSSD
rmmod blueftl
insmod /home/ssd/embedded_ftl/blueftl3/bin/blueftl.ko
/home/ssd/embedded_ftl/blueftl3/bin/blueftl_reset
