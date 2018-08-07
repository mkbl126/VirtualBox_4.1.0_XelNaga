/* $Id: VBoxMPShgsmi.h 36867 2011-04-28 07:27:03Z vboxsync $ */

/** @file
 * VBox WDDM Miniport driver
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

#ifndef ___VBoxMPShgsmi_h___
#define ___VBoxMPShgsmi_h___

#include <iprt/cdefs.h>
#include <VBox/VBoxVideo.h>

typedef DECLCALLBACK(void) FNVBOXSHGSMICMDCOMPLETION(struct _HGSMIHEAP * pHeap, void *pvCmd, void *pvContext);
typedef FNVBOXSHGSMICMDCOMPLETION *PFNVBOXSHGSMICMDCOMPLETION;

typedef DECLCALLBACK(void) FNVBOXSHGSMICMDCOMPLETION_IRQ(struct _HGSMIHEAP * pHeap, void *pvCmd, void *pvContext,
                                        PFNVBOXSHGSMICMDCOMPLETION *ppfnCompletion, void **ppvCompletion);
typedef FNVBOXSHGSMICMDCOMPLETION_IRQ *PFNVBOXSHGSMICMDCOMPLETION_IRQ;

typedef struct VBOXSHGSMILIST_ENTRY
{
    struct VBOXSHGSMILIST_ENTRY *pNext;
} VBOXSHGSMILIST_ENTRY, *PVBOXSHGSMILIST_ENTRY;

typedef struct VBOXSHGSMILIST
{
    PVBOXSHGSMILIST_ENTRY pFirst;
    PVBOXSHGSMILIST_ENTRY pLast;
} VBOXSHGSMILIST, *PVBOXSHGSMILIST;

DECLINLINE(bool) vboxSHGSMIListIsEmpty(PVBOXSHGSMILIST pList)
{
    return !pList->pFirst;
}

DECLINLINE(void) vboxSHGSMIListInit(PVBOXSHGSMILIST pList)
{
    pList->pFirst = pList->pLast = NULL;
}

DECLINLINE(void) vboxSHGSMIListPut(PVBOXSHGSMILIST pList, PVBOXSHGSMILIST_ENTRY pFirst, PVBOXSHGSMILIST_ENTRY pLast)
{
    Assert(pFirst);
    Assert(pLast);
    pLast->pNext = NULL;
    if (pList->pLast)
    {
        Assert(pList->pFirst);
        pList->pLast->pNext = pFirst;
        pList->pLast = pLast;
    }
    else
    {
        Assert(!pList->pFirst);
        pList->pFirst = pFirst;
        pList->pLast = pLast;
    }
}

DECLINLINE(void) vboxSHGSMIListCat(PVBOXSHGSMILIST pList1, PVBOXSHGSMILIST pList2)
{
    vboxSHGSMIListPut(pList1, pList2->pFirst, pList2->pLast);
    pList2->pFirst = pList2->pLast = NULL;
}

DECLINLINE(void) vboxSHGSMIListDetach(PVBOXSHGSMILIST pList, PVBOXSHGSMILIST_ENTRY *ppFirst, PVBOXSHGSMILIST_ENTRY *ppLast)
{
    *ppFirst = pList->pFirst;
    if (ppLast)
        *ppLast = pList->pLast;
    pList->pFirst = NULL;
    pList->pLast = NULL;
}

DECLINLINE(void) vboxSHGSMIListDetach2List(PVBOXSHGSMILIST pList, PVBOXSHGSMILIST pDstList)
{
    vboxSHGSMIListDetach(pList, &pDstList->pFirst, &pDstList->pLast);
}

DECLINLINE(void) vboxSHGSMIListDetachEntries(PVBOXSHGSMILIST pList, PVBOXSHGSMILIST_ENTRY pBeforeDetach, PVBOXSHGSMILIST_ENTRY pLast2Detach)
{
    if (pBeforeDetach)
    {
        pBeforeDetach->pNext = pLast2Detach->pNext;
        if (!pBeforeDetach->pNext)
            pList->pLast = pBeforeDetach;
    }
    else
    {
        pList->pFirst = pLast2Detach->pNext;
        if (!pList->pFirst)
            pList->pLast = NULL;
    }
    pLast2Detach->pNext = NULL;
}


const VBOXSHGSMIHEADER* VBoxSHGSMICommandPrepAsynchEvent(struct _HGSMIHEAP * pHeap, PVOID pvBuff, RTSEMEVENT hEventSem);
const VBOXSHGSMIHEADER* VBoxSHGSMICommandPrepSynch(struct _HGSMIHEAP * pHeap, PVOID pCmd);
const VBOXSHGSMIHEADER* VBoxSHGSMICommandPrepAsynch(struct _HGSMIHEAP * pHeap, PVOID pvBuff, PFNVBOXSHGSMICMDCOMPLETION pfnCompletion, PVOID pvCompletion, uint32_t fFlags);
const VBOXSHGSMIHEADER* VBoxSHGSMICommandPrepAsynchIrq(struct _HGSMIHEAP * pHeap, PVOID pvBuff, PFNVBOXSHGSMICMDCOMPLETION_IRQ pfnCompletion, PVOID pvCompletion, uint32_t fFlags);

void VBoxSHGSMICommandDoneAsynch(struct _HGSMIHEAP * pHeap, const VBOXSHGSMIHEADER* pHeader);
int VBoxSHGSMICommandDoneSynch(struct _HGSMIHEAP * pHeap, const VBOXSHGSMIHEADER* pHeader);
void VBoxSHGSMICommandCancelAsynch(struct _HGSMIHEAP * pHeap, const VBOXSHGSMIHEADER* pHeader);
void VBoxSHGSMICommandCancelSynch(struct _HGSMIHEAP * pHeap, const VBOXSHGSMIHEADER* pHeader);

DECLINLINE(HGSMIOFFSET) VBoxSHGSMICommandOffset(struct _HGSMIHEAP * pHeap, const VBOXSHGSMIHEADER* pHeader)
{
    return HGSMIHeapBufferOffset(pHeap, (void*)pHeader);
}

void* VBoxSHGSMICommandAlloc(struct _HGSMIHEAP * pHeap, HGSMISIZE cbData, uint8_t u8Channel, uint16_t u16ChannelInfo);
void VBoxSHGSMICommandFree(struct _HGSMIHEAP * pHeap, void *pvBuffer);
int VBoxSHGSMICommandProcessCompletion(struct _HGSMIHEAP * pHeap, VBOXSHGSMIHEADER* pCmd, bool bIrq, PVBOXSHGSMILIST pPostProcessList);
int VBoxSHGSMICommandPostprocessCompletion(struct _HGSMIHEAP * pHeap, PVBOXSHGSMILIST pPostProcessList);

#endif /* #ifndef ___VBoxMPShgsmi_h___ */
