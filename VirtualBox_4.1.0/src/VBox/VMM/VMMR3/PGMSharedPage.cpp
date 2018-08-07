/* $Id: PGMSharedPage.cpp 36891 2011-04-29 13:22:57Z vboxsync $ */
/** @file
 * PGM - Page Manager and Monitor, Shared page handling
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_PGM_SHARED
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/stam.h>
#include "PGMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/sup.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/string.h>

#include "PGMInline.h"

/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
#if defined(VBOX_STRICT) && HC_ARCH_BITS == 64
/** Keep a copy of all registered shared modules for the .pgmcheckduppages debugger command. */
static PGMMREGISTERSHAREDMODULEREQ g_apSharedModules[512] = {0};
static unsigned g_cSharedModules = 0;
#endif

/**
 * Registers a new shared module for the VM
 *
 * @returns VBox status code.
 * @param   pVM                 VM handle
 * @param   enmGuestOS          Guest OS type
 * @param   pszModuleName       Module name
 * @param   pszVersion          Module version
 * @param   GCBaseAddr          Module base address
 * @param   cbModule            Module size
 * @param   cRegions            Number of shared region descriptors
 * @param   pRegions            Shared region(s)
 */
VMMR3DECL(int) PGMR3SharedModuleRegister(PVM pVM, VBOXOSFAMILY enmGuestOS, char *pszModuleName, char *pszVersion, RTGCPTR GCBaseAddr, uint32_t cbModule,
                                         unsigned cRegions, VMMDEVSHAREDREGIONDESC *pRegions)
{
#ifdef VBOX_WITH_PAGE_SHARING
    PGMMREGISTERSHAREDMODULEREQ pReq;

    Log(("PGMR3SharedModuleRegister family=%d name=%s version=%s base=%RGv size=%x cRegions=%d\n", enmGuestOS, pszModuleName, pszVersion, GCBaseAddr, cbModule, cRegions));

    /* Sanity check. */
    AssertReturn(cRegions < VMMDEVSHAREDREGIONDESC_MAX, VERR_INVALID_PARAMETER);

    pReq = (PGMMREGISTERSHAREDMODULEREQ)RTMemAllocZ(RT_OFFSETOF(GMMREGISTERSHAREDMODULEREQ, aRegions[cRegions]));
    AssertReturn(pReq, VERR_NO_MEMORY);

    pReq->enmGuestOS    = enmGuestOS;
    pReq->GCBaseAddr    = GCBaseAddr;
    pReq->cbModule      = cbModule;
    pReq->cRegions      = cRegions;
    for (unsigned i = 0; i < cRegions; i++)
        pReq->aRegions[i] = pRegions[i];

    if (    RTStrCopy(pReq->szName, sizeof(pReq->szName), pszModuleName) != VINF_SUCCESS
        ||  RTStrCopy(pReq->szVersion, sizeof(pReq->szVersion), pszVersion) != VINF_SUCCESS)
    {
        RTMemFree(pReq);
        return VERR_BUFFER_OVERFLOW;
    }

    int rc = GMMR3RegisterSharedModule(pVM, pReq);
# if defined(VBOX_STRICT) && HC_ARCH_BITS == 64
    if (rc == VINF_SUCCESS)
    {
        PGMMREGISTERSHAREDMODULEREQ *ppSharedModule = NULL;

        if (g_cSharedModules < RT_ELEMENTS(g_apSharedModules))
        {
            for (unsigned i = 0; i < RT_ELEMENTS(g_apSharedModules); i++)
            {
                if (g_apSharedModules[i] == NULL)
                {
                    ppSharedModule = &g_apSharedModules[i];
                    break;
                }
            }
            Assert(ppSharedModule);

            if (ppSharedModule)
            {
                *ppSharedModule = (PGMMREGISTERSHAREDMODULEREQ)RTMemAllocZ(RT_OFFSETOF(GMMREGISTERSHAREDMODULEREQ, aRegions[cRegions]));
                memcpy(*ppSharedModule, pReq, RT_OFFSETOF(GMMREGISTERSHAREDMODULEREQ, aRegions[cRegions]));
                g_cSharedModules++;
            }
        }
    }
# endif

    RTMemFree(pReq);
    Assert(rc == VINF_SUCCESS || rc == VINF_PGM_SHARED_MODULE_COLLISION || rc == VINF_PGM_SHARED_MODULE_ALREADY_REGISTERED);
    if (RT_FAILURE(rc))
        return rc;

    return VINF_SUCCESS;
#else
    return VERR_NOT_IMPLEMENTED;
#endif
}

/**
 * Unregisters a shared module for the VM
 *
 * @returns VBox status code.
 * @param   pVM                 VM handle
 * @param   pszModuleName       Module name
 * @param   pszVersion          Module version
 * @param   GCBaseAddr          Module base address
 * @param   cbModule            Module size
 */
VMMR3DECL(int) PGMR3SharedModuleUnregister(PVM pVM, char *pszModuleName, char *pszVersion, RTGCPTR GCBaseAddr, uint32_t cbModule)
{
#ifdef VBOX_WITH_PAGE_SHARING
    PGMMUNREGISTERSHAREDMODULEREQ pReq;

    Log(("PGMR3SharedModuleUnregister name=%s version=%s base=%RGv size=%x\n", pszModuleName, pszVersion, GCBaseAddr, cbModule));

    pReq = (PGMMUNREGISTERSHAREDMODULEREQ)RTMemAllocZ(sizeof(*pReq));
    AssertReturn(pReq, VERR_NO_MEMORY);

    pReq->GCBaseAddr    = GCBaseAddr;
    pReq->cbModule      = cbModule;

    if (    RTStrCopy(pReq->szName, sizeof(pReq->szName), pszModuleName) != VINF_SUCCESS
        ||  RTStrCopy(pReq->szVersion, sizeof(pReq->szVersion), pszVersion) != VINF_SUCCESS)
    {
        RTMemFree(pReq);
        return VERR_BUFFER_OVERFLOW;
    }
    int rc = GMMR3UnregisterSharedModule(pVM, pReq);
    RTMemFree(pReq);

# if defined(VBOX_STRICT) && HC_ARCH_BITS == 64
    for (unsigned i = 0; i < g_cSharedModules; i++)
    {
        if (    g_apSharedModules[i]
            &&  !strcmp(g_apSharedModules[i]->szName, pszModuleName)
            &&  !strcmp(g_apSharedModules[i]->szVersion, pszVersion))
        {
            RTMemFree(g_apSharedModules[i]);
            g_apSharedModules[i] = NULL;
            g_cSharedModules--;
            break;
        }
    }
# endif
    return rc;
#else
    return VERR_NOT_IMPLEMENTED;
#endif
}

#ifdef VBOX_WITH_PAGE_SHARING
/**
 * Rendezvous callback that will be called once.
 *
 * @returns VBox strict status code.
 * @param   pVM                 VM handle.
 * @param   pVCpu               The VMCPU handle for the calling EMT.
 * @param   pvUser              Not used;
 */
static DECLCALLBACK(VBOXSTRICTRC) pgmR3SharedModuleRegRendezvous(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    VMCPUID idCpu = *(VMCPUID *)pvUser;

    /* Execute on the VCPU that issued the original request to make sure we're in the right cr3 context. */
    if (pVCpu->idCpu != idCpu)
    {
        Assert(pVM->cCpus > 1);
        return VINF_SUCCESS;
    }

    /* Flush all pending handy page operations before changing any shared page assignments. */
    int rc = PGMR3PhysAllocateHandyPages(pVM);
    AssertRC(rc);

    /* Lock it here as we can't deal with busy locks in this ring-0 path. */
    pgmLock(pVM);
    rc = GMMR3CheckSharedModules(pVM);
    pgmUnlock(pVM);
    AssertLogRelRC(rc);
    return rc;
}

/**
 * Shared module check helper (called on the way out).
 *
 * @param   pVM         The VM handle.
 * @param   VMCPUID     VCPU id
 */
static DECLCALLBACK(void) pgmR3CheckSharedModulesHelper(PVM pVM, VMCPUID idCpu)
{
    /* We must stall other VCPUs as we'd otherwise have to send IPI flush commands for every single change we make. */
    int rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE, pgmR3SharedModuleRegRendezvous, &idCpu);
    Assert(rc == VINF_SUCCESS);
}
#endif

/**
 * Check all registered modules for changes.
 *
 * @returns VBox status code.
 * @param   pVM                 VM handle
 */
VMMR3DECL(int) PGMR3SharedModuleCheckAll(PVM pVM)
{
#ifdef VBOX_WITH_PAGE_SHARING
    /* Queue the actual registration as we are under the IOM lock right now. Perform this operation on the way out. */
    return VMR3ReqCallNoWait(pVM, VMCPUID_ANY_QUEUE, (PFNRT)pgmR3CheckSharedModulesHelper, 2, pVM, VMMGetCpuId(pVM));
#else
    return VERR_NOT_IMPLEMENTED;
#endif
}

/**
 * Query the state of a page in a shared module
 *
 * @returns VBox status code.
 * @param   pVM                 VM handle
 * @param   GCPtrPage           Page address
 * @param   pfShared            Shared status (out)
 * @param   puPageFlags         Page flags (out)
 */
VMMR3DECL(int) PGMR3SharedModuleGetPageState(PVM pVM, RTGCPTR GCPtrPage, bool *pfShared, uint64_t *puPageFlags)
{
#if defined(VBOX_WITH_PAGE_SHARING) && defined(DEBUG)
    /* Debug only API for the page fusion testcase. */
    RTGCPHYS GCPhys;
    uint64_t fFlags;

    pgmLock(pVM);

    int rc = PGMGstGetPage(VMMGetCpu(pVM), GCPtrPage, &fFlags, &GCPhys);
    switch (rc)
    {
    case VINF_SUCCESS:
    {
        PPGMPAGE pPage = pgmPhysGetPage(pVM, GCPhys);
        if (pPage)
        {
            *pfShared    = PGM_PAGE_IS_SHARED(pPage);
            *puPageFlags = fFlags;
        }
        else
            rc = VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS;
        break;
    }
    case VERR_PAGE_NOT_PRESENT:
    case VERR_PAGE_TABLE_NOT_PRESENT:
    case VERR_PAGE_MAP_LEVEL4_NOT_PRESENT:
    case VERR_PAGE_DIRECTORY_PTR_NOT_PRESENT:
        *pfShared = false;
        *puPageFlags = 0;
        rc = VINF_SUCCESS;
        break;

    default:
        break;
    }

    pgmUnlock(pVM);
    return rc;
#else
    return VERR_NOT_IMPLEMENTED;
#endif
}


#if defined(VBOX_STRICT) && HC_ARCH_BITS == 64
/**
 * The '.pgmcheckduppages' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
DECLCALLBACK(int)  pgmR3CmdCheckDuplicatePages(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs)
{
    unsigned cBallooned = 0;
    unsigned cShared = 0;
    unsigned cZero = 0;
    unsigned cUnique = 0;
    unsigned cDuplicate = 0;
    unsigned cAllocZero = 0;
    unsigned cPages = 0;

    pgmLock(pVM);

    for (PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR3; pRam; pRam = pRam->pNextR3)
    {
        PPGMPAGE    pPage  = &pRam->aPages[0];
        RTGCPHYS    GCPhys = pRam->GCPhys;
        uint32_t    cLeft  = pRam->cb >> PAGE_SHIFT;
        while (cLeft-- > 0)
        {
            if (PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_RAM)
            {
                switch (PGM_PAGE_GET_STATE(pPage))
                {
                    case PGM_PAGE_STATE_ZERO:
                        cZero++;
                        break;

                    case PGM_PAGE_STATE_BALLOONED:
                        cBallooned++;
                        break;

                    case PGM_PAGE_STATE_SHARED:
                        cShared++;
                        break;

                    case PGM_PAGE_STATE_ALLOCATED:
                    case PGM_PAGE_STATE_WRITE_MONITORED:
                    {
                        const void *pvPage;
                        /* Check if the page was allocated, but completely zero. */
                        int rc = pgmPhysGCPhys2CCPtrInternalReadOnly(pVM, pPage, GCPhys, &pvPage);
                        if (    rc == VINF_SUCCESS
                            &&  ASMMemIsZeroPage(pvPage))
                        {
                            cAllocZero++;
                        }
                        else
                        if (GMMR3IsDuplicatePage(pVM, PGM_PAGE_GET_PAGEID(pPage)))
                            cDuplicate++;
                        else
                            cUnique++;

                        break;
                    }

                    default:
                        AssertFailed();
                        break;
                }
            }

            /* next */
            pPage++;
            GCPhys += PAGE_SIZE;
            cPages++;
            /* Give some feedback for every processed megabyte. */
            if ((cPages & 0x7f) == 0)
                pCmdHlp->pfnPrintf(pCmdHlp, NULL, ".");
        }
    }
    pgmUnlock(pVM);

    pCmdHlp->pfnPrintf(pCmdHlp, NULL, "\nNumber of zero pages      %08x (%d MB)\n", cZero, cZero / 256);
    pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Number of alloczero pages %08x (%d MB)\n", cAllocZero, cAllocZero / 256);
    pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Number of ballooned pages %08x (%d MB)\n", cBallooned, cBallooned / 256);
    pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Number of shared pages    %08x (%d MB)\n", cShared, cShared / 256);
    pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Number of unique pages    %08x (%d MB)\n", cUnique, cUnique / 256);
    pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Number of duplicate pages %08x (%d MB)\n", cDuplicate, cDuplicate / 256);
    return VINF_SUCCESS;
}

/**
 * The '.pgmsharedmodules' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
DECLCALLBACK(int)  pgmR3CmdShowSharedModules(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs)
{
    unsigned i = 0;

    pgmLock(pVM);
    do
    {
        if (g_apSharedModules[i])
        {
            pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Shared module %s (%s):\n", g_apSharedModules[i]->szName, g_apSharedModules[i]->szVersion);
            for (unsigned j = 0; j < g_apSharedModules[i]->cRegions; j++)
                pCmdHlp->pfnPrintf(pCmdHlp, NULL, "--- Region %d: base %RGv size %x\n", j, g_apSharedModules[i]->aRegions[j].GCRegionAddr, g_apSharedModules[i]->aRegions[j].cbRegion);
        }
        i++;
    } while (i < RT_ELEMENTS(g_apSharedModules));
    pgmUnlock(pVM);

    return VINF_SUCCESS;
}

#endif /* VBOX_STRICT && HC_ARCH_BITS == 64*/
