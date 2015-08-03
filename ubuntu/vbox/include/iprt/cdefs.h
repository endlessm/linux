/** @file
 * IPRT - Common C and C++ definitions.
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

#ifndef ___iprt_cdefs_h
#define ___iprt_cdefs_h


/** @defgroup grp_rt_cdefs  IPRT Common Definitions and Macros
 * @{
 */

/** @def RT_C_DECLS_BEGIN
 * Used to start a block of function declarations which are shared
 * between C and C++ program.
 */

/** @def RT_C_DECLS_END
 * Used to end a block of function declarations which are shared
 * between C and C++ program.
 */

#if defined(__cplusplus)
# define RT_C_DECLS_BEGIN extern "C" {
# define RT_C_DECLS_END   }
#else
# define RT_C_DECLS_BEGIN
# define RT_C_DECLS_END
#endif


/*
 * Shut up DOXYGEN warnings and guide it properly thru the code.
 */
#ifdef DOXYGEN_RUNNING
# define __AMD64__
# define __X86__
# define RT_ARCH_AMD64
# define RT_ARCH_X86
# define IN_RING0
# define IN_RING3
# define IN_RC
# define IN_RC
# define IN_RT_RC
# define IN_RT_R0
# define IN_RT_R3
# define IN_RT_STATIC
# define RT_STRICT
# define RT_NO_STRICT
# define RT_LOCK_STRICT
# define RT_LOCK_NO_STRICT
# define RT_LOCK_STRICT_ORDER
# define RT_LOCK_NO_STRICT_ORDER
# define Breakpoint
# define RT_NO_DEPRECATED_MACROS
# define RT_EXCEPTIONS_ENABLED
# define RT_BIG_ENDIAN
# define RT_LITTLE_ENDIAN
# define RT_COMPILER_GROKS_64BIT_BITFIELDS
# define RT_COMPILER_WITH_80BIT_LONG_DOUBLE
# define RT_NO_VISIBILITY_HIDDEN
#endif /* DOXYGEN_RUNNING */

/** @def RT_ARCH_X86
 * Indicates that we're compiling for the X86 architecture.
 */

/** @def RT_ARCH_AMD64
 * Indicates that we're compiling for the AMD64 architecture.
 */

/** @def RT_ARCH_SPARC
 * Indicates that we're compiling for the SPARC V8 architecture (32-bit).
 */

/** @def RT_ARCH_SPARC64
 * Indicates that we're compiling for the SPARC V9 architecture (64-bit).
 */
#if !defined(RT_ARCH_X86) \
 && !defined(RT_ARCH_AMD64) \
 && !defined(RT_ARCH_SPARC) \
 && !defined(RT_ARCH_SPARC64) \
 && !defined(RT_ARCH_ARM)
# if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(__AMD64__)
#  define RT_ARCH_AMD64
# elif defined(__i386__) || defined(_M_IX86) || defined(__X86__)
#  define RT_ARCH_X86
# elif defined(__sparcv9)
#  define RT_ARCH_SPARC64
# elif defined(__sparc__)
#  define RT_ARCH_SPARC
# elif defined(__arm__) || defined(__arm32__)
#  define RT_ARCH_ARM
# else /* PORTME: append test for new archs. */
#  error "Check what predefined macros your compiler uses to indicate architecture."
# endif
/* PORTME: append new archs checks. */
#elif defined(RT_ARCH_X86) && defined(RT_ARCH_AMD64)
# error "Both RT_ARCH_X86 and RT_ARCH_AMD64 cannot be defined at the same time!"
#elif defined(RT_ARCH_X86) && defined(RT_ARCH_SPARC)
# error "Both RT_ARCH_X86 and RT_ARCH_SPARC cannot be defined at the same time!"
#elif defined(RT_ARCH_X86) && defined(RT_ARCH_SPARC64)
# error "Both RT_ARCH_X86 and RT_ARCH_SPARC64 cannot be defined at the same time!"
#elif defined(RT_ARCH_AMD64) && defined(RT_ARCH_SPARC)
# error "Both RT_ARCH_AMD64 and RT_ARCH_SPARC cannot be defined at the same time!"
#elif defined(RT_ARCH_AMD64) && defined(RT_ARCH_SPARC64)
# error "Both RT_ARCH_AMD64 and RT_ARCH_SPARC64 cannot be defined at the same time!"
#elif defined(RT_ARCH_SPARC) && defined(RT_ARCH_SPARC64)
# error "Both RT_ARCH_SPARC and RT_ARCH_SPARC64 cannot be defined at the same time!"
#elif defined(RT_ARCH_ARM) && defined(RT_ARCH_AMD64)
# error "Both RT_ARCH_ARM and RT_ARCH_AMD64 cannot be defined at the same time!"
#elif defined(RT_ARCH_ARM) && defined(RT_ARCH_X86)
# error "Both RT_ARCH_ARM and RT_ARCH_X86 cannot be defined at the same time!"
#elif defined(RT_ARCH_ARM) && defined(RT_ARCH_SPARC64)
# error "Both RT_ARCH_ARM and RT_ARCH_SPARC64 cannot be defined at the same time!"
#elif defined(RT_ARCH_ARM) && defined(RT_ARCH_SPARC)
# error "Both RT_ARCH_ARM and RT_ARCH_SPARC cannot be defined at the same time!"
#endif


/** @def __X86__
 * Indicates that we're compiling for the X86 architecture.
 * @deprecated
 */

/** @def __AMD64__
 * Indicates that we're compiling for the AMD64 architecture.
 * @deprecated
 */
#if !defined(__X86__) && !defined(__AMD64__) && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))
# if defined(RT_ARCH_AMD64)
#  define __AMD64__
# elif defined(RT_ARCH_X86)
#  define __X86__
# else
#  error "Check what predefined macros your compiler uses to indicate architecture."
# endif
#elif defined(__X86__) && defined(__AMD64__)
# error "Both __X86__ and __AMD64__ cannot be defined at the same time!"
#elif defined(__X86__) && !defined(RT_ARCH_X86)
# error "__X86__ without RT_ARCH_X86!"
#elif defined(__AMD64__) && !defined(RT_ARCH_AMD64)
# error "__AMD64__ without RT_ARCH_AMD64!"
#endif

/** @def RT_BIG_ENDIAN
 * Defined if the architecture is big endian.  */
/** @def RT_LITTLE_ENDIAN
 * Defined if the architecture is little endian.  */
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86) || defined(RT_ARCH_ARM)
# define RT_LITTLE_ENDIAN
#elif defined(RT_ARCH_SPARC) || defined(RT_ARCH_SPARC64)
# define RT_BIG_ENDIAN
#else
# error "PORTME: architecture endianess"
#endif
#if defined(RT_BIG_ENDIAN) && defined(RT_LITTLE_ENDIAN)
# error "Both RT_BIG_ENDIAN and RT_LITTLE_ENDIAN are defined"
#endif


/** @def IN_RING0
 * Used to indicate that we're compiling code which is running
 * in Ring-0 Host Context.
 */

/** @def IN_RING3
 * Used to indicate that we're compiling code which is running
 * in Ring-3 Host Context.
 */

/** @def IN_RC
 * Used to indicate that we're compiling code which is running
 * in the Raw-mode Context (implies R0).
 */
#if !defined(IN_RING3) && !defined(IN_RING0) && !defined(IN_RC) && !defined(IN_RC)
# error "You must define which context the compiled code should run in; IN_RING3, IN_RING0 or IN_RC"
#endif
#if (defined(IN_RING3) && (defined(IN_RING0) || defined(IN_RC)) ) \
 || (defined(IN_RING0) && (defined(IN_RING3) || defined(IN_RC)) ) \
 || (defined(IN_RC)    && (defined(IN_RING3) || defined(IN_RING0)) )
# error "Only one of the IN_RING3, IN_RING0, IN_RC defines should be defined."
#endif


/** @def ARCH_BITS
 * Defines the bit count of the current context.
 */
#if !defined(ARCH_BITS) || defined(DOXYGEN_RUNNING)
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_SPARC64)
#  define ARCH_BITS 64
# else
#  define ARCH_BITS 32
# endif
#endif

/** @def HC_ARCH_BITS
 * Defines the host architecture bit count.
 */
#if !defined(HC_ARCH_BITS) || defined(DOXYGEN_RUNNING)
# ifndef IN_RC
#  define HC_ARCH_BITS ARCH_BITS
# else
#  define HC_ARCH_BITS 32
# endif
#endif

/** @def GC_ARCH_BITS
 * Defines the guest architecture bit count.
 */
#if !defined(GC_ARCH_BITS) && !defined(DOXYGEN_RUNNING)
# ifdef VBOX_WITH_64_BITS_GUESTS
#  define GC_ARCH_BITS  64
# else
#  define GC_ARCH_BITS  32
# endif
#endif

/** @def R3_ARCH_BITS
 * Defines the host ring-3 architecture bit count.
 */
#if !defined(R3_ARCH_BITS) || defined(DOXYGEN_RUNNING)
# ifdef IN_RING3
#  define R3_ARCH_BITS ARCH_BITS
# else
#  define R3_ARCH_BITS HC_ARCH_BITS
# endif
#endif

/** @def R0_ARCH_BITS
 * Defines the host ring-0 architecture bit count.
 */
#if !defined(R0_ARCH_BITS) || defined(DOXYGEN_RUNNING)
# ifdef IN_RING0
#  define R0_ARCH_BITS ARCH_BITS
# else
#  define R0_ARCH_BITS HC_ARCH_BITS
# endif
#endif

/** @def GC_ARCH_BITS
 * Defines the guest architecture bit count.
 */
#if !defined(GC_ARCH_BITS) || defined(DOXYGEN_RUNNING)
# ifdef IN_RC
#  define GC_ARCH_BITS ARCH_BITS
# else
#  define GC_ARCH_BITS 32
# endif
#endif



/** @name RT_OPSYS_XXX - Operative System Identifiers.
 * These are the value that the RT_OPSYS \#define can take. @{
 */
/** Unknown OS. */
#define RT_OPSYS_UNKNOWN    0
/** OS Agnostic. */
#define RT_OPSYS_AGNOSTIC   1
/** Darwin - aka Mac OS X. */
#define RT_OPSYS_DARWIN     2
/** DragonFly BSD. */
#define RT_OPSYS_DRAGONFLY  3
/** DOS. */
#define RT_OPSYS_DOS        4
/** FreeBSD. */
#define RT_OPSYS_FREEBSD    5
/** Haiku. */
#define RT_OPSYS_HAIKU      6
/** Linux. */
#define RT_OPSYS_LINUX      7
/** L4. */
#define RT_OPSYS_L4         8
/** Minix. */
#define RT_OPSYS_MINIX      9
/** NetBSD. */
#define RT_OPSYS_NETBSD     11
/** Netware. */
#define RT_OPSYS_NETWARE    12
/** NT (native). */
#define RT_OPSYS_NT         13
/** OpenBSD. */
#define RT_OPSYS_OPENBSD    14
/** OS/2. */
#define RT_OPSYS_OS2        15
/** Plan 9. */
#define RT_OPSYS_PLAN9      16
/** QNX. */
#define RT_OPSYS_QNX        17
/** Solaris. */
#define RT_OPSYS_SOLARIS    18
/** UEFI. */
#define RT_OPSYS_UEFI       19
/** Windows. */
#define RT_OPSYS_WINDOWS    20
/** The max RT_OPSYS_XXX value (exclusive). */
#define RT_OPSYS_MAX        21
/** @} */

/** @def RT_OPSYS
 * Indicates which OS we're targeting. It's a \#define with is
 * assigned one of the RT_OPSYS_XXX defines above.
 *
 * So to test if we're on FreeBSD do the following:
 * @code
 *  #if RT_OPSYS == RT_OPSYS_FREEBSD
 *  some_funky_freebsd_specific_stuff();
 *  #endif
 * @endcode
 */

/*
 * Set RT_OPSYS_XXX according to RT_OS_XXX.
 *
 * Search:  #define RT_OPSYS_([A-Z0-9]+) .*
 * Replace: # elif defined(RT_OS_\1)\n#  define RT_OPSYS RT_OPSYS_\1
 */
#ifndef RT_OPSYS
# if defined(RT_OS_UNKNOWN)
#  define RT_OPSYS RT_OPSYS_UNKNOWN
# elif defined(RT_OS_AGNOSTIC)
#  define RT_OPSYS RT_OPSYS_AGNOSTIC
# elif defined(RT_OS_DARWIN)
#  define RT_OPSYS RT_OPSYS_DARWIN
# elif defined(RT_OS_DRAGONFLY)
#  define RT_OPSYS RT_OPSYS_DRAGONFLY
# elif defined(RT_OS_DOS)
#  define RT_OPSYS RT_OPSYS_DOS
# elif defined(RT_OS_FREEBSD)
#  define RT_OPSYS RT_OPSYS_FREEBSD
# elif defined(RT_OS_HAIKU)
#  define RT_OPSYS RT_OPSYS_HAIKU
# elif defined(RT_OS_LINUX)
#  define RT_OPSYS RT_OPSYS_LINUX
# elif defined(RT_OS_L4)
#  define RT_OPSYS RT_OPSYS_L4
# elif defined(RT_OS_MINIX)
#  define RT_OPSYS RT_OPSYS_MINIX
# elif defined(RT_OS_NETBSD)
#  define RT_OPSYS RT_OPSYS_NETBSD
# elif defined(RT_OS_NETWARE)
#  define RT_OPSYS RT_OPSYS_NETWARE
# elif defined(RT_OS_NT)
#  define RT_OPSYS RT_OPSYS_NT
# elif defined(RT_OS_OPENBSD)
#  define RT_OPSYS RT_OPSYS_OPENBSD
# elif defined(RT_OS_OS2)
#  define RT_OPSYS RT_OPSYS_OS2
# elif defined(RT_OS_PLAN9)
#  define RT_OPSYS RT_OPSYS_PLAN9
# elif defined(RT_OS_QNX)
#  define RT_OPSYS RT_OPSYS_QNX
# elif defined(RT_OS_SOLARIS)
#  define RT_OPSYS RT_OPSYS_SOLARIS
# elif defined(RT_OS_UEFI)
#  define RT_OPSYS RT_OPSYS_UEFI
# elif defined(RT_OS_WINDOWS)
#  define RT_OPSYS RT_OPSYS_WINDOWS
# endif
#endif

/*
 * Guess RT_OPSYS based on compiler predefined macros.
 */
#ifndef RT_OPSYS
# if defined(__APPLE__)
#  define RT_OPSYS      RT_OPSYS_DARWIN
# elif defined(__DragonFly__)
#  define RT_OPSYS      RT_OPSYS_DRAGONFLY
# elif defined(__FreeBSD__) /*??*/
#  define RT_OPSYS      RT_OPSYS_FREEBSD
# elif defined(__gnu_linux__)
#  define RT_OPSYS      RT_OPSYS_LINUX
# elif defined(__NetBSD__) /*??*/
#  define RT_OPSYS      RT_OPSYS_NETBSD
# elif defined(__OpenBSD__) /*??*/
#  define RT_OPSYS      RT_OPSYS_OPENBSD
# elif defined(__OS2__)
#  define RT_OPSYS      RT_OPSYS_OS2
# elif defined(__sun__) || defined(__SunOS__) || defined(__sun) || defined(__SunOS)
#  define RT_OPSYS      RT_OPSYS_SOLARIS
# elif defined(_WIN32) || defined(_WIN64)
#  define RT_OPSYS      RT_OPSYS_WINDOWS
# else
#  error "Port Me"
# endif
#endif

#if RT_OPSYS < RT_OPSYS_UNKNOWN || RT_OPSYS >= RT_OPSYS_MAX
# error "Invalid RT_OPSYS value."
#endif

/*
 * Do some consistency checks.
 *
 * Search:  #define RT_OPSYS_([A-Z0-9]+) .*
 * Replace: #if defined(RT_OS_\1) && RT_OPSYS != RT_OPSYS_\1\n# error RT_OPSYS vs RT_OS_\1\n#endif
 */
#if defined(RT_OS_UNKNOWN) && RT_OPSYS != RT_OPSYS_UNKNOWN
# error RT_OPSYS vs RT_OS_UNKNOWN
#endif
#if defined(RT_OS_AGNOSTIC) && RT_OPSYS != RT_OPSYS_AGNOSTIC
# error RT_OPSYS vs RT_OS_AGNOSTIC
#endif
#if defined(RT_OS_DARWIN) && RT_OPSYS != RT_OPSYS_DARWIN
# error RT_OPSYS vs RT_OS_DARWIN
#endif
#if defined(RT_OS_DRAGONFLY) && RT_OPSYS != RT_OPSYS_DRAGONFLY
# error RT_OPSYS vs RT_OS_DRAGONFLY
#endif
#if defined(RT_OS_DOS) && RT_OPSYS != RT_OPSYS_DOS
# error RT_OPSYS vs RT_OS_DOS
#endif
#if defined(RT_OS_FREEBSD) && RT_OPSYS != RT_OPSYS_FREEBSD
# error RT_OPSYS vs RT_OS_FREEBSD
#endif
#if defined(RT_OS_HAIKU) && RT_OPSYS != RT_OPSYS_HAIKU
# error RT_OPSYS vs RT_OS_HAIKU
#endif
#if defined(RT_OS_LINUX) && RT_OPSYS != RT_OPSYS_LINUX
# error RT_OPSYS vs RT_OS_LINUX
#endif
#if defined(RT_OS_L4) && RT_OPSYS != RT_OPSYS_L4
# error RT_OPSYS vs RT_OS_L4
#endif
#if defined(RT_OS_MINIX) && RT_OPSYS != RT_OPSYS_MINIX
# error RT_OPSYS vs RT_OS_MINIX
#endif
#if defined(RT_OS_NETBSD) && RT_OPSYS != RT_OPSYS_NETBSD
# error RT_OPSYS vs RT_OS_NETBSD
#endif
#if defined(RT_OS_NETWARE) && RT_OPSYS != RT_OPSYS_NETWARE
# error RT_OPSYS vs RT_OS_NETWARE
#endif
#if defined(RT_OS_NT) && RT_OPSYS != RT_OPSYS_NT
# error RT_OPSYS vs RT_OS_NT
#endif
#if defined(RT_OS_OPENBSD) && RT_OPSYS != RT_OPSYS_OPENBSD
# error RT_OPSYS vs RT_OS_OPENBSD
#endif
#if defined(RT_OS_OS2) && RT_OPSYS != RT_OPSYS_OS2
# error RT_OPSYS vs RT_OS_OS2
#endif
#if defined(RT_OS_PLAN9) && RT_OPSYS != RT_OPSYS_PLAN9
# error RT_OPSYS vs RT_OS_PLAN9
#endif
#if defined(RT_OS_QNX) && RT_OPSYS != RT_OPSYS_QNX
# error RT_OPSYS vs RT_OS_QNX
#endif
#if defined(RT_OS_SOLARIS) && RT_OPSYS != RT_OPSYS_SOLARIS
# error RT_OPSYS vs RT_OS_SOLARIS
#endif
#if defined(RT_OS_UEFI) && RT_OPSYS != RT_OPSYS_UEFI
# error RT_OPSYS vs RT_OS_UEFI
#endif
#if defined(RT_OS_WINDOWS) && RT_OPSYS != RT_OPSYS_WINDOWS
# error RT_OPSYS vs RT_OS_WINDOWS
#endif

/*
 * Make sure the RT_OS_XXX macro is defined.
 *
 * Search:  #define RT_OPSYS_([A-Z0-9]+) .*
 * Replace: #elif RT_OPSYS == RT_OPSYS_\1\n# ifndef RT_OS_\1\n#  define RT_OS_\1\n# endif
 */
#if RT_OPSYS == RT_OPSYS_UNKNOWN
# ifndef RT_OS_UNKNOWN
#  define RT_OS_UNKNOWN
# endif
#elif RT_OPSYS == RT_OPSYS_AGNOSTIC
# ifndef RT_OS_AGNOSTIC
#  define RT_OS_AGNOSTIC
# endif
#elif RT_OPSYS == RT_OPSYS_DARWIN
# ifndef RT_OS_DARWIN
#  define RT_OS_DARWIN
# endif
#elif RT_OPSYS == RT_OPSYS_DRAGONFLY
# ifndef RT_OS_DRAGONFLY
#  define RT_OS_DRAGONFLY
# endif
#elif RT_OPSYS == RT_OPSYS_DOS
# ifndef RT_OS_DOS
#  define RT_OS_DOS
# endif
#elif RT_OPSYS == RT_OPSYS_FREEBSD
# ifndef RT_OS_FREEBSD
#  define RT_OS_FREEBSD
# endif
#elif RT_OPSYS == RT_OPSYS_HAIKU
# ifndef RT_OS_HAIKU
#  define RT_OS_HAIKU
# endif
#elif RT_OPSYS == RT_OPSYS_LINUX
# ifndef RT_OS_LINUX
#  define RT_OS_LINUX
# endif
#elif RT_OPSYS == RT_OPSYS_L4
# ifndef RT_OS_L4
#  define RT_OS_L4
# endif
#elif RT_OPSYS == RT_OPSYS_MINIX
# ifndef RT_OS_MINIX
#  define RT_OS_MINIX
# endif
#elif RT_OPSYS == RT_OPSYS_NETBSD
# ifndef RT_OS_NETBSD
#  define RT_OS_NETBSD
# endif
#elif RT_OPSYS == RT_OPSYS_NETWARE
# ifndef RT_OS_NETWARE
#  define RT_OS_NETWARE
# endif
#elif RT_OPSYS == RT_OPSYS_NT
# ifndef RT_OS_NT
#  define RT_OS_NT
# endif
#elif RT_OPSYS == RT_OPSYS_OPENBSD
# ifndef RT_OS_OPENBSD
#  define RT_OS_OPENBSD
# endif
#elif RT_OPSYS == RT_OPSYS_OS2
# ifndef RT_OS_OS2
#  define RT_OS_OS2
# endif
#elif RT_OPSYS == RT_OPSYS_PLAN9
# ifndef RT_OS_PLAN9
#  define RT_OS_PLAN9
# endif
#elif RT_OPSYS == RT_OPSYS_QNX
# ifndef RT_OS_QNX
#  define RT_OS_QNX
# endif
#elif RT_OPSYS == RT_OPSYS_SOLARIS
# ifndef RT_OS_SOLARIS
#  define RT_OS_SOLARIS
# endif
#elif RT_OPSYS == RT_OPSYS_UEFI
# ifndef RT_OS_UEFI
#  define RT_OS_UEFI
# endif
#elif RT_OPSYS == RT_OPSYS_WINDOWS
# ifndef RT_OS_WINDOWS
#  define RT_OS_WINDOWS
# endif
#else
# error "Bad RT_OPSYS value."
#endif


/**
 * Checks whether the given OpSys uses DOS-style paths or not.
 *
 * By DOS-style paths we include drive lettering and UNC paths.
 *
 * @returns true / false
 * @param   a_OpSys     The RT_OPSYS_XXX value to check, will be reference
 *                      multiple times.
 */
#define RT_OPSYS_USES_DOS_PATHS(a_OpSys) \
    (   (a_OpSys) == RT_OPSYS_WINDOWS \
     || (a_OpSys) == RT_OPSYS_OS2 \
     || (a_OpSys) == RT_OPSYS_DOS )



/** @def CTXTYPE
 * Declare a type differently in GC, R3 and R0.
 *
 * @param   GCType  The GC type.
 * @param   R3Type  The R3 type.
 * @param   R0Type  The R0 type.
 * @remark  For pointers used only in one context use RCPTRTYPE(), R3R0PTRTYPE(), R3PTRTYPE() or R0PTRTYPE().
 */
#ifdef IN_RC
# define CTXTYPE(GCType, R3Type, R0Type)  GCType
#elif defined(IN_RING3)
# define CTXTYPE(GCType, R3Type, R0Type)  R3Type
#else
# define CTXTYPE(GCType, R3Type, R0Type)  R0Type
#endif

/** @def RCPTRTYPE
 * Declare a pointer which is used in the raw mode context but appears in structure(s) used by
 * both HC and RC. The main purpose is to make sure structures have the same
 * size when built for different architectures.
 *
 * @param   RCType  The RC type.
 */
#define RCPTRTYPE(RCType)       CTXTYPE(RCType, RTRCPTR, RTRCPTR)

/** @def R3R0PTRTYPE
 * Declare a pointer which is used in HC, is explicitly valid in ring 3 and 0,
 * but appears in structure(s) used by both HC and GC. The main purpose is to
 * make sure structures have the same size when built for different architectures.
 *
 * @param   R3R0Type  The R3R0 type.
 * @remarks This used to be called HCPTRTYPE.
 */
#define R3R0PTRTYPE(R3R0Type)   CTXTYPE(RTHCPTR, R3R0Type, R3R0Type)

/** @def R3PTRTYPE
 * Declare a pointer which is used in R3 but appears in structure(s) used by
 * both HC and GC. The main purpose is to make sure structures have the same
 * size when built for different architectures.
 *
 * @param   R3Type  The R3 type.
 */
#define R3PTRTYPE(R3Type)       CTXTYPE(RTHCUINTPTR, R3Type, RTHCUINTPTR)

/** @def R0PTRTYPE
 * Declare a pointer which is used in R0 but appears in structure(s) used by
 * both HC and GC. The main purpose is to make sure structures have the same
 * size when built for different architectures.
 *
 * @param   R0Type  The R0 type.
 */
#define R0PTRTYPE(R0Type)       CTXTYPE(RTHCUINTPTR, RTHCUINTPTR, R0Type)

/** @def CTXSUFF
 * Adds the suffix of the current context to the passed in
 * identifier name. The suffix is HC or GC.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   var     Identifier name.
 * @deprecated Use CTX_SUFF. Do NOT use this for new code.
 */
/** @def OTHERCTXSUFF
 * Adds the suffix of the other context to the passed in
 * identifier name. The suffix is HC or GC.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   var     Identifier name.
 * @deprecated Use CTX_SUFF. Do NOT use this for new code.
 */
#ifdef IN_RC
# define CTXSUFF(var)       var##GC
# define OTHERCTXSUFF(var)  var##HC
#else
# define CTXSUFF(var)       var##HC
# define OTHERCTXSUFF(var)  var##GC
#endif

/** @def CTXALLSUFF
 * Adds the suffix of the current context to the passed in
 * identifier name. The suffix is R3, R0 or GC.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   var     Identifier name.
 * @deprecated Use CTX_SUFF. Do NOT use this for new code.
 */
#ifdef IN_RC
# define CTXALLSUFF(var)    var##GC
#elif defined(IN_RING0)
# define CTXALLSUFF(var)    var##R0
#else
# define CTXALLSUFF(var)    var##R3
#endif

/** @def CTX_SUFF
 * Adds the suffix of the current context to the passed in
 * identifier name. The suffix is R3, R0 or RC.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   var     Identifier name.
 *
 * @remark  This will replace CTXALLSUFF and CTXSUFF before long.
 */
#ifdef IN_RC
# define CTX_SUFF(var)      var##RC
#elif defined(IN_RING0)
# define CTX_SUFF(var)      var##R0
#else
# define CTX_SUFF(var)      var##R3
#endif

/** @def CTX_SUFF_Z
 * Adds the suffix of the current context to the passed in
 * identifier name, combining RC and R0 into RZ.
 * The suffix thus is R3 or RZ.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   var     Identifier name.
 *
 * @remark  This will replace CTXALLSUFF and CTXSUFF before long.
 */
#ifdef IN_RING3
# define CTX_SUFF_Z(var)    var##R3
#else
# define CTX_SUFF_Z(var)    var##RZ
#endif


/** @def CTXMID
 * Adds the current context as a middle name of an identifier name
 * The middle name is HC or GC.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   first   First name.
 * @param   last    Surname.
 */
/** @def OTHERCTXMID
 * Adds the other context as a middle name of an identifier name
 * The middle name is HC or GC.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   first   First name.
 * @param   last    Surname.
 * @deprecated use CTX_MID or CTX_MID_Z
 */
#ifdef IN_RC
# define CTXMID(first, last)        first##GC##last
# define OTHERCTXMID(first, last)   first##HC##last
#else
# define CTXMID(first, last)        first##HC##last
# define OTHERCTXMID(first, last)   first##GC##last
#endif

/** @def CTXALLMID
 * Adds the current context as a middle name of an identifier name.
 * The middle name is R3, R0 or GC.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   first   First name.
 * @param   last    Surname.
 * @deprecated use CTX_MID or CTX_MID_Z
 */
#ifdef IN_RC
# define CTXALLMID(first, last)     first##GC##last
#elif defined(IN_RING0)
# define CTXALLMID(first, last)     first##R0##last
#else
# define CTXALLMID(first, last)     first##R3##last
#endif

/** @def CTX_MID
 * Adds the current context as a middle name of an identifier name.
 * The middle name is R3, R0 or RC.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   first   First name.
 * @param   last    Surname.
 */
#ifdef IN_RC
# define CTX_MID(first, last)       first##RC##last
#elif defined(IN_RING0)
# define CTX_MID(first, last)       first##R0##last
#else
# define CTX_MID(first, last)       first##R3##last
#endif

/** @def CTX_MID_Z
 * Adds the current context as a middle name of an identifier name, combining RC
 * and R0 into RZ.
 * The middle name thus is either R3 or RZ.
 *
 * This is macro should only be used in shared code to avoid a forest of ifdefs.
 * @param   first   First name.
 * @param   last    Surname.
 */
#ifdef IN_RING3
# define CTX_MID_Z(first, last)     first##R3##last
#else
# define CTX_MID_Z(first, last)     first##RZ##last
#endif


/** @def R3STRING
 * A macro which in GC and R0 will return a dummy string while in R3 it will return
 * the parameter.
 *
 * This is typically used to wrap description strings in structures shared
 * between R3, R0 and/or GC. The intention is to avoid the \#ifdef IN_RING3 mess.
 *
 * @param   pR3String   The R3 string. Only referenced in R3.
 * @see R0STRING and GCSTRING
 */
#ifdef IN_RING3
# define R3STRING(pR3String)    (pR3String)
#else
# define R3STRING(pR3String)    ("<R3_STRING>")
#endif

/** @def R0STRING
 * A macro which in GC and R3 will return a dummy string while in R0 it will return
 * the parameter.
 *
 * This is typically used to wrap description strings in structures shared
 * between R3, R0 and/or GC. The intention is to avoid the \#ifdef IN_RING0 mess.
 *
 * @param   pR0String   The R0 string. Only referenced in R0.
 * @see R3STRING and GCSTRING
 */
#ifdef IN_RING0
# define R0STRING(pR0String)    (pR0String)
#else
# define R0STRING(pR0String)    ("<R0_STRING>")
#endif

/** @def RCSTRING
 * A macro which in R3 and R0 will return a dummy string while in RC it will return
 * the parameter.
 *
 * This is typically used to wrap description strings in structures shared
 * between R3, R0 and/or RC. The intention is to avoid the \#ifdef IN_RC mess.
 *
 * @param   pRCString   The RC string. Only referenced in RC.
 * @see R3STRING, R0STRING
 */
#ifdef IN_RC
# define RCSTRING(pRCString)    (pRCString)
#else
# define RCSTRING(pRCString)    ("<RC_STRING>")
#endif


/** @def RT_NOTHING
 * A macro that expands to nothing.
 * This is primarily intended as a dummy argument for macros to avoid the
 * undefined behavior passing empty arguments to an macro (ISO C90 and C++98,
 * gcc v4.4 warns about it).
 */
#define RT_NOTHING

/** @def RT_GCC_EXTENSION
 * Macro for shutting up GCC warnings about using language extensions. */
#ifdef __GNUC__
# define RT_GCC_EXTENSION       __extension__
#else
# define RT_GCC_EXTENSION
#endif

/** @def RT_COMPILER_GROKS_64BIT_BITFIELDS
 * Macro that is defined if the compiler understands 64-bit bitfields. */
#if !defined(RT_OS_OS2) || (!defined(__IBMC__) && !defined(__IBMCPP__))
# define RT_COMPILER_GROKS_64BIT_BITFIELDS
#endif

/** @def RT_COMPILER_WITH_80BIT_LONG_DOUBLE
 * Macro that is defined if the compiler implements long double as the
 * IEEE extended precision floating. */
#if (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)) && !defined(RT_OS_WINDOWS)
# define RT_COMPILER_WITH_80BIT_LONG_DOUBLE
#endif


/** @def RT_EXCEPTIONS_ENABLED
 * Defined when C++ exceptions are enabled.
 */
#if !defined(RT_EXCEPTIONS_ENABLED) \
 &&  defined(__cplusplus) \
 && (   (defined(_MSC_VER) && defined(_CPPUNWIND)) \
     || (defined(__GNUC__) && defined(__EXCEPTIONS)))
# define RT_EXCEPTIONS_ENABLED
#endif

/** @def RT_NO_THROW
 * How to express that a function doesn't throw C++ exceptions
 * and the compiler can thus save itself the bother of trying
 * to catch any of them. Put this between the closing parenthesis
 * and the semicolon in function prototypes (and implementation if C++).
 */
#ifdef RT_EXCEPTIONS_ENABLED
# define RT_NO_THROW            throw()
#else
# define RT_NO_THROW
#endif

/** @def RT_THROW
 * How to express that a method or function throws a type of exceptions. Some
 * compilers does not want this kind of information and will warning about it.
 *
 * @param   type    The type exception.
 *
 * @remarks If the actual throwing is done from the header, enclose it by
 *          \#ifdef RT_EXCEPTIONS_ENABLED ... \#else ... \#endif so the header
 *          compiles cleanly without exceptions enabled.
 *
 *          Do NOT use this for the actual throwing of exceptions!
 */
#ifdef RT_EXCEPTIONS_ENABLED
# ifdef _MSC_VER
#  if _MSC_VER >= 1310
#   define RT_THROW(type)
#  else
#   define RT_THROW(type)       throw(type)
#  endif
# else
#  define RT_THROW(type)        throw(type)
# endif
#else
# define RT_THROW(type)
#endif

/** @def RT_GCC_SUPPORTS_VISIBILITY_HIDDEN
 * Indicates that the "hidden" visibility attribute can be used (GCC) */
#if defined(__GNUC__)
# if __GNUC__ >= 4 && !defined(RT_OS_OS2) && !defined(RT_OS_WINDOWS)
#  define RT_GCC_SUPPORTS_VISIBILITY_HIDDEN
# endif
#endif

/** @def RTCALL
 * The standard calling convention for the Runtime interfaces.
 */
#ifdef _MSC_VER
# define RTCALL     __cdecl
#elif defined(RT_OS_OS2)
# define RTCALL     __cdecl
#elif defined(__GNUC__) && defined(IN_RING0) && defined(RT_ARCH_X86) /** @todo consider dropping IN_RING0 here. */
# define RTCALL     __attribute__((cdecl,regparm(0))) /* regparm(0) deals with -mregparm=x use in the linux kernel. */
#else
# define RTCALL
#endif

/** @def DECLEXPORT
 * How to declare an exported function.
 * @param   type    The return type of the function declaration.
 */
#if defined(_MSC_VER) || defined(RT_OS_OS2)
# define DECLEXPORT(type)       __declspec(dllexport) type
#elif defined(RT_USE_VISIBILITY_DEFAULT)
# define DECLEXPORT(type)      __attribute__((visibility("default"))) type
#else
# define DECLEXPORT(type)      type
#endif

/** @def DECLIMPORT
 * How to declare an imported function.
 * @param   type    The return type of the function declaration.
 */
#if defined(_MSC_VER) || (defined(RT_OS_OS2) && !defined(__IBMC__) && !defined(__IBMCPP__))
# define DECLIMPORT(type)       __declspec(dllimport) type
#else
# define DECLIMPORT(type)       type
#endif

/** @def DECLHIDDEN
 * How to declare a non-exported function or variable.
 * @param   type    The return type of the function or the data type of the variable.
 */
#if !defined(RT_GCC_SUPPORTS_VISIBILITY_HIDDEN) || defined(RT_NO_VISIBILITY_HIDDEN)
# define DECLHIDDEN(type)       type
#else
# define DECLHIDDEN(type)       __attribute__((visibility("hidden"))) type
#endif

/** @def DECL_HIDDEN_CONST
 * Workaround for g++ warnings when applying the hidden attribute to a const
 * definition.  Use DECLHIDDEN for the declaration.
 * @param   a_Type      The return type of the function or the data type of
 *                      the variable.
 */
#if defined(__cplusplus) && defined(__GNUC__)
# define DECL_HIDDEN_CONST(a_Type)   a_Type
#else
# define DECL_HIDDEN_CONST(a_Type)   DECLHIDDEN(a_Type)
#endif

/** @def DECL_INVALID
 * How to declare a function not available for linking in the current context.
 * The purpose is to create compile or like time errors when used.  This isn't
 * possible on all platforms.
 * @param   type    The return type of the function.
 */
#if defined(_MSC_VER)
# define DECL_INVALID(type)     __declspec(dllimport) type __stdcall
#elif defined(__GNUC__) && defined(__cplusplus)
# define DECL_INVALID(type)     extern "C++" type
#else
# define DECL_INVALID(type)     type
#endif

/** @def DECLASM
 * How to declare an internal assembly function.
 * @param   type    The return type of the function declaration.
 */
#ifdef __cplusplus
# if defined(_MSC_VER) || defined(RT_OS_OS2)
#  define DECLASM(type)          extern "C" type __cdecl
# elif defined(__GNUC__) && defined(RT_ARCH_X86)
#  define DECLASM(type)          extern "C" type __attribute__((cdecl,regparm(0)))
# else
#  define DECLASM(type)          extern "C" type
# endif
#else
# if defined(_MSC_VER) || defined(RT_OS_OS2)
#  define DECLASM(type)          type __cdecl
# elif defined(__GNUC__) && defined(RT_ARCH_X86)
#  define DECLASM(type)          type __attribute__((cdecl,regparm(0)))
# else
#  define DECLASM(type)          type
# endif
#endif

/** @def DECLASMTYPE
 * How to declare an internal assembly function type.
 * @param   type    The return type of the function.
 */
# if defined(_MSC_VER) || defined(RT_OS_OS2)
# define DECLASMTYPE(type)      type __cdecl
#else
# define DECLASMTYPE(type)      type
#endif

/** @def DECLNORETURN
 * How to declare a function which does not return.
 * @note: This macro can be combined with other macros, for example
 * @code
 *   EMR3DECL(DECLNORETURN(void)) foo(void);
 * @endcode
 */
#ifdef _MSC_VER
# define DECLNORETURN(type)     __declspec(noreturn) type
#elif defined(__GNUC__)
# define DECLNORETURN(type)     __attribute__((noreturn)) type
#else
# define DECLNORETURN(type)     type
#endif

/** @def DECLWEAK
 * How to declare a variable which is not necessarily resolved at
 * runtime.
 * @note: This macro can be combined with other macros, for example
 * @code
 *   EMR3DECL(DECLWEAK(int)) foo;
 * @endcode
 */
#if defined(__GNUC__)
# define DECLWEAK(type)         type __attribute__((weak))
#else
# define DECLWEAK(type)         type
#endif

/** @def DECLCALLBACK
 * How to declare an call back function type.
 * @param   type    The return type of the function declaration.
 */
#define DECLCALLBACK(type)      type RTCALL

/** @def DECLCALLBACKPTR
 * How to declare an call back function pointer.
 * @param   type    The return type of the function declaration.
 * @param   name    The name of the variable member.
 */
#if defined(__IBMC__) || defined(__IBMCPP__)
# define DECLCALLBACKPTR(type, name)    type (* RTCALL name)
#else
# define DECLCALLBACKPTR(type, name)    type (RTCALL * name)
#endif

/** @def DECLCALLBACKMEMBER
 * How to declare an call back function pointer member.
 * @param   type    The return type of the function declaration.
 * @param   name    The name of the struct/union/class member.
 */
#if defined(__IBMC__) || defined(__IBMCPP__)
# define DECLCALLBACKMEMBER(type, name) type (* RTCALL name)
#else
# define DECLCALLBACKMEMBER(type, name) type (RTCALL * name)
#endif

/** @def DECLR3CALLBACKMEMBER
 * How to declare an call back function pointer member - R3 Ptr.
 * @param   type    The return type of the function declaration.
 * @param   name    The name of the struct/union/class member.
 * @param   args    The argument list enclosed in parentheses.
 */
#ifdef IN_RING3
# define DECLR3CALLBACKMEMBER(type, name, args)  DECLCALLBACKMEMBER(type, name) args
#else
# define DECLR3CALLBACKMEMBER(type, name, args)  RTR3PTR name
#endif

/** @def DECLRCCALLBACKMEMBER
 * How to declare an call back function pointer member - RC Ptr.
 * @param   type    The return type of the function declaration.
 * @param   name    The name of the struct/union/class member.
 * @param   args    The argument list enclosed in parentheses.
 */
#ifdef IN_RC
# define DECLRCCALLBACKMEMBER(type, name, args)  DECLCALLBACKMEMBER(type, name)  args
#else
# define DECLRCCALLBACKMEMBER(type, name, args)  RTRCPTR name
#endif

/** @def DECLR0CALLBACKMEMBER
 * How to declare an call back function pointer member - R0 Ptr.
 * @param   type    The return type of the function declaration.
 * @param   name    The name of the struct/union/class member.
 * @param   args    The argument list enclosed in parentheses.
 */
#ifdef IN_RING0
# define DECLR0CALLBACKMEMBER(type, name, args)  DECLCALLBACKMEMBER(type, name) args
#else
# define DECLR0CALLBACKMEMBER(type, name, args)  RTR0PTR name
#endif

/** @def DECLINLINE
 * How to declare a function as inline.
 * @param   type    The return type of the function declaration.
 * @remarks Don't use this macro on C++ methods.
 */
#ifdef __GNUC__
# define DECLINLINE(type) static __inline__ type
#elif defined(__cplusplus)
# define DECLINLINE(type) inline type
#elif defined(_MSC_VER)
# define DECLINLINE(type) _inline type
#elif defined(__IBMC__)
# define DECLINLINE(type) _Inline type
#else
# define DECLINLINE(type) inline type
#endif


/** @def DECL_FORCE_INLINE
 * How to declare a function as inline and try convince the compiler to always
 * inline it regardless of optimization switches.
 * @param   type    The return type of the function declaration.
 * @remarks Use sparsely and with care. Don't use this macro on C++ methods.
 */
#ifdef __GNUC__
# define DECL_FORCE_INLINE(type)    __attribute__((__always_inline__)) DECLINLINE(type)
#elif defined(_MSC_VER)
# define DECL_FORCE_INLINE(type)    __forceinline type
#else
# define DECL_FORCE_INLINE(type)    DECLINLINE(type)
#endif


/** @def DECL_NO_INLINE
 * How to declare a function telling the compiler not to inline it.
 * @param   scope   The function scope, static or RT_NOTHING.
 * @param   type    The return type of the function declaration.
 * @remarks Don't use this macro on C++ methods.
 */
#ifdef __GNUC__
# define DECL_NO_INLINE(scope,type) __attribute__((__noinline__)) scope type
#elif defined(_MSC_VER)
# define DECL_NO_INLINE(scope,type) __declspec(noinline) scope type
#else
# define DECL_NO_INLINE(scope,type) scope type
#endif


/** @def IN_RT_STATIC
 * Used to indicate whether we're linking against a static IPRT
 * or not. The IPRT symbols will be declared as hidden (if
 * supported). Note that this define has no effect without setting
 * IN_RT_R0, IN_RT_R3 or IN_RT_RC indicators are set first.
 */

/** @def IN_RT_R0
 * Used to indicate whether we're inside the same link module as
 * the HC Ring-0 Runtime Library.
 */
/** @def RTR0DECL(type)
 * Runtime Library HC Ring-0 export or import declaration.
 * @param   type    The return type of the function declaration.
 */
#ifdef IN_RT_R0
# ifdef IN_RT_STATIC
#  define RTR0DECL(type)    DECLHIDDEN(type) RTCALL
# else
#  define RTR0DECL(type)    DECLEXPORT(type) RTCALL
# endif
#else
# define RTR0DECL(type)     DECLIMPORT(type) RTCALL
#endif

/** @def IN_RT_R3
 * Used to indicate whether we're inside the same link module as
 * the HC Ring-3 Runtime Library.
 */
/** @def RTR3DECL(type)
 * Runtime Library HC Ring-3 export or import declaration.
 * @param   type    The return type of the function declaration.
 */
#ifdef IN_RT_R3
# ifdef IN_RT_STATIC
#  define RTR3DECL(type)    DECLHIDDEN(type) RTCALL
# else
#  define RTR3DECL(type)    DECLEXPORT(type) RTCALL
# endif
#else
# define RTR3DECL(type)     DECLIMPORT(type) RTCALL
#endif

/** @def IN_RT_RC
 * Used to indicate whether we're inside the same link module as the raw-mode
 * context (RC) runtime library.
 */
/** @def RTRCDECL(type)
 * Runtime Library raw-mode context export or import declaration.
 * @param   type    The return type of the function declaration.
 */
#ifdef IN_RT_RC
# ifdef IN_RT_STATIC
#  define RTRCDECL(type)    DECLHIDDEN(type) RTCALL
# else
#  define RTRCDECL(type)    DECLEXPORT(type) RTCALL
# endif
#else
# define RTRCDECL(type)     DECLIMPORT(type) RTCALL
#endif

/** @def RTDECL(type)
 * Runtime Library export or import declaration.
 * Functions declared using this macro exists in all contexts.
 * @param   type    The return type of the function declaration.
 */
#if defined(IN_RT_R3) || defined(IN_RT_RC) || defined(IN_RT_R0)
# ifdef IN_RT_STATIC
#  define RTDECL(type)      DECLHIDDEN(type) RTCALL
# else
#  define RTDECL(type)      DECLEXPORT(type) RTCALL
# endif
#else
# define RTDECL(type)       DECLIMPORT(type) RTCALL
#endif

/** @def RTDATADECL(type)
 * Runtime Library export or import declaration.
 * Data declared using this macro exists in all contexts.
 * @param   type    The data type.
 */
/** @def RT_DECL_DATA_CONST(type)
 * Definition of a const variable. See DECL_HIDDEN_CONST.
 * @param   type    The const data type.
 */
#if defined(IN_RT_R3) || defined(IN_RT_RC) || defined(IN_RT_R0)
# ifdef IN_RT_STATIC
#  define RTDATADECL(type)          DECLHIDDEN(type)
#  define RT_DECL_DATA_CONST(type)  DECL_HIDDEN_CONST(type)
# else
#  define RTDATADECL(type)          DECLEXPORT(type)
#  if defined(__cplusplus) && defined(__GNUC__)
#   define RT_DECL_DATA_CONST(type) type
#  else
#   define RT_DECL_DATA_CONST(type) DECLEXPORT(type)
#  endif
# endif
#else
# define RTDATADECL(type)           DECLIMPORT(type)
# define RT_DECL_DATA_CONST(type)   DECLIMPORT(type)
#endif

/** @def RT_DECL_CLASS
 * Declares an class living in the runtime.
 */
#if defined(IN_RT_R3) || defined(IN_RT_RC) || defined(IN_RT_R0)
# ifdef IN_RT_STATIC
#  define RT_DECL_CLASS
# else
#  define RT_DECL_CLASS     DECLEXPORT_CLASS
# endif
#else
# define RT_DECL_CLASS      DECLIMPORT_CLASS
#endif


/** @def RT_NOCRT
 * Symbol name wrapper for the No-CRT bits.
 *
 * In order to coexist in the same process as other CRTs, we need to
 * decorate the symbols such that they don't conflict the ones in the
 * other CRTs. The result of such conflicts / duplicate symbols can
 * confuse the dynamic loader on Unix like systems.
 *
 * Define RT_WITHOUT_NOCRT_WRAPPERS to drop the wrapping.
 * Define RT_WITHOUT_NOCRT_WRAPPER_ALIASES to drop the aliases to the
 * wrapped names.
 */
/** @def RT_NOCRT_STR
 * Same as RT_NOCRT only it'll return a double quoted string of the result.
 */
#ifndef RT_WITHOUT_NOCRT_WRAPPERS
# define RT_NOCRT(name) nocrt_ ## name
# define RT_NOCRT_STR(name) "nocrt_" # name
#else
# define RT_NOCRT(name) name
# define RT_NOCRT_STR(name) #name
#endif



/** @def RT_LIKELY
 * Give the compiler a hint that an expression is very likely to hold true.
 *
 * Some compilers support explicit branch prediction so that the CPU backend
 * can hint the processor and also so that code blocks can be reordered such
 * that the predicted path sees a more linear flow, thus improving cache
 * behaviour, etc.
 *
 * IPRT provides the macros RT_LIKELY() and RT_UNLIKELY() as a way to utilize
 * this compiler feature when present.
 *
 * A few notes about the usage:
 *
 *      - Generally, order your code use RT_LIKELY() instead of RT_UNLIKELY().
 *
 *      - Generally, use RT_UNLIKELY() with error condition checks (unless you
 *        have some _strong_ reason to do otherwise, in which case document it),
 *        and/or RT_LIKELY() with success condition checks, assuming you want
 *        to optimize for the success path.
 *
 *      - Other than that, if you don't know the likelihood of a test succeeding
 *        from empirical or other 'hard' evidence, don't make predictions unless
 *        you happen to be a Dirk Gently character.
 *
 *      - These macros are meant to be used in places that get executed a lot. It
 *        is wasteful to make predictions in code that is executed rarely (e.g.
 *        at subsystem initialization time) as the basic block reordering that this
 *        affects can often generate larger code.
 *
 *      - Note that RT_SUCCESS() and RT_FAILURE() already makes use of RT_LIKELY()
 *        and RT_UNLIKELY().  Should you wish for prediction free status checks,
 *        use the RT_SUCCESS_NP() and RT_FAILURE_NP() macros instead.
 *
 *
 * @returns the boolean result of the expression.
 * @param   expr        The expression that's very likely to be true.
 * @see     RT_UNLIKELY
 */
/** @def RT_UNLIKELY
 * Give the compiler a hint that an expression is highly unlikely to hold true.
 *
 * See the usage instructions give in the RT_LIKELY() docs.
 *
 * @returns the boolean result of the expression.
 * @param   expr        The expression that's very unlikely to be true.
 * @see     RT_LIKELY
 *
 * @deprecated Please use RT_LIKELY() instead wherever possible!  That gives us
 *          a better chance of the windows compilers to generate favorable code
 *          too.  The belief is that the compiler will by default assume the
 *          if-case is more likely than the else-case.
 */
#if defined(__GNUC__)
# if __GNUC__ >= 3 && !defined(FORTIFY_RUNNING)
#  define RT_LIKELY(expr)       __builtin_expect(!!(expr), 1)
#  define RT_UNLIKELY(expr)     __builtin_expect(!!(expr), 0)
# else
#  define RT_LIKELY(expr)       (expr)
#  define RT_UNLIKELY(expr)     (expr)
# endif
#else
# define RT_LIKELY(expr)        (expr)
# define RT_UNLIKELY(expr)      (expr)
#endif


/** @def RT_STR
 * Returns the argument as a string constant.
 * @param   str     Argument to stringify.  */
#define RT_STR(str)             #str
/** @def RT_XSTR
 * Returns the expanded argument as a string.
 * @param   str     Argument to expand and stringy. */
#define RT_XSTR(str)            RT_STR(str)

/** @def RT_CONCAT
 * Concatenate the expanded arguments without any extra spaces in between.
 *
 * @param   a       The first part.
 * @param   b       The second part.
 */
#define RT_CONCAT(a,b)              RT_CONCAT_HLP(a,b)
/** RT_CONCAT helper, don't use.  */
#define RT_CONCAT_HLP(a,b)          a##b

/** @def RT_CONCAT
 * Concatenate the expanded arguments without any extra spaces in between.
 *
 * @param   a       The 1st part.
 * @param   b       The 2nd part.
 * @param   c       The 3rd part.
 */
#define RT_CONCAT3(a,b,c)           RT_CONCAT3_HLP(a,b,c)
/** RT_CONCAT3 helper, don't use.  */
#define RT_CONCAT3_HLP(a,b,c)       a##b##c

/** @def RT_CONCAT
 * Concatenate the expanded arguments without any extra spaces in between.
 *
 * @param   a       The 1st part.
 * @param   b       The 2nd part.
 * @param   c       The 3rd part.
 */
#define RT_CONCAT4(a,b,c,d)         RT_CONCAT4_HLP(a,b,c,d)
/** RT_CONCAT4 helper, don't use.  */
#define RT_CONCAT4_HLP(a,b,c,d)     a##b##c##d

/**
 * String constant tuple - string constant, strlen(string constant).
 *
 * @param   a_szConst   String constant.
 */
#define RT_STR_TUPLE(a_szConst)  a_szConst, (sizeof(a_szConst) - 1)


/**
 * Macro for using in switch statements that turns constants into strings.
 *
 * @param   a_Const     The constant (not string).
 */
#define RT_CASE_RET_STR(a_Const)     case a_Const: return #a_Const


/** @def RT_BIT
 * Convert a bit number into an integer bitmask (unsigned).
 * @param   bit     The bit number.
 */
#define RT_BIT(bit)                             ( 1U << (bit) )

/** @def RT_BIT_32
 * Convert a bit number into a 32-bit bitmask (unsigned).
 * @param   bit     The bit number.
 */
#define RT_BIT_32(bit)                          ( UINT32_C(1) << (bit) )

/** @def RT_BIT_64
 * Convert a bit number into a 64-bit bitmask (unsigned).
 * @param   bit     The bit number.
 */
#define RT_BIT_64(bit)                          ( UINT64_C(1) << (bit) )

/** @def RT_ALIGN
 * Align macro.
 * @param   u           Value to align.
 * @param   uAlignment  The alignment. Power of two!
 *
 * @remark  Be extremely careful when using this macro with type which sizeof != sizeof int.
 *          When possible use any of the other RT_ALIGN_* macros. And when that's not
 *          possible, make 101% sure that uAlignment is specified with a right sized type.
 *
 *          Specifying an unsigned 32-bit alignment constant with a 64-bit value will give
 *          you a 32-bit return value!
 *
 *          In short: Don't use this macro. Use RT_ALIGN_T() instead.
 */
#define RT_ALIGN(u, uAlignment)                 ( ((u) + ((uAlignment) - 1)) & ~((uAlignment) - 1) )

/** @def RT_ALIGN_T
 * Align macro.
 * @param   u           Value to align.
 * @param   uAlignment  The alignment. Power of two!
 * @param   type        Integer type to use while aligning.
 * @remark  This macro is the preferred alignment macro, it doesn't have any of the pitfalls RT_ALIGN has.
 */
#define RT_ALIGN_T(u, uAlignment, type)         ( ((type)(u) + ((uAlignment) - 1)) & ~(type)((uAlignment) - 1) )

/** @def RT_ALIGN_32
 * Align macro for a 32-bit value.
 * @param   u32         Value to align.
 * @param   uAlignment  The alignment. Power of two!
 */
#define RT_ALIGN_32(u32, uAlignment)            RT_ALIGN_T(u32, uAlignment, uint32_t)

/** @def RT_ALIGN_64
 * Align macro for a 64-bit value.
 * @param   u64         Value to align.
 * @param   uAlignment  The alignment. Power of two!
 */
#define RT_ALIGN_64(u64, uAlignment)            RT_ALIGN_T(u64, uAlignment, uint64_t)

/** @def RT_ALIGN_Z
 * Align macro for size_t.
 * @param   cb          Value to align.
 * @param   uAlignment  The alignment. Power of two!
 */
#define RT_ALIGN_Z(cb, uAlignment)              RT_ALIGN_T(cb, uAlignment, size_t)

/** @def RT_ALIGN_P
 * Align macro for pointers.
 * @param   pv          Value to align.
 * @param   uAlignment  The alignment. Power of two!
 */
#define RT_ALIGN_P(pv, uAlignment)              RT_ALIGN_PT(pv, uAlignment, void *)

/** @def RT_ALIGN_PT
 * Align macro for pointers with type cast.
 * @param   u           Value to align.
 * @param   uAlignment  The alignment. Power of two!
 * @param   CastType    The type to cast the result to.
 */
#define RT_ALIGN_PT(u, uAlignment, CastType)    ( (CastType)RT_ALIGN_T(u, uAlignment, uintptr_t) )

/** @def RT_ALIGN_R3PT
 * Align macro for ring-3 pointers with type cast.
 * @param   u           Value to align.
 * @param   uAlignment  The alignment. Power of two!
 * @param   CastType    The type to cast the result to.
 */
#define RT_ALIGN_R3PT(u, uAlignment, CastType)  ( (CastType)RT_ALIGN_T(u, uAlignment, RTR3UINTPTR) )

/** @def RT_ALIGN_R0PT
 * Align macro for ring-0 pointers with type cast.
 * @param   u           Value to align.
 * @param   uAlignment  The alignment. Power of two!
 * @param   CastType    The type to cast the result to.
 */
#define RT_ALIGN_R0PT(u, uAlignment, CastType)  ( (CastType)RT_ALIGN_T(u, uAlignment, RTR0UINTPTR) )

/** @def RT_ALIGN_GCPT
 * Align macro for GC pointers with type cast.
 * @param   u           Value to align.
 * @param   uAlignment  The alignment. Power of two!
 * @param   CastType        The type to cast the result to.
 */
#define RT_ALIGN_GCPT(u, uAlignment, CastType)  ( (CastType)RT_ALIGN_T(u, uAlignment, RTGCUINTPTR) )


/** @def RT_OFFSETOF
 * Our own special offsetof() variant, returns a signed result.
 *
 * This differs from the usual offsetof() in that it's not relying on builtin
 * compiler stuff and thus can use variables in arrays the structure may
 * contain. This is useful to determine the sizes of structures ending
 * with a variable length field. For gcc >= 4.4 see @bugref{7775}.
 *
 * @returns offset into the structure of the specified member. signed.
 * @param   type    Structure type.
 * @param   member  Member.
 */
#if defined(__GNUC__) && defined(__cplusplus) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
# define RT_OFFSETOF(type, member)              ( (int)(uintptr_t)&( ((type *)(void *)0x1000)->member) - 0x1000 )
#else
# define RT_OFFSETOF(type, member)              ( (int)(uintptr_t)&( ((type *)(void *)0)->member) )
#endif

/** @def RT_UOFFSETOF
 * Our own special offsetof() variant, returns an unsigned result.
 *
 * This differs from the usual offsetof() in that it's not relying on builtin
 * compiler stuff and thus can use variables in arrays the structure may
 * contain. This is useful to determine the sizes of structures ending
 * with a variable length field. For gcc >= 4.4 see @bugref{7775}.
 *
 * @returns offset into the structure of the specified member. unsigned.
 * @param   type    Structure type.
 * @param   member  Member.
 */
#if defined(__GNUC__) && defined(__cplusplus) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
# define RT_UOFFSETOF(type, member)             ( (uintptr_t)&( ((type *)(void *)0x1000)->member) - 0x1000 )
#else
# define RT_UOFFSETOF(type, member)             ( (uintptr_t)&( ((type *)(void *)0)->member) )
#endif

/** @def RT_OFFSETOF_ADD
 * RT_OFFSETOF with an addend.
 *
 * @returns offset into the structure of the specified member. signed.
 * @param   type    Structure type.
 * @param   member  Member.
 * @param   addend  The addend to add to the offset.
 */
#define RT_OFFSETOF_ADD(type, member, addend)   ( (int)RT_UOFFSETOF_ADD(type, member, addend) )

/** @def RT_UOFFSETOF_ADD
 * RT_UOFFSETOF with an addend.
 *
 * @returns offset into the structure of the specified member. signed.
 * @param   type    Structure type.
 * @param   member  Member.
 * @param   addend  The addend to add to the offset.
 */
#define RT_UOFFSETOF_ADD(type, member, addend)  ( (uintptr_t)&( ((type *)(void *)(uintptr_t)(addend))->member) )

/** @def RT_SIZEOFMEMB
 * Get the size of a structure member.
 *
 * @returns size of the structure member.
 * @param   type    Structure type.
 * @param   member  Member.
 */
#define RT_SIZEOFMEMB(type, member)             ( sizeof(((type *)(void *)0)->member) )

/** @def RT_FROM_MEMBER
 * Convert a pointer to a structure member into a pointer to the structure.
 *
 * @returns pointer to the structure.
 * @param   pMem    Pointer to the member.
 * @param   Type    Structure type.
 * @param   Member  Member name.
 */
#define RT_FROM_MEMBER(pMem, Type, Member)      ( (Type *) ((uint8_t *)(void *)(pMem) - RT_UOFFSETOF(Type, Member)) )

/** @def RT_FROM_CPP_MEMBER
 * Same as RT_FROM_MEMBER except it avoids the annoying g++ warnings about
 * invalid access to non-static data member of NULL object.
 *
 * @returns pointer to the structure.
 * @param   pMem    Pointer to the member.
 * @param   Type    Structure type.
 * @param   Member  Member name.
 *
 * @remarks Using the __builtin_offsetof does not shut up the compiler.
 */
#if defined(__GNUC__) && defined(__cplusplus)
# define RT_FROM_CPP_MEMBER(pMem, Type, Member) \
        ( (Type *) ((uintptr_t)(pMem) - (uintptr_t)&((Type *)0x1000)->Member + 0x1000U) )
#else
# define RT_FROM_CPP_MEMBER(pMem, Type, Member) RT_FROM_MEMBER(pMem, Type, Member)
#endif

/** @def RT_ELEMENTS
 * Calculates the number of elements in a statically sized array.
 * @returns Element count.
 * @param   aArray      Array in question.
 */
#define RT_ELEMENTS(aArray)                     ( sizeof(aArray) / sizeof((aArray)[0]) )

/**
 * Checks if the value is a power of two.
 *
 * @returns true if power of two, false if not.
 * @param   uVal                The value to test.
 * @remarks 0 is a power of two.
 * @see     VERR_NOT_POWER_OF_TWO
 */
#define RT_IS_POWER_OF_TWO(uVal)                ( ((uVal) & ((uVal) - 1)) == 0)

#ifdef RT_OS_OS2
/* Undefine RT_MAX since there is an unfortunate clash with the max
   resource type define in os2.h. */
# undef RT_MAX
#endif

/** @def RT_MAX
 * Finds the maximum value.
 * @returns The higher of the two.
 * @param   Value1      Value 1
 * @param   Value2      Value 2
 */
#define RT_MAX(Value1, Value2)                  ( (Value1) >= (Value2) ? (Value1) : (Value2) )

/** @def RT_MIN
 * Finds the minimum value.
 * @returns The lower of the two.
 * @param   Value1      Value 1
 * @param   Value2      Value 2
 */
#define RT_MIN(Value1, Value2)                  ( (Value1) <= (Value2) ? (Value1) : (Value2) )

/** @def RT_CLAMP
 * Clamps the value to minimum and maximum values.
 * @returns The clamped value.
 * @param   Value       The value to check.
 * @param   Min         Minimum value.
 * @param   Max         Maximum value.
 */
#define RT_CLAMP(Value, Min, Max)               ( (Value) > (Max) ? (Max) : (Value) < (Min) ? (Min) : (Value) )

/** @def RT_ABS
 * Get the absolute (non-negative) value.
 * @returns The absolute value of Value.
 * @param   Value       The value.
 */
#define RT_ABS(Value)                           ( (Value) >= 0 ? (Value) : -(Value) )

/** @def RT_BOOL
 * Turn non-zero/zero into true/false
 * @returns The resulting boolean value.
 * @param   Value       The value.
 */
#define RT_BOOL(Value)                          ( !!(Value) )

/** @def RT_LO_U8
 * Gets the low uint8_t of a uint16_t or something equivalent. */
#ifdef __GNUC__
# define RT_LO_U8(a)    __extension__ ({ AssertCompile(sizeof((a)) == sizeof(uint16_t)); (uint8_t)(a); })
#else
# define RT_LO_U8(a)                            ( (uint8_t)(a) )
#endif
/** @def RT_HI_U8
 * Gets the high uint8_t of a uint16_t or something equivalent. */
#ifdef __GNUC__
# define RT_HI_U8(a)    __extension__ ({ AssertCompile(sizeof((a)) == sizeof(uint16_t)); (uint8_t)((a) >> 8); })
#else
# define RT_HI_U8(a)                            ( (uint8_t)((a) >> 8) )
#endif

/** @def RT_LO_U16
 * Gets the low uint16_t of a uint32_t or something equivalent. */
#ifdef __GNUC__
# define RT_LO_U16(a)   __extension__ ({ AssertCompile(sizeof((a)) == sizeof(uint32_t)); (uint16_t)(a); })
#else
# define RT_LO_U16(a)                           ( (uint16_t)(a) )
#endif
/** @def RT_HI_U16
 * Gets the high uint16_t of a uint32_t or something equivalent. */
#ifdef __GNUC__
# define RT_HI_U16(a)   __extension__ ({ AssertCompile(sizeof((a)) == sizeof(uint32_t)); (uint16_t)((a) >> 16); })
#else
# define RT_HI_U16(a)                           ( (uint16_t)((a) >> 16) )
#endif

/** @def RT_LO_U32
 * Gets the low uint32_t of a uint64_t or something equivalent. */
#ifdef __GNUC__
# define RT_LO_U32(a)   __extension__ ({ AssertCompile(sizeof((a)) == sizeof(uint64_t)); (uint32_t)(a); })
#else
# define RT_LO_U32(a)                           ( (uint32_t)(a) )
#endif
/** @def RT_HI_U32
 * Gets the high uint32_t of a uint64_t or something equivalent. */
#ifdef __GNUC__
# define RT_HI_U32(a)   __extension__ ({ AssertCompile(sizeof((a)) == sizeof(uint64_t)); (uint32_t)((a) >> 32); })
#else
# define RT_HI_U32(a)                           ( (uint32_t)((a) >> 32) )
#endif

/** @def RT_BYTE1
 * Gets the first byte of something. */
#define RT_BYTE1(a)                             ( (a)         & 0xff )
/** @def RT_BYTE2
 * Gets the second byte of something. */
#define RT_BYTE2(a)                             ( ((a) >>  8) & 0xff )
/** @def RT_BYTE3
 * Gets the second byte of something. */
#define RT_BYTE3(a)                             ( ((a) >> 16) & 0xff )
/** @def RT_BYTE4
 * Gets the fourth byte of something. */
#define RT_BYTE4(a)                             ( ((a) >> 24) & 0xff )
/** @def RT_BYTE5
 * Gets the fifth byte of something. */
#define RT_BYTE5(a)                             ( ((a) >> 32) & 0xff )
/** @def RT_BYTE6
 * Gets the sixth byte of something. */
#define RT_BYTE6(a)                             ( ((a) >> 40) & 0xff )
/** @def RT_BYTE7
 * Gets the seventh byte of something. */
#define RT_BYTE7(a)                             ( ((a) >> 48) & 0xff )
/** @def RT_BYTE8
 * Gets the eight byte of something. */
#define RT_BYTE8(a)                             ( ((a) >> 56) & 0xff )


/** @def RT_LODWORD
 * Gets the low dword (=uint32_t) of something.
 * @deprecated  Use RT_LO_U32. */
#define RT_LODWORD(a)                           ( (uint32_t)(a) )
/** @def RT_HIDWORD
 * Gets the high dword (=uint32_t) of a 64-bit of something.
 * @deprecated  Use RT_HI_U32. */
#define RT_HIDWORD(a)                           ( (uint32_t)((a) >> 32) )

/** @def RT_LOWORD
 * Gets the low word (=uint16_t) of something.
 * @deprecated  Use RT_LO_U16. */
#define RT_LOWORD(a)                            ( (a) & 0xffff )
/** @def RT_HIWORD
 * Gets the high word (=uint16_t) of a 32-bit something.
 * @deprecated  Use RT_HI_U16. */
#define RT_HIWORD(a)                            ( (a) >> 16 )

/** @def RT_LOBYTE
 * Gets the low byte of something.
 * @deprecated  Use RT_LO_U8. */
#define RT_LOBYTE(a)                            ( (a) & 0xff )
/** @def RT_HIBYTE
 * Gets the high byte of a 16-bit something.
 * @deprecated  Use RT_HI_U8. */
#define RT_HIBYTE(a)                            ( (a) >> 8 )


/** @def RT_MAKE_U64
 * Constructs a uint64_t value from two uint32_t values.
 */
#define RT_MAKE_U64(Lo, Hi)                     ( (uint64_t)((uint32_t)(Hi)) << 32 | (uint32_t)(Lo) )

/** @def RT_MAKE_U64_FROM_U16
 * Constructs a uint64_t value from four uint16_t values.
 */
#define RT_MAKE_U64_FROM_U16(w0, w1, w2, w3) \
    ((uint64_t)(  (uint64_t)((uint16_t)(w3)) << 48 \
                | (uint64_t)((uint16_t)(w2)) << 32 \
                | (uint32_t)((uint16_t)(w1)) << 16 \
                |            (uint16_t)(w0) ))

/** @def RT_MAKE_U64_FROM_U8
 * Constructs a uint64_t value from eight uint8_t values.
 */
#define RT_MAKE_U64_FROM_U8(b0, b1, b2, b3, b4, b5, b6, b7) \
    ((uint64_t)(  (uint64_t)((uint8_t)(b7)) << 56 \
                | (uint64_t)((uint8_t)(b6)) << 48 \
                | (uint64_t)((uint8_t)(b5)) << 40 \
                | (uint64_t)((uint8_t)(b4)) << 32 \
                | (uint32_t)((uint8_t)(b3)) << 24 \
                | (uint32_t)((uint8_t)(b2)) << 16 \
                | (uint16_t)((uint8_t)(b1)) << 8 \
                |            (uint8_t)(b0) ))

/** @def RT_MAKE_U32
 * Constructs a uint32_t value from two uint16_t values.
 */
#define RT_MAKE_U32(Lo, Hi) \
    ((uint32_t)(  (uint32_t)((uint16_t)(Hi)) << 16 \
                | (uint16_t)(Lo) ))

/** @def RT_MAKE_U32_FROM_U8
 * Constructs a uint32_t value from four uint8_t values.
 */
#define RT_MAKE_U32_FROM_U8(b0, b1, b2, b3) \
    ((uint32_t)(  (uint32_t)((uint8_t)(b3)) << 24 \
                | (uint32_t)((uint8_t)(b2)) << 16 \
                | (uint16_t)((uint8_t)(b1)) << 8 \
                |            (uint8_t)(b0) ))

/** @def RT_MAKE_U16
 * Constructs a uint16_t value from two uint8_t values.
 */
#define RT_MAKE_U16(Lo, Hi) \
    ((uint16_t)(  (uint16_t)((uint8_t)(Hi)) << 8 \
                | (uint8_t)(Lo) ))


/** @def RT_BSWAP_U64
 * Reverses the byte order of an uint64_t value. */
#if 0
# define RT_BSWAP_U64(u64)  RT_BSWAP_U64_C(u64)
#elif defined(__GNUC__)
# define RT_BSWAP_U64(u64)  (__builtin_constant_p((u64)) \
                            ? RT_BSWAP_U64_C(u64) : ASMByteSwapU64(u64))
#else
# define RT_BSWAP_U64(u64)  ASMByteSwapU64(u64)
#endif

/** @def RT_BSWAP_U32
 * Reverses the byte order of an uint32_t value. */
#if 0
# define RT_BSWAP_U32(u32)  RT_BSWAP_U32_C(u32)
#elif defined(__GNUC__)
# define RT_BSWAP_U32(u32)  (__builtin_constant_p((u32)) \
                            ? RT_BSWAP_U32_C(u32) : ASMByteSwapU32(u32))
#else
# define RT_BSWAP_U32(u32)  ASMByteSwapU32(u32)
#endif

/** @def RT_BSWAP_U16
 * Reverses the byte order of an uint16_t value. */
#if 0
# define RT_BSWAP_U16(u16)  RT_BSWAP_U16_C(u16)
#elif defined(__GNUC__)
# define RT_BSWAP_U16(u16)  (__builtin_constant_p((u16)) \
                            ? RT_BSWAP_U16_C(u16) : ASMByteSwapU16(u16))
#else
# define RT_BSWAP_U16(u16)  ASMByteSwapU16(u16)
#endif


/** @def RT_BSWAP_U64_C
 * Reverses the byte order of an uint64_t constant. */
#define RT_BSWAP_U64_C(u64) RT_MAKE_U64(RT_BSWAP_U32_C((u64) >> 32), RT_BSWAP_U32_C((u64) & 0xffffffff))

/** @def RT_BSWAP_U32_C
 * Reverses the byte order of an uint32_t constant. */
#define RT_BSWAP_U32_C(u32) RT_MAKE_U32_FROM_U8(RT_BYTE4(u32), RT_BYTE3(u32), RT_BYTE2(u32), RT_BYTE1(u32))

/** @def RT_BSWAP_U16_C
 * Reverses the byte order of an uint16_t constant. */
#define RT_BSWAP_U16_C(u16) RT_MAKE_U16(RT_HIBYTE(u16), RT_LOBYTE(u16))


/** @def RT_H2LE_U64
 * Converts an uint64_t value from host to little endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2LE_U64(u64)   RT_BSWAP_U64(u64)
#else
# define RT_H2LE_U64(u64)   (u64)
#endif

/** @def RT_H2LE_U64_C
 * Converts an uint64_t constant from host to little endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2LE_U64_C(u64) RT_BSWAP_U64_C(u64)
#else
# define RT_H2LE_U64_C(u64) (u64)
#endif

/** @def RT_H2LE_U32
 * Converts an uint32_t value from host to little endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2LE_U32(u32)   RT_BSWAP_U32(u32)
#else
# define RT_H2LE_U32(u32)   (u32)
#endif

/** @def RT_H2LE_U32_C
 * Converts an uint32_t constant from host to little endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2LE_U32_C(u32) RT_BSWAP_U32_C(u32)
#else
# define RT_H2LE_U32_C(u32) (u32)
#endif

/** @def RT_H2LE_U16
 * Converts an uint16_t value from host to little endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2LE_U16(u16)   RT_BSWAP_U16(u16)
#else
# define RT_H2LE_U16(u16)   (u16)
#endif

/** @def RT_H2LE_U16_C
 * Converts an uint16_t constant from host to little endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2LE_U16_C(u16) RT_BSWAP_U16_C(u16)
#else
# define RT_H2LE_U16_C(u16) (u16)
#endif


/** @def RT_LE2H_U64
 * Converts an uint64_t value from little endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_LE2H_U64(u64)   RT_BSWAP_U64(u64)
#else
# define RT_LE2H_U64(u64)   (u64)
#endif

/** @def RT_LE2H_U64_C
 * Converts an uint64_t constant from little endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_LE2H_U64_C(u64) RT_BSWAP_U64_C(u64)
#else
# define RT_LE2H_U64_C(u64) (u64)
#endif

/** @def RT_LE2H_U32
 * Converts an uint32_t value from little endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_LE2H_U32(u32)   RT_BSWAP_U32(u32)
#else
# define RT_LE2H_U32(u32)   (u32)
#endif

/** @def RT_LE2H_U32_C
 * Converts an uint32_t constant from little endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_LE2H_U32_C(u32) RT_BSWAP_U32_C(u32)
#else
# define RT_LE2H_U32_C(u32) (u32)
#endif

/** @def RT_LE2H_U16
 * Converts an uint16_t value from little endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_LE2H_U16(u16)   RT_BSWAP_U16(u16)
#else
# define RT_LE2H_U16(u16)   (u16)
#endif

/** @def RT_LE2H_U16_C
 * Converts an uint16_t constant from little endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_LE2H_U16_C(u16) RT_BSWAP_U16_C(u16)
#else
# define RT_LE2H_U16_C(u16) (u16)
#endif


/** @def RT_H2BE_U64
 * Converts an uint64_t value from host to big endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2BE_U64(u64)   (u64)
#else
# define RT_H2BE_U64(u64)   RT_BSWAP_U64(u64)
#endif

/** @def RT_H2BE_U64_C
 * Converts an uint64_t constant from host to big endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2BE_U64_C(u64) (u64)
#else
# define RT_H2BE_U64_C(u64) RT_BSWAP_U64_C(u64)
#endif

/** @def RT_H2BE_U32
 * Converts an uint32_t value from host to big endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2BE_U32(u32)   (u32)
#else
# define RT_H2BE_U32(u32)   RT_BSWAP_U32(u32)
#endif

/** @def RT_H2BE_U32_C
 * Converts an uint32_t constant from host to big endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2BE_U32_C(u32) (u32)
#else
# define RT_H2BE_U32_C(u32) RT_BSWAP_U32_C(u32)
#endif

/** @def RT_H2BE_U16
 * Converts an uint16_t value from host to big endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2BE_U16(u16)   (u16)
#else
# define RT_H2BE_U16(u16)   RT_BSWAP_U16(u16)
#endif

/** @def RT_H2BE_U16_C
 * Converts an uint16_t constant from host to big endian byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_H2BE_U16_C(u16) (u16)
#else
# define RT_H2BE_U16_C(u16) RT_BSWAP_U16_C(u16)
#endif

/** @def RT_BE2H_U64
 * Converts an uint64_t value from big endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_BE2H_U64(u64)   (u64)
#else
# define RT_BE2H_U64(u64)   RT_BSWAP_U64(u64)
#endif

/** @def RT_BE2H_U64
 * Converts an uint64_t constant from big endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_BE2H_U64_C(u64) (u64)
#else
# define RT_BE2H_U64_C(u64) RT_BSWAP_U64_C(u64)
#endif

/** @def RT_BE2H_U32
 * Converts an uint32_t value from big endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_BE2H_U32(u32)   (u32)
#else
# define RT_BE2H_U32(u32)   RT_BSWAP_U32(u32)
#endif

/** @def RT_BE2H_U32_C
 * Converts an uint32_t value from big endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_BE2H_U32_C(u32) (u32)
#else
# define RT_BE2H_U32_C(u32) RT_BSWAP_U32_C(u32)
#endif

/** @def RT_BE2H_U16
 * Converts an uint16_t value from big endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_BE2H_U16(u16)   (u16)
#else
# define RT_BE2H_U16(u16)   RT_BSWAP_U16(u16)
#endif

/** @def RT_BE2H_U16_C
 * Converts an uint16_t constant from big endian to host byte order. */
#ifdef RT_BIG_ENDIAN
# define RT_BE2H_U16_C(u16) (u16)
#else
# define RT_BE2H_U16_C(u16) RT_BSWAP_U16_C(u16)
#endif


/** @def RT_H2N_U64
 * Converts an uint64_t value from host to network byte order. */
#define RT_H2N_U64(u64)     RT_H2BE_U64(u64)

/** @def RT_H2N_U64_C
 * Converts an uint64_t constant from host to network byte order. */
#define RT_H2N_U64_C(u64)   RT_H2BE_U64_C(u64)

/** @def RT_H2N_U32
 * Converts an uint32_t value from host to network byte order. */
#define RT_H2N_U32(u32)     RT_H2BE_U32(u32)

/** @def RT_H2N_U32_C
 * Converts an uint32_t constant from host to network byte order. */
#define RT_H2N_U32_C(u32)   RT_H2BE_U32_C(u32)

/** @def RT_H2N_U16
 * Converts an uint16_t value from host to network byte order. */
#define RT_H2N_U16(u16)     RT_H2BE_U16(u16)

/** @def RT_H2N_U16_C
 * Converts an uint16_t constant from host to network byte order. */
#define RT_H2N_U16_C(u16)   RT_H2BE_U16_C(u16)

/** @def RT_N2H_U64
 * Converts an uint64_t value from network to host byte order. */
#define RT_N2H_U64(u64)     RT_BE2H_U64(u64)

/** @def RT_N2H_U64_C
 * Converts an uint64_t constant from network to host byte order. */
#define RT_N2H_U64_C(u64)   RT_BE2H_U64_C(u64)

/** @def RT_N2H_U32
 * Converts an uint32_t value from network to host byte order. */
#define RT_N2H_U32(u32)     RT_BE2H_U32(u32)

/** @def RT_N2H_U32_C
 * Converts an uint32_t constant from network to host byte order. */
#define RT_N2H_U32_C(u32)   RT_BE2H_U32_C(u32)

/** @def RT_N2H_U16
 * Converts an uint16_t value from network to host byte order. */
#define RT_N2H_U16(u16)     RT_BE2H_U16(u16)

/** @def RT_N2H_U16_C
 * Converts an uint16_t value from network to host byte order. */
#define RT_N2H_U16_C(u16)   RT_BE2H_U16_C(u16)


/*
 * The BSD sys/param.h + machine/param.h file is a major source of
 * namespace pollution. Kill off some of the worse ones unless we're
 * compiling kernel code.
 */
#if defined(RT_OS_DARWIN) \
  && !defined(KERNEL) \
  && !defined(RT_NO_BSD_PARAM_H_UNDEFING) \
  && ( defined(_SYS_PARAM_H_) || defined(_I386_PARAM_H_) )
/* sys/param.h: */
# undef PSWP
# undef PVM
# undef PINOD
# undef PRIBO
# undef PVFS
# undef PZERO
# undef PSOCK
# undef PWAIT
# undef PLOCK
# undef PPAUSE
# undef PUSER
# undef PRIMASK
# undef MINBUCKET
# undef MAXALLOCSAVE
# undef FSHIFT
# undef FSCALE

/* i386/machine.h: */
# undef ALIGN
# undef ALIGNBYTES
# undef DELAY
# undef STATUS_WORD
# undef USERMODE
# undef BASEPRI
# undef MSIZE
# undef CLSIZE
# undef CLSIZELOG2
#endif

/** @def NIL_OFFSET
 * NIL offset.
 * Whenever we use offsets instead of pointers to save space and relocation effort
 * NIL_OFFSET shall be used as the equivalent to NULL.
 */
#define NIL_OFFSET   (~0U)

/** @def NOREF
 * Keeps the compiler from bitching about an unused parameter.
 */
#define NOREF(var)               (void)(var)

/** @def RT_BREAKPOINT
 * Emit a debug breakpoint instruction.
 *
 * @remarks In the x86/amd64 gnu world we add a nop instruction after the int3
 *          to force gdb to remain at the int3 source line.
 * @remarks The L4 kernel will try make sense of the breakpoint, thus the jmp on
 *          x86/amd64.
 */
#ifdef __GNUC__
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if !defined(__L4ENV__)
#   define RT_BREAKPOINT()      __asm__ __volatile__("int $3\n\tnop\n\t")
#  else
#   define RT_BREAKPOINT()      __asm__ __volatile__("int3; jmp 1f; 1:\n\t")
#  endif
# elif defined(RT_ARCH_SPARC64)
#  define RT_BREAKPOINT()       __asm__ __volatile__("illtrap 0\n\t")   /** @todo Sparc64: this is just a wild guess. */
# elif defined(RT_ARCH_SPARC)
#  define RT_BREAKPOINT()       __asm__ __volatile__("unimp 0\n\t")     /** @todo Sparc: this is just a wild guess (same as Sparc64, just different name). */
# endif
#endif
#ifdef _MSC_VER
# define RT_BREAKPOINT()        __debugbreak()
#endif
#if defined(__IBMC__) || defined(__IBMCPP__)
# define RT_BREAKPOINT()        __interrupt(3)
#endif
#ifndef RT_BREAKPOINT
# error "This compiler/arch is not supported!"
#endif


/** @defgroup grp_rt_cdefs_size     Size Constants
 * (Of course, these are binary computer terms, not SI.)
 * @{
 */
/** 1 K (Kilo)                     (1 024). */
#define _1K                     0x00000400
/** 2 K (Kilo)                     (2 048). */
#define _2K                     0x00000800
/** 4 K (Kilo)                     (4 096). */
#define _4K                     0x00001000
/** 8 K (Kilo)                     (8 192). */
#define _8K                     0x00002000
/** 16 K (Kilo)                   (16 384). */
#define _16K                    0x00004000
/** 32 K (Kilo)                   (32 678). */
#define _32K                    0x00008000
/** 64 K (Kilo)                   (65 536). */
#define _64K                    0x00010000
/** 128 K (Kilo)                 (131 072). */
#define _128K                   0x00020000
/** 256 K (Kilo)                 (262 144). */
#define _256K                   0x00040000
/** 512 K (Kilo)                 (524 288). */
#define _512K                   0x00080000
/** 1 M (Mega)                 (1 048 576). */
#define _1M                     0x00100000
/** 2 M (Mega)                 (2 097 152). */
#define _2M                     0x00200000
/** 4 M (Mega)                 (4 194 304). */
#define _4M                     0x00400000
/** 1 G (Giga)             (1 073 741 824). (32-bit) */
#define _1G                     0x40000000
/** 1 G (Giga)             (1 073 741 824). (64-bit) */
#define _1G64                   0x40000000LL
/** 2 G (Giga)             (2 147 483 648). (32-bit) */
#define _2G32                   0x80000000U
/** 2 G (Giga)             (2 147 483 648). (64-bit) */
#define _2G             0x0000000080000000LL
/** 4 G (Giga)             (4 294 967 296). */
#define _4G             0x0000000100000000LL
/** 1 T (Tera)         (1 099 511 627 776). */
#define _1T             0x0000010000000000LL
/** 1 P (Peta)     (1 125 899 906 842 624). */
#define _1P             0x0004000000000000LL
/** 1 E (Exa)  (1 152 921 504 606 846 976). */
#define _1E             0x1000000000000000LL
/** 2 E (Exa)  (2 305 843 009 213 693 952). */
#define _2E             0x2000000000000000ULL
/** @} */

/** @defgroup grp_rt_cdefs_decimal_grouping   Decimal Constant Grouping Macros
 * @{ */
#define RT_D1(g1)                                   g1
#define RT_D2(g1, g2)                               g1#g2
#define RT_D3(g1, g2, g3)                           g1#g2#g3
#define RT_D4(g1, g2, g3, g4)                       g1#g2#g3#g4
#define RT_D5(g1, g2, g3, g4, g5)                   g1#g2#g3#g4#g5
#define RT_D6(g1, g2, g3, g4, g5, g6)               g1#g2#g3#g4#g5#g6
#define RT_D7(g1, g2, g3, g4, g5, g6, g7)           g1#g2#g3#g4#g5#g6#g7

#define RT_D1_U(g1)                                 UINT32_C(g1)
#define RT_D2_U(g1, g2)                             UINT32_C(g1#g2)
#define RT_D3_U(g1, g2, g3)                         UINT32_C(g1#g2#g3)
#define RT_D4_U(g1, g2, g3, g4)                     UINT64_C(g1#g2#g3#g4)
#define RT_D5_U(g1, g2, g3, g4, g5)                 UINT64_C(g1#g2#g3#g4#g5)
#define RT_D6_U(g1, g2, g3, g4, g5, g6)             UINT64_C(g1#g2#g3#g4#g5#g6)
#define RT_D7_U(g1, g2, g3, g4, g5, g6, g7)         UINT64_C(g1#g2#g3#g4#g5#g6#g7)

#define RT_D1_S(g1)                                 INT32_C(g1)
#define RT_D2_S(g1, g2)                             INT32_C(g1#g2)
#define RT_D3_S(g1, g2, g3)                         INT32_C(g1#g2#g3)
#define RT_D4_S(g1, g2, g3, g4)                     INT64_C(g1#g2#g3#g4)
#define RT_D5_S(g1, g2, g3, g4, g5)                 INT64_C(g1#g2#g3#g4#g5)
#define RT_D6_S(g1, g2, g3, g4, g5, g6)             INT64_C(g1#g2#g3#g4#g5#g6)
#define RT_D7_S(g1, g2, g3, g4, g5, g6, g7)         INT64_C(g1#g2#g3#g4#g5#g6#g7)

#define RT_D1_U32(g1)                               UINT32_C(g1)
#define RT_D2_U32(g1, g2)                           UINT32_C(g1#g2)
#define RT_D3_U32(g1, g2, g3)                       UINT32_C(g1#g2#g3)
#define RT_D4_U32(g1, g2, g3, g4)                   UINT32_C(g1#g2#g3#g4)

#define RT_D1_S32(g1)                               INT32_C(g1)
#define RT_D2_S32(g1, g2)                           INT32_C(g1#g2)
#define RT_D3_S32(g1, g2, g3)                       INT32_C(g1#g2#g3)
#define RT_D4_S32(g1, g2, g3, g4)                   INT32_C(g1#g2#g3#g4)

#define RT_D1_U64(g1)                               UINT64_C(g1)
#define RT_D2_U64(g1, g2)                           UINT64_C(g1#g2)
#define RT_D3_U64(g1, g2, g3)                       UINT64_C(g1#g2#g3)
#define RT_D4_U64(g1, g2, g3, g4)                   UINT64_C(g1#g2#g3#g4)
#define RT_D5_U64(g1, g2, g3, g4, g5)               UINT64_C(g1#g2#g3#g4#g5)
#define RT_D6_U64(g1, g2, g3, g4, g5, g6)           UINT64_C(g1#g2#g3#g4#g5#g6)
#define RT_D7_U64(g1, g2, g3, g4, g5, g6, g7)       UINT64_C(g1#g2#g3#g4#g5#g6#g7)

#define RT_D1_S64(g1)                               INT64_C(g1)
#define RT_D2_S64(g1, g2)                           INT64_C(g1#g2)
#define RT_D3_S64(g1, g2, g3)                       INT64_C(g1#g2#g3)
#define RT_D4_S64(g1, g2, g3, g4)                   INT64_C(g1#g2#g3#g4)
#define RT_D5_S64(g1, g2, g3, g4, g5)               INT64_C(g1#g2#g3#g4#g5)
#define RT_D6_S64(g1, g2, g3, g4, g5, g6)           INT64_C(g1#g2#g3#g4#g5#g6)
#define RT_D7_S64(g1, g2, g3, g4, g5, g6, g7)       INT64_C(g1#g2#g3#g4#g5#g6#g7)
/** @}  */


/** @defgroup grp_rt_cdefs_time     Time Constants
 * @{
 */
/** 1 hour expressed in nanoseconds (64-bit). */
#define RT_NS_1HOUR             UINT64_C(3600000000000)
/** 1 minute expressed in nanoseconds (64-bit). */
#define RT_NS_1MIN              UINT64_C(60000000000)
/** 45 second expressed in nanoseconds. */
#define RT_NS_45SEC             UINT64_C(45000000000)
/** 30 second expressed in nanoseconds. */
#define RT_NS_30SEC             UINT64_C(30000000000)
/** 20 second expressed in nanoseconds. */
#define RT_NS_20SEC             UINT64_C(20000000000)
/** 15 second expressed in nanoseconds. */
#define RT_NS_15SEC             UINT64_C(15000000000)
/** 10 second expressed in nanoseconds. */
#define RT_NS_10SEC             UINT64_C(10000000000)
/** 1 second expressed in nanoseconds. */
#define RT_NS_1SEC              UINT32_C(1000000000)
/** 100 millsecond expressed in nanoseconds. */
#define RT_NS_100MS             UINT32_C(100000000)
/** 10 millsecond expressed in nanoseconds. */
#define RT_NS_10MS              UINT32_C(10000000)
/** 1 millsecond expressed in nanoseconds. */
#define RT_NS_1MS               UINT32_C(1000000)
/** 100 microseconds expressed in nanoseconds. */
#define RT_NS_100US             UINT32_C(100000)
/** 10 microseconds expressed in nanoseconds. */
#define RT_NS_10US              UINT32_C(10000)
/** 1 microsecond expressed in nanoseconds. */
#define RT_NS_1US               UINT32_C(1000)

/** 1 second expressed in nanoseconds - 64-bit type. */
#define RT_NS_1SEC_64           UINT64_C(1000000000)
/** 100 millsecond expressed in nanoseconds - 64-bit type. */
#define RT_NS_100MS_64          UINT64_C(100000000)
/** 10 millsecond expressed in nanoseconds - 64-bit type. */
#define RT_NS_10MS_64           UINT64_C(10000000)
/** 1 millsecond expressed in nanoseconds - 64-bit type. */
#define RT_NS_1MS_64            UINT64_C(1000000)
/** 100 microseconds expressed in nanoseconds - 64-bit type. */
#define RT_NS_100US_64          UINT64_C(100000)
/** 10 microseconds expressed in nanoseconds - 64-bit type. */
#define RT_NS_10US_64           UINT64_C(10000)
/** 1 microsecond expressed in nanoseconds - 64-bit type. */
#define RT_NS_1US_64            UINT64_C(1000)

/** 1 hour expressed in microseconds. */
#define RT_US_1HOUR             UINT32_C(3600000000)
/** 1 minute expressed in microseconds. */
#define RT_US_1MIN              UINT32_C(60000000)
/** 1 second expressed in microseconds. */
#define RT_US_1SEC              UINT32_C(1000000)
/** 100 millsecond expressed in microseconds. */
#define RT_US_100MS             UINT32_C(100000)
/** 10 millsecond expressed in microseconds. */
#define RT_US_10MS              UINT32_C(10000)
/** 1 millsecond expressed in microseconds. */
#define RT_US_1MS               UINT32_C(1000)

/** 1 hour expressed in microseconds - 64-bit type. */
#define RT_US_1HOUR_64          UINT64_C(3600000000)
/** 1 minute expressed in microseconds - 64-bit type. */
#define RT_US_1MIN_64           UINT64_C(60000000)
/** 1 second expressed in microseconds - 64-bit type. */
#define RT_US_1SEC_64           UINT64_C(1000000)
/** 100 millsecond expressed in microseconds - 64-bit type. */
#define RT_US_100MS_64          UINT64_C(100000)
/** 10 millsecond expressed in microseconds - 64-bit type. */
#define RT_US_10MS_64           UINT64_C(10000)
/** 1 millsecond expressed in microseconds - 64-bit type. */
#define RT_US_1MS_64            UINT64_C(1000)

/** 1 hour expressed in milliseconds. */
#define RT_MS_1HOUR             UINT32_C(3600000)
/** 1 minute expressed in milliseconds. */
#define RT_MS_1MIN              UINT32_C(60000)
/** 1 second expressed in milliseconds. */
#define RT_MS_1SEC              UINT32_C(1000)

/** 1 hour expressed in milliseconds - 64-bit type. */
#define RT_MS_1HOUR_64          UINT64_C(3600000)
/** 1 minute expressed in milliseconds - 64-bit type. */
#define RT_MS_1MIN_64           UINT64_C(60000)
/** 1 second expressed in milliseconds - 64-bit type. */
#define RT_MS_1SEC_64           UINT64_C(1000)

/** The number of seconds per week. */
#define RT_SEC_1WEEK            UINT32_C(604800)
/** The number of seconds per day. */
#define RT_SEC_1DAY             UINT32_C(86400)
/** The number of seconds per hour. */
#define RT_SEC_1HOUR            UINT32_C(3600)

/** The number of seconds per week - 64-bit type. */
#define RT_SEC_1WEEK_64         UINT64_C(604800)
/** The number of seconds per day - 64-bit type. */
#define RT_SEC_1DAY_64          UINT64_C(86400)
/** The number of seconds per hour - 64-bit type. */
#define RT_SEC_1HOUR_64         UINT64_C(3600)
/** @}  */


/** @defgroup grp_rt_cdefs_dbgtype  Debug Info Types
 * @{ */
/** Other format. */
#define RT_DBGTYPE_OTHER        RT_BIT_32(0)
/** Stabs. */
#define RT_DBGTYPE_STABS        RT_BIT_32(1)
/** Debug With Arbitrary Record Format (DWARF). */
#define RT_DBGTYPE_DWARF        RT_BIT_32(2)
/** Microsoft Codeview debug info. */
#define RT_DBGTYPE_CODEVIEW     RT_BIT_32(3)
/** Watcom debug info. */
#define RT_DBGTYPE_WATCOM       RT_BIT_32(4)
/** IBM High Level Language debug info. */
#define RT_DBGTYPE_HLL          RT_BIT_32(5)
/** Old OS/2 and Windows symbol file. */
#define RT_DBGTYPE_SYM          RT_BIT_32(6)
/** Map file. */
#define RT_DBGTYPE_MAP          RT_BIT_32(7)
/** @} */


/** @defgroup grp_rt_cdefs_exetype  Executable Image Types
 * @{ */
/** Some other format. */
#define RT_EXETYPE_OTHER        RT_BIT_32(0)
/** Portable Executable. */
#define RT_EXETYPE_PE           RT_BIT_32(1)
/** Linear eXecutable. */
#define RT_EXETYPE_LX           RT_BIT_32(2)
/** Linear Executable. */
#define RT_EXETYPE_LE           RT_BIT_32(3)
/** New Executable. */
#define RT_EXETYPE_NE           RT_BIT_32(4)
/** DOS Executable (Mark Zbikowski). */
#define RT_EXETYPE_MZ           RT_BIT_32(5)
/** COM Executable. */
#define RT_EXETYPE_COM          RT_BIT_32(6)
/** a.out Executable. */
#define RT_EXETYPE_AOUT         RT_BIT_32(7)
/** Executable and Linkable Format. */
#define RT_EXETYPE_ELF          RT_BIT_32(8)
/** Mach-O Executable (including FAT ones). */
#define RT_EXETYPE_MACHO        RT_BIT_32(9)
/** TE from UEFI. */
#define RT_EXETYPE_TE           RT_BIT_32(9)
/** @} */


/** @def VALID_PTR
 * Pointer validation macro.
 * @param   ptr         The pointer.
 */
#if defined(RT_ARCH_AMD64)
# ifdef IN_RING3
#  if defined(RT_OS_DARWIN) /* first 4GB is reserved for legacy kernel. */
#   define RT_VALID_PTR(ptr)    (   (uintptr_t)(ptr) >= _4G \
                                 && !((uintptr_t)(ptr) & 0xffff800000000000ULL) )
#  elif defined(RT_OS_SOLARIS) /* The kernel only used the top 2TB, but keep it simple. */
#   define RT_VALID_PTR(ptr)    (   (uintptr_t)(ptr) + 0x1000U >= 0x2000U \
                                 && (   ((uintptr_t)(ptr) & 0xffff800000000000ULL) == 0xffff800000000000ULL \
                                     || ((uintptr_t)(ptr) & 0xffff800000000000ULL) == 0) )
#  else
#   define RT_VALID_PTR(ptr)    (   (uintptr_t)(ptr) + 0x1000U >= 0x2000U \
                                 && !((uintptr_t)(ptr) & 0xffff800000000000ULL) )
#  endif
# else /* !IN_RING3 */
#  define RT_VALID_PTR(ptr)     (   (uintptr_t)(ptr) + 0x1000U >= 0x2000U \
                                 && (   ((uintptr_t)(ptr) & 0xffff800000000000ULL) == 0xffff800000000000ULL \
                                     || ((uintptr_t)(ptr) & 0xffff800000000000ULL) == 0) )
# endif /* !IN_RING3 */

#elif defined(RT_ARCH_X86)
# define RT_VALID_PTR(ptr)      ( (uintptr_t)(ptr) + 0x1000U >= 0x2000U )

#elif defined(RT_ARCH_SPARC64)
# ifdef IN_RING3
#  if defined(RT_OS_SOLARIS)
/** Sparc64 user mode: According to Figure 9.4 in solaris internals */
/** @todo #   define RT_VALID_PTR(ptr)    ( (uintptr_t)(ptr) + 0x80004000U >= 0x80004000U + 0x100000000ULL ) - figure this. */
#   define RT_VALID_PTR(ptr)    ( (uintptr_t)(ptr) + 0x80000000U >= 0x80000000U + 0x100000000ULL )
#  else
#   error "Port me"
#  endif
# else  /* !IN_RING3 */
#  if defined(RT_OS_SOLARIS)
/** @todo Sparc64 kernel mode: This is according to Figure 11.1 in solaris
 *        internals. Verify in sources. */
#   define RT_VALID_PTR(ptr)    ( (uintptr_t)(ptr) >= 0x01000000U )
#  else
#   error "Port me"
#  endif
# endif /* !IN_RING3 */

#elif defined(RT_ARCH_SPARC)
# ifdef IN_RING3
#  ifdef RT_OS_SOLARIS
/** Sparc user mode: According to
 * http://cvs.opensolaris.org/source/xref/onnv/onnv-gate/usr/src/uts/sun4/os/startup.c#510 */
#   define RT_VALID_PTR(ptr)    ( (uintptr_t)(ptr) + 0x400000U >= 0x400000U + 0x2000U )

#  else
#   error "Port me"
#  endif
# else  /* !IN_RING3 */
#  ifdef RT_OS_SOLARIS
/** @todo Sparc kernel mode: Check the sources! */
#   define RT_VALID_PTR(ptr)    ( (uintptr_t)(ptr) + 0x1000U >= 0x2000U )
#  else
#   error "Port me"
#  endif
# endif /* !IN_RING3 */

#elif defined(RT_ARCH_ARM)
/* ASSUMES that at least the last and first 4K are out of bounds. */
# define RT_VALID_PTR(ptr)      ( (uintptr_t)(ptr) + 0x1000U >= 0x2000U )

#else
# error "Architecture identifier missing / not implemented."
#endif

/** Old name for RT_VALID_PTR.  */
#define VALID_PTR(ptr)          RT_VALID_PTR(ptr)

/** @def RT_VALID_ALIGNED_PTR
 * Pointer validation macro that also checks the alignment.
 * @param   ptr         The pointer.
 * @param   align       The alignment, must be a power of two.
 */
#define RT_VALID_ALIGNED_PTR(ptr, align)   \
    (   !((uintptr_t)(ptr) & (uintptr_t)((align) - 1)) \
     && VALID_PTR(ptr) )


/** @def VALID_PHYS32
 * 32 bits physical address validation macro.
 * @param   Phys          The RTGCPHYS address.
 */
#define VALID_PHYS32(Phys)  ( (uint64_t)(Phys) < (uint64_t)_4G )

/** @def N_
 * The \#define N_ is used to mark a string for translation. This is usable in
 * any part of the code, as it is only used by the tools that create message
 * catalogs. This macro is a no-op as far as the compiler and code generation
 * is concerned.
 *
 * If you want to both mark a string for translation and translate it, use _().
 */
#define N_(s) (s)

/** @def _
 * The \#define _ is used to mark a string for translation and to translate it
 * in one step.
 *
 * If you want to only mark a string for translation, use N_().
 */
#define _(s) gettext(s)


/** @def __PRETTY_FUNCTION__
 *  With GNU C we'd like to use the builtin __PRETTY_FUNCTION__, so define that
 *  for the other compilers.
 */
#if !defined(__GNUC__) && !defined(__PRETTY_FUNCTION__)
# ifdef _MSC_VER
#  define __PRETTY_FUNCTION__    __FUNCSIG__
# else
#  define __PRETTY_FUNCTION__    __FUNCTION__
# endif
#endif


/** @def RT_STRICT
 * The \#define RT_STRICT controls whether or not assertions and other runtime
 * checks should be compiled in or not.  This is defined when DEBUG is defined.
 * If RT_NO_STRICT is defined, it will unconditionally be undefined.
 *
 * If you want assertions which are not subject to compile time options use
 * the AssertRelease*() flavors.
 */
#if !defined(RT_STRICT) && defined(DEBUG)
# define RT_STRICT
#endif
#ifdef RT_NO_STRICT
# undef RT_STRICT
#endif

/** @todo remove this: */
#if !defined(RT_LOCK_STRICT) && !defined(DEBUG_bird)
# define RT_LOCK_NO_STRICT
#endif
#if !defined(RT_LOCK_STRICT_ORDER) && !defined(DEBUG_bird)
# define RT_LOCK_NO_STRICT_ORDER
#endif

/** @def RT_LOCK_STRICT
 * The \#define RT_LOCK_STRICT controls whether deadlock detection and related
 * checks are done in the lock and semaphore code.  It is by default enabled in
 * RT_STRICT builds, but this behavior can be overridden by defining
 * RT_LOCK_NO_STRICT. */
#if !defined(RT_LOCK_STRICT) && !defined(RT_LOCK_NO_STRICT) && defined(RT_STRICT)
# define RT_LOCK_STRICT
#endif
/** @def RT_LOCK_NO_STRICT
 * The \#define RT_LOCK_NO_STRICT disables RT_LOCK_STRICT.  */
#if defined(RT_LOCK_NO_STRICT) && defined(RT_LOCK_STRICT)
# undef RT_LOCK_STRICT
#endif

/** @def RT_LOCK_STRICT_ORDER
 * The \#define RT_LOCK_STRICT_ORDER controls whether locking order is checked
 * by the lock and semaphore code.  It is by default enabled in RT_STRICT
 * builds, but this behavior can be overridden by defining
 * RT_LOCK_NO_STRICT_ORDER. */
#if !defined(RT_LOCK_STRICT_ORDER) && !defined(RT_LOCK_NO_STRICT_ORDER) && defined(RT_STRICT)
# define RT_LOCK_STRICT_ORDER
#endif
/** @def RT_LOCK_NO_STRICT_ORDER
 * The \#define RT_LOCK_NO_STRICT_ORDER disables RT_LOCK_STRICT_ORDER.  */
#if defined(RT_LOCK_NO_STRICT_ORDER) && defined(RT_LOCK_STRICT_ORDER)
# undef RT_LOCK_STRICT_ORDER
#endif


/** Source position. */
#define RT_SRC_POS         __FILE__, __LINE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__

/** Source position declaration. */
#define RT_SRC_POS_DECL    const char *pszFile, unsigned iLine, const char *pszFunction

/** Source position arguments. */
#define RT_SRC_POS_ARGS    pszFile, iLine, pszFunction

/** Applies NOREF() to the source position arguments. */
#define RT_SRC_POS_NOREF() do { NOREF(pszFile); NOREF(iLine); NOREF(pszFunction); } while (0)


/** @def RT_INLINE_ASM_EXTERNAL
 * Defined as 1 if the compiler does not support inline assembly.
 * The ASM* functions will then be implemented in external .asm files.
 */
#if (defined(_MSC_VER) && defined(RT_ARCH_AMD64)) \
 || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
# define RT_INLINE_ASM_EXTERNAL 1
#else
# define RT_INLINE_ASM_EXTERNAL 0
#endif

/** @def RT_INLINE_ASM_GNU_STYLE
 * Defined as 1 if the compiler understands GNU style inline assembly.
 */
#if defined(_MSC_VER)
# define RT_INLINE_ASM_GNU_STYLE 0
#else
# define RT_INLINE_ASM_GNU_STYLE 1
#endif

/** @def RT_INLINE_ASM_USES_INTRIN
 * Defined as the major MSC version if the compiler have and uses intrin.h.
 * Otherwise it is 0. */
#ifdef _MSC_VER
# if   _MSC_VER >= 1700 /* Visual C++ v11.0 / 2012 */
#  define RT_INLINE_ASM_USES_INTRIN 17
# elif _MSC_VER >= 1600 /* Visual C++ v10.0 / 2010 */
#  define RT_INLINE_ASM_USES_INTRIN 16
# elif _MSC_VER >= 1500 /* Visual C++ v9.0 / 2008 */
#  define RT_INLINE_ASM_USES_INTRIN 15
# elif _MSC_VER >= 1400 /* Visual C++ v8.0 / 2005 */
#  define RT_INLINE_ASM_USES_INTRIN 14
# endif
#endif
#ifndef RT_INLINE_ASM_USES_INTRIN
# define RT_INLINE_ASM_USES_INTRIN 0
#endif

/** @def RT_COMPILER_SUPPORTS_LAMBDA
 * If the defined, the compiler supports lambda expressions.   These expressions
 * are useful for embedding assertions and type checks into macros. */
#if defined(_MSC_VER) && defined(__cplusplus)
# if _MSC_VER >= 1600 /* Visual C++ v10.0 / 2010 */
#  define RT_COMPILER_SUPPORTS_LAMBDA
# endif
#elif defined(__GNUC__) && defined(__cplusplus)
/* 4.5 or later, I think, if in ++11 mode... */
#endif

/** @} */


/** @defgroup grp_rt_cdefs_cpp  Special Macros for C++
 * @ingroup grp_rt_cdefs
 * @{
 */

#ifdef __cplusplus

/** @def DECLEXPORT_CLASS
 * How to declare an exported class. Place this macro after the 'class'
 * keyword in the declaration of every class you want to export.
 *
 * @note It is necessary to use this macro even for inner classes declared
 * inside the already exported classes. This is a GCC specific requirement,
 * but it seems not to harm other compilers.
 */
#if defined(_MSC_VER) || defined(RT_OS_OS2)
# define DECLEXPORT_CLASS       __declspec(dllexport)
#elif defined(RT_USE_VISIBILITY_DEFAULT)
# define DECLEXPORT_CLASS       __attribute__((visibility("default")))
#else
# define DECLEXPORT_CLASS
#endif

/** @def DECLIMPORT_CLASS
 * How to declare an imported class Place this macro after the 'class'
 * keyword in the declaration of every class you want to export.
 *
 * @note It is necessary to use this macro even for inner classes declared
 * inside the already exported classes. This is a GCC specific requirement,
 * but it seems not to harm other compilers.
 */
#if defined(_MSC_VER) || (defined(RT_OS_OS2) && !defined(__IBMC__) && !defined(__IBMCPP__))
# define DECLIMPORT_CLASS       __declspec(dllimport)
#elif defined(RT_USE_VISIBILITY_DEFAULT)
# define DECLIMPORT_CLASS       __attribute__((visibility("default")))
#else
# define DECLIMPORT_CLASS
#endif

/** @def WORKAROUND_MSVC7_ERROR_C2593_FOR_BOOL_OP
 * Macro to work around error C2593 of the not-so-smart MSVC 7.x ambiguity
 * resolver. The following snippet clearly demonstrates the code causing this
 * error:
 * @code
 *      class A
 *      {
 *      public:
 *          operator bool() const { return false; }
 *          operator int*() const { return NULL; }
 *      };
 *      int main()
 *      {
 *          A a;
 *          if (!a);
 *          if (a && 0);
 *          return 0;
 *      }
 * @endcode
 * The code itself seems pretty valid to me and GCC thinks the same.
 *
 * This macro fixes the compiler error by explicitly overloading implicit
 * global operators !, && and || that take the given class instance as one of
 * their arguments.
 *
 * The best is to use this macro right after the class declaration.
 *
 * @note The macro expands to nothing for compilers other than MSVC.
 *
 * @param Cls Class to apply the workaround to
 */
#if defined(_MSC_VER)
# define WORKAROUND_MSVC7_ERROR_C2593_FOR_BOOL_OP(Cls) \
    inline bool operator! (const Cls &that) { return !bool (that); } \
    inline bool operator&& (const Cls &that, bool b) { return bool (that) && b; } \
    inline bool operator|| (const Cls &that, bool b) { return bool (that) || b; } \
    inline bool operator&& (bool b, const Cls &that) { return b && bool (that); } \
    inline bool operator|| (bool b, const Cls &that) { return b || bool (that); }
#else
# define WORKAROUND_MSVC7_ERROR_C2593_FOR_BOOL_OP(Cls)
#endif

/** @def WORKAROUND_MSVC7_ERROR_C2593_FOR_BOOL_OP_TPL
 * Version of WORKAROUND_MSVC7_ERROR_C2593_FOR_BOOL_OP for template classes.
 *
 * @param Tpl       Name of the template class to apply the workaround to
 * @param ArgsDecl  arguments of the template, as declared in |<>| after the
 *                  |template| keyword, including |<>|
 * @param Args      arguments of the template, as specified in |<>| after the
 *                  template class name when using the, including |<>|
 *
 * Example:
 * @code
 *      // template class declaration
 *      template <class C>
 *      class Foo { ... };
 *      // applied workaround
 *      WORKAROUND_MSVC7_ERROR_C2593_FOR_BOOL_OP_TPL (Foo, <class C>, <C>)
 * @endcode
 */
#if defined(_MSC_VER)
# define WORKAROUND_MSVC7_ERROR_C2593_FOR_BOOL_OP_TPL(Tpl, ArgsDecl, Args) \
    template ArgsDecl \
    inline bool operator! (const Tpl Args &that) { return !bool (that); } \
    template ArgsDecl \
    inline bool operator&& (const Tpl Args  &that, bool b) { return bool (that) && b; } \
    template ArgsDecl \
    inline bool operator|| (const Tpl Args  &that, bool b) { return bool (that) || b; } \
    template ArgsDecl \
    inline bool operator&& (bool b, const Tpl Args  &that) { return b && bool (that); } \
    template ArgsDecl \
    inline bool operator|| (bool b, const Tpl Args  &that) { return b || bool (that); }
#else
# define WORKAROUND_MSVC7_ERROR_C2593_FOR_BOOL_OP_TPL(Tpl, ArgsDecl, Args)
#endif


/** @def DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP
 * Declares the copy constructor and the assignment operation as inlined no-ops
 * (non-existent functions) for the given class. Use this macro inside the
 * private section if you want to effectively disable these operations for your
 * class.
 *
 * @param      Cls     class name to declare for
 */

#define DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(Cls) \
    inline Cls (const Cls &); \
    inline Cls &operator= (const Cls &);


/** @def DECLARE_CLS_NEW_DELETE_NOOP
 * Declares the new and delete operations as no-ops (non-existent functions)
 * for the given class. Use this macro inside the private section if you want
 * to effectively limit creating class instances on the stack only.
 *
 * @note The destructor of the given class must not be virtual, otherwise a
 * compile time error will occur. Note that this is not a drawback: having
 * the virtual destructor for a stack-based class is absolutely useless
 * (the real class of the stack-based instance is always known to the compiler
 * at compile time, so it will always call the correct destructor).
 *
 * @param      Cls     class name to declare for
 */
#define DECLARE_CLS_NEW_DELETE_NOOP(Cls) \
    inline static void *operator new (size_t); \
    inline static void operator delete (void *);

#endif /* __cplusplus */

/** @} */

#endif

