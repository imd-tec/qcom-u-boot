/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Block device definitions
 *
 * Minimal version for U-Boot ext4l - based on Linux 6.18
 */
#ifndef _LINUX_BLKDEV_H
#define _LINUX_BLKDEV_H

#include <linux/types.h>

struct block_device;
struct gendisk;

/* Largest string for a blockdev identifier */
#define BDEVNAME_SIZE	32

/* Block size helpers */
#define bdev_logical_block_size(bdev)	512

/**
 * struct blk_plug - block I/O plug
 *
 * U-Boot stub - block I/O plugging is not used.
 */
struct blk_plug {
	int dummy;
};

/**
 * blk_start_plug() - start plugging block I/O
 * @plug: plug structure
 *
 * U-Boot stub - no-op.
 */
#define blk_start_plug(plug)	do { (void)(plug); } while (0)

/**
 * blk_finish_plug() - finish plugging and submit I/O
 * @plug: plug structure
 *
 * U-Boot stub - no-op.
 */
#define blk_finish_plug(plug)	do { (void)(plug); } while (0)

#endif /* _LINUX_BLKDEV_H */
