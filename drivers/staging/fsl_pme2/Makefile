# PME
obj-$(CONFIG_FSL_PME2_CTRL)	+= pme2_ctrl.o pme2_sysfs.o
obj-$(CONFIG_FSL_PME2_PORTAL)	+= pme2.o
pme2-y				:= pme2_low.o pme2_high.o
obj-$(CONFIG_FSL_PME2_TEST_HIGH) += pme2_test_high.o
obj-$(CONFIG_FSL_PME2_TEST_SCAN) += pme2_test_scanning.o
pme2_test_scanning-y		= pme2_test_scan.o pme2_sample_db.o
obj-$(CONFIG_FSL_PME2_DB)       += pme2_db.o
obj-$(CONFIG_FSL_PME2_SCAN)       += pme2_scan.o

