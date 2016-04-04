/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
#include <linux/firmware.h>
#include "i915_drv.h"
#include "i915_reg.h"

/**
 * DOC: csr support for dmc
 *
 * Display Context Save and Restore (CSR) firmware support added from gen9
 * onwards to drive newly added DMC (Display microcontroller) in display
 * engine to save and restore the state of display engine when it enter into
 * low-power state and comes back to normal.
 *
 * Firmware loading status will be one of the below states: FW_UNINITIALIZED,
 * FW_LOADED, FW_FAILED.
 *
 * Once the firmware is written into the registers status will be moved from
 * FW_UNINITIALIZED to FW_LOADED and for any erroneous condition status will
 * be moved to FW_FAILED.
 */

#define I915_CSR_SKL "i915/skl_dmc_ver1.bin"
#define I915_CSR_BXT "i915/bxt_dmc_ver1.bin"

#define FIRMWARE_URL  "https://01.org/linuxgraphics/intel-linux-graphics-firmwares"

MODULE_FIRMWARE(I915_CSR_SKL);
MODULE_FIRMWARE(I915_CSR_BXT);

#define SKL_CSR_VERSION_REQUIRED	CSR_VERSION(1, 23)

#define CSR_MAX_FW_SIZE			0x2FFF
#define CSR_DEFAULT_FW_OFFSET		0xFFFFFFFF

struct intel_css_header {
	/* 0x09 for DMC */
	uint32_t module_type;

	/* Includes the DMC specific header in dwords */
	uint32_t header_len;

	/* always value would be 0x10000 */
	uint32_t header_ver;

	/* Not used */
	uint32_t module_id;

	/* Not used */
	uint32_t module_vendor;

	/* in YYYYMMDD format */
	uint32_t date;

	/* Size in dwords (CSS_Headerlen + PackageHeaderLen + dmc FWsLen)/4 */
	uint32_t size;

	/* Not used */
	uint32_t key_size;

	/* Not used */
	uint32_t modulus_size;

	/* Not used */
	uint32_t exponent_size;

	/* Not used */
	uint32_t reserved1[12];

	/* Major Minor */
	uint32_t version;

	/* Not used */
	uint32_t reserved2[8];

	/* Not used */
	uint32_t kernel_header_info;
} __packed;

struct intel_fw_info {
	uint16_t reserved1;

	/* Stepping (A, B, C, ..., *). * is a wildcard */
	char stepping;

	/* Sub-stepping (0, 1, ..., *). * is a wildcard */
	char substepping;

	uint32_t offset;
	uint32_t reserved2;
} __packed;

struct intel_package_header {
	/* DMC container header length in dwords */
	unsigned char header_len;

	/* always value would be 0x01 */
	unsigned char header_ver;

	unsigned char reserved[10];

	/* Number of valid entries in the FWInfo array below */
	uint32_t num_entries;

	struct intel_fw_info fw_info[20];
} __packed;

struct intel_dmc_header {
	/* always value would be 0x40403E3E */
	uint32_t signature;

	/* DMC binary header length */
	unsigned char header_len;

	/* 0x01 */
	unsigned char header_ver;

	/* Reserved */
	uint16_t dmcc_ver;

	/* Major, Minor */
	uint32_t	project;

	/* Firmware program size (excluding header) in dwords */
	uint32_t	fw_size;

	/* Major Minor version */
	uint32_t fw_version;

	/* Number of valid MMIO cycles present. */
	uint32_t mmio_count;

	/* MMIO address */
	uint32_t mmioaddr[8];

	/* MMIO data */
	uint32_t mmiodata[8];

	/* FW filename  */
	unsigned char dfile[32];

	uint32_t reserved1[2];
} __packed;

struct stepping_info {
	char stepping;
	char substepping;
};

/*
 * Kabylake derivated from Skylake H0, so SKL H0
 * is the right firmware for KBL A0 (revid 0).
 */
static const struct stepping_info kbl_stepping_info[] = {
	{'H', '0'}, {'I', '0'}
};

static const struct stepping_info skl_stepping_info[] = {
	{'A', '0'}, {'B', '0'}, {'C', '0'},
	{'D', '0'}, {'E', '0'}, {'F', '0'},
	{'G', '0'}, {'H', '0'}, {'I', '0'},
	{'J', '0'}, {'K', '0'}
};

static const struct stepping_info bxt_stepping_info[] = {
	{'A', '0'}, {'A', '1'}, {'A', '2'},
	{'B', '0'}, {'B', '1'}, {'B', '2'}
};

static const struct stepping_info *intel_get_stepping_info(struct drm_device *dev)
{
	const struct stepping_info *si;
	unsigned int size;

	if (IS_KABYLAKE(dev)) {
		size = ARRAY_SIZE(kbl_stepping_info);
		si = kbl_stepping_info;
	} else if (IS_SKYLAKE(dev)) {
		size = ARRAY_SIZE(skl_stepping_info);
		si = skl_stepping_info;
	} else if (IS_BROXTON(dev)) {
		size = ARRAY_SIZE(bxt_stepping_info);
		si = bxt_stepping_info;
	} else {
		return NULL;
	}

	if (INTEL_REVID(dev) < size)
		return si + INTEL_REVID(dev);

	return NULL;
}

/**
 * intel_csr_load_program() - write the firmware from memory to register.
 * @dev_priv: i915 drm device.
 *
 * CSR firmware is read from a .bin file and kept in internal memory one time.
 * Everytime display comes back from low power state this function is called to
 * copy the firmware from internal memory to registers.
 */
bool intel_csr_load_program(struct drm_i915_private *dev_priv)
{
	u32 *payload = dev_priv->csr.dmc_payload;
	uint32_t i, fw_size;

	if (!IS_GEN9(dev_priv)) {
		DRM_ERROR("No CSR support available for this platform\n");
		return false;
	}

	if (!dev_priv->csr.dmc_payload) {
		DRM_ERROR("Tried to program CSR with empty payload\n");
		return false;
	}

	fw_size = dev_priv->csr.dmc_fw_size;
	for (i = 0; i < fw_size; i++)
		I915_WRITE(CSR_PROGRAM(i), payload[i]);

	for (i = 0; i < dev_priv->csr.mmio_count; i++) {
		I915_WRITE(dev_priv->csr.mmioaddr[i],
			   dev_priv->csr.mmiodata[i]);
	}

	dev_priv->csr.dc_state = 0;

	return true;
}

static uint32_t *parse_csr_fw(struct drm_i915_private *dev_priv,
			      const struct firmware *fw)
{
	struct drm_device *dev = dev_priv->dev;
	struct intel_css_header *css_header;
	struct intel_package_header *package_header;
	struct intel_dmc_header *dmc_header;
	struct intel_csr *csr = &dev_priv->csr;
	const struct stepping_info *stepping_info = intel_get_stepping_info(dev);
	char stepping, substepping;
	uint32_t dmc_offset = CSR_DEFAULT_FW_OFFSET, readcount = 0, nbytes;
	uint32_t i;
	uint32_t *dmc_payload;

	if (!fw)
		return NULL;

	if (!stepping_info) {
		DRM_ERROR("Unknown stepping info, firmware loading failed\n");
		return NULL;
	}

	stepping = stepping_info->stepping;
	substepping = stepping_info->substepping;

	/* Extract CSS Header information*/
	css_header = (struct intel_css_header *)fw->data;
	if (sizeof(struct intel_css_header) !=
	    (css_header->header_len * 4)) {
		DRM_ERROR("Firmware has wrong CSS header length %u bytes\n",
			  (css_header->header_len * 4));
		return NULL;
	}

	csr->version = css_header->version;

	if ((IS_SKYLAKE(dev) || IS_KABYLAKE(dev)) &&
	    csr->version < SKL_CSR_VERSION_REQUIRED) {
		DRM_INFO("Refusing to load old Skylake DMC firmware v%u.%u,"
			 " please upgrade to v%u.%u or later"
			   " [" FIRMWARE_URL "].\n",
			 CSR_VERSION_MAJOR(csr->version),
			 CSR_VERSION_MINOR(csr->version),
			 CSR_VERSION_MAJOR(SKL_CSR_VERSION_REQUIRED),
			 CSR_VERSION_MINOR(SKL_CSR_VERSION_REQUIRED));
		return NULL;
	}

	readcount += sizeof(struct intel_css_header);

	/* Extract Package Header information*/
	package_header = (struct intel_package_header *)
		&fw->data[readcount];
	if (sizeof(struct intel_package_header) !=
	    (package_header->header_len * 4)) {
		DRM_ERROR("Firmware has wrong package header length %u bytes\n",
			  (package_header->header_len * 4));
		return NULL;
	}
	readcount += sizeof(struct intel_package_header);

	/* Search for dmc_offset to find firware binary. */
	for (i = 0; i < package_header->num_entries; i++) {
		if (package_header->fw_info[i].substepping == '*' &&
		    stepping == package_header->fw_info[i].stepping) {
			dmc_offset = package_header->fw_info[i].offset;
			break;
		} else if (stepping == package_header->fw_info[i].stepping &&
			substepping == package_header->fw_info[i].substepping) {
			dmc_offset = package_header->fw_info[i].offset;
			break;
		} else if (package_header->fw_info[i].stepping == '*' &&
			   package_header->fw_info[i].substepping == '*')
			dmc_offset = package_header->fw_info[i].offset;
	}
	if (dmc_offset == CSR_DEFAULT_FW_OFFSET) {
		DRM_ERROR("Firmware not supported for %c stepping\n", stepping);
		return NULL;
	}
	readcount += dmc_offset;

	/* Extract dmc_header information. */
	dmc_header = (struct intel_dmc_header *)&fw->data[readcount];
	if (sizeof(struct intel_dmc_header) != (dmc_header->header_len)) {
		DRM_ERROR("Firmware has wrong dmc header length %u bytes\n",
			  (dmc_header->header_len));
		return NULL;
	}
	readcount += sizeof(struct intel_dmc_header);

	/* Cache the dmc header info. */
	if (dmc_header->mmio_count > ARRAY_SIZE(csr->mmioaddr)) {
		DRM_ERROR("Firmware has wrong mmio count %u\n",
			  dmc_header->mmio_count);
		return NULL;
	}
	csr->mmio_count = dmc_header->mmio_count;
	for (i = 0; i < dmc_header->mmio_count; i++) {
		if (dmc_header->mmioaddr[i] < CSR_MMIO_START_RANGE ||
		    dmc_header->mmioaddr[i] > CSR_MMIO_END_RANGE) {
			DRM_ERROR(" Firmware has wrong mmio address 0x%x\n",
				  dmc_header->mmioaddr[i]);
			return NULL;
		}
		csr->mmioaddr[i] = _MMIO(dmc_header->mmioaddr[i]);
		csr->mmiodata[i] = dmc_header->mmiodata[i];
	}

	/* fw_size is in dwords, so multiplied by 4 to convert into bytes. */
	nbytes = dmc_header->fw_size * 4;
	if (nbytes > CSR_MAX_FW_SIZE) {
		DRM_ERROR("CSR firmware too big (%u) bytes\n", nbytes);
		return NULL;
	}
	csr->dmc_fw_size = dmc_header->fw_size;

	dmc_payload = kmalloc(nbytes, GFP_KERNEL);
	if (!dmc_payload) {
		DRM_ERROR("Memory allocation failed for dmc payload\n");
		return NULL;
	}

	memcpy(dmc_payload, &fw->data[readcount], nbytes);

	return dmc_payload;
}

static void csr_load_work_fn(struct work_struct *work)
{
	struct drm_i915_private *dev_priv;
	struct intel_csr *csr;
	const struct firmware *fw;
	int ret;

	dev_priv = container_of(work, typeof(*dev_priv), csr.work);
	csr = &dev_priv->csr;

	ret = request_firmware(&fw, dev_priv->csr.fw_path,
			       &dev_priv->dev->pdev->dev);
	if (!fw)
		goto out;

	dev_priv->csr.dmc_payload = parse_csr_fw(dev_priv, fw);
	if (!dev_priv->csr.dmc_payload)
		goto out;

	/* load csr program during system boot, as needed for DC states */
	intel_csr_load_program(dev_priv);

out:
	if (dev_priv->csr.dmc_payload) {
		intel_display_power_put(dev_priv, POWER_DOMAIN_INIT);

		DRM_INFO("Finished loading %s (v%u.%u)\n",
			 dev_priv->csr.fw_path,
			 CSR_VERSION_MAJOR(csr->version),
			 CSR_VERSION_MINOR(csr->version));
	} else {
		dev_notice(dev_priv->dev->dev,
			   "Failed to load DMC firmware"
			   " [" FIRMWARE_URL "],"
			   " disabling runtime power management.\n");
	}

	release_firmware(fw);
}

/**
 * intel_csr_ucode_init() - initialize the firmware loading.
 * @dev_priv: i915 drm device.
 *
 * This function is called at the time of loading the display driver to read
 * firmware from a .bin file and copied into a internal memory.
 */
void intel_csr_ucode_init(struct drm_i915_private *dev_priv)
{
	struct intel_csr *csr = &dev_priv->csr;

	INIT_WORK(&dev_priv->csr.work, csr_load_work_fn);

	if (!HAS_CSR(dev_priv))
		return;

	if (IS_SKYLAKE(dev_priv) || IS_KABYLAKE(dev_priv))
		csr->fw_path = I915_CSR_SKL;
	else if (IS_BROXTON(dev_priv))
		csr->fw_path = I915_CSR_BXT;
	else {
		DRM_ERROR("Unexpected: no known CSR firmware for platform\n");
		return;
	}

	DRM_DEBUG_KMS("Loading %s\n", csr->fw_path);

	/*
	 * Obtain a runtime pm reference, until CSR is loaded,
	 * to avoid entering runtime-suspend.
	 */
	intel_display_power_get(dev_priv, POWER_DOMAIN_INIT);

	schedule_work(&dev_priv->csr.work);
}

/**
 * intel_csr_ucode_fini() - unload the CSR firmware.
 * @dev_priv: i915 drm device.
 *
 * Firmmware unloading includes freeing the internal momory and reset the
 * firmware loading status.
 */
void intel_csr_ucode_fini(struct drm_i915_private *dev_priv)
{
	if (!HAS_CSR(dev_priv))
		return;

	flush_work(&dev_priv->csr.work);

	kfree(dev_priv->csr.dmc_payload);
}
