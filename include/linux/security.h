/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Linux Security plug
 *
 * Copyright (C) 2001 WireX Communications, Inc <chris@wirex.com>
 * Copyright (C) 2001 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2001 Networks Associates Technology, Inc <ssmalley@nai.com>
 * Copyright (C) 2001 James Morris <jmorris@intercode.com.au>
 * Copyright (C) 2001 Silicon Graphics, Inc. (Trust Technology Group)
 * Copyright (C) 2016 Mellanox Techonologies
 *
 * Stub definitions for Linux Security Module (LSM) hooks.
 * U-Boot doesn't implement security modules.
 */
#ifndef _LINUX_SECURITY_H
#define _LINUX_SECURITY_H

struct inode;
struct dentry;

static inline int security_inode_init_security(struct inode *inode,
					       struct inode *dir,
					       void *name, void *value,
					       void *len)
{
	return -EOPNOTSUPP;
}

#define security_inode_create(dir, dentry, mode)	0
#define security_inode_link(old, dir, new)		0
#define security_inode_unlink(dir, dentry)		0
#define security_inode_symlink(dir, dentry, name)	0
#define security_inode_mkdir(dir, dentry, mode)		0
#define security_inode_rmdir(dir, dentry)		0
#define security_inode_mknod(dir, dentry, mode, dev)	0
#define security_inode_rename(od, odent, nd, ndent, f)	0
#define security_inode_setattr(dentry, attr)		0

#endif /* _LINUX_SECURITY_H */
