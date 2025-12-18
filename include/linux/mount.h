/* SPDX-License-Identifier: GPL-2.0 */
/*
 *
 * Definitions for mount interface. This describes the in the kernel build
 * linkedlist with mounted filesystems.
 *
 * Author:  Marco van Wieringen <mvw@planets.elm.net>
 *
 */

#ifndef _LINUX_MOUNT_H
#define _LINUX_MOUNT_H

struct vfsmount {
	struct dentry *mnt_root;
	struct super_block *mnt_sb;
};

#endif /* _LINUX_MOUNT_H */
