/* SPDX-License-Identifier: GPL-2.0 */
/*
 * U-Boot compatibility header for ext4l filesystem
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * This provides minimal definitions to allow Linux ext4 code to compile
 * in U-Boot.
 */

#ifndef __EXT4_UBOOT_H__
#define __EXT4_UBOOT_H__

/*
 * Suppress warnings for unused static functions and variables in Linux ext4
 * source files. These are used in code paths that are stubbed out in U-Boot.
 */
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <div64.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <vsprintf.h>		/* For panic() */
#include <linux/string.h>
#include <linux/stat.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/log2.h>
#include <linux/init.h>
#include <linux/math64.h>
#include <linux/workqueue.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/iomap.h>
#include <linux/seq_file.h>
#include <linux/rbtree.h>	/* Real rbtree implementation */
#include <linux/time.h>		/* For timespec64, time64_t */
#include <linux/pagevec.h>	/* For struct folio_batch */
#include <linux/build_bug.h>	/* For BUILD_BUG_ON */
#include <linux/bug.h>		/* For WARN_ON, WARN_ONCE */
#include <u-boot/crc.h>		/* For crc32() used by crc32_be */
#include "ext4_trace.h"		/* Trace event stubs */
#include "ext4_fscrypt.h"	/* fscrypt stubs */

/*
 * __CHAR_UNSIGNED__ - directory hash algorithm selection
 *
 * The ext4 filesystem uses different hash algorithms for directory indexing
 * depending on whether the platform's 'char' type is signed or unsigned.
 * GCC automatically defines __CHAR_UNSIGNED__ on platforms where char is
 * unsigned (e.g., ARM), and leaves it undefined where char is signed
 * (e.g., x86/sandbox).
 *
 * The filesystem stores EXT2_FLAGS_UNSIGNED_HASH or EXT2_FLAGS_SIGNED_HASH
 * in the superblock to record which hash variant was used when the filesystem
 * was created, ensuring correct behavior regardless of the mounting platform.
 *
 * See super.c:5123 and ioctl.c:1489 for the hash algorithm selection code.
 */

/*
 * Override no_printk to avoid format warnings in disabled debug prints.
 * The Linux kernel uses sector_t as u64, but U-Boot uses unsigned long.
 * This causes format mismatches with %llu that we want to ignore.
 */
#undef no_printk
#define no_printk(fmt, ...)	({ 0; })

#include <asm-generic/atomic.h>
#include <linux/jiffies.h>
#include <linux/blkdev.h>
#include <linux/blk_types.h>
#include <linux/fs_context.h>
#include <linux/fs_parser.h>
#include <linux/dcache.h>
#include <linux/uuid.h>
#include <linux/smp.h>
#include <linux/refcount.h>
#include <linux/spinlock.h>
#include <linux/percpu_counter.h>
#include <linux/percpu.h>
#include <linux/projid.h>
#include <linux/kobject.h>
#include <linux/lockdep.h>
#include <linux/completion.h>
#include <linux/cache.h>
#include <linux/capability.h>
#include <linux/fiemap.h>
#include <linux/uio.h>
#include <linux/sched/mm.h>

#define EXT4_FIEMAP_EXTENT_HOLE		0x08000000
#define O_SYNC		0
#define S_NOQUOTA		0

#ifndef PAGE_SHIFT
#define PAGE_SHIFT	12
#endif

/*
 * BUG_ON / BUG - stubs (not using linux/bug.h which panics)
 * In Linux, these indicate kernel bugs. In ext4l, some BUG_ON conditions
 * that check for race conditions can trigger in single-threaded U-Boot,
 * so we stub them out as no-ops.
 */
#undef BUG_ON
#undef BUG
#define BUG_ON(cond)	do { (void)(cond); } while (0)
#define BUG()		do { } while (0)

#include <asm-generic/bitops/le.h>
#include <kunit/static_stub.h>
#include <linux/quotaops.h>
#include <linux/random.h>

/* Inode operations - iget_locked and new_inode are in interface.c */
extern struct inode *new_inode(struct super_block *sb);

/* Forward declarations for xattr functions */
struct super_block;
struct buffer_head;
struct qstr;

#ifdef CONFIG_EXT4_XATTR
int __ext4_xattr_set_credits(struct super_block *sb, struct inode *inode,
			     struct buffer_head *block_bh, size_t value_len,
			     bool is_create);
#endif

#include <linux/rcupdate.h>
#include <linux/slab.h>

void iput(struct inode *inode);

#include <linux/sched.h>
#include <linux/buffer_head.h>
#include <linux/jbd2.h>

/*
 * U-Boot buffer head private bits.
 *
 * Start at BH_JBDPrivateStart + 1 because ext4.h uses BH_JBDPrivateStart
 * for BH_BITMAP_UPTODATE.
 */
#define BH_OwnsData		(BH_JBDPrivateStart + 1)
BUFFER_FNS(OwnsData, ownsdata)

/*
 * U-Boot: marks buffer is in the buffer cache.
 * Cached buffers are freed by bh_cache_clear(), not brelse().
 */
#define BH_Cached		(BH_JBDPrivateStart + 2)
BUFFER_FNS(Cached, cached)

#include <linux/crc32c.h>
#include <linux/ratelimit.h>
#include <linux/mm_types.h>
#include <linux/mnt_idmapping.h>
#include <linux/rwsem.h>

/* Forward declarations */
struct pipe_inode_info;
struct kstat;
struct path;
struct file_kattr;
struct dir_context;
struct readahead_control;
struct fiemap_extent_info;
struct folio;

#define WHITEOUT_DEV	0
#define WHITEOUT_MODE	0

/* QSTR_INIT and dotdot_name are now in linux/dcache.h */

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

#ifdef EXT4_UBOOT_NO_EXT4_H
#define ext4_warning(sb, fmt, ...) \
	do { } while (0)

#define ext4_warning_inode(inode, fmt, ...) \
	do { } while (0)
#endif

#define file_modified(file)		({ (void)(file); 0; })
#define file_accessed(file)		do { (void)(file); } while (0)

#define vfs_setpos(file, offset, maxsize)	({ (void)(file); (void)(maxsize); (offset); })

#define daxdev_mapping_supported(f, i, d) ({ (void)(f); (void)(i); (void)(d); 1; })

#include <linux/minmax.h>

/* Memory retry wait */
#define memalloc_retry_wait(g)		do { } while (0)

/* indirect.c stubs */

/* ext4_sb_bread_nofail is stubbed in interface.c */

/* extents_status.c stubs */

/* shrinker - use linux/shrinker.h */
#include <linux/shrinker.h>

/* ktime functions - use linux/ktime.h */
#include <linux/ktime.h>

/* hrtimer - use linux/hrtimer.h */
#include <linux/hrtimer.h>

/* folio and pagemap - use linux/pagemap.h */
#include <linux/pagemap.h>
#include <linux/xarray.h>

/* wbc_to_tag, WB_REASON_* - use linux/writeback.h */
#include <linux/writeback.h>

/* projid_t is now in linux/projid.h */

/*
 * Additional stubs for inode.c
 */

/* try_cmpxchg is now in asm-generic/atomic.h */

/* hash_64 - use linux/hash.h */
#include <linux/hash.h>

/* Dentry operations are now in linux/dcache.h */
#define finish_open_simple(f, e)		(e)
#define ihold(i)				do { (void)(i); } while (0)

/* Sync operations - stubs */
#define sync_mapping_buffers(m)			({ (void)(m); 0; })
#define sync_inode_metadata(i, w)		({ (void)(i); (void)(w); 0; })
#define file_write_and_wait_range(f, s, e)	({ (void)(f); (void)(s); (void)(e); 0; })
#define file_check_and_advance_wb_err(f)	({ (void)(f); 0; })

/* DAX stubs - DAX not supported in U-Boot */
#define IS_DAX(inode)				(0)
#define dax_break_layout_final(inode)		do { } while (0)
#define dax_writeback_mapping_range(m, bd, wb)	({ (void)(m); (void)(bd); (void)(wb); 0; })
#define dax_zero_range(i, p, l, d, op)		({ (void)(i); (void)(p); (void)(l); (void)(d); (void)(op); -EOPNOTSUPP; })
#define dax_break_layout_inode(i, m)		({ (void)(i); (void)(m); 0; })

#include <linux/path.h>

#include <linux/fsverity.h>
#include <linux/iversion.h>
#include <linux/kdev_t.h>

/* UID/GID bit helpers - use linux/highuid.h */
#include <linux/highuid.h>

/* Inode allocation/state operations */
extern struct inode *iget_locked(struct super_block *sb, unsigned long ino);

/* Attribute operations */
#define setattr_prepare(m, d, a)	({ (void)(m); (void)(d); (void)(a); 0; })
#define setattr_copy(m, i, a)		do { } while (0)
#define posix_acl_chmod(m, i, mo)	({ (void)(m); (void)(i); (void)(mo); 0; })

/* File operations */
#define file_update_time(f)		do { } while (0)
#define vmf_fs_error(e)			((vm_fault_t)VM_FAULT_SIGBUS)

/* iomap stubs */
#define iomap_bmap(m, b, o)		({ (void)(m); (void)(b); (void)(o); 0UL; })
#define iomap_swapfile_activate(s, f, sp, o) ({ (void)(s); (void)(f); (void)(sp); (void)(o); -EOPNOTSUPP; })

/*
 * Additional stubs for dir.c
 */

/* FSTR_INIT - fscrypt_str initializer (fscrypt_str defined in ext4_fscrypt.h) */
#define FSTR_INIT(n, l)		{ .name = (n), .len = (l) }

/* struct_size - use linux/overflow.h */
#include <linux/overflow.h>

#include <linux/delayed_call.h>

#define kfree_link		kfree

/* nd_terminate_link - terminate symlink string */
static inline void nd_terminate_link(void *name, loff_t len, int maxlen)
{
	((char *)name)[min_t(loff_t, len, maxlen)] = '\0';
}

/* file open helper */
#define simple_open(i, f)		({ (void)(i); (void)(f); 0; })

/* simple_get_link - for fast symlinks stored in inode */
static inline const char *simple_get_link(struct dentry *dentry,
					  struct inode *inode,
					  struct delayed_call *callback)
{
	return inode->i_link;
}

/*
 * Additional stubs for super.c
 */

/* Part stat - not used in U-Boot. Note: sectors[X] is passed as second arg */
#define STAT_WRITE		0
#define STAT_READ		0
static u64 __attribute__((unused)) __ext4_sectors[2];
#define sectors			__ext4_sectors
#define part_stat_read(p, f)	({ (void)(p); (void)(f); 0ULL; })

/*
 * Hex dump - DUMP_PREFIX_* types are in hexdump.h.
 * However, the Linux kernel print_hex_dump has a different signature
 * (includes log level) than U-Boot's, so we stub it out here.
 */
#include <hexdump.h>
#undef print_hex_dump
#define print_hex_dump(l, p, pt, rg, gc, b, len, a) do { } while (0)

/* Forward declarations for super_operations and export_operations */
struct kstatfs;
struct fid;

#include <linux/exportfs.h>
#include <linux/statfs.h>
#include <linux/module.h>

/* EXT4_GOING flags */
#define EXT4_GOING_FLAGS_DEFAULT	0
#define EXT4_GOING_FLAGS_LOGFLUSH	1
#define EXT4_GOING_FLAGS_NOLOGFLUSH	2

/* ext4 superblock initialisation and commit */
int ext4_fill_super(struct super_block *sb, struct fs_context *fc);
int ext4_commit_super(struct super_block *sb);
void ext4_unregister_li_request(struct super_block *sb);

#include <linux/ctype.h>

#include <linux/crc16.h>
#include <linux/namei.h>

/* I/O priority classes - use linux/ioprio.h */
#include <linux/ioprio.h>

/* blk_holder_ops for block device */
struct blk_holder_ops {
	void (*mark_dead)(struct block_device *, bool);
};
static const struct blk_holder_ops fs_holder_ops;

/* end_buffer_write_sync - implemented in support.c */
void end_buffer_write_sync(struct buffer_head *bh, int uptodate);

/* File system management time flag */
#define FS_MGTIME			0

/* Block size */
#define BLOCK_SIZE			1024

#define EXT4_SUPER_MAGIC		0xEF53

/* blockgroup_lock - use linux/blockgroup_lock.h */
#include <linux/blockgroup_lock.h>

/* Buffer submission stubs - declaration for stub.c */
int trylock_buffer(struct buffer_head *bh);

/* Trace stubs for super.c - declaration for stub.c implementation */
void trace_ext4_error(struct super_block *sb, const char *func, unsigned int line);

/* ___ratelimit is now in linux/ratelimit.h */

/* Filesystem notification - declaration for stub.c */
void fsnotify_sb_error(struct super_block *sb, struct inode *inode, int error);

/* File path operations - declaration for stub.c */
char *file_path(struct file *file, char *buf, int buflen);
struct block_device *file_bdev(struct file *file);

/* kobject_put is now in linux/kobject.h */
/* wait_for_completion is now a macro in linux/completion.h */

/* DAX - declaration for stub.c */
void fs_put_dax(void *dax, void *holder);

/* slab usercopy - use regular kmem_cache_create */
#define kmem_cache_create_usercopy(n, sz, al, fl, uo, us, c) \
	kmem_cache_create(n, sz, al, fl, c)

/* Memory allocation - declarations for stub.c */
void *kvzalloc(size_t size, gfp_t flags);
#define kvmalloc(size, flags)	kvzalloc(size, flags)

/* Page allocation - declarations for stub.c */
unsigned long get_zeroed_page(gfp_t gfp);
void free_page(unsigned long addr);

/* DAX - declaration for stub.c */
void *fs_dax_get_by_bdev(struct block_device *bdev, u64 *start, u64 *len,
			 void *holder);

#include <linux/mbcache.h>

/* xattr helper stubs for xattr.c */
#define xattr_handler_can_list(h, d)		({ (void)(h); (void)(d); 0; })
#define xattr_prefix(h)				({ (void)(h); (const char *)NULL; })

/* Filesystem sync - declaration for stub.c */
int sync_filesystem(void *sb);

/*
 * Stubs for mballoc.c
 */

#include <asm-generic/timex.h>
#include <linux/nospec.h>

/* pde_data - proc dir entry data (not supported in U-Boot) */
#define pde_data(inode)			((void *)NULL)

/* DEFINE_RAW_FLEX - define a flexible array struct on the stack (stubbed to NULL) */
#define DEFINE_RAW_FLEX(type, name, member, count) \
	type *name = NULL

/* raw_cpu_ptr - get pointer to per-CPU data for current CPU */
#define raw_cpu_ptr(ptr)		(ptr)

/*
 * Stubs for page-io.c - bio types are in linux/bio.h
 */
#include <linux/bio.h>

/*
 * Stubs for readpage.c
 */

#include <linux/mempool.h>

/* prefetch operations */
#define prefetchw(addr)			do { (void)(addr); } while (0)

/*
 * Stubs for fast_commit.c
 */

/* Wait bit operations - use linux/wait_bit.h */
#include <linux/wait_bit.h>

/* JBD2 checkpoint.c and commit.c stubs */
#include <asm-generic/bitops/lock.h>
/* smp_mb__after_atomic is now in linux/smp.h */
#define ktime_get_coarse_real_ts64(ts)	do { (ts)->tv_sec = 0; (ts)->tv_nsec = 0; } while (0)
#define filemap_fdatawait_range_keep_errors(m, s, e) \
	({ (void)(m); (void)(s); (void)(e); 0; })
#define crc32_be(crc, p, len)		crc32(crc, p, len)

/* ext4l support functions (support.c) */
void ext4l_crc32c_init(void);
void bh_cache_release_jbd(void);
void bh_cache_clear(void);
int bh_cache_sync(void);
int ext4l_read_block(sector_t block, size_t size, void *buffer);
int ext4l_write_block(sector_t block, size_t size, void *buffer);
void ext4l_msg_init(void);
void ext4l_record_msg(const char *msg, int len);
struct membuf *ext4l_get_msg_buf(void);
void ext4l_print_msgs(void);

/* ext4l interface functions (interface.c) */
struct blk_desc *ext4l_get_blk_dev(void);
struct disk_partition *ext4l_get_partition(void);

/* DEFINE_WAIT stub - creates a wait queue entry */
#define DEFINE_WAIT(name)		int name = 0

/* JBD2 journal.c stubs */
int bmap(struct inode *inode, sector_t *block);

#include <linux/proc_fs.h>

/* Block device operations for journal.c */
#define truncate_inode_pages_range(m, s, e) \
	do { (void)(m); (void)(s); (void)(e); } while (0)

/* Memory allocation for journal.c */
#define __get_free_pages(gfp, order)	((unsigned long)memalign(PAGE_SIZE, PAGE_SIZE << (order)))
#define free_pages(addr, order)		free((void *)(addr))
#define get_order(size)			ilog2(roundup_pow_of_two((size) / PAGE_SIZE))

/*
 * Stubs for mmp.c
 */

/* init_utsname - use linux/utsname.h */
#include <linux/utsname.h>

/*
 * Stubs for move_extent.c
 */

/* down_write_nested - nested write lock acquisition */
#define down_write_nested(sem, subclass) \
	do { (void)(sem); (void)(subclass); } while (0)

/* PAGE_MASK - mask for page alignment */
#ifndef PAGE_MASK
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#endif

/*
 * Stubs for resize.c
 */

/* time_is_before_jiffies - check if time is before current jiffies */
#define time_is_before_jiffies(a)	({ (void)(a); 0; })

/* ext4_update_overhead - declaration for stub.c */
int ext4_update_overhead(struct super_block *sb, bool force);

/*
 * Stubs for fsmap.c
 */

/* fsmap is now in linux/fsmap.h */
#include <linux/fsmap.h>

/* list_sort and sort stubs for fsmap.c - not used in U-Boot */
#define list_sort(priv, head, cmp) \
	do { (void)(priv); (void)(head); (void)(cmp); } while (0)
#define sort(base, num, size, cmp, swap) \
	do { (void)(base); (void)(num); (void)(size); (void)(cmp); (void)(swap); } while (0)

#endif /* __EXT4_UBOOT_H__ */
