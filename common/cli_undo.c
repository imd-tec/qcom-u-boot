// SPDX-License-Identifier: GPL-2.0+
/*
 * CLI undo/yank support
 *
 * Copyright 2025 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <cli.h>
#include <command.h>
#include <stdio.h>
#include <linux/string.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

#define CTL_BACKSPACE		('\b')

/**
 * cls_putch() - Output a character, using callback if available
 *
 * @cls: CLI line state
 * @ch: Character to output
 */
static void cls_putch(struct cli_line_state *cls, int ch)
{
	struct cli_editor_state *ed = cli_editor(cls);

	if (ed && ed->putch)
		ed->putch(cls, ch);
	else
		putc(ch);
}

static void cls_putnstr(struct cli_line_state *cls, const char *str, size_t n)
{
	while (n-- > 0)
		cls_putch(cls, *str++);
}

/**
 * cls_putchars() - Output a character multiple times
 *
 * @cls: CLI line state
 * @count: Number of times to output the character
 * @ch: Character to output
 */
static void cls_putchars(struct cli_line_state *cls, int count, int ch)
{
	int i;

	for (i = 0; i < count; i++)
		cls_putch(cls, ch);
}

#define getcmd_cbeep(cls)	cls_putch(cls, '\a')

void cread_save_undo(struct cli_line_state *cls)
{
	struct cli_editor_state *ed = cli_editor(cls);
	struct cli_undo_state *undo = &ed->undo;

	if (abuf_size(&undo->buf)) {
		memcpy(abuf_data(&undo->buf), cls->buf, cls->len);
		undo->num = cls->num;
		undo->eol_num = cls->eol_num;
	}
}

void cread_restore_undo(struct cli_line_state *cls)
{
	struct cli_editor_state *ed = cli_editor(cls);
	struct cli_undo_state *undo = &ed->undo;

	if (!abuf_size(&undo->buf))
		return;

	/* go to start of line */
	while (cls->num) {
		cls_putch(cls, CTL_BACKSPACE);
		cls->num--;
	}

	/* erase current content on screen */
	cls_putchars(cls, cls->eol_num, ' ');
	cls_putchars(cls, cls->eol_num, CTL_BACKSPACE);

	/* restore from undo buffer */
	memcpy(cls->buf, abuf_data(&undo->buf), cls->len);
	cls->eol_num = undo->eol_num;

	/* display restored content */
	cls_putnstr(cls, cls->buf, cls->eol_num);

	/* position cursor */
	cls_putchars(cls, cls->eol_num - undo->num, CTL_BACKSPACE);
	cls->num = undo->num;
}

void cread_save_yank(struct cli_line_state *cls, const char *text, uint len)
{
	struct cli_editor_state *ed = cli_editor(cls);

	if (abuf_size(&ed->yank) && len > 0 && len < cls->len) {
		memcpy(abuf_data(&ed->yank), text, len);
		ed->yank_len = len;
	}
}

void cread_yank(struct cli_line_state *cls)
{
	struct cli_editor_state *ed = cli_editor(cls);
	char *buf = cls->buf;
	uint i;

	if (!abuf_size(&ed->yank) || !ed->yank_len)
		return;

	/* check if there's room */
	if (cls->eol_num + ed->yank_len > cls->len - 1) {
		getcmd_cbeep(cls);
		return;
	}

	cread_save_undo(cls);

	/* make room for yanked text */
	memmove(&buf[cls->num + ed->yank_len], &buf[cls->num],
		cls->eol_num - cls->num + 1);

	/* insert yanked text */
	memcpy(&buf[cls->num], abuf_data(&ed->yank), ed->yank_len);
	cls->eol_num += ed->yank_len;

	/* display from cursor to end */
	cls_putnstr(cls, &buf[cls->num], cls->eol_num - cls->num);

	/* move cursor to end of inserted text */
	cls->num += ed->yank_len;
	for (i = cls->num; i < cls->eol_num; i++)
		cls_putch(cls, CTL_BACKSPACE);
}
