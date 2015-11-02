
MODULE := zunicode

EXTRA_CFLAGS = $(ZFS_MODULE_CFLAGS) -DHAVE_SPL

obj-$(CONFIG_ZFS) := $(MODULE).o

$(MODULE)-objs += u8_textprep.o
$(MODULE)-objs += uconv.o
