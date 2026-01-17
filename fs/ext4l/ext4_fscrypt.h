/* SPDX-License-Identifier: GPL-2.0 */
/*
 * fscrypt stubs for U-Boot ext4l
 *
 * In Linux, fscrypt provides filesystem-level encryption. In U-Boot,
 * encryption is not supported, so all fscrypt operations are stubbed out.
 */

#ifndef _EXT4_FSCRYPT_H
#define _EXT4_FSCRYPT_H

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/dcache.h>

/* Forward declarations */
struct inode;
struct seq_file;
struct page;
struct folio;
struct bio;
struct buffer_head;
struct super_block;

/* fscrypt_str - encrypted filename string */
struct fscrypt_str {
	unsigned char *name;
	u32 len;
};

/* fscrypt_dummy_policy - stub */
struct fscrypt_dummy_policy {
	int dummy;
};

/* fscrypt_name - stub structure for encrypted filenames */
struct fscrypt_name {
	const struct qstr *usr_fname;
	struct fscrypt_str disk_name;
	u32 hash;
	u32 minor_hash;
	bool is_nokey_name;
};

/* fscrypt context size */
#define FSCRYPT_SET_CONTEXT_MAX_SIZE	40

/* IS_ENCRYPTED - always false in U-Boot */
#define IS_ENCRYPTED(inode)	(0)

/* fscrypt inline functions */
static inline bool fscrypt_has_encryption_key(const struct inode *inode)
{
	return false;
}

static inline u64 fscrypt_fname_siphash(const struct inode *dir,
					const struct qstr *name)
{
	return 0;
}

static inline int fscrypt_match_name(const struct fscrypt_name *fname,
				     const u8 *de_name, u32 de_name_len)
{
	if (fname->usr_fname->len != de_name_len)
		return 0;

	return !memcmp(fname->usr_fname->name, de_name, de_name_len);
}

/* fscrypt operation stubs */
#define fscrypt_prepare_new_inode(dir, i, e)	({ (void)(dir); (void)(i); (void)(e); 0; })
#define fscrypt_set_context(inode, handle)	({ (void)(inode); (void)(handle); 0; })
#define fscrypt_file_open(i, f)			({ (void)(i); (void)(f); 0; })
#define fscrypt_inode_uses_fs_layer_crypto(i)	(0)
#define fscrypt_decrypt_pagecache_blocks(f, l, o) ({ (void)(f); (void)(l); (void)(o); 0; })
#define fscrypt_encrypt_pagecache_blocks(f, l, o, g) \
	({ (void)(f); (void)(l); (void)(o); (void)(g); (struct page *)NULL; })
#define fscrypt_zeroout_range(i, lb, pb, l)	({ (void)(i); (void)(lb); (void)(pb); (void)(l); 0; })
#define fscrypt_limit_io_blocks(i, lb, l)	(l)
#define fscrypt_prepare_setattr(d, a)		({ (void)(d); (void)(a); 0; })
#define fscrypt_dio_supported(i)		(1)
#define fscrypt_has_permitted_context(p, c)	({ (void)(p); (void)(c); 1; })
#define fscrypt_is_nokey_name(d)		({ (void)(d); 0; })
#define fscrypt_prepare_symlink(d, s, l, m, dl)	\
	({ (void)(d); (void)(m); (dl)->name = (unsigned char *)(s); (dl)->len = (l) + 1; 0; })
#define fscrypt_encrypt_symlink(i, s, l, d)	({ (void)(i); (void)(s); (void)(l); (void)(d); 0; })
#define fscrypt_prepare_link(o, d, n)		({ (void)(o); (void)(d); (void)(n); 0; })
#define fscrypt_prepare_rename(od, ode, nd, nde, f) \
	({ (void)(od); (void)(ode); (void)(nd); (void)(nde); (void)(f); 0; })

/* fscrypt directory operations */
#define fscrypt_prepare_readdir(i)		({ (void)(i); 0; })
#define fscrypt_fname_alloc_buffer(len, buf)	({ (void)(len); (void)(buf); 0; })
#define fscrypt_fname_free_buffer(buf)		do { (void)(buf); } while (0)
#define fscrypt_fname_disk_to_usr(i, h1, h2, d, u) \
	({ (void)(i); (void)(h1); (void)(h2); (void)(d); (void)(u); 0; })

/* fscrypt symlink stubs */
#define fscrypt_get_symlink(i, c, m, d)	({ (void)(i); (void)(c); (void)(m); (void)(d); ERR_PTR(-EOPNOTSUPP); })
#define fscrypt_symlink_getattr(p, s)	({ (void)(p); (void)(s); 0; })

/* fscrypt inode operations */
#define fscrypt_put_encryption_info(i)	do { } while (0)
#define fscrypt_parse_test_dummy_encryption(p, d) ({ (void)(p); (void)(d); 0; })

/* fscrypt page-io stubs */
#define fscrypt_is_bounce_folio(f)	({ (void)(f); 0; })
#define fscrypt_pagecache_folio(f)	(f)
#define fscrypt_free_bounce_page(p)	do { (void)(p); } while (0)
#define fscrypt_set_bio_crypt_ctx_bh(bio, bh, gfp) \
	do { (void)(bio); (void)(bh); (void)(gfp); } while (0)
#define fscrypt_mergeable_bio_bh(bio, bh) \
	({ (void)(bio); (void)(bh); true; })

/* fscrypt readpage stubs */
#define fscrypt_decrypt_bio(bio)	({ (void)(bio); 0; })
#define fscrypt_enqueue_decrypt_work(work) do { (void)(work); } while (0)
#define fscrypt_mergeable_bio(bio, inode, blk) \
	({ (void)(bio); (void)(inode); (void)(blk); true; })
#define fscrypt_set_bio_crypt_ctx(bio, inode, blk, gfp) \
	do { (void)(bio); (void)(inode); (void)(blk); (void)(gfp); } while (0)

/* fscrypt function declarations (implemented in stub.c) */
void fscrypt_free_dummy_policy(struct fscrypt_dummy_policy *policy);
int fscrypt_drop_inode(struct inode *inode);
void fscrypt_free_inode(struct inode *inode);
int fscrypt_is_dummy_policy_set(const struct fscrypt_dummy_policy *policy);
int fscrypt_dummy_policies_equal(const struct fscrypt_dummy_policy *p1,
				 const struct fscrypt_dummy_policy *p2);
void fscrypt_show_test_dummy_encryption(struct seq_file *seq, char sep,
					struct super_block *sb);

#endif /* _EXT4_FSCRYPT_H */
