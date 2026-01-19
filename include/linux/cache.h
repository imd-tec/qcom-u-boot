/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Cache alignment definitions for U-Boot
 *
 * Based on Linux include/linux/cache.h
 */
#ifndef _LINUX_CACHE_H
#define _LINUX_CACHE_H

/*
 * U-Boot is single-threaded, so cache line alignment for SMP is not needed.
 * These are provided for compatibility with Linux code.
 */
#ifndef ____cacheline_aligned_in_smp
#define ____cacheline_aligned_in_smp
#endif

#ifndef __cacheline_aligned_in_smp
#define __cacheline_aligned_in_smp
#endif

#endif /* _LINUX_CACHE_H */
