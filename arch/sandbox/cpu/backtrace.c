// SPDX-License-Identifier: GPL-2.0+
/*
 * OS-level backtrace support for sandbox
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#define _GNU_SOURCE

#include <backtrace.h>
#include <execinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <os.h>
/* For BACKTRACE_MAX_FRAMES - include U-Boot's header after system headers */
#include "../../../include/backtrace.h"

/* libbacktrace state - created once and cached */
static struct backtrace_state *bt_state;

/* Context for collecting symbol info */
struct bt_sym_ctx {
	char *buf;
	size_t size;
	int found;
};

uint os_backtrace(void **buffer, uint size, uint skip)
{
	void *tmp[size + skip];
	uint count;
	int nptrs;

	nptrs = backtrace(tmp, size + skip);
	if ((int)skip >= nptrs)
		return 0;

	count = nptrs - skip;
	memcpy(buffer, tmp + skip, count * sizeof(*buffer));

	return count;
}

static void bt_error_callback(void *data, const char *msg, int errnum)
{
	/* Silently ignore errors - we'll fall back to addresses */
}

static struct backtrace_state *get_bt_state(void)
{
	if (!bt_state)
		bt_state = backtrace_create_state(NULL, 0, bt_error_callback,
						  NULL);

	return bt_state;
}

static int bt_full_callback(void *data, uintptr_t pc, const char *fname,
			    int lineno, const char *func)
{
	struct bt_sym_ctx *ctx = data;

	if (func) {
		if (fname && lineno)
			snprintf(ctx->buf, ctx->size, "%s() at %s:%d", func,
				 fname, lineno);
		else if (fname)
			snprintf(ctx->buf, ctx->size, "%s() at %s", func,
				 fname);
		else
			snprintf(ctx->buf, ctx->size, "%s()", func);
		ctx->found = 1;
	}

	return 0;  /* continue to get innermost frame for inlined functions */
}

void os_backtrace_symbols(struct backtrace_ctx *ctx)
{
	char *end = ctx->sym_buf + BACKTRACE_SYM_BUFSZ;
	struct backtrace_state *state;
	char *p = ctx->sym_buf;
	int remaining, i;

	state = get_bt_state();

	for (i = 0; i < ctx->count; i++) {
		struct backtrace_frame *frame = &ctx->frame[i];
		struct bt_sym_ctx sym_ctx;

		remaining = end - p;
		if (remaining <= 1) {
			/* No more space, leave remaining syms as NULL */
			frame->sym = NULL;
			continue;
		}

		frame->sym = p;
		sym_ctx.buf = p;
		sym_ctx.size = remaining;
		sym_ctx.found = 0;

		if (state) {
			backtrace_pcinfo(state, (uintptr_t)frame->addr,
					 bt_full_callback, bt_error_callback,
					 &sym_ctx);
		}

		/* Fall back to address if no symbol found */
		if (!sym_ctx.found)
			snprintf(p, remaining, "%p", frame->addr);

		p += strlen(p) + 1;
	}
}
