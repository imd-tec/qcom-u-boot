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
#include <menu.h>
#include <video_console.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include "scene_internal.h"

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

int scene_txtin_render_deps(struct scene *scn, struct scene_obj *obj,
			    struct scene_txtin *tin)
{
	struct cli_line_state *cls = &tin->cls;
	const bool open = obj->flags & SCENEOF_OPEN;
	struct udevice *cons = scn->expo->cons;
	void *ctx = tin->ctx;
	uint i;

	/* if open, render the edit text on top of the background */
	if (open) {
		scene_render_obj(scn, tin->edit_id, ctx);

		/* move cursor back to the correct position */
		for (i = cls->num; i < cls->eol_num; i++)
			vidconsole_put_char(cons, ctx, '\b');

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
	/* cursor is not needed now */
	vidconsole_readline_end(scn->expo->cons, tin->ctx);
}

int scene_txtin_open(struct scene *scn, struct scene_obj *obj,
		     struct scene_txtin *tin)
{
	struct cli_line_state *cls = &tin->cls;
	struct udevice *cons = scn->expo->cons;
	struct scene_obj_txt *txt;
	void *ctx;
	int ret;

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
	cli_cread_init(cls, abuf_data(&tin->buf), abuf_size(&tin->buf));
	cls->insert = true;
	cls->putch = scene_txtin_putch;
	cls->priv = scn;
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
	case BKEY_SELECT:
		if (!open)
			break;
		event->type = EXPOACT_CLOSE;
		event->select.id = obj->id;
		scene_txtin_close(scn, tin);
		break;
	default:
		cread_line_process_ch(cls, key);
		break;
	}

	return 0;
}
