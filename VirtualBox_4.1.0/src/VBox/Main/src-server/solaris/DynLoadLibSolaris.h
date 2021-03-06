/* $Id: DynLoadLibSolaris.h 31340 2010-08-03 14:29:43Z vboxsync $ */
/** @file
 * Dynamically loaded libraries for Solaris hosts, Internal header.
 */

/*
 * Copyright (C) 2008 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_DYN_LOAD_LIB_SOLARIS
#define ____H_DYN_LOAD_LIB_SOLARIS

#define LIB_DLPI "libdlpi.so.1"
#ifdef RT_OS_SOLARIS_10
#include <sys/dlpi.h>
#else
#include <libdlpi.h>
#endif

typedef boolean_t dlpi_walkfunc_t(const char*, void *);

extern int (*g_pfnLibDlpiWalk)(dlpi_walkfunc_t *, void *, uint_t);

extern bool VBoxSolarisLibDlpiFound(void);

#endif /*  ____H_DYN_LOAD_LIB_SOLARIS */

