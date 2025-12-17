/* SPDX-License-Identifier: GPL-2.0 */
/*
 * bit-based spin_lock()
 *
 * Minimal version for U-Boot ext4l - based on Linux 6.18
 * U-Boot is single-threaded so these are simplified.
 */
#ifndef __LINUX_BIT_SPINLOCK_H
#define __LINUX_BIT_SPINLOCK_H

#include <linux/bitops.h>

/*
 * bit-based spin_lock() - U-Boot single-threaded version
 */
static inline void bit_spin_lock(int bitnum, unsigned long *addr)
{
	set_bit(bitnum, addr);
}

static inline int bit_spin_trylock(int bitnum, unsigned long *addr)
{
	if (test_bit(bitnum, addr))
		return 0;
	set_bit(bitnum, addr);
	return 1;
}

static inline void bit_spin_unlock(int bitnum, unsigned long *addr)
{
	clear_bit(bitnum, addr);
}

static inline void __bit_spin_unlock(int bitnum, unsigned long *addr)
{
	clear_bit(bitnum, addr);
}

static inline int bit_spin_is_locked(int bitnum, unsigned long *addr)
{
	return test_bit(bitnum, addr);
}

#endif /* __LINUX_BIT_SPINLOCK_H */
