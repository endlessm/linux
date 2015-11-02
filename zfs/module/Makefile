subdir-m += avl
subdir-m += nvpair
subdir-m += unicode
subdir-m += zcommon
subdir-m += zfs
subdir-m += zpios

INSTALL_MOD_DIR ?= extra

ZFS_MODULE_CFLAGS += -include /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/spl/spl_config.h
ZFS_MODULE_CFLAGS += -include /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/zfs/zfs_config.h
ZFS_MODULE_CFLAGS += -I/home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/zfs/include -I/home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/spl/include -I/home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/spl
export ZFS_MODULE_CFLAGS

modules:
	@# Make the exported SPL symbols available to these modules.
	@# They may be in the root of SPL_OBJ when building against
	@# installed devel headers, or they may be in the module
	@# subdirectory when building against the spl source tree.
	@if [ -f /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/spl/Module.symvers ]; then \
		/bin/cp /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/spl/Module.symvers .; \
	elif [ -f /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/spl/module/Module.symvers ]; then \
		/bin/cp /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/spl/module/Module.symvers .; \
	else \
		echo -e "\n" \
		"*** Missing spl symbols ensure you have built the spl:\n" \
		"*** - /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/spl/Module.symvers, or\n" \
		"*** - /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic/spl/module/Module.symvers\n"; \
		exit 1; \
	fi
	$(MAKE) -C /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic SUBDIRS=`pwd`  O=/home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic CONFIG_ZFS=m $@

clean:
	@# Only cleanup the kernel build directories when CONFIG_KERNEL
	@# is defined.  This indicates that kernel modules should be built.
	$(MAKE) -C /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic SUBDIRS=`pwd`  O=/home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic $@

	if [ -f Module.symvers ]; then $(RM) Module.symvers; fi
	if [ -f Module.symvers ]; then $(RM) Module.symvers; fi
	if [ -f Module.markers ]; then $(RM) Module.markers; fi

modules_install:
	@# Install the kernel modules
	$(MAKE) -C /home/rtg/ukb/xenial/amd64/master-next/ubuntu-xenial/debian/build/build-generic SUBDIRS=`pwd` $@ \
		INSTALL_MOD_PATH=$(DESTDIR)$(INSTALL_MOD_PATH) \
		INSTALL_MOD_DIR=$(INSTALL_MOD_DIR) \
		KERNELRELEASE=4.3.0-0-generic
	@# Remove extraneous build products when packaging
	kmoddir=$(DESTDIR)$(INSTALL_MOD_PATH)/lib/modules/4.3.0-0-generic; \
	if [ -n $$kmoddir ]; then \
		find $$kmoddir -name 'modules.*' | xargs $(RM); \
	fi
	sysmap=$(DESTDIR)$(INSTALL_MOD_PATH)/boot/System.map-4.3.0-0-generic; \
	if [ -f $$sysmap ]; then \
		depmod -ae -F $$sysmap 4.3.0-0-generic; \
	fi

modules_uninstall:
	@# Uninstall the kernel modules
	kmoddir=$(DESTDIR)$(INSTALL_MOD_PATH)/lib/modules/4.3.0-0-generic
	list='$(subdir-m)'; for subdir in $$list; do \
		$(RM) -R $$kmoddir/$(INSTALL_MOD_DIR)/$$subdir; \
	done

distdir:
	list='$(subdir-m)'; for subdir in $$list; do \
		(find ../module/$$subdir -name '*.c' -o -name '*.h' |\
		xargs /bin/cp -t $$distdir/$$subdir); \
	done

distclean maintainer-clean: clean
install: modules_install
uninstall: modules_uninstall
all: modules
check:
