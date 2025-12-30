// SPDX-License-Identifier: GPL-2.0+
/*
 * Internal support functions for ext4l filesystem
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * This provides internal support functions for the ext4l driver,
 * including buffer_head I/O and buffer cache.
 */

#include <blk.h>
#include <part.h>
#include <malloc.h>
#include <u-boot/crc.h>
#include <linux/errno.h>
#include <linux/types.h>

#include "ext4_uboot.h"
#include "ext4.h"

/*
 * Global task_struct for U-Boot.
 * This must be a single global instance shared across all translation units,
 * so that journal_info remains consistent.
 */
struct task_struct ext4l_current_task = { .comm = "u-boot", .pid = 1 };

/*
 * CRC32C support - uses Castagnoli polynomial 0x82F63B78
 * Table is initialised on first mount
 */
static u32 ext4l_crc32c_table[256];
static bool ext4l_crc32c_inited;

void ext4l_crc32c_init(void)
{
	if (!ext4l_crc32c_inited) {
		crc32c_init(ext4l_crc32c_table, 0x82F63B78);
		ext4l_crc32c_inited = true;
	}
}

u32 ext4l_crc32c(u32 crc, const void *address, unsigned int length)
{
	return crc32c_cal(crc, address, length, ext4l_crc32c_table);
}

/*
 * iget_locked - allocate a new inode
 * @sb: super block of filesystem
 * @ino: inode number to allocate
 *
 * U-Boot implementation: allocates ext4_inode_info and returns the embedded
 * vfs_inode. In Linux, this would look up the inode in a hash table first.
 * Since U-Boot is single-threaded and doesn't cache inodes, we always allocate.
 */
struct inode *iget_locked(struct super_block *sb, unsigned long ino)
{
	struct ext4_inode_info *ei;
	struct inode *inode;

	ei = kzalloc(sizeof(struct ext4_inode_info), GFP_KERNEL);
	if (!ei)
		return NULL;

	/* Get pointer to the embedded vfs_inode using offsetof */
	inode = (struct inode *)((char *)ei +
				 offsetof(struct ext4_inode_info, vfs_inode));
	inode->i_sb = sb;
	inode->i_blkbits = sb->s_blocksize_bits;
	inode->i_ino = ino;
	inode->i_state = I_NEW;
	inode->i_count.counter = 1;
	inode->i_mapping = &inode->i_data;
	inode->i_data.host = inode;
	INIT_LIST_HEAD(&ei->i_es_list);

	return inode;
}

/*
 * new_inode - allocate a new empty inode
 * @sb: super block of filesystem
 *
 * U-Boot implementation: allocates ext4_inode_info for a new inode that
 * will be initialised by the caller (e.g., for creating new files).
 */
struct inode *new_inode(struct super_block *sb)
{
	struct ext4_inode_info *ei;
	struct inode *inode;

	ei = kzalloc(sizeof(struct ext4_inode_info), GFP_KERNEL);
	if (!ei)
		return NULL;

	inode = &ei->vfs_inode;
	inode->i_sb = sb;
	inode->i_blkbits = sb->s_blocksize_bits;
	inode->i_nlink = 1;
	inode->i_count.counter = 1;
	inode->i_mapping = &inode->i_data;
	inode->i_data.host = inode;
	INIT_LIST_HEAD(&ei->i_es_list);

	return inode;
}

/*
 * ext4_uboot_bmap - map a logical block to a physical block
 * @inode: inode to map
 * @block: on entry, logical block number; on exit, physical block number
 *
 * U-Boot implementation of bmap for ext4. Maps a logical block number
 * to the corresponding physical block on disk.
 */
int ext4_uboot_bmap(struct inode *inode, sector_t *block)
{
	struct ext4_map_blocks map;
	int ret;

	map.m_lblk = *block;
	map.m_len = 1;
	map.m_flags = 0;

	ret = ext4_map_blocks(NULL, inode, &map, 0);
	if (ret > 0) {
		*block = map.m_pblk;
		return 0;
	}

	return ret < 0 ? ret : -EINVAL;
}

/*
 * bmap - map a logical block to a physical block (VFS interface)
 * @inode: inode to map
 * @blockp: pointer to logical block number; updated to physical block number
 *
 * This is the VFS bmap interface used by jbd2.
 */
int bmap(struct inode *inode, sector_t *blockp)
{
	return ext4_uboot_bmap(inode, blockp);
}

/*
 * Buffer cache implementation
 *
 * Linux's sb_getblk() returns the same buffer_head for the same block number,
 * allowing flags like BH_Verified, BH_Uptodate, etc. to persist across calls.
 * This is critical for ext4's bitmap validation which sets buffer_verified()
 * and expects it to remain set on subsequent lookups.
 */
#define BH_CACHE_BITS	8
#define BH_CACHE_SIZE	(1 << BH_CACHE_BITS)
#define BH_CACHE_MASK	(BH_CACHE_SIZE - 1)

struct bh_cache_entry {
	struct buffer_head *bh;
	struct bh_cache_entry *next;
};

static struct bh_cache_entry *bh_cache[BH_CACHE_SIZE];

static inline unsigned int bh_cache_hash(sector_t block)
{
	return (unsigned int)(block & BH_CACHE_MASK);
}

/**
 * bh_cache_lookup() - Look up a buffer in the cache
 * @block: Block number to look up
 * @size: Expected block size
 * Return: Buffer head if found with matching size, NULL otherwise
 */
static struct buffer_head *bh_cache_lookup(sector_t block, size_t size)
{
	unsigned int hash = bh_cache_hash(block);
	struct bh_cache_entry *entry;

	for (entry = bh_cache[hash]; entry; entry = entry->next) {
		if (entry->bh && entry->bh->b_blocknr == block &&
		    entry->bh->b_size == size) {
			atomic_inc(&entry->bh->b_count);
			return entry->bh;
		}
	}
	return NULL;
}

/**
 * bh_cache_insert() - Insert a buffer into the cache
 * @bh: Buffer head to insert
 */
static void bh_cache_insert(struct buffer_head *bh)
{
	unsigned int hash = bh_cache_hash(bh->b_blocknr);
	struct bh_cache_entry *entry;

	/* Check if already in cache */
	for (entry = bh_cache[hash]; entry; entry = entry->next) {
		if (entry->bh && entry->bh->b_blocknr == bh->b_blocknr)
			return;  /* Already cached */
	}

	entry = malloc(sizeof(struct bh_cache_entry));
	if (!entry)
		return;  /* Silently fail - cache is optional */

	entry->bh = bh;
	entry->next = bh_cache[hash];
	bh_cache[hash] = entry;

	/* Add a reference to keep the buffer alive in cache */
	atomic_inc(&bh->b_count);
}

/**
 * bh_cache_clear() - Clear the entire buffer cache
 *
 * Called on unmount to free all cached buffers.
 */
/**
 * bh_clear_stale_jbd() - Clear stale journal_head from buffer_head
 * @bh: buffer_head to check
 *
 * Check if the buffer still has journal_head attached. This should not happen
 * if the journal was properly destroyed, but warn if it does to help debugging.
 * Clear the JBD flag and b_private to prevent issues with subsequent mounts.
 */
static void bh_clear_stale_jbd(struct buffer_head *bh)
{
	if (buffer_jbd(bh)) {
		log_err("bh %p block %llu still has JBD (b_private %p)\n",
			bh, (unsigned long long)bh->b_blocknr, bh->b_private);
		/*
		 * Clear the JBD flag and b_private to prevent issues.
		 * The journal_head itself will be freed when the
		 * journal_head cache is destroyed.
		 */
		clear_buffer_jbd(bh);
		bh->b_private = NULL;
	}
}

void bh_cache_clear(void)
{
	int i;
	struct bh_cache_entry *entry, *next;

	for (i = 0; i < BH_CACHE_SIZE; i++) {
		for (entry = bh_cache[i]; entry; entry = next) {
			next = entry->next;
			if (entry->bh) {
				struct buffer_head *bh = entry->bh;

				bh_clear_stale_jbd(bh);
				/* Release the cache's reference */
				if (atomic_dec_and_test(&bh->b_count))
					free_buffer_head(bh);
			}
			free(entry);
		}
		bh_cache[i] = NULL;
	}
}

/**
 * alloc_buffer_head() - Allocate a buffer_head structure
 * @gfp_mask: Allocation flags (ignored in U-Boot)
 * Return: Pointer to buffer_head or NULL on error
 */
struct buffer_head *alloc_buffer_head(gfp_t gfp_mask)
{
	struct buffer_head *bh;

	bh = malloc(sizeof(struct buffer_head));
	if (!bh)
		return NULL;

	memset(bh, 0, sizeof(struct buffer_head));

	/* Note: b_data will be allocated when needed by read functions */
	atomic_set(&bh->b_count, 1);

	return bh;
}

/**
 * alloc_buffer_head_with_data() - Allocate a buffer_head with data buffer
 * @size: Size of the data buffer to allocate
 * Return: Pointer to buffer_head or NULL on error
 */
static struct buffer_head *alloc_buffer_head_with_data(size_t size)
{
	struct buffer_head *bh;

	bh = malloc(sizeof(struct buffer_head));
	if (!bh)
		return NULL;

	memset(bh, 0, sizeof(struct buffer_head));

	bh->b_data = malloc(size);
	if (!bh->b_data) {
		free(bh);
		return NULL;
	}

	bh->b_size = size;
	/* Allocate a folio for kmap_local_folio() to work */
	bh->b_folio = malloc(sizeof(struct folio));
	if (bh->b_folio) {
		memset(bh->b_folio, 0, sizeof(struct folio));
		bh->b_folio->data = bh->b_data;
	}
	atomic_set(&bh->b_count, 1);
	/* Mark that this buffer owns its b_data and should free it */
	set_bit(BH_OwnsData, &bh->b_state);

	return bh;
}

/**
 * free_buffer_head() - Free a buffer_head
 * @bh: Buffer head to free
 *
 * Only free b_data if BH_OwnsData is set. Shadow buffers created by
 * jbd2_journal_write_metadata_buffer() share b_data with the original
 * buffer and should not free it.
 */
void free_buffer_head(struct buffer_head *bh)
{
	if (!bh)
		return;

	/* Only free b_data if this buffer owns it */
	if (bh->b_data && test_bit(BH_OwnsData, &bh->b_state))
		free(bh->b_data);
	if (bh->b_folio)
		free(bh->b_folio);
	free(bh);
}

/**
 * ext4l_read_block() - Read a block from the block device
 * @block: Block number (filesystem block, not sector)
 * @size: Block size in bytes
 * @buffer: Destination buffer
 * Return: 0 on success, negative on error
 */
int ext4l_read_block(sector_t block, size_t size, void *buffer)
{
	struct blk_desc *blk_dev;
	struct disk_partition *part;
	lbaint_t sector;
	lbaint_t sector_count;
	unsigned long n;

	blk_dev = ext4l_get_blk_dev();
	part = ext4l_get_partition();
	if (!blk_dev)
		return -EIO;

	/* Convert block to sector */
	sector = (block * size) / blk_dev->blksz + part->start;
	sector_count = size / blk_dev->blksz;

	if (sector_count == 0)
		sector_count = 1;

	n = blk_dread(blk_dev, sector, sector_count, buffer);
	if (n != sector_count)
		return -EIO;

	return 0;
}

/**
 * ext4l_write_block() - Write a block to the block device
 * @block: Block number (filesystem block, not sector)
 * @size: Block size in bytes
 * @buffer: Source buffer
 * Return: 0 on success, negative on error
 */
int ext4l_write_block(sector_t block, size_t size, void *buffer)
{
	struct blk_desc *blk_dev;
	struct disk_partition *part;
	lbaint_t sector;
	lbaint_t sector_count;
	unsigned long n;

	blk_dev = ext4l_get_blk_dev();
	part = ext4l_get_partition();
	if (!blk_dev)
		return -EIO;

	/* Convert block to sector */
	sector = (block * size) / blk_dev->blksz + part->start;
	sector_count = size / blk_dev->blksz;

	if (sector_count == 0)
		sector_count = 1;

	n = blk_dwrite(blk_dev, sector, sector_count, buffer);
	if (n != sector_count)
		return -EIO;

	return 0;
}

/**
 * sb_getblk() - Get a buffer, using cache if available
 * @sb: Super block
 * @block: Block number
 * Return: Buffer head or NULL on error
 */
struct buffer_head *sb_getblk(struct super_block *sb, sector_t block)
{
	struct buffer_head *bh;

	if (!sb)
		return NULL;

	/* Check cache first - must match block number AND size */
	bh = bh_cache_lookup(block, sb->s_blocksize);
	if (bh)
		return bh;

	/* Allocate new buffer */
	bh = alloc_buffer_head_with_data(sb->s_blocksize);
	if (!bh)
		return NULL;

	bh->b_blocknr = block;
	bh->b_bdev = sb->s_bdev;
	bh->b_size = sb->s_blocksize;

	/* Mark buffer as having a valid disk mapping */
	set_buffer_mapped(bh);

	/* Don't read - just allocate with zeroed data */
	memset(bh->b_data, '\0', bh->b_size);

	/* Add to cache */
	bh_cache_insert(bh);

	return bh;
}

/**
 * sb_bread() - Read a block via super_block
 * @sb: Super block
 * @block: Block number to read
 * Return: Buffer head or NULL on error
 */
struct buffer_head *sb_bread(struct super_block *sb, sector_t block)
{
	struct buffer_head *bh;
	int ret;

	if (!sb)
		return NULL;

	bh = sb_getblk(sb, block);
	if (!bh)
		return NULL;

	/* If buffer is already up-to-date, return it without re-reading */
	if (buffer_uptodate(bh))
		return bh;

	bh->b_blocknr = block;
	bh->b_bdev = sb->s_bdev;
	bh->b_size = sb->s_blocksize;

	ret = ext4l_read_block(block, sb->s_blocksize, bh->b_data);
	if (ret) {
		brelse(bh);
		return NULL;
	}

	/* Mark buffer as up-to-date */
	set_buffer_uptodate(bh);

	return bh;
}

/**
 * brelse() - Release a buffer_head
 * @bh: Buffer head to release
 */
void brelse(struct buffer_head *bh)
{
	if (!bh)
		return;

	if (atomic_dec_and_test(&bh->b_count))
		free_buffer_head(bh);
}

/**
 * __brelse() - Release a buffer_head (alternate API)
 * @bh: Buffer head to release
 */
void __brelse(struct buffer_head *bh)
{
	brelse(bh);
}

/**
 * bdev_getblk() - Get buffer via block_device
 * @bdev: Block device
 * @block: Block number
 * @size: Block size
 * @gfp: Allocation flags
 * Return: Buffer head or NULL
 */
struct buffer_head *bdev_getblk(struct block_device *bdev, sector_t block,
				unsigned size, gfp_t gfp)
{
	struct buffer_head *bh;

	/* Check cache first - must match block number AND size */
	bh = bh_cache_lookup(block, size);
	if (bh)
		return bh;

	bh = alloc_buffer_head_with_data(size);
	if (!bh)
		return NULL;

	bh->b_blocknr = block;
	bh->b_bdev = bdev;
	bh->b_size = size;

	/* Mark buffer as having a valid disk mapping */
	set_buffer_mapped(bh);

	/* Don't read - just allocate with zeroed data */
	memset(bh->b_data, 0, bh->b_size);

	/* Add to cache */
	bh_cache_insert(bh);

	return bh;
}

/**
 * __bread() - Read a block via block_device
 * @bdev: Block device
 * @block: Block number to read
 * @size: Block size
 * Return: Buffer head or NULL on error
 */
struct buffer_head *__bread(struct block_device *bdev, sector_t block,
			    unsigned size)
{
	struct buffer_head *bh;
	int ret;

	bh = alloc_buffer_head_with_data(size);
	if (!bh)
		return NULL;

	bh->b_blocknr = block;
	bh->b_bdev = bdev;
	bh->b_size = size;

	ret = ext4l_read_block(block, size, bh->b_data);
	if (ret) {
		free_buffer_head(bh);
		return NULL;
	}

	/* Mark buffer as up-to-date */
	set_bit(BH_Uptodate, &bh->b_state);

	return bh;
}

/**
 * submit_bh() - Submit a buffer_head for I/O
 * @op: Operation (REQ_OP_READ, REQ_OP_WRITE, etc.)
 * @bh: Buffer head to submit
 * Return: 0 on success, negative on error
 */
int submit_bh(int op, struct buffer_head *bh)
{
	int ret;
	int op_type = op & REQ_OP_MASK;  /* Mask out flags, keep operation type */

	if (op_type == REQ_OP_READ) {
		ret = ext4l_read_block(bh->b_blocknr, bh->b_size, bh->b_data);
		if (ret) {
			clear_buffer_uptodate(bh);
			return ret;
		}
		set_buffer_uptodate(bh);
	} else if (op_type == REQ_OP_WRITE) {
		ret = ext4l_write_block(bh->b_blocknr, bh->b_size, bh->b_data);
		if (ret) {
			clear_buffer_uptodate(bh);
			return ret;
		}
		/* Mark buffer as clean (not dirty) after write */
	}

	return 0;
}

/**
 * bh_read() - Read a buffer_head from disk
 * @bh: Buffer head to read
 * @flags: Read flags
 * Return: 0 on success, negative on error
 */
int bh_read(struct buffer_head *bh, int flags)
{
	if (!bh || !bh->b_data)
		return -EINVAL;

	submit_bh(REQ_OP_READ | flags, bh);
	return buffer_uptodate(bh) ? 0 : -EIO;
}

/**
 * __filemap_get_folio() - Get or create a folio for a mapping
 * @mapping: The address_space to search
 * @index: The page index
 * @fgp_flags: Flags (FGP_CREAT to create if not found)
 * @gfp: Memory allocation flags
 * Return: Folio pointer or ERR_PTR on error
 */
struct folio *__filemap_get_folio(struct address_space *mapping,
				  pgoff_t index, unsigned int fgp_flags,
				  gfp_t gfp)
{
	struct folio *folio;
	int i;

	/* Search for existing folio in cache */
	if (mapping) {
		for (i = 0; i < mapping->folio_cache_count; i++) {
			folio = mapping->folio_cache[i];
			if (folio && folio->index == index) {
				/* Found existing folio, bump refcount */
				folio->_refcount++;
				return folio;
			}
		}
	}

	/* If not creating, return error */
	if (!(fgp_flags & FGP_CREAT))
		return ERR_PTR(-ENOENT);

	/* Create new folio */
	folio = kzalloc(sizeof(struct folio), gfp);
	if (!folio)
		return ERR_PTR(-ENOMEM);

	folio->data = kzalloc(PAGE_SIZE, gfp);
	if (!folio->data) {
		kfree(folio);
		return ERR_PTR(-ENOMEM);
	}

	folio->index = index;
	folio->mapping = mapping;
	folio->_refcount = 1;

	/* Add to cache if there's room */
	if (mapping && mapping->folio_cache_count < FOLIO_CACHE_MAX) {
		mapping->folio_cache[mapping->folio_cache_count++] = folio;
		/* Extra ref for cache */
		folio->_refcount++;
	}

	return folio;
}

/**
 * folio_put() - Release a reference to a folio
 * @folio: The folio to release
 */
void folio_put(struct folio *folio)
{
	if (!folio)
		return;
	if (--folio->_refcount > 0)
		return;
	kfree(folio->data);
	kfree(folio);
}

/**
 * folio_get() - Acquire a reference to a folio
 * @folio: The folio to reference
 */
void folio_get(struct folio *folio)
{
	if (folio)
		folio->_refcount++;
}
