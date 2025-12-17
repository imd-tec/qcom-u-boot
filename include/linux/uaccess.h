/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_UACCESS_H
#define _LINUX_UACCESS_H

#include <linux/types.h>
#include <string.h>

/*
 * Stub definitions for Linux kernel user-space access functions.
 * In U-Boot there's no user/kernel separation, so these are simple copies.
 */

static inline unsigned long copy_from_user(void *to, const void *from,
					   unsigned long n)
{
	memcpy(to, from, n);
	return 0;
}

static inline unsigned long copy_to_user(void *to, const void *from,
					 unsigned long n)
{
	memcpy(to, from, n);
	return 0;
}

#define get_user(x, ptr) ({ x = *(ptr); 0; })
#define put_user(x, ptr) ({ *(ptr) = x; 0; })

#define access_ok(addr, size)	1

#endif /* _LINUX_UACCESS_H */
