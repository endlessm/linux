#
# Makefile for the Freescale Ethernet controllers
#
EXTRA_CFLAGS           += -DVERSION=\"\"
#
#Include netcomm SW specific definitions
include $(srctree)/drivers/net/dpa/NetCommSw/ncsw_config.mk
#
obj-y		+= etc/
obj-${CONFIG_FMAN_P3040_P4080_P5020}	+= integrations/P3040_P4080_P5020/
obj-${CONFIG_FMAN_P1023} 	        += integrations/P1023/
obj-y		+= Peripherals/FM/
obj-y		+= src/
