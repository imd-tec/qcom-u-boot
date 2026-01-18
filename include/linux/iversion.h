/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Inode version definitions for U-Boot
 *
 * Based on Linux iversion.h - inode version management.
 * U-Boot stub - version tracking not supported.
 */
#ifndef _LINUX_IVERSION_H
#define _LINUX_IVERSION_H

#include <linux/types.h>

/* Forward declarations */
struct inode;

/**
 * inode_peek_iversion_raw() - read inode version without side effects
 * @inode: inode to read
 *
 * U-Boot stub - always returns 0.
 *
 * Return: inode version
 */
#define inode_peek_iversion_raw(inode)		(0ULL)

/**
 * inode_peek_iversion() - read inode version
 * @inode: inode to read
 *
 * U-Boot stub - always returns 0.
 *
 * Return: inode version
 */
#define inode_peek_iversion(inode)		(0ULL)

/**
 * inode_set_iversion_raw() - set inode version directly
 * @inode: inode to modify
 * @version: version to set
 *
 * U-Boot stub - no-op.
 */
#define inode_set_iversion_raw(inode, version) \
	do { (void)(inode); (void)(version); } while (0)

/**
 * inode_set_iversion() - set inode version
 * @inode: inode to modify
 * @version: version to set
 *
 * U-Boot stub - no-op.
 */
#define inode_set_iversion(inode, version) \
	do { (void)(inode); (void)(version); } while (0)

/**
 * inode_set_iversion_queried() - set inode version as queried
 * @inode: inode to modify
 * @version: version to set
 *
 * U-Boot stub - no-op.
 */
#define inode_set_iversion_queried(inode, version) \
	do { (void)(inode); (void)(version); } while (0)

/**
 * inode_inc_iversion() - increment inode version
 * @inode: inode to modify
 *
 * U-Boot stub - no-op.
 */
#define inode_inc_iversion(inode)		do { (void)(inode); } while (0)

/**
 * inode_eq_iversion() - check if inode version matches
 * @inode: inode to check
 * @version: version to compare
 *
 * U-Boot stub - always returns true.
 *
 * Return: true if versions match
 */
#define inode_eq_iversion(inode, version) \
	({ (void)(inode); (void)(version); true; })

/**
 * inode_query_iversion() - query inode version
 * @inode: inode to query
 *
 * U-Boot stub - always returns 0.
 *
 * Return: inode version
 */
#define inode_query_iversion(inode)		({ (void)(inode); 0ULL; })

#endif /* _LINUX_IVERSION_H */
