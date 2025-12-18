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

/* Block size helpers */
#define bdev_logical_block_size(bdev)	512

#endif /* _LINUX_BLKDEV_H */
