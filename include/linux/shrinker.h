/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Shrinker interface stub for U-Boot
 *
 * U-Boot doesn't have memory pressure or reclaim, so these are stubs.
 */
#ifndef _LINUX_SHRINKER_H
#define _LINUX_SHRINKER_H

#include <linux/types.h>

/**
 * struct shrink_control - control structure for shrinker callbacks
 * @gfp_mask: allocation flags
 * @nid: NUMA node being shrunk
 * @nr_to_scan: number of objects to scan
 * @nr_scanned: number of objects scanned
 *
 * Stub for U-Boot - memory reclaim is not needed.
 */
struct shrink_control {
	gfp_t gfp_mask;
	int nid;
	unsigned long nr_to_scan;
	unsigned long nr_scanned;
};

struct shrinker;

/**
 * struct shrinker - memory reclaim callback structure
 * @count_objects: callback to count freeable objects
 * @scan_objects: callback to scan and free objects
 * @private_data: private data for the shrinker
 *
 * Stub for U-Boot - memory reclaim is not needed.
 */
struct shrinker {
	unsigned long (*count_objects)(struct shrinker *,
				       struct shrink_control *);
	unsigned long (*scan_objects)(struct shrinker *,
				      struct shrink_control *);
	void *private_data;
};

/**
 * shrinker_alloc() - allocate a shrinker structure
 * @flags: shrinker flags
 * @fmt: format string for name (unused)
 *
 * U-Boot stub - returns a static dummy shrinker.
 *
 * Return: pointer to dummy shrinker
 */
static inline struct shrinker *shrinker_alloc(unsigned int flags,
					      const char *fmt, ...)
{
	static struct shrinker dummy_shrinker;

	return &dummy_shrinker;
}

/**
 * shrinker_register() - register a shrinker
 * @s: shrinker to register
 *
 * U-Boot stub - no-op.
 */
static inline void shrinker_register(struct shrinker *s)
{
}

/**
 * shrinker_free() - free a shrinker
 * @s: shrinker to free
 *
 * U-Boot stub - no-op.
 */
static inline void shrinker_free(struct shrinker *s)
{
}

#endif /* _LINUX_SHRINKER_H */
