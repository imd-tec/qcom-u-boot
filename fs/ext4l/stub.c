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

int ext4_num_base_meta_blocks(struct super_block *sb, int group)
{
	return 0;
}

void *ext4_get_group_desc(struct super_block *sb, int group, void **bh)
{
	return NULL;
}

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
