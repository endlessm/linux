#
# Makefile for the Freescale Ethernet controllers
#
EXTRA_CFLAGS           += -DVERSION=\"\"
#
#Include netcomm SW specific definitions
include $(srctree)/drivers/net/dpa/NetCommSw/ncsw_config.mk
NCSW_FM_INC = $(srctree)/drivers/net/dpa/NetCommSw/Peripherals/FM/inc

EXTRA_CFLAGS += -I$(NCSW_FM_INC)

obj-y		+= fsl-ncsw-MAC.o

fsl-ncsw-MAC-objs	:=   dtsec.o dtsec_mii_acc.o fm_mac.o tgec.o tgec_mii_acc.o
