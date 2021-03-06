/* $Id: VSCSILun.cpp 33540 2010-10-28 09:27:05Z vboxsync $ */
/** @file
 * Virtual SCSI driver: LUN handling
 */

/*
 * Copyright (C) 2006-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#define LOG_GROUP LOG_GROUP_VSCSI
#include <VBox/log.h>
#include <VBox/err.h>
#include <VBox/types.h>
#include <VBox/vscsi.h>
#include <iprt/assert.h>
#include <iprt/mem.h>

#include "VSCSIInternal.h"

/** SBC descriptor */
extern VSCSILUNDESC g_VScsiLunTypeSbc;
/** MMC descriptor */
//extern PVSCSILUNDESC g_pVScsiLunTypeMmc;

/**
 * Array of supported SCSI LUN types.
 */
static PVSCSILUNDESC g_aVScsiLunTypesSupported[] =
{
    &g_VScsiLunTypeSbc
};

VBOXDDU_DECL(int) VSCSILunCreate(PVSCSILUN phVScsiLun, VSCSILUNTYPE enmLunType,
                                 PVSCSILUNIOCALLBACKS pVScsiLunIoCallbacks,
                                 void *pvVScsiLunUser)
{
    PVSCSILUNINT  pVScsiLun     = NULL;
    PVSCSILUNDESC pVScsiLunDesc = NULL;

    AssertPtrReturn(phVScsiLun, VERR_INVALID_POINTER);
    AssertReturn(   enmLunType > VSCSILUNTYPE_INVALID
                 && enmLunType < VSCSILUNTYPE_LAST, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pVScsiLunIoCallbacks, VERR_INVALID_PARAMETER);

    for (unsigned idxLunType = 0; idxLunType < RT_ELEMENTS(g_aVScsiLunTypesSupported); idxLunType++)
    {
        if (g_aVScsiLunTypesSupported[idxLunType]->enmLunType == enmLunType)
        {
            pVScsiLunDesc = g_aVScsiLunTypesSupported[idxLunType];
            break;
        }
    }

    if (!pVScsiLunDesc)
        return VERR_VSCSI_LUN_TYPE_NOT_SUPPORTED;

    pVScsiLun = (PVSCSILUNINT)RTMemAllocZ(pVScsiLunDesc->cbLun);
    if (!pVScsiLun)
        return VERR_NO_MEMORY;

    pVScsiLun->pVScsiDevice         = NULL;
    pVScsiLun->pvVScsiLunUser       = pvVScsiLunUser;
    pVScsiLun->pVScsiLunIoCallbacks = pVScsiLunIoCallbacks;
    pVScsiLun->pVScsiLunDesc        = pVScsiLunDesc;

    int rc = pVScsiLunDesc->pfnVScsiLunInit(pVScsiLun);
    if (RT_SUCCESS(rc))
    {
        /** @todo Init other stuff */
        *phVScsiLun = pVScsiLun;
        return VINF_SUCCESS;
    }

    RTMemFree(pVScsiLun);

    return rc;
}

/**
 * Destroy virtual SCSI LUN.
 *
 * @returns VBox status code.
 * @param   hVScsiLun               The virtual SCSI LUN handle to destroy.
 */
VBOXDDU_DECL(int) VSCSILunDestroy(VSCSILUN hVScsiLun)
{
    PVSCSILUNINT pVScsiLun = (PVSCSILUNINT)hVScsiLun;

    AssertPtrReturn(pVScsiLun, VERR_INVALID_HANDLE);
    AssertReturn(!pVScsiLun->pVScsiDevice, VERR_VSCSI_LUN_ATTACHED_TO_DEVICE);
    AssertReturn(vscsiIoReqOutstandingCountGet(pVScsiLun) == 0, VERR_VSCSI_LUN_BUSY);

    int rc = pVScsiLun->pVScsiLunDesc->pfnVScsiLunDestroy(pVScsiLun);
    if (RT_FAILURE(rc))
        return rc;

    /* Make LUN invalid */
    pVScsiLun->pvVScsiLunUser       = NULL;
    pVScsiLun->pVScsiLunIoCallbacks = NULL;
    pVScsiLun->pVScsiLunDesc        = NULL;

    RTMemFree(pVScsiLun);

    return VINF_SUCCESS;
}

