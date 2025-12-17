/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Dynamic loading of modules into the kernel.
 *
 * Rewritten by Richard Henderson <rth@tamu.edu> Dec 1996
 * Rewritten again by Rusty Russell, 2002
 *
 * Stub definitions for Linux kernel module support.
 * U-Boot doesn't use loadable modules.
 */
#ifndef _LINUX_MODULE_H
#define _LINUX_MODULE_H

struct module;

#define THIS_MODULE		0
#define try_module_get(...)	1
#define module_put(...)		do { } while (0)
#define __module_get(...)	do { } while (0)

#define module_init(fn)
#define module_exit(fn)

#define module_param(name, type, perm)
#define module_param_call(name, set, get, arg, perm)
#define module_param_named(name, var, type, perm)

#define MODULE_PARM_DESC(name, desc)
#define MODULE_VERSION(ver)
#define MODULE_DESCRIPTION(desc)
#define MODULE_AUTHOR(author)
#define MODULE_LICENSE(license)
#define MODULE_ALIAS(alias)
#define MODULE_SOFTDEP(dep)
#define MODULE_INFO(tag, info)

#endif /* _LINUX_MODULE_H */
