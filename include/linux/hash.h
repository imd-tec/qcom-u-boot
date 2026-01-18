/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Hash functions for U-Boot
 *
 * Based on Linux hash.h - fast hashing routines.
 */
#ifndef _LINUX_HASH_H
#define _LINUX_HASH_H

#include <linux/types.h>

/**
 * hash_64() - 64-bit hash function
 * @val: value to hash
 * @bits: number of bits in result
 *
 * Simple hash by shifting. In Linux this uses multiplication by a
 * golden ratio constant, but for U-Boot a simple shift suffices.
 *
 * Return: hash value with @bits significant bits
 */
#define hash_64(val, bits)	((unsigned long)((val) >> (64 - (bits))))

/**
 * hash_32() - 32-bit hash function
 * @val: value to hash
 * @bits: number of bits in result
 *
 * Return: hash value with @bits significant bits
 */
#define hash_32(val, bits)	((unsigned int)((val) >> (32 - (bits))))

#endif /* _LINUX_HASH_H */
