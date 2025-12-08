// SPDX-License-Identifier: GPL-2.0+
/*
 * Stack-backtrace support
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <backtrace.h>
#include <stdio.h>
#include <string.h>

static void print_sym(const char *sym)
{
	const char *p;

	/* Look for SRCTREE prefix in the string and skip it */
	p = strstr(sym, SRCTREE);
	if (p) {
		/* Print part before SRCTREE, then the rest after SRCTREE */
		printf("  %.*s%s\n", (int)(p - sym), sym, p + strlen(SRCTREE));
	} else {
		printf("  %s\n", sym);
	}
}

int backtrace_show(void)
{
	static struct backtrace_ctx ctx;
	uint i;
	int ret;

	ret = backtrace_init(&ctx, 1);
	if (ret < 0)
		return ret;

	ret = backtrace_get_syms(&ctx, NULL, 0);
	if (ret) {
		backtrace_uninit(&ctx);
		return ret;
	}

	printf("backtrace: %d addresses\n", ctx.count);
	for (i = 0; i < ctx.count; i++) {
		const struct backtrace_frame *frame = &ctx.frame[i];

		if (frame->sym)
			print_sym(frame->sym);
		else
			printf("  %p\n", frame->addr);
	}

	backtrace_uninit(&ctx);

	return 0;
}
