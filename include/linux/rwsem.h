/* SPDX-License-Identifier: GPL-2.0 */
/* rwsem.h: R/W semaphores, public interface
 *
 * Written by David Howells (dhowells@redhat.com).
 * Derived from asm-i386/semaphore.h
 *
 * Stub definitions for Linux kernel read-write semaphores.
 * U-Boot is single-threaded, no locking needed.
 */
#ifndef _LINUX_RWSEM_H
#define _LINUX_RWSEM_H

struct rw_semaphore {
	int count;
};

#define DECLARE_RWSEM(name)	struct rw_semaphore name = { 0 }

#define init_rwsem(sem)		do { } while (0)
#define down_read(sem)		do { } while (0)
#define down_read_trylock(sem)	1
#define up_read(sem)		do { } while (0)
#define down_write(sem)		do { } while (0)
#define down_write_trylock(sem)	1
#define down_write_nested(sem, subclass) \
	do { (void)(sem); (void)(subclass); } while (0)
#define up_write(sem)		do { } while (0)
#define downgrade_write(sem)	do { } while (0)
#define rwsem_is_locked(sem)	(1)

#endif /* _LINUX_RWSEM_H */
