/** @file
 * VBoxGuest - Mangling of IPRT symbols for guest drivers.
 *
 * This is included via a compiler directive on platforms with a global kernel
 * symbol name space (i.e. not Windows, OS/2 and Mac OS X (?)).
 */

/*
 * Copyright (C) 2011-2019 Oracle Corporation
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

#ifndef VBOX_INCLUDED_VBoxGuestMangling_h
#define VBOX_INCLUDED_VBoxGuestMangling_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#define RT_MANGLER(symbol)   VBoxGuest_##symbol
#include <iprt/mangling.h>

#endif /* !VBOX_INCLUDED_VBoxGuestMangling_h */

