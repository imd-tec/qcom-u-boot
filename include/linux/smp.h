/* SPDX-License-Identifier: GPL-2.0 */
/*
 * SMP stubs for U-Boot
 *
 * U-Boot is single-threaded, so all SMP operations are stubs.
 */
#ifndef _LINUX_SMP_H
#define _LINUX_SMP_H

#include <linux/types.h>

/**
 * raw_smp_processor_id() - get current processor ID
 *
 * U-Boot stub - always returns 0 (single CPU).
 */
#define raw_smp_processor_id()	0

/**
 * smp_processor_id() - get current processor ID
 *
 * U-Boot stub - always returns 0 (single CPU).
 */
#define smp_processor_id()	0

/* Memory barriers - stubs for single-threaded U-Boot */

/**
 * smp_rmb() - read memory barrier
 *
 * Ensures that all reads before this point are completed before
 * any reads after this point. No-op in single-threaded U-Boot.
 */
#define smp_rmb()		do { } while (0)

/**
 * smp_wmb() - write memory barrier
 *
 * Ensures that all writes before this point are completed before
 * any writes after this point. No-op in single-threaded U-Boot.
 */
#define smp_wmb()		do { } while (0)

/**
 * smp_mb() - full memory barrier
 *
 * Ensures that all memory operations before this point are completed
 * before any memory operations after this point. No-op in single-threaded
 * U-Boot.
 */
#define smp_mb()		do { } while (0)

/**
 * smp_mb__after_atomic() - memory barrier after atomic operation
 *
 * No-op in single-threaded U-Boot.
 */
#define smp_mb__after_atomic()	do { } while (0)

#endif /* _LINUX_SMP_H */
