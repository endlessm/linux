#
# Makefile for the Freescale Ethernet controllers
#
EXTRA_CFLAGS           += -DVERSION=\"\"
#
#Include netcomm SW specific definitions
include $(srctree)/drivers/net/dpa/NetCommSw/ncsw_config.mk

EXTRA_CFLAGS += -I$(NET_DPA)

#Netcomm SW tree
obj-$(CONFIG_FSL_FMAN) += NetCommSw/
obj-$(CONFIG_FSL_DPA_1588) += dpaa_1588.o
obj-$(CONFIG_DPA) += fsl-mac.o fsl-dpa.o
obj-$(CONFIG_DPA_OFFLINE_PORTS) += fsl-oh.o

fsl-dpa-objs	:= dpa-ethtool.o dpaa_eth.o
fsl-mac-objs	:= mac.o mac-api.o
fsl-oh-objs	:= offline_port.o
