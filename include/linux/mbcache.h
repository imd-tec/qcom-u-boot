/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Metadata block cache definitions for U-Boot
 *
 * Based on Linux mbcache.h - cache for filesystem metadata blocks.
 * U-Boot stubs - metadata caching is not supported.
 */
#ifndef _LINUX_MBCACHE_H
#define _LINUX_MBCACHE_H

#include <linux/types.h>
#include <linux/slab.h>

/**
 * struct mb_cache - metadata block cache
 *
 * U-Boot stub - caching not supported.
 */
struct mb_cache {
	int dummy;
};

/**
 * struct mb_cache_entry - cache entry
 * @e_value: cached value
 * @e_flags: entry flags
 */
struct mb_cache_entry {
	u64 e_value;
	unsigned long e_flags;
};

/* MB cache flags */
#define MBE_REUSABLE_B	0

/* MB cache operations - all stubbed as no-ops */
#define mb_cache_create(bits) \
	kzalloc(sizeof(struct mb_cache), GFP_KERNEL)
#define mb_cache_destroy(cache) \
	do { kfree(cache); } while (0)
#define mb_cache_entry_find_first(c, h) \
	((struct mb_cache_entry *)NULL)
#define mb_cache_entry_find_next(c, e) \
	((struct mb_cache_entry *)NULL)
#define mb_cache_entry_delete_or_get(c, k, v) \
	((struct mb_cache_entry *)NULL)
#define mb_cache_entry_get(c, k, v) \
	((struct mb_cache_entry *)NULL)
#define mb_cache_entry_put(c, e) \
	do { (void)(c); (void)(e); } while (0)
#define mb_cache_entry_create(c, f, k, v, r) \
	({ (void)(c); (void)(f); (void)(k); (void)(v); (void)(r); 0; })
#define mb_cache_entry_delete(c, k, v) \
	do { (void)(c); (void)(k); (void)(v); } while (0)
#define mb_cache_entry_touch(c, e) \
	do { (void)(c); (void)(e); } while (0)
#define mb_cache_entry_wait_unused(e) \
	do { (void)(e); } while (0)

#endif /* _LINUX_MBCACHE_H */
