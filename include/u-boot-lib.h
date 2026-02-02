/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * U-Boot library interface
 *
 * This provides basic access to setup of the U-Boot library.
 *
 * Library functions must be individually accessed via their respective headers.
 *
 * Copyright 2025 Canonical Ltd.
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#ifndef __U_BOOT_LIB_H
#define __U_BOOT_LIB_H

struct global_data;

/**
 * ulib_init() - set up the U-Boot library
 *
 * @progname: Program name to use (must be a writeable string, typically argv[0])
 * @data: Global data (must remain valid until the program exits)
 * Return: 0 if OK, -ve error code on error
 */
int ulib_init(char *progname);

/**
 * ulib_uninit() - shut down the U-Boot library
 *
 * Call this when your program has finished using the library, before it exits
 */
void ulib_uninit(void);

/**
 * ulib_get_version() - Get the version string
 *
 * Return: Full U-Boot version string
 */
const char *ulib_get_version(void);

/**
 * ulib_putsn() - Write a string with specified length
 *
 * This outputs exactly @len characters from @s, regardless of any nul
 * characters that may be present. This is useful for printing substrings
 * or binary data with embedded nuls.
 *
 * If CONFIG_CONSOLE_PUTSN is enabled, this calls putsn() directly.
 * Otherwise, it outputs characters one at a time using putc().
 *
 * @s: String to output (need not be nul-terminated)
 * @len: Number of characters to output
 */
void ulib_putsn(const char *s, int len);

#endif
