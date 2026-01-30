// SPDX-License-Identifier: GPL-2.0+
/*
 * Expo-based environment variable editor
 *
 * Copyright 2025 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <dm.h>
#include <expo.h>
#include <video.h>
#include <video_console.h>

/* IDs for expo objects */
enum {
	EDITENV_SCENE = EXPOID_BASE_ID + 1,
	EDITENV_OBJ_TEXTEDIT,
	EDITENV_OBJ_LABEL,
	EDITENV_OBJ_EDIT,
};

static int editenv_setup(struct expo *exp, struct udevice *dev,
			 const char *varname, const char *value,
			 struct editenv_info *info)
{
	struct scene_obj_txtedit *ted;
	struct scene *scn;
	const char *name;
	uint font_size;
	int ret;

	ret = expo_set_display(exp, dev);
	if (ret)
		return log_msg_ret("dis", ret);

	ret = vidconsole_get_font_size(exp->cons, NULL, &name, &font_size);
	if (ret)
		font_size = 16;

	exp->theme.font_size = font_size;
	exp->theme.textline_label_margin_x = 10;

	ret = scene_new(exp, "edit", EDITENV_SCENE, &scn);
	if (ret < 0)
		return log_msg_ret("scn", ret);

	ret = scene_texted(scn, "textedit", EDITENV_OBJ_TEXTEDIT, 70, &ted);
	if (ret < 0)
		return log_msg_ret("ted", ret);
	ted->obj.flags |= SCENEOF_MULTILINE;

	ret = scene_obj_set_bbox(scn, EDITENV_OBJ_TEXTEDIT, 50, 200, 1300, 400);
	if (ret < 0)
		return log_msg_ret("sbb", ret);

	/* Create the label text object */
	ret = scene_txt_str(scn, "label", EDITENV_OBJ_LABEL, 0, varname, NULL);
	if (ret < 0)
		return log_msg_ret("lab", ret);

	ted->tin.label_id = EDITENV_OBJ_LABEL;

	/* Create the edit text object pointing to the textedit buffer */
	ret = scene_txt_str(scn, "edit", EDITENV_OBJ_EDIT, 0,
			    abuf_data(&ted->tin.buf), NULL);
	if (ret < 0)
		return log_msg_ret("edi", ret);

	ted->tin.edit_id = EDITENV_OBJ_EDIT;

	ret = expo_apply_theme(exp, true);
	if (ret)
		return log_msg_ret("thm", ret);

	/* Copy initial value into the textedit buffer */
	if (value)
		strlcpy(abuf_data(&ted->tin.buf), value,
			abuf_size(&ted->tin.buf));

	ret = expo_set_scene_id(exp, EDITENV_SCENE);
	if (ret)
		return log_msg_ret("sid", ret);

	/* Set the textedit as highlighted and open for editing */
	scene_set_highlight_id(scn, EDITENV_OBJ_TEXTEDIT);
	ret = scene_set_open(scn, EDITENV_OBJ_TEXTEDIT, true);
	if (ret)
		return log_msg_ret("ope", ret);

	expo_enter_mode(exp);

	info->exp = exp;
	info->scn = scn;
	info->ted = ted;

	ret = scene_arrange(scn);
	if (ret)
		return log_msg_ret("arr", ret);

	ret = expo_render(exp);
	if (ret)
		return log_msg_ret("ren", ret);

	return 0;
}

int expo_editenv_init(const char *varname, const char *value,
		      struct editenv_info *info)
{
	struct udevice *dev;
	struct expo *exp;
	int ret;

	ret = uclass_first_device_err(UCLASS_VIDEO, &dev);
	if (ret)
		return log_msg_ret("vid", ret);

	ret = expo_new("editenv", NULL, &exp);
	if (ret)
		return log_msg_ret("exp", ret);

	ret = editenv_setup(exp, dev, varname, value, info);
	if (ret) {
		expo_destroy(exp);
		return log_msg_ret("set", ret);

	}

	return 0;
}

int expo_editenv_poll(struct editenv_info *info)
{
	struct expo_action act;
	int ret;

	ret = scene_arrange(info->scn);
	if (ret)
		return log_msg_ret("arr", ret);

	ret = expo_render(info->exp);
	if (ret)
		return log_msg_ret("ren", ret);

	ret = expo_poll(info->exp, &act);
	if (ret == -EAGAIN)
		return -EAGAIN;
	if (ret)
		return log_msg_ret("pol", ret);

	if (act.type == EXPOACT_QUIT)
		return -ECANCELED;

	if (act.type == EXPOACT_CLOSE)
		return 0;

	return -EAGAIN;
}

void expo_editenv_uninit(struct editenv_info *info)
{
	expo_exit_mode(info->exp);
	expo_destroy(info->exp);
}

const char *expo_editenv_result(struct editenv_info *info)
{
	return abuf_data(&info->ted->tin.buf);
}

int expo_editenv(const char *varname, const char *value, char *buf, int size)
{
	struct editenv_info info;
	int ret;

	ret = expo_editenv_init(varname, value, &info);
	if (ret)
		return log_msg_ret("ini", ret);

	/* Render and process input */
	while (1) {
		ret = expo_editenv_poll(&info);
		if (ret != -EAGAIN)
			break;
	}

	if (!ret)
		strlcpy(buf, expo_editenv_result(&info), size);

	expo_editenv_uninit(&info);

	return ret;
}
