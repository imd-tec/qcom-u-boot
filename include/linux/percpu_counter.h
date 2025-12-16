/* SPDX-License-Identifier: GPL-2.0 */
/*
 * A simple "approximate counter" for use in ext2 and ext3 superblocks.
 *
 * WARNING: these things are HUGE.  4 kbytes per counter on 32-way P4.
 *
 * Stub definitions for percpu counters.
 * U-Boot is single-threaded, use simple counters.
 */
#ifndef _LINUX_PERCPU_COUNTER_H
#define _LINUX_PERCPU_COUNTER_H

#include <linux/types.h>

struct percpu_counter {
	s64 count;
};

static inline int percpu_counter_init(struct percpu_counter *fbc, s64 amount,
				      gfp_t gfp)
{
	fbc->count = amount;
	return 0;
}

static inline void percpu_counter_destroy(struct percpu_counter *fbc)
{
}

static inline void percpu_counter_set(struct percpu_counter *fbc, s64 amount)
{
	fbc->count = amount;
}

static inline void percpu_counter_add(struct percpu_counter *fbc, s64 amount)
{
	fbc->count += amount;
}

static inline void percpu_counter_sub(struct percpu_counter *fbc, s64 amount)
{
	fbc->count -= amount;
}

static inline void percpu_counter_inc(struct percpu_counter *fbc)
{
	fbc->count++;
}

static inline void percpu_counter_dec(struct percpu_counter *fbc)
{
	fbc->count--;
}

static inline s64 percpu_counter_read(struct percpu_counter *fbc)
{
	return fbc->count;
}

static inline s64 percpu_counter_read_positive(struct percpu_counter *fbc)
{
	return fbc->count > 0 ? fbc->count : 0;
}

static inline s64 percpu_counter_sum(struct percpu_counter *fbc)
{
	return fbc->count;
}

static inline s64 percpu_counter_sum_positive(struct percpu_counter *fbc)
{
	return fbc->count > 0 ? fbc->count : 0;
}

static inline bool percpu_counter_initialized(struct percpu_counter *fbc)
{
	return true;
}

#endif /* _LINUX_PERCPU_COUNTER_H */
