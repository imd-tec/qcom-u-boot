/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Writeback definitions for U-Boot
 *
 * Based on Linux writeback.h - writeback control and operations.
 * U-Boot stub - writeback operations are no-ops.
 */
#ifndef WRITEBACK_H
#define WRITEBACK_H

#include <linux/pagemap.h>
#include <linux/xarray.h>

/* Forward declarations */
struct super_block;
struct writeback_control;

/* Writeback reasons */
#define WB_REASON_FS_FREE_SPACE		0
#define WB_REASON_SYNC			1
#define WB_REASON_PERIODIC		2
#define WB_REASON_LAPTOP_TIMER		3
#define WB_REASON_VMSCAN		4
#define WB_REASON_FORKER_THREAD		5

/**
 * wbc_to_tag() - convert writeback control to pagecache tag
 * @wbc: writeback control structure
 *
 * Return: PAGECACHE_TAG_TOWRITE for sync writes, PAGECACHE_TAG_DIRTY otherwise
 */
static inline xa_mark_t wbc_to_tag(struct writeback_control *wbc)
{
	if (wbc->sync_mode == WB_SYNC_ALL || wbc->tagged_writepages)
		return PAGECACHE_TAG_TOWRITE;
	return PAGECACHE_TAG_DIRTY;
}

/**
 * try_to_writeback_inodes_sb() - try to start writeback on superblock
 * @sb: superblock to writeback
 * @reason: reason for writeback
 *
 * U-Boot stub - no-op since writeback is synchronous.
 */
#define try_to_writeback_inodes_sb(sb, reason) \
	do { (void)(sb); (void)(reason); } while (0)

/**
 * inode_io_list_del() - remove inode from I/O list
 * @inode: inode to remove
 *
 * U-Boot stub - no I/O list management.
 */
#define inode_io_list_del(inode)	do { (void)(inode); } while (0)

#endif /* WRITEBACK_H */
