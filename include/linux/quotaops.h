/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Definitions for diskquota-operations. When diskquota is configured these
 * macros expand to the right source-code.
 *
 * Author:  Marco van Wieringen <mvw@planets.elm.net>
 *
 * Stub definitions for quota operations.
 * U-Boot doesn't support disk quotas.
 */
#ifndef _LINUX_QUOTAOPS_H
#define _LINUX_QUOTAOPS_H

struct inode;
struct dentry;
struct kqid;

#define dquot_initialize(inode)		0
#define dquot_drop(inode)		do { } while (0)
#define dquot_alloc_inode(inode)	0
#define dquot_free_inode(inode)		do { } while (0)
#define dquot_transfer(inode, attr)	0
#define dquot_claim_space_nodirty(inode, nr)	0
#define dquot_reclaim_space_nodirty(inode, nr)	do { } while (0)
#define dquot_disable(sb, type, flags)	0
#define dquot_suspend(sb, type)		0
#define dquot_resume(sb, type)		0
#define dquot_file_open(inode, file)	0

#define sb_has_quota_usage_enabled(sb, type)	0
#define sb_has_quota_limits_enabled(sb, type)	0
#define sb_has_quota_suspended(sb, type)	0
#define sb_has_quota_loaded(sb, type)		0
#define sb_has_quota_active(sb, type)		0
#define sb_any_quota_loaded(sb)			0
#define sb_any_quota_active(sb)			0

#endif /* _LINUX_QUOTAOPS_H */
