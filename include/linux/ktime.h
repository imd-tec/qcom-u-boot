/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ktime_t - nanosecond-resolution time format.
 *
 * Stub implementation for U-Boot.
 */
#ifndef _LINUX_KTIME_H
#define _LINUX_KTIME_H

#include <linux/types.h>

/* ktime_t is defined in linux/types.h */

/**
 * ktime_get() - get current time
 *
 * U-Boot stub - returns 0 as we don't track real time during operations.
 *
 * Return: current time as ktime_t (always 0)
 */
static inline ktime_t ktime_get(void)
{
	return 0;
}

/**
 * ktime_to_ns() - convert ktime_t to nanoseconds
 * @kt: the ktime_t value to convert
 *
 * Return: the nanosecond value
 */
static inline s64 ktime_to_ns(ktime_t kt)
{
	return kt;
}

/**
 * ktime_sub() - subtract two ktime_t values
 * @a: first ktime_t value
 * @b: second ktime_t value
 *
 * Return: a - b
 */
static inline ktime_t ktime_sub(ktime_t a, ktime_t b)
{
	return a - b;
}

/**
 * ktime_add_ns() - add nanoseconds to a ktime_t value
 * @kt: base ktime_t value
 * @ns: nanoseconds to add
 *
 * Return: kt + ns
 */
static inline ktime_t ktime_add_ns(ktime_t kt, s64 ns)
{
	return kt + ns;
}

/**
 * ktime_get_ns() - get current time in nanoseconds
 *
 * U-Boot stub - returns 0.
 *
 * Return: current time in nanoseconds (always 0)
 */
#define ktime_get_ns()		(0ULL)

#endif /* _LINUX_KTIME_H */
