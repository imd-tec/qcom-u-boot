// SPDX-License-Identifier: GPL-2.0+
/*
 * Implementation of a menu in a scene
 *
 * Copyright 2023 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#define LOG_CATEGORY	LOGC_EXPO

#include <expo.h>
#include <menu.h>
#include <log.h>
#include <video_console.h>
#include <linux/errno.h>
#include <linux/string.h>
#include "scene_internal.h"

int scene_textline(struct scene *scn, const char *name, uint id,
		   uint line_chars, struct scene_obj_textline **tlinep)
{
	struct scene_obj_textline *tline;
	int ret;

	if (line_chars >= EXPO_MAX_CHARS)
		return log_msg_ret("chr", -E2BIG);

	ret = scene_obj_add(scn, name, id, SCENEOBJT_TEXTLINE,
			    sizeof(struct scene_obj_textline),
			    (struct scene_obj **)&tline);
	if (ret < 0)
		return log_msg_ret("obj", -ENOMEM);
	ret = scene_txtin_init(&tline->tin, line_chars + 1, line_chars);
	if (ret)
		return log_msg_ret("tin", ret);
	tline->pos = line_chars;

	if (tlinep)
		*tlinep = tline;

	return tline->obj.id;
}

int scene_textline_calc_dims(struct scene_obj_textline *tline,
			     struct udevice *cons)
{
	struct scene *scn = tline->obj.scene;
	struct vidconsole_bbox bbox;
	struct scene_obj_txt *txt;
	int ret;

	txt = scene_obj_find(scn, tline->tin.edit_id, SCENEOBJT_NONE);
	if (!txt)
		return log_msg_ret("dim", -ENOENT);

	ret = vidconsole_nominal(cons, txt->gen.font_name, txt->gen.font_size,
				 tline->tin.line_chars, &bbox);
	if (ret)
		return log_msg_ret("nom", ret);

	if (bbox.valid) {
		struct scene_obj *obj = &txt->obj;

		obj->dims.x = bbox.x1 - bbox.x0;
		obj->dims.y = bbox.y1 - bbox.y0;
	}

	return 0;
}

int scene_textline_arrange(struct scene *scn, struct expo_arrange_info *arr,
			   struct scene_obj_textline *tline)
{
	struct scene_obj *edit;
	int x, y;
	int ret;

	x = scene_txtin_arrange(scn, arr, &tline->obj, &tline->tin);
	if (x < 0)
		return log_msg_ret("arr", x);

	y = tline->obj.req_bbox.y0;
	ret = scene_obj_set_pos(scn, tline->tin.edit_id, x, y);
	if (ret < 0)
		return log_msg_ret("pos", ret);

	edit = scene_obj_find(scn, tline->tin.edit_id, SCENEOBJT_NONE);
	if (!edit)
		return log_msg_ret("fnd", -ENOENT);
	x += edit->dims.x;
	y += edit->dims.y;

	tline->obj.dims.x = x - tline->obj.req_bbox.x0;
	tline->obj.dims.y = y - tline->obj.req_bbox.y0;
	scene_obj_set_size(scn, tline->obj.id, tline->obj.dims.x,
			   tline->obj.dims.y);

	return 0;
}

bool scene_textline_within(const struct scene *scn,
			   struct scene_obj_textline *tline, int x, int y)
{
	return scene_within(scn, tline->tin.edit_id, x, y);
}

