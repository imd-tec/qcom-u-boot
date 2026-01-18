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
