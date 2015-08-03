KBUILD_EXTMOD=${srctree}/ubuntu/vbox
#
# VirtualBox Guest Additions Module Makefile.
#
# (For 2.6.x this file must be 'Makefile'!)
#
# Copyright (C) 2006-2011 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

# Linux kbuild sets this to our source directory if we are called from there
obj ?= $(CURDIR)
include $(obj)/Makefile.include.header

MOD_NAME   = vboxvideo

MOD_OBJS   = vboxvideo_drm.o

ifneq ($(wildcard $(KBUILD_EXTMOD)/vboxvideo),)
 MANGLING := $(KBUILD_EXTMOD)/vboxvideo/include/VBox/VBoxGuestMangling.h
else
 MANGLING := $(KBUILD_EXTMOD)/include/VBox/VBoxGuestMangling.h
endif
MOD_CFLAGS = -Wno-declaration-after-statement -fshort-wchar -include $(MANGLING)
MOD_INCL   = $(addprefix -I$(KBUILD_EXTMOD),/ /include /r0drv/linux)
# What on earth is this?
MOD_INCL  += $(addprefix -I$(KBUILD_EXTMOD)/vboxvideo,/ /include /r0drv/linux)
# Enterprise Linux 6.5 does not include the drm user API headers with the kernel
# headers.
MOD_INCL += $(foreach inc,$(KERN_INCL) include,\
              $(if $(wildcard $(inc)/linux/utsrelease.h),\
                $(if $(shell grep '"2.6.32.*el6.*"' $(inc)/linux/utsrelease.h),\
		      -I/usr/include,),))
MOD_DEFS  := -DRT_OS_LINUX -DIN_RING0 -DIN_RT_R0 \
	     -DIN_SUP_R0 -DVBOX -DVBOX_WITH_HGCM -DLOG_TO_BACKDOOR -DIN_MODULE \
	     -DIN_GUEST_R0
# our module does not export any symbol
MOD_DEFS  += -DRT_NO_EXPORT_SYMBOL
ifeq ($(BUILD_TARGET_ARCH),amd64)
 MOD_DEFS += -DRT_ARCH_AMD64 -DVBOX_WITH_64_BITS_GUESTS
else
 MOD_DEFS += -DRT_ARCH_X86
endif
MOD_CLEAN  = . linux r0drv r0drv/linux

include $(obj)/Makefile.include.footer
