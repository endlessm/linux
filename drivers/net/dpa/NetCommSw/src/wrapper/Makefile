#
# Makefile for the Freescale Ethernet controllers
#
EXTRA_CFLAGS           += -DVERSION=\"\"
#
#Include netcomm SW specific definitions
include $(srctree)/drivers/net/dpa/NetCommSw/ncsw_config.mk

EXTRA_CFLAGS += -I$(NCSW_FM_INC)
EXTRA_CFLAGS += -I$(NET_DPA)

obj-y		+= fsl-ncsw-PFM.o
obj-$(CONFIG_FSL_FMAN_TEST)	+= fman_test.o

fsl-ncsw-PFM-objs	:=	lnxwrp_fm.o lnxwrp_fm_port.o lnxwrp_ioctls_fm.o \
				lnxwrp_sysfs.o lnxwrp_sysfs_fm.o lnxwrp_sysfs_fm_port.o \
				lnxwrp_resources.o
obj-$(CONFIG_COMPAT)     +=   lnxwrp_ioctls_fm_compat.o
