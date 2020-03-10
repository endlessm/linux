human_arch      = RISC-V
build_arch      = riscv
header_arch     = $(build_arch)
defconfig       = defconfig
flavours        = generic
build_image	= Image
kernel_file	= arch/$(build_arch)/boot/Image
install_file	= vmlinuz

loader		= grub
vdso		= vdso_install
no_dumpfile	= true

do_extras_package   = true
do_tools_usbip      = true
do_tools_cpupower   = true
do_tools_perf       = true
do_tools_perf_jvmti = true
do_tools_bpftool    = true

do_dkms_wireguard = true
