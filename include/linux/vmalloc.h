/* SPDX-License-Identifier: GPL-2.0 */
/*
 * vmalloc functions for Linux kernel compatibility.
 * In U-Boot, these just map to regular malloc.
 */
#ifndef _LINUX_VMALLOC_H
#define _LINUX_VMALLOC_H

#include <linux/slab.h>

#define vmalloc(size)			kmalloc(size, 0)
#define __vmalloc(size, flags, pgsz)	kmalloc(size, flags)

static inline void *vzalloc(unsigned long size)
{
	return kzalloc(size, 0);
}

static inline void vfree(const void *addr)
{
	free((void *)addr);
}

#endif /* _LINUX_VMALLOC_H */
