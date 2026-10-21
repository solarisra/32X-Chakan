#ifndef CHAKAN_SH_PROFILE_H
#define CHAKAN_SH_PROFILE_H
#include "mars.h"
/* 68000 publishes a monotonic scanline clock. PicoDrive does not advance FRT
 * in this configuration; this clock also measures bus stalls and flip waits. */
static inline void profile_timer_init(void) {}
static inline uint16_t profile_timer_read(void) { return MARS_SYS_COMM10; }
#endif
