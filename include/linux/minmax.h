/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Min/max and related utilities for U-Boot
 *
 * Based on Linux minmax.h - min, max, clamp, and range helpers.
 */
#ifndef _LINUX_MINMAX_H
#define _LINUX_MINMAX_H

#include <linux/types.h>

/**
 * in_range - check if value is within a range
 * @val: value to test
 * @start: start of range (inclusive)
 * @len: length of range
 *
 * Return: true if @val is in [@start, @start + @len), false otherwise
 */
static inline bool in_range(unsigned long val, unsigned long start,
			    unsigned long len)
{
	return val >= start && val < start + len;
}

/**
 * in_range64 - check if 64-bit value is within a range
 * @val: value to test
 * @start: start of range (inclusive)
 * @len: length of range
 *
 * Return: true if @val is in [@start, @start + @len), false otherwise
 */
static inline bool in_range64(u64 val, u64 start, u64 len)
{
	return (val - start) < len;
}

/**
 * in_range32 - check if 32-bit value is within a range
 * @val: value to test
 * @start: start of range (inclusive)
 * @len: length of range
 *
 * Return: true if @val is in [@start, @start + @len), false otherwise
 */
static inline bool in_range32(u32 val, u32 start, u32 len)
{
	return (val - start) < len;
}

#endif /* _LINUX_MINMAX_H */
