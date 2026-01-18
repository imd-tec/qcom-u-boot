/* SPDX-License-Identifier: GPL-2.0 */
/*
 * fs-verity definitions for U-Boot
 *
 * Based on Linux fsverity.h - fs-verity verification.
 * U-Boot stub - fs-verity not supported.
 */
#ifndef _LINUX_FSVERITY_H
#define _LINUX_FSVERITY_H

/* Forward declarations */
struct bio;
struct dentry;
struct file;
struct folio;
struct iattr;
struct inode;
struct work_struct;

/**
 * IS_VERITY() - check if inode has fs-verity enabled
 * @inode: inode to check
 *
 * U-Boot stub - always returns false.
 */
#define IS_VERITY(inode)		(0)

/**
 * fsverity_file_open() - check verity on file open
 * @inode: inode being opened
 * @file: file being opened
 *
 * U-Boot stub - always succeeds.
 *
 * Return: 0
 */
#define fsverity_file_open(inode, file) \
	({ (void)(inode); (void)(file); 0; })

/**
 * fsverity_prepare_setattr() - prepare for attribute change
 * @dentry: dentry being modified
 * @attr: new attributes
 *
 * U-Boot stub - always succeeds.
 *
 * Return: 0
 */
#define fsverity_prepare_setattr(dentry, attr) \
	({ (void)(dentry); (void)(attr); 0; })

/**
 * fsverity_active() - check if verity is active on inode
 * @inode: inode to check
 *
 * U-Boot stub - always returns false.
 *
 * Return: false
 */
#define fsverity_active(inode)		({ (void)(inode); 0; })

/**
 * fsverity_cleanup_inode() - cleanup verity data on inode
 * @inode: inode to clean up
 *
 * U-Boot stub - no-op.
 */
#define fsverity_cleanup_inode(inode)	do { (void)(inode); } while (0)

/**
 * fsverity_verify_bio() - verify bio data
 * @bio: bio to verify
 *
 * U-Boot stub - no-op.
 */
#define fsverity_verify_bio(bio)	do { (void)(bio); } while (0)

/**
 * fsverity_enqueue_verify_work() - enqueue verification work
 * @work: work item
 *
 * U-Boot stub - no-op.
 */
#define fsverity_enqueue_verify_work(work) \
	do { (void)(work); } while (0)

/**
 * fsverity_verify_folio() - verify folio data
 * @folio: folio to verify
 *
 * U-Boot stub - always succeeds.
 *
 * Return: true (verified)
 */
#define fsverity_verify_folio(folio)	({ (void)(folio); true; })

#endif /* _LINUX_FSVERITY_H */
