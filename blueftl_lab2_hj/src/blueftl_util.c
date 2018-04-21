#ifdef KERNEL_MODE
/* This is provided for better understanidng for perf counts */
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/proc_fs.h>

#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_util.h"
#include "blueftl_wl_dual_pool.h"

#else

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "blueftl_ftl_base.h"
#include "blueftl_ssdmgmt.h"
#include "blueftl_util.h"
#include "blueftl_wl_dual_pool.h"

#endif

static uint32_t timer_startup_timestamp = 0;

uint32_t timer_get_timestamp_in_us (void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);

	return tv.tv_sec * 1000000 + tv.tv_usec;
}

uint32_t timer_get_timestamp_in_sec (void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);

	return tv.tv_sec - timer_startup_timestamp;
}

void timer_init (void)
{
	timer_startup_timestamp = timer_get_timestamp_in_sec ();
}

/* performance profiling */
static uint32_t _perf_nr_page_reads = 0;
static uint32_t _perf_nr_page_writes = 0;
static uint32_t _perf_nr_blk_erasures = 0;
static uint32_t _perf_gc_nr_page_copies = 0;
static uint32_t _perf_gc_nr_blk_erasures = 0;
static uint32_t _perf_wl_nr_page_copies = 0;
static uint32_t _perf_wl_nr_blk_erasures = 0;
static uint32_t _perf_wl_nr_maximum_erasures = 0;

void perf_inc_page_reads (void)
{
	_perf_nr_page_reads++;
}

void perf_inc_page_writes (void)
{
	_perf_nr_page_writes++;
}

void perf_inc_blk_erasures (void)
{
	_perf_nr_blk_erasures++;
}

void perf_gc_inc_page_copies (void)
{
	_perf_gc_nr_page_copies++;
}

void perf_gc_inc_blk_erasures (void)
{
	_perf_gc_nr_blk_erasures++;
}

void perf_wl_inc_page_copies (void)
{
	_perf_wl_nr_page_copies++;
}

void perf_wl_inc_blk_erasures (void)
{
	_perf_wl_nr_blk_erasures++;
}

void perf_wl_set_blk_max_erasures(int32_t cnt)
{
	_perf_wl_nr_maximum_erasures = cnt;
}

void perf_wl_log_blk_erasures(struct ftl_context_t* ptr_ftl_context)
{
	struct flash_ssd_t* ptr_ssd = ptr_ftl_context->ptr_ssd;
	int i;
	FILE *fp;
	char index[100], access[100];
	char *line = NULL;
	char *file_path = "log";
	struct flash_block_t* ptr_block;

	if ((fp = fopen(file_path, "w")) != NULL) {
		fwrite("X Y\n", 4, 1, fp);
		for (i = 0; i < ptr_ssd->nr_blocks_per_chip; i++) {
			ptr_block = &ptr_ssd->list_buses[0].list_chips[0].list_blocks[i];
			sprintf(index, "%u", ptr_block->no_block);
			sprintf(access, "%u", ptr_block->nr_erase_cnt);
			line = (char *)malloc((strlen(index) + strlen(access) + 2) *sizeof(char));
			sprintf(line, "%s %s\n", index, access);
			fwrite(line, strlen(line), 1, fp);
		}
	}
	else
		printf("no file!\n");
	fclose(fp);
}

void perf_display_results (void)
{
	printf ("bluessd: ==========================Performance summary========================\n");
	printf ("bluessd: # of page reads: %u\n", _perf_nr_page_reads);
	printf ("bluessd: # of page writes: %u\n", _perf_nr_page_writes);
	printf ("bluessd: # of blk erasures: %u\n", _perf_nr_blk_erasures);
	printf ("bluessd: ===================Wear-leveling performance summary=================\n");
	printf ("bluessd: # of page copies: %u\n", _perf_wl_nr_page_copies);
	printf ("bluessd: # of blk erasures: %u\n", _perf_wl_nr_blk_erasures);
	printf ("bluessd: # of erasure count of the oldest blk: %u\n", _perf_wl_nr_maximum_erasures);
	printf ("bluessd: =====================================================================\n");
}

