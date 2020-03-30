/* $Rev: 129380 $ */
/** @file
 * VBoxGuest - Linux specifics.
 *
 * Note. Unfortunately, the difference between this and SUPDrv-linux.c is
 *       a little bit too big to be helpful.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SUP_DRV

#include "the-linux-kernel.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
# define VBOXGUEST_WITH_INPUT_DRIVER
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
# define CONST_4_15 const
#else
# define CONST_4_15
#endif

#include "VBoxGuestInternal.h"
#ifdef VBOXGUEST_WITH_INPUT_DRIVER
# include <linux/input.h>
#endif
#include <linux/miscdevice.h>
#include <linux/poll.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
# include <linux/tty.h>
#endif
#include <VBox/version.h>
#include "revision-generated.h"

#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/ctype.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/mp.h>
#include <iprt/process.h>
#include <iprt/spinlock.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <VBox/err.h>
#include <VBox/log.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The device name. */
#define DEVICE_NAME             "vboxguest"
/** The device name for the device node open to everyone. */
#define DEVICE_NAME_USER        "vboxuser"
/** The name of the PCI driver */
#define DRIVER_NAME             DEVICE_NAME


/* 2.4.x compatibility macros that may or may not be defined. */
#ifndef IRQ_RETVAL
# define irqreturn_t            void
# define IRQ_RETVAL(n)
#endif

/* uidgid.h was introduced in 3.5.0. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
# define kgid_t gid_t
# define kuid_t uid_t
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void vgdrvLinuxTermPci(struct pci_dev *pPciDev);
static int  vgdrvLinuxProbePci(struct pci_dev *pPciDev, const struct pci_device_id *id);
static int  __init vgdrvLinuxModInit(void);
static void __exit vgdrvLinuxModExit(void);
static int  vgdrvLinuxOpen(struct inode *pInode, struct file *pFilp);
static int  vgdrvLinuxRelease(struct inode *pInode, struct file *pFilp);
#ifdef HAVE_UNLOCKED_IOCTL
static long vgdrvLinuxIOCtl(struct file *pFilp, unsigned int uCmd, unsigned long ulArg);
#else
static int  vgdrvLinuxIOCtl(struct inode *pInode, struct file *pFilp, unsigned int uCmd, unsigned long ulArg);
#endif
static int  vgdrvLinuxIOCtlSlow(struct file *pFilp, unsigned int uCmd, unsigned long ulArg, PVBOXGUESTSESSION pSession);
static int  vgdrvLinuxFAsync(int fd, struct file *pFile, int fOn);
static unsigned int vgdrvLinuxPoll(struct file *pFile, poll_table *pPt);
static ssize_t vgdrvLinuxRead(struct file *pFile, char *pbBuf, size_t cbRead, loff_t *poff);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Device extention & session data association structure.
 */
static VBOXGUESTDEVEXT          g_DevExt;
/** The PCI device. */
static struct pci_dev          *g_pPciDev = NULL;
/** The base of the I/O port range. */
static RTIOPORT                 g_IOPortBase;
/** The base of the MMIO range. */
static RTHCPHYS                 g_MMIOPhysAddr = NIL_RTHCPHYS;
/** The size of the MMIO range as seen by PCI. */
static uint32_t                 g_cbMMIO;
/** The pointer to the mapping of the MMIO range. */
static void                    *g_pvMMIOBase;
/** Wait queue used by polling. */
static wait_queue_head_t        g_PollEventQueue;
/** Asynchronous notification stuff.  */
static struct fasync_struct    *g_pFAsyncQueue;
#ifdef VBOXGUEST_WITH_INPUT_DRIVER
/** Pre-allocated mouse status VMMDev request for use in the IRQ
 * handler. */
static VMMDevReqMouseStatus    *g_pMouseStatusReq;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
/** Whether we've create the logger or not. */
static volatile bool            g_fLoggerCreated;
/** Release logger group settings. */
static char                     g_szLogGrp[128];
/** Release logger flags settings. */
static char                     g_szLogFlags[128];
/** Release logger destination settings. */
static char                     g_szLogDst[128];
# if 0
/** Debug logger group settings. */
static char                     g_szDbgLogGrp[128];
/** Debug logger flags settings. */
static char                     g_szDbgLogFlags[128];
/** Debug logger destination settings. */
static char                     g_szDbgLogDst[128];
# endif
#endif

/** The input device handle */
#ifdef VBOXGUEST_WITH_INPUT_DRIVER
static struct input_dev        *g_pInputDevice = NULL;
#endif

/** The file_operations structure. */
static struct file_operations   g_FileOps =
{
    owner:          THIS_MODULE,
    open:           vgdrvLinuxOpen,
    release:        vgdrvLinuxRelease,
#ifdef HAVE_UNLOCKED_IOCTL
    unlocked_ioctl: vgdrvLinuxIOCtl,
#else
    ioctl:          vgdrvLinuxIOCtl,
#endif
    fasync:         vgdrvLinuxFAsync,
    read:           vgdrvLinuxRead,
    poll:           vgdrvLinuxPoll,
    llseek:         no_llseek,
};

/** The miscdevice structure. */
static struct miscdevice        g_MiscDevice =
{
    minor:          MISC_DYNAMIC_MINOR,
    name:           DEVICE_NAME,
    fops:           &g_FileOps,
};

/** The file_operations structure for the user device.
 * @remarks For the time being we'll be using the same implementation as
 *          /dev/vboxguest here. */
static struct file_operations   g_FileOpsUser =
{
    owner:          THIS_MODULE,
    open:           vgdrvLinuxOpen,
    release:        vgdrvLinuxRelease,
#ifdef HAVE_UNLOCKED_IOCTL
    unlocked_ioctl: vgdrvLinuxIOCtl,
#else
    ioctl:          vgdrvLinuxIOCtl,
#endif
};

/** The miscdevice structure for the user device. */
static struct miscdevice        g_MiscDeviceUser =
{
    minor:          MISC_DYNAMIC_MINOR,
    name:           DEVICE_NAME_USER,
    fops:           &g_FileOpsUser,
};


/** PCI hotplug structure. */
static const struct pci_device_id g_VBoxGuestPciId[] =
{
    {
        vendor:     VMMDEV_VENDORID,
        device:     VMMDEV_DEVICEID
    },
    {
        /* empty entry */
    }
};

MODULE_DEVICE_TABLE(pci, g_VBoxGuestPciId);

/** Structure for registering the PCI driver. */
static struct pci_driver  g_PciDriver =
{
    name:           DRIVER_NAME,
    id_table:       g_VBoxGuestPciId,
    probe:          vgdrvLinuxProbePci,
    remove:         vgdrvLinuxTermPci
};

#ifdef VBOXGUEST_WITH_INPUT_DRIVER
/** Kernel IDC session to ourselves for use with the mouse events. */
static PVBOXGUESTSESSION        g_pKernelSession = NULL;
#endif



/**
 * Converts a VBox status code to a linux error code.
 *
 * @returns corresponding negative linux error code.
 * @param   rc  supdrv error code (SUPDRV_ERR_* defines).
 */
static int vgdrvLinuxConvertToNegErrno(int rc)
{
    if (   rc > -1000
        && rc < 1000)
        return -RTErrConvertToErrno(rc);
    switch (rc)
    {
        case VERR_HGCM_SERVICE_NOT_FOUND:      return -ESRCH;
        case VINF_HGCM_CLIENT_REJECTED:        return 0;
        case VERR_HGCM_INVALID_CMD_ADDRESS:    return -EFAULT;
        case VINF_HGCM_ASYNC_EXECUTE:          return 0;
        case VERR_HGCM_INTERNAL:               return -EPROTO;
        case VERR_HGCM_INVALID_CLIENT_ID:      return -EINVAL;
        case VINF_HGCM_SAVE_STATE:             return 0;
        /* No reason to return this to a guest */
        // case VERR_HGCM_SERVICE_EXISTS:         return -EEXIST;
        default:
            AssertMsgFailed(("Unhandled error code %Rrc\n", rc));
            return -EPROTO;
    }
}


/**
 * Does the PCI detection and init of the device.
 *
 * @returns 0 on success, negated errno on failure.
 */
static int vgdrvLinuxProbePci(struct pci_dev *pPciDev, const struct pci_device_id *id)
{
    int rc;

    NOREF(id);
    AssertReturn(!g_pPciDev, -EINVAL);
    rc = pci_enable_device(pPciDev);
    if (rc >= 0)
    {
        /* I/O Ports are mandatory, the MMIO bit is not. */
        g_IOPortBase = pci_resource_start(pPciDev, 0);
        if (g_IOPortBase != 0)
        {
            /*
             * Map the register address space.
             */
            g_MMIOPhysAddr = pci_resource_start(pPciDev, 1);
            g_cbMMIO       = pci_resource_len(pPciDev, 1);
            if (request_mem_region(g_MMIOPhysAddr, g_cbMMIO, DEVICE_NAME) != NULL)
            {
                g_pvMMIOBase = ioremap(g_MMIOPhysAddr, g_cbMMIO);
                if (g_pvMMIOBase)
                {
                    /** @todo why aren't we requesting ownership of the I/O ports as well? */
                    g_pPciDev = pPciDev;
                    return 0;
                }

                /* failure cleanup path */
                LogRel((DEVICE_NAME ": ioremap failed; MMIO Addr=%RHp cb=%#x\n", g_MMIOPhysAddr, g_cbMMIO));
                rc = -ENOMEM;
                release_mem_region(g_MMIOPhysAddr, g_cbMMIO);
            }
            else
            {
                LogRel((DEVICE_NAME ": failed to obtain adapter memory\n"));
                rc = -EBUSY;
            }
            g_MMIOPhysAddr = NIL_RTHCPHYS;
            g_cbMMIO       = 0;
            g_IOPortBase   = 0;
        }
        else
        {
            LogRel((DEVICE_NAME ": did not find expected hardware resources\n"));
            rc = -ENXIO;
        }
        pci_disable_device(pPciDev);
    }
    else
        LogRel((DEVICE_NAME ": could not enable device: %d\n", rc));
    return rc;
}


/**
 * Clean up the usage of the PCI device.
 */
static void vgdrvLinuxTermPci(struct pci_dev *pPciDev)
{
    g_pPciDev = NULL;
    if (pPciDev)
    {
        iounmap(g_pvMMIOBase);
        g_pvMMIOBase = NULL;

        release_mem_region(g_MMIOPhysAddr, g_cbMMIO);
        g_MMIOPhysAddr = NIL_RTHCPHYS;
        g_cbMMIO = 0;

        pci_disable_device(pPciDev);
    }
}


/**
 * Interrupt service routine.
 *
 * @returns In 2.4 it returns void.
 *          In 2.6 we indicate whether we've handled the IRQ or not.
 *
 * @param   iIrq            The IRQ number.
 * @param   pvDevId         The device ID, a pointer to g_DevExt.
 * @param   pRegs           Register set. Removed in 2.6.19.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19) && !defined(DOXYGEN_RUNNING)
static irqreturn_t vgdrvLinuxISR(int iIrq, void *pvDevId)
#else
static irqreturn_t vgdrvLinuxISR(int iIrq, void *pvDevId, struct pt_regs *pRegs)
#endif
{
    bool fTaken = VGDrvCommonISR(&g_DevExt);
    return IRQ_RETVAL(fTaken);
}


/**
 * Registers the ISR and initializes the poll wait queue.
 */
static int __init vgdrvLinuxInitISR(void)
{
    int rc;

    init_waitqueue_head(&g_PollEventQueue);
    rc = request_irq(g_pPciDev->irq,
                     vgdrvLinuxISR,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
                     IRQF_SHARED,
#else
                     SA_SHIRQ,
#endif
                     DEVICE_NAME,
                     &g_DevExt);
    if (rc)
    {
        LogRel((DEVICE_NAME ": could not request IRQ %d: err=%d\n", g_pPciDev->irq, rc));
        return rc;
    }
    return 0;
}


/**
 * Deregisters the ISR.
 */
static void vgdrvLinuxTermISR(void)
{
    free_irq(g_pPciDev->irq, &g_DevExt);
}


#ifdef VBOXGUEST_WITH_INPUT_DRIVER

/**
 * Reports the mouse integration status to the host.
 *
 * Calls the kernel IOCtl to report mouse status to the host on behalf of
 * our kernel session.
 *
 * @param   fStatus     The mouse status to report.
 */
static int vgdrvLinuxSetMouseStatus(uint32_t fStatus)
{
    int rc;
    VBGLIOCSETMOUSESTATUS Req;
    VBGLREQHDR_INIT(&Req.Hdr, SET_MOUSE_STATUS);
    Req.u.In.fStatus = fStatus;
    rc = VGDrvCommonIoCtl(VBGL_IOCTL_SET_MOUSE_STATUS, &g_DevExt, g_pKernelSession, &Req.Hdr, sizeof(Req));
    if (RT_SUCCESS(rc))
        rc = Req.Hdr.rc;
    return rc;
}


/**
 * Called when the input device is first opened.
 *
 * Sets up absolute mouse reporting.
 */
static int vboxguestOpenInputDevice(struct input_dev *pDev)
{
    int rc = vgdrvLinuxSetMouseStatus(VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE | VMMDEV_MOUSE_NEW_PROTOCOL);
    if (RT_FAILURE(rc))
        return ENODEV;
    NOREF(pDev);
    return 0;
}


/**
 * Called if all open handles to the input device are closed.
 *
 * Disables absolute reporting.
 */
static void vboxguestCloseInputDevice(struct input_dev *pDev)
{
    NOREF(pDev);
    vgdrvLinuxSetMouseStatus(0);
}


/**
 * Creates the kernel input device.
 */
static int __init vgdrvLinuxCreateInputDevice(void)
{
    int rc = VbglR0GRAlloc((VMMDevRequestHeader **)&g_pMouseStatusReq, sizeof(*g_pMouseStatusReq), VMMDevReq_GetMouseStatus);
    if (RT_SUCCESS(rc))
    {
        g_pInputDevice = input_allocate_device();
        if (g_pInputDevice)
        {
            g_pInputDevice->id.bustype = BUS_PCI;
            g_pInputDevice->id.vendor  = VMMDEV_VENDORID;
            g_pInputDevice->id.product = VMMDEV_DEVICEID;
            g_pInputDevice->id.version = VBOX_SHORT_VERSION;
            g_pInputDevice->open       = vboxguestOpenInputDevice;
            g_pInputDevice->close      = vboxguestCloseInputDevice;
# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
            g_pInputDevice->cdev.dev   = &g_pPciDev->dev;
# else
            g_pInputDevice->dev.parent = &g_pPciDev->dev;
# endif
            rc = input_register_device(g_pInputDevice);
            if (rc == 0)
            {
                /* Do what one of our competitors apparently does as that works. */
                ASMBitSet(g_pInputDevice->evbit, EV_ABS);
                ASMBitSet(g_pInputDevice->evbit, EV_KEY);
# ifdef EV_SYN
                ASMBitSet(g_pInputDevice->evbit, EV_SYN);
# endif
                input_set_abs_params(g_pInputDevice, ABS_X, VMMDEV_MOUSE_RANGE_MIN, VMMDEV_MOUSE_RANGE_MAX, 0, 0);
                input_set_abs_params(g_pInputDevice, ABS_Y, VMMDEV_MOUSE_RANGE_MIN, VMMDEV_MOUSE_RANGE_MAX, 0, 0);
                ASMBitSet(g_pInputDevice->keybit, BTN_MOUSE);
                /** @todo this string should be in a header file somewhere. */
                g_pInputDevice->name = "VirtualBox mouse integration";
                return 0;
            }

            input_free_device(g_pInputDevice);
        }
        else
            rc = -ENOMEM;
        VbglR0GRFree(&g_pMouseStatusReq->header);
        g_pMouseStatusReq = NULL;
    }
    else
        rc = -ENOMEM;
    return rc;
}


/**
 * Terminates the kernel input device.
 */
static void vgdrvLinuxTermInputDevice(void)
{
    VbglR0GRFree(&g_pMouseStatusReq->header);
    g_pMouseStatusReq = NULL;

    /* See documentation of input_register_device(): input_free_device()
     * should not be called after a device has been registered. */
    input_unregister_device(g_pInputDevice);
}

#endif /* VBOXGUEST_WITH_INPUT_DRIVER */

/**
 * Creates the device nodes.
 *
 * @returns 0 on success, negated errno on failure.
 */
static int __init vgdrvLinuxInitDeviceNodes(void)
{
    /*
     * The full feature device node.
     */
    int rc = misc_register(&g_MiscDevice);
    if (!rc)
    {
        /*
         * The device node intended to be accessible by all users.
         */
        rc = misc_register(&g_MiscDeviceUser);
        if (!rc)
            return 0;
        LogRel((DEVICE_NAME ": misc_register failed for %s (rc=%d)\n", DEVICE_NAME_USER, rc));
        misc_deregister(&g_MiscDevice);
    }
    else
        LogRel((DEVICE_NAME ": misc_register failed for %s (rc=%d)\n", DEVICE_NAME, rc));
    return rc;
}


/**
 * Deregisters the device nodes.
 */
static void vgdrvLinuxTermDeviceNodes(void)
{
    misc_deregister(&g_MiscDevice);
    misc_deregister(&g_MiscDeviceUser);
}


/**
 * Initialize module.
 *
 * @returns appropriate status code.
 */
static int __init vgdrvLinuxModInit(void)
{
    static const char * const   s_apszGroups[] = VBOX_LOGGROUP_NAMES;
    PRTLOGGER                   pRelLogger;
    int                         rc;

    /*
     * Initialize IPRT first.
     */
    rc = RTR0Init(0);
    if (RT_FAILURE(rc))
    {
        printk(KERN_ERR DEVICE_NAME ": RTR0Init failed, rc=%d.\n", rc);
        return -EINVAL;
    }

    /*
     * Create the release log.
     * (We do that here instead of common code because we want to log
     * early failures using the LogRel macro.)
     */
    rc = RTLogCreate(&pRelLogger, 0 /* fFlags */, "all",
                     "VBOX_RELEASE_LOG", RT_ELEMENTS(s_apszGroups), s_apszGroups,
                     RTLOGDEST_STDOUT | RTLOGDEST_DEBUGGER | RTLOGDEST_USER, NULL);
    if (RT_SUCCESS(rc))
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
        RTLogGroupSettings(pRelLogger, g_szLogGrp);
        RTLogFlags(pRelLogger, g_szLogFlags);
        RTLogDestinations(pRelLogger, g_szLogDst);
#endif
        RTLogRelSetDefaultInstance(pRelLogger);
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
    g_fLoggerCreated = true;
#endif

    /*
     * Locate and initialize the PCI device.
     */
    rc = pci_register_driver(&g_PciDriver);
    if (rc >= 0 && g_pPciDev)
    {
        /*
         * Call the common device extension initializer.
         */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) && defined(RT_ARCH_X86)
        VBOXOSTYPE enmOSType = VBOXOSTYPE_Linux26;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) && defined(RT_ARCH_AMD64)
        VBOXOSTYPE enmOSType = VBOXOSTYPE_Linux26_x64;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0) && defined(RT_ARCH_X86)
        VBOXOSTYPE enmOSType = VBOXOSTYPE_Linux24;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0) && defined(RT_ARCH_AMD64)
        VBOXOSTYPE enmOSType = VBOXOSTYPE_Linux24_x64;
#else
# warning "huh? which arch + version is this?"
        VBOXOSTYPE enmOsType = VBOXOSTYPE_Linux;
#endif
        rc = VGDrvCommonInitDevExt(&g_DevExt,
                                   g_IOPortBase,
                                   g_pvMMIOBase,
                                   g_cbMMIO,
                                   enmOSType,
                                   VMMDEV_EVENT_MOUSE_POSITION_CHANGED);
        if (RT_SUCCESS(rc))
        {
            /*
             * Register the interrupt service routine for it now that g_DevExt can handle IRQs.
             */
            rc = vgdrvLinuxInitISR();
            if (rc >= 0) /** @todo r=bird: status check differs from that inside vgdrvLinuxInitISR. */
            {
#ifdef VBOXGUEST_WITH_INPUT_DRIVER
                /*
                 * Create the kernel session for this driver.
                 */
                rc = VGDrvCommonCreateKernelSession(&g_DevExt, &g_pKernelSession);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Create the kernel input device.
                     */
                    rc = vgdrvLinuxCreateInputDevice();
                    if (rc >= 0)
                    {
#endif
                        /*
                         * Read host configuration.
                         */
                        VGDrvCommonProcessOptionsFromHost(&g_DevExt);

                        /*
                         * Finally, create the device nodes.
                         */
                        rc = vgdrvLinuxInitDeviceNodes();
                        if (rc >= 0)
                        {
                            /* some useful information for the user but don't show this on the console */
                            LogRel((DEVICE_NAME ": misc device minor %d, IRQ %d, I/O port %RTiop, MMIO at %RHp (size 0x%x)\n",
                                    g_MiscDevice.minor, g_pPciDev->irq, g_IOPortBase, g_MMIOPhysAddr, g_cbMMIO));
                            printk(KERN_DEBUG DEVICE_NAME ": Successfully loaded version "
                                   VBOX_VERSION_STRING " (interface " RT_XSTR(VMMDEV_VERSION) ")\n");
                            return rc;
                        }

                        /* bail out */
#ifdef VBOXGUEST_WITH_INPUT_DRIVER
                        vgdrvLinuxTermInputDevice();
                    }
                    else
                    {
                        LogRel((DEVICE_NAME ": vboxguestCreateInputDevice failed with rc=%Rrc\n", rc));
                        rc = RTErrConvertFromErrno(rc);
                    }
                    VGDrvCommonCloseSession(&g_DevExt, g_pKernelSession);
                }
#endif
                vgdrvLinuxTermISR();
            }
            VGDrvCommonDeleteDevExt(&g_DevExt);
        }
        else
        {
            LogRel((DEVICE_NAME ": VGDrvCommonInitDevExt failed with rc=%Rrc\n", rc));
            rc = RTErrConvertFromErrno(rc);
        }
    }
    else
    {
        LogRel((DEVICE_NAME ": PCI device not found, probably running on physical hardware.\n"));
        rc = -ENODEV;
    }
    pci_unregister_driver(&g_PciDriver);
    RTLogDestroy(RTLogRelSetDefaultInstance(NULL));
    RTLogDestroy(RTLogSetDefaultInstance(NULL));
    RTR0Term();
    return rc;
}


/**
 * Unload the module.
 */
static void __exit vgdrvLinuxModExit(void)
{
    /*
     * Inverse order of init.
     */
    vgdrvLinuxTermDeviceNodes();
#ifdef VBOXGUEST_WITH_INPUT_DRIVER
    vgdrvLinuxTermInputDevice();
    VGDrvCommonCloseSession(&g_DevExt, g_pKernelSession);
#endif
    vgdrvLinuxTermISR();
    VGDrvCommonDeleteDevExt(&g_DevExt);
    pci_unregister_driver(&g_PciDriver);
    RTLogDestroy(RTLogRelSetDefaultInstance(NULL));
    RTLogDestroy(RTLogSetDefaultInstance(NULL));
    RTR0Term();
}


/**
 * Get the process user ID.
 *
 * @returns UID.
 */
DECLINLINE(RTUID) vgdrvLinuxGetUid(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
    return from_kuid(current_user_ns(), current->cred->uid);
# else
    return current->cred->uid;
# endif
#else
    return current->uid;
#endif
}


/**
 * Checks if the given group number is zero or not.
 *
 * @returns true / false.
 * @param   gid                 The group to check for.
 */
DECLINLINE(bool) vgdrvLinuxIsGroupZero(kgid_t gid)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
    return from_kgid(current_user_ns(), gid);
#else
    return gid == 0;
#endif
}


/**
 * Searches the effective group and supplementary groups for @a gid.
 *
 * @returns true if member, false if not.
 * @param   gid                 The group to check for.
 */
DECLINLINE(RTGID) vgdrvLinuxIsInGroupEff(kgid_t gid)
{
    return in_egroup_p(gid) != 0;
}


/**
 * Check if we can positively or negatively determine that the process is
 * running under a login on the physical machine console.
 *
 * Havne't found a good way to figure this out for graphical sessions, so this
 * is mostly pointless.  But let us try do what we can do.
 *
 * @returns VMMDEV_REQUESTOR_CON_XXX.
 */
static uint32_t vgdrvLinuxRequestorOnConsole(void)
{
    uint32_t           fRet = VMMDEV_REQUESTOR_CON_DONT_KNOW;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28) /* First with tty_kref_put(). */
    /*
     * Check for tty0..63, ASSUMING that these are only used for the physical console.
     */
    struct tty_struct *pTty = get_current_tty();
    if (pTty)
    {
# if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
        const char *pszName = tty_name(pTty);
# else
        char szBuf[64];
        const char *pszName = tty_name(pTty, szBuf);
# endif
        if (   pszName
            && pszName[0] == 't'
            && pszName[1] == 't'
            && pszName[2] == 'y'
            && RT_C_IS_DIGIT(pszName[3])
            && (   pszName[4] == '\0'
                || (   RT_C_IS_DIGIT(pszName[4])
                    && pszName[5] == '\0'
                    && (pszName[3] - '0') * 10 + (pszName[4] - '0') <= 63)) )
               fRet = VMMDEV_REQUESTOR_CON_YES;
        tty_kref_put(pTty);
    }
#endif

    return fRet;
}


/**
 * Device open. Called on open /dev/vboxdrv
 *
 * @param   pInode      Pointer to inode info structure.
 * @param   pFilp       Associated file pointer.
 */
static int vgdrvLinuxOpen(struct inode *pInode, struct file *pFilp)
{
    int                 rc;
    PVBOXGUESTSESSION   pSession;
    uint32_t            fRequestor;
    Log((DEVICE_NAME ": pFilp=%p pid=%d/%d %s\n", pFilp, RTProcSelf(), current->pid, current->comm));

    /*
     * Figure out the requestor flags.
     * ASSUMES that the gid of /dev/vboxuser is what we should consider the special vbox group.
     */
    fRequestor = VMMDEV_REQUESTOR_USERMODE | VMMDEV_REQUESTOR_TRUST_NOT_GIVEN;
    if (vgdrvLinuxGetUid() == 0)
        fRequestor |= VMMDEV_REQUESTOR_USR_ROOT;
    else
        fRequestor |= VMMDEV_REQUESTOR_USR_USER;
    if (MINOR(pInode->i_rdev) == g_MiscDeviceUser.minor)
    {
        fRequestor |= VMMDEV_REQUESTOR_USER_DEVICE;
        if (!vgdrvLinuxIsGroupZero(pInode->i_gid) && vgdrvLinuxIsInGroupEff(pInode->i_gid))
            fRequestor |= VMMDEV_REQUESTOR_GRP_VBOX;
    }
    fRequestor |= vgdrvLinuxRequestorOnConsole();

    /*
     * Call common code to create the user session. Associate it with
     * the file so we can access it in the other methods.
     */
    rc = VGDrvCommonCreateUserSession(&g_DevExt, fRequestor, &pSession);
    if (RT_SUCCESS(rc))
        pFilp->private_data = pSession;

    Log(("vgdrvLinuxOpen: g_DevExt=%p pSession=%p rc=%d/%d (pid=%d/%d %s)\n",
         &g_DevExt, pSession, rc, vgdrvLinuxConvertToNegErrno(rc), RTProcSelf(), current->pid, current->comm));
    return vgdrvLinuxConvertToNegErrno(rc);
}


/**
 * Close device.
 *
 * @param   pInode      Pointer to inode info structure.
 * @param   pFilp       Associated file pointer.
 */
static int vgdrvLinuxRelease(struct inode *pInode, struct file *pFilp)
{
    Log(("vgdrvLinuxRelease: pFilp=%p pSession=%p pid=%d/%d %s\n",
         pFilp, pFilp->private_data, RTProcSelf(), current->pid, current->comm));

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
    /* This housekeeping was needed in older kernel versions to ensure that
     * the file pointer didn't get left on the polling queue. */
    vgdrvLinuxFAsync(-1, pFilp, 0);
#endif
    VGDrvCommonCloseSession(&g_DevExt, (PVBOXGUESTSESSION)pFilp->private_data);
    pFilp->private_data = NULL;
    return 0;
}


/**
 * Device I/O Control entry point.
 *
 * @param   pFilp       Associated file pointer.
 * @param   uCmd        The function specified to ioctl().
 * @param   ulArg       The argument specified to ioctl().
 */
#if defined(HAVE_UNLOCKED_IOCTL) || defined(DOXYGEN_RUNNING)
static long vgdrvLinuxIOCtl(struct file *pFilp, unsigned int uCmd, unsigned long ulArg)
#else
static int vgdrvLinuxIOCtl(struct inode *pInode, struct file *pFilp, unsigned int uCmd, unsigned long ulArg)
#endif
{
    PVBOXGUESTSESSION pSession = (PVBOXGUESTSESSION)pFilp->private_data;
    int rc;
#ifndef HAVE_UNLOCKED_IOCTL
    unlock_kernel();
#endif

#if 0 /* no fast I/O controls defined atm. */
    if (RT_LIKELY(   (   uCmd == SUP_IOCTL_FAST_DO_RAW_RUN
                      || uCmd == SUP_IOCTL_FAST_DO_HM_RUN
                      || uCmd == SUP_IOCTL_FAST_DO_NOP)
                  && pSession->fUnrestricted == true))
        rc = VGDrvCommonIoCtlFast(uCmd, ulArg, &g_DevExt, pSession);
    else
#endif
        rc = vgdrvLinuxIOCtlSlow(pFilp, uCmd, ulArg, pSession);

#ifndef HAVE_UNLOCKED_IOCTL
    lock_kernel();
#endif
    return rc;
}


/**
 * Device I/O Control entry point, slow variant.
 *
 * @param   pFilp       Associated file pointer.
 * @param   uCmd        The function specified to ioctl().
 * @param   ulArg       The argument specified to ioctl().
 * @param   pSession    The session instance.
 */
static int vgdrvLinuxIOCtlSlow(struct file *pFilp, unsigned int uCmd, unsigned long ulArg, PVBOXGUESTSESSION pSession)
{
    int                 rc;
    VBGLREQHDR          Hdr;
    PVBGLREQHDR         pHdr;
    uint32_t            cbBuf;

    Log6(("vgdrvLinuxIOCtlSlow: pFilp=%p uCmd=%#x ulArg=%p pid=%d/%d\n", pFilp, uCmd, (void *)ulArg, RTProcSelf(), current->pid));

    /*
     * Read the header.
     */
    if (RT_FAILURE(RTR0MemUserCopyFrom(&Hdr, ulArg, sizeof(Hdr))))
    {
        Log(("vgdrvLinuxIOCtlSlow: copy_from_user(,%#lx,) failed; uCmd=%#x\n", ulArg, uCmd));
        return -EFAULT;
    }
    if (RT_UNLIKELY(Hdr.uVersion != VBGLREQHDR_VERSION))
    {
        Log(("vgdrvLinuxIOCtlSlow: bad header version %#x; uCmd=%#x\n", Hdr.uVersion, uCmd));
        return -EINVAL;
    }

    /*
     * Buffer the request.
     * Note! The header is revalidated by the common code.
     */
    cbBuf = RT_MAX(Hdr.cbIn, Hdr.cbOut);
    if (RT_UNLIKELY(cbBuf > _1M*16))
    {
        Log(("vgdrvLinuxIOCtlSlow: too big cbBuf=%#x; uCmd=%#x\n", cbBuf, uCmd));
        return -E2BIG;
    }
    if (RT_UNLIKELY(   Hdr.cbIn < sizeof(Hdr)
                    || (cbBuf != _IOC_SIZE(uCmd) && _IOC_SIZE(uCmd) != 0)))
    {
        Log(("vgdrvLinuxIOCtlSlow: bad ioctl cbBuf=%#x _IOC_SIZE=%#x; uCmd=%#x\n", cbBuf, _IOC_SIZE(uCmd), uCmd));
        return -EINVAL;
    }
    pHdr = RTMemAlloc(cbBuf);
    if (RT_UNLIKELY(!pHdr))
    {
        LogRel(("vgdrvLinuxIOCtlSlow: failed to allocate buffer of %d bytes for uCmd=%#x\n", cbBuf, uCmd));
        return -ENOMEM;
    }
    if (RT_FAILURE(RTR0MemUserCopyFrom(pHdr, ulArg, Hdr.cbIn)))
    {
        Log(("vgdrvLinuxIOCtlSlow: copy_from_user(,%#lx, %#x) failed; uCmd=%#x\n", ulArg, Hdr.cbIn, uCmd));
        RTMemFree(pHdr);
        return -EFAULT;
    }
    if (Hdr.cbIn < cbBuf)
        RT_BZERO((uint8_t *)pHdr + Hdr.cbIn, cbBuf - Hdr.cbIn);

    /*
     * Process the IOCtl.
     */
    rc = VGDrvCommonIoCtl(uCmd, &g_DevExt, pSession, pHdr, cbBuf);

    /*
     * Copy ioctl data and output buffer back to user space.
     */
    if (RT_SUCCESS(rc))
    {
        uint32_t cbOut = pHdr->cbOut;
        if (RT_UNLIKELY(cbOut > cbBuf))
        {
            LogRel(("vgdrvLinuxIOCtlSlow: too much output! %#x > %#x; uCmd=%#x!\n", cbOut, cbBuf, uCmd));
            cbOut = cbBuf;
        }
        if (RT_FAILURE(RTR0MemUserCopyTo(ulArg, pHdr, cbOut)))
        {
            /* this is really bad! */
            LogRel(("vgdrvLinuxIOCtlSlow: copy_to_user(%#lx,,%#x); uCmd=%#x!\n", ulArg, cbOut, uCmd));
            rc = -EFAULT;
        }
    }
    else
    {
        Log(("vgdrvLinuxIOCtlSlow: pFilp=%p uCmd=%#x ulArg=%p failed, rc=%d\n", pFilp, uCmd, (void *)ulArg, rc));
        rc = -EINVAL;
    }
    RTMemFree(pHdr);

    Log6(("vgdrvLinuxIOCtlSlow: returns %d (pid=%d/%d)\n", rc, RTProcSelf(), current->pid));
    return rc;
}


/**
 * @note This code is duplicated on other platforms with variations, so please
 *       keep them all up to date when making changes!
 */
int VBOXCALL VBoxGuestIDC(void *pvSession, uintptr_t uReq, PVBGLREQHDR pReqHdr, size_t cbReq)
{
    /*
     * Simple request validation (common code does the rest).
     */
    int rc;
    if (   RT_VALID_PTR(pReqHdr)
        && cbReq >= sizeof(*pReqHdr))
    {
        /*
         * All requests except the connect one requires a valid session.
         */
        PVBOXGUESTSESSION pSession = (PVBOXGUESTSESSION)pvSession;
        if (pSession)
        {
            if (   RT_VALID_PTR(pSession)
                && pSession->pDevExt == &g_DevExt)
                rc = VGDrvCommonIoCtl(uReq, &g_DevExt, pSession, pReqHdr, cbReq);
            else
                rc = VERR_INVALID_HANDLE;
        }
        else if (uReq == VBGL_IOCTL_IDC_CONNECT)
        {
            rc = VGDrvCommonCreateKernelSession(&g_DevExt, &pSession);
            if (RT_SUCCESS(rc))
            {
                rc = VGDrvCommonIoCtl(uReq, &g_DevExt, pSession, pReqHdr, cbReq);
                if (RT_FAILURE(rc))
                    VGDrvCommonCloseSession(&g_DevExt, pSession);
            }
        }
        else
            rc = VERR_INVALID_HANDLE;
    }
    else
        rc = VERR_INVALID_POINTER;
    return rc;
}
EXPORT_SYMBOL(VBoxGuestIDC);


/**
 * Asynchronous notification activation method.
 *
 * @returns 0 on success, negative errno on failure.
 *
 * @param   fd          The file descriptor.
 * @param   pFile       The file structure.
 * @param   fOn         On/off indicator.
 */
static int vgdrvLinuxFAsync(int fd, struct file *pFile, int fOn)
{
    return fasync_helper(fd, pFile, fOn, &g_pFAsyncQueue);
}


/**
 * Poll function.
 *
 * This returns ready to read if the mouse pointer mode or the pointer position
 * has changed since last call to read.
 *
 * @returns 0 if no changes, POLLIN | POLLRDNORM if there are unseen changes.
 *
 * @param   pFile       The file structure.
 * @param   pPt         The poll table.
 *
 * @remarks This is probably not really used, X11 is said to use the fasync
 *          interface instead.
 */
static unsigned int vgdrvLinuxPoll(struct file *pFile, poll_table *pPt)
{
    PVBOXGUESTSESSION   pSession  = (PVBOXGUESTSESSION)pFile->private_data;
    uint32_t            u32CurSeq = ASMAtomicUoReadU32(&g_DevExt.u32MousePosChangedSeq);
    unsigned int        fMask     = pSession->u32MousePosChangedSeq != u32CurSeq
                                  ? POLLIN | POLLRDNORM
                                  : 0;
    poll_wait(pFile, &g_PollEventQueue, pPt);
    return fMask;
}


/**
 * Read to go with our poll/fasync response.
 *
 * @returns 1 or -EINVAL.
 *
 * @param   pFile       The file structure.
 * @param   pbBuf       The buffer to read into.
 * @param   cbRead      The max number of bytes to read.
 * @param   poff        The current file position.
 *
 * @remarks This is probably not really used as X11 lets the driver do its own
 *          event reading. The poll condition is therefore also cleared when we
 *          see VMMDevReq_GetMouseStatus in vgdrvIoCtl_VMMRequest.
 */
static ssize_t vgdrvLinuxRead(struct file *pFile, char *pbBuf, size_t cbRead, loff_t *poff)
{
    PVBOXGUESTSESSION   pSession  = (PVBOXGUESTSESSION)pFile->private_data;
    uint32_t            u32CurSeq = ASMAtomicUoReadU32(&g_DevExt.u32MousePosChangedSeq);

    if (*poff != 0)
        return -EINVAL;

    /*
     * Fake a single byte read if we're not up to date with the current mouse position.
     */
    if (    pSession->u32MousePosChangedSeq != u32CurSeq
        &&  cbRead > 0)
    {
        pSession->u32MousePosChangedSeq = u32CurSeq;
        pbBuf[0] = 0;
        return 1;
    }
    return 0;
}


void VGDrvNativeISRMousePollEvent(PVBOXGUESTDEVEXT pDevExt)
{
#ifdef VBOXGUEST_WITH_INPUT_DRIVER
    int rc;
#endif
    NOREF(pDevExt);

    /*
     * Wake up everyone that's in a poll() and post anyone that has
     * subscribed to async notifications.
     */
    Log3(("VGDrvNativeISRMousePollEvent: wake_up_all\n"));
    wake_up_all(&g_PollEventQueue);
    Log3(("VGDrvNativeISRMousePollEvent: kill_fasync\n"));
    kill_fasync(&g_pFAsyncQueue, SIGIO, POLL_IN);
#ifdef VBOXGUEST_WITH_INPUT_DRIVER
    /* Report events to the kernel input device */
    g_pMouseStatusReq->mouseFeatures = 0;
    g_pMouseStatusReq->pointerXPos = 0;
    g_pMouseStatusReq->pointerYPos = 0;
    rc = VbglR0GRPerform(&g_pMouseStatusReq->header);
    if (RT_SUCCESS(rc))
    {
        input_report_abs(g_pInputDevice, ABS_X,
                         g_pMouseStatusReq->pointerXPos);
        input_report_abs(g_pInputDevice, ABS_Y,
                         g_pMouseStatusReq->pointerYPos);
# ifdef EV_SYN
        input_sync(g_pInputDevice);
# endif
    }
#endif
    Log3(("VGDrvNativeISRMousePollEvent: done\n"));
}


bool VGDrvNativeProcessOption(PVBOXGUESTDEVEXT pDevExt, const char *pszName, const char *pszValue)
{
    RT_NOREF(pDevExt); RT_NOREF(pszName); RT_NOREF(pszValue);
    return false;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)

/** log and dbg_log parameter setter. */
static int vgdrvLinuxParamLogGrpSet(const char *pszValue, CONST_4_15 struct kernel_param *pParam)
{
    if (g_fLoggerCreated)
    {
        PRTLOGGER pLogger = pParam->name[0] == 'd' ? RTLogDefaultInstance() : RTLogRelGetDefaultInstance();
        if (pLogger)
            RTLogGroupSettings(pLogger, pszValue);
    }
    else if (pParam->name[0] != 'd')
        strlcpy(&g_szLogGrp[0], pszValue, sizeof(g_szLogGrp));

    return 0;
}

/** log and dbg_log parameter getter. */
static int vgdrvLinuxParamLogGrpGet(char *pszBuf, CONST_4_15 struct kernel_param *pParam)
{
    PRTLOGGER pLogger = pParam->name[0] == 'd' ? RTLogDefaultInstance() : RTLogRelGetDefaultInstance();
    *pszBuf = '\0';
    if (pLogger)
        RTLogGetGroupSettings(pLogger, pszBuf, _4K);
    return strlen(pszBuf);
}


/** log and dbg_log_flags parameter setter. */
static int vgdrvLinuxParamLogFlagsSet(const char *pszValue, CONST_4_15 struct kernel_param *pParam)
{
    if (g_fLoggerCreated)
    {
        PRTLOGGER pLogger = pParam->name[0] == 'd' ? RTLogDefaultInstance() : RTLogRelGetDefaultInstance();
        if (pLogger)
            RTLogFlags(pLogger, pszValue);
    }
    else if (pParam->name[0] != 'd')
        strlcpy(&g_szLogFlags[0], pszValue, sizeof(g_szLogFlags));
    return 0;
}

/** log and dbg_log_flags parameter getter. */
static int vgdrvLinuxParamLogFlagsGet(char *pszBuf, CONST_4_15 struct kernel_param *pParam)
{
    PRTLOGGER pLogger = pParam->name[0] == 'd' ? RTLogDefaultInstance() : RTLogRelGetDefaultInstance();
    *pszBuf = '\0';
    if (pLogger)
        RTLogGetFlags(pLogger, pszBuf, _4K);
    return strlen(pszBuf);
}


/** log and dbg_log_dest parameter setter. */
static int vgdrvLinuxParamLogDstSet(const char *pszValue, CONST_4_15 struct kernel_param *pParam)
{
    if (g_fLoggerCreated)
    {
        PRTLOGGER pLogger = pParam->name[0] == 'd' ? RTLogDefaultInstance() : RTLogRelGetDefaultInstance();
        if (pLogger)
            RTLogDestinations(pLogger, pszValue);
    }
    else if (pParam->name[0] != 'd')
        strlcpy(&g_szLogDst[0], pszValue, sizeof(g_szLogDst));
    return 0;
}

/** log and dbg_log_dest parameter getter. */
static int vgdrvLinuxParamLogDstGet(char *pszBuf, CONST_4_15 struct kernel_param *pParam)
{
    PRTLOGGER pLogger = pParam->name[0] == 'd' ? RTLogDefaultInstance() : RTLogRelGetDefaultInstance();
    *pszBuf = '\0';
    if (pLogger)
        RTLogGetDestinations(pLogger, pszBuf, _4K);
    return strlen(pszBuf);
}


/** r3_log_to_host parameter setter. */
static int vgdrvLinuxParamR3LogToHostSet(const char *pszValue, CONST_4_15 struct kernel_param *pParam)
{
    g_DevExt.fLoggingEnabled = VBDrvCommonIsOptionValueTrue(pszValue);
    return 0;
}

/** r3_log_to_host parameter getter. */
static int vgdrvLinuxParamR3LogToHostGet(char *pszBuf, CONST_4_15 struct kernel_param *pParam)
{
    strcpy(pszBuf, g_DevExt.fLoggingEnabled ? "enabled" : "disabled");
    return strlen(pszBuf);
}


/*
 * Define module parameters.
 */
module_param_call(log,            vgdrvLinuxParamLogGrpSet,   vgdrvLinuxParamLogGrpGet,   NULL, 0664);
module_param_call(log_flags,      vgdrvLinuxParamLogFlagsSet, vgdrvLinuxParamLogFlagsGet, NULL, 0664);
module_param_call(log_dest,       vgdrvLinuxParamLogDstSet,   vgdrvLinuxParamLogDstGet,   NULL, 0664);
# ifdef LOG_ENABLED
module_param_call(dbg_log,        vgdrvLinuxParamLogGrpSet,   vgdrvLinuxParamLogGrpGet,   NULL, 0664);
module_param_call(dbg_log_flags,  vgdrvLinuxParamLogFlagsSet, vgdrvLinuxParamLogFlagsGet, NULL, 0664);
module_param_call(dbg_log_dest,   vgdrvLinuxParamLogDstSet,   vgdrvLinuxParamLogDstGet,   NULL, 0664);
# endif
module_param_call(r3_log_to_host, vgdrvLinuxParamR3LogToHostSet, vgdrvLinuxParamR3LogToHostGet, NULL, 0664);

#endif /* 2.6.0 and later */


module_init(vgdrvLinuxModInit);
module_exit(vgdrvLinuxModExit);

MODULE_AUTHOR(VBOX_VENDOR);
MODULE_DESCRIPTION(VBOX_PRODUCT " Guest Additions for Linux Module");
MODULE_LICENSE("GPL");
#ifdef MODULE_VERSION
MODULE_VERSION(VBOX_VERSION_STRING " r" RT_XSTR(VBOX_SVN_REV));
#endif

