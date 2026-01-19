/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Random number generation definitions for U-Boot
 *
 * Based on Linux random.h - random number generation.
 * U-Boot stub - returns constant values.
 */
#ifndef _LINUX_RANDOM_H
#define _LINUX_RANDOM_H

#include <linux/types.h>

#define get_random_bytes(buf, len)	do { } while (0)
#define prandom_u32()			0
#define get_random_u32()		0
#define get_random_u64()		0ULL

/**
 * get_random_u32_below() - get random number below a ceiling
 * @ceil: upper bound (exclusive)
 *
 * U-Boot stub - always returns 0.
 *
 * Return: random value in [0, ceil)
 */
#define get_random_u32_below(ceil)	(0)

/**
 * prandom_u32_max() - get random number up to a maximum
 * @max: upper bound (inclusive)
 *
 * U-Boot stub - always returns 0.
 *
 * Return: random value in [0, max]
 */
#define prandom_u32_max(max)		(0)

#endif /* _LINUX_RANDOM_H */
