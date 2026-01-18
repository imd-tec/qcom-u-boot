/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Definitions for diskquota-operations. When diskquota is configured these
 * macros expand to the right source-code.
 *
 * Author:  Marco van Wieringen <mvw@planets.elm.net>
 *
 * Stub definitions for quota operations.
 * U-Boot does not support disk quotas.
 */
#ifndef _LINUX_QUOTAOPS_H
#define _LINUX_QUOTAOPS_H

#include <linux/types.h>

struct inode;
struct dentry;
struct kqid;
struct mnt_idmap;
struct iattr;
struct super_block;

/* Quota initialisation and cleanup */
#define dquot_initialize(inode)			({ (void)(inode); 0; })
#define dquot_initialize_needed(inode)		(0)
#define dquot_drop(inode)			do { (void)(inode); } while (0)

/* Inode quota operations */
#define dquot_alloc_inode(inode)		({ (void)(inode); 0; })
#define dquot_free_inode(inode)			do { (void)(inode); } while (0)

/* Block quota operations */
#define dquot_alloc_block(inode, nr) \
	({ (inode)->i_blocks += (nr) << ((inode)->i_blkbits - 9); 0; })
#define dquot_alloc_block_nofail(inode, nr) \
	({ (inode)->i_blocks += (nr) << ((inode)->i_blkbits - 9); 0; })
#define dquot_free_block(inode, nr) \
	do { (inode)->i_blocks -= (nr) << ((inode)->i_blkbits - 9); } while (0)
#define dquot_claim_block(inode, nr)		({ (void)(inode); (void)(nr); 0; })
#define dquot_reclaim_block(inode, nr)		do { } while (0)
#define dquot_reserve_block(inode, nr)		({ (void)(inode); (void)(nr); 0; })
#define dquot_release_reservation_block(inode, nr) \
	do { (void)(inode); (void)(nr); } while (0)

/* Space quota operations */
#define dquot_alloc_space_nodirty(inode, size)	({ (void)(inode); (void)(size); 0; })
#define dquot_free_space_nodirty(inode, size)	do { (void)(inode); (void)(size); } while (0)
#define dquot_claim_space_nodirty(inode, nr)	0
#define dquot_reclaim_space_nodirty(inode, nr)	do { } while (0)

/* Transfer and modification checks */
#define dquot_transfer(idmap, inode, attr) \
	({ (void)(idmap); (void)(inode); (void)(attr); 0; })
#define is_quota_modification(idmap, inode, attr) \
	({ (void)(idmap); (void)(inode); (void)(attr); 0; })

/* Quota control */
#define dquot_disable(sb, type, flags)		0
#define dquot_suspend(sb, type)			({ (void)(sb); (void)(type); 0; })
#define dquot_resume(sb, type)			do { (void)(sb); (void)(type); } while (0)
#define dquot_writeback_dquots(sb, type)	do { (void)(sb); (void)(type); } while (0)
#define dquot_file_open(inode, file)		({ (void)(inode); (void)(file); 0; })

/* Quota format identifiers */
#define QFMT_VFS_OLD		1	/* Original quota format */
#define QFMT_VFS_V0		2	/* 32-bit UID/GID quota */
#define QFMT_VFS_V1		4	/* 32-bit UID/GID with grace period */

/* Quota status queries */
#define sb_has_quota_usage_enabled(sb, type)	0
#define sb_has_quota_limits_enabled(sb, type)	0
#define sb_has_quota_suspended(sb, type)	0
#define sb_has_quota_loaded(sb, type)		0
#define sb_has_quota_active(sb, type)		0
#define sb_any_quota_loaded(sb)			0
#define sb_any_quota_active(sb)			0
#define sb_any_quota_suspended(sb)		({ (void)(sb); 0; })

#endif /* _LINUX_QUOTAOPS_H */
