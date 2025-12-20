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
/* ext4_block_bitmap is now in super.c */
/* ext4_inode_bitmap is now in super.c */
/* ext4_inode_table is now in super.c */
/* __ext4_error_inode is now in super.c */
/* __ext4_error is now in super.c */
/* __ext4_std_error is now in super.c */
/* ext4_decode_error is now in super.c */

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

int jbd2_trans_will_send_data_barrier(journal_t *journal, unsigned long tid)
{
	return 0;
}

/*
 * Stubs for balloc.c
 */
/* ext4_mark_group_bitmap_corrupted is now in super.c */
/* __ext4_warning is now in super.c */

unsigned long long ext4_mb_new_blocks(void *handle, void *ar, int *errp)
{
	*errp = -1;
	return 0;
}

/* ext4_free_group_clusters is now in super.c */
/* ext4_clear_inode is now in super.c */
/* __ext4_msg is now in super.c */
/* ext4_free_group_clusters_set is now in super.c */
/* ext4_group_desc_csum_set is now in super.c */
/* ext4_itable_unused_count is now in super.c */
/* ext4_itable_unused_set is now in super.c */
/* ext4_free_inodes_count is now in super.c */
/* ext4_free_inodes_set is now in super.c */
/* ext4_used_dirs_count is now in super.c */

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
/* ext4_convert_inline_data is now in inline.c */

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

/* ext4_read_bh is now in super.c */
/* ext4_sb_bread_nofail is now in super.c */

/* ext4_init_security stub is provided by xattr.h */
/* xattr functions are now in xattr.c */

/*
 * Stubs for orphan.c
 */
struct ext4_iloc;

/* ext4_superblock_csum_set is now in super.c */
/* ext4_feature_set_ok is now in super.c */

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


/* __ext4_warning_inode is now in super.c */

/* Readahead */
int ext4_mpage_readpages(void *mapping, void *rac, void *folio)
{
	return 0;
}

/* ext4_readpage_inline is now in inline.c */

/* Xattr functions are now in xattr.c */

/* More JBD2 stubs */
int jbd2_journal_inode_ranged_write(void *handle, struct inode *inode,
				    loff_t start, loff_t len)
{
	return 0;
}


/* ext4_read_bh_lock is now in super.c */

/* Fast commit */
int ext4_fc_commit(void *journal, unsigned int tid)
{
	return 0;
}

/* ext4_force_commit is now in super.c */

/* Inline data is now in inline.c */

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

/* Inline data functions are now in inline.c */

/* __xattr_check_inode is now in xattr.c */

/* File and inode operations symbols */
/* ext4_file_inode_operations is now in file.c */
/* ext4_file_operations is now in file.c */
/* ext4_dir_inode_operations is now in namei.c */
/* ext4_dir_operations is now in dir.c */
/* ext4_special_inode_operations is now in namei.c */
/* ext4_symlink_inode_operations is now in symlink.c */
/* ext4_fast_symlink_inode_operations is now in symlink.c */


/* ext4_update_dynamic_rev is now in super.c */

/* Inline data stubs are now in inline.c */

/* xattr stubs are now in xattr.c */

/* JBD2 stubs for inode.c */
struct kmem_cache *jbd2_inode_cache;

int jbd2_journal_try_to_free_buffers(journal_t *journal, struct folio *folio)
{
	return 1;
}

void jbd2_journal_init_jbd_inode(void *jinode, struct inode *inode)
{
}

/* ext4_read_inline_link is now in inline.c */

/*
 * Stubs for dir.c
 */
ssize_t generic_read_dir(struct file *f, char *buf, size_t count, loff_t *ppos)
{
	return -EISDIR;
}

/* __ext4_error_file is now in super.c */

/* ext4_llseek is now in file.c */

/* ext4_htree_fill_tree is now in namei.c */

/* Inline dir stubs are now in inline.c */

/* Fast commit stubs */
void ext4_fc_track_unlink(void *handle, struct dentry *dentry)
{
}

void ext4_fc_track_link(void *handle, struct dentry *dentry)
{
}

void ext4_fc_track_create(void *handle, struct dentry *dentry)
{
}

void __ext4_fc_track_link(void *handle, struct inode *inode,
			  struct dentry *dentry)
{
}

void __ext4_fc_track_unlink(void *handle, struct inode *inode,
			    struct dentry *dentry)
{
}

void __ext4_fc_track_create(void *handle, struct inode *inode,
			    struct dentry *dentry)
{
}

/* fileattr stubs */
int ext4_fileattr_get(struct dentry *dentry, void *fa)
{
	return 0;
}

int ext4_fileattr_set(void *idmap, struct dentry *dentry, void *fa)
{
	return 0;
}

/* ext4_dirblock_csum_verify is now in namei.c */

/* ext4_ioctl is now in super.c */

/* ext4_sync_file is now in fsync.c */

/*
 * Stubs for super.c
 */

/* fscrypt stubs */
void fscrypt_free_dummy_policy(struct fscrypt_dummy_policy *policy)
{
}

int fscrypt_is_dummy_policy_set(const struct fscrypt_dummy_policy *policy)
{
	return 0;
}

int fscrypt_dummy_policies_equal(const struct fscrypt_dummy_policy *p1,
				 const struct fscrypt_dummy_policy *p2)
{
	return 1;
}

void fscrypt_show_test_dummy_encryption(struct seq_file *seq, char sep,
					struct super_block *sb)
{
}

void fscrypt_free_inode(struct inode *inode)
{
}

int fscrypt_drop_inode(struct inode *inode)
{
	return 0;
}

/* Block device stubs */
void bdev_fput(void *file)
{
}

void *bdev_file_open_by_dev(dev_t dev, int flags, void *holder,
			    const struct blk_holder_ops *ops)
{
	return ERR_PTR(-ENODEV);
}

struct buffer_head *bdev_getblk(struct block_device *bdev, sector_t block,
				unsigned int size, gfp_t gfp)
{
	return NULL;
}

int trylock_buffer(struct buffer_head *bh)
{
	return 1;
}

void submit_bh(int op, struct buffer_head *bh)
{
}

/* NFS export stubs */
struct dentry *generic_fh_to_parent(struct super_block *sb, struct fid *fid,
				    int fh_len, int fh_type,
				    struct inode *(*get_inode)(struct super_block *, u64, u32))
{
	return ERR_PTR(-ESTALE);
}

struct dentry *generic_fh_to_dentry(struct super_block *sb, struct fid *fid,
				    int fh_len, int fh_type,
				    struct inode *(*get_inode)(struct super_block *, u64, u32))
{
	return ERR_PTR(-ESTALE);
}

/* Inode stubs */
int inode_generic_drop(struct inode *inode)
{
	return 0;
}

void *alloc_inode_sb(struct super_block *sb, struct kmem_cache *cache,
		     gfp_t gfp)
{
	return NULL;
}

void inode_set_iversion(struct inode *inode, u64 version)
{
}

/* rwlock stubs */
void rwlock_init(rwlock_t *lock)
{
}

/* trace_ext4_drop_inode is now a macro in ext4_uboot.h */

/* Shutdown stub */
void ext4_force_shutdown(void *sb, int flags)
{
}

/* Memory stubs */
unsigned long roundup_pow_of_two(unsigned long n)
{
	unsigned long ret = 1;

	while (ret < n)
		ret <<= 1;
	return ret;
}

void *kvzalloc(size_t size, gfp_t flags)
{
	return calloc(1, size);
}

void ext4_kvfree_array_rcu(void *p)
{
	free(p);
}

/* String stubs */
/* strtomem_pad is now a macro in ext4_uboot.h */

char *strreplace(const char *str, char old, char new)
{
	char *s = (char *)str;

	while (*s) {
		if (*s == old)
			*s = new;
		s++;
	}
	return (char *)str;
}

char *kmemdup_nul(const char *s, size_t len, gfp_t gfp)
{
	char *buf;

	buf = kmalloc(len + 1, gfp);
	if (buf) {
		memcpy(buf, s, len);
		buf[len] = '\0';
	}
	return buf;
}

/* Page allocation */
unsigned long get_zeroed_page(gfp_t gfp)
{
	void *p = memalign(4096, 4096);

	if (p)
		memset(p, 0, 4096);
	return (unsigned long)p;
}

void free_page(unsigned long addr)
{
	free((void *)addr);
}

/* Trace stubs */
void trace_ext4_error(struct super_block *sb, const char *func, unsigned int line)
{
}

/* Rate limiting */
int ___ratelimit(struct ratelimit_state *rs, const char *func)
{
	return 1;
}

/* I/O priority */
int IOPRIO_PRIO_VALUE(int class, int data)
{
	return (class << 13) | data;
}

void set_task_ioprio(void *task, int ioprio)
{
}

/* Fast commit */
void ext4_fc_init(void *sb, void *journal)
{
}

/* Filesystem sync */
int sync_filesystem(void *sb)
{
	return 0;
}

/* Quota */
int dquot_suspend(void *sb, int flags)
{
	return 0;
}

/* MMP daemon */
void ext4_stop_mmpd(void *sbi)
{
}

/* Sysfs */
void ext4_unregister_sysfs(void *sb)
{
}

/* Journal destroy */
int jbd2_journal_destroy(void *journal)
{
	return 0;
}

/* percpu rwsem */
void percpu_free_rwsem(struct percpu_rw_semaphore *sem)
{
}

/* Block device ops */
void sync_blockdev(struct block_device *bdev)
{
}

void invalidate_bdev(struct block_device *bdev)
{
}

struct block_device *file_bdev(struct file *file)
{
	return NULL;
}

/* xattr cache is now in xattr.c */

/* kobject */
void kobject_put(struct kobject *kobj)
{
}

/* completion */
void wait_for_completion(struct completion *comp)
{
}

/* DAX */
void *fs_dax_get_by_bdev(struct block_device *bdev, u64 *start, u64 *len,
			 void *holder)
{
	return NULL;
}

void fs_put_dax(void *dax, void *holder)
{
}

/* Block size */
int sb_set_blocksize(struct super_block *sb, int size)
{
	return size;
}

/* Power of 2 check */
int is_power_of_2(unsigned long n)
{
	return n != 0 && (n & (n - 1)) == 0;
}

/* strscpy_pad is now a macro in ext4_uboot.h */
/* kmemdup_nul is defined earlier in this file */

/* Address check */
int generic_check_addressable(unsigned int blocksize_bits, u64 num_blocks)
{
	return 0;
}

/* Block device blocks */
u64 sb_bdev_nr_blocks(struct super_block *sb)
{
	return 0;
}

/* bgl_lock_init is now a macro in ext4_uboot.h */

/* xattr handlers are now in xattr.c */

/* super_set_uuid is now a macro in ext4_uboot.h */
/* super_set_sysfs_name_bdev is now a macro in ext4_uboot.h */
/* bdev_can_atomic_write is now a macro in ext4_uboot.h */
/* bdev_atomic_write_unit_max_bytes is now a macro in ext4_uboot.h */

/* Multi-mount protection */
int ext4_multi_mount_protect(void *sb, unsigned long long mmp_block)
{
	return 0;
}

/* Generic dentry ops */
void generic_set_sb_d_ops(struct super_block *sb)
{
}

struct dentry *d_make_root(struct inode *inode)
{
	return NULL;
}

/* percpu init rwsem */
int percpu_init_rwsem(struct percpu_rw_semaphore *sem)
{
	return 0;
}

/* Atomic operations */
void atomic_add(int val, atomic_t *v)
{
	v->counter += val;
}

void atomic64_add(s64 val, atomic64_t *v)
{
	v->counter += val;
}

/* Discard */
unsigned int bdev_max_discard_sectors(struct block_device *bdev)
{
	return 0;
}

/* Rate limit init */
void ratelimit_state_init(void *rs, int interval, int burst)
{
}

/* Sysfs */
int ext4_register_sysfs(void *sb)
{
	return 0;
}

/* dput - now provided as macro in ext4_uboot.h */

/* timer_delete_sync is now a macro in linux/timer.h */

/* ext4_get_parent is now in namei.c */

/* fsnotify */
void fsnotify_sb_error(struct super_block *sb, struct inode *inode, int error)
{
}

/* JBD2 force commit */
int jbd2_journal_force_commit(void *journal)
{
	return 0;
}

/* File path */
char *file_path(struct file *file, char *buf, int buflen)
{
	return buf;
}

/* Fast commit delete */
void ext4_fc_del(struct inode *inode)
{
}

/* invalidate_inode_buffers is now a macro in ext4_uboot.h */
/* clear_inode is now a macro in ext4_uboot.h */
/* fscrypt_put_encryption_info is now a macro in ext4_uboot.h */
/* fsverity_cleanup_inode is now a macro in ext4_uboot.h */

/* ext4_ioctl - file ioctls not supported in U-Boot */
long ext4_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOTSUPP;
}

/* JBD2 journal abort */
void jbd2_journal_abort(void *journal, int error)
{
}

/* JBD2 journal inode release */
void jbd2_journal_release_jbd_inode(void *journal, void *jinode)
{
}

/* nop_mnt_idmap - no-op mount ID map for xattr.c */
struct mnt_idmap nop_mnt_idmap;

/* Quota stubs for xattr.c */
int dquot_alloc_space_nodirty(struct inode *inode, loff_t size)
{
	return 0;
}

void dquot_free_space_nodirty(struct inode *inode, loff_t size)
{
}

int dquot_alloc_block(struct inode *inode, loff_t nr)
{
	return 0;
}

void dquot_free_block(struct inode *inode, loff_t nr)
{
}
