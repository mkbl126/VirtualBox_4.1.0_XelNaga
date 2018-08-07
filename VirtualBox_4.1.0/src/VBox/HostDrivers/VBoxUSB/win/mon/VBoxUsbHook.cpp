/* $Id: VBoxUsbHook.cpp 37083 2011-05-13 19:24:39Z vboxsync $ */
/** @file
 * Driver Dispatch Table Hooking API
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
#include "VBoxUsbMon.h"

#define VBOXUSBHOOK_MEMTAG 'HUBV'

NTSTATUS VBoxUsbHookInstall(PVBOXUSBHOOK_ENTRY pHook)
{
    KIRQL Irql;
    KeAcquireSpinLock(&pHook->Lock, &Irql);
    if (pHook->fIsInstalled)
    {
        AssertFailed();
        KeReleaseSpinLock(&pHook->Lock, Irql);
        return STATUS_UNSUCCESSFUL;
    }

    pHook->pfnOldHandler = (PDRIVER_DISPATCH)InterlockedExchangePointer((PVOID*)&pHook->pDrvObj->MajorFunction[pHook->iMjFunction], pHook->pfnHook);
    Assert(pHook->pfnOldHandler);
    Assert(pHook->pfnHook != pHook->pfnOldHandler);
    pHook->fIsInstalled = TRUE;
    KeReleaseSpinLock(&pHook->Lock, Irql);
    return STATUS_SUCCESS;

}
NTSTATUS VBoxUsbHookUninstall(PVBOXUSBHOOK_ENTRY pHook)
{
    KIRQL Irql;
    KeAcquireSpinLock(&pHook->Lock, &Irql);
    if (!pHook->fIsInstalled)
    {
        KeReleaseSpinLock(&pHook->Lock, Irql);
        return STATUS_SUCCESS;
    }

    PDRIVER_DISPATCH pfnOldVal = (PDRIVER_DISPATCH)InterlockedCompareExchangePointer((PVOID*)&pHook->pDrvObj->MajorFunction[pHook->iMjFunction], pHook->pfnOldHandler, pHook->pfnHook);
    Assert(pfnOldVal == pHook->pfnHook);
    if (pfnOldVal != pHook->pfnHook)
    {
        AssertMsgFailed(("unhook failed!!!\n"));
        /* this is bad! this could happen if someone else has chained another hook,
         * or (which is even worse) restored the "initial" entry value it saved when doing a hooking before us
         * return the failure and don't do anything else
         * the best thing to do if this happens is to leave everything as is
         * and to prevent the driver from being unloaded to ensure no one references our unloaded hook routine */
        KeReleaseSpinLock(&pHook->Lock, Irql);
        return STATUS_UNSUCCESSFUL;
    }

    pHook->fIsInstalled = FALSE;
    KeReleaseSpinLock(&pHook->Lock, Irql);

    /* wait for the current handlers to exit */
    VBoxDrvToolRefWaitEqual(&pHook->HookRef, 1);

    return STATUS_SUCCESS;
}

BOOLEAN VBoxUsbHookIsInstalled(PVBOXUSBHOOK_ENTRY pHook)
{
    KIRQL Irql;
    BOOLEAN fIsInstalled;
    KeAcquireSpinLock(&pHook->Lock, &Irql);
    fIsInstalled = pHook->fIsInstalled;
    KeReleaseSpinLock(&pHook->Lock, Irql);
    return fIsInstalled;
}

VOID VBoxUsbHookInit(PVBOXUSBHOOK_ENTRY pHook, PDRIVER_OBJECT pDrvObj, UCHAR iMjFunction, PDRIVER_DISPATCH pfnHook)
{
    Assert(pDrvObj);
    Assert(iMjFunction <= IRP_MJ_MAXIMUM_FUNCTION);
    Assert(pfnHook);
    memset(pHook, 0, sizeof (*pHook));
    InitializeListHead(&pHook->RequestList);
    KeInitializeSpinLock(&pHook->Lock);
    VBoxDrvToolRefInit(&pHook->HookRef);
    pHook->pDrvObj = pDrvObj;
    pHook->iMjFunction = iMjFunction;
    pHook->pfnHook = pfnHook;
    Assert(!pHook->pfnOldHandler);
    Assert(!pHook->fIsInstalled);

}

static void vboxUsbHookRequestRegisterCompletion(PVBOXUSBHOOK_ENTRY pHook, PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_COMPLETION_ROUTINE pfnCompletion, PVBOXUSBHOOK_REQUEST pRequest)
{
    Assert(pfnCompletion);
    Assert(pRequest);
    Assert(pDevObj);
    Assert(pIrp);
    PIO_STACK_LOCATION pSl = IoGetCurrentIrpStackLocation(pIrp);
    memset(pRequest, 0, sizeof (*pRequest));
    pRequest->pHook = pHook;
    pRequest->OldLocation = *pSl;
    pRequest->pDevObj = pDevObj;
    pRequest->pIrp = pIrp;
    pRequest->bCompletionStopped = FALSE;
    pSl->CompletionRoutine = pfnCompletion;
    pSl->Context = pRequest;
    pSl->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

    KIRQL oldIrql;
    KeAcquireSpinLock(&pHook->Lock, &oldIrql);
    InsertTailList(&pHook->RequestList, &pRequest->ListEntry);
    KeReleaseSpinLock(&pHook->Lock, oldIrql);
}

NTSTATUS VBoxUsbHookRequestPassDownHookCompletion(PVBOXUSBHOOK_ENTRY pHook, PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_COMPLETION_ROUTINE pfnCompletion, PVBOXUSBHOOK_REQUEST pRequest)
{
    Assert(pfnCompletion);
    vboxUsbHookRequestRegisterCompletion(pHook, pDevObj, pIrp, pfnCompletion, pRequest);
    return pHook->pfnOldHandler(pDevObj, pIrp);
}

NTSTATUS VBoxUsbHookRequestPassDownHookSkip(PVBOXUSBHOOK_ENTRY pHook, PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    return pHook->pfnOldHandler(pDevObj, pIrp);
}

NTSTATUS VBoxUsbHookRequestMoreProcessingRequired(PVBOXUSBHOOK_ENTRY pHook, PDEVICE_OBJECT pDevObj, PIRP pIrp, PVBOXUSBHOOK_REQUEST pRequest)
{
    Assert(!pRequest->bCompletionStopped);
    pRequest->bCompletionStopped = TRUE;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS VBoxUsbHookRequestComplete(PVBOXUSBHOOK_ENTRY pHook, PDEVICE_OBJECT pDevObj, PIRP pIrp, PVBOXUSBHOOK_REQUEST pRequest)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (pRequest->OldLocation.CompletionRoutine && pRequest->OldLocation.Control)
    {
        Status = pRequest->OldLocation.CompletionRoutine(pDevObj, pIrp, pRequest->OldLocation.Context);
    }

    if (Status != STATUS_MORE_PROCESSING_REQUIRED)
    {
        if (pRequest->bCompletionStopped)
        {
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        }
    }
    /*
     * else - in case driver returned STATUS_MORE_PROCESSING_REQUIRED,
     * it will call IoCompleteRequest itself
     */

    KIRQL oldIrql;
    KeAcquireSpinLock(&pHook->Lock, &oldIrql);
    RemoveEntryList(&pRequest->ListEntry);
    KeReleaseSpinLock(&pHook->Lock, oldIrql);
    return Status;
}

#define PVBOXUSBHOOK_REQUEST_FROM_LE(_pLe) ( (PVBOXUSBHOOK_REQUEST)( ((uint8_t*)(_pLe)) - RT_OFFSETOF(VBOXUSBHOOK_REQUEST, ListEntry) ) )

VOID VBoxUsbHookVerifyCompletion(PVBOXUSBHOOK_ENTRY pHook, PVBOXUSBHOOK_REQUEST pRequest, PIRP pIrp)
{
    KIRQL oldIrql;
    KeAcquireSpinLock(&pHook->Lock, &oldIrql);
    for (PLIST_ENTRY pLe = pHook->RequestList.Flink; pLe != &pHook->RequestList; pLe = pLe->Flink)
    {
        PVBOXUSBHOOK_REQUEST pCur = PVBOXUSBHOOK_REQUEST_FROM_LE(pLe);
        if (pCur != pRequest)
            continue;
        if (pCur->pIrp != pIrp)
            continue;
        AssertFailed();
    }
    KeReleaseSpinLock(&pHook->Lock, oldIrql);

}