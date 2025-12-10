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
	void *addrs[BACKTRACE_MAX_FRAMES];
	uint i;

	/* +1 to skip this function */
	ctx->count = os_backtrace(addrs, BACKTRACE_MAX_FRAMES, skip + 1);

	for (i = 0; i < ctx->count; i++) {
		ctx->frame[i].addr = addrs[i];
		ctx->frame[i].sym = NULL;
	}

	return ctx->count;
}

int backtrace_get_syms(struct backtrace_ctx *ctx, char *buf, int size)
{
	os_backtrace_symbols(ctx);

	return 0;
}

void backtrace_uninit(struct backtrace_ctx *ctx)
{
	/* Nothing to free - caller owns the buffer */
}
