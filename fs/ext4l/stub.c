// SPDX-License-Identifier: GPL-2.0+
/*
 * Stub functions for ext4l filesystem
 *
 * These stubs allow the ext4l code to link during development while not all
 * source files are present. They will be removed once the full ext4l
 * implementation is complete.
 *
 * DO NOT use this file as a reference - these are temporary placeholders only.
 */

#include "ext4_uboot.h"
#include <linux/types.h>

struct super_block;
struct buffer_head;
struct inode;
struct ext4_map_blocks;
struct file;

/* bh_end_io_t - buffer head end io callback */
typedef void bh_end_io_t(struct buffer_head *bh, int uptodate);

/* ext4_num_base_meta_blocks and ext4_get_group_desc are now in balloc.c */

unsigned long ext4_block_bitmap(struct super_block *sb, void *gdp)
{
	return 0;
}

unsigned long ext4_inode_bitmap(struct super_block *sb, void *gdp)
{
	return 0;
}

unsigned long ext4_inode_table(struct super_block *sb, void *gdp)
{
	return 0;
}

void __ext4_error_inode(struct inode *inode, const char *func,
			unsigned int line, unsigned long block,
			int error, const char *fmt, ...)
{
}

void __ext4_error(struct super_block *sb, const char *func,
		  unsigned int line, bool force_ro, int error,
		  unsigned long long block, const char *fmt, ...)
{
}

const char *ext4_decode_error(struct super_block *sb, int errno, char *nbuf)
{
	return "error";
}

void __ext4_std_error(struct super_block *sb, const char *func,
		      unsigned int line, int errno)
{
}

/*
 * JBD2 journal stubs
 */
struct jbd2_journal_handle;
typedef struct jbd2_journal_handle handle_t;
struct journal_s;
typedef struct journal_s journal_t;
struct jbd2_buffer_trigger_type;

handle_t *jbd2__journal_start(journal_t *journal, int nblocks, int rsv_blocks,
			      int revoke_records, int gfp_mask, int type,
			      unsigned int line_no)
{
	return NULL;
}

int jbd2_journal_stop(handle_t *handle)
{
	return 0;
}

void jbd2_journal_free_reserved(handle_t *handle)
{
}

int jbd2_journal_start_reserved(handle_t *handle, int type, unsigned int line)
{
	return 0;
}

int jbd2_journal_extend(handle_t *handle, int nblocks, int revoke_records)
{
	return 0;
}

int jbd2_journal_get_write_access(handle_t *handle, struct buffer_head *bh)
{
	return 0;
}

void jbd2_journal_set_triggers(struct buffer_head *bh,
			       struct jbd2_buffer_trigger_type *type)
{
}

int jbd2_journal_forget(handle_t *handle, struct buffer_head *bh)
{
	return 0;
}

int jbd2_journal_revoke(handle_t *handle, unsigned long long blocknr,
			struct buffer_head *bh)
{
	return 0;
}

int jbd2_journal_get_create_access(handle_t *handle, struct buffer_head *bh)
{
	return 0;
}

int jbd2_journal_dirty_metadata(handle_t *handle, struct buffer_head *bh)
{
	return 0;
}

int jbd2_journal_force_commit_nested(journal_t *journal)
{
	return 0;
}

int jbd2__journal_restart(void *handle, int nblocks, int revoke_records,
			  int gfp_mask)
{
	return 0;
}

/*
 * Stubs for balloc.c
 */
void ext4_mark_group_bitmap_corrupted(struct super_block *sb,
				      unsigned int group, unsigned int flags)
{
}

void __ext4_warning(struct super_block *sb, const char *func,
		    unsigned int line, const char *fmt, ...)
{
}

unsigned long long ext4_mb_new_blocks(void *handle, void *ar, int *errp)
{
	*errp = -1;
	return 0;
}

unsigned int ext4_free_group_clusters(struct super_block *sb, void *gdp)
{
	return 0;
}

/*
 * Stubs for ialloc.c
 */
void ext4_clear_inode(struct inode *inode)
{
}

void __ext4_msg(struct super_block *sb, const char *prefix,
		const char *fmt, ...)
{
}

void ext4_free_group_clusters_set(struct super_block *sb, void *gdp,
				  unsigned int count)
{
}

void ext4_group_desc_csum_set(struct super_block *sb, unsigned int group,
			      void *gdp)
{
}

unsigned int ext4_itable_unused_count(struct super_block *sb, void *gdp)
{
	return 0;
}

void ext4_itable_unused_set(struct super_block *sb, void *gdp, unsigned int v)
{
}

unsigned int ext4_free_inodes_count(struct super_block *sb, void *gdp)
{
	return 0;
}

void ext4_free_inodes_set(struct super_block *sb, void *gdp, unsigned int v)
{
}

unsigned int ext4_used_dirs_count(struct super_block *sb, void *gdp)
{
	return 0;
}

/*
 * Bit operations - sandbox declares these extern but doesn't implement them.
 */
void set_bit(int nr, void *addr)
{
	unsigned long *p = (unsigned long *)addr;

	*p |= (1UL << nr);
}

void clear_bit(int nr, void *addr)
{
	unsigned long *p = (unsigned long *)addr;

	*p &= ~(1UL << nr);
}

void change_bit(int nr, void *addr)
{
	unsigned long *p = (unsigned long *)addr;

	*p ^= (1UL << nr);
}

/*
 * Stubs for extents.c
 */
struct ext4_sb_info;
struct ext4_es_tree;
struct extent_status;

/* ext4_es_cache_extent is now in extents_status.c */

/* ext4_es_insert_extent is now in extents_status.c */

/* ext4_remove_pending is now in extents_status.c */

void ext4_free_blocks(void *handle, struct inode *inode,
		      struct buffer_head *bh, unsigned long long block,
		      unsigned long count, int flags)
{
}

void ext4_discard_preallocations(struct inode *inode, unsigned int needed)
{
}

/* ext4_is_pending is now in extents_status.c */

int ext4_convert_inline_data(struct inode *inode)
{
	return 0;
}

void ext4_fc_mark_ineligible(struct super_block *sb, int reason,
			     void *handle)
{
}

/* ext4_es_lookup_extent is now in extents_status.c */

/* ext4_es_remove_extent is now in extents_status.c */

/* ext4_es_find_extent_range is now in extents_status.c */

void ext4_mb_mark_bb(struct super_block *sb, unsigned long long block,
		     int len, int state)
{
}

void ext4_fc_record_regions(struct super_block *sb, int ino,
			    unsigned long lblk, unsigned long long pblk,
			    int len, int mapped)
{
}

int ext4_read_bh(struct buffer_head *bh, unsigned int op_flags,
		 bh_end_io_t *end_io, bool simu_fail)
{
	return 0;
}

struct buffer_head *ext4_sb_bread_nofail(struct super_block *sb,
					 unsigned long long block)
{
	return NULL;
}

/*
 * Stubs for ialloc.c - xattr functions
 */
int __ext4_xattr_set_credits(struct super_block *sb, struct inode *inode,
			     struct buffer_head *block_bh, size_t value_len,
			     bool is_create)
{
	return 0;
}

/* ext4_init_security stub is provided by xattr.h */

/*
 * Stubs for xattr_trusted.c
 */
int ext4_xattr_get(struct inode *inode, int name_index, const char *name,
		   void *buffer, size_t buffer_size)
{
	return -1;
}

int ext4_xattr_set(struct inode *inode, int name_index, const char *name,
		   const void *value, size_t value_len, int flags)
{
	return -1;
}

/*
 * Stubs for orphan.c
 */
struct ext4_iloc;

void ext4_superblock_csum_set(struct super_block *sb)
{
}

int ext4_feature_set_ok(struct super_block *sb, int readonly)
{
	return 1;
}

/*
 * Stubs for inode.c
 */
#include <linux/sched.h>

/* JBD2 stubs for inode.c */
int jbd2_journal_blocks_per_folio(struct inode *inode)
{
	return 1;
}

int jbd2_transaction_committed(void *journal, unsigned int tid)
{
	return 1;
}


void __ext4_warning_inode(struct inode *inode, const char *func,
			  unsigned int line, const char *fmt, ...)
{
}


/* Readahead */
int ext4_mpage_readpages(void *mapping, void *rac, void *folio)
{
	return 0;
}

int ext4_readpage_inline(struct inode *inode, void *folio)
{
	return 0;
}

/* Xattr */
int ext4_expand_extra_isize_ea(struct inode *inode, int new_extra_isize,
			       void *raw_inode, void *handle)
{
	return 0;
}

void ext4_evict_ea_inode(struct inode *inode)
{
}


/* More JBD2 stubs */
int jbd2_journal_inode_ranged_write(void *handle, struct inode *inode,
				    loff_t start, loff_t len)
{
	return 0;
}


int ext4_read_bh_lock(struct buffer_head *bh, int op_flags, int nowait)
{
	return 0;
}


/* Fast commit */
int ext4_fc_commit(void *journal, unsigned int tid)
{
	return 0;
}

int ext4_force_commit(struct super_block *sb)
{
	return 0;
}


/* Inline data */
int ext4_destroy_inline_data(void *handle, struct inode *inode)
{
	return 0;
}

/* I/O submit */
void ext4_io_submit_init(void *io, void *wbc)
{
}


void *ext4_init_io_end(struct inode *inode, int gfp)
{
	return NULL;
}

void ext4_io_submit(void *io)
{
}

void ext4_put_io_end_defer(void *io_end)
{
}

void ext4_put_io_end(void *io_end)
{
}

void *ext4_alloc_io_end_vec(void *io_end, unsigned long num)
{
	return NULL;
}


/* JBD2 ordered truncate */
int jbd2_journal_begin_ordered_truncate(void *ji, loff_t new_size)
{
	return 0;
}

void jbd2_journal_invalidate_folio(void *journal, void *folio,
				   unsigned long off, unsigned int len)
{
}

int jbd2_log_wait_commit(void *journal, unsigned int tid)
{
	return 0;
}

/* Fast commit */
void ext4_fc_track_range(void *handle, struct inode *inode,
			 unsigned long long start, unsigned long long end)
{
}


/* JBD2 journal update locking */
void jbd2_journal_lock_updates(void *journal)
{
}

void jbd2_journal_unlock_updates(void *journal)
{
}

int jbd2_journal_flush(void *journal, unsigned int flags)
{
	return 0;
}


/* Fast commit */
void ext4_fc_track_inode(void *handle, struct inode *inode)
{
}

void ext4_fc_init_inode(void **head, struct inode *inode)
{
}

/* JBD2 */
int jbd2_journal_inode_ranged_wait(void *handle, struct inode *inode,
				   loff_t start, loff_t len)
{
	return 0;
}

/* Inline data */
int ext4_inline_data_iomap(struct inode *inode, void *iomap)
{
	return 0;
}


/* xattr */
int __xattr_check_inode(struct inode *inode, void *entry, void *end,
			unsigned int size, int check_block)
{
	return 0;
}

int ext4_find_inline_data_nolock(struct inode *inode)
{
	return 0;
}


/* File and inode operations symbols */
char ext4_file_inode_operations;
char ext4_file_operations;
char ext4_dir_inode_operations;
char ext4_dir_operations;
char ext4_special_inode_operations;
char ext4_symlink_inode_operations;
char ext4_fast_symlink_inode_operations;


void ext4_update_dynamic_rev(struct super_block *sb)
{
}


/* Inline data */
int ext4_inline_data_truncate(struct inode *inode, int *has_inline)
{
	*has_inline = 0;
	return 0;
}

int ext4_try_to_write_inline_data(struct address_space *mapping,
				  struct inode *inode, loff_t pos,
				  unsigned int len, struct folio **foliop)
{
	return 0;
}

int ext4_generic_write_inline_data(struct address_space *mapping,
				   struct inode *inode, loff_t pos,
				   unsigned int len, struct folio **foliop)
{
	return 0;
}

int ext4_write_inline_data_end(struct inode *inode, loff_t pos, unsigned int len,
			       unsigned int copied, struct folio *folio)
{
	return copied;
}

/* xattr stubs for inode.c */
int ext4_xattr_delete_inode(handle_t *handle, struct inode *inode,
			    void **array, int extra_credits)
{
	return 0;
}

void ext4_xattr_inode_array_free(void *array)
{
}

/* JBD2 stubs for inode.c */
struct kmem_cache *jbd2_inode_cache;

int jbd2_journal_try_to_free_buffers(journal_t *journal, struct folio *folio)
{
	return 1;
}

void jbd2_journal_init_jbd_inode(void *jinode, struct inode *inode)
{
}
