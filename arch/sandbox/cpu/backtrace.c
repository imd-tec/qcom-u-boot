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

char **os_backtrace_symbols(void *const *buffer, uint count)
{
	struct backtrace_state *state;
	char *str_storage;
	char **strings;
	uint i;

	state = get_bt_state();

	/* Allocate array of string pointers plus space for strings */
	strings = malloc(count * sizeof(char *) + count * 256);
	if (!strings)
		return NULL;

	/* String storage starts after the pointer array */
	str_storage = (char *)(strings + count);

	for (i = 0; i < count; i++) {
		struct bt_sym_ctx ctx;

		strings[i] = str_storage + i * 256;
		ctx.buf = strings[i];
		ctx.size = 256;
		ctx.found = 0;

		if (state) {
			backtrace_pcinfo(state, (uintptr_t)buffer[i],
					 bt_full_callback, bt_error_callback,
					 &ctx);
		}

		/* Fall back to address if no symbol found */
		if (!ctx.found)
			snprintf(strings[i], 256, "%p", buffer[i]);
	}

	return strings;
}

void os_backtrace_symbols_free(char **strings)
{
	free(strings);
}
