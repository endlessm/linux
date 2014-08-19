human_arch	= PowerPC 64el
build_arch	= powerpc
header_arch	= $(build_arch)
defconfig	= pseries_le_defconfig
flavours	= generic
build_image	= zImage
kernel_file	= arch/$(build_arch)/boot/zImage
install_file	= vmlinuz
no_dumpfile	= true
vdso		= vdso_install
loader		= grub
do_extras_package = true

do_tools_cpupower = true
do_tools_perf	  = true

#do_flavour_image_package = false
