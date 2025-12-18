/* SPDX-License-Identifier: GPL-2.0 */
/*
  File: linux/posix_acl_xattr.h

  Extended attribute system call representation of Access Control Lists.

  Copyright (C) 2000 by Andreas Gruenbacher <a.gruenbacher@computer.org>
  Copyright (C) 2002 SGI - Silicon Graphics, Inc <linux-xfs@oss.sgi.com>
 */
#ifndef _POSIX_ACL_XATTR_H
#define _POSIX_ACL_XATTR_H

#include <linux/types.h>

/* ACL entry structure for on-disk format */
struct posix_acl_xattr_entry {
	__le16 e_tag;
	__le16 e_perm;
	__le32 e_id;
};

struct posix_acl_xattr_header {
	__le32 a_version;
};

/* POSIX ACL in-memory structure */
struct posix_acl_entry {
	short e_tag;
	unsigned short e_perm;
	union {
		kuid_t e_uid;
		kgid_t e_gid;
	};
};

struct posix_acl {
	int a_count;
	struct posix_acl_entry a_entries[];
};

/* ACL extended attribute names */
#define XATTR_NAME_POSIX_ACL_ACCESS	"system.posix_acl_access"
#define XATTR_NAME_POSIX_ACL_DEFAULT	"system.posix_acl_default"

/* ACL tag types */
#define ACL_UNDEFINED_TAG	(0x00)
#define ACL_USER_OBJ		(0x01)
#define ACL_USER		(0x02)
#define ACL_GROUP_OBJ		(0x04)
#define ACL_GROUP		(0x08)
#define ACL_MASK		(0x10)
#define ACL_OTHER		(0x20)

/* ACL permissions */
#define ACL_READ		(0x04)
#define ACL_WRITE		(0x02)
#define ACL_EXECUTE		(0x01)

/* Stubs for U-Boot */
static inline struct posix_acl *get_inode_acl(struct inode *inode, int type)
{
	return NULL;
}

static inline void posix_acl_release(struct posix_acl *acl)
{
}

#endif /* _POSIX_ACL_XATTR_H */
