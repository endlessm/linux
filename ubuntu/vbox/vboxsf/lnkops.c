/** @file
 *
 * vboxsf -- VirtualBox Guest Additions for Linux:
 * Operations for symbolic links.
 */

/*
 * Copyright (C) 2010-2011 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "vfsmod.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)

static void *sf_follow_link(struct dentry *dentry, struct nameidata *nd)
{
    struct inode *inode = dentry->d_inode;
    struct sf_glob_info *sf_g = GET_GLOB_INFO(inode->i_sb);
    struct sf_inode_info *sf_i = GET_INODE_INFO(inode);
    int error = -ENOMEM;
    char *path = (char*)get_zeroed_page(GFP_KERNEL);
    int rc;

    if (path)
    {
        error = 0;
        rc = vboxReadLink(&client_handle, &sf_g->map, sf_i->path, PATH_MAX, path);
        if (RT_FAILURE(rc))
        {
            LogFunc(("vboxReadLink failed, caller=%s, rc=%Rrc\n", __func__, rc));
            free_page((unsigned long)path);
            error = -EPROTO;
        }
    }
    nd_set_link(nd, error ? ERR_PTR(error) : path);
    return NULL;
}

static void sf_put_link(struct dentry *dentry, struct nameidata *nd, void *cookie)
{
    char *page = nd_get_link(nd);
    if (!IS_ERR(page))
        free_page((unsigned long)page);
}

struct inode_operations sf_lnk_iops =
{
    .readlink       = generic_readlink,
    .follow_link    = sf_follow_link,
    .put_link       = sf_put_link
};

#endif
