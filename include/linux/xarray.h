/* SPDX-License-Identifier: GPL-2.0 */
/*
 * XArray stubs for U-Boot
 *
 * U-Boot doesn't have the XArray data structure, so these are stubs.
 */
#ifndef _LINUX_XARRAY_H
#define _LINUX_XARRAY_H

#include <linux/types.h>

/**
 * typedef xa_mark_t - XArray mark type
 *
 * Used to tag entries in an XArray.
 */
typedef unsigned int xa_mark_t;

/**
 * struct xarray - XArray data structure
 *
 * U-Boot stub - the XArray is not used.
 */
struct xarray {
	int dummy;
};

/* XArray initialisation/destruction stubs */
#define xa_init(xa)		do { } while (0)
#define xa_destroy(xa)		do { } while (0)

/* XArray lookup stubs - always return NULL */
#define xa_load(xa, index)	((void *)NULL)

/* XArray modification stubs */
#define xa_erase(xa, index)	do { (void)(xa); (void)(index); } while (0)
#define xa_insert(xa, index, entry, gfp) \
	({ (void)(xa); (void)(index); (void)(entry); (void)(gfp); 0; })

/* XArray query stubs - always empty */
#define xa_empty(xa)		({ (void)(xa); 1; })

/* XArray iteration stubs - iterate zero times */
#define xa_for_each(xa, index, entry) \
	for ((index) = 0, (entry) = NULL; 0; )

#define xa_for_each_range(xa, index, entry, start, end) \
	for ((index) = (start), (entry) = NULL; 0; )

#endif /* _LINUX_XARRAY_H */
