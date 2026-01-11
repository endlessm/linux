ccflags-y := -I$(src)/../ \
	-Wmaybe-uninitialized

obj-$(CONFIG_IGH_ECAT) += ec_master.o

ec_master-$(CONFIG_IGH_ECAT) := \
	cdev.o \
	coe_emerg_ring.o \
	datagram.o \
	datagram_pair.o \
	device.o \
	domain.o \
	flag.o \
	fmmu_config.o \
	foe_request.o \
	fsm_change.o \
	fsm_coe.o \
	fsm_foe.o \
	fsm_master.o \
	fsm_pdo.o \
	fsm_pdo_entry.o \
	fsm_sii.o \
	fsm_slave.o \
	fsm_slave_config.o \
	fsm_slave_scan.o \
	fsm_soe.o \
	ioctl.o \
	mailbox.o \
	master.o \
	module.o \
	pdo.o \
	pdo_entry.o \
	pdo_list.o \
	reg_request.o \
	sdo.o \
	sdo_entry.o \
	sdo_request.o \
	slave.o \
	slave_config.o \
	soe_errors.o \
	soe_request.o \
	sync.o \
	sync_config.o \
	voe_handler.o

ec_master-$(CONFIG_IGH_ECAT_ENABLE_EOE) += eoe_request.o ethernet.o fsm_eoe.o

