/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2023 Addiva Elektronik
 * Author: Tobias Waldekranz <tobias@waldekranz.com>
 *
 * Internal blkmap structures and functions
 */

#ifndef _BLKMAP_INTERNAL_H
#define _BLKMAP_INTERNAL_H

#include <dm/lists.h>

struct blkmap;

/**
 * struct blkmap_slice - Region mapped to a blkmap
 *
 * Common data for a region mapped to a blkmap, specialized by each
 * map type.
 *
 * @node: List node used to associate this slice with a blkmap
 * @blknr: Start block number of the mapping
 * @blkcnt: Number of blocks covered by this mapping
 */
struct blkmap_slice {
	struct list_head node;

	lbaint_t blknr;
	lbaint_t blkcnt;

	/**
	 * @read: - Read from slice
	 *
	 * @read.bm: Blkmap to which this slice belongs
	 * @read.bms: This slice
	 * @read.blknr: Start block number to read from
	 * @read.blkcnt: Number of blocks to read
	 * @read.buffer: Buffer to store read data to
	 */
	ulong (*read)(struct blkmap *bm, struct blkmap_slice *bms,
		      lbaint_t blknr, lbaint_t blkcnt, void *buffer);

	/**
	 * @write: - Write to slice
	 *
	 * @write.bm: Blkmap to which this slice belongs
	 * @write.bms: This slice
	 * @write.blknr: Start block number to write to
	 * @write.blkcnt: Number of blocks to write
	 * @write.buffer: Data to be written
	 */
	ulong (*write)(struct blkmap *bm, struct blkmap_slice *bms,
		       lbaint_t blknr, lbaint_t blkcnt, const void *buffer);

	/**
	 * @destroy: - Tear down slice
	 *
	 * @read.bm: Blkmap to which this slice belongs
	 * @read.bms: This slice
	 */
	void (*destroy)(struct blkmap *bm, struct blkmap_slice *bms);
};

/**
 * blkmap_slice_add() - Add a slice to a blkmap
 *
 * @bm: Blkmap to add the slice to
 * @new: New slice to add
 * Returns: 0 on success, negative error code on failure
 */
int blkmap_slice_add(struct blkmap *bm, struct blkmap_slice *new);

#endif /* _BLKMAP_INTERNAL_H */
