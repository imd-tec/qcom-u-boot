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
	int ret;

	ret = scene_obj_add(scn, name, id, SCENEOBJT_TEXTEDIT,
			    sizeof(struct scene_obj_txtedit),
			    (struct scene_obj **)&ted);
	if (ret < 0)
		return log_msg_ret("obj", ret);

	ret = scene_txtin_init(&ted->tin, INITIAL_SIZE, line_chars);
	if (ret)
		return log_msg_ret("tin", ret);

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

int scene_txted_calc_dims(struct scene_obj_txtedit *ted, struct udevice *cons)
{
	struct scene *scn = ted->obj.scene;
	struct scene_obj_txt *txt;
	int ret;

	txt = scene_obj_find(scn, ted->tin.edit_id, SCENEOBJT_NONE);
	if (!txt)
		return log_msg_ret("txt", -ENOENT);

	/*
	 * Set the edit text's bbox to match the textedit's bbox. This ensures
	 * SCENEOF_SIZE_VALID is set so vidconsole_measure() applies the width
	 * limit for word-wrapping/clipping.
	 */
	ret = scene_obj_set_bbox(scn, ted->tin.edit_id,
				 ted->obj.req_bbox.x0, ted->obj.req_bbox.y0,
				 ted->obj.req_bbox.x1, ted->obj.req_bbox.y1);
	if (ret < 0)
		return log_msg_ret("sbb", ret);

	/* Measure the edit text now that its bbox is set correctly */
	ret = scene_obj_get_hw(scn, ted->tin.edit_id, NULL);
	if (ret < 0)
		return log_msg_ret("hw", ret);

	return 0;
}

int scene_txted_arrange(struct scene *scn, struct expo_arrange_info *arr,
			struct scene_obj_txtedit *ted)
{
	int x;
	int ret;

	x = scene_txtin_arrange(scn, arr, &ted->obj, &ted->tin);
	if (x < 0)
		return log_msg_ret("arr", x);

	/* constrain the edit text to fit within the textedit bbox */
	ret = scene_obj_set_bbox(scn, ted->tin.edit_id, x, ted->obj.req_bbox.y0,
				 ted->obj.req_bbox.x1, ted->obj.req_bbox.y1);
	if (ret < 0)
		return log_msg_ret("edi", ret);

	ted->obj.dims.x = x - ted->obj.req_bbox.x0;
	ted->obj.dims.y = 0;
	scene_obj_set_size(scn, ted->obj.id, ted->obj.dims.x, ted->obj.dims.y);

	return 0;
}
