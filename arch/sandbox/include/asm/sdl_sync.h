/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2026 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#ifndef __SANDBOX_SDL_SYNC_H
#define __SANDBOX_SDL_SYNC_H

#include <stdbool.h>

/**
 * struct sandbox_sdl_sync_opts - Options for sandbox_sdl_sync()
 *
 * @draw_grid: Draw a grid overlay on the display
 * @grid_size: Grid size in pixels (0 for default of 0x20)
 */
struct sandbox_sdl_sync_opts {
	bool draw_grid;
	int grid_size;
};

#endif
