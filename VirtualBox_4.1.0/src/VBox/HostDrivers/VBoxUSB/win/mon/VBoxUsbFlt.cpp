/* $Id: VBoxUsbFlt.cpp 37047 2011-05-12 10:29:26Z vboxsync $ */
/** @file
 * VBox USB Monitor Device Filtering functionality
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "VBoxUsbMon.h"
#include "../cmn/VBoxUsbTool.h"

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <iprt/process.h>
#include <iprt/assert.h>
#include <VBox/err.h>
//#include <VBox/sup.h>

#include <iprt/assert.h>
#include <stdio.h>

#pragma warning(disable : 4200)
#include "usbdi.h"
#pragma warning(default : 4200)
#include "usbdlib.h"
#include "VBoxUSBFilterMgr.h"
#include <VBox/usblib.h>
#include <devguid.h>

/*
 * Note: Must match the VID & PID in the USB driver .inf file!!
 */
/*
  BusQueryDeviceID USB\Vid_80EE&Pid_CAFE
  BusQueryInstanceID 2
  BusQueryHardwareIDs USB\Vid_80EE&Pid_CAFE&Rev_0100
  BusQueryHardwareIDs USB\Vid_80EE&Pid_CAFE
  BusQueryCompatibleIDs USB\Class_ff&SubClass_00&Prot_00
  BusQueryCompatibleIDs USB\Class_ff&SubClass_00
  BusQueryCompatibleIDs USB\Class_ff
*/

#define szBusQueryDeviceId                  L"USB\\Vid_80EE&Pid_CAFE"
#define szBusQueryHardwareIDs               L"USB\\Vid_80EE&Pid_CAFE&Rev_0100\0USB\\Vid_80EE&Pid_CAFE\0\0"
#define szBusQueryCompatibleIDs             L"USB\\Class_ff&SubClass_00&Prot_00\0USB\\Class_ff&SubClass_00\0USB\\Class_ff\0\0"

#define szDeviceTextDescription             L"VirtualBox USB"

/* Possible USB bus driver names. */
static LPWSTR lpszStandardControllerName[1] =
{
    L"\\Driver\\usbhub",
};

/*
 * state transitions:
 *
 *           (we are not filtering this device )
 * ADDED --> UNCAPTURED ------------------------------->-
 *       |                                              |
 *       |   (we are filtering this device,             | (the device is being
 *       |    waiting for our device driver             |  re-plugged to perform
 *       |    to pick it up)                            |  capture-uncapture transition)
 *       |-> CAPTURING -------------------------------->|---> REPLUGGING -----
 *            ^  |    (device driver picked             |                    |
 *            |  |     up the device)                   | (remove cased      |  (device is removed
 *            |  ->---> CAPTURED ---------------------->|  by "real" removal |   the device info is removed form the list)
 *            |            |                            |------------------->->--> REMOVED
 *            |            |                            |
 *            |-----------<->---> USED_BY_GUEST ------->|
 *            |                         |
 *            |------------------------<-
 *
 * NOTE: the order of enums DOES MATTER!!
 * Do not blindly modify!! as the code assumes the state is ordered this way.
 */
typedef enum
{
    VBOXUSBFLT_DEVSTATE_UNKNOWN = 0,
    VBOXUSBFLT_DEVSTATE_REMOVED,
    VBOXUSBFLT_DEVSTATE_REPLUGGING,
    VBOXUSBFLT_DEVSTATE_ADDED,
    VBOXUSBFLT_DEVSTATE_UNCAPTURED,
    VBOXUSBFLT_DEVSTATE_CAPTURING,
    VBOXUSBFLT_DEVSTATE_CAPTURED,
    VBOXUSBFLT_DEVSTATE_USED_BY_GUEST,
    VBOXUSBFLT_DEVSTATE_32BIT_HACK = 0x7fffffff
} VBOXUSBFLT_DEVSTATE;

typedef struct VBOXUSBFLT_DEVICE
{
    LIST_ENTRY      GlobalLe;
    /* auxiliary list to be used for gathering devices to be re-plugged
     * only thread that puts the device to the REPLUGGING state can use this list */
    LIST_ENTRY      RepluggingLe;
    /* Owning session. Each matched device has an owning session. */
    struct VBOXUSBFLTCTX *pOwner;
    /* filter id - if NULL AND device has an owner - the filter is destroyed */
    uintptr_t uFltId;
    /* true iff device is filtered with a one-shot filter */
    bool fIsFilterOneShot;
    /* The device state. If the non-owner session is requesting the state while the device is grabbed,
     * the USBDEVICESTATE_USED_BY_HOST is returned. */
    VBOXUSBFLT_DEVSTATE  enmState;
    volatile uint32_t cRefs;
    PDEVICE_OBJECT  Pdo;
    uint16_t        idVendor;
    uint16_t        idProduct;
    uint16_t        bcdDevice;
    uint8_t         bClass;
    uint8_t         bSubClass;
    uint8_t         bProtocol;
    char            szSerial[MAX_USB_SERIAL_STRING];
    char            szMfgName[MAX_USB_SERIAL_STRING];
    char            szProduct[MAX_USB_SERIAL_STRING];
#if 0
    char            szDrvKeyName[512];
    BOOLEAN         fHighSpeed;
#endif
} VBOXUSBFLT_DEVICE, *PVBOXUSBFLT_DEVICE;

#define PVBOXUSBFLT_DEVICE_FROM_LE(_pLe) ( (PVBOXUSBFLT_DEVICE)( ((uint8_t*)(_pLe)) - RT_OFFSETOF(VBOXUSBFLT_DEVICE, GlobalLe) ) )
#define PVBOXUSBFLT_DEVICE_FROM_REPLUGGINGLE(_pLe)  ( (PVBOXUSBFLT_DEVICE)( ((uint8_t*)(_pLe)) - RT_OFFSETOF(VBOXUSBFLT_DEVICE, RepluggingLe) ) )
#define PVBOXUSBFLTCTX_FROM_LE(_pLe) ( (PVBOXUSBFLTCTX)( ((uint8_t*)(_pLe)) - RT_OFFSETOF(VBOXUSBFLTCTX, ListEntry) ) )

typedef struct VBOXUSBFLT_LOCK
{
    KSPIN_LOCK Lock;
    KIRQL OldIrql;
} VBOXUSBFLT_LOCK, *PVBOXUSBFLT_LOCK;

#define VBOXUSBFLT_LOCK_INIT() \
    KeInitializeSpinLock(&g_VBoxUsbFltGlobals.Lock.Lock)
#define VBOXUSBFLT_LOCK_TERM() do { } while (0)
#define VBOXUSBFLT_LOCK_ACQUIRE() \
    KeAcquireSpinLock(&g_VBoxUsbFltGlobals.Lock.Lock, &g_VBoxUsbFltGlobals.Lock.OldIrql);
#define VBOXUSBFLT_LOCK_RELEASE() \
    KeReleaseSpinLock(&g_VBoxUsbFltGlobals.Lock.Lock, g_VBoxUsbFltGlobals.Lock.OldIrql);


typedef struct VBOXUSBFLT_BLDEV
{
    LIST_ENTRY ListEntry;
    uint16_t   idVendor;
    uint16_t   idProduct;
    uint16_t   bcdDevice;
} VBOXUSBFLT_BLDEV, *PVBOXUSBFLT_BLDEV;

#define PVBOXUSBFLT_BLDEV_FROM_LE(_pLe) ( (PVBOXUSBFLT_BLDEV)( ((uint8_t*)(_pLe)) - RT_OFFSETOF(VBOXUSBFLT_BLDEV, ListEntry) ) )

typedef struct VBOXUSBFLTGLOBALS
{
    LIST_ENTRY DeviceList;
    LIST_ENTRY ContextList;
    /* devices known to misbehave */
    LIST_ENTRY BlackDeviceList;
    VBOXUSBFLT_LOCK Lock;
} VBOXUSBFLTGLOBALS, *PVBOXUSBFLTGLOBALS;
static VBOXUSBFLTGLOBALS g_VBoxUsbFltGlobals;

static bool vboxUsbFltBlDevMatchLocked(uint16_t idVendor, uint16_t idProduct, uint16_t bcdDevice)
{
    for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.BlackDeviceList.Flink;
            pEntry != &g_VBoxUsbFltGlobals.BlackDeviceList;
            pEntry = pEntry->Flink)
    {
        PVBOXUSBFLT_BLDEV pDev = PVBOXUSBFLT_BLDEV_FROM_LE(pEntry);
        if (pDev->idVendor != idVendor)
            continue;
        if (pDev->idProduct != idProduct)
            continue;
        if (pDev->bcdDevice != bcdDevice)
            continue;

        return true;
    }
    return false;
}

static NTSTATUS vboxUsbFltBlDevAddLocked(uint16_t idVendor, uint16_t idProduct, uint16_t bcdDevice)
{
    if (vboxUsbFltBlDevMatchLocked(idVendor, idProduct, bcdDevice))
        return STATUS_SUCCESS;
    PVBOXUSBFLT_BLDEV pDev = (PVBOXUSBFLT_BLDEV)VBoxUsbMonMemAllocZ(sizeof (*pDev));
    if (!pDev)
    {
        AssertFailed();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pDev->idVendor = idVendor;
    pDev->idProduct = idProduct;
    pDev->bcdDevice = bcdDevice;
    InsertHeadList(&g_VBoxUsbFltGlobals.BlackDeviceList, &pDev->ListEntry);
    return STATUS_SUCCESS;
}

static void vboxUsbFltBlDevClearLocked()
{
    PLIST_ENTRY pNext;
    for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.BlackDeviceList.Flink;
            pEntry != &g_VBoxUsbFltGlobals.BlackDeviceList;
            pEntry = pNext)
    {
        pNext = pEntry->Flink;
        VBoxUsbMonMemFree(pEntry);
    }
}

static void vboxUsbFltBlDevPopulateWithKnownLocked()
{
    /* this one halts when trying to get string descriptors from it */
    vboxUsbFltBlDevAddLocked(0x5ac, 0x921c, 0x115);
}


DECLINLINE(void) vboxUsbFltDevRetain(PVBOXUSBFLT_DEVICE pDevice)
{
    Assert(pDevice->cRefs);
    ASMAtomicIncU32(&pDevice->cRefs);
}

static void vboxUsbFltDevDestroy(PVBOXUSBFLT_DEVICE pDevice)
{
    Assert(!pDevice->cRefs);
    Assert(pDevice->enmState == VBOXUSBFLT_DEVSTATE_REMOVED);
    VBoxUsbMonMemFree(pDevice);
}

DECLINLINE(void) vboxUsbFltDevRelease(PVBOXUSBFLT_DEVICE pDevice)
{
    uint32_t cRefs = ASMAtomicDecU32(&pDevice->cRefs);
    Assert(cRefs < UINT32_MAX/2);
    if (!cRefs)
    {
        vboxUsbFltDevDestroy(pDevice);
    }
}

static void vboxUsbFltDevOwnerSetLocked(PVBOXUSBFLT_DEVICE pDevice, PVBOXUSBFLTCTX pContext, uintptr_t uFltId, bool fIsOneShot)
{
    Assert(!pDevice->pOwner);
    ++pContext->cActiveFilters;
    pDevice->pOwner = pContext;
    pDevice->uFltId = uFltId;
    pDevice->fIsFilterOneShot = fIsOneShot;
}

static void vboxUsbFltDevOwnerClearLocked(PVBOXUSBFLT_DEVICE pDevice)
{
    Assert(pDevice->pOwner);
    --pDevice->pOwner->cActiveFilters;
    Assert(pDevice->pOwner->cActiveFilters < UINT32_MAX/2);
    pDevice->pOwner = NULL;
    pDevice->uFltId = 0;
}

static void vboxUsbFltDevOwnerUpdateLocked(PVBOXUSBFLT_DEVICE pDevice, PVBOXUSBFLTCTX pContext, uintptr_t uFltId, bool fIsOneShot)
{
    if (pDevice->pOwner != pContext)
    {
        if (pDevice->pOwner)
            vboxUsbFltDevOwnerClearLocked(pDevice);
        if (pContext)
            vboxUsbFltDevOwnerSetLocked(pDevice, pContext, uFltId, fIsOneShot);
    }
    else if (pContext)
    {
        pDevice->uFltId = uFltId;
        pDevice->fIsFilterOneShot = fIsOneShot;
    }
}

static PVBOXUSBFLT_DEVICE vboxUsbFltDevGetLocked(PDEVICE_OBJECT pPdo)
{
#ifdef DEBUG_misha
    for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.DeviceList.Flink;
            pEntry != &g_VBoxUsbFltGlobals.DeviceList;
            pEntry = pEntry->Flink)
    {
        PVBOXUSBFLT_DEVICE pDevice = PVBOXUSBFLT_DEVICE_FROM_LE(pEntry);
        for (PLIST_ENTRY pEntry2 = pEntry->Flink;
                pEntry2 != &g_VBoxUsbFltGlobals.DeviceList;
                pEntry2 = pEntry2->Flink)
        {
            PVBOXUSBFLT_DEVICE pDevice2 = PVBOXUSBFLT_DEVICE_FROM_LE(pEntry2);
            Assert(    pDevice->idVendor  != pDevice2->idVendor
                    || pDevice->idProduct != pDevice2->idProduct
                    || pDevice->bcdDevice != pDevice2->bcdDevice);
        }
    }
#endif
    for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.DeviceList.Flink;
            pEntry != &g_VBoxUsbFltGlobals.DeviceList;
            pEntry = pEntry->Flink)
    {
        PVBOXUSBFLT_DEVICE pDevice = PVBOXUSBFLT_DEVICE_FROM_LE(pEntry);
        Assert(    pDevice->enmState == VBOXUSBFLT_DEVSTATE_REPLUGGING
                || pDevice->enmState == VBOXUSBFLT_DEVSTATE_UNCAPTURED
                || pDevice->enmState == VBOXUSBFLT_DEVSTATE_CAPTURING
                || pDevice->enmState == VBOXUSBFLT_DEVSTATE_CAPTURED
                || pDevice->enmState == VBOXUSBFLT_DEVSTATE_USED_BY_GUEST);
        if (pDevice->Pdo == pPdo)
            return pDevice;
    }
    return NULL;
}

PVBOXUSBFLT_DEVICE vboxUsbFltDevGet(PDEVICE_OBJECT pPdo)
{
    PVBOXUSBFLT_DEVICE pDevice;

    VBOXUSBFLT_LOCK_ACQUIRE();
    pDevice = vboxUsbFltDevGetLocked(pPdo);
    if (pDevice->enmState > VBOXUSBFLT_DEVSTATE_ADDED)
    {
        vboxUsbFltDevRetain(pDevice);
    }
    else
    {
        pDevice = NULL;
    }
    VBOXUSBFLT_LOCK_RELEASE();

    return pDevice;
}

static NTSTATUS vboxUsbFltPdoReplug(PDEVICE_OBJECT pDo)
{
    NTSTATUS Status = VBoxUsbToolIoInternalCtlSendSync(pDo, IOCTL_INTERNAL_USB_CYCLE_PORT, NULL, NULL);
    Assert(Status == STATUS_SUCCESS);
    return Status;
}

static PVBOXUSBFLTCTX vboxUsbFltDevMatchLocked(PVBOXUSBFLT_DEVICE pDevice, uintptr_t *puId, bool fRemoveFltIfOneShot, bool *pfFilter, bool *pfIsOneShot)
{
    USBFILTER DevFlt;
    USBFilterInit(&DevFlt, USBFILTERTYPE_CAPTURE);
    USBFilterSetNumExact(&DevFlt, USBFILTERIDX_VENDOR_ID, pDevice->idVendor, true);
    USBFilterSetNumExact(&DevFlt, USBFILTERIDX_PRODUCT_ID, pDevice->idProduct, true);
    USBFilterSetNumExact(&DevFlt, USBFILTERIDX_DEVICE_REV, pDevice->bcdDevice, true);
    USBFilterSetNumExact(&DevFlt, USBFILTERIDX_DEVICE_CLASS, pDevice->bClass, true);
    USBFilterSetNumExact(&DevFlt, USBFILTERIDX_DEVICE_SUB_CLASS, pDevice->bSubClass, true);
    USBFilterSetNumExact(&DevFlt, USBFILTERIDX_DEVICE_PROTOCOL, pDevice->bProtocol, true);
    USBFilterSetStringExact(&DevFlt, USBFILTERIDX_MANUFACTURER_STR, pDevice->szMfgName, true);
    USBFilterSetStringExact(&DevFlt, USBFILTERIDX_PRODUCT_STR, pDevice->szProduct, true);
    USBFilterSetStringExact(&DevFlt, USBFILTERIDX_SERIAL_NUMBER_STR, pDevice->szSerial, true);

    /* Run filters on the thing. */
    *puId = 0;
    *pfFilter = false;
    *pfIsOneShot = false;
    PVBOXUSBFLTCTX pOwner = VBoxUSBFilterMatchEx(&DevFlt, puId, fRemoveFltIfOneShot, pfFilter, pfIsOneShot);
    USBFilterDelete(&DevFlt);
    return pOwner;
}

static void vboxUsbFltDevStateMarkReplugLocked(PVBOXUSBFLT_DEVICE pDevice)
{
    vboxUsbFltDevOwnerUpdateLocked(pDevice, NULL, 0, false);
    pDevice->enmState = VBOXUSBFLT_DEVSTATE_REPLUGGING;
}

static bool vboxUsbFltDevStateIsNotFiltered(PVBOXUSBFLT_DEVICE pDevice)
{
    return pDevice->enmState == VBOXUSBFLT_DEVSTATE_UNCAPTURED;
}

static bool vboxUsbFltDevStateIsFiltered(PVBOXUSBFLT_DEVICE pDevice)
{
    return pDevice->enmState >= VBOXUSBFLT_DEVSTATE_CAPTURING;
}

#define VBOXUSBMON_POPULATE_REQUEST_TIMEOUT_MS 10000

static NTSTATUS vboxUsbFltDevPopulate(PVBOXUSBFLT_DEVICE pDevice, PDEVICE_OBJECT pDo /*, BOOLEAN bPopulateNonFilterProps*/)
{
    NTSTATUS Status;
    PUSB_DEVICE_DESCRIPTOR pDevDr = 0;

    pDevice->Pdo = pDo;

    pDevDr = (PUSB_DEVICE_DESCRIPTOR)VBoxUsbMonMemAllocZ(sizeof(*pDevDr));
    if (pDevDr == NULL)
    {
        AssertMsgFailed(("Failed to alloc mem for urb\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    do
    {
        Status = VBoxUsbToolGetDescriptor(pDo, pDevDr, sizeof(*pDevDr), USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, VBOXUSBMON_POPULATE_REQUEST_TIMEOUT_MS);
        if (!NT_SUCCESS(Status))
        {
            LogRel((__FUNCTION__": getting device descriptor failed\n"));
            break;
        }

        if (vboxUsbFltBlDevMatchLocked(pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice))
        {
            LogRel((__FUNCTION__": found a known black list device, vid(0x%x), pid(0x%x), rev(0x%x)\n", pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice));
#ifdef DEBUG_misha
            AssertFailed();
#endif
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        Log(("Device pid=%x vid=%x rev=%x\n", pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice));
        pDevice->idVendor     = pDevDr->idVendor;
        pDevice->idProduct    = pDevDr->idProduct;
        pDevice->bcdDevice    = pDevDr->bcdDevice;
        pDevice->bClass       = pDevDr->bDeviceClass;
        pDevice->bSubClass    = pDevDr->bDeviceSubClass;
        pDevice->bProtocol    = pDevDr->bDeviceProtocol;
        pDevice->szSerial[0]  = 0;
        pDevice->szMfgName[0] = 0;
        pDevice->szProduct[0] = 0;

        /* If there are no strings, don't even try to get any string descriptors. */
        if (pDevDr->iSerialNumber || pDevDr->iManufacturer || pDevDr->iProduct)
        {
            int             langId;

            Status = VBoxUsbToolGetLangID(pDo, &langId, VBOXUSBMON_POPULATE_REQUEST_TIMEOUT_MS);
            if (!NT_SUCCESS(Status))
            {
                AssertMsgFailed((__FUNCTION__": reading language ID failed\n"));
                if (Status == STATUS_CANCELLED)
                {
                    AssertMsgFailed((__FUNCTION__": found a new black list device, vid(0x%x), pid(0x%x), rev(0x%x)\n", pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice));
                    vboxUsbFltBlDevAddLocked(pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice);
                    Status = STATUS_UNSUCCESSFUL;
                }
                break;
            }

            if (pDevDr->iSerialNumber)
            {
                Status = VBoxUsbToolGetStringDescriptorA(pDo, pDevice->szSerial, sizeof (pDevice->szSerial), pDevDr->iSerialNumber, langId, VBOXUSBMON_POPULATE_REQUEST_TIMEOUT_MS);
                if (!NT_SUCCESS(Status))
                {
                    AssertMsgFailed((__FUNCTION__": reading serial number failed\n"));
                    if (Status == STATUS_CANCELLED)
                    {
                        AssertMsgFailed((__FUNCTION__": found a new black list device, vid(0x%x), pid(0x%x), rev(0x%x)\n", pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice));
                        vboxUsbFltBlDevAddLocked(pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice);
                        Status = STATUS_UNSUCCESSFUL;
                    }
                    break;
                }
            }

            if (pDevDr->iManufacturer)
            {
                Status = VBoxUsbToolGetStringDescriptorA(pDo, pDevice->szMfgName, sizeof (pDevice->szMfgName), pDevDr->iManufacturer, langId, VBOXUSBMON_POPULATE_REQUEST_TIMEOUT_MS);
                if (!NT_SUCCESS(Status))
                {
                    AssertMsgFailed((__FUNCTION__": reading manufacturer name failed\n"));
                    if (Status == STATUS_CANCELLED)
                    {
                        AssertMsgFailed((__FUNCTION__": found a new black list device, vid(0x%x), pid(0x%x), rev(0x%x)\n", pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice));
                        vboxUsbFltBlDevAddLocked(pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice);
                        Status = STATUS_UNSUCCESSFUL;
                    }
                    break;
                }
            }

            if (pDevDr->iProduct)
            {
                Status = VBoxUsbToolGetStringDescriptorA(pDo, pDevice->szProduct, sizeof (pDevice->szProduct), pDevDr->iProduct, langId, VBOXUSBMON_POPULATE_REQUEST_TIMEOUT_MS);
                if (!NT_SUCCESS(Status))
                {
                    AssertMsgFailed((__FUNCTION__": reading product name failed\n"));
                    if (Status == STATUS_CANCELLED)
                    {
                        AssertMsgFailed((__FUNCTION__": found a new black list device, vid(0x%x), pid(0x%x), rev(0x%x)\n", pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice));
                        vboxUsbFltBlDevAddLocked(pDevDr->idVendor, pDevDr->idProduct, pDevDr->bcdDevice);
                        Status = STATUS_UNSUCCESSFUL;
                    }
                    break;
                }
            }

#if 0
            if (bPopulateNonFilterProps)
            {
                WCHAR RegKeyBuf[512];
                ULONG cbRegKeyBuf = sizeof (RegKeyBuf);
                Status = IoGetDeviceProperty(pDo,
                                              DevicePropertyDriverKeyName,
                                              cbRegKeyBuf,
                                              RegKeyBuf,
                                              &cbRegKeyBuf);
                if (!NT_SUCCESS(Status))
                {
                    AssertMsgFailed((__FUNCTION__": IoGetDeviceProperty failed Status (0x%x)\n", Status));
                    break;
                }

                ANSI_STRING Ansi;
                UNICODE_STRING Unicode;
                Ansi.Buffer = pDevice->szDrvKeyName;
                Ansi.Length = 0;
                Ansi.MaximumLength = sizeof(pDevice->szDrvKeyName);
                RtlInitUnicodeString(&Unicode, RegKeyBuf);

                Status = RtlUnicodeStringToAnsiString(&Ansi, &Unicode, FALSE /* do not allocate */);
                if (!NT_SUCCESS(Status))
                {
                    AssertMsgFailed((__FUNCTION__": RtlUnicodeStringToAnsiString failed Status (0x%x)\n", Status));
                    break;
                }

                pDevice->fHighSpend = FALSE;
                Status = VBoxUsbToolGetDeviceSpeed(pDo, &pDevice->fHighSpend);
                if (!NT_SUCCESS(Status))
                {
                    AssertMsgFailed((__FUNCTION__": VBoxUsbToolGetDeviceSpeed failed Status (0x%x)\n", Status));
                    break;
                }
            }
#endif
            Log((__FUNCTION__": strings: '%s':'%s':'%s' (lang ID %x)\n",
                        pDevice->szMfgName, pDevice->szProduct, pDevice->szSerial, langId));
        }

        Status = STATUS_SUCCESS;
    } while (0);

    VBoxUsbMonMemFree(pDevDr);
    return Status;
}

static void vboxUsbFltSignalChangeLocked()
{
    for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.ContextList.Flink;
            pEntry != &g_VBoxUsbFltGlobals.ContextList;
            pEntry = pEntry->Flink)
    {
        PVBOXUSBFLTCTX pCtx = PVBOXUSBFLTCTX_FROM_LE(pEntry);
        /* the removed context can not be in a list */
        Assert(!pCtx->bRemoved);
        if (pCtx->pChangeEvent)
        {
            KeSetEvent(pCtx->pChangeEvent,
                    0, /* increment*/
                    FALSE /* wait */);
        }
    }
}

static bool vboxUsbFltDevCheckReplugLocked(PVBOXUSBFLT_DEVICE pDevice, PVBOXUSBFLTCTX pContext)
{
    Assert(pContext);

    /* check if device is already replugging */
    if (pDevice->enmState <= VBOXUSBFLT_DEVSTATE_ADDED)
    {
        /* it is, do nothing */
        Assert(pDevice->enmState == VBOXUSBFLT_DEVSTATE_REPLUGGING);
        return false;
    }

    if (pDevice->pOwner && pContext != pDevice->pOwner)
    {
        /* this device is owned by another context, we're not allowed to do anything */
        return false;
    }

    uintptr_t uId = 0;
    bool bNeedReplug = false;
    bool fFilter = false;
    bool fIsOneShot = false;
    PVBOXUSBFLTCTX pNewOwner = vboxUsbFltDevMatchLocked(pDevice, &uId,
            false, /* do not remove a one-shot filter */
            &fFilter, &fIsOneShot);
    if (pDevice->pOwner && pNewOwner && pDevice->pOwner != pNewOwner)
    {
        /* the device is owned by another owner, we can not change the owner here */
        return false;
    }

    if (!fFilter)
    {
        /* the device should NOT be filtered, check the current state  */
        if (vboxUsbFltDevStateIsNotFiltered(pDevice))
        {
            /* no changes */
            if (fIsOneShot)
            {
                Assert(pNewOwner);
                /* remove a one-shot filter and keep the original filter data */
                int tmpRc = VBoxUSBFilterRemove(pNewOwner, uId);
                AssertRC(tmpRc);
                if (!pDevice->pOwner)
                {
                    /* update owner for one-shot if the owner is changed (i.e. assigned) */
                    vboxUsbFltDevOwnerUpdateLocked(pDevice, pNewOwner, uId, true);
                }
            }
            else
            {
                if (pNewOwner)
                {
                    vboxUsbFltDevOwnerUpdateLocked(pDevice, pNewOwner, uId, false);
                }
            }
        }
        else
        {
            /* the device is currently filtered, we should release it only if
             * 1. device does not have an owner
             * or
             * 2. it should be released bue to a one-shot filter
             * or
             * 3. it is NOT grabbed by a one-shot filter */
            if (!pDevice->pOwner || fIsOneShot || !pDevice->fIsFilterOneShot)
            {
                bNeedReplug = true;
            }
        }
    }
    else
    {
        /* the device should be filtered, check the current state  */
        Assert(uId);
        Assert(pNewOwner);
        if (vboxUsbFltDevStateIsFiltered(pDevice))
        {
            /* the device is filtered */
            if (pNewOwner == pDevice->pOwner)
            {
                /* no changes */
                if (fIsOneShot)
                {
                    /* remove a one-shot filter and keep the original filter data */
                    int tmpRc = VBoxUSBFilterRemove(pNewOwner, uId);
                    AssertRC(tmpRc);
                }
                else
                {
                    vboxUsbFltDevOwnerUpdateLocked(pDevice, pDevice->pOwner, uId, false);
                }
            }
            else
            {
                Assert(!pDevice->pOwner);
                /* the device needs to be filtered, but the owner changes, replug needed */
                bNeedReplug = true;
            }
        }
        else
        {
            /* the device is currently NOT filtered,
             * we should replug it only if
             * 1. device does not have an owner
             * or
             * 2. it should be captured due to a one-shot filter
             * or
             * 3. it is NOT released by a one-shot filter */
            if (!pDevice->pOwner || fIsOneShot || !pDevice->fIsFilterOneShot)
            {
                bNeedReplug = true;
            }
        }
    }

    if (bNeedReplug)
    {
        vboxUsbFltDevStateMarkReplugLocked(pDevice);
    }

    return bNeedReplug;
}

static void vboxUsbFltReplugList(PLIST_ENTRY pList)
{
    PLIST_ENTRY pNext;
    for (PLIST_ENTRY pEntry = pList->Flink;
            pEntry != pList;
            pEntry = pNext)
    {
        pNext = pEntry->Flink;
        PVBOXUSBFLT_DEVICE pDevice = PVBOXUSBFLT_DEVICE_FROM_REPLUGGINGLE(pEntry);
        Assert(pDevice->enmState == VBOXUSBFLT_DEVSTATE_REPLUGGING
                || pDevice->enmState == VBOXUSBFLT_DEVSTATE_REMOVED);

        vboxUsbFltPdoReplug(pDevice->Pdo);
        ObDereferenceObject(pDevice->Pdo);
        vboxUsbFltDevRelease(pDevice);
    }
}

NTSTATUS VBoxUsbFltFilterCheck(PVBOXUSBFLTCTX pContext)
{
    NTSTATUS Status;
    UNICODE_STRING szStandardControllerName[RT_ELEMENTS(lpszStandardControllerName)];
    KIRQL Irql = KeGetCurrentIrql();
    Assert(Irql == PASSIVE_LEVEL);

    Log(("==" __FUNCTION__"\n"));

    for (int i=0;i<RT_ELEMENTS(lpszStandardControllerName);i++)
    {
        szStandardControllerName[i].Length = 0;
        szStandardControllerName[i].MaximumLength = 0;
        szStandardControllerName[i].Buffer = 0;

        RtlInitUnicodeString(&szStandardControllerName[i], lpszStandardControllerName[i]);
    }

    for (int i = 0; i < 16; i++)
    {
        char            szHubName[32];
        WCHAR           szwHubName[32];
        UNICODE_STRING  UnicodeName;
        ANSI_STRING     AnsiName;
        PDEVICE_OBJECT  pHubDevObj;
        PFILE_OBJECT    pHubFileObj;

        sprintf(szHubName, "\\Device\\USBPDO-%d", i);

        UnicodeName.Length = 0;
        UnicodeName.MaximumLength = sizeof (szwHubName);
        UnicodeName.Buffer = szwHubName;

        RtlInitAnsiString(&AnsiName, szHubName);
        RtlAnsiStringToUnicodeString(&UnicodeName, &AnsiName, FALSE);

        Status = IoGetDeviceObjectPointer(&UnicodeName, FILE_READ_DATA, &pHubFileObj, &pHubDevObj);
        if (Status == STATUS_SUCCESS)
        {
            Log(("IoGetDeviceObjectPointer for %s returned %p %p\n", szHubName, pHubDevObj, pHubFileObj));

            if (pHubDevObj->DriverObject
                && pHubDevObj->DriverObject->DriverName.Buffer
                && pHubDevObj->DriverObject->DriverName.Length
               )
            {
                for (int j = 0; j < RT_ELEMENTS(lpszStandardControllerName); ++j)
                {
                    if (!RtlCompareUnicodeString(&szStandardControllerName[j], &pHubDevObj->DriverObject->DriverName, TRUE /* case insensitive */))
                    {
                        PDEVICE_RELATIONS pDevRelations = NULL;

#ifdef DEBUG
                        Log(("Associated driver "));
                        vboxUsbDbgPrintUnicodeString(&pHubDevObj->DriverObject->DriverName);
                        Log((" -> related dev obj=0x%p\n", IoGetRelatedDeviceObject(pHubFileObj)));
#endif

                        Status = VBoxUsbMonQueryBusRelations(pHubDevObj, pHubFileObj, &pDevRelations);
                        if (Status == STATUS_SUCCESS && pDevRelations)
                        {
                            ULONG cReplugPdos = pDevRelations->Count;
                            LIST_ENTRY ReplugDevList;
                            InitializeListHead(&ReplugDevList);
                            for (ULONG k = 0; k < pDevRelations->Count; ++k)
                            {
                                PDEVICE_OBJECT pDevObj = pDevRelations->Objects[k];

                                Log(("Found existing USB PDO 0x%p\n", pDevObj));
                                VBOXUSBFLT_LOCK_ACQUIRE();
                                PVBOXUSBFLT_DEVICE pDevice = vboxUsbFltDevGetLocked(pDevObj);
                                if (pDevice)
                                {
                                    bool bReplug = vboxUsbFltDevCheckReplugLocked(pDevice, pContext);
                                    if (bReplug)
                                    {
                                        InsertHeadList(&ReplugDevList, &pDevice->RepluggingLe);
                                        vboxUsbFltDevRetain(pDevice);
                                        /* do not dereference object since we will use it later */
                                    }
                                    else
                                    {
                                        ObDereferenceObject(pDevObj);
                                    }

                                    VBOXUSBFLT_LOCK_RELEASE();

                                    pDevRelations->Objects[k] = NULL;
                                    --cReplugPdos;
                                    Assert((uint32_t)cReplugPdos < UINT32_MAX/2);
                                    continue;
                                }
                                VBOXUSBFLT_LOCK_RELEASE();

                                VBOXUSBFLT_DEVICE Device;
                                Status = vboxUsbFltDevPopulate(&Device, pDevObj /*, FALSE /* only need filter properties */);
                                if (NT_SUCCESS(Status))
                                {
                                    uintptr_t uId = 0;
                                    bool fFilter = false;
                                    bool fIsOneShot = false;
                                    VBOXUSBFLT_LOCK_ACQUIRE();
                                    PVBOXUSBFLTCTX pCtx = vboxUsbFltDevMatchLocked(&Device, &uId,
                                            false, /* do not remove a one-shot filter */
                                            &fFilter, &fIsOneShot);
                                    VBOXUSBFLT_LOCK_RELEASE();
                                    if (!fFilter)
                                    {
                                        /* this device should not be filtered, and it's not */
                                        ObDereferenceObject(pDevObj);
                                        pDevRelations->Objects[k] = NULL;
                                        --cReplugPdos;
                                        Assert((uint32_t)cReplugPdos < UINT32_MAX/2);
                                        continue;
                                    }

                                    /* this device needs to be filtered, but it's not,
                                     * leave the PDO in array to issue a replug request for it
                                     * later on */

                                }
                            }

                            if (cReplugPdos)
                            {
                                for (ULONG k = 0; k < pDevRelations->Count; ++k)
                                {
                                    if (!pDevRelations->Objects[k])
                                        continue;

                                    Status = vboxUsbFltPdoReplug(pDevRelations->Objects[k]);
                                    Assert(Status == STATUS_SUCCESS);
                                    ObDereferenceObject(pDevRelations->Objects[k]);
                                    if (!--cReplugPdos)
                                        break;
                                }

                                Assert(!cReplugPdos);
                            }

                            vboxUsbFltReplugList(&ReplugDevList);

                            ExFreePool(pDevRelations);
                        }
                    }
                }
            }
            ObDereferenceObject(pHubFileObj);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS VBoxUsbFltClose(PVBOXUSBFLTCTX pContext)
{
    LIST_ENTRY ReplugDevList;
    InitializeListHead(&ReplugDevList);

    Assert(pContext);

    KIRQL Irql = KeGetCurrentIrql();
    Assert(Irql == PASSIVE_LEVEL);

    VBOXUSBFLT_LOCK_ACQUIRE();
    uint32_t cActiveFilters = pContext->cActiveFilters;
    pContext->bRemoved = TRUE;
    if (pContext->pChangeEvent)
    {
        KeSetEvent(pContext->pChangeEvent,
                0, /* increment*/
                FALSE /* wait */);
        ObDereferenceObject(pContext->pChangeEvent);
        pContext->pChangeEvent = NULL;
    }
    RemoveEntryList(&pContext->ListEntry);

    /* now re-arrange the filters */
    /* 1. remove filters */
    VBoxUSBFilterRemoveOwner(pContext);

    /* 2. check if there are devices owned */
    for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.DeviceList.Flink;
            pEntry != &g_VBoxUsbFltGlobals.DeviceList;
            pEntry = pEntry->Flink)
    {
        PVBOXUSBFLT_DEVICE pDevice = PVBOXUSBFLT_DEVICE_FROM_LE(pEntry);
        if (pDevice->pOwner != pContext)
            continue;

        Assert(pDevice->enmState != VBOXUSBFLT_DEVSTATE_ADDED);
        Assert(pDevice->enmState != VBOXUSBFLT_DEVSTATE_REMOVED);

        vboxUsbFltDevOwnerClearLocked(pDevice);

        if (vboxUsbFltDevCheckReplugLocked(pDevice, pContext))
        {
            InsertHeadList(&ReplugDevList, &pDevice->RepluggingLe);
            /* retain to ensure the device is not removed before we issue a replug */
            vboxUsbFltDevRetain(pDevice);
            /* keep the PDO alive */
            ObReferenceObject(pDevice->Pdo);
        }
    }
    VBOXUSBFLT_LOCK_RELEASE();

    /* this should replug all devices that were either skipped or grabbed due to the context's */
    vboxUsbFltReplugList(&ReplugDevList);

    return STATUS_SUCCESS;
}

NTSTATUS VBoxUsbFltCreate(PVBOXUSBFLTCTX pContext)
{
    memset(pContext, 0, sizeof (*pContext));
    pContext->Process = RTProcSelf();
    VBOXUSBFLT_LOCK_ACQUIRE();
    InsertHeadList(&g_VBoxUsbFltGlobals.ContextList, &pContext->ListEntry);
    VBOXUSBFLT_LOCK_RELEASE();
    return STATUS_SUCCESS;
}

int VBoxUsbFltAdd(PVBOXUSBFLTCTX pContext, PUSBFILTER pFilter, uintptr_t *pId)
{
    *pId = 0;
    /* Log the filter details. */
    Log((__FUNCTION__": %s %s %s\n",
        USBFilterGetString(pFilter, USBFILTERIDX_MANUFACTURER_STR)  ? USBFilterGetString(pFilter, USBFILTERIDX_MANUFACTURER_STR)  : "<null>",
        USBFilterGetString(pFilter, USBFILTERIDX_PRODUCT_STR)       ? USBFilterGetString(pFilter, USBFILTERIDX_PRODUCT_STR)       : "<null>",
        USBFilterGetString(pFilter, USBFILTERIDX_SERIAL_NUMBER_STR) ? USBFilterGetString(pFilter, USBFILTERIDX_SERIAL_NUMBER_STR) : "<null>"));
#ifdef DEBUG
    Log(("VBoxUSBClient::addFilter: idVendor=%#x idProduct=%#x bcdDevice=%#x bDeviceClass=%#x bDeviceSubClass=%#x bDeviceProtocol=%#x bBus=%#x bPort=%#x\n",
              USBFilterGetNum(pFilter, USBFILTERIDX_VENDOR_ID),
              USBFilterGetNum(pFilter, USBFILTERIDX_PRODUCT_ID),
              USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_REV),
              USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_CLASS),
              USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_SUB_CLASS),
              USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_PROTOCOL),
              USBFilterGetNum(pFilter, USBFILTERIDX_BUS),
              USBFilterGetNum(pFilter, USBFILTERIDX_PORT)));
#endif

    /* We can't get the bus/port numbers. Ignore them while matching. */
    USBFilterSetMustBePresent(pFilter, USBFILTERIDX_BUS, false);
    USBFilterSetMustBePresent(pFilter, USBFILTERIDX_PORT, false);

    uintptr_t uId = 0;
    VBOXUSBFLT_LOCK_ACQUIRE();
    /* Add the filter. */
    int rc = VBoxUSBFilterAdd(pFilter, pContext, &uId);
    VBOXUSBFLT_LOCK_RELEASE();
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        Assert(uId);
#ifdef VBOX_USBMON_WITH_FILTER_AUTOAPPLY
        VBoxUsbFltFilterCheck();
#endif
    }
    else
    {
        Assert(!uId);
    }

    *pId = uId;
    return rc;
}

int VBoxUsbFltRemove(PVBOXUSBFLTCTX pContext, uintptr_t uId)
{
    Assert(uId);

    VBOXUSBFLT_LOCK_ACQUIRE();
    int rc = VBoxUSBFilterRemove(pContext, uId);
    if (!RT_SUCCESS(rc))
    {
        AssertFailed();
        VBOXUSBFLT_LOCK_RELEASE();
        return rc;
    }

    for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.DeviceList.Flink;
            pEntry != &g_VBoxUsbFltGlobals.DeviceList;
            pEntry = pEntry->Flink)
    {
        PVBOXUSBFLT_DEVICE pDevice = PVBOXUSBFLT_DEVICE_FROM_LE(pEntry);
        if (pDevice->fIsFilterOneShot)
        {
            Assert(!pDevice->uFltId);
        }

        if (pDevice->uFltId != uId)
            continue;

        Assert(pDevice->pOwner == pContext);
        if (pDevice->pOwner != pContext)
            continue;

        Assert(!pDevice->fIsFilterOneShot);
        pDevice->uFltId = 0;
        /* clear the fIsFilterOneShot flag to ensure the device is replugged on the next VBoxUsbFltFilterCheck call */
        pDevice->fIsFilterOneShot = false;
    }
    VBOXUSBFLT_LOCK_RELEASE();

    if (RT_SUCCESS(rc))
    {
#ifdef VBOX_USBMON_WITH_FILTER_AUTOAPPLY
        VBoxUsbFltFilterCheck();
#endif
    }
    return rc;
}

NTSTATUS VBoxUsbFltSetNotifyEvent(PVBOXUSBFLTCTX pContext, HANDLE hEvent)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKEVENT pEvent = NULL;
    PKEVENT pOldEvent = NULL;
    if (hEvent)
    {
        Status = ObReferenceObjectByHandle(hEvent,
                    EVENT_MODIFY_STATE,
                    *ExEventObjectType, UserMode,
                    (PVOID*)&pEvent,
                    NULL);
        Assert(Status == STATUS_SUCCESS);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    VBOXUSBFLT_LOCK_ACQUIRE();
    pOldEvent = pContext->pChangeEvent;
    pContext->pChangeEvent = pEvent;
    VBOXUSBFLT_LOCK_RELEASE();

    if (pOldEvent)
    {
        ObDereferenceObject(pOldEvent);
    }

    return STATUS_SUCCESS;
}

static USBDEVICESTATE vboxUsbDevGetUserState(PVBOXUSBFLTCTX pContext, PVBOXUSBFLT_DEVICE pDevice)
{
    if (vboxUsbFltDevStateIsNotFiltered(pDevice))
        return USBDEVICESTATE_USED_BY_HOST_CAPTURABLE;

    /* the device is filtered, or replugging */
    if (pDevice->enmState == VBOXUSBFLT_DEVSTATE_REPLUGGING)
    {
        Assert(!pDevice->pOwner);
        Assert(!pDevice->uFltId);
        AssertFailed();
        /* no user state for this, we should not return it tu the user */
        return USBDEVICESTATE_USED_BY_HOST;
    }

    /* the device is filtered, if owner differs from the context, return as USED_BY_HOST */
    Assert(pDevice->pOwner);
    /* the id can be null if a filter is removed */
//    Assert(pDevice->uFltId);

    if (pDevice->pOwner != pContext)
        return USBDEVICESTATE_USED_BY_HOST;

    switch (pDevice->enmState)
    {
        case VBOXUSBFLT_DEVSTATE_UNCAPTURED:
        case VBOXUSBFLT_DEVSTATE_CAPTURING:
            return USBDEVICESTATE_USED_BY_HOST_CAPTURABLE;
        case VBOXUSBFLT_DEVSTATE_CAPTURED:
            return USBDEVICESTATE_HELD_BY_PROXY;
        case VBOXUSBFLT_DEVSTATE_USED_BY_GUEST:
            return USBDEVICESTATE_USED_BY_GUEST;
        default:
            AssertFailed();
            return USBDEVICESTATE_UNSUPPORTED;
    }
}

static void vboxUsbDevToUserInfo(PVBOXUSBFLTCTX pContext, PVBOXUSBFLT_DEVICE pDevice, PUSBSUP_DEVINFO pDevInfo)
{
#if 0
    pDevInfo->usVendorId = pDevice->idVendor;
    pDevInfo->usProductId = pDevice->idProduct;
    pDevInfo->usRevision = pDevice->bcdDevice;
    pDevInfo->enmState = vboxUsbDevGetUserState(pContext, pDevice);

    strcpy(pDevInfo->szDrvKeyName, pDevice->szDrvKeyName);
    if (pDevInfo->enmState == USBDEVICESTATE_HELD_BY_PROXY
            || pDevInfo->enmState == USBDEVICESTATE_USED_BY_GUEST)
    {
        /* this is the only case where we return the obj name to the client */
        strcpy(pDevInfo->szObjName, pDevice->szObjName);
    }
    pDevInfo->fHighSpeed = pDevice->fHighSpeed;
#endif
}

NTSTATUS VBoxUsbFltGetDevice(PVBOXUSBFLTCTX pContext, HVBOXUSBDEVUSR hDevice, PUSBSUP_GETDEV_MON pInfo)
{
    Assert(hDevice);

    memset (pInfo, 0, sizeof (*pInfo));
    VBOXUSBFLT_LOCK_ACQUIRE();
    for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.DeviceList.Flink;
            pEntry != &g_VBoxUsbFltGlobals.DeviceList;
            pEntry = pEntry->Flink)
    {
        PVBOXUSBFLT_DEVICE pDevice = PVBOXUSBFLT_DEVICE_FROM_LE(pEntry);
        Assert(pDevice->enmState != VBOXUSBFLT_DEVSTATE_REMOVED);
        Assert(pDevice->enmState != VBOXUSBFLT_DEVSTATE_ADDED);

        if (pDevice != hDevice)
            continue;

        USBDEVICESTATE enmUsrState = vboxUsbDevGetUserState(pContext, pDevice);
        pInfo->enmState = enmUsrState;
        VBOXUSBFLT_LOCK_RELEASE();
        return STATUS_SUCCESS;
    }

    VBOXUSBFLT_LOCK_RELEASE();

    /* this should not occur */
    AssertFailed();

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS VBoxUsbFltPdoAdd(PDEVICE_OBJECT pPdo, BOOLEAN *pbFiltered)
{
    *pbFiltered = FALSE;
    PVBOXUSBFLT_DEVICE pDevice;

    /* first check if device is in the a already */
    VBOXUSBFLT_LOCK_ACQUIRE();
    pDevice = vboxUsbFltDevGetLocked(pPdo);
    if (pDevice)
    {
        Assert(pDevice->enmState != VBOXUSBFLT_DEVSTATE_ADDED);
        Assert(pDevice->enmState != VBOXUSBFLT_DEVSTATE_REMOVED);
        *pbFiltered = pDevice->enmState >= VBOXUSBFLT_DEVSTATE_CAPTURING;
        VBOXUSBFLT_LOCK_RELEASE();
        return STATUS_SUCCESS;
    }
    VBOXUSBFLT_LOCK_RELEASE();
    pDevice = (PVBOXUSBFLT_DEVICE)VBoxUsbMonMemAllocZ(sizeof (*pDevice));
    if (!pDevice)
    {
        AssertFailed();
        return STATUS_NO_MEMORY;
    }

    pDevice->enmState = VBOXUSBFLT_DEVSTATE_ADDED;
    pDevice->cRefs = 1;
    NTSTATUS Status = vboxUsbFltDevPopulate(pDevice, pPdo /* , TRUE /* need all props */);
    if (!NT_SUCCESS(Status))
    {
        AssertFailed();
        VBoxUsbMonMemFree(pDevice);
        return Status;
    }

    uintptr_t uId;
    bool fFilter = false;
    bool fIsOneShot = false;
    PVBOXUSBFLTCTX pCtx;
    PVBOXUSBFLT_DEVICE pTmpDev;
    VBOXUSBFLT_LOCK_ACQUIRE();
    /* (paranoia) re-check the device is still not here */
    pTmpDev = vboxUsbFltDevGetLocked(pPdo);
    if (pTmpDev)
    {
        Assert(pTmpDev->enmState != VBOXUSBFLT_DEVSTATE_ADDED);
        Assert(pTmpDev->enmState != VBOXUSBFLT_DEVSTATE_REMOVED);
        *pbFiltered = pTmpDev->enmState >= VBOXUSBFLT_DEVSTATE_CAPTURING;
        VBOXUSBFLT_LOCK_RELEASE();
        VBoxUsbMonMemFree(pDevice);
        return STATUS_SUCCESS;
    }

    pCtx = vboxUsbFltDevMatchLocked(pDevice, &uId,
            true, /* remove a one-shot filter */
            &fFilter, &fIsOneShot);
    if (fFilter)
    {
        Assert(pCtx);
        Assert(uId);
        pDevice->enmState = VBOXUSBFLT_DEVSTATE_CAPTURING;
    }
    else
    {
        Assert(!uId == !pCtx); /* either both zero or both not */
        pDevice->enmState = VBOXUSBFLT_DEVSTATE_UNCAPTURED;
    }

    if (pCtx)
        vboxUsbFltDevOwnerSetLocked(pDevice, pCtx, fIsOneShot ? 0 : uId, fIsOneShot);

    InsertHeadList(&g_VBoxUsbFltGlobals.DeviceList, &pDevice->GlobalLe);

    /* do not need to signal anything here -
     * going to do that once the proxy device object starts */
    VBOXUSBFLT_LOCK_RELEASE();

    *pbFiltered = fFilter;

    return STATUS_SUCCESS;
}

NTSTATUS VBoxUsbFltPdoAddCompleted(PDEVICE_OBJECT pPdo)
{
    VBOXUSBFLT_LOCK_ACQUIRE();
    vboxUsbFltSignalChangeLocked();
    VBOXUSBFLT_LOCK_RELEASE();
    return STATUS_SUCCESS;
}

BOOLEAN VBoxUsbFltPdoIsFiltered(PDEVICE_OBJECT pPdo)
{
    VBOXUSBFLT_DEVSTATE enmState = VBOXUSBFLT_DEVSTATE_REMOVED;
    VBOXUSBFLT_LOCK_ACQUIRE();
    PVBOXUSBFLT_DEVICE pDevice = vboxUsbFltDevGetLocked(pPdo);
    if (pDevice)
    {
        enmState = pDevice->enmState;
    }
    VBOXUSBFLT_LOCK_RELEASE();

    return enmState >= VBOXUSBFLT_DEVSTATE_CAPTURING;
}

NTSTATUS VBoxUsbFltPdoRemove(PDEVICE_OBJECT pPdo)
{
    PVBOXUSBFLT_DEVICE pDevice;
    VBOXUSBFLT_DEVSTATE enmOldState;

    VBOXUSBFLT_LOCK_ACQUIRE();
    pDevice = vboxUsbFltDevGetLocked(pPdo);
    if (pDevice)
    {
        RemoveEntryList(&pDevice->GlobalLe);
        enmOldState = pDevice->enmState;
        pDevice->enmState = VBOXUSBFLT_DEVSTATE_REMOVED;
        if (enmOldState != VBOXUSBFLT_DEVSTATE_REPLUGGING)
        {
            vboxUsbFltSignalChangeLocked();
        }
        else
        {
            /* the device *should* reappear, do signlling on re-appear only
             * to avoid extra signaling. still there might be a situation
             * when the device will not re-appear if it gets physically removed
             * before it re-appears
             * @todo: set a timer callback to do a notification from it */
        }
    }
    VBOXUSBFLT_LOCK_RELEASE();
    if (pDevice)
        vboxUsbFltDevRelease(pDevice);
    return STATUS_SUCCESS;
}

HVBOXUSBFLTDEV VBoxUsbFltProxyStarted(PDEVICE_OBJECT pPdo)
{
    PVBOXUSBFLT_DEVICE pDevice;
    VBOXUSBFLT_LOCK_ACQUIRE();
    pDevice = vboxUsbFltDevGetLocked(pPdo);
    if (pDevice->enmState = VBOXUSBFLT_DEVSTATE_CAPTURING)
    {
        pDevice->enmState = VBOXUSBFLT_DEVSTATE_CAPTURED;
        vboxUsbFltDevRetain(pDevice);
        vboxUsbFltSignalChangeLocked();
    }
    else
    {
        AssertFailed();
        pDevice = NULL;
    }
    VBOXUSBFLT_LOCK_RELEASE();
    return pDevice;
}

void VBoxUsbFltProxyStopped(HVBOXUSBFLTDEV hDev)
{
    PVBOXUSBFLT_DEVICE pDevice = (PVBOXUSBFLT_DEVICE)hDev;
    VBOXUSBFLT_LOCK_ACQUIRE();
    if (pDevice->enmState == VBOXUSBFLT_DEVSTATE_CAPTURED
            || pDevice->enmState == VBOXUSBFLT_DEVSTATE_USED_BY_GUEST)
    {
        /* this is due to devie was physically removed */
        Log(("The proxy notified progy stop for the captured device 0x%x\n", pDevice));
        pDevice->enmState = VBOXUSBFLT_DEVSTATE_CAPTURING;
        vboxUsbFltSignalChangeLocked();
    }
    else
    {
        Assert(pDevice->enmState == VBOXUSBFLT_DEVSTATE_REPLUGGING);
    }
    VBOXUSBFLT_LOCK_RELEASE();

    vboxUsbFltDevRelease(pDevice);
}

NTSTATUS VBoxUsbFltInit()
{
    int rc = VBoxUSBFilterInit();
    AssertRC(rc);
    if (RT_FAILURE(rc))
        return STATUS_UNSUCCESSFUL;

    memset(&g_VBoxUsbFltGlobals, 0, sizeof (g_VBoxUsbFltGlobals));
    InitializeListHead(&g_VBoxUsbFltGlobals.DeviceList);
    InitializeListHead(&g_VBoxUsbFltGlobals.ContextList);
    InitializeListHead(&g_VBoxUsbFltGlobals.BlackDeviceList);
    vboxUsbFltBlDevPopulateWithKnownLocked();
    VBOXUSBFLT_LOCK_INIT();
    return STATUS_SUCCESS;
}

NTSTATUS VBoxUsbFltTerm()
{
    bool bBusy = false;
    VBOXUSBFLT_LOCK_ACQUIRE();
    do
    {
        if (!IsListEmpty(&g_VBoxUsbFltGlobals.ContextList))
        {
            AssertFailed();
            bBusy = true;
            break;
        }

        PLIST_ENTRY pNext = NULL;
        for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.DeviceList.Flink;
                pEntry != &g_VBoxUsbFltGlobals.DeviceList;
                pEntry = pNext)
        {
            pNext = pEntry->Flink;
            PVBOXUSBFLT_DEVICE pDevice = PVBOXUSBFLT_DEVICE_FROM_LE(pEntry);
            Assert(!pDevice->uFltId);
            Assert(!pDevice->pOwner);
            if (pDevice->cRefs != 1)
            {
                AssertFailed();
                bBusy = true;
                break;
            }
        }
    } while (0);

    VBOXUSBFLT_LOCK_RELEASE()

    if (bBusy)
    {
        return STATUS_DEVICE_BUSY;
    }

    for (PLIST_ENTRY pEntry = g_VBoxUsbFltGlobals.DeviceList.Flink;
            pEntry != &g_VBoxUsbFltGlobals.DeviceList;
            pEntry = g_VBoxUsbFltGlobals.DeviceList.Flink)
    {
        RemoveEntryList(pEntry);
        PVBOXUSBFLT_DEVICE pDevice = PVBOXUSBFLT_DEVICE_FROM_LE(pEntry);
        pDevice->enmState = VBOXUSBFLT_DEVSTATE_REMOVED;
        vboxUsbFltDevRelease(pDevice);
    }

    vboxUsbFltBlDevClearLocked();

    VBOXUSBFLT_LOCK_TERM();

    VBoxUSBFilterTerm();

    return STATUS_SUCCESS;
}

