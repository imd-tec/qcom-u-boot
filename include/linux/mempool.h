/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Memory pool stubs for U-Boot
 *
 * U-Boot doesn't have memory pools, so these are stubs.
 */
#ifndef _LINUX_MEMPOOL_H
#define _LINUX_MEMPOOL_H

#include <linux/types.h>

/**
 * typedef mempool_t - memory pool handle
 *
 * U-Boot stub - memory pools are not used.
 */
typedef void *mempool_t;

/**
 * mempool_alloc() - allocate element from pool
 * @pool: memory pool
 * @gfp: allocation flags
 *
 * U-Boot stub - always returns NULL.
 */
#define mempool_alloc(pool, gfp) \
	({ (void)(pool); (void)(gfp); (void *)NULL; })

/**
 * mempool_free() - free element back to pool
 * @elem: element to free
 * @pool: memory pool
 *
 * U-Boot stub - no-op.
 */
#define mempool_free(elem, pool) \
	do { (void)(elem); (void)(pool); } while (0)

/**
 * mempool_create_slab_pool() - create a memory pool backed by a slab cache
 * @min_nr: minimum number of elements
 * @cache: slab cache
 *
 * U-Boot stub - always returns NULL.
 */
#define mempool_create_slab_pool(min_nr, cache) \
	({ (void)(min_nr); (void)(cache); (mempool_t *)NULL; })

/**
 * mempool_destroy() - destroy a memory pool
 * @pool: memory pool to destroy
 *
 * U-Boot stub - no-op.
 */
#define mempool_destroy(pool)	do { (void)(pool); } while (0)

#endif /* _LINUX_MEMPOOL_H */
