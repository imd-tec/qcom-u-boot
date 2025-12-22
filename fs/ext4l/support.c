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
#include <linux/errno.h>
#include <linux/types.h>

#include "ext4_uboot.h"
#include "ext4.h"

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
void bh_cache_clear(void)
{
	int i;
	struct bh_cache_entry *entry, *next;

	for (i = 0; i < BH_CACHE_SIZE; i++) {
		for (entry = bh_cache[i]; entry; entry = next) {
			next = entry->next;
			if (entry->bh) {
				/* Release the cache's reference */
				if (atomic_dec_and_test(&entry->bh->b_count))
					free_buffer_head(entry->bh);
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
 */
void submit_bh(int op, struct buffer_head *bh)
{
	int ret;
	int op_type = op & 0xff;  /* Mask out flags, keep operation type */

	if (op_type == REQ_OP_READ) {
		ret = ext4l_read_block(bh->b_blocknr, bh->b_size, bh->b_data);
		if (ret) {
			clear_buffer_uptodate(bh);
			return;
		}
		set_buffer_uptodate(bh);
	} else if (op_type == REQ_OP_WRITE) {
		/* Write support not implemented yet */
		clear_buffer_uptodate(bh);
	}
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
