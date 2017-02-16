// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2014 Intel Corporation; author Matt Fleming
 * Copyright (c) 2014 Red Hat, Inc., Mark Salter <msalter@redhat.com>
 */
#include <linux/efi.h>
#include <linux/reboot.h>
#include <linux/dmi.h>

static void (*orig_pm_power_off)(void);

int efi_reboot_quirk_mode = -1;

void efi_reboot(enum reboot_mode reboot_mode, const char *__unused)
{
	const char *str[] = { "cold", "warm", "shutdown", "platform" };
	int efi_mode, cap_reset_mode;

	if (!efi_enabled(EFI_RUNTIME_SERVICES))
		return;

	switch (reboot_mode) {
	case REBOOT_WARM:
	case REBOOT_SOFT:
		efi_mode = EFI_RESET_WARM;
		break;
	default:
		efi_mode = EFI_RESET_COLD;
		break;
	}

	/*
	 * If a quirk forced an EFI reset mode, always use that.
	 */
	if (efi_reboot_quirk_mode != -1)
		efi_mode = efi_reboot_quirk_mode;

	if (efi_capsule_pending(&cap_reset_mode)) {
		if (efi_mode != cap_reset_mode)
			printk(KERN_CRIT "efi: %s reset requested but pending "
			       "capsule update requires %s reset... Performing "
			       "%s reset.\n", str[efi_mode], str[cap_reset_mode],
			       str[cap_reset_mode]);
		efi_mode = cap_reset_mode;
	}

	efi.reset_system(efi_mode, EFI_SUCCESS, 0, NULL);
}

static const struct dmi_system_id force_efi_poweroff[] = {
        {
                .ident = "Packard Bell Easynote ENLG81AP",
                .matches = {
                        DMI_MATCH(DMI_SYS_VENDOR, "Packard Bell"),
                        DMI_MATCH(DMI_PRODUCT_NAME, "Easynote ENLG81AP"),
                },
        },
        {
                .ident = "Packard Bell Easynote ENTE69AP",
                .matches = {
                        DMI_MATCH(DMI_SYS_VENDOR, "Packard Bell"),
                        DMI_MATCH(DMI_PRODUCT_NAME, "Easynote ENTE69AP"),
                },
        },
        {
                .ident = "Acer Aspire ES1-533",
                .matches = {
                        DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
                        DMI_MATCH(DMI_PRODUCT_NAME, "Aspire ES1-533"),
                },
        },
        {
                .ident = "Acer Aspire ES1-732",
                .matches = {
                        DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
                        DMI_MATCH(DMI_PRODUCT_NAME, "Aspire ES1-732"),
                },
        },
        {}
};

bool efi_poweroff_forced(void)
{
	if (dmi_check_system(force_efi_poweroff))
		return true;
	return false;
}

bool __weak efi_poweroff_required(void)
{
	return false;
}

static void efi_power_off(void)
{
	efi.reset_system(EFI_RESET_SHUTDOWN, EFI_SUCCESS, 0, NULL);
	/*
	 * The above call should not return, if it does fall back to
	 * the original power off method (typically ACPI poweroff).
	 */
	if (orig_pm_power_off)
		orig_pm_power_off();
}

static int __init efi_shutdown_init(void)
{
	if (!efi_enabled(EFI_RUNTIME_SERVICES))
		return -ENODEV;

	if (efi_poweroff_required() || efi_poweroff_forced()) {
		orig_pm_power_off = pm_power_off;
		pm_power_off = efi_power_off;
	}

	return 0;
}
late_initcall(efi_shutdown_init);
