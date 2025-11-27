/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2025 Canonical Ltd
 *
 * Tillitis TKey security-token uclass interface
 */

#ifndef _TKEY_UCLASS_H
#define _TKEY_UCLASS_H

#include <stdbool.h>

struct tkey_frame;
struct udevice;

/**
 * struct tkey_load_ctx - Context for iterative app loading
 *
 * @dev: TKey device
 * @app_data: Complete app binary data
 * @app_size: Size of app data
 * @offset: Current offset in app data
 */
struct tkey_load_ctx {
	struct udevice *dev;
	const void *app_data;
	int app_size;
	int offset;
};

/* TKey constants */
#define TKEY_NAME_SIZE			5
#define TKEY_CDI_SIZE			32
#define TKEY_UDI_SIZE			8
#define TKEY_WRAPPING_KEY_SIZE		32
#define TKEY_USS_MAX_SIZE		32
#define TKEY_PUBKEY_SIZE		32
#define TKEY_DISK_KEY_SIZE		32
#define TKEY_HASH_SIZE			32

/* Embedded TKey signer binary */
extern char __signer_1_0_0_begin[];
extern char __signer_1_0_0_end[];
#define TKEY_SIGNER_SIZE	(__signer_1_0_0_end - __signer_1_0_0_begin)

/**
 * struct tkey_ops - The functions that a TKey driver must implement.
 *
 * @read: Read data from TKey device
 * @write: Write data to TKey device
 * @read_all: Read all available data from TKey device (optional)
 */
struct tkey_ops {
	/**
	 * read() - Read data from TKey device
	 *
	 * @dev: TKey device
	 * @buf: Buffer to store read data
	 * @len: Maximum number of bytes to read
	 * @timeout_ms: Timeout in milliseconds
	 *
	 * Returns: Number of bytes read on success, -ve error on failure
	 */
	int (*read)(struct udevice *dev, void *buf, int len, int timeout_ms);

	/**
	 * write() - Write data to TKey device
	 *
	 * @dev: TKey device
	 * @buf: Buffer containing data to write
	 * @len: Number of bytes to write
	 *
	 * Returns: Number of bytes written on success, -ve error on failure
	 */
	int (*write)(struct udevice *dev, const void *buf, int len);

	/**
	 * read_all() - Read all available data from TKey device (optional)
	 *
	 * @dev: TKey device
	 * @buf: Buffer to store read data
	 * @maxlen: Maximum number of bytes to read
	 * @timeout_ms: Timeout in milliseconds
	 *
	 * This method reads all available data in one operation, which is
	 * more suitable for USB devices that send complete frames.
	 *
	 * Returns: Number of bytes read on success, -ve error on failure
	 */
	int (*read_all)(struct udevice *dev, void *buf, int maxlen,
			int timeout_ms);
};

#define tkey_get_ops(dev)	((struct tkey_ops *)(dev)->driver->ops)

/**
 * tkey_read() - Read data from TKey device
 *
 * @dev: TKey device
 * @buf: Buffer to store read data
 * @len: Maximum number of bytes to read
 * @timeout_ms: Timeout in milliseconds
 * Return: Number of bytes read on success, -ve error on failure
 */
int tkey_read(struct udevice *dev, void *buf, int len, int timeout_ms);

/**
 * tkey_write() - Write data to TKey device
 *
 * @dev: TKey device
 * @buf: Buffer containing data to write
 * @len: Number of bytes to write
 * Return: Number of bytes written on success, -ve error on failure
 */
int tkey_write(struct udevice *dev, const void *buf, int len);

/**
 * tkey_read_all() - Read all available data from TKey device
 *
 * @dev: TKey device
 * @buf: Buffer to store read data
 * @maxlen: Maximum number of bytes to read
 * @timeout_ms: Timeout in milliseconds
 *
 * Reads all available data in one operation, suitable for USB devices.
 * Falls back to regular read if read_all is not implemented.
 *
 * Return: Number of bytes read on success, -ve error on failure
 */
int tkey_read_all(struct udevice *dev, void *buf, int maxlen,
		  int timeout_ms);

/**
 * tkey_get_udi() - Get Unique Device Identifier
 *
 * @dev: TKey device
 * @udi: Buffer to store UDI (must be at least 8 bytes)
 * Return: 0 on success, -ve error on failure
 */
int tkey_get_udi(struct udevice *dev, void *udi);

/**
 * tkey_get_name_version() - Get device name and version
 *
 * @dev: TKey device
 * @name0: Buffer for first name field (TKEY_NAME_SIZE bytes)
 * @name1: Buffer for second name field (TKEY_NAME_SIZE bytes)
 * @version: Pointer to store version number
 * Return: 0 on success, -ve error on failure
 */
int tkey_get_name_version(struct udevice *dev, char name0[TKEY_NAME_SIZE],
			  char name1[TKEY_NAME_SIZE], u32 *version);

/**
 * tkey_in_app_mode() - Check if device is in firmware or app mode
 *
 * @dev: TKey device
 * Return: 0 if in firmware mode, 1 if in app mode, -ve error on failure
 */
int tkey_in_app_mode(struct udevice *dev);

/**
 * tkey_load_app() - Load complete app to TKey device
 *
 * @dev: TKey device
 * @app_data: Complete app binary data
 * @app_size: Size of app data
 *
 * This function performs all steps needed to load an app:
 * - Verifies device is in firmware mode
 * - Sends app header with size
 * - Sends app data in chunks
 *
 * Return: 0 on success, -ve error on failure (-ENOTSUPP if not in
 *	firmware mode)
 */
int tkey_load_app(struct udevice *dev, const void *app_data, int app_size);

/**
 * tkey_load_app_with_uss() - Load app with User-Supplied Secret
 *
 * @dev: TKey device
 * @app_data: Complete app binary data
 * @app_size: Size of app data
 * @uss: User-Supplied Secret (password/passphrase) - can be NULL
 * @uss_size: Size of USS data (max 32 bytes)
 *
 * This function performs all steps needed to load an app with USS:
 * - Verifies device is in firmware mode
 * - Sends app header with size
 * - Sends app data in chunks
 * - Sends USS if provided
 *
 * Return: 0 on success, -ve error on failure (-ENOTSUPP if not in
 *	firmware mode)
 */
int tkey_load_app_with_uss(struct udevice *dev, const void *app_data,
			   int app_size, const void *uss, int uss_size);

/**
 * tkey_load_start() - Start iterative app loading
 *
 * @ctx: Context structure to initialize
 * @dev: TKey device
 * @app_data: Complete app binary data
 * @app_size: Size of app data
 * @uss: User-Supplied Secret (password/passphrase) - can be NULL
 * @uss_size: Size of USS data (max 32 bytes)
 *
 * This function initializes the context and sends the app header.
 * Call tkey_load_next() repeatedly to send the data blocks.
 *
 * Return: 0 on success, -ve error on failure (-ENOTSUPP if not in
 *	firmware mode)
 */
int tkey_load_start(struct tkey_load_ctx *ctx, struct udevice *dev,
		    const void *app_data, int app_size,
		    const void *uss, int uss_size);

/**
 * tkey_load_next() - Send next block(s) of app data
 *
 * @ctx: Context structure from tkey_load_start()
 * @max_blocks: Maximum number of blocks to send (0 = send all remaining)
 *
 * This function sends the next n blocks of app data. Call repeatedly
 * until it returns 0 (done) instead of -EAGAIN (more blocks remain).
 *
 * Return: 0 if loading is complete, -EAGAIN if more blocks remain,
 *	-ve error on failure
 */
int tkey_load_next(struct tkey_load_ctx *ctx, int max_blocks);

/**
 * tkey_get_pubkey() - Get public key from signer app
 *
 * @dev: TKey device (must be in app mode with signer loaded)
 * @pubkey: Buffer to store public key (must be at least 32 bytes)
 * Return: 0 on success, -ve error on failure
 */
int tkey_get_pubkey(struct udevice *dev, void *pubkey);

/**
 * tkey_derive_disk_key() - Derive disk encryption key from USS
 *
 * @dev: TKey device
 * @app_data: Signer app binary data
 * @app_size: Size of app data
 * @uss: User-Supplied Secret for key derivation
 * @uss_size: Size of USS
 * @disk_key: Buffer to store derived disk key (32 bytes)
 * @pubkey: Buffer to store public key (32 bytes)
 * @key_hash: Buffer to store verification hash (32 bytes), or NULL if
 *	not needed
 *
 * This function loads the signer app with USS, retrieves the public key,
 * and derives a disk encryption key. Optionally generates a verification
 * hash.
 *
 * Return: 0 on success, -ve error on failure
 */
int tkey_derive_disk_key(struct udevice *dev, const void *app_data,
			 int app_size, const void *uss, int uss_size,
			 void *disk_key, void *pubkey, void *key_hash);

/**
 * tkey_derive_wrapping_key() - Derive wrapping key from password and UDI
 *
 * @dev: TKey device (must be in firmware mode to access UDI)
 * @password: User-provided password
 * @wrapping_key: Buffer to store wrapping key (32 bytes)
 *
 * This function gets the device UDI and derives a wrapping key using
 * BLAKE2b(UDI || password).
 *
 * Return: 0 on success, -ve error on failure
 */
int tkey_derive_wrapping_key(struct udevice *dev, const char *password,
			     void *wrapping_key);

/**
 * tkey_emul_reset_for_test() - Reset emulator to firmware mode for testing
 *
 * This is a back-door function for tests to simulate TKey replug by resetting
 * the emulator to firmware mode. Only works with tkey-emul driver.
 *
 * @dev: TKey device (must be tkey-emul)
 * Return: 0 on success, -ve error on failure
 */
int tkey_emul_reset_for_test(struct udevice *dev);

/**
 * tkey_emul_set_pubkey_for_test() - Set public key returned by emulator
 *
 * This is a back-door function for tests to configure what public key the
 * emulator returns from APP_CMD_GET_PUBKEY. The disk key will be derived
 * from this pubkey using SHA256(hex(pubkey)).
 *
 * @dev: TKey device (must be tkey-emul)
 * @pubkey: Public key to return (32 bytes)
 * Return: 0 on success, -ve error on failure
 */
int tkey_emul_set_pubkey_for_test(struct udevice *dev, const void *pubkey);

/**
 * tkey_emul_set_app_mode_for_test() - Set emulator mode for testing
 *
 * This is a back-door function for tests to force the emulator into app mode
 * or firmware mode, allowing tests to verify replug behavior.
 *
 * @dev: TKey device (must be tkey-emul)
 * @app_mode: true for app mode, false for firmware mode
 * Return: 0 on success, -ve error on failure
 */
int tkey_emul_set_app_mode_for_test(struct udevice *dev, bool app_mode);

/**
 * tkey_emul_set_connected_for_test() - Simulate device connection state
 *
 * This is a back-door function for tests to simulate physical insertion or
 * removal of the TKey device. When disconnected, all I/O operations and
 * probe attempts will fail.
 *
 * @dev: TKey device (must be tkey-emul)
 * @connected: true to simulate device present, false to simulate removal
 * Return: 0 on success, -ve error on failure
 */
int tkey_emul_set_connected_for_test(struct udevice *dev, bool connected);

/**
 * tkey_get_device() - Get the current TKey device
 *
 * Return: Pointer to TKey device, or NULL if not found
 */
struct udevice *tkey_get_device(void);

#endif /* _TKEY_UCLASS_H */
