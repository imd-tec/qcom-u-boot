/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mount ID mapping definitions for U-Boot
 *
 * Based on Linux mnt_idmapping.h - user ID mapping for mounts.
 * U-Boot stub - ID mapping not supported.
 */
#ifndef _LINUX_MNT_IDMAPPING_H
#define _LINUX_MNT_IDMAPPING_H

/**
 * struct mnt_idmap - mount ID mapping
 *
 * U-Boot stub - ID mapping not used.
 */
struct mnt_idmap {
	int dummy;
};

/* Global no-op ID map */
extern struct mnt_idmap nop_mnt_idmap;

#endif /* _LINUX_MNT_IDMAPPING_H */
