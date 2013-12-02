human_arch	= PowerPC 64el
build_arch	= powerpc
header_arch	= $(build_arch)
defconfig	= pseries_le_defconfig
flavours	= generic
build_image	= vmlinux.strip
kernel_file     = arch/powerpc/boot/vmlinux.strip
install_file	= vmlinux
no_dumpfile	= true
loader		= yaboot

do_tools	= false
#do_flavour_image_package = false
