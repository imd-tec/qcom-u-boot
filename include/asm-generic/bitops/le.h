/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Little-endian bitops for U-Boot
 *
 * Based on Linux include/asm-generic/bitops/le.h
 */
#ifndef _ASM_GENERIC_BITOPS_LE_H
#define _ASM_GENERIC_BITOPS_LE_H

#include <asm/bitops.h>

/*
 * Little-endian bit operations.
 * These operate on byte boundaries regardless of CPU endianness.
 */

#define find_next_zero_bit_le(addr, size, offset) \
	find_next_zero_bit((void *)(addr), (size), (offset))

#define find_next_bit_le(addr, size, offset) \
	ext4_find_next_bit_le((addr), (size), (offset))

static inline int test_bit_le(int nr, const void *addr)
{
	return test_bit(nr, addr);
}

static inline void __set_bit_le(int nr, void *addr)
{
	set_bit(nr, addr);
}

static inline void __clear_bit_le(int nr, void *addr)
{
	clear_bit(nr, addr);
}

static inline int __test_and_set_bit_le(int nr, void *addr)
{
	int old = test_bit(nr, addr);

	set_bit(nr, addr);
	return old;
}

static inline int __test_and_clear_bit_le(int nr, void *addr)
{
	int old = test_bit(nr, addr);

	clear_bit(nr, addr);
	return old;
}

/*
 * ext4_find_next_bit_le - find next set bit in little-endian bitmap
 * @addr: bitmap address
 * @size: bitmap size in bits
 * @offset: starting bit position
 *
 * Return: bit position of next set bit, or @size if none found
 */
static inline unsigned long ext4_find_next_bit_le(const void *addr,
						  unsigned long size,
						  unsigned long offset)
{
	const unsigned char *p = addr;
	unsigned long bit;

	for (bit = offset; bit < size; bit++) {
		if (p[bit >> 3] & (1 << (bit & 7)))
			return bit;
	}
	return size;
}

#endif /* _ASM_GENERIC_BITOPS_LE_H */
