# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>

ifndef CONFIG_EFI_APP
extra-y = start.o
endif

obj-y += cpu.o
ifndef CONFIG_EFI_APP
obj-y += mtrap.o
endif
