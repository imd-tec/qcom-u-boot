/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Locking bit operations
 *
 * U-Boot stub - single-threaded, no actual locking needed.
 */
#ifndef _ASM_GENERIC_BITOPS_LOCK_H
#define _ASM_GENERIC_BITOPS_LOCK_H

#include <linux/bitops.h>

/**
 * clear_bit_unlock - clear a bit with release semantics
 * @nr: bit number to clear
 * @addr: address of the bitmap
 *
 * U-Boot stub - just calls clear_bit() since we're single-threaded.
 */
#define clear_bit_unlock(nr, addr)	clear_bit(nr, addr)

/**
 * test_and_set_bit_lock - test and set a bit with acquire semantics
 * @nr: bit number to test and set
 * @addr: address of the bitmap
 *
 * U-Boot stub - just calls test_and_set_bit() since we're single-threaded.
 */
#define test_and_set_bit_lock(nr, addr)	test_and_set_bit(nr, addr)

#endif /* _ASM_GENERIC_BITOPS_LOCK_H */
