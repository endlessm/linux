human_arch	= ARM (hard float)
build_arch	= arm
header_arch	= arm
defconfig	= defconfig
flavours	= generic generic-lpae
build_image	= zImage
kernel_file	= arch/$(build_arch)/boot/zImage
install_file	= vmlinuz
no_dumpfile	= true

loader		= grub

do_tools_cpupower = true
do_tools_perf	= true

# Flavour specific configuration.
dtb_files_generic	+= highbank.dtb
dtb_files_generic	+= imx6q-sabrelite.dtb
dtb_files_generic	+= imx6dl-wandboard.dtb
dtb_files_generic	+= imx6q-wandboard.dtb
dtb_files_generic	+= omap3-beagle-xm.dtb
dtb_files_generic	+= omap4-panda.dtb
dtb_files_generic	+= omap4-panda-es.dtb
dtb_files_generic	+= tegra20-harmony.dtb
dtb_files_generic	+= tegra20-paz00.dtb
dtb_files_generic	+= tegra20-seaboard.dtb
dtb_files_generic	+= tegra20-trimslice.dtb
dtb_files_generic	+= tegra20-ventana.dtb
dtb_files_generic	+= tegra20-whistler.dtb
dtb_files_generic	+= am335x-bone.dtb
dtb_files_generic	+= am335x-boneblack.dtb

dtb_files_generic-lpae   += highbank.dtb
dtb_files_generic-lpae	 += vexpress-v2p-ca15-tc1.dtb
