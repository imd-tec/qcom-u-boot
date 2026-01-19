/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Superblock definitions
 *
 * Minimal version for U-Boot - based on Linux
 */
#ifndef _LINUX_FS_SUPER_TYPES_H
#define _LINUX_FS_SUPER_TYPES_H

#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/uuid.h>

/* Forward declarations */
struct block_device;
struct dentry;
struct file_system_type;
struct super_operations;
struct export_operations;
struct xattr_handler;

/* sb_writers stub */
struct sb_writers {
	int frozen;
};

/* super_block - filesystem superblock */
struct super_block {
	void *s_fs_info;
	unsigned long s_blocksize;
	unsigned char s_blocksize_bits;
	unsigned long s_magic;
	loff_t s_maxbytes;
	unsigned long s_flags;
	unsigned long s_iflags;		/* Internal flags */
	struct rw_semaphore s_umount;
	struct sb_writers s_writers;
	struct block_device *s_bdev;
	char s_id[32];
	struct dentry *s_root;
	uuid_t s_uuid;
	struct file_system_type *s_type;
	s32 s_time_gran;		/* Time granularity (ns) */
	time64_t s_time_min;		/* Min supported time */
	time64_t s_time_max;		/* Max supported time */
	const struct super_operations *s_op;
	const struct export_operations *s_export_op;
	const struct xattr_handler * const *s_xattr;
	struct dentry *d_sb;		/* Parent dentry - stub */

	/* U-Boot: list of all inodes, for freeing on unmount */
	struct list_head s_inodes;
};

/* Superblock flags - also defined in linux/fs.h */
#ifndef SB_RDONLY
#define SB_RDONLY	(1 << 0)	/* Read-only mount */
#endif

/* sb_rdonly - check if filesystem is mounted read-only */
static inline bool sb_rdonly(const struct super_block *sb)
{
	return sb->s_flags & SB_RDONLY;
}

#endif /* _LINUX_FS_SUPER_TYPES_H */
