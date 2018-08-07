/* $Id: initterm-r0drv-linux.c 36555 2011-04-05 12:34:09Z vboxsync $ */
/** @file
 * IPRT - Initialization & Termination, R0 Driver, Linux.
 */

/*
 * Copyright (C) 2006-2007 Oracle Corporation
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "the-linux-kernel.h"
#include "internal/iprt.h"
#include <iprt/err.h>
#include <iprt/assert.h>
#include "internal/initterm.h"


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
#ifdef RT_ARCH_AMD64
/* in alloc-r0drv0-linux.c */
DECLHIDDEN(void) rtR0MemExecCleanup(void);
#endif


DECLHIDDEN(int) rtR0InitNative(void)
{
    return VINF_SUCCESS;
}


DECLHIDDEN(void) rtR0TermNative(void)
{
#ifdef RT_ARCH_AMD64
    rtR0MemExecCleanup();
#endif
}

