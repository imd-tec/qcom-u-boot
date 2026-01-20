/* SPDX-License-Identifier: GPL-2.0 */
/*
  File: linux/xattr.h

  Extended attributes handling.

  Copyright (C) 2001 by Andreas Gruenbacher <a.gruenbacher@computer.org>
  Copyright (c) 2001-2002 Silicon Graphics, Inc.  All Rights Reserved.
  Copyright (c) 2004 Red Hat, Inc., James Morris <jmorris@redhat.com>
*/

#ifndef _LINUX_XATTR_H
#define _LINUX_XATTR_H

#include <linux/types.h>

/* XATTR namespace prefixes */
#define XATTR_USER_PREFIX	"user."
#define XATTR_USER_PREFIX_LEN	5

#define XATTR_TRUSTED_PREFIX	"trusted."
#define XATTR_TRUSTED_PREFIX_LEN	8

#define XATTR_SECURITY_PREFIX	"security."
#define XATTR_SECURITY_PREFIX_LEN	9

#define XATTR_SYSTEM_PREFIX	"system."
#define XATTR_SYSTEM_PREFIX_LEN	7

#define XATTR_HURD_PREFIX	"gnu."
#define XATTR_HURD_PREFIX_LEN	4

/* Maximum size of an xattr value */
#define XATTR_SIZE_MAX		65536

/* Maximum length of an xattr name */
#define XATTR_NAME_MAX		255

/* Maximum size of a listxattr buffer */
#define XATTR_LIST_MAX		65536

struct xattr_handler {
	const char *name;
	const char *prefix;
	int flags;
	bool (*list)(struct dentry *dentry);
	int (*get)(const struct xattr_handler *handler,
		   struct dentry *dentry, struct inode *inode,
		   const char *name, void *buffer, size_t size);
	int (*set)(const struct xattr_handler *handler,
		   struct mnt_idmap *idmap, struct dentry *dentry,
		   struct inode *inode, const char *name, const void *value,
		   size_t size, int flags);
};

/* Common flags */
#define XATTR_CREATE	0x1
#define XATTR_REPLACE	0x2

/* xattr handler helpers - stubs for U-Boot */
#define xattr_handler_can_list(h, d)	({ (void)(h); (void)(d); 0; })
#define xattr_prefix(h)			({ (void)(h); (const char *)NULL; })

#endif /* _LINUX_XATTR_H */
