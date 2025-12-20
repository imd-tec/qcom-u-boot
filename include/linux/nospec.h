/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_NOSPEC_H
#define _LINUX_NOSPEC_H

/*
 * Stub header for U-Boot ext4l.
 *
 * array_index_nospec bounds-checks array access, but in U-Boot's
 * single-user environment this is not necessary.
 */

#define array_index_nospec(index, size)	(index)

#endif /* _LINUX_NOSPEC_H */
