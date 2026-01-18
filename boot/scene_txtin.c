// SPDX-License-Identifier: GPL-2.0+
/*
 * Common code for text-input scene objects (textline, textedit)
 *
 * Copyright 2026 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#define LOG_CATEGORY	LOGC_EXPO

#include <expo.h>
#include <log.h>
#include <video_console.h>
#include <linux/errno.h>
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
	const bool open = obj->flags & SCENEOF_OPEN;
	struct udevice *cons = scn->expo->cons;
	uint i;

	/* if open, render the edit text on top of the background */
	if (open) {
		int ret;

		ret = vidconsole_entry_restore(cons, &scn->entry_save);
		if (ret)
			return log_msg_ret("sav", ret);
		scene_render_obj(scn, tin->edit_id);

		/* move cursor back to the correct position */
		for (i = scn->cls.num; i < scn->cls.eol_num; i++)
			vidconsole_put_char(cons, '\b');
		ret = vidconsole_entry_save(cons, &scn->entry_save);
		if (ret)
			return log_msg_ret("sav", ret);

		vidconsole_show_cursor(cons);
	}

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
