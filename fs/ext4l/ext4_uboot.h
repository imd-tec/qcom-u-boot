/* SPDX-License-Identifier: GPL-2.0 */
/*
 * U-Boot compatibility header for ext4l filesystem
 *
 * This provides minimal definitions to allow Linux ext4 code to compile
 * in U-Boot.
 */

#ifndef __EXT4_UBOOT_H__
#define __EXT4_UBOOT_H__

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/cred.h>

/* Rotate left - not available in U-Boot */
static inline u32 rol32(u32 word, unsigned int shift)
{
	return (word << (shift & 31)) | (word >> ((-shift) & 31));
}

/* Time types */
struct timespec64 {
	time_t tv_sec;
	long tv_nsec;
};

/* ktime_t - kernel time type */
typedef s64 ktime_t;

/* Jiffy constants */
#define MAX_JIFFY_OFFSET	((~0UL >> 1) - 1)

/* Block device name size */
#define BDEVNAME_SIZE		32

/* Atomic types - stubs for single-threaded U-Boot */
typedef struct { int counter; } atomic_t;
typedef struct { long counter; } atomic64_t;

#define atomic_read(v)		((v)->counter)
#define atomic_set(v, i)	((v)->counter = (i))
#define atomic64_read(v)	((v)->counter)
#define atomic64_set(v, i)	((v)->counter = (i))

/* Reference count type */
typedef struct { atomic_t refs; } refcount_t;

/* Lock types - stubs for single-threaded U-Boot */
typedef int rwlock_t;
/* spinlock_t is defined in linux/compat.h */

#define read_lock(l)		do { } while (0)
#define read_unlock(l)		do { } while (0)
#define write_lock(l)		do { } while (0)
#define write_unlock(l)		do { } while (0)

/* RB tree types - stubs */
struct rb_node {
	unsigned long __rb_parent_color;
	struct rb_node *rb_right;
	struct rb_node *rb_left;
};

struct rb_root {
	struct rb_node *rb_node;
};

#define RB_ROOT (struct rb_root) { NULL, }

/* percpu_counter - stub */
struct percpu_counter {
	long count;
};

static inline long percpu_counter_sum(struct percpu_counter *fbc)
{
	return fbc->count;
}

/* name_snapshot - stub */
struct name_snapshot {
	const char *name;
};

/* Project ID type */
typedef struct { unsigned int val; } kprojid_t;

/* kobject - stub */
struct kobject {
	const char *name;
};

/* completion - stub */
struct completion {
	unsigned int done;
};

/* Cache alignment - stub */
#define ____cacheline_aligned_in_smp

/* fscrypt_str - stub */
struct fscrypt_str {
	unsigned char *name;
	u32 len;
};

/* percpu rw semaphore - stubs */
struct percpu_rw_semaphore {
	int dummy;
};

#define percpu_down_read(sem)	do { } while (0)
#define percpu_up_read(sem)	do { } while (0)
#define percpu_down_write(sem)	do { } while (0)
#define percpu_up_write(sem)	do { } while (0)

/* Memory allocation context - stubs */
static inline unsigned int memalloc_nofs_save(void) { return 0; }
static inline void memalloc_nofs_restore(unsigned int flags) { }

/* Inode flags - stubs */
#define IS_CASEFOLDED(inode)	(0)
#define IS_ENCRYPTED(inode)	(0)

/* BUG_ON / BUG - stubs */
#define BUG_ON(cond)	do { } while (0)
#define BUG()		do { } while (0)

/* might_sleep - stub */
#define might_sleep()	do { } while (0)

/* sb_rdonly - stub */
#define sb_rdonly(sb)	(0)

/* Trace stubs */
#define trace_ext4_journal_start_inode(...)	do { } while (0)
#define trace_ext4_journal_start_sb(...)	do { } while (0)
#define trace_ext4_journal_start_reserved(...)	do { } while (0)
#define trace_ext4_forget(...)			do { } while (0)

/* Buffer operations - stubs */
#define wait_on_buffer(bh)		do { } while (0)
#define __bforget(bh)			do { } while (0)
#define mark_buffer_dirty_inode(bh, i)	do { } while (0)
#define mark_buffer_dirty(bh)		do { } while (0)
#define sync_dirty_buffer(bh)		do { } while (0)

/* inode_needs_sync - stub */
#define inode_needs_sync(inode)		(0)

/* Memory barriers - stubs for single-threaded */
#define smp_rmb()	do { } while (0)
#define smp_wmb()	do { } while (0)

/* Inode locking - stubs */
#define inode_is_locked(i)	(1)
#define i_size_write(i, s)	do { (i)->i_size = (s); } while (0)
#define i_size_read(i)		((i)->i_size)

/* spin_trylock is defined in linux/spinlock.h */

/* Atomic extras */
#define atomic_add_unless(v, a, u)	({ (void)(v); (void)(a); (void)(u); 1; })

/* Block group lock - stub */
#define bgl_lock_ptr(lock, group)	NULL

/* RCU stubs */
#define rcu_read_lock()			do { } while (0)
#define rcu_read_unlock()		do { } while (0)
#define rcu_dereference(p)		(p)
#define rcu_dereference_protected(p, c)	(p)
#define rcu_assign_pointer(p, v)	((p) = (v))
#define call_rcu(head, func)		do { func(head); } while (0)

/* RCU head for callbacks - defined in linux/compat.h as callback_head */

/* lockdep stubs */
#define lockdep_is_held(lock)		(1)

/* Memory allocation - use linux/slab.h which is already available */
#include <linux/slab.h>

/* KMEM_CACHE macro - not in U-Boot's slab.h */
#define KMEM_CACHE(s, flags)		((struct kmem_cache *)1)

/* RB tree operations - stubs */
#define rb_entry(ptr, type, member) \
	container_of(ptr, type, member)
#define rb_first(root)		((root)->rb_node)
#define rb_next(node)		((node)->rb_right)
#define rb_prev(node)		((node)->rb_left)
#define rb_insert_color(node, root)	do { } while (0)
#define rb_erase(node, root)		do { } while (0)
#define rb_link_node(node, parent, rb_link)	do { *(rb_link) = (node); } while (0)
#define RB_EMPTY_ROOT(root)	((root)->rb_node == NULL)
#define rbtree_postorder_for_each_entry_safe(pos, n, root, field) \
	for (pos = NULL, (void)(n); pos != NULL; )

/* RCU barrier - stub */
#define rcu_barrier()		do { } while (0)

/* inode operations - stubs */
#define iput(inode)		do { } while (0)

/* current task - from linux/sched.h */
#include <linux/sched.h>

/* _RET_IP_ - return instruction pointer */
#define _RET_IP_	((unsigned long)__builtin_return_address(0))

/* SB_FREEZE constants */
#define SB_FREEZE_WRITE		1
#define SB_FREEZE_PAGEFAULT	2
#define SB_FREEZE_FS		3
#define SB_FREEZE_COMPLETE	4

/* sb_writers stub */
struct sb_writers {
	int frozen;
};

/* mapping_large_folio_support stub */
#define mapping_large_folio_support(m)	(0)

/* sector_t - needed before buffer_head.h */
typedef unsigned long sector_t;

/* Buffer head - from linux/buffer_head.h */
#include <linux/buffer_head.h>

/* BH_JBDPrivateStart is defined in jbd2.h as an enum value */

/* Forward declare for get_block_t */
struct inode;
struct buffer_head;

/* get_block_t - block mapping callback */
typedef int (get_block_t)(struct inode *inode, sector_t iblock,
			  struct buffer_head *bh_result, int create);

/* crc32c - from linux/crc32c.h */
#include <linux/crc32c.h>

/* ratelimit_state - stub */
struct ratelimit_state {
	int dummy;
};

/* fscrypt_dummy_policy - stub */
struct fscrypt_dummy_policy {
	int dummy;
};

/* errseq_t is defined in linux/fs.h */

/* time64_t */
typedef s64 time64_t;

/* IS_NOQUOTA - stub */
#define IS_NOQUOTA(inode)	(0)

/* qstr - quick string for filenames (must be before dentry) */
struct qstr {
	const unsigned char *name;
	unsigned int len;
};

/* dentry - stub */
struct dentry {
	struct qstr d_name;
	struct inode *d_inode;
};

/* vm_fault_t - stub */
typedef unsigned int vm_fault_t;

/* Forward declarations for function prototypes */
struct kstat;
struct path;
struct vm_fault;
struct file_kattr;
struct dir_context;
struct readahead_control;
struct fiemap_extent_info;
struct folio;

/* qsize_t - quota size type */
typedef long long qsize_t;

/* blk_opf_t - block operation flags */
typedef unsigned int blk_opf_t;

/* Forward declare buffer_head for bh_end_io_t */
struct buffer_head;

/* bh_end_io_t - buffer head end io callback */
typedef void bh_end_io_t(struct buffer_head *bh, int uptodate);

/* Directory entry types */
#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

/* mnt_idmap - stub */
struct mnt_idmap;

/* fstrim_range - stub */
struct fstrim_range {
	u64 start;
	u64 len;
	u64 minlen;
};

/* rw_semaphore - defined in linux/rwsem.h, include it */
#include <linux/rwsem.h>

/* block_device is defined in linux/fs.h */

/* super_block - minimal stub */
struct super_block {
	void *s_fs_info;
	unsigned long s_blocksize;
	unsigned char s_blocksize_bits;
	unsigned long s_magic;
	loff_t s_maxbytes;
	struct rw_semaphore s_umount;
	struct sb_writers s_writers;
	struct block_device *s_bdev;
};

/* inode - minimal stub */
struct inode {
	struct super_block *i_sb;
	unsigned long i_ino;
	umode_t i_mode;
	unsigned int i_nlink;
	loff_t i_size;
	struct address_space *i_mapping;
};

#define QSTR_INIT(n, l) { .name = (const unsigned char *)(n), .len = (l) }

/*
 * Hash info structure - defined in ext4.h.
 * Only defined here for files that don't include ext4.h (like hash.c)
 * This is wrapped in EXT4_UBOOT_NO_EXT4_H which hash.c defines.
 */
#ifdef EXT4_UBOOT_NO_EXT4_H
struct dx_hash_info {
	u32 hash;
	u32 minor_hash;
	int hash_version;
	u32 *seed;
};
#endif

/* Hash algorithm types */
#define DX_HASH_LEGACY			0
#define DX_HASH_HALF_MD4		1
#define DX_HASH_TEA			2
#define DX_HASH_LEGACY_UNSIGNED		3
#define DX_HASH_HALF_MD4_UNSIGNED	4
#define DX_HASH_TEA_UNSIGNED		5
#define DX_HASH_SIPHASH			6
#define DX_HASH_LAST			DX_HASH_SIPHASH

/* EOF markers for htree */
#define EXT4_HTREE_EOF_32BIT   ((1UL  << (32 - 1)) - 1)
#define EXT4_HTREE_EOF_64BIT   ((1ULL << (64 - 1)) - 1)

/* jbd2_buffer_trigger_type is defined in jbd2.h */

/* seq_file - forward declaration */
struct seq_file;

/* fscrypt stubs - encryption not supported in U-Boot */
static inline bool fscrypt_has_encryption_key(const struct inode *inode)
{
	return false;
}

static inline u64 fscrypt_fname_siphash(const struct inode *dir,
					const struct qstr *name)
{
	return 0;
}

/* ext4 warning macros - stubs (only when ext4.h is not included) */
#ifdef EXT4_UBOOT_NO_EXT4_H
#define ext4_warning(sb, fmt, ...) \
	do { } while (0)

#define ext4_warning_inode(inode, fmt, ...) \
	do { } while (0)
#endif

/* fallthrough annotation */
#ifndef fallthrough
#define fallthrough __attribute__((__fallthrough__))
#endif

/* BUILD_BUG_ON - compile-time assertion */
#define BUILD_BUG_ON(cond) ((void)sizeof(char[1 - 2 * !!(cond)]))

/* Warning macros - stubs */
#define WARN_ON_ONCE(cond) ({ (void)(cond); 0; })
#define WARN_ON(cond) ({ (void)(cond); 0; })

/* Memory weight - count set bits */
static inline unsigned long memweight(const void *ptr, size_t bytes)
{
	unsigned long ret = 0;
	const unsigned char *p = ptr;
	size_t i;

	for (i = 0; i < bytes; i++)
		ret += hweight8(p[i]);
	return ret;
}

/* BITS_PER_BYTE */
#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE 8
#endif

#endif /* __EXT4_UBOOT_H__ */
