// SPDX-License-Identifier: GPL-2.0+
/*
 * CLI undo/redo/yank support
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

void cread_save_redo(struct cli_line_state *cls)
{
	struct cli_editor_state *ed = cli_editor(cls);
	struct cli_undo_state *redo = &ed->redo;
	struct cli_undo_pos *pos;
	uint idx;

	if (!redo->pos.alloc)
		return;

	/* save at current head position */
	idx = redo->head;
	pos = alist_getw(&redo->pos, idx, struct cli_undo_pos);
	memcpy(abuf_data(&pos->buf), cls->buf, cls->len);
	pos->num = cls->num;
	pos->eol_num = cls->eol_num;

	/* advance head (ring buffer) */
	redo->head = (redo->head + 1) % redo->pos.alloc;

	/* track how many redo levels are available */
	if (redo->count < redo->pos.alloc)
		redo->count++;
}

void cread_clear_redo(struct cli_line_state *cls)
{
	struct cli_editor_state *ed = cli_editor(cls);

	ed->redo.count = 0;
	ed->redo.head = 0;
}

void cread_save_undo(struct cli_line_state *cls)
{
	struct cli_editor_state *ed = cli_editor(cls);
	struct cli_undo_state *undo = &ed->undo;
	struct cli_undo_pos *pos;
	uint idx;

	if (!undo->pos.alloc)
		return;

	/* save at current head position */
	idx = undo->head;
	pos = alist_getw(&undo->pos, idx, struct cli_undo_pos);
	memcpy(abuf_data(&pos->buf), cls->buf, cls->len);
	pos->num = cls->num;
	pos->eol_num = cls->eol_num;

	/* advance head (ring buffer) */
	undo->head = (undo->head + 1) % undo->pos.alloc;

	/* track how many undo levels are available */
	if (undo->count < undo->pos.alloc)
		undo->count++;

	/* new edit invalidates redo history */
	cread_clear_redo(cls);
}

void cread_restore_undo(struct cli_line_state *cls)
{
	struct cli_undo_state *undo = &cli_editor(cls)->undo;
	const struct cli_undo_pos *pos;
	uint idx;

	if (!undo->pos.alloc || !undo->count)
		return;

	/* save current state to redo buffer before restoring */
	cread_save_redo(cls);

	/* move back to previous undo state */
	undo->head = undo->head ? undo->head - 1 : undo->pos.alloc - 1;
	undo->count--;
	idx = undo->head;

	/* go to start of line */
	while (cls->num) {
		cls_putch(cls, CTL_BACKSPACE);
		cls->num--;
	}

	/* erase current content on screen */
	cls_putchars(cls, cls->eol_num, ' ');
	cls_putchars(cls, cls->eol_num, CTL_BACKSPACE);

	/* restore from undo buffer */
	pos = alist_get(&undo->pos, idx, struct cli_undo_pos);
	memcpy(cls->buf, abuf_data(&pos->buf), cls->len);
	cls->eol_num = pos->eol_num;

	/* display restored content */
	cls_putnstr(cls, cls->buf, cls->eol_num);

	/* position cursor */
	cls_putchars(cls, cls->eol_num - pos->num, CTL_BACKSPACE);
	cls->num = pos->num;
}

void cread_redo(struct cli_line_state *cls)
{
	struct cli_editor_state *ed = cli_editor(cls);
	struct cli_undo_state *undo = &ed->undo;
	struct cli_undo_state *redo = &ed->redo;
	struct cli_undo_pos *pos;
	const struct cli_undo_pos *rpos;
	uint idx;

	if (!redo->pos.alloc || !redo->count)
		return;

	/* save current state to undo buffer */
	idx = undo->head;
	pos = alist_getw(&undo->pos, idx, struct cli_undo_pos);
	memcpy(abuf_data(&pos->buf), cls->buf, cls->len);
	pos->num = cls->num;
	pos->eol_num = cls->eol_num;
	undo->head = (undo->head + 1) % undo->pos.alloc;
	if (undo->count < undo->pos.alloc)
		undo->count++;

	/* move back to previous redo state */
	redo->head = redo->head ? redo->head - 1 : redo->pos.alloc - 1;
	redo->count--;
	idx = redo->head;

	/* go to start of line */
	while (cls->num) {
		cls_putch(cls, CTL_BACKSPACE);
		cls->num--;
	}

	/* erase current content on screen */
	cls_putchars(cls, cls->eol_num, ' ');
	cls_putchars(cls, cls->eol_num, CTL_BACKSPACE);

	/* restore from redo buffer */
	rpos = alist_get(&redo->pos, idx, struct cli_undo_pos);
	memcpy(cls->buf, abuf_data(&rpos->buf), cls->len);
	cls->eol_num = rpos->eol_num;

	/* display restored content */
	cls_putnstr(cls, cls->buf, cls->eol_num);

	/* position cursor */
	cls_putchars(cls, cls->eol_num - rpos->num, CTL_BACKSPACE);
	cls->num = rpos->num;
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
