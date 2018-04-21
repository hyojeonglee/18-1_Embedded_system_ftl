#ifndef _BLUESSD_UTIL
#define _BLUESSD_UTIL

#include <fcntl.h>
#include "blueftl_ftl_base.h"

#if !defined( min )
# define min(a,b) (a<b?a:b)
#endif

#if !defined( max )
# define max(a,b) (a>b?a:b)
#endif

#define MAX_SUMMARY_BUFFER 4096

/* get current time in microsecond */
uint32_t timer_get_timestamp_in_us (void);
uint32_t timer_get_timestamp_in_sec (void);
void timer_init (void);

/* performance profiling */
void perf_init (void);
void perf_inc_page_reads (void);
void perf_inc_page_writes (void);
void perf_inc_blk_erasures (void);
void perf_gc_inc_page_copies (void);
void perf_gc_inc_blk_erasures (void);
void perf_display_results (void);

void perf_wl_set_blk_max_erasures (int32_t cnt);
void perf_wl_log_blk_erasures (struct ftl_context_t* ptr_ftl_context);
void perf_wl_inc_page_copies (void);
void perf_wl_inc_blk_erasures (void);

#endif
