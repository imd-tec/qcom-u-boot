/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Backtrace support
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#ifndef __BACKTRACE_H
#define __BACKTRACE_H

#define BACKTRACE_MAX		100
#define BACKTRACE_SYM_SIZE	128
#define BACKTRACE_BUFSZ		(BACKTRACE_MAX * BACKTRACE_SYM_SIZE)

/**
 * struct backtrace_ctx - context for backtrace operations
 *
 * @addrs: array of return addresses
 * @syms: array of symbol strings (NULL until backtrace_get_syms() called)
 * @count: number of entries in addrs/syms arrays
 */
struct backtrace_ctx {
	void *addrs[BACKTRACE_MAX];
	char *syms[BACKTRACE_MAX];
	unsigned int count;
};

/**
 * backtrace_init() - collect a backtrace
 *
 * Collect backtrace addresses into the context. Call backtrace_uninit() when
 * done with the context.
 *
 * @ctx: context to fill
 * @skip: number of stack frames to skip (0 to include backtrace_init itself)
 * Return: number of addresses collected, or -ve on error (e.g. -ENOSYS)
 */
int backtrace_init(struct backtrace_ctx *ctx, unsigned int skip);

/**
 * backtrace_get_syms() - get symbol strings for a backtrace
 *
 * Convert the addresses in the context to symbol strings. The strings are
 * stored in ctx->syms[]. The caller must provide a buffer of sufficient size.
 *
 * @ctx: context with addresses from backtrace_init()
 * @buf: buffer to use for string storage
 * @size: size of buffer in bytes
 * Return: 0 if OK, -ENOSPC if buffer too small
 */
int backtrace_get_syms(struct backtrace_ctx *ctx, char *buf, int size);

/**
 * backtrace_uninit() - free backtrace resources
 *
 * Free any memory allocated in the context.
 *
 * @ctx: context to free
 */
void backtrace_uninit(struct backtrace_ctx *ctx);

/**
 * backtrace_show() - print a backtrace
 *
 * Print a backtrace of the current call stack.
 *
 * Return: 0 if OK, -ve on error
 */
int backtrace_show(void);

#endif /* __BACKTRACE_H */
