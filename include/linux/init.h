/* SPDX-License-Identifier: GPL-2.0 */
/*
 * These macros are used to mark some functions or initialized data
 * as 'initialization' functions. The kernel can take this as hint
 * that the function is used only during the initialization phase
 * and free up used memory resources after.
 *
 * Stub definitions for Linux kernel initialization macros.
 * U-Boot has its own initialization mechanism.
 */
#ifndef _LINUX_INIT_H
#define _LINUX_INIT_H

/* Section markers - these are no-ops in U-Boot */
#define __init
#define __exit
#define __initdata
#define __exitdata
#define __initconst
#define __exitconst
#define __devinit
#define __devexit
#define __devinitdata
#define __devexitdata
#define __devinitconst
#define __devexitconst

/* Initcall levels - no-ops in U-Boot */
#define pure_initcall(fn)
#define core_initcall(fn)
#define core_initcall_sync(fn)
#define postcore_initcall(fn)
#define postcore_initcall_sync(fn)
#define arch_initcall(fn)
#define arch_initcall_sync(fn)
#define subsys_initcall(fn)
#define subsys_initcall_sync(fn)
#define fs_initcall(fn)
#define fs_initcall_sync(fn)
#define rootfs_initcall(fn)
#define device_initcall(fn)
#define device_initcall_sync(fn)
#define late_initcall(fn)
#define late_initcall_sync(fn)

#define __initcall(fn)
#define __exitcall(fn)

#endif /* _LINUX_INIT_H */
