/* $Id: TRPMRCHandlers.cpp 37955 2011-07-14 12:23:02Z vboxsync $ */
/** @file
 * TRPM - Guest Context Trap Handlers, CPP part
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
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_TRPM
#include <VBox/vmm/selm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/csam.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/cpum.h>
#include "TRPMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/param.h>

#include <VBox/err.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <VBox/log.h>
#include <VBox/vmm/tm.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>
#include <iprt/x86.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/* still here. MODR/M byte parsing */
#define X86_OPCODE_MODRM_MOD_MASK       0xc0
#define X86_OPCODE_MODRM_REG_MASK       0x38
#define X86_OPCODE_MODRM_RM_MASK        0x07

/** @todo fix/remove/permanent-enable this when DIS/PATM handles invalid lock sequences. */
#define DTRACE_EXPERIMENT


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Pointer to a readonly hypervisor trap record. */
typedef const struct TRPMGCHYPER *PCTRPMGCHYPER;

/**
 * A hypervisor trap record.
 * This contains information about a handler for a instruction range.
 *
 * @remark This must match what TRPM_HANDLER outputs.
 */
typedef struct TRPMGCHYPER
{
    /** The start address. */
    uintptr_t uStartEIP;
    /** The end address. (exclusive)
     * If NULL the it's only for the instruction at pvStartEIP. */
    uintptr_t uEndEIP;
    /**
     * The handler.
     *
     * @returns VBox status code
     *          VINF_SUCCESS means we've handled the trap.
     *          Any other error code means returning to the host context.
     * @param   pVM             The VM handle.
     * @param   pRegFrame       The register frame.
     * @param   uUser           The user argument.
     */
    DECLRCCALLBACKMEMBER(int, pfnHandler, (PVM pVM, PCPUMCTXCORE pRegFrame, uintptr_t uUser));
    /** Whatever the handler desires to put here. */
    uintptr_t uUser;
} TRPMGCHYPER;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
RT_C_DECLS_BEGIN
/** Defined in VMMGC0.asm or VMMGC99.asm.
 * @{ */
extern const TRPMGCHYPER g_aTrap0bHandlers[1];
extern const TRPMGCHYPER g_aTrap0bHandlersEnd[1];
extern const TRPMGCHYPER g_aTrap0dHandlers[1];
extern const TRPMGCHYPER g_aTrap0dHandlersEnd[1];
extern const TRPMGCHYPER g_aTrap0eHandlers[1];
extern const TRPMGCHYPER g_aTrap0eHandlersEnd[1];
/** @} */
RT_C_DECLS_END


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
RT_C_DECLS_BEGIN /* addressed from asm (not called so no DECLASM). */
DECLCALLBACK(int) trpmGCTrapInGeneric(PVM pVM, PCPUMCTXCORE pRegFrame, uintptr_t uUser);
RT_C_DECLS_END



/**
 * Exits the trap, called when exiting a trap handler.
 *
 * Will reset the trap if it's not a guest trap or the trap
 * is already handled. Will process resume guest FFs.
 *
 * @returns rc, can be adjusted if its VINF_SUCCESS or something really bad
 *          happened.
 * @param   pVM         VM handle.
 * @param   pVCpu       The virtual CPU handle.
 * @param   rc          The VBox status code to return.
 * @param   pRegFrame   Pointer to the register frame for the trap.
 *
 * @remarks This must not be used for hypervisor traps, only guest traps.
 */
static int trpmGCExitTrap(PVM pVM, PVMCPU pVCpu, int rc, PCPUMCTXCORE pRegFrame)
{
    uint32_t uOldActiveVector = pVCpu->trpm.s.uActiveVector;
    NOREF(uOldActiveVector);

    /* Reset trap? */
    if (    rc != VINF_EM_RAW_GUEST_TRAP
        &&  rc != VINF_EM_RAW_RING_SWITCH_INT)
        pVCpu->trpm.s.uActiveVector = ~0;

#ifdef VBOX_HIGH_RES_TIMERS_HACK
    /*
     * We should poll the timers occasionally.
     * We must *NOT* do this too frequently as it adds a significant overhead
     * and it'll kill us if the trap load is high. (See #1354.)
     * (The heuristic is not very intelligent, we should really check trap
     * frequency etc. here, but alas, we lack any such information atm.)
     */
    static unsigned s_iTimerPoll = 0;
    if (rc == VINF_SUCCESS)
    {
        if (!(++s_iTimerPoll & 0xf))
        {
            TMTimerPollVoid(pVM, pVCpu);
            Log2(("TMTimerPoll at %08RX32 - VM_FF_TM_VIRTUAL_SYNC=%d VM_FF_TM_VIRTUAL_SYNC=%d\n", pRegFrame->eip,
                  VM_FF_ISPENDING(pVM, VM_FF_TM_VIRTUAL_SYNC), VMCPU_FF_ISPENDING(pVCpu, VMCPU_FF_TIMER)));
        }
    }
    else
        s_iTimerPoll = 0;
#endif

    /* Clear pending inhibit interrupt state if required. (necessary for dispatching interrupts later on) */
    if (VMCPU_FF_ISSET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
    {
        Log2(("VM_FF_INHIBIT_INTERRUPTS at %08RX32 successor %RGv\n", pRegFrame->eip, EMGetInhibitInterruptsPC(pVCpu)));
        if (pRegFrame->eip != EMGetInhibitInterruptsPC(pVCpu))
        {
            /** @note we intentionally don't clear VM_FF_INHIBIT_INTERRUPTS here if the eip is the same as the inhibited instr address.
             *  Before we are able to execute this instruction in raw mode (iret to guest code) an external interrupt might
             *  force a world switch again. Possibly allowing a guest interrupt to be dispatched in the process. This could
             *  break the guest. Sounds very unlikely, but such timing sensitive problem are not as rare as you might think.
             */
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
        }
    }

    /*
     * Pending resume-guest-FF?
     * Or pending (A)PIC interrupt? Windows XP will crash if we delay APIC interrupts.
     */
    if (    rc == VINF_SUCCESS
        &&  (   VM_FF_ISPENDING(pVM, VM_FF_TM_VIRTUAL_SYNC | VM_FF_REQUEST | VM_FF_PGM_NO_MEMORY | VM_FF_PDM_DMA)
             || VMCPU_FF_ISPENDING(pVCpu, VMCPU_FF_TIMER | VMCPU_FF_TO_R3 | VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC
                                          | VMCPU_FF_REQUEST | VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL
                                          | VMCPU_FF_PDM_CRITSECT)
            )
       )
    {
        /* The out of memory condition naturally outranks the others. */
        if (RT_UNLIKELY(VM_FF_ISPENDING(pVM, VM_FF_PGM_NO_MEMORY)))
            rc = VINF_EM_NO_MEMORY;
        /* Pending Ring-3 action. */
        else if (VMCPU_FF_ISPENDING(pVCpu, VMCPU_FF_TO_R3 | VMCPU_FF_PDM_CRITSECT))
        {
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TO_R3);
            rc = VINF_EM_RAW_TO_R3;
        }
        /* Pending timer action. */
        else if (VMCPU_FF_ISPENDING(pVCpu, VMCPU_FF_TIMER))
            rc = VINF_EM_RAW_TIMER_PENDING;
        /* The Virtual Sync clock has stopped. */
        else if (VM_FF_ISPENDING(pVM, VM_FF_TM_VIRTUAL_SYNC))
            rc = VINF_EM_RAW_TO_R3;
        /* DMA work pending? */
        else if (VM_FF_ISPENDING(pVM, VM_FF_PDM_DMA))
            rc = VINF_EM_RAW_TO_R3;
        /* Pending request packets might contain actions that need immediate
           attention, such as pending hardware interrupts. */
        else if (   VM_FF_ISPENDING(pVM, VM_FF_REQUEST)
                 || VMCPU_FF_ISPENDING(pVCpu, VMCPU_FF_REQUEST))
            rc = VINF_EM_PENDING_REQUEST;
        /* Pending interrupt: dispatch it. */
        else if (    VMCPU_FF_ISPENDING(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC)
                 && !VMCPU_FF_ISSET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
                 &&  PATMAreInterruptsEnabledByCtxCore(pVM, pRegFrame)
           )
        {
            uint8_t u8Interrupt;
            rc = PDMGetInterrupt(pVCpu, &u8Interrupt);
            Log(("trpmGCExitTrap: u8Interrupt=%d (%#x) rc=%Rrc\n", u8Interrupt, u8Interrupt, rc));
            AssertFatalMsgRC(rc, ("PDMGetInterrupt failed with %Rrc\n", rc));
            rc = TRPMForwardTrap(pVCpu, pRegFrame, (uint32_t)u8Interrupt, 0, TRPM_TRAP_NO_ERRORCODE, TRPM_HARDWARE_INT, uOldActiveVector);
            /* can't return if successful */
            Assert(rc != VINF_SUCCESS);

            /* Stop the profile counter that was started in TRPMGCHandlersA.asm */
            Assert(uOldActiveVector <= 16);
            STAM_PROFILE_ADV_STOP(&pVM->trpm.s.aStatGCTraps[uOldActiveVector], a);

            /* Assert the trap and go to the recompiler to dispatch it. */
            TRPMAssertTrap(pVCpu, u8Interrupt, TRPM_HARDWARE_INT);

            STAM_PROFILE_ADV_START(&pVM->trpm.s.aStatGCTraps[uOldActiveVector], a);
            rc = VINF_EM_RAW_INTERRUPT_PENDING;
        }
        /*
         * Try sync CR3?
         */
        else if (VMCPU_FF_ISPENDING(pVCpu, VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL))
        {
#if 1
            PGMRZDynMapReleaseAutoSet(pVCpu);
            PGMRZDynMapStartAutoSet(pVCpu);
            rc = PGMSyncCR3(pVCpu, CPUMGetGuestCR0(pVCpu), CPUMGetGuestCR3(pVCpu), CPUMGetGuestCR4(pVCpu), VMCPU_FF_ISSET(pVCpu, VMCPU_FF_PGM_SYNC_CR3));
#else
            rc = VINF_PGM_SYNC_CR3;
#endif
        }
    }

    AssertMsg(     rc != VINF_SUCCESS
              ||   (   pRegFrame->eflags.Bits.u1IF
                    && ( pRegFrame->eflags.Bits.u2IOPL < (unsigned)(pRegFrame->ss & X86_SEL_RPL) || pRegFrame->eflags.Bits.u1VM))
              , ("rc=%Rrc\neflags=%RX32 ss=%RTsel IOPL=%d\n", rc, pRegFrame->eflags.u32, pRegFrame->ss, pRegFrame->eflags.Bits.u2IOPL));
    PGMRZDynMapReleaseAutoSet(pVCpu);
    return rc;
}


/**
 * \#DB (Debug event) handler.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCTrap01Handler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    RTGCUINTREG uDr6  = ASMGetAndClearDR6();
    PVM         pVM   = TRPMCPU_2_VM(pTrpmCpu);
    PVMCPU      pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);

    LogFlow(("TRPMGC01: cs:eip=%04x:%08x uDr6=%RTreg\n", pRegFrame->cs, pRegFrame->eip, uDr6));

    /*
     * We currently don't make use of the X86_DR7_GD bit, but
     * there might come a time when we do.
     */
    AssertReleaseMsgReturn((uDr6 & X86_DR6_BD) != X86_DR6_BD,
                           ("X86_DR6_BD isn't used, but it's set! dr7=%RTreg(%RTreg) dr6=%RTreg\n",
                            ASMGetDR7(), CPUMGetHyperDR7(pVCpu), uDr6),
                           VERR_NOT_IMPLEMENTED);
    AssertReleaseMsg(!(uDr6 & X86_DR6_BT), ("X86_DR6_BT is impossible!\n"));

    /*
     * Now leave the rest to the DBGF.
     */
    PGMRZDynMapStartAutoSet(pVCpu);
    int rc = DBGFRZTrap01Handler(pVM, pVCpu, pRegFrame, uDr6);
    if (rc == VINF_EM_RAW_GUEST_TRAP)
        CPUMSetGuestDR6(pVCpu, uDr6);

    rc = trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
    Log6(("TRPMGC01: %Rrc (%04x:%08x %RTreg)\n", rc, pRegFrame->cs, pRegFrame->eip, uDr6));
    return rc;
}


/**
 * \#DB (Debug event) handler for the hypervisor code.
 *
 * This is mostly the same as TRPMGCTrap01Handler, but we skip the PGM auto
 * mapping set as well as the default trap exit path since they are both really
 * bad ideas in this context.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCHyperTrap01Handler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    RTGCUINTREG uDr6  = ASMGetAndClearDR6();
    PVM         pVM   = TRPMCPU_2_VM(pTrpmCpu);
    PVMCPU      pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);

    LogFlow(("TRPMGCHyper01: cs:eip=%04x:%08x uDr6=%RTreg\n", pRegFrame->cs, pRegFrame->eip, uDr6));

    /*
     * We currently don't make use of the X86_DR7_GD bit, but
     * there might come a time when we do.
     */
    AssertReleaseMsgReturn((uDr6 & X86_DR6_BD) != X86_DR6_BD,
                           ("X86_DR6_BD isn't used, but it's set! dr7=%RTreg(%RTreg) dr6=%RTreg\n",
                            ASMGetDR7(), CPUMGetHyperDR7(pVCpu), uDr6),
                           VERR_NOT_IMPLEMENTED);
    AssertReleaseMsg(!(uDr6 & X86_DR6_BT), ("X86_DR6_BT is impossible!\n"));

    /*
     * Now leave the rest to the DBGF.
     */
    int rc = DBGFRZTrap01Handler(pVM, pVCpu, pRegFrame, uDr6);
    AssertStmt(rc != VINF_EM_RAW_GUEST_TRAP, rc = VERR_INTERNAL_ERROR_3);

    Log6(("TRPMGCHyper01: %Rrc (%04x:%08x %RTreg)\n", rc, pRegFrame->cs, pRegFrame->eip, uDr6));
    return rc;
}


/**
 * NMI handler, for when we are using NMIs to debug things.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 * @remark  This is not hooked up unless you're building with VBOX_WITH_NMI defined.
 */
DECLASM(int) TRPMGCTrap02Handler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    LogFlow(("TRPMGCTrap02Handler: cs:eip=%04x:%08x\n", pRegFrame->cs, pRegFrame->eip));
    RTLogComPrintf("TRPMGCTrap02Handler: cs:eip=%04x:%08x\n", pRegFrame->cs, pRegFrame->eip);
    return VERR_TRPM_DONT_PANIC;
}


/**
 * NMI handler, for when we are using NMIs to debug things.
 *
 * This is the handler we're most likely to hit when the NMI fires (it is
 * unlikely that we'll be stuck in guest code).
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 * @remark  This is not hooked up unless you're building with VBOX_WITH_NMI defined.
 */
DECLASM(int) TRPMGCHyperTrap02Handler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    LogFlow(("TRPMGCHyperTrap02Handler: cs:eip=%04x:%08x\n", pRegFrame->cs, pRegFrame->eip));
    RTLogComPrintf("TRPMGCHyperTrap02Handler: cs:eip=%04x:%08x\n", pRegFrame->cs, pRegFrame->eip);
    return VERR_TRPM_DONT_PANIC;
}


/**
 * \#BP (Breakpoint) handler.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCTrap03Handler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    LogFlow(("TRPMGC03: %04x:%08x\n", pRegFrame->cs, pRegFrame->eip));
    PVM     pVM   = TRPMCPU_2_VM(pTrpmCpu);
    PVMCPU  pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);
    int     rc;
    PGMRZDynMapStartAutoSet(pVCpu);

    /*
     * PATM is using INT3s, let them have a go first.
     */
    if (    (pRegFrame->ss & X86_SEL_RPL) == 1
        &&  !pRegFrame->eflags.Bits.u1VM)
    {
        rc = PATMHandleInt3PatchTrap(pVM, pRegFrame);
        if (rc == VINF_SUCCESS || rc == VINF_EM_RAW_EMULATE_INSTR || rc == VINF_PATM_PATCH_INT3 || rc == VINF_PATM_DUPLICATE_FUNCTION)
        {
            rc = trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
            Log6(("TRPMGC03: %Rrc (%04x:%08x) (PATM)\n", rc, pRegFrame->cs, pRegFrame->eip));
            return rc;
        }
    }
    rc = DBGFRZTrap03Handler(pVM, pVCpu, pRegFrame);

    /* anything we should do with this? Schedule it in GC? */
    rc = trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
    Log6(("TRPMGC03: %Rrc (%04x:%08x)\n", rc, pRegFrame->cs, pRegFrame->eip));
    return rc;
}


/**
 * \#BP (Breakpoint) handler.
 *
 * This is similar to TRPMGCTrap03Handler but we bits which are potentially
 * harmful to us (common trap exit and the auto mapping set).
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCHyperTrap03Handler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    LogFlow(("TRPMGCHyper03: %04x:%08x\n", pRegFrame->cs, pRegFrame->eip));
    PVM     pVM   = TRPMCPU_2_VM(pTrpmCpu);
    PVMCPU  pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);

    /*
     * Hand it over to DBGF.
     */
    int rc = DBGFRZTrap03Handler(pVM, pVCpu, pRegFrame);
    AssertStmt(rc != VINF_EM_RAW_GUEST_TRAP, rc = VERR_INTERNAL_ERROR_3);

    Log6(("TRPMGCHyper03: %Rrc (%04x:%08x)\n", rc, pRegFrame->cs, pRegFrame->eip));
    return rc;
}


/**
 * Trap handler for illegal opcode fault (\#UD).
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCTrap06Handler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    LogFlow(("TRPMGC06: %04x:%08x efl=%x\n", pRegFrame->cs, pRegFrame->eip, pRegFrame->eflags.u32));
    PVM     pVM   = TRPMCPU_2_VM(pTrpmCpu);
    PVMCPU  pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);
    int     rc;
    PGMRZDynMapStartAutoSet(pVCpu);

    if (CPUMGetGuestCPL(pVCpu, pRegFrame) == 0)
    {
        /*
         * Decode the instruction.
         */
        RTGCPTR PC;
        rc = SELMValidateAndConvertCSAddr(pVM, pRegFrame->eflags, pRegFrame->ss, pRegFrame->cs, &pRegFrame->csHid,
                                          (RTGCPTR)pRegFrame->eip, &PC);
        if (RT_FAILURE(rc))
        {
            Log(("TRPMGCTrap06Handler: Failed to convert %RTsel:%RX32 (cpl=%d) - rc=%Rrc !!\n", pRegFrame->cs, pRegFrame->eip, pRegFrame->ss & X86_SEL_RPL, rc));
            rc = trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_GUEST_TRAP, pRegFrame);
            Log6(("TRPMGC06: %Rrc (%04x:%08x) (SELM)\n", rc, pRegFrame->cs, pRegFrame->eip));
            return rc;
        }

        DISCPUSTATE Cpu;
        uint32_t    cbOp;
        rc = EMInterpretDisasOneEx(pVM, pVCpu, (RTGCUINTPTR)PC, pRegFrame, &Cpu, &cbOp);
        if (RT_FAILURE(rc))
        {
            rc = trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_EMULATE_INSTR, pRegFrame);
            Log6(("TRPMGC06: %Rrc (%04x:%08x) (EM)\n", rc, pRegFrame->cs, pRegFrame->eip));
            return rc;
        }

        /*
         * UD2 in a patch?
         * Note! PATMGCHandleIllegalInstrTrap doesn't always return.
         */
        if (    Cpu.pCurInstr->opcode == OP_ILLUD2
            &&  PATMIsPatchGCAddr(pVM, pRegFrame->eip))
        {
            LogFlow(("TRPMGCTrap06Handler: -> PATMGCHandleIllegalInstrTrap\n"));
            rc = PATMGCHandleIllegalInstrTrap(pVM, pRegFrame);
            /** @todo  These tests are completely unnecessary, should just follow the
             *         flow and return at the end of the function. */
            if (    rc == VINF_SUCCESS
                ||  rc == VINF_EM_RAW_EMULATE_INSTR
                ||  rc == VINF_PATM_DUPLICATE_FUNCTION
                ||  rc == VINF_PATM_PENDING_IRQ_AFTER_IRET
                ||  rc == VINF_EM_RESCHEDULE)
            {
                rc = trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
                Log6(("TRPMGC06: %Rrc (%04x:%08x) (PATM)\n", rc, pRegFrame->cs, pRegFrame->eip));
                return rc;
            }
        }
        /*
         * Speed up dtrace and don't entrust invalid lock sequences to the recompiler.
         */
        else if (Cpu.prefix & PREFIX_LOCK)
        {
            Log(("TRPMGCTrap06Handler: pc=%08x op=%d\n", pRegFrame->eip, Cpu.pCurInstr->opcode));
#ifdef DTRACE_EXPERIMENT /** @todo fix/remove/permanent-enable this when DIS/PATM handles invalid lock sequences. */
            Assert(!PATMIsPatchGCAddr(pVM, pRegFrame->eip));
            rc = TRPMForwardTrap(pVCpu, pRegFrame, 0x6, 0, TRPM_TRAP_NO_ERRORCODE, TRPM_TRAP, 0x6);
            Assert(rc == VINF_EM_RAW_GUEST_TRAP);
#else
            rc = VINF_EM_RAW_EMULATE_INSTR;
#endif
        }
        /*
         * Handle MONITOR - it causes an #UD exception instead of #GP when not executed in ring 0.
         */
        else if (Cpu.pCurInstr->opcode == OP_MONITOR)
        {
            LogFlow(("TRPMGCTrap06Handler: -> EMInterpretInstructionCPU\n"));
            uint32_t cbIgnored;
            rc = EMInterpretInstructionCPU(pVM, pVCpu, &Cpu, pRegFrame, PC, EMCODETYPE_SUPERVISOR, &cbIgnored);
            if (RT_SUCCESS(rc))
                pRegFrame->eip += Cpu.opsize;
        }
        /* Never generate a raw trap here; it might be an instruction, that requires emulation. */
        else
        {
            LogFlow(("TRPMGCTrap06Handler: -> VINF_EM_RAW_EMULATE_INSTR\n"));
            rc = VINF_EM_RAW_EMULATE_INSTR;
        }
    }
    else
    {
        LogFlow(("TRPMGCTrap06Handler: -> TRPMForwardTrap\n"));
        rc = TRPMForwardTrap(pVCpu, pRegFrame, 0x6, 0, TRPM_TRAP_NO_ERRORCODE, TRPM_TRAP, 0x6);
        Assert(rc == VINF_EM_RAW_GUEST_TRAP);
    }

    rc = trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
    Log6(("TRPMGC06: %Rrc (%04x:%08x)\n", rc, pRegFrame->cs, pRegFrame->eip));
    return rc;
}


/**
 * Trap handler for device not present fault (\#NM).
 *
 * Device not available, FP or (F)WAIT instruction.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCTrap07Handler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    LogFlow(("TRPMGC07: %04x:%08x\n", pRegFrame->cs, pRegFrame->eip));
    PVM     pVM   = TRPMCPU_2_VM(pTrpmCpu);
    PVMCPU  pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);
    PGMRZDynMapStartAutoSet(pVCpu);

    int rc = CPUMHandleLazyFPU(pVCpu);
    rc = trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
    Log6(("TRPMGC07: %Rrc (%04x:%08x)\n", rc, pRegFrame->cs, pRegFrame->eip));
    return rc;
}


/**
 * \#NP ((segment) Not Present) handler.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCTrap0bHandler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    LogFlow(("TRPMGC0b: %04x:%08x\n", pRegFrame->cs, pRegFrame->eip));
    PVM     pVM   = TRPMCPU_2_VM(pTrpmCpu);
    PVMCPU  pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);
    PGMRZDynMapStartAutoSet(pVCpu);

    /*
     * Try to detect instruction by opcode which caused trap.
     * XXX note: this code may cause \#PF (trap e) or \#GP (trap d) while
     * accessing user code. need to handle it somehow in future!
     */
    RTGCPTR GCPtr;
    if (   SELMValidateAndConvertCSAddr(pVM, pRegFrame->eflags, pRegFrame->ss, pRegFrame->cs, &pRegFrame->csHid,
                                        (RTGCPTR)pRegFrame->eip, &GCPtr)
        == VINF_SUCCESS)
    {
        uint8_t *pu8Code = (uint8_t *)(uintptr_t)GCPtr;

        /*
         * First skip possible instruction prefixes, such as:
         *      OS, AS
         *      CS:, DS:, ES:, SS:, FS:, GS:
         *      REPE, REPNE
         *
         * note: Currently we supports only up to 4 prefixes per opcode, more
         *       prefixes (normally not used anyway) will cause trap d in guest.
         * note: Instruction length in IA-32 may be up to 15 bytes, we dont
         *       check this issue, its too hard.
         */
        for (unsigned i = 0; i < 4; i++)
        {
            if (    pu8Code[0] != 0xf2     /* REPNE/REPNZ */
                &&  pu8Code[0] != 0xf3     /* REP/REPE/REPZ */
                &&  pu8Code[0] != 0x2e     /* CS: */
                &&  pu8Code[0] != 0x36     /* SS: */
                &&  pu8Code[0] != 0x3e     /* DS: */
                &&  pu8Code[0] != 0x26     /* ES: */
                &&  pu8Code[0] != 0x64     /* FS: */
                &&  pu8Code[0] != 0x65     /* GS: */
                &&  pu8Code[0] != 0x66     /* OS */
                &&  pu8Code[0] != 0x67     /* AS */
               )
                break;
            pu8Code++;
        }

        /*
         * Detect right switch using a callgate.
         *
         * We recognize the following causes for the trap 0b:
         *      CALL FAR, CALL FAR []
         *      JMP FAR, JMP FAR []
         *      IRET (may cause a task switch)
         *
         * Note: we can't detect whether the trap was caused by a call to a
         *       callgate descriptor or it is a real trap 0b due to a bad selector.
         *       In both situations we'll pass execution to our recompiler so we don't
         *       have to worry.
         *       If we wanted to do better detection, we have set GDT entries to callgate
         *       descriptors pointing to our own handlers.
         */
        /** @todo not sure about IRET, may generate Trap 0d (\#GP), NEED TO CHECK! */
        if (    pu8Code[0] == 0x9a       /* CALL FAR */
            ||  (    pu8Code[0] == 0xff  /* CALL FAR [] */
                 && (pu8Code[1] & X86_OPCODE_MODRM_REG_MASK) == 0x18)
            ||  pu8Code[0] == 0xea       /* JMP FAR */
            ||  (    pu8Code[0] == 0xff  /* JMP FAR [] */
                 && (pu8Code[1] & X86_OPCODE_MODRM_REG_MASK) == 0x28)
            ||  pu8Code[0] == 0xcf       /* IRET */
           )
        {
            /*
             * Got potential call to callgate.
             * We simply return execution to the recompiler to do emulation
             * starting from the instruction which caused the trap.
             */
            pTrpmCpu->uActiveVector = ~0;
            Log6(("TRPMGC0b: %Rrc (%04x:%08x) (CG)\n", VINF_EM_RAW_RING_SWITCH, pRegFrame->cs, pRegFrame->eip));
            PGMRZDynMapReleaseAutoSet(pVCpu);
            return VINF_EM_RAW_RING_SWITCH;
        }
    }

    /*
     * Pass trap 0b as is to the recompiler in all other cases.
     */
    Log6(("TRPMGC0b: %Rrc (%04x:%08x)\n", VINF_EM_RAW_GUEST_TRAP, pRegFrame->cs, pRegFrame->eip));
    PGMRZDynMapReleaseAutoSet(pVCpu);
    return VINF_EM_RAW_GUEST_TRAP;
}


/**
 * \#GP (General Protection Fault) handler for Ring-0 privileged instructions.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pVM         The VM handle.
 * @param   pVCpu       The virtual CPU handle.
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @param   pCpu        The opcode info.
 * @param   PC          The program counter corresponding to cs:eip in pRegFrame.
 */
static int trpmGCTrap0dHandlerRing0(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, PDISCPUSTATE pCpu, RTGCPTR PC)
{
    int     rc;

    /*
     * Try handle it here, if not return to HC and emulate/interpret it there.
     */
    if ( pCpu->pCurInstr->opcode==OP_SYSRET ||OP_SYSRET==OP_SYSEXIT )
        Log(("OP_SYSEXIT|OP_SYSRET, opcode=%d\n", pCpu->pCurInstr->opcode));
        
    switch (pCpu->pCurInstr->opcode)
    {
        case OP_INT3:
            /*
             * Little hack to make the code below not fail
             */
            pCpu->param1.flags  = USE_IMMEDIATE8;
            pCpu->param1.parval = 3;
            /* fallthru */
        case OP_INT:
        {
            Assert(pCpu->param1.flags & USE_IMMEDIATE8);
            Assert(!(PATMIsPatchGCAddr(pVM, PC)));
            if (pCpu->param1.parval == 3)
            {
                /* Int 3 replacement patch? */
                if (PATMHandleInt3PatchTrap(pVM, pRegFrame) == VINF_SUCCESS)
                {
                    AssertFailed();
                    return trpmGCExitTrap(pVM, pVCpu, VINF_SUCCESS, pRegFrame);
                }
            }
            rc = TRPMForwardTrap(pVCpu, pRegFrame, (uint32_t)pCpu->param1.parval, pCpu->opsize, TRPM_TRAP_NO_ERRORCODE, TRPM_SOFTWARE_INT, 0xd);
            if (RT_SUCCESS(rc) && rc != VINF_EM_RAW_GUEST_TRAP)
                return trpmGCExitTrap(pVM, pVCpu, VINF_SUCCESS, pRegFrame);

            pVCpu->trpm.s.uActiveVector = (pVCpu->trpm.s.uActiveErrorCode & X86_TRAP_ERR_SEL_MASK) >> X86_TRAP_ERR_SEL_SHIFT;
            pVCpu->trpm.s.enmActiveType = TRPM_SOFTWARE_INT;
            return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_RING_SWITCH_INT, pRegFrame);
        }

#ifdef PATM_EMULATE_SYSENTER
        case OP_SYSEXIT:
        case OP_SYSRET:
            rc = PATMSysCall(pVM, pRegFrame, pCpu);
            return trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
#endif

        case OP_HLT:
            /* If it's in patch code, defer to ring-3. */
            if (PATMIsPatchGCAddr(pVM, PC))
                break;

            pRegFrame->eip += pCpu->opsize;
            return trpmGCExitTrap(pVM, pVCpu, VINF_EM_HALT, pRegFrame);


        /*
         * These instructions are used by PATM and CASM for finding
         * dangerous non-trapping instructions. Thus, since all
         * scanning and patching is done in ring-3 we'll have to
         * return to ring-3 on the first encounter of these instructions.
         */
        case OP_MOV_CR:
        case OP_MOV_DR:
            /* We can safely emulate control/debug register move instructions in patched code. */
            if (    !PATMIsPatchGCAddr(pVM, PC)
                &&  !CSAMIsKnownDangerousInstr(pVM, PC))
                break;
        case OP_INVLPG:
        case OP_LLDT:
        case OP_STI:
        case OP_RDTSC:  /* just in case */
        case OP_RDPMC:
        case OP_CLTS:
        case OP_WBINVD: /* nop */
        case OP_RDMSR:
        case OP_WRMSR:
        {
            uint32_t cbIgnored;
            rc = EMInterpretInstructionCPU(pVM, pVCpu, pCpu, pRegFrame, PC, EMCODETYPE_SUPERVISOR, &cbIgnored);
            if (RT_SUCCESS(rc))
                pRegFrame->eip += pCpu->opsize;
            else if (rc == VERR_EM_INTERPRETER)
                rc = VINF_EM_RAW_EXCEPTION_PRIVILEGED;
            return trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
        }
    }

    return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_EXCEPTION_PRIVILEGED, pRegFrame);
}


/**
 * \#GP (General Protection Fault) handler for Ring-3.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pVM         The VM handle.
 * @param   pVCpu       The virtual CPU handle.
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @param   pCpu        The opcode info.
 * @param   PC          The program counter corresponding to cs:eip in pRegFrame.
 */
static int trpmGCTrap0dHandlerRing3(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, PDISCPUSTATE pCpu, RTGCPTR PC)
{
    int     rc;
    Assert(!pRegFrame->eflags.Bits.u1VM);
    
    if ( pCpu->pCurInstr->opcode==OP_SYSCALL ||OP_SYSRET==OP_SYSENTER )
    Log(("OP_SYSCALL|OP_SYSENTER, opcode=%d\n", pCpu->pCurInstr->opcode));
    
    switch (pCpu->pCurInstr->opcode)
    {
        /*
         * INT3 and INT xx are ring-switching.
         * (The shadow IDT will have set the entries to DPL=0, that's why we're here.)
         */
        case OP_INT3:
            /*
             * Little hack to make the code below not fail
             */
            pCpu->param1.flags  = USE_IMMEDIATE8;
            pCpu->param1.parval = 3;
            /* fall thru */
        case OP_INT:
        {
            Assert(pCpu->param1.flags & USE_IMMEDIATE8);
            rc = TRPMForwardTrap(pVCpu, pRegFrame, (uint32_t)pCpu->param1.parval, pCpu->opsize, TRPM_TRAP_NO_ERRORCODE, TRPM_SOFTWARE_INT, 0xd);
            if (RT_SUCCESS(rc) && rc != VINF_EM_RAW_GUEST_TRAP)
                return trpmGCExitTrap(pVM, pVCpu, VINF_SUCCESS, pRegFrame);

            pVCpu->trpm.s.uActiveVector = (pVCpu->trpm.s.uActiveErrorCode & X86_TRAP_ERR_SEL_MASK) >> X86_TRAP_ERR_SEL_SHIFT;
            pVCpu->trpm.s.enmActiveType = TRPM_SOFTWARE_INT;
            return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_RING_SWITCH_INT, pRegFrame);
        }

        /*
         * SYSCALL, SYSENTER, INTO and BOUND are also ring-switchers.
         */
        case OP_SYSCALL:
        case OP_SYSENTER:
#ifdef PATM_EMULATE_SYSENTER
            rc = PATMSysCall(pVM, pRegFrame, pCpu);
            if (rc == VINF_SUCCESS)
                return trpmGCExitTrap(pVM, pVCpu, VINF_SUCCESS, pRegFrame);
            /* else no break; */
#endif
        case OP_BOUND:
        case OP_INTO:
            pVCpu->trpm.s.uActiveVector = ~0;
            return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_RING_SWITCH, pRegFrame);

        /*
         * Handle virtualized TSC & PMC reads, just in case.
         */
        case OP_RDTSC:
        case OP_RDPMC:
        {
            uint32_t cbIgnored;
            rc = EMInterpretInstructionCPU(pVM, pVCpu, pCpu, pRegFrame, PC, EMCODETYPE_SUPERVISOR, &cbIgnored);
            if (RT_SUCCESS(rc))
                pRegFrame->eip += pCpu->opsize;
            else if (rc == VERR_EM_INTERPRETER)
                rc = VINF_EM_RAW_EXCEPTION_PRIVILEGED;
            return trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
        }

        /*
         * STI and CLI are I/O privileged, i.e. if IOPL
         */
        case OP_STI:
        case OP_CLI:
        {
            uint32_t efl = CPUMRawGetEFlags(pVCpu, pRegFrame);
            if (X86_EFL_GET_IOPL(efl) >= (unsigned)(pRegFrame->ss & X86_SEL_RPL))
            {
                LogFlow(("trpmGCTrap0dHandlerRing3: CLI/STI -> REM\n"));
                return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RESCHEDULE_REM, pRegFrame);
            }
            LogFlow(("trpmGCTrap0dHandlerRing3: CLI/STI -> #GP(0)\n"));
            break;
        }
    }

    /*
     * A genuine guest fault.
     */
    return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_GUEST_TRAP, pRegFrame);
}


/**
 * Emulates RDTSC for the \#GP handler.
 *
 * @returns VINF_SUCCESS or VINF_EM_RAW_EMULATE_INSTR.
 *
 * @param   pVM         Pointer to the shared VM structure.
 * @param   pVCpu       The virtual CPU handle.
 * @param   pRegFrame   Pointer to the register frame for the trap.
 *                      This will be updated on successful return.
 */
DECLINLINE(int) trpmGCTrap0dHandlerRdTsc(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame)
{
    STAM_COUNTER_INC(&pVM->trpm.s.StatTrap0dRdTsc);

    if (CPUMGetGuestCR4(pVCpu) & X86_CR4_TSD)
        return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_EMULATE_INSTR, pRegFrame); /* will trap (optimize later). */

    uint64_t uTicks = TMCpuTickGet(pVCpu);
    pRegFrame->eax = uTicks;
    pRegFrame->edx = uTicks >> 32;
    pRegFrame->eip += 2;
    return trpmGCExitTrap(pVM, pVCpu, VINF_SUCCESS, pRegFrame);
}


/**
 * \#GP (General Protection Fault) handler.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pVM         The VM handle.
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 */
static int trpmGCTrap0dHandler(PVM pVM, PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    LogFlow(("trpmGCTrap0dHandler: cs:eip=%RTsel:%08RX32 uErr=%RGv\n", pRegFrame->ss, pRegFrame->eip, pTrpmCpu->uActiveErrorCode));
    PVMCPU  pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);

    /*
     * Convert and validate CS.
     */
    STAM_PROFILE_START(&pVM->trpm.s.StatTrap0dDisasm, a);
    RTGCPTR PC;
    uint32_t cBits;
    int rc = SELMValidateAndConvertCSAddrGCTrap(pVM, pRegFrame->eflags, pRegFrame->ss, pRegFrame->cs,
                                                (RTGCPTR)pRegFrame->eip, &PC, &cBits);
    if (RT_FAILURE(rc))
    {
        Log(("trpmGCTrap0dHandler: Failed to convert %RTsel:%RX32 (cpl=%d) - rc=%Rrc !!\n",
             pRegFrame->cs, pRegFrame->eip, pRegFrame->ss & X86_SEL_RPL, rc));
        STAM_PROFILE_STOP(&pVM->trpm.s.StatTrap0dDisasm, a);
        return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_EMULATE_INSTR, pRegFrame);
    }

    /*
     * Disassemble the instruction.
     */
    DISCPUSTATE Cpu;
    uint32_t    cbOp;
    rc = EMInterpretDisasOneEx(pVM, pVCpu, (RTGCUINTPTR)PC, pRegFrame, &Cpu, &cbOp);
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("DISCoreOneEx failed to PC=%RGv rc=%Rrc\n", PC, rc));
        STAM_PROFILE_STOP(&pVM->trpm.s.StatTrap0dDisasm, a);
        return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_EMULATE_INSTR, pRegFrame);
    }
    STAM_PROFILE_STOP(&pVM->trpm.s.StatTrap0dDisasm, a);

    /*
     * Optimize RDTSC traps.
     * Some guests (like Solaris) are using RDTSC all over the place and
     * will end up trapping a *lot* because of that.
     *
     * Note: it's no longer safe to access the instruction opcode directly due to possible stale code TLB entries
     */
    if (Cpu.pCurInstr->opcode == OP_RDTSC)
        return trpmGCTrap0dHandlerRdTsc(pVM, pVCpu, pRegFrame);

    /*
     * Deal with I/O port access.
     */
    if (    pVCpu->trpm.s.uActiveErrorCode == 0
        &&  (Cpu.pCurInstr->optype & OPTYPE_PORTIO))
    {
        VBOXSTRICTRC rcStrict = EMInterpretPortIO(pVM, pVCpu, pRegFrame, &Cpu, cbOp);
        rc = VBOXSTRICTRC_TODO(rcStrict);
        return trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
    }

    /*
     * Deal with Ring-0 (privileged instructions)
     */
    if (    (pRegFrame->ss & X86_SEL_RPL) <= 1
        &&  !pRegFrame->eflags.Bits.u1VM)
        return trpmGCTrap0dHandlerRing0(pVM, pVCpu, pRegFrame, &Cpu, PC);

    /*
     * Deal with Ring-3 GPs.
     */
    if (!pRegFrame->eflags.Bits.u1VM)
        return trpmGCTrap0dHandlerRing3(pVM, pVCpu, pRegFrame, &Cpu, PC);

    /*
     * Deal with v86 code.
     *
     * We always set IOPL to zero which makes e.g. pushf fault in V86
     * mode. The guest might use IOPL=3 and therefore not expect a #GP.
     * Simply fall back to the recompiler to emulate this instruction if
     * that's the case. To get the correct we must use CPUMRawGetEFlags.
     */
    X86EFLAGS eflags;
    eflags.u32 = CPUMRawGetEFlags(pVCpu, pRegFrame); /* Get the correct value. */
    Log3(("TRPM #GP V86: cs:eip=%04x:%08x IOPL=%d efl=%08x\n", pRegFrame->cs, pRegFrame->eip, eflags.Bits.u2IOPL, eflags.u));
    if (eflags.Bits.u2IOPL != 3)
    {
        Assert(eflags.Bits.u2IOPL == 0);

        rc = TRPMForwardTrap(pVCpu, pRegFrame, 0xD, 0, TRPM_TRAP_HAS_ERRORCODE, TRPM_TRAP, 0xd);
        Assert(rc == VINF_EM_RAW_GUEST_TRAP);
        return trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
    }
    return trpmGCExitTrap(pVM, pVCpu, VINF_EM_RAW_EMULATE_INSTR, pRegFrame);
}


/**
 * \#GP (General Protection Fault) handler.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCTrap0dHandler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    PVM     pVM   = TRPMCPU_2_VM(pTrpmCpu);
    PVMCPU  pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);

    LogFlow(("TRPMGC0d: %04x:%08x err=%x\n", pRegFrame->cs, pRegFrame->eip, (uint32_t)pVCpu->trpm.s.uActiveErrorCode));

    PGMRZDynMapStartAutoSet(pVCpu);
    int rc = trpmGCTrap0dHandler(pVM, pTrpmCpu, pRegFrame);
    switch (rc)
    {
        case VINF_EM_RAW_GUEST_TRAP:
        case VINF_EM_RAW_EXCEPTION_PRIVILEGED:
            if (PATMIsPatchGCAddr(pVM, pRegFrame->eip))
                rc = VINF_PATM_PATCH_TRAP_GP;
            break;

        case VINF_EM_RAW_INTERRUPT_PENDING:
            Assert(TRPMHasTrap(pVCpu));
            /* no break; */
        case VINF_PGM_SYNC_CR3: /** @todo Check this with Sander. */
        case VINF_EM_RAW_EMULATE_INSTR:
        case VINF_IOM_HC_IOPORT_READ:
        case VINF_IOM_HC_IOPORT_WRITE:
        case VINF_IOM_HC_MMIO_WRITE:
        case VINF_IOM_HC_MMIO_READ:
        case VINF_IOM_HC_MMIO_READ_WRITE:
        case VINF_PATM_PATCH_INT3:
        case VINF_EM_NO_MEMORY:
        case VINF_EM_RAW_TO_R3:
        case VINF_EM_RAW_TIMER_PENDING:
        case VINF_EM_PENDING_REQUEST:
        case VINF_EM_HALT:
        case VINF_SUCCESS:
            break;

        default:
            AssertMsg(PATMIsPatchGCAddr(pVM, pRegFrame->eip) == false, ("return code %d\n", rc));
            break;
        }
    Log6(("TRPMGC0d: %Rrc (%04x:%08x)\n", rc, pRegFrame->cs, pRegFrame->eip));
    return rc;
}


/**
 * \#PF (Page Fault) handler.
 *
 * Calls PGM which does the actual handling.
 *
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed execution to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCTrap0eHandler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    PVM     pVM   = TRPMCPU_2_VM(pTrpmCpu);
    PVMCPU  pVCpu = TRPMCPU_2_VMCPU(pTrpmCpu);

    LogFlow(("TRPMGC0e: %04x:%08x err=%x cr2=%08x\n", pRegFrame->cs, pRegFrame->eip, (uint32_t)pVCpu->trpm.s.uActiveErrorCode, (uint32_t)pVCpu->trpm.s.uActiveCR2));

    /*
     * This is all PGM stuff.
     */
    PGMRZDynMapStartAutoSet(pVCpu);
    int rc = PGMTrap0eHandler(pVCpu, pVCpu->trpm.s.uActiveErrorCode, pRegFrame, (RTGCPTR)pVCpu->trpm.s.uActiveCR2);
    switch (rc)
    {
        case VINF_EM_RAW_EMULATE_INSTR:
        case VINF_EM_RAW_EMULATE_INSTR_PD_FAULT:
        case VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT:
        case VINF_EM_RAW_EMULATE_INSTR_TSS_FAULT:
        case VINF_EM_RAW_EMULATE_INSTR_LDT_FAULT:
        case VINF_EM_RAW_EMULATE_INSTR_IDT_FAULT:
            if (PATMIsPatchGCAddr(pVM, pRegFrame->eip))
                rc = VINF_PATCH_EMULATE_INSTR;
            break;

        case VINF_EM_RAW_GUEST_TRAP:
            if (PATMIsPatchGCAddr(pVM, pRegFrame->eip))
            {
                PGMRZDynMapReleaseAutoSet(pVCpu);
                return VINF_PATM_PATCH_TRAP_PF;
            }

            rc = TRPMForwardTrap(pVCpu, pRegFrame, 0xE, 0, TRPM_TRAP_HAS_ERRORCODE, TRPM_TRAP, 0xe);
            Assert(rc == VINF_EM_RAW_GUEST_TRAP);
            break;

        case VINF_EM_RAW_INTERRUPT_PENDING:
            Assert(TRPMHasTrap(pVCpu));
            /* no break; */
        case VINF_IOM_HC_MMIO_READ:
        case VINF_IOM_HC_MMIO_WRITE:
        case VINF_IOM_HC_MMIO_READ_WRITE:
        case VINF_PATM_HC_MMIO_PATCH_READ:
        case VINF_PATM_HC_MMIO_PATCH_WRITE:
        case VINF_SUCCESS:
        case VINF_EM_RAW_TO_R3:
        case VINF_EM_PENDING_REQUEST:
        case VINF_EM_RAW_TIMER_PENDING:
        case VINF_EM_NO_MEMORY:
        case VINF_CSAM_PENDING_ACTION:
        case VINF_PGM_SYNC_CR3: /** @todo Check this with Sander. */
            break;

        default:
            AssertMsg(PATMIsPatchGCAddr(pVM, pRegFrame->eip) == false, ("Patch address for return code %d. eip=%08x\n", rc, pRegFrame->eip));
            break;
    }
    rc = trpmGCExitTrap(pVM, pVCpu, rc, pRegFrame);
    Log6(("TRPMGC0e: %Rrc (%04x:%08x)\n", rc, pRegFrame->cs, pRegFrame->eip));
    return rc;
}


/**
 * Scans for the EIP in the specified array of trap handlers.
 *
 * If we don't fine the EIP, we'll panic.
 *
 * @returns VBox status code.
 *
 * @param   pVM         The VM handle.
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @param   paHandlers  The array of trap handler records.
 * @param   pEndRecord  The end record (exclusive).
 */
static int trpmGCHyperGeneric(PVM pVM, PCPUMCTXCORE pRegFrame, PCTRPMGCHYPER paHandlers, PCTRPMGCHYPER pEndRecord)
{
    uintptr_t uEip  = (uintptr_t)pRegFrame->eip;
    Assert(paHandlers <= pEndRecord);

    Log(("trpmGCHyperGeneric: uEip=%x %p-%p\n", uEip, paHandlers, pEndRecord));

#if 0 /// @todo later
    /*
     * Start by doing a kind of binary search.
     */
    unsigned iStart = 0;
    unsigned iEnd   = pEndRecord - paHandlers;
    unsigned i      = iEnd / 2;
#endif

    /*
     * Do a linear search now (in case the array wasn't properly sorted).
     */
    for (PCTRPMGCHYPER pCur = paHandlers; pCur < pEndRecord; pCur++)
    {
        if (    pCur->uStartEIP <= uEip
            &&  (pCur->uEndEIP ? pCur->uEndEIP > uEip : pCur->uStartEIP == uEip))
            return pCur->pfnHandler(pVM, pRegFrame, pCur->uUser);
    }

    return VERR_TRPM_DONT_PANIC;
}


/**
 * Hypervisor \#NP ((segment) Not Present) handler.
 *
 * Scans for the EIP in the registered trap handlers.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed back to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCHyperTrap0bHandler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    return trpmGCHyperGeneric(TRPMCPU_2_VM(pTrpmCpu), pRegFrame, g_aTrap0bHandlers, g_aTrap0bHandlersEnd);
}


/**
 * Hypervisor \#GP (General Protection Fault) handler.
 *
 * Scans for the EIP in the registered trap handlers.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed back to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCHyperTrap0dHandler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    return trpmGCHyperGeneric(TRPMCPU_2_VM(pTrpmCpu), pRegFrame, g_aTrap0dHandlers, g_aTrap0dHandlersEnd);
}


/**
 * Hypervisor \#PF (Page Fault) handler.
 *
 * Scans for the EIP in the registered trap handlers.
 *
 * @returns VBox status code.
 *          VINF_SUCCESS means we completely handled this trap,
 *          other codes are passed back to host context.
 *
 * @param   pTrpmCpu    Pointer to TRPMCPU data (within VM).
 * @param   pRegFrame   Pointer to the register frame for the trap.
 * @internal
 */
DECLASM(int) TRPMGCHyperTrap0eHandler(PTRPMCPU pTrpmCpu, PCPUMCTXCORE pRegFrame)
{
    return trpmGCHyperGeneric(TRPMCPU_2_VM(pTrpmCpu), pRegFrame, g_aTrap0dHandlers, g_aTrap0dHandlersEnd);
}


/**
 * Deal with hypervisor traps occurring when resuming execution on a trap.
 *
 * @returns VBox status code.
 * @param   pVM         The VM handle.
 * @param   pRegFrame   Register frame.
 * @param   uUser       User arg.
 */
DECLCALLBACK(int) trpmGCTrapInGeneric(PVM pVM, PCPUMCTXCORE pRegFrame, uintptr_t uUser)
{
    Log(("********************************************************\n"));
    Log(("trpmGCTrapInGeneric: eip=%RX32 uUser=%#x\n", pRegFrame->eip, uUser));
    Log(("********************************************************\n"));

    if (uUser & TRPM_TRAP_IN_HYPER)
    {
        /*
         * Check that there is still some stack left, if not we'll flag
         * a guru meditation (the alternative is a triple fault).
         */
        RTRCUINTPTR cbStackUsed = (RTRCUINTPTR)VMMGetStackRC(VMMGetCpu(pVM)) - pRegFrame->esp;
        if (cbStackUsed > VMM_STACK_SIZE - _1K)
        {
            LogRel(("trpmGCTrapInGeneric: ran out of stack: esp=#x cbStackUsed=%#x\n", pRegFrame->esp, cbStackUsed));
            return VERR_TRPM_DONT_PANIC;
        }

        /*
         * Just zero the register containing the selector in question.
         * We'll deal with the actual stale or troublesome selector value in
         * the outermost trap frame.
         */
        switch (uUser & TRPM_TRAP_IN_OP_MASK)
        {
            case TRPM_TRAP_IN_MOV_GS:
                pRegFrame->eax = 0;
                pRegFrame->gs = 0; /* prevent recursive trouble. */
                break;
            case TRPM_TRAP_IN_MOV_FS:
                pRegFrame->eax = 0;
                pRegFrame->fs = 0; /* prevent recursive trouble. */
                return VINF_SUCCESS;

            default:
                AssertMsgFailed(("Invalid uUser=%#x\n", uUser));
                return VERR_INTERNAL_ERROR;
        }
    }
    else
    {
        /*
         * Reconstruct the guest context and switch to the recompiler.
         * We ASSUME we're only at
         */
        CPUMCTXCORE  CtxCore = *pRegFrame;
        uint32_t    *pEsp = (uint32_t *)pRegFrame->esp;
        int          rc;

        switch (uUser)
        {
            /*
             * This will only occur when resuming guest code in a trap handler!
             */
            /* @note ASSUMES esp points to the temporary guest CPUMCTXCORE!!! */
            case TRPM_TRAP_IN_MOV_GS:
            case TRPM_TRAP_IN_MOV_FS:
            case TRPM_TRAP_IN_MOV_ES:
            case TRPM_TRAP_IN_MOV_DS:
            {
                PCPUMCTXCORE pTempGuestCtx = (PCPUMCTXCORE)pEsp;

                /* Just copy the whole thing; several selector registers, eip (etc) and eax are not yet in pRegFrame. */
                CtxCore = *pTempGuestCtx;
                rc = VINF_EM_RAW_STALE_SELECTOR;
                break;
            }

            /*
             * This will only occur when resuming guest code!
             */
            case TRPM_TRAP_IN_IRET:
                CtxCore.eip = *pEsp++;
                CtxCore.cs = (RTSEL)*pEsp++;
                CtxCore.eflags.u32 = *pEsp++;
                CtxCore.esp = *pEsp++;
                CtxCore.ss = (RTSEL)*pEsp++;
                rc = VINF_EM_RAW_IRET_TRAP;
                break;

            /*
             * This will only occur when resuming V86 guest code!
             */
            case TRPM_TRAP_IN_IRET | TRPM_TRAP_IN_V86:
                CtxCore.eip = *pEsp++;
                CtxCore.cs = (RTSEL)*pEsp++;
                CtxCore.eflags.u32 = *pEsp++;
                CtxCore.esp = *pEsp++;
                CtxCore.ss = (RTSEL)*pEsp++;
                CtxCore.es = (RTSEL)*pEsp++;
                CtxCore.ds = (RTSEL)*pEsp++;
                CtxCore.fs = (RTSEL)*pEsp++;
                CtxCore.gs = (RTSEL)*pEsp++;
                rc = VINF_EM_RAW_IRET_TRAP;
                break;

            default:
                AssertMsgFailed(("Invalid uUser=%#x\n", uUser));
                return VERR_INTERNAL_ERROR;
        }


        CPUMSetGuestCtxCore(VMMGetCpu0(pVM), &CtxCore);
        TRPMGCHyperReturnToHost(pVM, rc);
    }

    AssertMsgFailed(("Impossible!\n"));
    return VERR_INTERNAL_ERROR;
}

