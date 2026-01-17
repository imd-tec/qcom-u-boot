/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Page cache and folio management stubs for U-Boot
 *
 * U-Boot doesn't have a real page cache, so these are stubs.
 */
#ifndef _LINUX_PAGEMAP_H
#define _LINUX_PAGEMAP_H

#include <linux/types.h>
#include <linux/mm_types.h>

/* Forward declarations */
struct address_space;
struct buffer_head;

/**
 * struct folio - memory page container stub
 * @page: associated page
 * @index: page index in the mapping
 * @mapping: address space this folio belongs to
 * @flags: folio flags
 * @data: pointer to the actual data
 * @private: private data for filesystem
 * @_refcount: reference count
 *
 * U-Boot stub - simplified folio structure.
 */
struct folio {
	struct page *page;
	unsigned long index;
	struct address_space *mapping;
	unsigned long flags;
	void *data;
	struct buffer_head *private;
	int _refcount;
};

/* FGP flags for __filemap_get_folio() */
#define FGP_ACCESSED	0x00000001
#define FGP_LOCK	0x00000002
#define FGP_CREAT	0x00000004
#define FGP_WRITE	0x00000008
#define FGP_NOFS	0x00000010
#define FGP_NOWAIT	0x00000020
#define FGP_FOR_MMAP	0x00000040
#define FGP_STABLE	0x00000080
#define FGP_WRITEBEGIN	(FGP_LOCK | FGP_WRITE | FGP_CREAT | FGP_STABLE)

/* Page cache tags */
#define PAGECACHE_TAG_DIRTY	0
#define PAGECACHE_TAG_TOWRITE	1
#define PAGECACHE_TAG_WRITEBACK	2

/* Folio operations - stubs */
#define folio_mark_dirty(f)		do { (void)(f); } while (0)
#define folio_test_uptodate(f)		({ (void)(f); 1; })
#define folio_pos(f)			({ (void)(f); 0LL; })
#define folio_size(f)			({ (void)(f); PAGE_SIZE; })
#define folio_unlock(f)			do { (void)(f); } while (0)
#define folio_lock(f)			do { (void)(f); } while (0)
#define folio_buffers(f)		({ (void)(f); (struct buffer_head *)NULL; })
#define virt_to_folio(p)		({ (void)(p); (struct folio *)NULL; })
#define folio_zero_tail(f, off, kaddr)	({ (void)(f); (void)(off); (void)(kaddr); (void *)NULL; })
#define folio_zero_segment(f, s, e)	do { (void)(f); (void)(s); (void)(e); } while (0)
#define folio_zero_segments(f, s1, e1, s2, e2)	do { } while (0)
#define folio_zero_new_buffers(f, f2, t)	do { } while (0)
#define folio_wait_stable(f)		do { } while (0)
#define folio_zero_range(f, s, l)	do { } while (0)
#define folio_mark_uptodate(f)		do { } while (0)
#define folio_next_index(f)		((f)->index + 1)
#define folio_next_pos(f)		((loff_t)folio_next_index(f) << PAGE_SHIFT)
#define folio_mapped(f)			(0)
#define fgf_set_order(size)		(0)
#define folio_clear_dirty_for_io(f)	({ (void)(f); 1; })
#define folio_clear_uptodate(f)		do { } while (0)
#define folio_nr_pages(f)		(1UL)
#define folio_contains(f, idx)		({ (void)(f); (void)(idx); 1; })
#define folio_clear_checked(f)		do { } while (0)
#define folio_test_dirty(f)		(0)
#define folio_test_writeback(f)		(0)
#define folio_wait_writeback(f)		do { } while (0)
#define folio_clear_dirty(f)		do { } while (0)
#define folio_test_checked(f)		(0)
#define folio_maybe_dma_pinned(f)	(0)
#define folio_set_checked(f)		do { } while (0)
#define folio_test_locked(f)		(0)
#define folio_mkclean(f)		(0)
#define page_folio(page)		((struct folio *)(page))
#define folio_address(folio)		((folio)->data)
#define folio_trylock(f)		({ (void)(f); 1; })

/* Folio writeback operations */
#define folio_end_writeback(f)		do { (void)(f); } while (0)
#define folio_start_writeback(f)	do { (void)(f); } while (0)
#define folio_start_writeback_keepwrite(f) do { (void)(f); } while (0)
#define folio_end_read(f, success)	do { (void)(f); (void)(success); } while (0)
#define folio_set_mappedtodisk(f)	do { (void)(f); } while (0)
#define folio_redirty_for_writepage(wbc, folio) \
	({ (void)(wbc); (void)(folio); false; })

/*
 * offset_in_folio - calculate offset of pointer within folio's data
 *
 * In Linux this uses page alignment, but in U-Boot we use the folio's
 * actual data pointer since our buffers are malloc'd.
 */
#define offset_in_folio(f, p)		((f) ? (unsigned int)((uintptr_t)(p) - (uintptr_t)(f)->data) : 0U)

/* folio_set_bh - associate buffer_head with folio */
#define folio_set_bh(bh, f, off)	do { if ((bh) && (f)) { (bh)->b_folio = (f); (bh)->b_data = (char *)(f)->data + (off); } } while (0)

#define memcpy_from_folio(dst, f, off, len)	do { (void)(dst); (void)(f); (void)(off); (void)(len); } while (0)

/* kmap/kunmap for folio access */
#define kmap_local_folio(folio, off)	((folio) ? (char *)(folio)->data + (off) : NULL)
#define kunmap_local(addr)		do { (void)(addr); } while (0)

/* mapping_gfp_mask - get GFP mask for address_space */
#define mapping_gfp_mask(m)		({ (void)(m); GFP_KERNEL; })

/* mapping_large_folio_support stub */
#define mapping_large_folio_support(m)	(0)

/* Filemap operations - stubs */
#define filemap_get_folios(m, i, e, fb)	({ (void)(m); (void)(i); (void)(e); (void)(fb); 0U; })
#define filemap_get_folio(m, i)		((struct folio *)NULL)
#define filemap_get_folios_tag(m, s, e, t, fb) \
	({ (void)(m); (void)(s); (void)(e); (void)(t); (void)(fb); 0U; })
#define filemap_lock_folio(m, i)	((struct folio *)NULL)
#define filemap_dirty_folio(m, f)	({ (void)(m); (void)(f); false; })
#define filemap_invalidate_lock(m)	do { } while (0)
#define filemap_invalidate_unlock(m)	do { } while (0)
#define filemap_invalidate_lock_shared(m) do { } while (0)
#define filemap_invalidate_unlock_shared(m) do { } while (0)
#define filemap_write_and_wait_range(m, s, e) ({ (void)(m); (void)(s); (void)(e); 0; })
#define filemap_fdatawrite_range(m, s, e) ({ (void)(m); (void)(s); (void)(e); 0; })
#define filemap_flush(m)		({ (void)(m); 0; })
#define filemap_write_and_wait(m)	({ (void)(m); 0; })
#define filemap_release_folio(folio, gfp) ({ (void)(folio); (void)(gfp); 1; })
#define mapping_tagged(m, t)		(0)
#define tag_pages_for_writeback(m, s, e) do { } while (0)
#define mapping_gfp_constraint(m, g)	(g)
#define mapping_set_folio_order_range(m, l, h) do { } while (0)
#define filemap_splice_read(i, p, pi, l, f) ({ (void)(i); (void)(p); (void)(pi); (void)(l); (void)(f); 0L; })
#define mapping_max_folio_order(m)	({ (void)(m); 0; })

/* Truncation stubs */
#define truncate_pagecache(i, s)	do { } while (0)
#define truncate_inode_pages(m, s)	do { } while (0)
#define truncate_inode_pages_final(m)	do { } while (0)
#define truncate_pagecache_range(i, s, e) do { } while (0)
#define truncate_inode_pages_range(m, s, e) do { (void)(m); (void)(s); (void)(e); } while (0)
#define pagecache_isize_extended(i, f, t) do { } while (0)
#define invalidate_mapping_pages(m, s, e) do { (void)(m); (void)(s); (void)(e); } while (0)

/* Filemap fault handlers */
static inline vm_fault_t filemap_fault(struct vm_fault *vmf)
{
	return 0;
}

static inline vm_fault_t filemap_map_pages(struct vm_fault *vmf,
					   pgoff_t start, pgoff_t end)
{
	return 0;
}

/* readahead_control stub */
struct readahead_control {
	struct address_space *mapping;
	struct file *file;
	unsigned long _index;
	unsigned int _batch_count;
};

#define readahead_pos(rac)		({ (void)(rac); 0LL; })
#define readahead_length(rac)		({ (void)(rac); 0UL; })
#define readahead_count(rac)		({ (void)(rac); 0UL; })
#define readahead_folio(rac)		({ (void)(rac); (struct folio *)NULL; })
#define page_cache_sync_readahead(m, ra, f, i, n) do { } while (0)
#define ra_has_index(ra, idx)		({ (void)(ra); (void)(idx); 0; })

/* Stub implementations for address_space_operations callbacks */
static inline bool block_is_partially_uptodate(struct folio *folio,
					       size_t from, size_t count)
{
	return false;
}

static inline int generic_error_remove_folio(struct address_space *mapping,
					     struct folio *folio)
{
	return 0;
}

/* writeback_iter stub */
#define writeback_iter(mapping, wbc, folio, error) \
	({ (void)(mapping); (void)(wbc); (void)(error); (struct folio *)NULL; })

#endif /* _LINUX_PAGEMAP_H */
