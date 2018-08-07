/* $Id: VBoxDispD3DCmn.h 36867 2011-04-28 07:27:03Z vboxsync $ */

/** @file
 * VBoxVideo Display D3D User mode dll
 */

/*
 * Copyright (C) 2011 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxDispD3DCmn_h___
#define ___VBoxDispD3DCmn_h___

#include <windows.h>
#include <d3d9types.h>
//#include <d3dtypes.h>
#include <D3dumddi.h>
#include <d3dhal.h>

#include <iprt/initterm.h>
#include <iprt/log.h>
#include <iprt/mem.h>

#include <VBox/Log.h>

#include <VBox/VBoxGuestLib.h>

#include "VBoxDispDbg.h"
#include "VBoxDispD3DIf.h"
#include "common/wddm/VBoxMPIf.h"
#include "VBoxDispCm.h"
#include "VBoxDispKmt.h"
#ifdef VBOX_WITH_CRHGSMI
#include "VBoxUhgsmiBase.h"
#include "VBoxUhgsmiDisp.h"
#include "VBoxUhgsmiKmt.h"
#endif
#include "VBoxDispD3D.h"


# ifdef VBOXWDDMDISP
#  define VBOXWDDMDISP_DECL(_type) DECLEXPORT(_type)
# else
#  define VBOXWDDMDISP_DECL(_type) DECLIMPORT(_type)
# endif

#endif /* #ifndef ___VBoxDispD3DCmn_h___ */
