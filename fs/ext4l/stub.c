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

#include <linux/types.h>

struct super_block;
struct buffer_head;
struct inode;
struct ext4_map_blocks;

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

struct inode *__ext4_iget(struct super_block *sb, unsigned long ino,
			  int flags, const char *func, unsigned int line)
{
	return NULL;
}

int ext4_map_blocks(void *handle, struct inode *inode,
		    struct ext4_map_blocks *map, int flags)
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
