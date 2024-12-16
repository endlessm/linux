# Check the signature of staging modules
module-signature-check-%: $(stampdir)/stamp-install-%
	@echo Debug: $@
	debian/scripts/checks/module-signature-check "$*" \
		"debian/$(mods_pkg_name)-$*" \
		"debian/$(mods_extra_pkg_name)-$*" \
		$(do_skip_checks)

checks-%: module-signature-check-%
	@echo Debug: $@
