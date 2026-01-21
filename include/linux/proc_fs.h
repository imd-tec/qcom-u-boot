/* SPDX-License-Identifier: GPL-2.0 */
/*
 * The proc filesystem constants/structures
 */

#ifndef _LINUX_PROC_FS_H
#define _LINUX_PROC_FS_H

/* proc_fs is not used in U-Boot - provide empty stubs */

struct proc_dir_entry;
struct inode;
struct file;

/**
 * struct proc_ops - proc file operations
 * @proc_open: open callback
 * @proc_read: read callback
 * @proc_lseek: seek callback
 * @proc_release: release callback
 */
struct proc_ops {
	int (*proc_open)(struct inode *, struct file *);
	ssize_t (*proc_read)(struct file *, char *, size_t, loff_t *);
	loff_t (*proc_lseek)(struct file *, loff_t, int);
	int (*proc_release)(struct inode *, struct file *);
};

/* procfs stubs - not supported in U-Boot */
#define proc_mkdir(name, parent) \
	({ (void)(name); (void)(parent); (struct proc_dir_entry *)NULL; })
#define proc_create_data(n, m, p, ops, d) \
	({ (void)(n); (void)(m); (void)(p); (void)(ops); (void)(d); \
	   (struct proc_dir_entry *)NULL; })
#define remove_proc_entry(n, p) \
	do { (void)(n); (void)(p); } while (0)
#define pde_data(inode)		((void *)NULL)

#endif /* _LINUX_PROC_FS_H */
