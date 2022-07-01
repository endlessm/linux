# SPDX-License-Identifier: GPL-2.0

top_srcdir = ../../../..

LINUX_TOOL_ARCH_INCLUDE = $(top_srcdir)/tools/arch/x86/include

CFLAGS += -O3 -Wl,-no-as-needed -Wall -static -I$(LINUX_TOOL_ARCH_INCLUDE)

TEST_GEN_PROGS := tdx_attest_test

include ../lib.mk
