/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Written by Mark Hemment, 1996 (markhe@nextd.demon.co.uk).
 *
 * (C) SGI 2006, Christoph Lameter
 *	Cleaned up and restructured to ease the addition of alternative
 *	implementations of SLAB allocators.
 * (C) Linux Foundation 2008-2013
 *      Unified interface for all slab allocators
 *
 * Memory allocation functions for Linux kernel compatibility.
 * These map to U-Boot's malloc/free infrastructure.
 */
#ifndef _LINUX_SLAB_H
#define _LINUX_SLAB_H

#include <malloc.h>
#include <linux/types.h>

#ifndef GFP_ATOMIC
#define GFP_ATOMIC	((gfp_t)0)
#endif
#ifndef GFP_KERNEL
#define GFP_KERNEL	((gfp_t)0)
#endif
#ifndef GFP_NOFS
#define GFP_NOFS	((gfp_t)0)
#endif
#ifndef GFP_USER
#define GFP_USER	((gfp_t)0)
#endif
#ifndef GFP_NOWAIT
#define GFP_NOWAIT	((gfp_t)0)
#endif
#ifndef __GFP_NOWARN
#define __GFP_NOWARN	((gfp_t)0)
#endif
#ifndef __GFP_ZERO
#define __GFP_ZERO	((__force gfp_t)0x8000u)
#endif
#ifndef __GFP_NOFAIL
#define __GFP_NOFAIL	((gfp_t)0)
#endif

void *kmalloc(size_t size, gfp_t flags);

static inline void *kzalloc(size_t size, gfp_t flags)
{
	return kmalloc(size, flags | __GFP_ZERO);
}

static inline void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	if (size != 0 && n > SIZE_MAX / size)
		return NULL;
	return kmalloc(n * size, flags | __GFP_ZERO);
}

static inline void *kcalloc(size_t n, size_t size, gfp_t flags)
{
	return kmalloc_array(n, size, flags | __GFP_ZERO);
}

static inline void kfree(const void *block)
{
	free((void *)block);
}

static inline void *krealloc(const void *p, size_t new_size, gfp_t flags)
{
	return realloc((void *)p, new_size);
}

void *kmemdup(const void *src, size_t len, gfp_t gfp);

/* kmem_cache stubs */
struct kmem_cache {
	int sz;
};

struct kmem_cache *get_mem(int element_sz);
#define kmem_cache_create(a, sz, c, d, e)	get_mem(sz)
void *kmem_cache_alloc(struct kmem_cache *obj, gfp_t flag);

static inline void kmem_cache_free(struct kmem_cache *cachep, void *obj)
{
	free(obj);
}

static inline void kmem_cache_destroy(struct kmem_cache *cachep)
{
	free(cachep);
}

#endif /* _LINUX_SLAB_H */
