/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * fat_internal.h
 *
 * Internal definitions and structures for FAT filesystem implementation
 */

#ifndef _FAT_INTERNAL_H_
#define _FAT_INTERNAL_H_

#include <fat.h>
#include <linux/compiler.h>

struct blk_desc;
struct disk_partition;

/* Maximum number of clusters for FAT12 */
#define MAX_FAT12	0xFF4

/* Boot sector offsets */
#define DOS_BOOT_MAGIC_OFFSET	0x1fe
#define DOS_FS_TYPE_OFFSET	0x36
#define DOS_FS32_TYPE_OFFSET	0x52

/**
 * struct fat_itr - directory iterator, to simplify filesystem traversal
 *
 * Implements an iterator pattern to traverse directory tables,
 * transparently handling directory tables split across multiple
 * clusters, and the difference between FAT12/FAT16 root directory
 * (contiguous) and subdirectories + FAT32 root (chained).
 *
 * Rough usage
 *
 * .. code-block:: c
 *
 *     for (fat_itr_root(&itr, fsdata); fat_itr_next(&itr); ) {
 *         // to traverse down to a subdirectory pointed to by
 *         // current iterator position:
 *         fat_itr_child(&itr, &itr);
 *     }
 *
 * For a more complete example, see fat_itr_resolve().
 */
struct fat_itr {
	/**
	 * @fsdata:		filesystem parameters
	 */
	fsdata *fsdata;
	/**
	 * @start_clust:	first cluster
	 */
	unsigned int start_clust;
	/**
	 * @clust:		current cluster
	 */
	unsigned int clust;
	/**
	 * @next_clust:		next cluster if remaining == 0
	 */
	unsigned int next_clust;
	/**
	 * @last_cluster:	set if last cluster of directory reached
	 */
	int last_cluster;
	/**
	 * @is_root:		is iterator at root directory
	 */
	int is_root;
	/**
	 * @remaining:		remaining directory entries in current cluster
	 */
	int remaining;
	/**
	 * @dent:		current directory entry
	 */
	dir_entry *dent;
	/**
	 * @dent_rem:		remaining entries after long name start
	 */
	int dent_rem;
	/**
	 * @dent_clust:		cluster of long name start
	 */
	unsigned int dent_clust;
	/**
	 * @dent_start:		first directory entry for long name
	 */
	dir_entry *dent_start;
	/**
	 * @l_name:		long name of current directory entry
	 */
	char l_name[VFAT_MAXLEN_BYTES];
	/**
	 * @s_name:		short 8.3 name of current directory entry
	 */
	char s_name[14];
	/**
	 * @name:		l_name if there is one, else s_name
	 */
	char *name;
	/**
	 * @block:		buffer for current cluster
	 */
	u8 block[MAX_CLUSTSIZE] __aligned(ARCH_DMA_MINALIGN);
};

#define TYPE_FILE 0x1
#define TYPE_DIR  0x2
#define TYPE_ANY  (TYPE_FILE | TYPE_DIR)

#endif /* _FAT_INTERNAL_H_ */
