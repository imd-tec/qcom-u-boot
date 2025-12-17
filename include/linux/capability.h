/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This is <linux/capability.h>
 *
 * Andrew G. Morgan <morgan@kernel.org>
 * Alexander Kjeldaas <astor@guardian.no>
 * with help from Aleph1, Roland Buresund and Andrew Main.
 *
 * Stub definitions for Linux kernel capabilities.
 * U-Boot doesn't implement capability checks.
 */
#ifndef _LINUX_CAPABILITY_H
#define _LINUX_CAPABILITY_H

#define CAP_SYS_RESOURCE	24

static inline bool capable(int cap)
{
	return true;
}

static inline bool ns_capable(void *ns, int cap)
{
	return true;
}

#endif /* _LINUX_CAPABILITY_H */
