/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/buffer_head.h
 *
 * Everything to do with buffer_heads.
 * Minimal version for U-Boot ext4l - based on Linux 6.18
 */

#ifndef _LINUX_BUFFER_HEAD_H
#define _LINUX_BUFFER_HEAD_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>
/*
 * Note: atomic_t and sector_t are expected to be defined by the including
 * file (ext4_uboot.h) before including this header.
 */

enum bh_state_bits {
	BH_Uptodate,	/* Contains valid data */
	BH_Dirty,	/* Is dirty */
	BH_Lock,	/* Is locked */
	BH_Req,		/* Has been submitted for I/O */

	BH_Mapped,	/* Has a disk mapping */
	BH_New,		/* Disk mapping was newly created by get_block */
	BH_Async_Read,	/* Is under end_buffer_async_read I/O */
	BH_Async_Write,	/* Is under end_buffer_async_write I/O */
	BH_Delay,	/* Buffer is not yet allocated on disk */
	BH_Boundary,	/* Block is followed by a discontiguity */
	BH_Write_EIO,	/* I/O error on write */
	BH_Unwritten,	/* Buffer is allocated on disk but not written */
	BH_Quiet,	/* Buffer Error Prinks to be quiet */
	BH_Meta,	/* Buffer contains metadata */
	BH_Prio,	/* Buffer should be submitted with REQ_PRIO */
	BH_Defer_Completion, /* Defer AIO completion to workqueue */
	BH_Migrate,     /* Buffer is being migrated (norefs) */

	BH_PrivateStart,/* not a state bit, but the first bit available
			 * for private allocation by other entities
			 */
};

#define MAX_BUF_PER_PAGE (PAGE_SIZE / 512)

struct page;
struct folio;
struct buffer_head;
struct address_space;
struct block_device;
typedef void (bh_end_io_t)(struct buffer_head *bh, int uptodate);

/*
 * Historically, a buffer_head was used to map a single block
 * within a page, and of course as the unit of I/O through the
 * filesystem and block layers.  Nowadays the basic I/O unit
 * is the bio, and buffer_heads are used for extracting block
 * mappings (via a get_block_t call), for tracking state within
 * a folio (via a folio_mapping) and for wrapping bio submission
 * for backward compatibility reasons (e.g. submit_bh).
 */
struct buffer_head {
	unsigned long b_state;		/* buffer state bitmap (see above) */
	struct buffer_head *b_this_page;/* circular list of page's buffers */
	union {
		struct page *b_page;	/* the page this bh is mapped to */
		struct folio *b_folio;	/* the folio this bh is mapped to */
	};

	sector_t b_blocknr;		/* start block number */
	size_t b_size;			/* size of mapping */
	char *b_data;			/* pointer to data within the page */

	struct block_device *b_bdev;
	bh_end_io_t *b_end_io;		/* I/O completion */
	void *b_private;		/* reserved for b_end_io */
	struct list_head b_assoc_buffers; /* associated with another mapping */
	struct address_space *b_assoc_map;	/* mapping this buffer is
						   associated with */
	atomic_t b_count;		/* users using this buffer_head */
	spinlock_t b_uptodate_lock;	/* Used by the first bh in a page, to
					 * serialise IO completion of other
					 * buffers in the page */
};

/*
 * Buffer head bit operations - inline implementations.
 * These don't use the extern set_bit/clear_bit from asm/bitops.h
 * since sandbox doesn't implement them.
 */
static inline void bh_set_bit(int nr, unsigned long *addr)
{
	*addr |= (1UL << nr);
}

static inline void bh_clear_bit(int nr, unsigned long *addr)
{
	*addr &= ~(1UL << nr);
}

static inline int bh_test_bit(int nr, const unsigned long *addr)
{
	return (*addr >> nr) & 1;
}

static inline int bh_test_and_set_bit(int nr, unsigned long *addr)
{
	int old = (*addr >> nr) & 1;
	*addr |= (1UL << nr);
	return old;
}

static inline int bh_test_and_clear_bit(int nr, unsigned long *addr)
{
	int old = (*addr >> nr) & 1;
	*addr &= ~(1UL << nr);
	return old;
}

/*
 * macro tricks to expand the set_buffer_foo(), clear_buffer_foo()
 * and buffer_foo() functions.
 */
#define BUFFER_FNS(bit, name)						\
static __always_inline void set_buffer_##name(struct buffer_head *bh)	\
{									\
	if (!bh_test_bit(BH_##bit, &(bh)->b_state))			\
		bh_set_bit(BH_##bit, &(bh)->b_state);			\
}									\
static __always_inline void clear_buffer_##name(struct buffer_head *bh)	\
{									\
	bh_clear_bit(BH_##bit, &(bh)->b_state);				\
}									\
static __always_inline int buffer_##name(const struct buffer_head *bh)	\
{									\
	return bh_test_bit(BH_##bit, &(bh)->b_state);			\
}

/*
 * test_set_buffer_foo() and test_clear_buffer_foo()
 */
#define TAS_BUFFER_FNS(bit, name)					\
static __always_inline int test_set_buffer_##name(struct buffer_head *bh) \
{									\
	return bh_test_and_set_bit(BH_##bit, &(bh)->b_state);		\
}									\
static __always_inline int test_clear_buffer_##name(struct buffer_head *bh) \
{									\
	return bh_test_and_clear_bit(BH_##bit, &(bh)->b_state);		\
}

BUFFER_FNS(Uptodate, uptodate)
BUFFER_FNS(Dirty, dirty)
BUFFER_FNS(Lock, locked)
BUFFER_FNS(Req, req)
BUFFER_FNS(Mapped, mapped)
BUFFER_FNS(New, new)
BUFFER_FNS(Async_Read, async_read)
BUFFER_FNS(Async_Write, async_write)
BUFFER_FNS(Delay, delay)
BUFFER_FNS(Boundary, boundary)
BUFFER_FNS(Write_EIO, write_io_error)
BUFFER_FNS(Unwritten, unwritten)
BUFFER_FNS(Meta, meta)
BUFFER_FNS(Prio, prio)
BUFFER_FNS(Defer_Completion, defer_completion)

static inline void get_bh(struct buffer_head *bh)
{
	atomic_inc(&bh->b_count);
}

static inline void put_bh(struct buffer_head *bh)
{
	atomic_dec(&bh->b_count);
}

/* Stubs for U-Boot */
#define brelse(bh)		do { if (bh) put_bh(bh); } while (0)
#define __brelse(bh)		do { put_bh(bh); } while (0)

#endif /* _LINUX_BUFFER_HEAD_H */
