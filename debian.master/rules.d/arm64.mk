human_arch	= ARMv8
build_arch	= arm64
header_arch	= arm64
defconfig	= defconfig
flavours	= generic
build_image	= Image
kernel_file	= arch/$(build_arch)/boot/Image
install_file	= vmlinuz
no_dumpfile = true

loader		= grub
do_tools	= false
#do_flavour_image_package = false
#disable_d_i	= true
