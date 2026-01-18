/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Path definitions for U-Boot
 *
 * Based on Linux path.h - filesystem path operations.
 */
#ifndef _LINUX_PATH_H
#define _LINUX_PATH_H

struct dentry;
struct vfsmount;

struct path {
	struct vfsmount *mnt;
	struct dentry *dentry;
};

/**
 * path_put() - release a path reference
 * @path: path to release
 *
 * U-Boot stub - no reference counting.
 */
#define path_put(path)		do { (void)(path); } while (0)

/**
 * d_path() - get pathname from path structure
 * @path: path to convert
 * @buf: buffer for pathname
 * @buflen: size of buffer
 *
 * U-Boot stub - returns empty string.
 *
 * Return: pointer to pathname in buffer
 */
static inline char *d_path(const struct path *path, char *buf, int buflen)
{
	(void)path;
	if (buflen > 0)
		buf[0] = '\0';
	return buf;
}

#endif /* _LINUX_PATH_H */
