// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/err.h>
#include <linux/efi.h>
#include <linux/slab.h>
#include <keys/asymmetric-type.h>
#include <keys/system_keyring.h>
#include "../integrity.h"
#include "keyring_handler.h"

static efi_guid_t efi_cert_x509_guid __initdata = EFI_CERT_X509_GUID;
static efi_guid_t efi_cert_x509_sha256_guid __initdata =
	EFI_CERT_X509_SHA256_GUID;
static efi_guid_t efi_cert_sha256_guid __initdata = EFI_CERT_SHA256_GUID;

/*
 * Look to see if a UEFI variable called MokIgnoreDB exists and return true if
 * it does.
 *
 * This UEFI variable is set by the shim if a user tells the shim to not use
 * the certs/hashes in the UEFI db variable for verification purposes.  If it
 * is set, we should ignore the db variable also and the true return indicates
 * this.
 */
static __init bool uefi_check_ignore_db(void)
{
	efi_status_t status;
	unsigned int db = 0;
	unsigned long size = sizeof(db);
	efi_guid_t guid = EFI_SHIM_LOCK_GUID;

	status = efi.get_variable(L"MokIgnoreDB", &guid, NULL, &size, &db);
	return status == EFI_SUCCESS;
}

/*
 * Get a certificate list blob from the named EFI variable.
 */
static __init int get_cert_list(efi_char16_t *name, efi_guid_t *guid,
				  unsigned long *size , void **cert_list,
				  efi_status_t *status)
{
	unsigned long lsize = 4;
	unsigned long tmpdb[4];
	void *db;

	*status = efi.get_variable(name, guid, NULL, &lsize, &tmpdb);
	if (*status == EFI_NOT_FOUND) {
		*size = 0;
		*cert_list = NULL;
		return 0;
	}

	if (*status != EFI_BUFFER_TOO_SMALL) {
		pr_err("Couldn't get size: %s (0x%lx)\n",
		       efi_status_to_str(*status), *status);
		return efi_status_to_err(*status);
	}

	db = kmalloc(lsize, GFP_KERNEL);
	if (!db)
		return -ENOMEM;

	*status = efi.get_variable(name, guid, NULL, &lsize, db);
	if (*status != EFI_SUCCESS) {
		kfree(db);
		pr_err("Error reading db var: %s (0x%lx)\n",
		       efi_status_to_str(*status), *status);
		return efi_status_to_err(*status);
	}

	*size = lsize;
	*cert_list = db;
	return 0;
}

/*
 * Load the certs contained in the UEFI databases into the platform trusted
 * keyring and the UEFI blacklisted X.509 cert SHA256 hashes into the blacklist
 * keyring.
 */
static int __init load_uefi_certs(void)
{
	efi_guid_t secure_var = EFI_IMAGE_SECURITY_DATABASE_GUID;
	efi_guid_t mok_var = EFI_SHIM_LOCK_GUID;
	void *db = NULL, *dbx = NULL, *mok = NULL;
	unsigned long dbsize = 0, dbxsize = 0, moksize = 0;
	efi_status_t status;
	int rc = 0;

	if (!efi.get_variable)
		return false;

	/* Get db, MokListRT, and dbx.  They might not exist, so it isn't
	 * an error if we can't get them.
	 */
	if (!uefi_check_ignore_db()) {
		rc = get_cert_list(L"db", &secure_var, &dbsize, &db, &status);
		if (rc < 0) {
			if (status == EFI_NOT_FOUND)
				pr_debug("MODSIGN: db variable wasn't found\n");
			else
				pr_err("MODSIGN: Couldn't get UEFI db list\n");
		} else if (dbsize != 0) {
			rc = parse_efi_signature_list("UEFI:db",
					db, dbsize, get_handler_for_db);
			if (rc)
				pr_err("Couldn't parse db signatures: %d\n",
				       rc);
			kfree(db);
		}
	}

	rc = get_cert_list(L"MokListRT", &mok_var, &moksize, &mok, &status);
	if (rc < 0) {
		if (status == EFI_NOT_FOUND)
			pr_debug("MokListRT variable wasn't found\n");
		else
			pr_info("Couldn't get UEFI MokListRT\n");
	} else if (moksize != 0) {
		rc = parse_efi_signature_list("UEFI:MokListRT",
					      mok, moksize, get_handler_for_db);
		if (rc)
			pr_err("Couldn't parse MokListRT signatures: %d\n", rc);
		kfree(mok);
	}

	rc = get_cert_list(L"dbx", &secure_var, &dbxsize, &dbx, &status);
	if (rc < 0) {
		if (status == EFI_NOT_FOUND)
			pr_debug("dbx variable wasn't found\n");
		else
			pr_info("Couldn't get UEFI dbx list\n");
	} else if (dbxsize != 0) {
		rc = parse_efi_signature_list("UEFI:dbx",
					      dbx, dbxsize,
					      get_handler_for_dbx);
		if (rc)
			pr_err("Couldn't parse dbx signatures: %d\n", rc);
		kfree(dbx);
	}

	return rc;
}
late_initcall(load_uefi_certs);
