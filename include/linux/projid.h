/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Project ID definitions for U-Boot
 *
 * Based on Linux projid.h - filesystem project IDs for quotas.
 */
#ifndef _LINUX_PROJID_H
#define _LINUX_PROJID_H

/**
 * typedef kprojid_t - kernel project ID
 *
 * Wrapper type for project IDs used in filesystem quotas.
 */
typedef struct { unsigned int val; } kprojid_t;

/**
 * typedef projid_t - user-space project ID
 */
typedef unsigned int projid_t;

/**
 * make_kprojid() - create a kernel project ID
 * @ns: user namespace (ignored in U-Boot)
 * @id: project ID value
 */
#define make_kprojid(ns, id)	((kprojid_t){ .val = (id) })

/**
 * from_kprojid() - extract project ID value
 * @ns: user namespace (ignored in U-Boot)
 * @kprojid: kernel project ID
 */
#define from_kprojid(ns, kprojid)	((kprojid).val)

/**
 * projid_eq() - compare two project IDs
 * @a: first project ID
 * @b: second project ID
 */
#define projid_eq(a, b)		((a).val == (b).val)

#endif /* _LINUX_PROJID_H */
