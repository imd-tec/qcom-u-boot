// SPDX-License-Identifier: GPL-2.0+
/*
 * Stack-backtrace support
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <backtrace.h>
#include <stdio.h>

int backtrace_show(void)
{
	char buf[BACKTRACE_BUFSZ];
	struct backtrace_ctx ctx;
	uint i;
	int ret;

	ret = backtrace_init(&ctx, 1);
	if (ret < 0)
		return ret;

	ret = backtrace_get_syms(&ctx, buf, sizeof(buf));
	if (ret) {
		backtrace_uninit(&ctx);
		return ret;
	}

	printf("backtrace: %d addresses\n", ctx.count);
	for (i = 0; i < ctx.count; i++) {
		if (ctx.syms[i])
			printf("  %s\n", ctx.syms[i]);
		else
			printf("  %p\n", ctx.addrs[i]);
	}

	backtrace_uninit(&ctx);

	return 0;
}
