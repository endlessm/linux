EXTRA_CFLAGS += $(USER_EXTRA_CFLAGS)
EXTRA_CFLAGS += -O1
#EXTRA_CFLAGS += -O3
EXTRA_CFLAGS += -Wall
#EXTRA_CFLAGS += -Wextra
#EXTRA_CFLAGS += -Werror
#EXTRA_CFLAGS += -pedantic
#EXTRA_CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes

EXTRA_CFLAGS += -I$(src)/include -I$(src)/hal -g

ccflags-y += -D__CHECK_ENDIAN__

#EXTRA_LDFLAGS += --strip-debug

########################## Features ###########################
CONFIG_HW_PWRP_DETECTION = n
CONFIG_INTEL_WIDI = n
CONFIG_EXT_CLK = n
CONFIG_TRAFFIC_PROTECT = y
CONFIG_LOAD_PHY_PARA_FROM_FILE = y
CONFIG_CALIBRATE_TX_POWER_BY_REGULATORY = n
CONFIG_CALIBRATE_TX_POWER_TO_MAX = n
CONFIG_ODM_ADAPTIVITY = n
CONFIG_SKIP_SIGNAL_SCALE_MAPPING = n
######################## Wake On Lan ##########################
CONFIG_WOWLAN = n
CONFIG_GPIO_WAKEUP = n
CONFIG_PNO_SUPPORT = n
CONFIG_PNO_SET_DEBUG = n
CONFIG_AP_WOWLAN = n
######### Notify SDIO Host Keep Power During Syspend ##########
CONFIG_RTW_SDIO_PM_KEEP_POWER = y
###################### Platform Related #######################
CONFIG_PLATFORM_I386_PC = y
###############################################################

########### COMMON  #################################

_OS_INTFS_FILES :=	os_dep/osdep_service.o \
			os_dep/os_intfs.o \
			os_dep/sdio_intf.o \
			os_dep/sdio_ops_linux.o \
			os_dep/ioctl_linux.o \
			os_dep/xmit_linux.o \
			os_dep/mlme_linux.o \
			os_dep/recv_linux.o \
			os_dep/ioctl_cfg80211.o \
			os_dep/wifi_regd.o \
			os_dep/rtw_proc.o \
			os_dep/sdio_ops_linux.o

_HAL_INTFS_FILES :=	hal/hal_intf.o \
			hal/hal_com.o \
			hal/hal_com_phycfg.o \
			hal/hal_btcoex.o \
			hal/hal_sdio.o

_OUTSRC_FILES := hal/odm_debug.o	\
		hal/odm_HWConfig.o\
		hal/odm.o\
		hal/HalPhyRf.o\
		hal/odm_EdcaTurboCheck.o\
		hal/odm_DIG.o\
		hal/odm_PathDiv.o\
		hal/odm_DynamicBBPowerSaving.o\
		hal/odm_DynamicTxPower.o\
		hal/odm_CfoTracking.o\
		hal/odm_NoiseMonitor.o

EXTRA_CFLAGS += -I$(src)/hal/OUTSRC-BTCoexist
_OUTSRC_FILES += \
				hal/HalBtc8723b1Ant.o \
				hal/HalBtc8723b2Ant.o

########### HAL_RTL8723B #################################
MODULE_NAME = r8723bs

_HAL_INTFS_FILES += hal/HalPwrSeqCmd.o \
					hal/Hal8723BPwrSeq.o

_HAL_INTFS_FILES +=	hal/rtl8723b_hal_init.o \
			hal/rtl8723b_phycfg.o \
			hal/rtl8723b_rf6052.o \
			hal/rtl8723b_dm.o \
			hal/rtl8723b_rxdesc.o \
			hal/rtl8723b_cmd.o \

_HAL_INTFS_FILES +=	\
			hal/sdio_halinit.o \
			hal/rtl8723bs_xmit.o \
			hal/rtl8723bs_recv.o

_HAL_INTFS_FILES += hal/sdio_ops.o

_OUTSRC_FILES += hal/HalHWImg8723B_BB.o\
			hal/HalHWImg8723B_MAC.o\
			hal/HalHWImg8723B_RF.o\
			hal/odm_RegConfig8723B.o\
			hal/HalPhyRf_8723B.o\
			hal/odm_RTL8723B.o

########### END OF PATH  #################################


ifeq ($(CONFIG_HW_PWRP_DETECTION), y)
EXTRA_CFLAGS += -DCONFIG_HW_PWRP_DETECTION
endif

ifeq ($(CONFIG_INTEL_WIDI), y)
EXTRA_CFLAGS += -DCONFIG_INTEL_WIDI
endif

ifeq ($(CONFIG_EXT_CLK), y)
EXTRA_CFLAGS += -DCONFIG_EXT_CLK
endif

ifeq ($(CONFIG_TRAFFIC_PROTECT), y)
EXTRA_CFLAGS += -DCONFIG_TRAFFIC_PROTECT
endif

ifeq ($(CONFIG_LOAD_PHY_PARA_FROM_FILE), y)
EXTRA_CFLAGS += -DCONFIG_LOAD_PHY_PARA_FROM_FILE
endif

ifeq ($(CONFIG_CALIBRATE_TX_POWER_BY_REGULATORY), y)
EXTRA_CFLAGS += -DCONFIG_CALIBRATE_TX_POWER_BY_REGULATORY
endif

ifeq ($(CONFIG_CALIBRATE_TX_POWER_TO_MAX), y)
EXTRA_CFLAGS += -DCONFIG_CALIBRATE_TX_POWER_TO_MAX
endif

ifeq ($(CONFIG_ODM_ADAPTIVITY), y)
EXTRA_CFLAGS += -DCONFIG_ODM_ADAPTIVITY
endif

ifeq ($(CONFIG_SKIP_SIGNAL_SCALE_MAPPING), y)
EXTRA_CFLAGS += -DCONFIG_SKIP_SIGNAL_SCALE_MAPPING
endif

ifeq ($(CONFIG_WOWLAN), y)
EXTRA_CFLAGS += -DCONFIG_WOWLAN
EXTRA_CFLAGS += -DCONFIG_RTW_SDIO_PM_KEEP_POWER
endif

ifeq ($(CONFIG_AP_WOWLAN), y)
EXTRA_CFLAGS += -DCONFIG_AP_WOWLAN
EXTRA_CFLAGS += -DCONFIG_RTW_SDIO_PM_KEEP_POWER
endif

ifeq ($(CONFIG_PNO_SUPPORT), y)
EXTRA_CFLAGS += -DCONFIG_PNO_SUPPORT
ifeq ($(CONFIG_PNO_SET_DEBUG), y)
EXTRA_CFLAGS += -DCONFIG_PNO_SET_DEBUG
endif
endif

ifeq ($(CONFIG_GPIO_WAKEUP), y)
EXTRA_CFLAGS += -DCONFIG_GPIO_WAKEUP
endif

ifeq ($(CONFIG_RTW_SDIO_PM_KEEP_POWER), y)
EXTRA_CFLAGS += -DCONFIG_RTW_SDIO_PM_KEEP_POWER
endif

ifeq ($(CONFIG_PLATFORM_I386_PC), y)
SUBARCH := $(shell uname -m | sed -e s/i.86/i386/ | sed -e s/armv7l/arm/)
ARCH ?= $(SUBARCH)
CROSS_COMPILE ?=
KVER  := $(shell uname -r)
KSRC := /lib/modules/$(KVER)/build
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/
INSTALL_PREFIX :=
endif

ifneq ($(KERNELRELEASE),)

rtk_core :=	core/rtw_cmd.o \
		core/rtw_security.o \
		core/rtw_debug.o \
		core/rtw_io.o \
		core/rtw_ioctl_set.o \
		core/rtw_ieee80211.o \
		core/rtw_mlme.o \
		core/rtw_mlme_ext.o \
		core/rtw_wlan_util.o \
		core/rtw_pwrctrl.o \
		core/rtw_rf.o \
		core/rtw_recv.o \
		core/rtw_sta_mgt.o \
		core/rtw_ap.o \
		core/rtw_xmit.o	\
		core/rtw_btcoex.o \
		core/rtw_odm.o \
		core/rtw_efuse.o

$(MODULE_NAME)-y += $(rtk_core)

$(MODULE_NAME)-$(CONFIG_INTEL_WIDI) += core/rtw_intel_widi.o

$(MODULE_NAME)-y += $(_OS_INTFS_FILES)
$(MODULE_NAME)-y += $(_HAL_INTFS_FILES)
$(MODULE_NAME)-y += $(_OUTSRC_FILES)
$(MODULE_NAME)-y += $(_PLATFORM_FILES)

obj-$(CONFIG_RTL8723BS) := $(MODULE_NAME).o

endif

export CONFIG_RTL8723BS = m

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(shell pwd)  modules

strip:
	$(CROSS_COMPILE)strip $(MODULE_NAME).ko --strip-unneeded

install:
	install -p -m 644 $(MODULE_NAME).ko  $(MODDESTDIR)
	@mkdir -p /lib/firmware/rtlwifi/
	@cp -n rtl8723bs_nic.bin /lib/firmware/rtlwifi/rtl8723bs_nic.bin
	@cp -n rtl8723bs_wowlan.bin /lib/firmware/rtlwifi/rtl8723bs_wowlan.bin
	/sbin/depmod -a ${KVER}

uninstall:
	rm -f $(MODDESTDIR)/$(MODULE_NAME).ko
	/sbin/depmod -a ${KVER}

config_r:
	@echo "make config"
	/bin/bash script/Configure script/config.in

cppcheck: cppcheck.log

cppcheck.log:
	@echo "Creating cppcheck.log"
	cppcheck -f --enable=all -Iinclude -Ihal -Ios_dep  . 2> cppcheck.log

.PHONY: modules clean

clean:
	@rm -fr hal/*/*.mod.c hal/*/*.mod hal/*/*.o hal/*/.*.cmd hal/*/*.ko \
		hal/*.mod.c hal/*.mod hal/*.o hal/.*.cmd hal/*.ko \
		core/*.mod.c core/*.mod core/*.o core/.*.cmd core/*.ko \
		os_dep/*.mod.c os_dep/*.mod os_dep/*.o os_dep/.*.cmd *.ko \
		platform/*.mod.c platform/*.mod platform/*.o platform/.*.cmd platform/*.ko \
		Module.symvers Module.markers modules.order *.mod.c *.mod *.o .*.cmd *.ko *~ .tmp_versions \
		cppcheck.log
