/* $Id: DevFwCommon.h 28800 2010-04-27 08:22:32Z vboxsync $ */
/** @file
 * FwCommon - Shared firmware code, header.
 */

/*
 * Copyright (C) 2009 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___PC_FwCommon_h
#define ___PC_FwCommon_h

#include "DevPcBios.h"

/** @def VBOX_MPS_TABLE_BASE
 *
 * Must be located in the same page as the DMI table.
 */
#define VBOX_MPS_TABLE_BASE          (VBOX_DMI_TABLE_BASE+VBOX_DMI_TABLE_SIZE)

/* Plant DMI table */
int FwCommonPlantDMITable(PPDMDEVINS pDevIns, uint8_t *pTable, unsigned cbMax, PCRTUUID pUuid, PCFGMNODE pCfg);
void FwCommonPlantSmbiosAndDmiHdrs(PPDMDEVINS pDevIns);

/* Plant MPS table */
void FwCommonPlantMpsTable(PPDMDEVINS pDevIns, uint8_t *pTable, unsigned cbMax, uint16_t cCpus);
void FwCommonPlantMpsFloatPtr(PPDMDEVINS pDevIns);

#endif
