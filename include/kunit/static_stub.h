/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KUnit static stub support for U-Boot
 *
 * Based on Linux include/kunit/static_stub.h
 */
#ifndef _KUNIT_STATIC_STUB_H
#define _KUNIT_STATIC_STUB_H

/*
 * KUNIT_STATIC_STUB_REDIRECT - call a replacement stub if one exists
 *
 * U-Boot doesn't support KUnit, so this is a no-op.
 */
#define KUNIT_STATIC_STUB_REDIRECT(...)	do { } while (0)

#endif /* _KUNIT_STATIC_STUB_H */
