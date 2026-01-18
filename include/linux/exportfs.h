/* SPDX-License-Identifier: GPL-2.0 */
/*
 * NFS export operations for U-Boot
 *
 * Based on Linux exportfs.h - filesystem export support for NFS.
 */
#ifndef _LINUX_EXPORTFS_H
#define _LINUX_EXPORTFS_H

#include <linux/types.h>

struct super_block;
struct dentry;
struct inode;

/**
 * struct fid - NFS file handle
 *
 * Flexible file handle for NFS export.
 */
struct fid {
	union {
		struct {
			u32 ino;
			u32 gen;
			u32 parent_ino;
			u32 parent_gen;
		} i32;
		__u32 raw[0];
	};
};

/**
 * struct export_operations - NFS export operations
 * @encode_fh: encode file handle
 * @fh_to_dentry: decode file handle to dentry
 * @fh_to_parent: decode file handle to parent dentry
 * @get_parent: get parent directory
 * @commit_metadata: commit metadata changes
 *
 * Operations for NFS export support.
 */
struct export_operations {
	int (*encode_fh)(struct inode *, __u32 *, int *, struct inode *);
	struct dentry *(*fh_to_dentry)(struct super_block *, struct fid *,
				       int, int);
	struct dentry *(*fh_to_parent)(struct super_block *, struct fid *,
				       int, int);
	struct dentry *(*get_parent)(struct dentry *);
	int (*commit_metadata)(struct inode *);
};

/**
 * generic_encode_ino32_fh() - generic file handle encoder
 * @inode: inode to encode
 * @fh: file handle buffer
 * @max_len: maximum length of buffer
 * @parent: parent inode (may be NULL)
 *
 * U-Boot stub - returns 0.
 */
static inline int generic_encode_ino32_fh(struct inode *inode, __u32 *fh,
					  int *max_len, struct inode *parent)
{
	return 0;
}

/* NFS export helpers - stubs for U-Boot */
struct dentry *generic_fh_to_dentry(struct super_block *sb, struct fid *fid,
				    int fh_len, int fh_type,
				    struct inode *(*get_inode)(struct super_block *,
							       u64, u32));
struct dentry *generic_fh_to_parent(struct super_block *sb, struct fid *fid,
				    int fh_len, int fh_type,
				    struct inode *(*get_inode)(struct super_block *,
							       u64, u32));

#endif /* _LINUX_EXPORTFS_H */
