# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the IPU3 cio2 and ImGU drivers
#

# force check the compile warning to make sure zero warnings
# note we may have build issue when gcc upgraded.
subdir-ccflags-y := -Wall -Wextra
subdir-ccflags-y += $(call cc-disable-warning, unused-parameter)
subdir-ccflags-y += $(call cc-disable-warning, implicit-fallthrough)
subdir-ccflags-y += $(call cc-disable-warning, missing-field-initializers)
subdir-ccflags-$(CONFIG_VIDEO_INTEL_IPU_WERROR) += -Werror

obj-$(CONFIG_VIDEO_IPU3_CIO2) += ipu3/
#obj-$(CONFIG_VIDEO_INTEL_IPU4)	 += ipu4/
#obj-$(CONFIG_VIDEO_INTEL_IPU4P) += ipu4/
obj-$(CONFIG_VIDEO_INTEL_IPU6)	+= ipu6/
