// SPDX-License-Identifier: GPL-2.0+
/*
 * Log driver to write to a file (sandbox only)
 *
 * Copyright 2026 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <env.h>
#include <log.h>
#include <os.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

static int log_fd = -1;

static void append(char **buf, char *buf_end, const char *fmt, ...)
{
	va_list args;
	size_t size = buf_end - *buf;

	va_start(args, fmt);
	vsnprintf(*buf, size, fmt, args);
	va_end(args);
	*buf += strlen(*buf);
}

int log_file_set_fname(const char *fname)
{
	if (log_fd != -1) {
		os_close(log_fd);
		log_fd = -1;
	}

	if (!fname)
		return 0;

	log_fd = os_open(fname, OS_O_WRONLY | OS_O_CREAT | OS_O_TRUNC);
	if (log_fd < 0)
		return log_fd;

	return 0;
}

static int log_file_emit(struct log_device *ldev, struct log_rec *rec)
{
	int fmt = gd->log_fmt;
	char buf[512];
	char *buf_end = buf + sizeof(buf);
	char *ptr = buf;
	const char *fname;
	int len;

	/* If no file open, try to open one from the environment */
	if (log_fd == -1) {
		fname = env_get("log_file");
		if (!fname)
			return 0;

		log_fd = os_open(fname, OS_O_WRONLY | OS_O_CREAT | OS_O_TRUNC);
		if (log_fd < 0)
			return 0;
	}

	/*
	 * The output format is designed to give someone a fighting chance of
	 * figuring out which field is which:
	 *    - level is in CAPS
	 *    - cat is lower case and ends with comma
	 *    - file normally has a .c extension and ends with a colon
	 *    - line is integer and ends with a -
	 *    - function is an identifier and ends with ()
	 *    - message has a space before it unless it is on its own
	 */
	if (!(rec->flags & LOGRECF_CONT) && fmt != BIT(LOGF_MSG)) {
		if (fmt & BIT(LOGF_LEVEL))
			append(&ptr, buf_end, "%s.",
			       log_get_level_name(rec->level));
		if (fmt & BIT(LOGF_CAT))
			append(&ptr, buf_end, "%s,",
			       log_get_cat_name(rec->cat));
		if (fmt & BIT(LOGF_FILE))
			append(&ptr, buf_end, "%s:", rec->file);
		if (fmt & BIT(LOGF_LINE))
			append(&ptr, buf_end, "%d-", rec->line);
		if (fmt & BIT(LOGF_FUNC))
			append(&ptr, buf_end, "%s() ", rec->func ?: "?");
	}
	if (fmt & BIT(LOGF_MSG))
		append(&ptr, buf_end, "%s", rec->msg);

	len = ptr - buf;
	os_write(log_fd, buf, len);

	return 0;
}

LOG_DRIVER(file) = {
	.name	= "file",
	.emit	= log_file_emit,
	.flags	= LOGDF_ENABLE,
};
