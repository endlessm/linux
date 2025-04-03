build_arch     = riscv
defconfig      = defconfig
flavours       = generic
build_image    = Image
kernel_file    = arch/$(build_arch)/boot/Image
install_file   = vmlinuz

vdso           = vdso_install
no_dumpfile    = true

do_flavour_image_package = false
do_tools                 = true
do_flavour_header_package = false
do_common_headers_indep = false
do_extras_package      = false
do_tools_usbip         = false
do_tools_cpupower      = false
do_tools_perf          = true
do_tools_perf_jvmti    = true
do_tools_perf_python = true
do_tools_bpftool       = true
do_dtbs                = false
