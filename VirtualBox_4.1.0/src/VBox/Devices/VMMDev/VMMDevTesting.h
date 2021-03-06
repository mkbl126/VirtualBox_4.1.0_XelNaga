/* $Id: VMMDevTesting.h 30724 2010-07-08 08:30:20Z vboxsync $ */
/** @file
 * VMMDev - Guest <-> VMM/Host communication device, internal header.
 */

/*
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


#ifndef ___VMMDev_VMMDevTesting_h
#define ___VMMDev_VMMDevTesting_h

#include <VBox/types.h>
#include <VBox/VMMDevTesting.h>

RT_C_DECLS_BEGIN

int vmmdevTestingInitialize(PPDMDEVINS pDevIns);

RT_C_DECLS_END

#endif

