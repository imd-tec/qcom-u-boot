/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Filesystem definitions
 *
 * Minimal version for U-Boot ext4l - based on Linux 6.18
 */
#ifndef _LINUX_FS_H
#define _LINUX_FS_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>

/* Forward declarations */
struct inode;
struct super_block;
struct buffer_head;
struct file;
struct folio;
struct readahead_control;
struct kiocb;
struct writeback_control;
struct swap_info_struct;

/* errseq_t - error sequence type */
typedef u32 errseq_t;

/* fmode_t - file mode type */
typedef unsigned int fmode_t;

/* File mode flags */
#define FMODE_READ		((__force fmode_t)(1 << 0))
#define FMODE_WRITE		((__force fmode_t)(1 << 1))
#define FMODE_LSEEK		((__force fmode_t)(1 << 2))
#define FMODE_NOWAIT		((__force fmode_t)(1 << 20))
#define FMODE_CAN_ODIRECT	((__force fmode_t)(1 << 21))
#define FMODE_CAN_ATOMIC_WRITE	((__force fmode_t)(1 << 22))

/* Directory file mode flags - use low bits for hash mode */
#define FMODE_32BITHASH		((__force fmode_t)0x00000001)
#define FMODE_64BITHASH		((__force fmode_t)0x00000002)

/* Seek constants */
#ifndef SEEK_HOLE
#define SEEK_HOLE	4
#define SEEK_DATA	3
#endif

/* vfsmount - mount point */
struct vfsmount {
	struct dentry *mnt_root;
};

/* path - pathname components */
struct path {
	struct vfsmount *mnt;
	struct dentry *dentry;
};

/* Buffer operations are in buffer_head.h */

#ifdef __UBOOT__
/* Maximum number of cached folios per address_space */
#define FOLIO_CACHE_MAX 64
#endif

/* address_space_operations - forward declare for address_space */
struct address_space_operations;

/* address_space - extended for inode.c */
struct address_space {
	struct inode *host;
	errseq_t wb_err;
	unsigned long nrpages;
	unsigned long writeback_index;
	struct list_head i_private_list;
	const struct address_space_operations *a_ops;
#ifdef __UBOOT__
	/* Simple folio cache for U-Boot (no XA/radix tree) */
	struct folio *folio_cache[FOLIO_CACHE_MAX];
	int folio_cache_count;
#endif
};

/* address_space_operations - filesystem address space methods */
struct address_space_operations {
	int (*read_folio)(struct file *, struct folio *);
	void (*readahead)(struct readahead_control *);
	sector_t (*bmap)(struct address_space *, sector_t);
	void (*invalidate_folio)(struct folio *, size_t, size_t);
	bool (*release_folio)(struct folio *, gfp_t);
	int (*write_begin)(const struct kiocb *, struct address_space *,
			   loff_t, unsigned, struct folio **, void **);
	int (*write_end)(const struct kiocb *, struct address_space *,
			 loff_t, unsigned, unsigned, struct folio *, void *);
	int (*writepages)(struct address_space *, struct writeback_control *);
	bool (*dirty_folio)(struct address_space *, struct folio *);
	bool (*is_partially_uptodate)(struct folio *, size_t, size_t);
	int (*error_remove_folio)(struct address_space *, struct folio *);
	int (*migrate_folio)(struct address_space *, struct folio *,
			     struct folio *, int);
	int (*swap_activate)(struct swap_info_struct *, struct file *,
			     sector_t *);
};

/* block_device - minimal stub */
struct block_device {
	struct address_space *bd_mapping;
	void *bd_disk;
	struct super_block *bd_super;
	dev_t bd_dev;
	bool read_only;
};

/* errseq functions - stubs */
static inline int errseq_check(errseq_t *eseq, errseq_t since)
{
	return 0;
}

static inline int errseq_check_and_advance(errseq_t *eseq, errseq_t *since)
{
	return 0;
}

/* File readahead state - stub */
struct file_ra_state {
	unsigned long start;
	unsigned int size;
	unsigned int async_size;
	unsigned int ra_pages;
	unsigned int mmap_miss;
	long long prev_pos;
};

/* file - minimal stub */
struct file {
	fmode_t f_mode;
	struct inode *f_inode;
	unsigned int f_flags;
	struct address_space *f_mapping;
	void *private_data;
	struct file_ra_state f_ra;
	struct path f_path;
	loff_t f_pos;
};

/* Get inode from file */
static inline struct inode *file_inode(struct file *f)
{
	return f->f_inode;
}

/* iattr - inode attributes for setattr */
struct iattr {
	unsigned int ia_valid;
	umode_t ia_mode;
	uid_t ia_uid;
	gid_t ia_gid;
	loff_t ia_size;
};

/* iattr valid flags - specify which fields of iattr are valid */
#define ATTR_MODE	(1 << 0)
#define ATTR_UID	(1 << 1)
#define ATTR_GID	(1 << 2)
#define ATTR_SIZE	(1 << 3)
#define ATTR_ATIME	(1 << 4)
#define ATTR_MTIME	(1 << 5)
#define ATTR_CTIME	(1 << 6)
#define ATTR_ATIME_SET	(1 << 7)
#define ATTR_MTIME_SET	(1 << 8)
#define ATTR_FORCE	(1 << 9)
#define ATTR_KILL_SUID	(1 << 11)
#define ATTR_KILL_SGID	(1 << 12)
#define ATTR_TIMES_SET	(ATTR_ATIME_SET | ATTR_MTIME_SET)

/* writeback_control - defined in linux/compat.h */

/* fsnotify - stub */
#define fsnotify_change(d, m)	do { } while (0)

/* inode_init_once - stub */
static inline void inode_init_once(struct inode *inode)
{
}

/* S_ISDIR, etc. - already in linux/stat.h */
#include <linux/stat.h>

/* Inode flags for i_flags field */
#define S_SYNC		1	/* Synchronous writes */
#define S_NOATIME	2	/* No access time updates */
#define S_APPEND	4	/* Append only */
#define S_IMMUTABLE	8	/* Immutable file */
#define S_DAX		16	/* Direct access */
#define S_DIRSYNC	32	/* Directory sync */
#define S_ENCRYPTED	64	/* Encrypted */
#define S_CASEFOLD	128	/* Case-folded */
#define S_VERITY	256	/* Verity enabled */

/* Permission mode constants */
#define S_IRWXUGO	(S_IRWXU | S_IRWXG | S_IRWXO)
#define S_IRUGO		(S_IRUSR | S_IRGRP | S_IROTH)

/* Rename flags */
#define RENAME_NOREPLACE	(1 << 0)
#define RENAME_EXCHANGE		(1 << 1)
#define RENAME_WHITEOUT		(1 << 2)

/* fallocate() flags */
#define FALLOC_FL_KEEP_SIZE		0x01
#define FALLOC_FL_PUNCH_HOLE		0x02
#define FALLOC_FL_COLLAPSE_RANGE	0x08
#define FALLOC_FL_ZERO_RANGE		0x10
#define FALLOC_FL_INSERT_RANGE		0x20
#define FALLOC_FL_WRITE_ZEROES		0x40
#define FALLOC_FL_ALLOCATE_RANGE	0x80
#define FALLOC_FL_MODE_MASK		0xff

/* Directory entry types */
#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

/* Directory context for readdir iteration */
struct dir_context;
typedef int (*filldir_t)(struct dir_context *, const char *, int, loff_t,
			 u64, unsigned);

struct dir_context {
	filldir_t actor;
	loff_t pos;
};

/* dir_emit - emit a directory entry to the context callback */
static inline bool dir_emit(struct dir_context *ctx, const char *name, int len,
			    u64 ino, unsigned int type)
{
	return ctx->actor(ctx, name, len, ctx->pos, ino, type) == 0;
}

#define dir_relax_shared(i)	({ (void)(i); 1; })

#endif /* _LINUX_FS_H */
