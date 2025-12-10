// SPDX-License-Identifier: GPL-2.0+
/*
 * Stack-backtrace support
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <backtrace.h>
#include <stdbool.h>
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

/**
 * extract_func_info() - extract function name and line number from a symbol
 *
 * Parse a backtrace symbol string and extract function name with line number.
 * The format is typically "func_name() at /path/to/file.c:line" or similar.
 *
 * @sym: symbol string from backtrace
 * @buf: buffer to write "func_name:line" to
 * @size: size of buffer
 * Return: pointer to buf, or NULL if extraction failed
 */
static char *extract_func_info(const char *sym, char *buf, int size)
{
	const char *start, *end, *colon;
	int len;

	if (!sym)
		return NULL;

	/*
	 * Skip leading whitespace and any address prefix (e.g. "0x12345678 ")
	 * Look for the function name which ends at '+' or '(' or ' '
	 */
	start = sym;
	while (*start == ' ')
		start++;

	/* Skip hex address if present */
	if (start[0] == '0' && start[1] == 'x') {
		while (*start && *start != ' ')
			start++;
		while (*start == ' ')
			start++;
	}

	/* Find end of function name */
	end = start;
	while (*end && *end != '+' && *end != '(' && *end != ' ')
		end++;

	len = end - start;
	if (len <= 0 || len >= size)
		return NULL;

	memcpy(buf, start, len);

	/* Look for line number after last colon (file:line format) */
	colon = strrchr(sym, ':');
	if (colon && colon[1] >= '0' && colon[1] <= '9') {
		buf[len++] = ':';
		colon++;
		/* Copy digits */
		while (*colon >= '0' && *colon <= '9' && len < size - 1)
			buf[len++] = *colon++;
	}
	buf[len] = '\0';

	return buf;
}

char *backtrace_strf(unsigned int skip, char *buf, int size)
{
	static struct backtrace_ctx ctx;
	int remaining = size;
	bool first = true;
	char func[64];
	char *p = buf;
	uint i, count;
	int ret, len;

	/* skip + 1 to skip backtrace_strf() */
	ret = backtrace_init(&ctx, skip + 1);
	if (ret < 0)
		return NULL;

	ret = backtrace_get_syms(&ctx, NULL, 0);
	if (ret) {
		backtrace_uninit(&ctx);
		return NULL;
	}

	count = ctx.count;
	if (count > CONFIG_BACKTRACE_SUMMARY_FRAMES)
		count = CONFIG_BACKTRACE_SUMMARY_FRAMES;

	for (i = 0; i < count; i++) {
		if (!extract_func_info(ctx.frame[i].sym, func, sizeof(func)))
			continue;

		if (!first) {
			if (remaining < 4)
				break;
			*p++ = ' ';
			*p++ = '<';
			*p++ = '-';
			remaining -= 3;
		}
		first = false;

		len = strlen(func);
		if (len >= remaining)
			break;
		memcpy(p, func, len);
		p += len;
		remaining -= len;
	}
	*p = '\0';

	backtrace_uninit(&ctx);

	return buf;
}

const char *backtrace_str(unsigned int skip)
{
	static char result[CONFIG_BACKTRACE_SUMMARY_FRAMES * 64];

	/* skip + 1 to account for this wrapper function */
	return backtrace_strf(skip + 1, result, sizeof(result));
}
