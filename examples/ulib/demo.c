// SPDX-License-Identifier: GPL-2.0+
/*
 * Demo program showing U-Boot library functionality
 *
 * This demonstrates using U-Boot library functions from external programs
 * (sandbox) or as a standalone example linked into U-Boot.
 *
 * Copyright 2025 Canonical Ltd.
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <os.h>
#include <stdio.h>
#include <u-boot-lib.h>
#include <version_string.h>
#include "demo_helper.h"

#ifndef CONFIG_SANDBOX
bool ulib_has_main(void)
{
	return true;
}
#endif

static const char *get_version(void)
{
	if (IS_ENABLED(CONFIG_SANDBOX))
		return ulib_get_version();
	return version_string;
}

static int demo_run(void)
{
	demo_show_banner();
	printf("U-Boot version: %s\n", get_version());
	printf("\n");

	demo_add_numbers(42, 13);
	demo_show_footer();

	return 0;
}

#ifdef CONFIG_SANDBOX
int main(int argc, char *argv[])
{
	int fd, lines = 0;
	char line[256];
	int ret;

	if (ulib_init(argv[0]) < 0) {
		fprintf(stderr, "Failed to initialize U-Boot library\n");
		return 1;
	}

	ret = demo_run();

	/* Also demonstrate using U-Boot's os_* functions to read a file */
	fd = os_open("/proc/version", 0);
	if (fd >= 0) {
		printf("\nSystem version:\n");
		while (os_fgets(line, sizeof(line), fd)) {
			printf("  %s", line);
			lines++;
		}
		os_close(fd);
		printf("\nRead %d line(s) using U-Boot library functions.\n",
		       lines);
	}

	ulib_uninit();

	return ret;
}
#else
int main(void)
{
	return demo_run();
}
#endif
