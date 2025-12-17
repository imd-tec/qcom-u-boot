/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Wrapper functions for accessing the file_struct fd array.
 */
#ifndef _LINUX_FILE_H
#define _LINUX_FILE_H

/*
 * Stub definitions for Linux kernel file handling.
 */

struct file;
struct fd {
	struct file *file;
	unsigned int flags;
};

#define EMPTY_FD ((struct fd){ NULL, 0 })

static inline struct fd fdget(unsigned int fd)
{
	return EMPTY_FD;
}

static inline void fdput(struct fd fd)
{
}

#endif /* _LINUX_FILE_H */
