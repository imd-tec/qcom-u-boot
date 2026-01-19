/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_SPINLOCK_H
#define __LINUX_SPINLOCK_H
#define __LINUX_INSIDE_SPINLOCK_H

/*
 * include/linux/spinlock.h - generic spinlock/rwlock declarations
 *
 * here's the role of the various spinlock/rwlock related include files:
 *
 * on SMP builds:
 *
 *  asm/spinlock_types.h: contains the arch_spinlock_t/arch_rwlock_t and the
 *                        initializers
 *
 *  linux/spinlock_types_raw:
 *			  The raw types and initializers
 *  linux/spinlock_types.h:
 *                        defines the generic type and initializers
 *
 *  asm/spinlock.h:       contains the arch_spin_*()/etc. lowlevel
 *                        implementations, mostly inline assembly code
 *
 *   (also included on UP-debug builds:)
 *
 *  linux/spinlock_api_smp.h:
 *                        contains the prototypes for the _spin_*() APIs.
 *
 *  linux/spinlock.h:     builds the final spin_*() APIs.
 *
 * on UP builds:
 *
 *  linux/spinlock_type_up.h:
 *                        contains the generic, simplified UP spinlock type.
 *                        (which is an empty structure on non-debug builds)
 *
 *  linux/spinlock_types_raw:
 *			  The raw RT types and initializers
 *  linux/spinlock_types.h:
 *                        defines the generic type and initializers
 *
 *  linux/spinlock_up.h:
 *                        contains the arch_spin_*()/etc. version of UP
 *                        builds. (which are NOPs on non-debug, non-preempt
 *                        builds)
 *
 *   (included on UP-non-debug builds:)
 *
 *  linux/spinlock_api_up.h:
 *                        builds the _spin_*() APIs.
 *
 *  linux/spinlock.h:     builds the final spin_*() APIs.
 *
 * U-Boot is single-threaded, so spinlocks are stubs (no-ops).
 */

/* Simple spinlock type - just an int for U-Boot */
typedef struct {
	int lock;
} spinlock_t;

#define __SPIN_LOCK_UNLOCKED(lockname)	{ .lock = 0 }
#define DEFINE_SPINLOCK(x)		spinlock_t x = __SPIN_LOCK_UNLOCKED(x)

/* Spinlock operations - all no-ops for single-threaded U-Boot */
#define spin_lock_init(lock)			do { } while (0)
#define spin_lock(lock)				do { } while (0)
#define spin_unlock(lock)			do { } while (0)
#define spin_lock_bh(lock)			do { } while (0)
#define spin_unlock_bh(lock)			do { } while (0)
#define spin_lock_irq(lock)			do { } while (0)
#define spin_unlock_irq(lock)			do { } while (0)
#define spin_lock_irqsave(lock, flags)		do { (void)(flags); } while (0)
#define spin_unlock_irqrestore(lock, flags)	do { (void)(flags); } while (0)
#define spin_trylock(lock)			(1)
#define spin_is_locked(lock)			(0)

/* Assert variants */
#define assert_spin_locked(lock)		do { } while (0)

/* spin_needbreak - check if lock should be released (always false in U-Boot) */
#define spin_needbreak(lock)			({ (void)(lock); 0; })

/* Read-write lock type - just an int for U-Boot */
typedef int rwlock_t;

#define __RW_LOCK_UNLOCKED(lockname)		(0)
#define DEFINE_RWLOCK(x)			rwlock_t x = __RW_LOCK_UNLOCKED(x)

/* Read-write lock operations - all no-ops for single-threaded U-Boot */
#define rwlock_init(lock)			do { } while (0)
#define read_lock(lock)				do { } while (0)
#define read_unlock(lock)			do { } while (0)
#define write_lock(lock)			do { } while (0)
#define write_unlock(lock)			do { } while (0)
#define read_lock_irq(lock)			do { } while (0)
#define read_unlock_irq(lock)			do { } while (0)
#define write_lock_irq(lock)			do { } while (0)
#define write_unlock_irq(lock)			do { } while (0)
#define read_lock_irqsave(lock, flags)		do { (void)(flags); } while (0)
#define read_unlock_irqrestore(lock, flags)	do { (void)(flags); } while (0)
#define write_lock_irqsave(lock, flags)		do { (void)(flags); } while (0)
#define write_unlock_irqrestore(lock, flags)	do { (void)(flags); } while (0)

#endif /* __LINUX_SPINLOCK_H */
