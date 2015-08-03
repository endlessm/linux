/* $Id: log-vbox.cpp $ */
/** @file
 * VirtualBox Runtime - Logging configuration.
 */

/*
 * Copyright (C) 2006-2015 Oracle Corporation
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

/** @page pg_rtlog      Runtime - Logging
 *
 * VBox uses the IPRT logging system which supports group level flags and multiple
 * destinations. The GC logging is making it even more interesting since GC logging will
 * have to be buffered and written when back in host context.
 *
 * [more later]
 *
 *
 * @section sec_logging_destination     The Destination Specifier.
 *
 * The {logger-env-base}_DEST environment variable can be used to specify where
 * the log output goes. The following specifiers are recognized:
 *
 *      - file=\<filename\>
 *        This sets the logger output filename to \<filename\>. Not formatting
 *        or anything is supported. Each logger specifies a default name if
 *        file logging should be enabled by default.
 *
 *      - nofile
 *        This disables the file output.
 *
 *      - stdout
 *        Enables logger output to stdout.
 *
 *      - nostdout
 *        Disables logger output to stdout.
 *
 *      - stderr
 *        Enables logger output to stderr.
 *
 *      - nostderr
 *        Disables logger output to stderr.
 *
 *      - debugger
 *        Enables logger output to native debugger. (Win32/64 only)
 *
 *      - nodebugger
 *        Disables logger output to native debugger. (Win32/64 only)
 *
 *      - user
 *        Enables logger output to special backdoor if in guest r0.
 *
 *      - nodebugger
 *        Disables logger output to special user stream.
 *
 *
 *
 * @section sec_logging_destination     The Group Specifier.
 *
 * The {logger-env-base} environment variable can be used to specify which
 * logger groups to enable and which to disable. By default all groups are
 * disabled. For your convenience this specifier is case in-sensitive (ASCII).
 *
 * The specifier is evaluated from left to right.
 *
 * [more later]
 *
 * The groups settings can be reprogrammed during execution using the
 * RTLogGroupSettings() command and a group specifier.
 *
 *
 *
 * @section sec_logging_default         The Default Logger
 *
 * The default logger uses VBOX_LOG_DEST as destination specifier. File output is
 * enabled by default and goes to a file "./VBox-\<pid\>.log".
 *
 * The default logger have all groups turned off by default to force the developer
 * to be careful with what log information to collect - logging everything is
 * generally NOT a good idea.
 *
 * The log groups of the default logger can be found in the LOGGROUP in enum. The
 * VBOX_LOG environment variable and the .log debugger command can be used to
 * configure the groups.
 *
 * Each group have flags in addition to the enable/disable flag. These flags can
 * be appended to the group name using dot separators. The flags correspond to
 * RTLOGGRPFLAGS and have a short and a long version:
 *
 *      - e - Enabled:  Whether the group is enabled at all.
 *      - l - Level2:   Level-2 logging.
 *      - f - Flow:     Execution flow logging (entry messages)
 *      - s - Sander:   Special Sander logging messages.
 *      - b - Bird:     Special Bird logging messages.
 *
 * @todo Update this section...
 *
 * Example:
 *
 *      VBOX_LOG=+all+pgm.e.s.b.z.l-qemu
 *
 * Space and ';' separators are allowed:
 *
 *      VBOX_LOG=+all +pgm.e.s.b.z.l ; - qemu
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#ifdef IN_RING3
# if defined(RT_OS_WINDOWS)
#  include <Windows.h>
# elif defined(RT_OS_LINUX)
#  include <unistd.h>
# elif defined(RT_OS_FREEBSD)
#  include <sys/param.h>
#  include <sys/sysctl.h>
#  include <sys/user.h>
#  include <stdlib.h>
#  include <unistd.h>
# elif defined(RT_OS_HAIKU)
#  include <OS.h>
# elif defined(RT_OS_SOLARIS)
#  define _STRUCTURED_PROC 1
#  undef _FILE_OFFSET_BITS /* procfs doesn't like this */
#  include <sys/procfs.h>
#  include <unistd.h>
# elif defined(RT_OS_OS2)
#  include <stdlib.h>
# endif
#endif

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/err.h>
#include <iprt/time.h>
#ifdef IN_RING3
# include <iprt/param.h>
# include <iprt/assert.h>
# include <iprt/path.h>
# include <iprt/process.h>
# include <iprt/string.h>
# include <iprt/mem.h>
# include <stdio.h>
#endif
#if defined(IN_RING0) && defined(RT_OS_DARWIN)
# include <iprt/asm-amd64-x86.h>
# include <iprt/thread.h>
#endif


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The default logger. */
static PRTLOGGER                    g_pLogger = NULL;
/** The default logger groups.
 * This must match LOGGROUP! */
static const char                  *g_apszGroups[] =
VBOX_LOGGROUP_NAMES;


/**
 * Creates the default logger instance for a VBox process.
 *
 * @returns Pointer to the logger instance.
 */
RTDECL(PRTLOGGER) RTLogDefaultInit(void)
{
    /*
     * Initialize the default logger instance.
     * Take care to do this once and not recursively.
     */
    static volatile uint32_t fInitializing = 0;
    PRTLOGGER pLogger;
    int rc;

    if (g_pLogger || !ASMAtomicCmpXchgU32(&fInitializing, 1, 0))
        return g_pLogger;

#ifdef IN_RING3
    /*
     * Assert the group definitions.
     */
#define ASSERT_LOG_GROUP(grp)  ASSERT_LOG_GROUP2(LOG_GROUP_##grp, #grp)
#define ASSERT_LOG_GROUP2(def, str) \
    do { if (strcmp(g_apszGroups[def], str)) {printf("%s='%s' expects '%s'\n", #def, g_apszGroups[def], str); RTAssertDoPanic(); } } while (0)
    ASSERT_LOG_GROUP(DEFAULT);
    ASSERT_LOG_GROUP(AUDIO_MIXER);
    ASSERT_LOG_GROUP(AUDIO_MIXER_BUFFER);
    ASSERT_LOG_GROUP(CFGM);
    ASSERT_LOG_GROUP(CPUM);
    ASSERT_LOG_GROUP(CSAM);
    ASSERT_LOG_GROUP(DBGC);
    ASSERT_LOG_GROUP(DBGF);
    ASSERT_LOG_GROUP(DBGF_INFO);
    ASSERT_LOG_GROUP(DEV);
    ASSERT_LOG_GROUP(DEV_AC97);
    ASSERT_LOG_GROUP(DEV_ACPI);
    ASSERT_LOG_GROUP(DEV_APIC);
    ASSERT_LOG_GROUP(DEV_FDC);
    ASSERT_LOG_GROUP(DEV_HDA);
    ASSERT_LOG_GROUP(DEV_HDA_CODEC);
    ASSERT_LOG_GROUP(DEV_HPET);
    ASSERT_LOG_GROUP(DEV_IDE);
    ASSERT_LOG_GROUP(DEV_KBD);
    ASSERT_LOG_GROUP(DEV_LPC);
    ASSERT_LOG_GROUP(DEV_NE2000);
    ASSERT_LOG_GROUP(DEV_PC);
    ASSERT_LOG_GROUP(DEV_PC_ARCH);
    ASSERT_LOG_GROUP(DEV_PC_BIOS);
    ASSERT_LOG_GROUP(DEV_PCI);
    ASSERT_LOG_GROUP(DEV_PCNET);
    ASSERT_LOG_GROUP(DEV_PIC);
    ASSERT_LOG_GROUP(DEV_PIT);
    ASSERT_LOG_GROUP(DEV_RTC);
    ASSERT_LOG_GROUP(DEV_SB16);
    ASSERT_LOG_GROUP(DEV_SERIAL);
    ASSERT_LOG_GROUP(DEV_SMC);
    ASSERT_LOG_GROUP(DEV_VGA);
    ASSERT_LOG_GROUP(DEV_VMM);
    ASSERT_LOG_GROUP(DEV_VMM_STDERR);
    ASSERT_LOG_GROUP(DIS);
    ASSERT_LOG_GROUP(DRV);
    ASSERT_LOG_GROUP(DRV_ACPI);
    ASSERT_LOG_GROUP(DRV_AUDIO);
    ASSERT_LOG_GROUP(DRV_BLOCK);
    ASSERT_LOG_GROUP(DRV_FLOPPY);
    ASSERT_LOG_GROUP(DRV_HOST_AUDIO);
    ASSERT_LOG_GROUP(DRV_HOST_DVD);
    ASSERT_LOG_GROUP(DRV_HOST_FLOPPY);
    ASSERT_LOG_GROUP(DRV_ISO);
    ASSERT_LOG_GROUP(DRV_KBD_QUEUE);
    ASSERT_LOG_GROUP(DRV_MOUSE_QUEUE);
    ASSERT_LOG_GROUP(DRV_NAT);
    ASSERT_LOG_GROUP(DRV_RAW_IMAGE);
    ASSERT_LOG_GROUP(DRV_TUN);
    ASSERT_LOG_GROUP(DRV_USBPROXY);
    ASSERT_LOG_GROUP(DRV_VBOXHDD);
    ASSERT_LOG_GROUP(DRV_VRDE_AUDIO);
    ASSERT_LOG_GROUP(DRV_VSWITCH);
    ASSERT_LOG_GROUP(DRV_VUSB);
    ASSERT_LOG_GROUP(EM);
    ASSERT_LOG_GROUP(GUI);
    ASSERT_LOG_GROUP(HGCM);
    ASSERT_LOG_GROUP(HM);
    ASSERT_LOG_GROUP(IOM);
    ASSERT_LOG_GROUP(LWIP);
    ASSERT_LOG_GROUP(MAIN);
    ASSERT_LOG_GROUP(MM);
    ASSERT_LOG_GROUP(MM_HEAP);
    ASSERT_LOG_GROUP(MM_HYPER);
    ASSERT_LOG_GROUP(MM_HYPER_HEAP);
    ASSERT_LOG_GROUP(MM_PHYS);
    ASSERT_LOG_GROUP(MM_POOL);
    ASSERT_LOG_GROUP(NAT_SERVICE);
    ASSERT_LOG_GROUP(NET_SERVICE);
    ASSERT_LOG_GROUP(PATM);
    ASSERT_LOG_GROUP(PDM);
    ASSERT_LOG_GROUP(PDM_DEVICE);
    ASSERT_LOG_GROUP(PDM_DRIVER);
    ASSERT_LOG_GROUP(PDM_LDR);
    ASSERT_LOG_GROUP(PDM_QUEUE);
    ASSERT_LOG_GROUP(PGM);
    ASSERT_LOG_GROUP(PGM_POOL);
    ASSERT_LOG_GROUP(REM);
    ASSERT_LOG_GROUP(REM_DISAS);
    ASSERT_LOG_GROUP(REM_HANDLER);
    ASSERT_LOG_GROUP(REM_IOPORT);
    ASSERT_LOG_GROUP(REM_MMIO);
    ASSERT_LOG_GROUP(REM_PRINTF);
    ASSERT_LOG_GROUP(REM_RUN);
    ASSERT_LOG_GROUP(SELM);
    ASSERT_LOG_GROUP(SSM);
    ASSERT_LOG_GROUP(STAM);
    ASSERT_LOG_GROUP(SUP);
    ASSERT_LOG_GROUP(TM);
    ASSERT_LOG_GROUP(TRPM);
    ASSERT_LOG_GROUP(VM);
    ASSERT_LOG_GROUP(VMM);
    ASSERT_LOG_GROUP(VRDP);
#undef ASSERT_LOG_GROUP
#undef ASSERT_LOG_GROUP2
#endif /* IN_RING3 */

    /*
     * Create the default logging instance.
     */
#ifdef IN_RING3
# ifndef IN_GUEST
    char szExecName[RTPATH_MAX];
    if (!RTProcGetExecutablePath(szExecName, sizeof(szExecName)))
        strcpy(szExecName, "VBox");
    RTTIMESPEC TimeSpec;
    RTTIME Time;
    RTTimeExplode(&Time, RTTimeNow(&TimeSpec));
    rc = RTLogCreate(&pLogger, 0, NULL, "VBOX_LOG", RT_ELEMENTS(g_apszGroups), &g_apszGroups[0], RTLOGDEST_FILE,
                     "./%04d-%02d-%02d-%02d-%02d-%02d.%03d-%s-%d.log",
                     Time.i32Year, Time.u8Month, Time.u8MonthDay, Time.u8Hour, Time.u8Minute, Time.u8Second, Time.u32Nanosecond / 10000000,
                     RTPathFilename(szExecName), RTProcSelf());
    if (RT_SUCCESS(rc))
    {
        /*
         * Write a log header.
         */
        char szBuf[RTPATH_MAX];
        RTTimeSpecToString(&TimeSpec, szBuf, sizeof(szBuf));
        RTLogLoggerEx(pLogger, 0, ~0U, "Log created: %s\n", szBuf);
        RTLogLoggerEx(pLogger, 0, ~0U, "Executable: %s\n", szExecName);

        /* executable and arguments - tricky and all platform specific. */
#  if defined(RT_OS_WINDOWS)
        RTLogLoggerEx(pLogger, 0, ~0U, "Commandline: %ls\n", GetCommandLineW());

#  elif defined(RT_OS_SOLARIS)
        psinfo_t psi;
        char szArgFileBuf[80];
        RTStrPrintf(szArgFileBuf, sizeof(szArgFileBuf), "/proc/%ld/psinfo", (long)getpid());
        FILE* pFile = fopen(szArgFileBuf, "rb");
        if (pFile)
        {
            if (fread(&psi, sizeof(psi), 1, pFile) == 1)
            {
#   if 0     /* 100% safe:*/
                RTLogLoggerEx(pLogger, 0, ~0U, "Args: %s\n", psi.pr_psargs);
#   else     /* probably safe: */
                const char * const *argv = (const char * const *)psi.pr_argv;
                for (int iArg = 0; iArg < psi.pr_argc; iArg++)
                    RTLogLoggerEx(pLogger, 0, ~0U, "Arg[%d]: %s\n", iArg, argv[iArg]);
#   endif

            }
            fclose(pFile);
        }

#  elif defined(RT_OS_LINUX)
        FILE *pFile = fopen("/proc/self/cmdline", "r");
        if (pFile)
        {
            /* braindead */
            unsigned iArg = 0;
            int ch;
            bool fNew = true;
            while (!feof(pFile) && (ch = fgetc(pFile)) != EOF)
            {
                if (fNew)
                {
                    RTLogLoggerEx(pLogger, 0, ~0U, "Arg[%u]: ", iArg++);
                    fNew = false;
                }
                if (ch)
                    RTLogLoggerEx(pLogger, 0, ~0U, "%c", ch);
                else
                {
                    RTLogLoggerEx(pLogger, 0, ~0U, "\n");
                    fNew = true;
                }
            }
            if (!fNew)
                RTLogLoggerEx(pLogger, 0, ~0U, "\n");
            fclose(pFile);
        }

#  elif defined(RT_OS_HAIKU)
        team_info info;
        if (get_team_info(0, &info) == B_OK)
        {
            /* there is an info.argc, but no way to know arg boundaries */
            RTLogLoggerEx(pLogger, 0, ~0U, "Commandline: %.64s\n", info.args);
        }

#  elif defined(RT_OS_FREEBSD)
        /* Retrieve the required length first */
        int aiName[4];
        aiName[0] = CTL_KERN;
        aiName[1] = KERN_PROC;
        aiName[2] = KERN_PROC_ARGS;     /* Introduced in FreeBSD 4.0 */
        aiName[3] = getpid();
        size_t cchArgs = 0;
        int rcBSD = sysctl(aiName, RT_ELEMENTS(aiName), NULL, &cchArgs, NULL, 0);
        if (cchArgs > 0)
        {
            char *pszArgFileBuf = (char *)RTMemAllocZ(cchArgs + 1 /* Safety */);
            if (pszArgFileBuf)
            {
                /* Retrieve the argument list */
                rcBSD = sysctl(aiName, RT_ELEMENTS(aiName), pszArgFileBuf, &cchArgs, NULL, 0);
                if (!rcBSD)
                {
                    unsigned    iArg = 0;
                    size_t      off = 0;
                    while (off < cchArgs)
                    {
                        size_t cchArg = strlen(&pszArgFileBuf[off]);
                        RTLogLoggerEx(pLogger, 0, ~0U, "Arg[%u]: %s\n", iArg, &pszArgFileBuf[off]);

                        /* advance */
                        off += cchArg + 1;
                        iArg++;
                    }
                }
                RTMemFree(pszArgFileBuf);
            }
        }

#  elif defined(RT_OS_OS2) || defined(RT_OS_DARWIN)
        /* commandline? */
#  else
#   error needs porting.
#  endif
    }

# else  /* IN_GUEST */
    /* The user destination is backdoor logging. */
    rc = RTLogCreate(&pLogger, 0, NULL, "VBOX_LOG", RT_ELEMENTS(g_apszGroups), &g_apszGroups[0], RTLOGDEST_USER, "VBox.log");
# endif /* IN_GUEST */

#else /* IN_RING0 */

    /* Some platforms has trouble allocating memory with interrupts and/or
       preemption disabled. Check and fail before we panic. */
# if defined(RT_OS_DARWIN)
    if (   !ASMIntAreEnabled()
        || !RTThreadPreemptIsEnabled(NIL_RTTHREAD))
        return NULL;
# endif

# ifndef IN_GUEST
    rc = RTLogCreate(&pLogger, 0, NULL, "VBOX_LOG", RT_ELEMENTS(g_apszGroups), &g_apszGroups[0], RTLOGDEST_FILE, "VBox-ring0.log");
# else  /* IN_GUEST */
    rc = RTLogCreate(&pLogger, 0, NULL, "VBOX_LOG", RT_ELEMENTS(g_apszGroups), &g_apszGroups[0], RTLOGDEST_USER, "VBox-ring0.log");
# endif /* IN_GUEST */
    if (RT_SUCCESS(rc))
    {
        /*
         * This is where you set your ring-0 logging preferences.
         *
         * On platforms which don't differ between debugger and kernel
         * log printing, STDOUT is gonna be a stub and the DEBUGGER
         * destination is the one doing all the work. On platforms
         * that do differ (like Darwin), STDOUT is the kernel log.
         */
# if defined(DEBUG_bird)
        /*RTLogGroupSettings(pLogger, "all=~0 -default.l6.l5.l4.l3");*/
        RTLogFlags(pLogger, "enabled unbuffered pid tid");
#  ifndef IN_GUEST
        pLogger->fDestFlags |= RTLOGDEST_DEBUGGER | RTLOGDEST_STDOUT;
#  endif
# endif
# if defined(DEBUG_sandervl) && !defined(IN_GUEST)
        RTLogGroupSettings(pLogger, "+all");
        RTLogFlags(pLogger, "enabled unbuffered");
        pLogger->fDestFlags |= RTLOGDEST_DEBUGGER;
# endif
# if defined(DEBUG_ramshankar)  /* Guest ring-0 as well */
        RTLogGroupSettings(pLogger, "+all.e.l.f");
        RTLogFlags(pLogger, "enabled unbuffered");
        pLogger->fDestFlags |= RTLOGDEST_DEBUGGER;
# endif
# if defined(DEBUG_aleksey)  /* Guest ring-0 as well */
        RTLogGroupSettings(pLogger, "net_flt_drv.e.l.f.l3.l4.l5 +net_adp_drv.e.l.f.l3.l4.l5");
        RTLogFlags(pLogger, "enabled unbuffered");
        pLogger->fDestFlags |= RTLOGDEST_DEBUGGER | RTLOGDEST_STDOUT;
# endif
# if defined(DEBUG_andy)  /* Guest ring-0 as well */
        RTLogGroupSettings(pLogger, "+all.e.l.f");
        RTLogFlags(pLogger, "enabled unbuffered pid tid");
        pLogger->fDestFlags |= RTLOGDEST_DEBUGGER | RTLOGDEST_STDOUT;
# endif
# if defined(DEBUG_misha) /* Guest ring-0 as well */
        RTLogFlags(pLogger, "enabled unbuffered");
        pLogger->fDestFlags |= RTLOGDEST_DEBUGGER;
# endif
# if defined(DEBUG_michael) && defined(IN_GUEST)
        RTLogGroupSettings(pLogger, "+all.e.l.f");
        RTLogFlags(pLogger, "enabled unbuffered");
        pLogger->fDestFlags |= RTLOGDEST_DEBUGGER | RTLOGDEST_STDOUT;
# endif
# if 0 /* vboxdrv logging - ATTENTION: this is what we're referring to guys! Change to '# if 1'. */
        RTLogGroupSettings(pLogger, "all=~0 -default.l6.l5.l4.l3");
        RTLogFlags(pLogger, "enabled unbuffered tid");
        pLogger->fDestFlags |= RTLOGDEST_DEBUGGER | RTLOGDEST_STDOUT;
# endif
    }
#endif /* IN_RING0 */
    return g_pLogger = RT_SUCCESS(rc) ? pLogger : NULL;
}
