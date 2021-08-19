human_arch	= ARMv8
build_arch	= arm64
header_arch	= arm64
defconfig	= defconfig
flavours	= generic generic-64k
build_image	= Image.gz
kernel_file	= arch/$(build_arch)/boot/Image.gz
install_file	= vmlinuz
no_dumpfile = true
uefi_signed     = true

vdso		= vdso_install

do_extras_package = true
do_tools_usbip  = true
do_tools_cpupower = true
do_tools_perf   = true
do_tools_perf_jvmti = true
do_tools_bpftool = true

do_dtbs		= true
do_zfs		= true
