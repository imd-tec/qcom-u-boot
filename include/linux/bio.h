/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Block I/O structures stub for U-Boot
 *
 * U-Boot doesn't have a real block I/O layer, so these are stubs.
 */
#ifndef __LINUX_BIO_H
#define __LINUX_BIO_H

#include <linux/types.h>
#include <malloc.h>

/* Forward declarations */
struct block_device;
struct page;
struct folio;

/**
 * struct bio_vec - segment in a bio
 * @bv_page: page containing the data
 * @bv_len: length of the segment
 * @bv_offset: offset within the page
 */
struct bio_vec {
	struct page *bv_page;
	unsigned int bv_len;
	unsigned int bv_offset;
};

/**
 * struct bvec_iter - iterator for bio_vec
 * @bi_sector: current sector
 * @bi_size: remaining size
 * @bi_idx: current index into bio_vec array
 * @bi_bvec_done: bytes completed in current bvec
 */
struct bvec_iter {
	sector_t bi_sector;
	unsigned int bi_size;
	unsigned int bi_idx;
	unsigned int bi_bvec_done;
};

/**
 * struct bio - block I/O structure
 * @bi_next: next bio in chain
 * @bi_bdev: target block device
 * @bi_opf: operation and flags
 * @bi_flags: bio flags
 * @bi_ioprio: I/O priority
 * @bi_write_hint: write lifetime hint
 * @bi_status: completion status
 * @bi_iter: current position iterator
 * @__bi_remaining: remaining count for chained bios
 * @bi_private: private data for completion
 * @bi_end_io: completion callback
 *
 * U-Boot stub.
 */
struct bio {
	struct bio *bi_next;
	struct block_device *bi_bdev;
	unsigned long bi_opf;
	unsigned short bi_flags;
	unsigned short bi_ioprio;
	unsigned short bi_write_hint;
	int bi_status;
	struct bvec_iter bi_iter;
	atomic_t __bi_remaining;
	void *bi_private;
	void (*bi_end_io)(struct bio *);
};

/**
 * bio_sectors() - return number of sectors in bio
 * @bio: bio to query
 *
 * Return: number of 512-byte sectors
 */
static inline unsigned int bio_sectors(struct bio *bio)
{
	return bio->bi_iter.bi_size >> 9;
}

/**
 * struct folio_iter - iterator for folio iteration over bio
 * @i: current index
 * @folio: current folio
 * @offset: offset within folio
 * @length: length of current segment
 */
struct folio_iter {
	int i;
	struct folio *folio;
	size_t offset;
	size_t length;
};

/* Maximum number of bio_vecs */
#define BIO_MAX_VECS		256

/* bio operations - stubs */
#define bio_for_each_folio_all(fi, bio) \
	for ((fi).i = 0; (fi).i < 0; (fi).i++)
#define bio_put(bio)			free(bio)
#define bio_alloc(bdev, vecs, op, gfp)	((struct bio *)calloc(1, sizeof(struct bio)))
#define submit_bio(bio)			do { } while (0)
#define bio_add_folio(bio, folio, len, off) \
	({ (void)(bio); (void)(folio); (void)(len); (void)(off); 1; })

/* blk_status_to_errno - convert block status to errno */
#define blk_status_to_errno(status)	(-(status))

/* mapping_set_error - record error in address_space */
#define mapping_set_error(m, e)		do { (void)(m); (void)(e); } while (0)

#endif /* __LINUX_BIO_H */
