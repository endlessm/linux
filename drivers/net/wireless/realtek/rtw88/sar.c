// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2018-2019  Realtek Corporation
 */

#include <linux/acpi.h>
#include "main.h"
#include "debug.h"
#include "phy.h"
#include "sar.h"

#define RTW_SAR_WRDS_CHAIN_NR	2

enum rtw_sar_limit_index {
	RTW_SAR_LMT_CH1_14,
	RTW_SAR_LMT_CH36_64,
	RTW_SAR_LMT_UND1,
	RTW_SAR_LMT_CH100_144,
	RTW_SAR_LMT_CH149_165,

	RTW_SAR_LMT_TOTAL_NR,
};

struct rtw_sar_limits {
	s8 limit[RTW_SAR_LMT_TOTAL_NR];
};

struct rtw_sar_wrds {
	struct rtw_sar_limits chain[RTW_SAR_WRDS_CHAIN_NR];
};

#define ACPI_WRDS_METHOD	"WRDS"
#define ACPI_WRDS_SIZE		sizeof(struct rtw_sar_wrds)
#define ACPI_WRDS_TOTAL_SIZE	(sizeof(struct rtw_sar_wrds) + 2)
#define ACPI_WIFI_DOMAIN	0x07

#ifdef CONFIG_ACPI
static union acpi_object *rtw_sar_get_acpiobj(struct rtw_dev *rtwdev,
					      const char *method)
{
	struct device *dev = rtwdev->dev;
	acpi_handle root_handle;
	acpi_handle handle;
	acpi_status status;
	struct acpi_buffer buf = {ACPI_ALLOCATE_BUFFER, NULL};

	/* Check device handler */
	root_handle = ACPI_HANDLE(dev);
	if (!root_handle) {
		rtw_dbg(rtwdev, RTW_DBG_REGD,
			"SAR: Could not retireve root port ACPI handle\n");
		return NULL;
	}

	/* Get method's handler */
	status = acpi_get_handle(root_handle, (acpi_string)method, &handle);
	if (ACPI_FAILURE(status)) {
		rtw_dbg(rtwdev, RTW_DBG_REGD, "SAR: %s method not found (0x%x)\n",
			method, status);
		return NULL;
	}

	/* Call specific method with no argument */
	status = acpi_evaluate_object(handle, NULL, NULL, &buf);
	if (ACPI_FAILURE(status)) {
		rtw_dbg(rtwdev, RTW_DBG_REGD,
			"SAR: %s invocation failed (0x%x)\n", method, status);
		return NULL;
	}

	return buf.pointer;
}

static union acpi_object *rtw_sar_get_wifi_pkt(struct rtw_dev *rtwdev,
					       union acpi_object *obj,
					       u32 element_count)
{
	union acpi_object *wifi_pkg;
	u32 i;

	if (obj->type != ACPI_TYPE_PACKAGE ||
	    obj->package.count < 2 ||
	    obj->package.elements[0].type != ACPI_TYPE_INTEGER ||
	    obj->package.elements[0].integer.value != 0) {
		rtw_dbg(rtwdev, RTW_DBG_REGD,
			"SAR: Unsupported wifi package structure\n");
		return NULL;
	}

	/* loop through all the packages to find the one for WiFi */
	for (i = 1; i < obj->package.count; i++) {
		union acpi_object *domain;

		wifi_pkg = &obj->package.elements[i];

		/* Skip anything that is not a package with the right amount of
		 * elements (i.e. domain_type, enabled/disabled plus the sar
		 * table size.)
		 */
		if (wifi_pkg->type != ACPI_TYPE_PACKAGE ||
		    wifi_pkg->package.count != element_count)
			continue;

		domain = &wifi_pkg->package.elements[0];
		if (domain->type == ACPI_TYPE_INTEGER &&
		    domain->integer.value == ACPI_WIFI_DOMAIN)
			return wifi_pkg;
	}

	return NULL;
}

static void *rtw_sar_get_wrds_table(struct rtw_dev *rtwdev)
{
	union acpi_object *wrds, *wrds_pkg;
	int i, idx = 2;
	u8 *wrds_raw = NULL;

	wrds = rtw_sar_get_acpiobj(rtwdev, ACPI_WRDS_METHOD);
	if (!wrds)
		return NULL;

	wrds_pkg = rtw_sar_get_wifi_pkt(rtwdev, wrds, ACPI_WRDS_TOTAL_SIZE);
	if (!wrds_pkg)
		goto out;

	/* WiFiSarEnable 0: ignore BIOS config; 1: use BIOS config */
	if (wrds_pkg->package.elements[1].type != ACPI_TYPE_INTEGER ||
	    wrds_pkg->package.elements[1].integer.value == 0)
		goto out;

	wrds_raw = kmalloc(ACPI_WRDS_SIZE, GFP_KERNEL);
	if (!wrds_raw)
		goto out;

	/* read elements[2~11] */
	for (i = 0; i < ACPI_WRDS_SIZE; i++) {
		union acpi_object *entry;

		entry = &wrds_pkg->package.elements[idx++];
		if (entry->type != ACPI_TYPE_INTEGER ||
		    entry->integer.value > U8_MAX) {
			kfree(wrds_raw);
			wrds_raw = NULL;
			goto out;
		}

		wrds_raw[i] = entry->integer.value;
	}
out:
	kfree(wrds);

	return wrds_raw;
}

static void rtw_sar_apply_wrds(struct rtw_dev *rtwdev,
			       const struct rtw_sar_wrds *wrds)
{
	int path;

	for (path = 0; path < RTW_SAR_WRDS_CHAIN_NR; path++) {
		rtw_phy_set_tx_power_sar(rtwdev, RTW_REGD_WW, path, 1, 14,
					 wrds->chain[path].limit[RTW_SAR_LMT_CH1_14]);
		rtw_phy_set_tx_power_sar(rtwdev, RTW_REGD_WW, path, 36, 64,
					 wrds->chain[path].limit[RTW_SAR_LMT_CH36_64]);
		rtw_phy_set_tx_power_sar(rtwdev, RTW_REGD_WW, path, 100, 144,
					 wrds->chain[path].limit[RTW_SAR_LMT_CH100_144]);
		rtw_phy_set_tx_power_sar(rtwdev, RTW_REGD_WW, path, 149, 165,
					 wrds->chain[path].limit[RTW_SAR_LMT_CH149_165]);
	}

	rtwdev->sar.source = RTW_SAR_SOURCE_ACPI_STATIC;
}

static int rtw_sar_load_static_tables(struct rtw_dev *rtwdev)
{
	struct rtw_sar_wrds *wrds;

	wrds = rtw_sar_get_wrds_table(rtwdev);
	if (!wrds)
		return -ENOENT;

	rtw_dbg(rtwdev, RTW_DBG_REGD,
		"SAR: Apply WRDS to TX power\n");

	rtw_sar_apply_wrds(rtwdev, wrds);
	kfree(wrds);

	return 0;
}
#else
static int rtw_sar_load_static_tables(struct rtw_dev *rtwdev)
{
	return -ENOENT;
}
#endif /* CONFIG_ACPI */

void rtw_sar_load_table(struct rtw_dev *rtwdev)
{
	rtw_sar_load_static_tables(rtwdev);
}
