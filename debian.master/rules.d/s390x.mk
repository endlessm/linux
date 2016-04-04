human_arch      = System 390x
build_arch      = s390
header_arch     = $(build_arch)
defconfig       = defconfig
flavours        = generic
build_image	= image
kernel_file	= arch/$(build_arch)/boot/image
install_file	= vmlinuz

vdso		= vdso_install
no_dumpfile	= true

do_extras_package = true

do_tools_usbip    = true
do_tools_cpupower = true
do_tools_perf     = true

do_zfs		= true
