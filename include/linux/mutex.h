/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mutexes: blocking mutual exclusion locks
 *
 * Minimal version for U-Boot ext4l - based on Linux 6.18
 * U-Boot is single-threaded so these are mostly no-ops.
 */
#ifndef __LINUX_MUTEX_H
#define __LINUX_MUTEX_H

#include <linux/types.h>

/*
 * Simple mutex for U-Boot (single-threaded, so these are no-ops)
 */
struct mutex {
	int locked;
};

/* No-op macros that don't reference argument - for backward compatibility */
#define DEFINE_MUTEX(name)	struct mutex name __maybe_unused = { .locked = 0 }
#define mutex_init(lock)	do { } while (0)
#define mutex_lock(lock)	do { } while (0)
#define mutex_unlock(lock)	do { } while (0)
#define mutex_trylock(lock)	({ 1; })
#define mutex_is_locked(lock)	({ 0; })
#define mutex_destroy(lock)	do { } while (0)
#define mutex_lock_io(lock)	mutex_lock(lock)

#define __MUTEX_INITIALIZER(lockname)	{ .locked = 0 }

#endif /* __LINUX_MUTEX_H */
