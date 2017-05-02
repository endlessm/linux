ccflags-y += -I$(src)/

phydm_mod-objs := \
		phydm_debug.o	\
		phydm_antdiv.o\
		phydm_interface.o\
		phydm_hwconfig.o\
		phydm.o\
		halphyrf_ce.o\
		phydm_edcaturbocheck.o\
		phydm_dig.o\
		phydm_rainfo.o\
		phydm_dynamicbbpowersaving.o\
		phydm_powertracking_ce.o\
		phydm_dynamictxpower.o\
		phydm_adaptivity.o\
		phydm_cfotracking.o\
		phydm_noisemonitor.o\
		phydm_acs.o\
		phydm_psd.o\
		phydm_adc_sampling.o\
		phydm_kfree.o\
		phydm_ccx.o

phydm_mod-objs += \
		rtl8822b/halhwimg8822b_bb.o\
		rtl8822b/halhwimg8822b_mac.o\
		rtl8822b/halhwimg8822b_rf.o\
		rtl8822b/halphyrf_8822b.o\
		rtl8822b/phydm_hal_api8822b.o\
		rtl8822b/phydm_iqk_8822b.o\
		rtl8822b/phydm_regconfig8822b.o\
		rtl8822b/phydm_rtl8822b.o

phydm_mod-objs += \
		rtl_phydm.o

obj-m = phydm_mod.o
