/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Block group lock definitions for U-Boot
 *
 * Based on Linux blockgroup_lock.h - per-block-group locking.
 * U-Boot stub - locking not needed in single-threaded environment.
 */
#ifndef _LINUX_BLOCKGROUP_LOCK_H
#define _LINUX_BLOCKGROUP_LOCK_H

/**
 * struct blockgroup_lock - per-block-group lock
 * @num_locks: number of locks (unused in U-Boot)
 *
 * U-Boot stub - real locking not needed.
 */
struct blockgroup_lock {
	int num_locks;
};

/* Block group lock operations - all no-ops */
#define bgl_lock_init(lock)		do { } while (0)
#define bgl_lock_ptr(lock, group)	((spinlock_t *)NULL)

#endif /* _LINUX_BLOCKGROUP_LOCK_H */
