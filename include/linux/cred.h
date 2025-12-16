/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Credentials management - see Documentation/security/credentials.rst
 *
 * Copyright (C) 2008 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */
#ifndef _LINUX_CRED_H
#define _LINUX_CRED_H

#include <linux/types.h>

/*
 * Stub definitions for Linux kernel credentials.
 * U-Boot doesn't implement user credentials.
 */

typedef struct {
	uid_t val;
} kuid_t;

typedef struct {
	gid_t val;
} kgid_t;

struct cred {
	kuid_t uid;
	kgid_t gid;
	kuid_t fsuid;
	kgid_t fsgid;
};

#define current_cred()		NULL
#define current_uid()		((kuid_t){0})
#define current_gid()		((kgid_t){0})
#define current_fsuid()		((kuid_t){0})
#define current_fsgid()		((kgid_t){0})

#define from_kuid(ns, uid)	((uid).val)
#define from_kgid(ns, gid)	((gid).val)
#define make_kuid(ns, uid)	((kuid_t){uid})
#define make_kgid(ns, gid)	((kgid_t){gid})

#define uid_eq(a, b)		((a).val == (b).val)
#define gid_eq(a, b)		((a).val == (b).val)
#define uid_valid(uid)		((uid).val != (uid_t)-1)
#define gid_valid(gid)		((gid).val != (gid_t)-1)

#define GLOBAL_ROOT_UID		((kuid_t){0})
#define GLOBAL_ROOT_GID		((kgid_t){0})
#define INVALID_UID		((kuid_t){-1})
#define INVALID_GID		((kgid_t){-1})

#endif /* _LINUX_CRED_H */
