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
#include "scene_internal.h"

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
