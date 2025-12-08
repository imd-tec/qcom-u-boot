/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Implementation of the logic to perform a boot
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#ifndef __bootctl_logic_h
#define __bootctl_logic_h

#include <bootctl/oslist.h>
#include <tkey.h>

struct udevice;

/**
 * enum unlock_state - State of the disk unlock process
 *
 * @UNS_IDLE: No unlock in progress
 * @UNS_WAITING_PASS: Waiting for user to enter passphrase
 * @UNS_UNLOCK_NORMAL: Unlocking with direct passphrase
 * @UNS_TKEY_START: Unlocking with TKey
 * @UNS_TKEY_WAIT_REMOVE: Waiting for TKey to be removed (after wrong passphrase)
 * @UNS_TKEY_WAIT_INSERT: Waiting for TKey to be inserted (after removal)
 * @UNS_TKEY_INSERTED: TKey inserted, starting app load
 * @UNS_TKEY_LOADING: Loading TKey app
 * @UNS_TKEY_READY: TKey key derived, ready to unlock
 * @UNS_TKEY_UNLOCK: Unlocking LUKS partition
 * @UNS_UNLOCK_RESULT: Processing unlock result (success or failure)
 * @UNS_BAD_PASS: Unlock failed, showing error message
 * @UNS_OK: Unlock succeeded, showing success message
 */
enum unlock_state {
	UNS_IDLE,
	UNS_WAITING_PASS,
	UNS_UNLOCK_NORMAL,
	UNS_TKEY_START,
	UNS_TKEY_WAIT_REMOVE,
	UNS_TKEY_WAIT_INSERT,
	UNS_TKEY_INSERTED,
	UNS_TKEY_LOADING,
	UNS_TKEY_READY,
	UNS_TKEY_UNLOCK,
	UNS_UNLOCK_RESULT,
	UNS_BAD_PASS,
	UNS_OK,
};

/**
 * struct logic_priv - Information maintained by the boot logic as it works
 *
 * @opt_persist_state: true if state can be preserved across reboots
 * @opt_default_os: true if we record a default OS to boot
 * @opt_timeout: boot timeout in seconds
 * @opt_skip_timeout: true to skip any boot timeout if the last boot succeeded
 * @opt_track_success: true to track whether the last boot succeeded (made it to
 * user space)
 * @opt_labels: if non-NULL, a space-separated list of bootstd labels which can
 * be used to boot
 * @opt_autoboot: true to autoboot the default OS after a timeout
 * @opt_measure: true to measure loaded images, etc.
 * @opt_slow_refresh: refresh the UI only when needed
 * @opt_tkey: true to use TKey for unlocking encrypted volumes
 *
 * @state_loaded: true if the state information has been loaded
 * @scanning: true if scanning for new OSes
 * @start_time: monotonic time when the boot started
 * @next_countdown: next monotonic time to check the timeout
 * @autoboot_remain_s: remaining autoboot time in seconds
 * @autoboot_active: true if autoboot is active
 * @default_os: name of the default OS to boot
 * @osinfo: List of OSes to show
 * @refresh: true if we need to refresh the UI because something has changed
 * @selected_seq: sequence number of OS waiting for passphrase, or -1 if none
 * @ready_to_boot: true if success message shown, ready to boot on next poll
 *
 * @tkey: TKey device (pointer never changes once set)
 * @tkey_present: true if TKey is physically present and accessible
 * @tkey_load_ctx: TKey app loading context for iterative loading
 * @tkey_disk_key: Buffer to store derived disk key from TKey
 * @ustate: Current state of the disk unlock process
 * @unlock_result: Result of disk unlock (0 = OK, -ve on error)
 * @time_error: monotonic time when error message display started
 *
 * @iter: oslist iterator, used to find new OSes
 * @meas: TPM-measurement device
 * @oslist: provides OSes to boot; we iterate through each osinfo driver to find
 * all OSes
 * @state: provides persistent state
 * @ui: provides a visual boot menu on a display / console device
 */
struct logic_priv {
	bool opt_persist_state;
	bool opt_default_os;
	uint opt_timeout;
	bool opt_skip_timeout;
	bool opt_track_success;
	const char *opt_labels;
	bool opt_autoboot;
	bool opt_measure;
	bool opt_slow_refresh;
	bool opt_tkey;

	bool state_loaded;
	bool state_saved;
	bool scanning;
	ulong start_time;
	uint next_countdown;
	uint autoboot_remain_s;
	bool autoboot_active;
	const char *default_os;
	struct alist osinfo;
	bool refresh;
	int selected_seq;
	bool ready_to_boot;

	struct udevice *tkey;
	bool tkey_present;
	struct tkey_load_ctx tkey_load_ctx;
	u8 tkey_disk_key[TKEY_DISK_KEY_SIZE];
	enum unlock_state ustate;
	int unlock_result;
	ulong time_error;

	struct oslist_iter iter;
	struct udevice *meas;
	struct udevice *oslist;
	struct udevice *state;
	struct udevice *ui;
};

/**
 * struct bc_logic_ops - Operations related to boot loader
 */
struct bc_logic_ops {
	/**
	 * prepare() - Prepare the components needed for the boot
	 *
	 * This sets up the various device, like ui and oslist
	 *
	 * This must be called before start()
	 *
	 * @dev: Logic device
	 * Return: 0 if OK, or -ve error code
	 */
	int (*prepare)(struct udevice *dev);

	/**
	 * start() - Start the boot process
	 *
	 * Gets things ready, shows the UI, etc.
	 *
	 * This pust be called before poll()
	 *
	 * @dev: Logic device
	 * Return: 0 if OK, or -ve error code
	 */
	int (*start)(struct udevice *dev);

	/**
	 * poll() - Poll the boot process
	 *
	 * Try to progress the boot towards a result
	 *
	 * This should be called repeatedly until it either boots and OS (iwc
	 * it won't return) or returns an error code
	 *
	 * @dev: Logic device
	 * Return: does not return if OK, -ESHUTDOWN if something went wrong
	 */
	int (*poll)(struct udevice *dev);
};

#define bc_logic_get_ops(dev)  ((struct bc_logic_ops *)(dev)->driver->ops)

/**
 * bc_logic_prepare() - Prepare the components needed for the boot
 *
 * This sets up the various device, like ui and oslist
 *
 * This must be called before start()
 *
 * @dev: Logic device
 * Return: 0 if OK, or -ve error code
 */
int bc_logic_prepare(struct udevice *dev);

/**
 * bc_logic_start() - Start the boot process
 *
 * Gets things ready, shows the UI, etc.
 *
 * This pust be called before poll()
 *
 * @dev: Logic device
 * Return: 0 if OK, or -ve error code
 */
int bc_logic_start(struct udevice *dev);

/**
 * bc_logic_poll() - Poll the boot process
 *
 * Try to progress the boot towards a result
 *
 * This should be called repeatedly until it either boots and OS (iwc
 * it won't return) or returns an error code
 *
 * @dev: Logic device
 * Return: does not return if OK, -ESHUTDOWN if something went wrong
 */
int bc_logic_poll(struct udevice *dev);

#endif
