// SPDX-License-Identifier: GPL-2.0+
/*
 * Common code for text-input scene objects (textline, textedit)
 *
 * Copyright 2026 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#define LOG_CATEGORY	LOGC_EXPO

#include <cli.h>
#include <expo.h>
#include <log.h>
#include <malloc.h>
#include <menu.h>
#include <video_console.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include "scene_internal.h"

#ifdef CONFIG_CMDLINE_UNDO_COUNT
#define UNDO_COUNT	CONFIG_CMDLINE_UNDO_COUNT
#else
#define UNDO_COUNT	64
#endif

int scene_txtin_init(struct scene_txtin *tin, uint size, uint line_chars)
{
	char *buf;

	if (!abuf_init_size(&tin->buf, size))
		return log_msg_ret("buf", -ENOMEM);
	buf = abuf_data(&tin->buf);
	*buf = '\0';
	tin->line_chars = line_chars;

	return 0;
}

void scene_txtin_destroy(struct scene *scn, struct scene_txtin *tin)
{
	abuf_uninit(&tin->buf);
	if (tin->ctx && scn->expo->cons)
		vidconsole_ctx_dispose(scn->expo->cons, tin->ctx);
}

int scene_txtin_arrange(struct scene *scn, struct expo_arrange_info *arr,
			struct scene_obj *obj, struct scene_txtin *tin)
{
	const bool open = obj->flags & SCENEOF_OPEN;
	const struct expo_theme *theme = &scn->expo->theme;
	bool point;
	int x;
	int ret;

	x = obj->req_bbox.x0;
	if (tin->label_id) {
		ret = scene_obj_set_pos(scn, tin->label_id, x, obj->req_bbox.y0);
		if (ret < 0)
			return log_msg_ret("lab", ret);

		if (scene_chklog(obj->name))
			log_debug("arr->label_width %d margin %d\n",
				  arr->label_width,
				  theme->textline_label_margin_x);
		x += arr->label_width + theme->textline_label_margin_x;
	}

	point = scn->highlight_id == obj->id;
	point &= !open;
	scene_obj_flag_clrset(scn, tin->edit_id, SCENEOF_POINT,
			      point ? SCENEOF_POINT : 0);

	return x;
}

/**
 * set_cursor_pos() - Set cursor position for multiline text
 *
 * Finds the visual line containing the cursor and sets the cursor position
 * to the correct pixel location within that line.
 *
 * @cons: Vidconsole device
 * @ctx: Vidconsole context
 * @txt: Text object containing line measurement info
 * @buf: Text buffer
 * @pos: Cursor position in buffer
 */
static void set_cursor_pos(struct udevice *cons, void *ctx,
			   struct scene_obj_txt *txt, const char *buf, uint pos)
{
	const struct vidconsole_mline *mline, *res;
	struct vidconsole_bbox bbox;
	struct alist lines;
	uint i;

	alist_init_struct(&lines, struct vidconsole_mline);
	for (i = 0; i < txt->gen.lines.count; i++) {
		mline = alist_get(&txt->gen.lines, i, struct vidconsole_mline);
		if (pos < mline->start || pos > mline->start + mline->len)
			continue;
		if (vidconsole_measure(cons, txt->gen.font_name,
				       txt->gen.font_size, buf + mline->start,
				       pos - mline->start, -1, &bbox, &lines))
			break;
		/* measured text is within a single line, so only one result */
		res = alist_get(&lines, 0, struct vidconsole_mline);
		if (!res)
			break;
		vidconsole_set_cursor_pos(cons, ctx, txt->obj.bbox.x0 + res->xpos,
					  txt->obj.bbox.y0 + mline->bbox.y0);
		break;
	}
	alist_uninit(&lines);
}

int scene_txtin_render_deps(struct scene *scn, struct scene_obj *obj,
			    struct scene_txtin *tin)
{
	struct cli_line_state *cls = &tin->cls;
	struct cli_editor_state *ed = cli_editor(cls);
	const bool open = obj->flags & SCENEOF_OPEN;
	struct udevice *cons = scn->expo->cons;
	void *ctx = tin->ctx;
	uint i;

	/* if open, render the edit text on top of the background */
	if (open) {
		scene_render_obj(scn, tin->edit_id, ctx);

		if (ed->multiline) {
			/* for multiline, set cursor position directly */
			struct scene_obj_txt *txt;

			txt = scene_obj_find(scn, tin->edit_id, SCENEOBJT_NONE);
			if (txt)
				set_cursor_pos(cons, ctx, txt, cls->buf,
					       cls->num);
		} else {
			/* for single-line, use backspaces */
			for (i = cls->num; i < cls->eol_num; i++)
				vidconsole_put_char(cons, ctx, '\b');
		}

		vidconsole_show_cursor(cons, ctx);
	}

	return 0;
}

/**
 * scene_txtin_putch() - Output a character to the vidconsole
 *
 * This is used as the putch callback for CLI line editing, so that characters
 * are sent to the correct vidconsole.
 *
 * @cls: CLI line state
 * @ch: Character to output
 */
static void scene_txtin_putch(struct cli_line_state *cls, int ch)
{
	struct scene_txtin *tin = container_of(cls, struct scene_txtin, cls);
	struct scene *scn = cls->priv;

	vidconsole_put_char(scn->expo->cons, tin->ctx, ch);
}

void scene_txtin_close(struct scene *scn, struct scene_txtin *tin)
{
	struct cli_line_state *cls = &tin->cls;

	cli_cread_uninit(cls);

	/* cursor is not needed now */
	vidconsole_readline_end(scn->expo->cons, tin->ctx);
}

/**
 * scene_txtin_line_nav() - Navigate to previous/next line in multi-line input
 *
 * Moves the cursor to the previous or next line, trying to maintain the same
 * horizontal pixel position. Uses the text measurement info attached to the
 * edit text object.
 *
 * @cls: CLI line state
 * @up: true to move to previous line, false for next line
 * Return: New cursor position, or -ve if at boundary
 */
static int scene_txtin_line_nav(struct cli_line_state *cls, bool up)
{
	struct scene_txtin *tin = container_of(cls, struct scene_txtin, cls);
	struct scene *scn = cls->priv;
	struct scene_obj_txt *txt;
	const struct vidconsole_mline *mline;
	const struct vidconsole_mline *target;
	struct vidconsole_bbox bbox;
	uint pos = cls->num;
	int cur_line, target_line;
	int target_x, best_pos, best_diff;
	int i, ret;

	txt = scene_obj_find(scn, tin->edit_id, SCENEOBJT_NONE);
	if (!txt || !txt->gen.lines.count)
		return -ENOENT;

	/* find which line the cursor is on */
	cur_line = -1;
	for (i = 0; i < txt->gen.lines.count; i++) {
		mline = alist_get(&txt->gen.lines, i, struct vidconsole_mline);
		if (pos >= mline->start && pos <= mline->start + mline->len) {
			cur_line = i;
			break;
		}
	}
	if (cur_line < 0)
		return -EINVAL;

	/* find target line */
	target_line = up ? cur_line - 1 : cur_line + 1;
	if (target_line < 0 || target_line >= txt->gen.lines.count)
		return -EINVAL;

	/* measure text from line start to cursor to get x position */
	ret = vidconsole_measure(scn->expo->cons, txt->gen.font_name,
				 txt->gen.font_size, cls->buf + mline->start,
				 pos - mline->start, -1, &bbox, NULL);
	if (ret)
		return ret;
	target_x = bbox.x1;

	/* find character position on target line closest to target_x */
	target = alist_get(&txt->gen.lines, target_line, struct vidconsole_mline);
	best_pos = target->start;
	best_diff = target_x;  /* diff from position 0 */

	for (i = 1; i <= target->len; i++) {
		int diff;

		ret = vidconsole_measure(scn->expo->cons, txt->gen.font_name,
					 txt->gen.font_size,
					 cls->buf + target->start, i, -1,
					 &bbox, NULL);
		if (ret)
			break;
		diff = abs(bbox.x1 - target_x);
		if (diff < best_diff) {
			best_diff = diff;
			best_pos = target->start + i;
		}
		/* stop if we've gone past the target */
		if (bbox.x1 > target_x)
			break;
	}

	/* measure text to best_pos to get x coordinate for cursor */
	ret = vidconsole_measure(scn->expo->cons, txt->gen.font_name,
				 txt->gen.font_size, cls->buf + target->start,
				 best_pos - target->start, -1, &bbox, NULL);
	if (ret)
		return ret;

	/* set cursor position: text object position + line offset + char offset */
	vidconsole_set_cursor_pos(scn->expo->cons, tin->ctx,
				  txt->obj.bbox.x0 + bbox.x1,
				  txt->obj.bbox.y0 + target->bbox.y0);

	return best_pos;
}

int scene_txtin_open(struct scene *scn, struct scene_obj *obj,
		     struct scene_txtin *tin)
{
	struct cli_line_state *cls = &tin->cls;
	struct cli_editor_state *ed = cli_editor(cls);
	struct udevice *cons = scn->expo->cons;
	struct scene_obj_txt *txt;
	void *ctx;
	int ret, i;

	ctx = tin->ctx;
	if (!ctx) {
		ret = vidconsole_ctx_new(cons, &ctx);
		if (ret)
			return log_msg_ret("ctx", ret);
		tin->ctx = ctx;
	}

	/* Copy the text into the scene buffer in case the edit is cancelled */
	memcpy(abuf_data(&scn->buf), abuf_data(&tin->buf),
	       abuf_size(&scn->buf));

	/* get the position of the editable */
	txt = scene_obj_find(scn, tin->edit_id, SCENEOBJT_NONE);
	if (!txt)
		return log_msg_ret("cur", -ENOENT);

	vidconsole_set_cursor_pos(cons, ctx, txt->obj.bbox.x0, txt->obj.bbox.y0);
	vidconsole_entry_start(cons, ctx);
	cli_cread_init_undo(cls, abuf_data(&tin->buf), abuf_size(&tin->buf));
	cls->insert = true;
	ed->putch = scene_txtin_putch;
	cls->priv = scn;

	/* Initialise undo ring buffer */
	alist_init_struct(&ed->undo.pos, struct cli_undo_pos);
	for (i = 0; i < UNDO_COUNT; i++) {
		struct cli_undo_pos *pos;

		pos = alist_ensure(&ed->undo.pos, i, struct cli_undo_pos);
		abuf_init_size(&pos->buf, abuf_size(&tin->buf));
	}

	/* Initialise redo ring buffer */
	alist_init_struct(&ed->redo.pos, struct cli_undo_pos);
	for (i = 0; i < UNDO_COUNT; i++) {
		struct cli_undo_pos *pos;

		pos = alist_ensure(&ed->redo.pos, i, struct cli_undo_pos);
		abuf_init_size(&pos->buf, abuf_size(&tin->buf));
	}

	/* yank buffer is initialised by cli_cread_init_undo() above */

	if (obj->type == SCENEOBJT_TEXTEDIT) {
		ed->multiline = true;
		ed->line_nav = scene_txtin_line_nav;
	}
	cli_cread_add_initial(cls);

	/* make sure the cursor is visible */
	vidconsole_readline_start(cons, ctx, true);

	return 0;
}

void scene_txtin_calc_bbox(struct scene_obj *obj, struct scene_txtin *tin,
			   struct vidconsole_bbox *bbox,
			   struct vidconsole_bbox *edit_bbox)
{
	struct scene *scn = obj->scene;
	const struct expo_theme *theme = &scn->expo->theme;
	int inset = theme->menu_inset;

	bbox->valid = false;
	scene_bbox_union(scn, tin->label_id, inset, bbox);
	scene_bbox_union(scn, tin->edit_id, inset, bbox);

	edit_bbox->valid = false;
	scene_bbox_union(scn, tin->edit_id, inset, edit_bbox);
}

int scene_txtin_send_key(struct scene_obj *obj, struct scene_txtin *tin,
			 int key, struct expo_action *event)
{
	struct cli_line_state *cls = &tin->cls;
	const bool open = obj->flags & SCENEOF_OPEN;
	struct scene *scn = obj->scene;

	log_debug("key=%d\n", key);
	switch (key) {
	case BKEY_QUIT:
		if (open) {
			event->type = EXPOACT_CLOSE;
			event->select.id = obj->id;

			/* Copy the backup text from the scene buffer */
			memcpy(abuf_data(&tin->buf), abuf_data(&scn->buf),
			       abuf_size(&scn->buf));

			scene_txtin_close(scn, tin);
		} else {
			event->type = EXPOACT_QUIT;
			log_debug("menu quit\n");
		}
		break;
	case BKEY_SAVE:
		if (!open)
			break;
		/* Accept contents even in multiline mode */
		event->type = EXPOACT_CLOSE;
		event->select.id = obj->id;
		scene_txtin_close(scn, tin);
		break;
	case BKEY_SELECT:
		if (!open)
			break;
		if (obj->flags & SCENEOF_MULTILINE) {
			char *buf = cls->buf;
			int wlen = cls->eol_num - cls->num;

			/* Insert newline at cursor position */
			memmove(&buf[cls->num + 1], &buf[cls->num], wlen);
			buf[cls->num] = '\n';
			cls->num++;
			cls->eol_num++;
		} else {
			event->type = EXPOACT_CLOSE;
			event->select.id = obj->id;
			scene_txtin_close(scn, tin);
		}
		break;
	case BKEY_UP:
		cread_line_process_ch(cls, CTL_CH('p'));
		break;
	case BKEY_DOWN:
		cread_line_process_ch(cls, CTL_CH('n'));
		break;
	default:
		cread_line_process_ch(cls, key);
		break;
	}

	return 0;
}
