/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Generic cache management functions. Everything is arch-specific,
 * but this header exists to make sure the defines/functions can be
 * used in a generic way.
 *
 * 2000-11-13  Arjan van de Ven   <arjan@fenrus.demon.nl>
 *
 * Stub definitions for prefetch operations.
 */
#ifndef _LINUX_PREFETCH_H
#define _LINUX_PREFETCH_H

#define prefetch(x)		do { } while (0)
#define prefetchw(x)		do { } while (0)

#endif /* _LINUX_PREFETCH_H */
