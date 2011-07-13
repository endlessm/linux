#
# Makefile for the Freescale Ethernet controllers
#
EXTRA_CFLAGS           += -DVERSION=\"\"
#
#Include netcomm SW specific definitions
include $(srctree)/drivers/net/dpa/NetCommSw/ncsw_config.mk

NCSW_FM_INC = $(srctree)/drivers/net/dpa/NetCommSw/Peripherals/FM/inc

EXTRA_CFLAGS += -I$(NCSW_FM_INC)

obj-y		+= fsl-ncsw-Pcd.o

fsl-ncsw-Pcd-objs	:=   fm_cc.o fm_kg.o fm_pcd.o fm_plcr.o fm_prs.o




