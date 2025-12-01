// SPDX-License-Identifier: GPL-2.0+
/*
 * Backtrace support for sandbox
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <backtrace.h>
#include <errno.h>
#include <os.h>
#include <string.h>

int backtrace_init(struct backtrace_ctx *ctx, uint skip)
{
	uint i;

	for (i = 0; i < BACKTRACE_MAX; i++)
		ctx->syms[i] = NULL;
	/* +1 to skip this function */
	ctx->count = os_backtrace(ctx->addrs, BACKTRACE_MAX, skip + 1);

	return ctx->count;
}

int backtrace_get_syms(struct backtrace_ctx *ctx, char *buf, int size)
{
	char **raw_syms;
	size_t total_len;
	char *p;
	uint i;

	raw_syms = os_backtrace_symbols(ctx->addrs, ctx->count);
	if (!raw_syms)
		return -ENOMEM;

	/* Calculate total buffer size needed */
	total_len = 0;
	for (i = 0; i < ctx->count; i++) {
		if (raw_syms[i])
			total_len += strlen(raw_syms[i]) + 1;
		else
			total_len += 1;  /* empty string */
	}

	if ((size_t)size < total_len) {
		os_backtrace_symbols_free(raw_syms);
		return -ENOSPC;
	}

	/* Copy strings into buffer */
	p = buf;
	for (i = 0; i < ctx->count; i++) {
		ctx->syms[i] = p;
		if (raw_syms[i]) {
			strcpy(p, raw_syms[i]);
			p += strlen(raw_syms[i]) + 1;
		} else {
			*p++ = '\0';
		}
	}

	os_backtrace_symbols_free(raw_syms);

	return 0;
}

void backtrace_uninit(struct backtrace_ctx *ctx)
{
	/* Nothing to free - caller owns the buffer */
}
