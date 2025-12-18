/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _KUNIT_STATIC_STUB_H
#define _KUNIT_STATIC_STUB_H

/*
 * Stub header for U-Boot ext4l.
 *
 * KUnit static stubs are for kernel unit testing - not needed in U-Boot.
 */

#define KUNIT_STATIC_STUB_REDIRECT(func, args...)	do { } while (0)

#endif /* _KUNIT_STATIC_STUB_H */
