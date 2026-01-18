/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_UTSNAME_H
#define _LINUX_UTSNAME_H

/* Stub for linux/utsname.h */
struct new_utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char domainname[65];
};

struct uts_namespace {
	struct new_utsname name;
};

extern struct uts_namespace init_uts_ns;

/**
 * init_utsname() - get initial UTS name structure
 *
 * Return: pointer to static utsname structure
 */
static inline struct new_utsname *init_utsname(void)
{
	static struct new_utsname uts = { .nodename = "u-boot" };

	return &uts;
}

#endif /* _LINUX_UTSNAME_H */
