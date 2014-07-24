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
vdso		= vdso_install

do_tools_cpupower = true
do_tools_perf   = true

dtb_files_generic += apm-mustang.dtb
dtb_files_generic += foundation-v8.dtb
dtb_files_generic += rtsm_ve-aemv8a.dtb
