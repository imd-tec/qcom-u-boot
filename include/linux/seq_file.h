/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_SEQ_FILE_H
#define _LINUX_SEQ_FILE_H

/*
 * Stub definitions for seq_file interface.
 * U-Boot doesn't use /proc filesystem.
 */

struct seq_file {
	void *private;
};

#define seq_printf(m, fmt, ...)		do { } while (0)
#define seq_puts(m, s)			do { } while (0)
#define seq_putc(m, c)			do { } while (0)

#endif /* _LINUX_SEQ_FILE_H */
