// SPDX-License-Identifier: GPL-2.0+
/*
 * kmem_cache implementation for U-Boot
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <malloc.h>
#include <linux/slab.h>

void kmem_cache_free(struct kmem_cache *cachep, void *obj)
{
	free(obj);
}

void kmem_cache_destroy(struct kmem_cache *cachep)
{
	free(cachep);
}
