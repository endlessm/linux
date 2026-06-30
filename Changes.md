Driver-related patches (dropped at every major release if they are not yet upstream):

Ubuntu-specific features not supported anymore:

7.1
 - UBUNTU: SAUCE: import Huawei ES3000_V2 (2.1.0.23)
 - UBUNTU: SAUCE: cdc-acm: Exclude Exar USB serial ports
 - UBUNTU: SAUCE: riscv: mm: Force disable sv57
 - UBUNTU: SAUCE: make ASYNCB_INITIALIZED available for the kernel
 - UBUNTU: SAUCE: Microsemi PCIe expansion board DT entry.
 - UBUNTU: SAUCE: riscv: sifive: fu740: cpu{1, 2, 3, 4} set compatible to sifive, u74-mc
 - UBUNTU: SAUCE: md/raid0: Link to wiki with guidance on multi-zone RAID0 layout migration
 - UBUNTU: SAUCE: md/raid0: Use kernel specific layout
 - UBUNTU: SAUCE: Revert "scsi: libsas: allow async aborts"
 - UBUNTU: SAUCE: cacheinfo: Check for null last-level cache info
 - UBUNTU: SAUCE: (no-up) PCI: fix system hang issue of Marvell SATA host controller
 - UBUNTU: SAUCE: (no-up) intel_ips: blacklist ASUSTek G60JX laptops
 - UBUNTU: SAUCE: (no-up) arm64: gicv3: its: Increase FORCE_MAX_ZONEORDER for Cavium ThunderX
 - UBUNTU: SAUCE: [nf,v2] netfilter: x_tables: don't rely on well-behaving userspace
 - UBUNTU: SAUCE: x86/PCI: Export find_cap() to be used in early PCI code
 - UBUNTU: SAUCE: x86/quirks: Add parameter to clear MSIs early on boot
 - UBUNTU: SAUCE: x86/quirks: Scan all busses for early PCI quirks
 - UBUNTU: SAUCE: kthread: Do not leave kthread_create() immediately upon SIGKILL.

7.2:
 - UBUNTU: SAUCE: ACPI: scan: Update HID for new platform
 - UBUNTU: SAUCE: media: platform: amd: Add isp4 fw and hw interface
 - UBUNTU: SAUCE: media: platform: amd: isp4 video node and buffers handling added
