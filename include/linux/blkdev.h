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

/* Block device open flags */
#define BLK_OPEN_READ			(1 << 0)
#define BLK_OPEN_WRITE			(1 << 1)
#define BLK_OPEN_EXCL			(1 << 2)
#define BLK_OPEN_NDELAY			(1 << 3)
#define BLK_OPEN_RESTRICT_WRITES	(1 << 4)

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

/* Block device atomic write support - not supported in U-Boot */
#define bdev_can_atomic_write(bdev)		({ (void)(bdev); 0; })
#define bdev_atomic_write_unit_max_bytes(bdev)	({ (void)(bdev); (unsigned int)0; })
#define bdev_atomic_write_unit_min_bytes(bdev)	({ (void)(bdev); 0UL; })

/* Block device read-only check - implemented in ext4l/stub.c */
int bdev_read_only(struct block_device *bdev);

/* Block device property stubs */
#define bdev_write_zeroes_unmap_sectors(b)	({ (void)(b); 0; })
#define bdev_dma_alignment(bd)			(0)
#define bdev_nonrot(bdev)			({ (void)(bdev); 0; })
#define bdev_discard_granularity(bdev)		({ (void)(bdev); 0U; })
#define set_blocksize(f, size)			({ (void)(f); (void)(size); 0; })

/* Block layer constants */
#define BLK_MAX_SEGMENT_SIZE			65536

/* Block device I/O operations - stubs */
#define blkdev_issue_flush(bdev)		({ (void)(bdev); 0; })
#define blkdev_issue_discard(bdev, s, n, gfp) \
	({ (void)(bdev); (void)(s); (void)(n); (void)(gfp); 0; })
#define blkdev_issue_zeroout(bdev, s, n, gfp, f) \
	({ (void)(bdev); (void)(s); (void)(n); (void)(gfp); (void)(f); 0; })

/* Block device sync - implemented in ext4l/stub.c */
int sync_blockdev(struct block_device *bdev);
void invalidate_bdev(struct block_device *bdev);

/* Block device size - implemented in ext4l/stub.c */
unsigned int bdev_max_discard_sectors(struct block_device *bdev);

/* Block device file operations - implemented in ext4l/stub.c */
struct blk_holder_ops;
void bdev_fput(void *file);
void *bdev_file_open_by_dev(dev_t dev, int flags, void *holder,
			    const struct blk_holder_ops *ops);

/* Buffer operations on block devices - implemented in ext4l/stub.c */
struct buffer_head *bdev_getblk(struct block_device *bdev, sector_t block,
				unsigned int size, gfp_t gfp);
struct buffer_head *__bread(struct block_device *bdev, sector_t block,
			    unsigned int size);

#endif /* _LINUX_BLKDEV_H */
