RTL871X := rtl8822b
EXTRA_CFLAGS += -DCONFIG_RTL8822B

ifeq ($(CONFIG_USB_HCI), y)
ifeq ($(CONFIG_BT_COEXIST), n)
MODULE_NAME = 8812bu
else
MODULE_NAME = 88x2bu
endif
endif
ifeq ($(CONFIG_PCI_HCI), y)
MODULE_NAME = 88x2be
endif
ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME = 88x2bs
endif

ifeq ($(CONFIG_MP_INCLUDED), y)
### 8822B Default Enable VHT MP HW TX MODE ###
#EXTRA_CFLAGS += -DCONFIG_MP_VHT_HW_TX_MODE
#CONFIG_MP_VHT_HW_TX_MODE = y
endif

_HAL_HALMAC_FILES +=	hal/halmac/halmac_api.o

_HAL_HALMAC_FILES +=	hal/halmac/halmac_88xx/halmac_api_88xx.o \
			hal/halmac/halmac_88xx/halmac_api_88xx_usb.o \
			hal/halmac/halmac_88xx/halmac_api_88xx_sdio.o \
			hal/halmac/halmac_88xx/halmac_api_88xx_pcie.o \
			hal/halmac/halmac_88xx/halmac_func_88xx.o \
			hal/halmac/halmac_88xx/halmac_gpio_88xx.o

_HAL_HALMAC_FILES +=	hal/halmac/halmac_88xx/halmac_8822b/halmac_8822b_phy.o \
			hal/halmac/halmac_88xx/halmac_8822b/halmac_8822b_pwr_seq.o \
			hal/halmac/halmac_88xx/halmac_8822b/halmac_api_8822b.o \
			hal/halmac/halmac_88xx/halmac_8822b/halmac_api_8822b_pcie.o \
			hal/halmac/halmac_88xx/halmac_8822b/halmac_api_8822b_sdio.o \
			hal/halmac/halmac_88xx/halmac_8822b/halmac_api_8822b_usb.o \
			hal/halmac/halmac_88xx/halmac_8822b/halmac_func_8822b.o \
			hal/halmac/halmac_88xx/halmac_8822b/halmac_gpio_8822b.o

_HAL_INTFS_FILES +=	hal/hal_halmac.o

_HAL_INTFS_FILES +=	hal/rtl8822b/rtl8822b_halinit.o \
			hal/rtl8822b/rtl8822b_mac.o \
			hal/rtl8822b/rtl8822b_cmd.o \
			hal/rtl8822b/rtl8822b_phy.o \
			hal/rtl8822b/rtl8822b_ops.o \
			hal/rtl8822b/hal8822b_fw.o

ifeq ($(CONFIG_USB_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8822b/$(HCI_NAME)/rtl8822bu_halinit.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bu_halmac.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bu_io.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bu_xmit.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bu_recv.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bu_led.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bu_ops.o

_HAL_INTFS_FILES +=hal/efuse/rtl8822b/HalEfuseMask8822B_USB.o
endif
ifeq ($(CONFIG_PCI_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8822b/$(HCI_NAME)/rtl8822be_halinit.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822be_halmac.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822be_io.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822be_xmit.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822be_recv.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822be_led.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822be_ops.o

_HAL_INTFS_FILES +=hal/efuse/rtl8822b/HalEfuseMask8822B_PCIE.o
endif
ifeq ($(CONFIG_SDIO_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8822b/$(HCI_NAME)/rtl8822bs_halinit.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bs_halmac.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bs_io.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bs_xmit.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bs_recv.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bs_led.o \
			hal/rtl8822b/$(HCI_NAME)/rtl8822bs_ops.o

_HAL_INTFS_FILES +=hal/efuse/rtl8822b/HalEfuseMask8822B_SDIO.o
endif

_HAL_INTFS_FILES += $(_HAL_HALMAC_FILES)