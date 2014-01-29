# Do udebs if not disabled in the arch-specific makefile
binary-udebs: binary-debs
	@echo Debug: $@
ifeq ($(disable_d_i),)
	@$(MAKE) --no-print-directory -f $(DROOT)/rules DEBIAN=$(DEBIAN) \
		do-binary-udebs
endif

do-binary-udebs: debian/control
	@echo Debug: $@
	dh_testdir
	dh_testroot

	# unpack the kernels into a temporary directory
	mkdir -p debian/d-i-${arch}

	imagelist=$$(cat $(builddir)/kernel-versions | grep ^${arch} | gawk '{print $$4}') && \
	for i in $$imagelist; do \
	  dpkg -x $$(ls ../linux-image-$$i\_$(release)-$(revision)_${arch}.deb) \
		debian/d-i-${arch}; \
	  if [ -f ../linux-image-extra-$$i\_$(release)-$(revision)_${arch}.deb ] ; then \
	    dpkg -x ../linux-image-extra-$$i\_$(release)-$(revision)_${arch}.deb \
		  debian/d-i-${arch}; \
	  fi; \
	  /sbin/depmod -b debian/d-i-${arch} $$i; \
	done

	# kernel-wedge will error if no modules unless this is touched
	touch $(CURDIR)/debian/build/no-modules

	touch ignore-dups
	export SOURCEDIR=$(CURDIR)/debian/d-i-${arch} && \
	  cd $(builddir) && \
	  kernel-wedge install-files && \
	  kernel-wedge check

        # Build just the udebs
	dilist=$$(dh_listpackages -s | grep "\-di$$") && \
	[ -z "$dilist" ] || \
	for i in $$dilist; do \
	  dh_fixperms -p$$i; \
	  $(lockme) dh_gencontrol -p$$i; \
	  dh_builddeb -p$$i; \
	done
	
	# Generate the meta-udeb dependancy lists.
	@gawk '										\
		/^Package:/ {								\
			package=$$2; flavour=""; parch="" }				\
		(/Package-Type: udeb/ && package !~ /^'$(src_pkg_name)'-udebs-/) {      \
			match(package, "'$(release)'-'$(abinum)'-(.*)-di", bits);       \
			flavour = bits[1];						\
		}									\
		(/^Architecture:/ && $$0 " " ~ / '$(arch)'/) {				\
			parch=$$0;							\
		}									\
		(flavour != "" && parch != "") {					\
			udebs[flavour] = udebs[flavour] package ", ";			\
			flavour=""; parch="";						\
		}                                                      			\
		END {                                                  			\
			for (flavour in udebs) {					\
				package="'$(src_pkg_name)'-udebs-" flavour;		\
				file="debian/" package ".substvars";			\
				print("udeb:Depends=" udebs[flavour]) > file;		\
				metas="'$(builddir)'/udeb-meta-packages";		\
				print(package) >metas					\
			}								\
		}									\
	' <$(CURDIR)/debian/control
	@while read i; do \
		$(lockme) dh_gencontrol -p$$i; \
		dh_builddeb -p$$i; \
	done <$(builddir)/udeb-meta-packages
