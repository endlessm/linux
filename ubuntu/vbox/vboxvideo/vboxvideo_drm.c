/** @file $Id: vboxvideo_drm.c $
 *
 * VirtualBox Additions Linux kernel driver, DRM support
 */

/*
 * Copyright (C) 2006-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 * tdfx_drv.c -- tdfx driver -*- linux-c -*-
 * Created: Thu Oct  7 10:38:32 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Rickard E. (Rik) Faith <faith@valinux.com>
 *    Daryll Strauss <daryll@valinux.com>
 *    Gareth Hughes <gareth@valinux.com>
 */

#include "version-generated.h"

#include <linux/module.h>
#include <linux/version.h>
#include <drm/drmP.h>
#include "vboxvideo_drm.h"

/* This definition and the file-operations-as-pointer change were both added in
 * kernel 3.3.  All back-ports of the structure change to distribution kernels
 * that I have checked also back-ported the definition at the same time. */
#ifdef DRM_IOCTL_MODE_ADDFB2
# define DRM_FOPS_AS_POINTER
#endif

/* The first of these was introduced when drm was generalised to work with
 * non-PCI buses, but was removed between 3.15 and 3.16.  The second is a
 * random definition introduced in the mean-time. */
#if defined(DRIVER_BUS_PCI) || defined(DRIVER_PRIME)
# define DRM_NEW_BUS_INIT 1
#endif

static struct pci_device_id pciidlist[] = {
        vboxvideo_PCI_IDS
};

MODULE_DEVICE_TABLE(pci, pciidlist);

int vboxvideo_driver_load(struct drm_device * dev, unsigned long flags)
{
    return 0;
}

#ifdef DRM_FOPS_AS_POINTER
/* since linux-3.3.0-rc1 drm_driver::fops is pointer */
static struct file_operations driver_fops =
{
        .owner = THIS_MODULE,
        .open = drm_open,
        .release = drm_release,
        .unlocked_ioctl = drm_ioctl,
# if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)
        /* This shouldn't be necessary even for old kernels as there is
         * nothing sensible to mmap. But we play safe and keep it for
         * legacy reasons. */
        .mmap = drm_mmap,
# endif
        .poll = drm_poll,
};
#endif

static struct drm_driver driver =
{
    /* .driver_features = DRIVER_USE_MTRR, */
    .load = vboxvideo_driver_load,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0)
    /* If this is missing a warning gets printed to dmesg.  We will not
     * attempt to make kernels work to which the change (915b4d11b) got back-
     * ported, as the problem is only cosmetic. */
    .set_busid = drm_pci_set_busid,
#endif
# ifndef DRM_FOPS_AS_POINTER
    .fops =
    {
        .owner = THIS_MODULE,
        .open = drm_open,
        .release = drm_release,
        /* This was changed with Linux 2.6.33 but Fedora backported this
         * change to their 2.6.32 kernel. */
#if defined(DRM_UNLOCKED)
        .unlocked_ioctl = drm_ioctl,
#else
        .ioctl = drm_ioctl,
#endif
        .mmap = drm_mmap,
        .poll = drm_poll,
    },
#else /* defined(DRM_FOPS_AS_POINTER) */
    .fops = &driver_fops,
#endif
#ifndef DRM_NEW_BUS_INIT
    .pci_driver =
    {
        .name = DRIVER_NAME,
        .id_table = pciidlist,
    },
#endif
    .name = DRIVER_NAME,
    .desc = DRIVER_DESC,
    .date = DRIVER_DATE,
    .major = DRIVER_MAJOR,
    .minor = DRIVER_MINOR,
    .patchlevel = DRIVER_PATCHLEVEL,
};

#ifdef DRM_NEW_BUS_INIT
static struct pci_driver pci_driver =
{
    .name = DRIVER_NAME,
    .id_table = pciidlist,
};
#endif

static int __init vboxvideo_init(void)
{
#ifndef DRM_NEW_BUS_INIT
    return drm_init(&driver);
#else
    return drm_pci_init(&driver, &pci_driver);
#endif
}

static void __exit vboxvideo_exit(void)
{
#ifndef DRM_NEW_BUS_INIT
    drm_exit(&driver);
#else
    drm_pci_exit(&driver, &pci_driver);
#endif
}

module_init(vboxvideo_init);
module_exit(vboxvideo_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
#ifdef MODULE_VERSION
MODULE_VERSION(VBOX_VERSION_STRING);
#endif
MODULE_LICENSE("GPL and additional rights");
