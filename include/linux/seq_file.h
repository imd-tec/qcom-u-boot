/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_SEQ_FILE_H
#define _LINUX_SEQ_FILE_H

#include <linux/types.h>

/*
 * Stub definitions for seq_file interface.
 * U-Boot doesn't use /proc filesystem.
 */

struct seq_file {
	void *private;
	struct file *file;
};

/* seq_operations for procfs iteration */
struct seq_operations {
	void *(*start)(struct seq_file *m, loff_t *pos);
	void (*stop)(struct seq_file *m, void *v);
	void *(*next)(struct seq_file *m, void *v, loff_t *pos);
	int (*show)(struct seq_file *m, void *v);
};

/* SEQ_START_TOKEN for iteration start marker */
#define SEQ_START_TOKEN			((void *)1)

#define seq_printf(m, fmt, ...)		do { (void)(m); } while (0)
#define seq_puts(m, s)			do { (void)(m); (void)(s); } while (0)
#define seq_putc(m, c)			do { (void)(m); (void)(c); } while (0)

/* seq_file operations - stubs */
#define seq_open(f, ops)		({ (void)(f); (void)(ops); 0; })
#define seq_release(i, f)		({ (void)(i); (void)(f); 0; })

/* seq_read and seq_lseek - implemented in ext4l/stub.c */
ssize_t seq_read(struct file *f, char *b, size_t s, loff_t *p);
loff_t seq_lseek(struct file *f, loff_t o, int w);

#endif /* _LINUX_SEQ_FILE_H */
