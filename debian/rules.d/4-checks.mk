# Check ABI for package against last release (if not same abinum)
abi-check-%: $(stampdir)/stamp-install-%
	@echo Debug: $@
	@perl -f $(DROOT)/scripts/abi-check "$*" "$(prev_abinum)" "$(abinum)" \
		"$(prev_abidir)" "$(abidir)" "$(skipabi)"

# Check the module list against the last release (always)
module-check-%: $(stampdir)/stamp-install-%
	@echo Debug: $@
	$(DROOT)/scripts/module-check "$*" \
		"$(prev_abidir)" "$(abidir)" $(skipmodule)

# Check the signature of staging modules
module-signature-check-%: $(stampdir)/stamp-install-%
	@echo Debug: $@
	$(DROOT)/scripts/module-signature-check "$*" \
		"$(DROOT)/$(mods_pkg_name)-$*" \
		"$(DROOT)/$(mods_extra_pkg_name)-$*"

# Check the reptoline jmp/call functions against the last release.
retpoline-check-%: $(stampdir)/stamp-install-%
	@echo Debug: $@
	$(SHELL) $(DROOT)/scripts/retpoline-check "$*" \
		"$(prev_abidir)" "$(abidir)" "$(skipretpoline)" "$(builddir)/build-$*"

checks-%: module-check-% module-signature-check-% abi-check-% retpoline-check-%
	@echo Debug: $@

# Check the config against the known options list.
config-prepare-check-%: $(stampdir)/stamp-prepare-tree-%
	@echo Debug: $@
	@perl -f $(DROOT)/scripts/config-check \
		$(builddir)/build-$*/.config "$(arch)" "$*" "$(commonconfdir)" \
		"$(skipconfig)" "$(do_enforce_all)"

