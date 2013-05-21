human_arch	= PowerPC 64el
build_arch	= powerpc
header_arch	= $(build_arch)
defconfig	= ppc64_defconfig
flavours	=
build_image	= vmlinux
kernel_file	= $(build_image)
install_file	= $(build_image)
no_dumpfile	= true
loader		= yaboot

do_tools	= false
do_flavour_image_package = false
