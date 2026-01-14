/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Lock dependency validator stubs for U-Boot
 *
 * U-Boot is single-threaded, so lock dependency checking is not needed.
 * These stubs allow Linux kernel code to compile unchanged.
 */
#ifndef _LINUX_LOCKDEP_H
#define _LINUX_LOCKDEP_H

/* Lock class key - used for lockdep annotations */
struct lock_class_key {
	int dummy;
};

/* Lockdep map - used for lock tracking */
struct lockdep_map {
	int dummy;
};

/* Lockdep assertion macros - all no-ops in U-Boot */
#define lockdep_is_held(lock)			(1)
#define lockdep_assert_held(lock)		do { (void)(lock); } while (0)
#define lockdep_assert_held_read(lock)		do { (void)(lock); } while (0)
#define lockdep_assert_held_write(lock)		do { (void)(lock); } while (0)
#define lockdep_assert_not_held(lock)		do { (void)(lock); } while (0)

/* Lockdep initialisation and tracking - no-ops */
#define lockdep_init_map(...)			do { } while (0)

/* RW semaphore lockdep stubs */
#define rwsem_acquire(l, s, t, i)		do { } while (0)
#define rwsem_acquire_read(l, s, t, i)		do { } while (0)
#define rwsem_release(l, i)			do { } while (0)

#endif /* _LINUX_LOCKDEP_H */
