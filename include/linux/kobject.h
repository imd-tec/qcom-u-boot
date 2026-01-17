/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Kobject stubs for U-Boot
 *
 * U-Boot doesn't have sysfs or the full kobject infrastructure,
 * so these are minimal stubs.
 */
#ifndef _LINUX_KOBJECT_H
#define _LINUX_KOBJECT_H

#include <linux/types.h>

/**
 * struct kobject - kernel object
 * @name: name of the object
 *
 * U-Boot stub - minimal structure for filesystem code.
 */
struct kobject {
	const char *name;
};

/**
 * kobject_put() - decrement refcount on kobject
 * @kobj: object to release
 *
 * U-Boot stub - declared here, implemented in stub.c.
 */
void kobject_put(struct kobject *kobj);

/* sysfs stubs */
#define super_set_sysfs_name_bdev(sb)	do { } while (0)

#endif /* _LINUX_KOBJECT_H */
