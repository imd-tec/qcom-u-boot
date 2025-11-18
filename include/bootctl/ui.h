/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Bootctl display
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#ifndef __bootctl_display_h
#define __bootctl_display_h

#include <stdbool.h>
#include <abuf.h>

struct expo;
struct logic_priv;
struct osinfo;
struct oslist_iter;
struct scene;
struct udevice;

/**
 * struct bc_ui_priv - Common uclass private data for UI devices
 *
 * @expo: Expo containing the menu
 * @scn: Current scene being shown
 * @lpriv: Private data of logic device
 * @console: vidconsole device in use
 * @autoboot_template: template string to use for autoboot
 * @autoboot_str: current string displayed for autoboot timeout
 * @logo: logo in bitmap format, NULL to use default
 * @logo_size: size of the logo in bytes
 */
struct bc_ui_priv {
	struct expo *expo;
	struct scene *scn;
	struct logic_priv *lpriv;
	struct udevice *console;
	struct abuf autoboot_template;
	struct abuf *autoboot_str;
	const void *logo;
	int logo_size;
};

/**
 * struct bc_ui_ops - Operations for displays
 */
struct bc_ui_ops {
	/**
	 * print() - Show a string on the display
	 *
	 * @dev: Display device
	 * @msg: Message to show
	 * Return 0 if OK, -ve on error
	 */
	int (*print)(struct udevice *dev, const char *msg);

	/**
	 * show() - Show the display, ready to accept boot options
	 *
	 * @dev: Display device
	 * Return 0 if OK, -ve on error
	 */
	int (*show)(struct udevice *dev);

	/**
	 * add() - Add an OS to the display, so the user can select it
	 *
	 * @dev: Display device
	 * @info: Information about the OS to display
	 * Return 0 if OK, -ve on error
	 */
	int (*add)(struct udevice *dev, struct osinfo *info);

	/**
	 * render() - Render any updates to the display
	 *
	 * @dev: Display device
	 * Return 0 if OK, -ve on error
	 */
	int (*render)(struct udevice *dev);

	/**
	 * poll() - Check for user activity
	 *
	 * @dev: Display device
	 * @seqp: Returns the sequence number of the osinfo that is currently
	 *	pointed to/highlighted, or -1 if nothing
	 * @selectedp: Returns true if the user selected an item, else false
	 * Return: 0 if OK, -EPIPE if the user tried to quit the menu, other
	 *	-ve on error
	 */
	int (*poll)(struct udevice *dev, int *seqp, bool *selectedp);

	/**
	 * switch_layout() - Switch between different UI layout modes
	 *
	 * @dev: Display device
	 * Return 0 if OK, -ve on error
	 */
	int (*switch_layout)(struct udevice *dev);

	/**
	 * show_pass() - Show or hide the passphrase input field
	 *
	 * @dev: Display device
	 * @seq: Sequence number of the bootflow item
	 * @show: true to show the pass field, false to hide it
	 * Return 0 if OK, -ve on error
	 */
	int (*show_pass)(struct udevice *dev, int seq, bool show);

	/**
	 * get_pass() - Get the passphrase entered by the user
	 *
	 * @dev: Display device
	 * @seq: Sequence number of the bootflow item
	 * @passp: Returns pointer to the passphrase string
	 * Return 0 if OK, -ve on error
	 */
	int (*get_pass)(struct udevice *dev, int seq, const char **passp);

	/**
	 * show_pass_msg() - Show or hide the pass message
	 *
	 * @dev: Display device
	 * @seq: Sequence number of the bootflow item
	 * @show: true to show the message, false to hide it
	 * Return 0 if OK, -ve on error
	 */
	int (*show_pass_msg)(struct udevice *dev, int seq, bool show);

	/**
	 * set_pass_msg() - Set the pass message text
	 *
	 * @dev: Display device
	 * @seq: Sequence number of the bootflow item
	 * @msg: Message text to display
	 * Return 0 if OK, -ve on error
	 */
	int (*set_pass_msg)(struct udevice *dev, int seq, const char *msg);
};

#define bc_ui_get_ops(dev)  ((struct bc_ui_ops *)(dev)->driver->ops)

/**
 * bc_ui_show() - Show the display, ready to accept boot options
 *
 * @dev: Display device
 * Return 0 if OK, -ve on error
 */
int bc_ui_show(struct udevice *dev);

/**
 * bc_ui_add() - Add an OS to the display, so the user can select it
 *
 * @dev: Display device
 * @info: Information about the OS to display
 * Return 0 if OK, -ve on error
 */
int bc_ui_add(struct udevice *dev, struct osinfo *info);

/**
 * bc_ui_render() - Render any updates to the display
 *
 * @dev: Display device
 * Return 0 if OK, -ve on error
 */
int bc_ui_render(struct udevice *dev);

/**
 * bc_ui_poll() - Check for user activity
 *
 * @dev: Display device
 * @seqp: Returns the sequence number of the osinfo that is currently
 *	pointed to/highlighted, or -1 if nothing
 * @selectedp: Returns true if the user selected an item, else false
 * Return: 0 if OK, -EPIPE if the user tried to quit the menu, other
 *	-ve on error
 */
int bc_ui_poll(struct udevice *dev, int *seqp, bool *selectedp);

/**
 * bc_ui_switch_layout() - Switch between different UI layout modes
 *
 * @dev: Display device
 * Return 0 if OK, -ve on error
 */
int bc_ui_switch_layout(struct udevice *dev);

/**
 * bc_ui_show_pass() - Show or hide the pass input field
 *
 * @dev: Display device
 * @seq: Sequence number of the bootflow item
 * @show: true to show the pass field, false to hide it
 * Return 0 if OK, -ve on error
 */
int bc_ui_show_pass(struct udevice *dev, int seq, bool show);

/**
 * bc_ui_get_pass() - Get the pass entered by the user
 *
 * @dev: Display device
 * @seq: Sequence number of the bootflow item
 * @passp: Returns pointer to the pass string
 * Return 0 if OK, -ve on error
 */
int bc_ui_get_pass(struct udevice *dev, int seq, const char **passp);

/**
 * bc_ui_show_pass_msg() - Show or hide the pass message
 *
 * @dev: Display device
 * @seq: Sequence number of the bootflow item
 * @show: true to show the message, false to hide it
 * Return 0 if OK, -ve on error
 */
int bc_ui_show_pass_msg(struct udevice *dev, int seq, bool show);

/**
 * bc_ui_set_pass_msg() - Set the pass message text
 *
 * @dev: Display device
 * @seq: Sequence number of the bootflow item
 * @msg: Message text to display
 * Return 0 if OK, -ve on error
 */
int bc_ui_set_pass_msg(struct udevice *dev, int seq, const char *msg);

#endif
