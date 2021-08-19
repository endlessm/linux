# The following targets are for the maintainer only! do not run if you don't
# know what they do.

.PHONY: printenv updateconfigs printchanges insertchanges startnewrelease diffupstream help updateportsconfigs editportsconfigs autoreconstruct finalchecks

help:
	@echo "These are the targets in addition to the normal $(DEBIAN) ones:"
	@echo
	@echo "  printenv        : Print some variables used in the build"
	@echo
	@echo "  updateconfigs        : Update core arch configs"
	@echo
	@echo "  editconfigs          : Update core arch configs interractively"
	@echo "  genconfigs           : Generate core arch configs in CONFIGS/*"
	@echo
	@echo "  updateportsconfigs   : Update ports arch configs"
	@echo
	@echo "  editportsconfigs     : Update ports arch configs interactivly"
	@echo "  genportconfigs       : Generate ports arch configs in CONFIGS/*"
	@echo
	@echo "  printchanges    : Print the current changelog entries (from git)"
	@echo
	@echo "  insertchanges   : Insert current changelog entries (from git)"
	@echo
	@echo "  startnewrelease : Start a new changelog set"
	@echo
	@echo "  diffupstream    : Diff stock kernel code against upstream (git)"
	@echo
	@echo "  compileselftests : Only compile the selftests listed on ubuntu_selftests variable"
	@echo
	@echo "  runselftests    : Run the selftests listed on ubuntu_selftests variable"
	@echo
	@echo "  help            : If you are kernel hacking, you need the professional"
	@echo "                    version of this"
	@echo
	@echo "Environment variables:"
	@echo
	@echo "  NOKERNLOG       : Do not add upstream kernel commits to changelog"
	@echo "  CONCURRENCY_LEVEL=X"
	@echo "                  : Use -jX for kernel compile"
	@echo "  PRINTSHAS       : Include SHAs for commits in changelog"

printdebian:
	@echo "$(DEBIAN)"

updateconfigs defaultconfigs editconfigs genconfigs dumpconfigs:
	dh_testdir;
	$(SHELL) $(DROOT)/scripts/misc/kernelconfig $@ "$(do_enforce_all)"
	rm -rf build

updateportsconfigs defaultportsconfigs editportsconfigs genportsconfigs askconfigs:
	dh_testdir;
	$(SHELL) $(DROOT)/scripts/misc/kernelconfig $@ ports
	rm -rf build

printenv:
	dh_testdir
	@echo "src package name  = $(src_pkg_name)"
	@echo "series            = $(series)"
	@echo "release           = $(release)"
	@echo "revisions         = $(revisions)"
	@echo "revision          = $(revision)"
	@echo "uploadnum         = $(uploadnum)"
	@echo "prev_revisions    = $(prev_revisions)"
	@echo "prev_revision     = $(prev_revision)"
	@echo "abinum            = $(abinum)"
	@echo "upstream_tag      = $(upstream_tag)"
	@echo "gitver            = $(gitver)"
	@echo "variants          = $(variants)"
	@echo "flavours          = $(flavours)"
	@echo "skipabi           = $(skipabi)"
	@echo "skipmodule        = $(skipmodule)"
	@echo "skipdbg           = $(skipdbg)"
	@echo "ubuntu_log_opts   = $(ubuntu_log_opts)"
	@echo "CONCURRENCY_LEVEL = $(CONCURRENCY_LEVEL)"
	@echo "ubuntu_selftests  = $(ubuntu_selftests)"
	@echo "bin package name  = $(bin_pkg_name)"
	@echo "hdr package name  = $(hdrs_pkg_name)"
	@echo "doc package name  = $(doc_pkg_name)"
	@echo "do_doc_package            = $(do_doc_package)"
	@echo "do_doc_package_content    = $(do_doc_package_content)"
	@echo "do_source_package         = $(do_source_package)"
	@echo "do_source_package_content = $(do_source_package_content)"
	@echo "do_libc_dev_package       = $(do_libc_dev_package)"
	@echo "do_flavour_image_package  = $(do_flavour_image_package)"
	@echo "do_flavour_header_package = $(do_flavour_header_package)"
	@echo "do_common_headers_indep   = $(do_common_headers_indep)"
	@echo "do_full_source            = $(do_full_source)"
	@echo "do_tools                  = $(do_tools)"
	@echo "do_any_tools              = $(do_any_tools)"
	@echo "do_linux_tools            = $(do_linux_tools)"
	@echo " do_tools_cpupower         = $(do_tools_cpupower)"
	@echo " do_tools_perf             = $(do_tools_perf)"
	@echo " do_tools_bpftool          = $(do_tools_bpftool)"
	@echo " do_tools_x86              = $(do_tools_x86)"
	@echo " do_tools_host             = $(do_tools_host)"
	@echo "do_cloud_tools            = $(do_cloud_tools)"
	@echo " do_tools_hyperv           = $(do_tools_hyperv)"
	@echo "any_signed                = $(any_signed)"
	@echo " uefi_signed               = $(uefi_signed)"
	@echo " opal_signed               = $(opal_signed)"
	@echo " sipl_signed               = $(sipl_signed)"
	@echo "full_build                = $(full_build)"
	@echo "libc_dev_version          = $(libc_dev_version)"
	@echo "DEB_HOST_GNU_TYPE         = $(DEB_HOST_GNU_TYPE)"
	@echo "DEB_BUILD_GNU_TYPE        = $(DEB_BUILD_GNU_TYPE)"
	@echo "DEB_HOST_ARCH             = $(DEB_HOST_ARCH)"
	@echo "DEB_BUILD_ARCH            = $(DEB_BUILD_ARCH)"
	@echo "arch                      = $(arch)"
	@echo "kmake                     = $(kmake)"
	@echo "disable_d_i               = $(disable_d_i)"

printchanges:
	@baseCommit=$$(git log --pretty=format:'%H %s' | \
		gawk '/UBUNTU: '".*Ubuntu-.*`echo $(prev_fullver) | sed 's/+/\\\\+/'`"'(~.*)?$$/ { print $$1; exit }'); \
	if [ -z "$$baseCommit" ]; then \
		echo "WARNING: couldn't find a commit for the previous version. Using the lastest one." >&2; \
		baseCommit=$$(git log --pretty=format:'%H %s' | \
			gawk '/UBUNTU:\s*Ubuntu-.*$$/ { print $$1; exit }'); \
	fi; \
	git log "$$baseCommit"..HEAD | \
	$(DROOT)/scripts/misc/git-ubuntu-log $(ubuntu_log_opts)

insertchanges: autoreconstruct finalchecks
	@perl -w -f $(DROOT)/scripts/misc/insert-changes.pl $(DROOT) $(DEBIAN) 

autoreconstruct:
	# No need for reconstruct for -rc kernels since we don't upload an
	# orig tarball, so just remove it.
	if grep -q "^EXTRAVERSION = -rc[0-9]\+$$" Makefile; then \
		echo "exit 0" >$(DEBIAN)/reconstruct; \
	else \
		$(DROOT)/scripts/misc/gen-auto-reconstruct $(upstream_tag) $(DEBIAN)/reconstruct $(DROOT)/source/options; \
	fi

finalchecks: debian/control
	$(DROOT)/scripts/misc/final-checks "$(DEBIAN)" "$(prev_fullver)"

diffupstream:
	@git diff-tree -p refs/remotes/linux-2.6/master..HEAD $(shell ls | grep -vE '^(ubuntu|$(DEBIAN)|\.git.*)')

startnewrelease:
	dh_testdir
	@[ -f "$(DEBIAN)/etc/update.conf" ] && . "$(DEBIAN)/etc/update.conf"; \
	if [ -n "$$BACKPORT_SUFFIX" ]; then \
		ver="$$(dpkg-parsechangelog -l"$$DEBIAN_MASTER/changelog" -SVersion)$${BACKPORT_SUFFIX}.1"; \
		prev_ver="$$(dpkg-parsechangelog -l"$(DEBIAN)/changelog" -SVersion)"; \
		if [ "$${ver%.*}" = "$${prev_ver%.*}" ]; then \
			ver="$${ver%.*}.$$(( $${prev_ver##*.} +1 ))"; \
		fi; \
	else \
		ver="$(release)-$$(echo "$(revision)" | \
			perl -ne 'if (/^(\d*)\.(\d*)(.*)?$$/) { printf("%d.%d%s\n", $$1 + 1, $$2 +1, $$3) }')"; \
	fi; \
	now="$(shell date -R)"; \
	echo "Creating new changelog set for $$ver..."; \
	echo -e "$(src_pkg_name) ($$ver) UNRELEASED; urgency=medium\n" > $(DEBIAN)/changelog.new; \
	echo "  CHANGELOG: Do not edit directly. Autogenerated at release." >> \
		$(DEBIAN)/changelog.new; \
	echo "  CHANGELOG: Use the printchanges target to see the curent changes." \
		>> $(DEBIAN)/changelog.new; \
	echo "  CHANGELOG: Use the insertchanges target to create the final log." \
		>> $(DEBIAN)/changelog.new; \
	echo -e "\n -- $$DEBFULLNAME <$$DEBEMAIL>  $$now\n" >> \
		$(DEBIAN)/changelog.new ; \
	cat $(DEBIAN)/changelog >> $(DEBIAN)/changelog.new; \
	mv $(DEBIAN)/changelog.new $(DEBIAN)/changelog

compileselftests:
	# a loop is needed here to fail on errors
	for test in $(ubuntu_selftests); do \
		$(kmake) -C tools/testing/selftests TARGETS="$$test"; \
	done;

runselftests:
	$(kmake) -C tools/testing/selftests TARGETS="$(ubuntu_selftests)" run_tests
