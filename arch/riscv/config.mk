# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2000-2002
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# Copyright (c) 2017 Microsemi Corporation.
# Padmarao Begari, Microsemi Corporation <padmarao.begari@microsemi.com>
#
# Copyright (C) 2017 Andes Technology Corporation
# Rick Chen, Andes Technology Corporation <rick@andestech.com>
#

32bit-emul		:= elf32lriscv
64bit-emul		:= elf64lriscv

ifdef CONFIG_32BIT
KBUILD_LDFLAGS		+= -m $(32bit-emul)
EFI_LDS			:= elf_riscv32_efi.lds
PLATFORM_ELFFLAGS	+= -B riscv -O elf32-littleriscv
endif

ifdef CONFIG_64BIT
KBUILD_LDFLAGS		+= -m $(64bit-emul)
EFI_LDS			:= elf_riscv64_efi.lds
PLATFORM_ELFFLAGS	+= -B riscv -O elf64-littleriscv
endif

PLATFORM_CPPFLAGS	+= -ffixed-x3 -fpic
PLATFORM_RELFLAGS	+= -fno-common -ffunction-sections -fdata-sections
ifndef CONFIG_EFI_APP
LDFLAGS_u-boot		+= --gc-sections -static -pie
endif

EFI_CRT0		:= crt0_riscv_efi.o
EFI_RELOC		:= reloc_riscv_efi.o

OBJCOPYFLAGS_EFI += -j .text -j .rodata -j .data -j .sdata -j .dynamic \
	-j .dynsym -j .rela -j .reloc -j .got -j .got.plt \
	-j __u_boot_list -j .embedded_dtb -O binary

ifeq ($(CONFIG_EFI_APP),y)

LDFLAGS_FINAL += -znocombreloc -shared -Bsymbolic --gc-sections
LDSCRIPT := $(srctree)/arch/riscv/lib/elf_riscv64_efi_app.lds

# The EFI app linker script places all data sections contiguously
# between _data and _edata so the PE .data section covers them.
OBJCOPYFLAGS_EFI := -j .text -j .rela.dyn \
	-j .dynsym -j .dynstr -j .embedded_dtb -j .data \
	-O binary

endif
