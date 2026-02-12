# SPDX-License-Identifier: GPL-2.0+
# Copyright 2026 Canonical Ltd
# Written by Simon Glass <simon.glass@canonical.com>

BUILD_CFLAGS += -shared
PLATFORM_CPPFLAGS += $(CFLAGS_EFI)
