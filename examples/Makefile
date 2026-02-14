# SPDX-License-Identifier: GPL-2.0+

ifdef CONFIG_EXAMPLES

ifdef FTRACE
subdir-ccflags-y += -finstrument-functions -DFTRACE
endif

subdir-$(EXAMPLES_STANDALONE) += standalone
subdir-$(CONFIG_LEGACY_API) += api
# Sandbox examples use the standalone Makefile; other archs use kbuild
ifndef CONFIG_SANDBOX
subdir-$(CONFIG_ULIB) += ulib
endif
endif
