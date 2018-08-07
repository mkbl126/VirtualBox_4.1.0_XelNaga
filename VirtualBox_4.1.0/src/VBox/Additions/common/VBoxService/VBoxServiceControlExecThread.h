/* $Id: VBoxServiceControlExecThread.h 36548 2011-04-05 09:27:33Z vboxsync $ */
/** @file
 * VBoxServiceControlExecThread - Thread for an executed guest process.
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

#ifndef ___VBoxServiceControlExecThread_h
#define ___VBoxServiceControlExecThread_h

#include "VBoxServiceInternal.h"

int VBoxServiceControlExecThreadAlloc(PVBOXSERVICECTRLTHREAD pThread,
                                      uint32_t u32ContextID,
                                      const char *pszCmd, uint32_t uFlags,
                                      const char *pszArgs, uint32_t uNumArgs,
                                      const char *pszEnv, uint32_t cbEnv, uint32_t uNumEnvVars,
                                      const char *pszUser, const char *pszPassword, uint32_t uTimeLimitMS);
void VBoxServiceControlExecThreadDestroy(PVBOXSERVICECTRLTHREADDATAEXEC pData);
int VBoxServiceControlExecThreadSetInput(uint32_t uPID, bool fPendingClose, uint8_t *pBuf,
                                         uint32_t cbSize, uint32_t *pcbWritten);
int VBoxServiceControlExecThreadGetOutput(uint32_t uPID, uint32_t uHandleId, uint32_t uTimeout,
                                          uint8_t *pBuf, uint32_t cbSize, uint32_t *pcbRead);
void VBoxServiceControlExecThreadsShutdown(void);

#endif  /* !___VBoxServiceControlExecThread_h */

