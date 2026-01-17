// SPDX-License-Identifier: GPL-2.0+
/*
 * Implementation of a menu in a scene
 *
 * Copyright 2025 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#define LOG_CATEGORY	LOGC_EXPO

#include <expo.h>
#include <log.h>
#include <linux/err.h>
#include <linux/sizes.h>
#include "scene_internal.h"

enum {
	INITIAL_SIZE	= SZ_4K,
};

int scene_texted(struct scene *scn, const char *name, uint id,
		 uint line_chars, struct scene_obj_txtedit **teditp)
{
	struct scene_obj_txtedit *ted;
	char *buf;
	int ret;

	ret = scene_obj_add(scn, name, id, SCENEOBJT_TEXTEDIT,
			    sizeof(struct scene_obj_txtedit),
			    (struct scene_obj **)&ted);
	if (ret < 0)
		return log_msg_ret("obj", ret);

	abuf_init(&ted->tin.buf);
	if (!abuf_realloc(&ted->tin.buf, INITIAL_SIZE))
		return log_msg_ret("buf", -ENOMEM);
	buf = abuf_data(&ted->tin.buf);
	*buf = '\0';
	ted->tin.line_chars = line_chars;

	if (teditp)
		*teditp = ted;

	return ted->obj.id;
}

int scene_txted_set_font(struct scene *scn, uint id, const char *font_name,
			 uint font_size)
{
	struct scene_obj_txtedit *ted;

	ted = scene_obj_find(scn, id, SCENEOBJT_TEXTEDIT);
	if (!ted)
		return log_msg_ret("find", -ENOENT);

	return scene_txt_set_font(scn, ted->tin.edit_id, font_name, font_size);
}

int scene_txted_arrange(struct scene *scn, struct expo_arrange_info *arr,
			struct scene_obj_txtedit *ted)
{
	const bool open = ted->obj.flags & SCENEOF_OPEN;
	const struct expo_theme *theme = &scn->expo->theme;
	bool point;
	int x, y;
	int ret;

	x = ted->obj.req_bbox.x0;
	y = ted->obj.req_bbox.y0;
	if (ted->tin.label_id) {
		ret = scene_obj_set_pos(scn, ted->tin.label_id, x, y);
		if (ret < 0)
			return log_msg_ret("tit", ret);

		x += arr->label_width + theme->textline_label_margin_x;
	}

	/* constrain the edit text to fit within the textedit bbox */
	ret = scene_obj_set_bbox(scn, ted->tin.edit_id, x, y,
				 ted->obj.req_bbox.x1, ted->obj.req_bbox.y1);
	if (ret < 0)
		return log_msg_ret("edi", ret);

	point = scn->highlight_id == ted->obj.id;
	point &= !open;
	scene_obj_flag_clrset(scn, ted->tin.edit_id, SCENEOF_POINT,
			      point ? SCENEOF_POINT : 0);

	ted->obj.dims.x = x - ted->obj.req_bbox.x0;
	ted->obj.dims.y = y - ted->obj.req_bbox.y0;
	scene_obj_set_size(scn, ted->obj.id, ted->obj.dims.x, ted->obj.dims.y);

	return 0;
}
