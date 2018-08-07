
/* max length for a single syscall function name.*/
#define     MAX_NAME_LENGTH         64

/* System call names in the SSDT of Windows 2000.
*/
#define     COUNT_SYSCALL_WIN2K     248
static const char syscall_windows_2k[COUNT_SYSCALL_WIN2K][MAX_NAME_LENGTH] = 
{ 
    "NtAcceptConnectPort",
    "NtAccessCheck",
    "NtAccessCheckAndAuditAlarm",
    "NtAccessCheckByType",
    "NtAccessCheckByTypeAndAuditAlarm",
    "NtAccessCheckByTypeResultList",
    "NtAccessCheckByTypeResultListAndAuditAlarm",
    "NtAccessCheckByTypeResultListAndAuditAlarmByHandle",
    "NtAddAtom",
    "NtAdjustGroupsToken",
    "NtAdjustPrivilegesToken",
    "NtAlertResumeThread",
    "NtAlertThread",
    "NtAllocateLocallyUniqueId",
    "NtAllocateUserPhysicalPages",
    "NtAllocateUuids",
    "NtAllocateVirtualMemory",
    "NtAreMappedFilesTheSame",
    "NtAssignProcessToJobObject",
    "NtCallbackReturn",
    "NtCancelIoFile",
    "NtCancelTimer",
    "NtCancelDeviceWakeupRequest",
    "NtClearEvent",
    "NtClose",
    "NtCloseObjectAuditAlarm",
    "NtCompleteConnectPort",
    "NtConnectPort",
    "NtContinue",
    "NtCreateDirectoryObject",
    "NtCreateEvent",
    "NtCreateEventPair",
    "NtCreateFile",
    "NtCreateIoCompletion",
    "NtCreateJobObject",
    "NtCreateKey",
    "NtCreateMailslotFile",
    "NtCreateMutant",
    "NtCreateNamedPipeFile",
    "NtCreatePagingFile",
    "NtCreatePort",
    "NtCreateProcess",
    "NtCreateProfile",
    "NtCreateSection",
    "NtCreateSemaphore",
    "NtCreateSymbolicLinkObject",
    "NtCreateThread",
    "NtCreateTimer",
    "NtCreateToken",
    "NtCreateWaitablePort",
    "NtDelayExecution",
    "NtDeleteAtom",
    "NtDeleteFile",
    "NtDeleteKey",
    "NtDeleteObjectAuditAlarm",
    "NtDeleteValueKey",
    "NtDeviceIoControlFile",
    "NtDisplayString",
    "NtDuplicateObject",
    "NtDuplicateToken",
    "NtEnumerateKey",
    "NtEnumerateValueKey",
    "NtExtendSection",
    "NtFilterToken",
    "NtFindAtom",
    "NtFlushBuffersFile",
    "NtFlushInstructionCache",
    "NtFlushKey",
    "NtFlushVirtualMemory",
    "NtFlushWriteBuffer",
    "NtFreeUserPhysicalPages",
    "NtFreeVirtualMemory",
    "NtFsControlFile",
    "NtGetContextThread",
    "NtGetDevicePowerState",
    "NtGetPlugPlayEvent",
    "NtGetTickCount",
    "NtGetWriteWatch",
    "NtImpersonateAnonymousToken",
    "NtImpersonateClientOfPort",
    "NtImpersonateThread",
    "NtInitializeRegistry",
    "NtInitiatePowerAction",
    "NtIsSystemResumeAutomatic",
    "NtListenPort",
    "NtLoadDriver",
    "NtLoadKey",
    "NtLoadKey2",
    "NtLockFile",
    "NtLockVirtualMemory",
    "NtMakeTemporaryObject",
    "NtMapUserPhysicalPages",
    "NtMapUserPhysicalPagesScatter",
    "NtMapViewOfSection",
    "NtNotifyChangeDirectoryFile",
    "NtNotifyChangeKey",
    "NtNotifyChangeMultipleKeys",
    "NtOpenDirectoryObject",
    "NtOpenEvent",
    "NtOpenEventPair",
    "NtOpenFile",
    "NtOpenIoCompletion",
    "NtOpenJobObject",
    "NtOpenKey",
    "NtOpenMutant",
    "NtOpenObjectAuditAlarm",
    "NtOpenProcess",
    "NtOpenProcessToken",
    "NtOpenSection",
    "NtOpenSemaphore",
    "NtOpenSymbolicLinkObject",
    "NtOpenThread",
    "NtOpenThreadToken",
    "NtOpenTimer",
    "NtPlugPlayControl",
    "NtPowerInformation",
    "NtPrivilegeCheck",
    "NtPrivilegedServiceAuditAlarm",
    "NtPrivilegeObjectAuditAlarm",
    "NtProtectVirtualMemory",
    "NtPulseEvent",
    "NtQueryInformationAtom",
    "NtQueryAttributesFile",
    "NtQueryDefaultLocale",
    "NtQueryDefaultUILanguage",
    "NtQueryDirectoryFile",
    "NtQueryDirectoryObject",
    "NtQueryEaFile",
    "NtQueryEvent",
    "NtQueryFullAttributesFile",
    "NtQueryInformationFile",
    "NtQueryInformationJobObject",
    "NtQueryIoCompletion",
    "NtQueryInformationPort",
    "NtQueryInformationProcess",
    "NtQueryInformationThread",
    "NtQueryInformationToken",
    "NtQueryInstallUILanguage",
    "NtQueryIntervalProfile",
    "NtQueryKey",
    "NtQueryMultipleValueKey",
    "NtQueryMutant",
    "NtQueryObject",
    "NtQueryOpenSubKeys",
    "NtQueryPerformanceCounter",
    "NtQueryQuotaInformationFile",
    "NtQuerySection",
    "NtQuerySecurityObject",
    "NtQuerySemaphore",
    "NtQuerySymbolicLinkObject",
    "NtQuerySystemEnvironmentValue",
    "NtQuerySystemInformation",
    "NtQuerySystemTime",
    "NtQueryTimer",
    "NtQueryTimerResolution",
    "NtQueryValueKey",
    "NtQueryVirtualMemory",
    "NtQueryVolumeInformationFile",
    "NtQueueApcThread",
    "NtRaiseException",
    "NtRaiseHardError",
    "NtReadFile",
    "NtReadFileScatter",
    "NtReadRequestData",
    "NtReadVirtualMemory",
    "NtRegisterThreadTerminatePort",
    "NtReleaseMutant",
    "NtReleaseSemaphore",
    "NtRemoveIoCompletion",
    "NtReplaceKey",
    "NtReplyPort",
    "NtReplyWaitReceivePort",
    "NtReplyWaitReceivePortEx",
    "NtReplyWaitReplyPort",
    "NtRequestDeviceWakeup",
    "NtRequestPort",
    "NtRequestWaitReplyPort",
    "NtRequestWakeupLatency",
    "NtResetEvent",
    "NtResetWriteWatch",
    "NtRestoreKey",
    "NtResumeThread",
    "NtSaveKey",
    "NtSaveMergedKeys",
    "NtSecureConnectPort",
    "NtSetIoCompletion",
    "NtSetContextThread",
    "NtSetDefaultHardErrorPort",
    "NtSetDefaultLocale",
    "NtSetDefaultUILanguage",
    "NtSetEaFile",
    "NtSetEvent",
    "NtSetHighEventPair",
    "NtSetHighWaitLowEventPair",
    "NtSetInformationFile",
    "NtSetInformationJobObject",
    "NtSetInformationKey",
    "NtSetInformationObject",
    "NtSetInformationProcess",
    "NtSetInformationThread",
    "NtSetInformationToken",
    "NtSetIntervalProfile",
    "NtSetLdtEntries",
    "NtSetLowEventPair",
    "NtSetLowWaitHighEventPair",
    "NtSetQuotaInformationFile",
    "NtSetSecurityObject",
    "NtSetSystemEnvironmentValue",
    "NtSetSystemInformation",
    "NtSetSystemPowerState",
    "NtSetSystemTime",
    "NtSetThreadExecutionState",
    "NtSetTimer",
    "NtSetTimerResolution",
    "NtSetUuidSeed",
    "NtSetValueKey",
    "NtSetVolumeInformationFile",
    "NtShutdownSystem",
    "NtSignalAndWaitForSingleObject",
    "NtStartProfile",
    "NtStopProfile",
    "NtSuspendThread",
    "NtSystemDebugControl",
    "NtTerminateJobObject",
    "NtTerminateProcess",
    "NtTerminateThread",
    "NtTestAlert",
    "NtUnloadDriver",
    "NtUnloadKey",
    "NtUnlockFile",
    "NtUnlockVirtualMemory",
    "NtUnmapViewOfSection",
    "NtVdmControl",
    "NtWaitForMultipleObjects",
    "NtWaitForSingleObject",
    "NtWaitHighEventPair",
    "NtWaitLowEventPair",
    "NtWriteFile",
    "NtWriteFileGather",
    "NtWriteRequestData",
    "NtWriteVirtualMemory",
    "NtCreateChannel",
    "NtListenChannel",
    "NtOpenChannel",
    "NtReplyWaitSendChannel",
    "NtSendWaitReplyChannel",
    "NtSetContextChannel",
    "NtYieldExecution"
};


/* The SSDT and ShadowSSDT function names on WindowsXP.
*/

/* System call names in the SSDT of Windows XP.
*/
#define     COUNT_SYSCALL_WINXP     284
static const char syscall_windows_xp[COUNT_SYSCALL_WINXP][MAX_NAME_LENGTH] = 
{ 
    "NtAcceptConnectPort", /* 0 */
    "NtAccessCheck", /* 1 */
    "NtAccessCheckAndAuditAlarm", /* 2 */
    "NtAccessCheckByType", /* 3 */
    "NtAccessCheckByTypeAndAuditAlarm", /* 4 */
    "NtAccessCheckByTypeResultList", /* 5 */
    "NtAccessCheckByTypeResultListAndAuditAlarm", /* 6 */
    "NtAccessCheckByTypeResultListAndAuditAlarmByHandle", /* 7 */
    "NtAddAtom", /* 8 */
    "NtAddBootEntry", /* 9 */
    "NtAdjustGroupsToken", /* 10 */
    "NtAdjustPrivilegesToken", /* 11 */
    "NtAlertResumeThread", /* 12 */
    "NtAlertThread", /* 13 */
    "NtAllocateLocallyUniqueId", /* 14 */
    "NtAllocateUserPhysicalPages", /* 15 */
    "NtAllocateUuids", /* 16 */
    "NtAllocateVirtualMemory", /* 17 */
    "NtAreMappedFilesTheSame", /* 18 */
    "NtAssignProcessToJobObject", /* 19 */
    "NtCallbackReturn", /* 20 */
    "NtCancelDeviceWakeupRequest", /* 21 */
    "NtCancelIoFile", /* 22 */
    "NtCancelTimer", /* 23 */
    "NtClearEvent", /* 24 */
    "NtClose", /* 25 */
    "NtCloseObjectAuditAlarm", /* 26 */
    "NtCompactKeys", /* 27 */
    "NtCompareTokens", /* 28 */
    "NtCompleteConnectPort", /* 29 */
    "NtCompressKey", /* 30 */
    "NtConnectPort", /* 31 */
    "NtContinue", /* 32 */
    "NtCreateDebugObject", /* 33 */
    "NtCreateDirectoryObject", /* 34 */
    "NtCreateEvent", /* 35 */
    "NtCreateEventPair", /* 36 */
    "NtCreateFile", /* 37 */
    "NtCreateIoCompletion", /* 38 */
    "NtCreateJobObject", /* 39 */
    "NtCreateJobSet", /* 40 */
    "NtCreateKey", /* 41 */
    "NtCreateMailslotFile", /* 42 */
    "NtCreateMutant", /* 43 */
    "NtCreateNamedPipeFile", /* 44 */
    "NtCreatePagingFile", /* 45 */
    "NtCreatePort", /* 46 */
    "NtCreateProcess", /* 47 */
    "NtCreateProcessEx", /* 48 */
    "NtCreateProfile", /* 49 */
    "NtCreateSection", /* 50 */
    "NtCreateSemaphore", /* 51 */
    "NtCreateSymbolicLinkObject", /* 52 */
    "NtCreateThread", /* 53 */
    "NtCreateTimer", /* 54 */
    "NtCreateToken", /* 55 */
    "NtCreateWaitablePort", /* 56 */
    "NtDebugActiveProcess", /* 57 */
    "NtDebugContinue", /* 58 */
    "NtDelayExecution", /* 59 */
    "NtDeleteAtom", /* 60 */
    "NtDeleteBootEntry", /* 61 */
    "NtDeleteFile", /* 62 */
    "NtDeleteKey", /* 63 */
    "NtDeleteObjectAuditAlarm", /* 64 */
    "NtDeleteValueKey", /* 65 */
    "NtDeviceIoControlFile", /* 66 */
    "NtDisplayString", /* 67 */
    "NtDuplicateObject", /* 68 */
    "NtDuplicateToken", /* 69 */
    "NtEnumerateBootEntries", /* 70 */
    "NtEnumerateKey", /* 71 */
    "NtEnumerateSystemEnvironmentValuesEx", /* 72 */
    "NtEnumerateValueKey", /* 73 */
    "NtExtendSection", /* 74 */
    "NtFilterToken", /* 75 */
    "NtFindAtom", /* 76 */
    "NtFlushBuffersFile", /* 77 */
    "NtFlushInstructionCache", /* 78 */
    "NtFlushKey", /* 79 */
    "NtFlushVirtualMemory", /* 80 */
    "NtFlushWriteBuffer", /* 81 */
    "NtFreeUserPhysicalPages", /* 82 */
    "NtFreeVirtualMemory", /* 83 */
    "NtFsControlFile", /* 84 */
    "NtGetContextThread", /* 85 */
    "NtGetDevicePowerState", /* 86 */
    "NtGetPlugPlayEvent", /* 87 */
    "NtGetWriteWatch", /* 88 */
    "NtImpersonateAnonymousToken", /* 89 */
    "NtImpersonateClientOfPort", /* 90 */
    "NtImpersonateThread", /* 91 */
    "NtInitializeRegistry", /* 92 */
    "NtInitiatePowerAction", /* 93 */
    "NtIsProcessInJob", /* 94 */
    "NtIsSystemResumeAutomatic", /* 95 */
    "NtListenPort", /* 96 */
    "NtLoadDriver", /* 97 */
    "NtLoadKey", /* 98 */
    "NtLoadKey2", /* 99 */
    "NtLockFile", /* 100 */
    "NtLockProductActivationKeys", /* 101 */
    "NtLockRegistryKey", /* 102 */
    "NtLockVirtualMemory", /* 103 */
    "NtMakePermanentObject", /* 104 */
    "NtMakeTemporaryObject", /* 105 */
    "NtMapUserPhysicalPages", /* 106 */
    "NtMapUserPhysicalPagesScatter", /* 107 */
    "NtMapViewOfSection", /* 108 */
    "NtModifyBootEntry", /* 109 */
    "NtNotifyChangeDirectoryFile", /* 110 */
    "NtNotifyChangeKey", /* 111 */
    "NtNotifyChangeMultipleKeys", /* 112 */
    "NtOpenDirectoryObject", /* 113 */
    "NtOpenEvent", /* 114 */
    "NtOpenEventPair", /* 115 */
    "NtOpenFile", /* 116 */
    "NtOpenIoCompletion", /* 117 */
    "NtOpenJobObject", /* 118 */
    "NtOpenKey", /* 119 */
    "NtOpenMutant", /* 120 */
    "NtOpenObjectAuditAlarm", /* 121 */
    "NtOpenProcess", /* 122 */
    "NtOpenProcessToken", /* 123 */
    "NtOpenProcessTokenEx", /* 124 */
    "NtOpenSection", /* 125 */
    "NtOpenSemaphore", /* 126 */
    "NtOpenSymbolicLinkObject", /* 127 */
    "NtOpenThread", /* 128 */
    "NtOpenThreadToken", /* 129 */
    "NtOpenThreadTokenEx", /* 130 */
    "NtOpenTimer", /* 131 */
    "NtPlugPlayControl", /* 132 */
    "NtPowerInformation", /* 133 */
    "NtPrivilegeCheck", /* 134 */
    "NtPrivilegeObjectAuditAlarm", /* 135 */
    "NtPrivilegedServiceAuditAlarm", /* 136 */
    "NtProtectVirtualMemory", /* 137 */
    "NtPulseEvent", /* 138 */
    "NtQueryAttributesFile", /* 139 */
    "NtQueryBootEntryOrder", /* 140 */
    "NtQueryBootOptions", /* 141 */
    "NtQueryDebugFilterState", /* 142 */
    "NtQueryDefaultLocale", /* 143 */
    "NtQueryDefaultUILanguage", /* 144 */
    "NtQueryDirectoryFile", /* 145 */
    "NtQueryDirectoryObject", /* 146 */
    "NtQueryEaFile", /* 147 */
    "NtQueryEvent", /* 148 */
    "NtQueryFullAttributesFile", /* 149 */
    "NtQueryInformationAtom", /* 150 */
    "NtQueryInformationFile", /* 151 */
    "NtQueryInformationJobObject", /* 152 */
    "NtQueryInformationPort", /* 153 */
    "NtQueryInformationProcess", /* 154 */
    "NtQueryInformationThread", /* 155 */
    "NtQueryInformationToken", /* 156 */
    "NtQueryInstallUILanguage", /* 157 */
    "NtQueryIntervalProfile", /* 158 */
    "NtQueryIoCompletion", /* 159 */
    "NtQueryKey", /* 160 */
    "NtQueryMultipleValueKey", /* 161 */
    "NtQueryMutant", /* 162 */
    "NtQueryObject", /* 163 */
    "NtQueryOpenSubKeys", /* 164 */
    "NtQueryPerformanceCounter", /* 165 */
    "NtQueryQuotaInformationFile", /* 166 */
    "NtQuerySection", /* 167 */
    "NtQuerySecurityObject", /* 168 */
    "NtQuerySemaphore", /* 169 */
    "NtQuerySymbolicLinkObject", /* 170 */
    "NtQuerySystemEnvironmentValue", /* 171 */
    "NtQuerySystemEnvironmentValueEx", /* 172 */
    "NtQuerySystemInformation", /* 173 */
    "NtQuerySystemTime", /* 174 */
    "NtQueryTimer", /* 175 */
    "NtQueryTimerResolution", /* 176 */
    "NtQueryValueKey", /* 177 */
    "NtQueryVirtualMemory", /* 178 */
    "NtQueryVolumeInformationFile", /* 179 */
    "NtQueueApcThread", /* 180 */
    "NtRaiseException", /* 181 */
    "NtRaiseHardError", /* 182 */
    "NtReadFile", /* 183 */
    "NtReadFileScatter", /* 184 */
    "NtReadRequestData", /* 185 */
    "NtReadVirtualMemory", /* 186 */
    "NtRegisterThreadTerminatePort", /* 187 */
    "NtReleaseMutant", /* 188 */
    "NtReleaseSemaphore", /* 189 */
    "NtRemoveIoCompletion", /* 190 */
    "NtRemoveProcessDebug", /* 191 */
    "NtRenameKey", /* 192 */
    "NtReplaceKey", /* 193 */
    "NtReplyPort", /* 194 */
    "NtReplyWaitReceivePort", /* 195 */
    "NtReplyWaitReceivePortEx", /* 196 */
    "NtReplyWaitReplyPort", /* 197 */
    "NtRequestDeviceWakeup", /* 198 */
    "NtRequestPort", /* 199 */
    "NtRequestWaitReplyPort", /* 200 */
    "NtRequestWakeupLatency", /* 201 */
    "NtResetEvent", /* 202 */
    "NtResetWriteWatch", /* 203 */
    "NtRestoreKey", /* 204 */
    "NtResumeProcess", /* 205 */
    "NtResumeThread", /* 206 */
    "NtSaveKey", /* 207 */
    "NtSaveKeyEx", /* 208 */
    "NtSaveMergedKeys", /* 209 */
    "NtSecureConnectPort", /* 210 */
    "NtSetBootEntryOrder", /* 211 */
    "NtSetBootOptions", /* 212 */
    "NtSetContextThread", /* 213 */
    "NtSetDebugFilterState", /* 214 */
    "NtSetDefaultHardErrorPort", /* 215 */
    "NtSetDefaultLocale", /* 216 */
    "NtSetDefaultUILanguage", /* 217 */
    "NtSetEaFile", /* 218 */
    "NtSetEvent", /* 219 */
    "NtSetEventBoostPriority", /* 220 */
    "NtSetHighEventPair", /* 221 */
    "NtSetHighWaitLowEventPair", /* 222 */
    "NtSetInformationDebugObject", /* 223 */
    "NtSetInformationFile", /* 224 */
    "NtSetInformationJobObject", /* 225 */
    "NtSetInformationKey", /* 226 */
    "NtSetInformationObject", /* 227 */
    "NtSetInformationProcess", /* 228 */
    "NtSetInformationThread", /* 229 */
    "NtSetInformationToken", /* 230 */
    "NtSetIntervalProfile", /* 231 */
    "NtSetIoCompletion", /* 232 */
    "NtSetLdtEntries", /* 233 */
    "NtSetLowEventPair", /* 234 */
    "NtSetLowWaitHighEventPair", /* 235 */
    "NtSetQuotaInformationFile", /* 236 */
    "NtSetSecurityObject", /* 237 */
    "NtSetSystemEnvironmentValue", /* 238 */
    "NtSetSystemEnvironmentValueEx", /* 239 */
    "NtSetSystemInformation", /* 240 */
    "NtSetSystemPowerState", /* 241 */
    "NtSetSystemTime", /* 242 */
    "NtSetThreadExecutionState", /* 243 */
    "NtSetTimer", /* 244 */
    "NtSetTimerResolution", /* 245 */
    "NtSetUuidSeed", /* 246 */
    "NtSetValueKey", /* 247 */
    "NtSetVolumeInformationFile", /* 248 */
    "NtShutdownSystem", /* 249 */
    "NtSignalAndWaitForSingleObject", /* 250 */
    "NtStartProfile", /* 251 */
    "NtStopProfile", /* 252 */
    "NtSuspendProcess", /* 253 */
    "NtSuspendThread", /* 254 */
    "NtSystemDebugControl", /* 255 */
    "NtTerminateJobObject", /* 256 */
    "NtTerminateProcess", /* 257 */
    "NtTerminateThread", /* 258 */
    "NtTestAlert", /* 259 */
    "NtTraceEvent", /* 260 */
    "NtTranslateFilePath", /* 261 */
    "NtUnloadDriver", /* 262 */
    "NtUnloadKey", /* 263 */
    "NtUnloadKeyEx", /* 264 */
    "NtUnlockFile", /* 265 */
    "NtUnlockVirtualMemory", /* 266 */
    "NtUnmapViewOfSection", /* 267 */
    "NtVdmControl", /* 268 */
    "NtWaitForDebugEvent", /* 269 */
    "NtWaitForMultipleObjects", /* 270 */
    "NtWaitForSingleObject", /* 271 */
    "NtWaitHighEventPair", /* 272 */
    "NtWaitLowEventPair", /* 273 */
    "NtWriteFile", /* 274 */
    "NtWriteFileGather", /* 275 */
    "NtWriteRequestData", /* 276 */
    "NtWriteVirtualMemory", /* 277 */
    "NtYieldExecution", /* 278 */
    "NtCreateKeyedEvent", /* 279 */
    "NtOpenKeyedEvent", /* 280 */
    "NtReleaseKeyedEvent", /* 281 */
    "NtWaitForKeyedEvent", /* 282 */
    "NtQueryPortInformationProcess" /* 283 */
};

/* System call names in the SSDT of Windows XP.
*/
#define     COUNT_SHADOW_SYSCALL_WINXP     700
static const char shadow_syscall_windows_xp[COUNT_SHADOW_SYSCALL_WINXP][MAX_NAME_LENGTH] = 
{
    "NtGdiAbortDoc", /* 0 */
    "NtGdiAbortPath", /* 1 */
    "NtGdiAddFontResourceW", /* 2 */
    "NtGdiAddRemoteFontToDC", /* 3 */
    "NtGdiAddFontMemResourceEx", /* 4 */
    "NtGdiRemoveMergeFont", /* 5 */
    "NtGdiAddRemoteMMInstanceToDC", /* 6 */
    "NtGdiAlphaBlend ", /* 7 */
    "NtGdiAngleArc", /* 8 */
    "NtGdiAnyLinkedFonts", /* 9 */
    "NtGdiFontIsLinked", /* 10 */
    "NtGdiArcInternal ", /* 11 */
    "NtGdiBeginPath", /* 12 */
    "NtGdiBitBlt ", /* 13 */
    "NtGdiCancelDC", /* 14 */
    "NtGdiCheckBitmapBits", /* 15 */
    "NtGdiCloseFigure", /* 16 */
    "NtGdiClearBitmapAttributes", /* 17 */
    "NtGdiClearBrushAttributes", /* 18 */
    "NtGdiColorCorrectPalette", /* 19 */
    "NtGdiCombineRgn", /* 20 */
    "NtGdiCombineTransform", /* 21 */
    "NtGdiComputeXformCoefficients", /* 22 */
    "NtGdiConsoleTextOut", /* 23 */
    "NtGdiConvertMetafileRect", /* 24 */
    "NtGdiCreateBitmap", /* 25 */
    "NtGdiCreateClientObj", /* 26 */
    "NtGdiCreateColorSpace", /* 27 */
    "NtGdiCreateColorTransform", /* 28 */
    "NtGdiCreateCompatibleBitmap", /* 29 */
    "NtGdiCreateCompatibleDC", /* 30 */
    "NtGdiCreateDIBBrush", /* 31 */
    "NtGdiCreateDIBitmapInternal ", /* 32 */
    "NtGdiCreateDIBSection", /* 33 */
    "NtGdiCreateEllipticRgn", /* 34 */
    "NtGdiCreateHalftonePalette", /* 35 */
    "NtGdiCreateHatchBrushInternal", /* 36 */
    "NtGdiCreateMetafileDC", /* 37 */
    "NtGdiCreatePaletteInternal", /* 38 */
    "NtGdiCreatePatternBrushInternal", /* 39 */
    "NtGdiCreatePen", /* 40 */
    "NtGdiCreateRectRgn", /* 41 */
    "NtGdiCreateRoundRectRgn", /* 42 */
    "NtGdiCreateServerMetaFile", /* 43 */
    "NtGdiCreateSolidBrush", /* 44 */
    "NtGdiD3dContextCreate", /* 45 */
    "NtGdiD3dContextDestroy", /* 46 */
    "NtGdiD3dContextDestroyAll", /* 47 */
    "NtGdiD3dValidateTextureStageState", /* 48 */
    "NtGdiD3dDrawPrimitives2", /* 49 */
    "NtGdiDdGetDriverState", /* 50 */
    "NtGdiDdAddAttachedSurface", /* 51 */
    "NtGdiDdAlphaBlt", /* 52 */
    "NtGdiDdAttachSurface", /* 53 */
    "NtGdiDdBeginMoCompFrame", /* 54 */
    "NtGdiDdBlt", /* 55 */
    "NtGdiDdCanCreateSurface", /* 56 */
    "NtGdiDdCanCreateD3DBuffer", /* 57 */
    "NtGdiDdColorControl", /* 58 */
    "NtGdiDdCreateDirectDrawObject", /* 59 */
    "NtGdiDdCreateSurface", /* 60 */
    "NtGdiDdCreateD3DBuffer", /* 61 */
    "NtGdiDdCreateMoComp", /* 62 */
    "NtGdiDdCreateSurfaceObject", /* 63 */
    "NtGdiDdDeleteDirectDrawObject", /* 64 */
    "NtGdiDdDeleteSurfaceObject", /* 65 */
    "NtGdiDdDestroyMoComp", /* 66 */
    "NtGdiDdDestroySurface", /* 67 */
    "NtGdiDdDestroyD3DBuffer", /* 68 */
    "NtGdiDdEndMoCompFrame", /* 69 */
    "NtGdiDdFlip", /* 70 */
    "NtGdiDdFlipToGDISurface", /* 71 */
    "NtGdiDdGetAvailDriverMemory", /* 72 */
    "NtGdiDdGetBltStatus", /* 73 */
    "NtGdiDdGetDC", /* 74 */
    "NtGdiDdGetDriverInfo", /* 75 */
    "NtGdiDdGetDxHandle", /* 76 */
    "NtGdiDdGetFlipStatus", /* 77 */
    "NtGdiDdGetInternalMoCompInfo", /* 78 */
    "NtGdiDdGetMoCompBuffInfo", /* 79 */
    "NtGdiDdGetMoCompGuids", /* 80 */
    "NtGdiDdGetMoCompFormats", /* 81 */
    "NtGdiDdGetScanLine", /* 82 */
    "NtGdiDdLock", /* 83 */
    "NtGdiDdLockD3D", /* 84 */
    "NtGdiDdQueryDirectDrawObject ", /* 85 */
    "NtGdiDdQueryMoCompStatus", /* 86 */
    "NtGdiDdReenableDirectDrawObject", /* 87 */
    "NtGdiDdReleaseDC", /* 88 */
    "NtGdiDdRenderMoComp", /* 89 */
    "NtGdiDdResetVisrgn", /* 90 */
    "NtGdiDdSetColorKey", /* 91 */
    "NtGdiDdSetExclusiveMode", /* 92 */
    "NtGdiDdSetGammaRamp", /* 93 */
    "NtGdiDdCreateSurfaceEx", /* 94 */
    "NtGdiDdSetOverlayPosition", /* 95 */
    "NtGdiDdUnattachSurface", /* 96 */
    "NtGdiDdUnlock", /* 97 */
    "NtGdiDdUnlockD3D", /* 98 */
    "NtGdiDdUpdateOverlay", /* 99 */
    "NtGdiDdWaitForVerticalBlank", /* 100 */
    "NtGdiDvpCanCreateVideoPort", /* 101 */
    "NtGdiDvpColorControl", /* 102 */
    "NtGdiDvpCreateVideoPort", /* 103 */
    "NtGdiDvpDestroyVideoPort", /* 104 */
    "NtGdiDvpFlipVideoPort", /* 105 */
    "NtGdiDvpGetVideoPortBandwidth", /* 106 */
    "NtGdiDvpGetVideoPortField", /* 107 */
    "NtGdiDvpGetVideoPortFlipStatus", /* 108 */
    "NtGdiDvpGetVideoPortInputFormats", /* 109 */
    "NtGdiDvpGetVideoPortLine", /* 110 */
    "NtGdiDvpGetVideoPortOutputFormats", /* 111 */
    "NtGdiDvpGetVideoPortConnectInfo", /* 112 */
    "NtGdiDvpGetVideoSignalStatus", /* 113 */
    "NtGdiDvpUpdateVideoPort", /* 114 */
    "NtGdiDvpWaitForVideoPortSync", /* 115 */
    "NtGdiDvpAcquireNotification", /* 116 */
    "NtGdiDvpReleaseNotification", /* 117 */
    "NtGdiDxgGenericThunk", /* 118 */
    "NtGdiDeleteClientObj", /* 119 */
    "NtGdiDeleteColorSpace", /* 120 */
    "NtGdiDeleteColorTransform", /* 121 */
    "NtGdiDeleteObjectApp", /* 122 */
    "NtGdiDescribePixelFormat", /* 123 */
    "NtGdiGetPerBandInfo", /* 124 */
    "NtGdiDoBanding", /* 125 */
    "NtGdiDoPalette", /* 126 */
    "NtGdiDrawEscape", /* 127 */
    "NtGdiEllipse", /* 128 */
    "NtGdiEnableEudc", /* 129 */
    "NtGdiEndDoc", /* 130 */
    "NtGdiEndPage", /* 131 */
    "NtGdiEndPath", /* 132 */
    "NtGdiEnumFontChunk", /* 133 */
    "NtGdiEnumFontClose", /* 134 */
    "NtGdiEnumFontOpen", /* 135 */
    "NtGdiEnumObjects", /* 136 */
    "NtGdiEqualRgn", /* 137 */
    "NtGdiEudcLoadUnloadLink", /* 138 */
    "NtGdiExcludeClipRect", /* 139 */
    "NtGdiExtCreatePen ", /* 140 */
    "NtGdiExtCreateRegion", /* 141 */
    "NtGdiExtEscape", /* 142 */
    "NtGdiExtFloodFill", /* 143 */
    "NtGdiExtGetObjectW", /* 144 */
    "NtGdiExtSelectClipRgn", /* 145 */
    "NtGdiExtTextOutW", /* 146 */
    "NtGdiFillPath", /* 147 */
    "NtGdiFillRgn", /* 148 */
    "NtGdiFlattenPath", /* 149 */
    "NtGdiFlushUserBatch", /* 150 */
    "NtGdiFlush", /* 151 */
    "NtGdiForceUFIMapping", /* 152 */
    "NtGdiFrameRgn", /* 153 */
    "NtGdiFullscreenControl", /* 154 */
    "NtGdiGetAndSetDCDword", /* 155 */
    "NtGdiGetAppClipBox", /* 156 */
    "NtGdiGetBitmapBits", /* 157 */
    "NtGdiGetBitmapDimension", /* 158 */
    "NtGdiGetBoundsRect", /* 159 */
    "NtGdiGetCharABCWidthsW", /* 160 */
    "NtGdiGetCharacterPlacementW", /* 161 */
    "NtGdiGetCharSet", /* 162 */
    "NtGdiGetCharWidthW", /* 163 */
    "NtGdiGetCharWidthInfo", /* 164 */
    "NtGdiGetColorAdjustment", /* 165 */
    "NtGdiGetColorSpaceforBitmap", /* 166 */
    "NtGdiGetDCDword", /* 167 */
    "NtGdiGetDCforBitmap", /* 168 */
    "NtGdiGetDCObject", /* 169 */
    "NtGdiGetDCPoint", /* 170 */
    "NtGdiGetDeviceCaps", /* 171 */
    "NtGdiGetDeviceGammaRamp", /* 172 */
    "NtGdiGetDeviceCapsAll", /* 173 */
    "NtGdiGetDIBitsInternal", /* 174 */
    "NtGdiGetETM", /* 175 */
    "NtGdiGetEudcTimeStampEx", /* 176 */
    "NtGdiGetFontData", /* 177 */
    "NtGdiGetFontResourceInfoInternalW", /* 178 */
    "NtGdiGetGlyphIndicesW", /* 179 */
    "NtGdiGetGlyphIndicesWInternal", /* 180 */
    "NtGdiGetGlyphOutline", /* 181 */
    "NtGdiGetKerningPairs", /* 182 */
    "NtGdiGetLinkedUFIs", /* 183 */
    "NtGdiGetMiterLimit", /* 184 */
    "NtGdiGetMonitorID", /* 185 */
    "NtGdiGetNearestColor", /* 186 */
    "NtGdiGetNearestPaletteIndex", /* 187 */
    "NtGdiGetObjectBitmapHandle", /* 188 */
    "NtGdiGetOutlineTextMetricsInternalW", /* 189 */
    "NtGdiGetPath", /* 190 */
    "NtGdiGetPixel", /* 191 */
    "NtGdiGetRandomRgn", /* 192 */
    "NtGdiGetRasterizerCaps", /* 193 */
    "NtGdiGetRealizationInfo", /* 194 */
    "NtGdiGetRegionData", /* 195 */
    "NtGdiGetRgnBox", /* 196 */
    "NtGdiGetServerMetaFileBits", /* 197 */
    "NtGdiGetSpoolMessage", /* 198 */
    "NtGdiGetStats", /* 199 */
    "NtGdiGetStockObject", /* 200 */
    "NtGdiGetStringBitmapW", /* 201 */
    "NtGdiGetSystemPaletteUse", /* 202 */
    "NtGdiGetTextCharsetInfo", /* 203 */
    "NtGdiGetTextExtent", /* 204 */
    "NtGdiGetTextExtentExW", /* 205 */
    "NtGdiGetTextFaceW", /* 206 */
    "NtGdiGetTextMetricsW", /* 207 */
    "NtGdiGetTransform", /* 208 */
    "NtGdiGetUFI", /* 209 */
    "NtGdiGetEmbUFI", /* 210 */
    "NtGdiGetUFIPathname ", /* 211 */
    "NtGdiGetEmbedFonts", /* 212 */
    "NtGdiChangeGhostFont", /* 213 */
    "NtGdiAddEmbFontToDC", /* 214 */
    "NtGdiGetFontUnicodeRanges", /* 215 */
    "NtGdiGetWidthTable", /* 216 */
    "NtGdiGradientFill", /* 217 */
    "NtGdiHfontCreate", /* 218 */
    "NtGdiIcmBrushInfo", /* 219 */
    "NtGdiInit", /* 220 */
    "NtGdiInitSpool", /* 221 */
    "NtGdiIntersectClipRect", /* 222 */
    "NtGdiInvertRgn", /* 223 */
    "NtGdiLineTo", /* 224 */
    "NtGdiMakeFontDir", /* 225 */
    "NtGdiMakeInfoDC", /* 226 */
    "NtGdiMaskBlt ", /* 227 */
    "NtGdiModifyWorldTransform", /* 228 */
    "NtGdiMonoBitmap", /* 229 */
    "NtGdiMoveTo", /* 230 */
    "NtGdiOffsetClipRgn", /* 231 */
    "NtGdiOffsetRgn", /* 232 */
    "NtGdiOpenDCW", /* 233 */
    "NtGdiPatBlt", /* 234 */
    "NtGdiPolyPatBlt", /* 235 */
    "NtGdiPathToRegion", /* 236 */
    "NtGdiPlgBlt ", /* 237 */
    "NtGdiPolyDraw", /* 238 */
    "NtGdiPolyPolyDraw", /* 239 */
    "NtGdiPolyTextOutW", /* 240 */
    "NtGdiPtInRegion", /* 241 */
    "NtGdiPtVisible", /* 242 */
    "NtGdiQueryFonts", /* 243 */
    "NtGdiQueryFontAssocInfo", /* 244 */
    "NtGdiRectangle", /* 245 */
    "NtGdiRectInRegion", /* 246 */
    "NtGdiRectVisible", /* 247 */
    "NtGdiRemoveFontResourceW", /* 248 */
    "NtGdiRemoveFontMemResourceEx", /* 249 */
    "NtGdiResetDC", /* 250 */
    "NtGdiResizePalette", /* 251 */
    "NtGdiRestoreDC", /* 252 */
    "NtGdiRoundRect", /* 253 */
    "NtGdiSaveDC", /* 254 */
    "NtGdiScaleViewportExtEx", /* 255 */
    "NtGdiScaleWindowExtEx", /* 256 */
    "NtGdiSelectBitmap", /* 257 */
    "NtGdiSelectBrush", /* 258 */
    "NtGdiSelectClipPath", /* 259 */
    "NtGdiSelectFont", /* 260 */
    "NtGdiSelectPen", /* 261 */
    "NtGdiSetBitmapAttributes", /* 262 */
    "NtGdiSetBitmapBits ", /* 263 */
    "NtGdiSetBitmapDimension", /* 264 */
    "NtGdiSetBoundsRect", /* 265 */
    "NtGdiSetBrushAttributes", /* 266 */
    "NtGdiSetBrushOrg ", /* 267 */
    "NtGdiSetColorAdjustment", /* 268 */
    "NtGdiSetColorSpace ", /* 269 */
    "NtGdiSetDeviceGammaRamp", /* 270 */
    "NtGdiSetDIBitsToDeviceInternal", /* 271 */
    "NtGdiSetFontEnumeration", /* 272 */
    "NtGdiSetFontXform", /* 273 */
    "NtGdiSetIcmMode", /* 274 */
    "NtGdiSetLinkedUFIs", /* 275 */
    "NtGdiSetMagicColors", /* 276 */
    "NtGdiSetMetaRgn", /* 277 */
    "NtGdiSetMiterLimit", /* 278 */
    "NtGdiGetDeviceWidth", /* 279 */
    "NtGdiMirrorWindowOrg", /* 280 */
    "NtGdiSetLayout", /* 281 */
    "NtGdiSetPixel", /* 282 */
    "NtGdiSetPixelFormat", /* 283 */
    "NtGdiSetRectRgn", /* 284 */
    "NtGdiSetSystemPaletteUse", /* 285 */
    "NtGdiSetTextJustification", /* 286 */
    "NtGdiSetupPublicCFONT", /* 287 */
    "NtGdiSetVirtualResolution ", /* 288 */
    "NtGdiSetSizeDevice", /* 289 */
    "NtGdiStartDoc", /* 290 */
    "NtGdiStartPage", /* 291 */
    "NtGdiStretchBlt", /* 292 */
    "NtGdiStretchDIBitsInternal", /* 293 */
    "NtGdiStrokeAndFillPath", /* 294 */
    "NtGdiStrokePath", /* 295 */
    "NtGdiSwapBuffers", /* 296 */
    "NtGdiTransformPoints", /* 297 */
    "NtGdiTransparentBlt", /* 298 */
    "NtGdiUnloadPrinterDriver", /* 299 */
    "NtGdiUnmapMemFont", /* 300 */
    "NtGdiUnrealizeObject", /* 301 */
    "NtGdiUpdateColors", /* 302 */
    "NtGdiWidenPath", /* 303 */
    "NtUserActivateKeyboardLayout", /* 304 */
    "NtUserAlterWindowStyle", /* 305 */
    "NtUserAssociateInputContext", /* 306 */
    "NtUserAttachThreadInput", /* 307 */
    "NtUserBeginPaint", /* 308 */
    "NtUserBitBltSysBmp", /* 309 */
    "NtUserBlockInput", /* 310 */
    "NtUserBuildHimcList", /* 311 */
    "NtUserBuildHwndList", /* 312 */
    "NtUserBuildNameList", /* 313 */
    "NtUserBuildPropList", /* 314 */
    "NtUserCallHwnd", /* 315 */
    "NtUserCallHwndLock", /* 316 */
    "NtUserCallHwndOpt", /* 317 */
    "NtUserCallHwndParam", /* 318 */
    "NtUserCallHwndParamLock", /* 319 */
    "NtUserCallMsgFilter", /* 320 */
    "NtUserCallNextHookEx", /* 321 */
    "NtUserCallNoParam", /* 322 */
    "NtUserCallOneParam", /* 323 */
    "NtUserCallTwoParam", /* 324 */
    "NtUserChangeClipboardChain", /* 325 */
    "NtUserChangeDisplaySettings", /* 326 */
    "NtUserCheckImeHotKey", /* 327 */
    "NtUserCheckMenuItem", /* 328 */
    "NtUserChildWindowFromPointEx", /* 329 */
    "NtUserClipCursor", /* 330 */
    "NtUserCloseClipboard", /* 331 */
    "NtUserCloseDesktop", /* 332 */
    "NtUserCloseWindowStation", /* 333 */
    "NtUserConsoleControl", /* 334 */
    "NtUserConvertMemHandle", /* 335 */
    "NtUserCopyAcceleratorTable", /* 336 */
    "NtUserCountClipboardFormats", /* 337 */
    "NtUserCreateAcceleratorTable", /* 338 */
    "NtUserCreateCaret", /* 339 */
    "NtUserCreateDesktop", /* 340 */
    "NtUserCreateInputContext ", /* 341 */
    "NtUserCreateLocalMemHandle", /* 342 */
    "NtUserCreateWindowEx", /* 343 */
    "NtUserCreateWindowStation", /* 344 */
    "NtUserDdeGetQualityOfService", /* 345 */
    "NtUserDdeInitialize", /* 346 */
    "NtUserDdeSetQualityOfService", /* 347 */
    "NtUserDeferWindowPos", /* 348 */
    "NtUserDefSetText", /* 349 */
    "NtUserDeleteMenu", /* 350 */
    "NtUserDestroyAcceleratorTable", /* 351 */
    "NtUserDestroyCursor", /* 352 */
    "NtUserDestroyInputContext", /* 353 */
    "NtUserDestroyMenu", /* 354 */
    "NtUserDestroyWindow", /* 355 */
    "NtUserDisableThreadIme", /* 356 */
    "NtUserDispatchMessage", /* 357 */
    "NtUserDragDetect", /* 358 */
    "NtUserDragObject", /* 359 */
    "NtUserDrawAnimatedRects", /* 360 */
    "NtUserDrawCaption", /* 361 */
    "NtUserDrawCaptionTemp", /* 362 */
    "NtUserDrawIconEx", /* 363 */
    "NtUserDrawMenuBarTemp", /* 364 */
    "NtUserEmptyClipboard", /* 365 */
    "NtUserEnableMenuItem", /* 366 */
    "NtUserEnableScrollBar", /* 367 */
    "NtUserEndMenu", /* 369 */
    "NtUserEndPaint", /* 370 */
    "NtUserEnumDisplayDevices", /* 371 */
    "NtUserEnumDisplayMonitors", /* 372 */
    "NtUserEnumDisplaySettings", /* 373 */
    "NtUserEvent", /* 374 */
    "NtUserExcludeUpdateRgn", /* 375 */
    "NtUserFillWindow", /* 376 */
    "NtUserFindExistingCursorIcon", /* 377 */
    "NtUserFindWindowEx", /* 378 */
    "NtUserFlashWindowEx", /* 379 */
    "NtUserGetAltTabInfo", /* 380 */
    "NtUserGetAncestor", /* 381 */
    "NtUserGetAppImeLevel", /* 382 */
    "NtUserGetAsyncKeyState", /* 383 */
    "NtUserGetAtomName", /* 384 */
    "NtUserGetCaretBlinkTime", /* 385 */
    "NtUserGetCaretPos", /* 386 */
    "NtUserGetClassInfo", /* 387 */
    "NtUserGetClassName", /* 388 */
    "NtUserGetClipboardData", /* 389 */
    "NtUserGetClipboardFormatName", /* 390 */
    "NtUserGetClipboardOwner", /* 391 */
    "NtUserGetClipboardSequenceNumber", /* 392 */
    "NtUserGetClipboardViewer", /* 393 */
    "NtUserGetClipCursor", /* 394 */
    "NtUserGetComboBoxInfo", /* 395 */
    "NtUserGetControlBrush ", /* 396 */
    "NtUserGetControlColor", /* 397 */
    "NtUserGetCPD", /* 398 */
    "NtUserGetCursorFrameInfo", /* 399 */
    "NtUserGetCursorInfo", /* 400 */
    "NtUserGetDC", /* 401 */
    "NtUserGetDCEx", /* 402 */
    "NtUserGetDoubleClickTime", /* 403 */
    "NtUserGetForegroundWindow", /* 404 */
    "NtUserGetGuiResources", /* 405 */
    "NtUserGetGUIThreadInfo", /* 406 */
    "NtUserGetIconInfo", /* 407 */
    "NtUserGetIconSize", /* 408 */
    "NtUserGetImeHotKey", /* 409 */
    "NtUserGetImeInfoEx", /* 410 */
    "NtUserGetInternalWindowPos", /* 411 */
    "NtUserGetKeyboardLayoutList", /* 412 */
    "NtUserGetKeyboardLayoutName", /* 413 */
    "NtUserGetKeyboardState", /* 414 */
    "NtUserGetKeyNameText", /* 415 */
    "NtUserGetKeyState", /* 416 */
    "NtUserGetListBoxInfo", /* 417 */
    "NtUserGetMenuBarInfo", /* 418 */
    "NtUserGetMenuIndex", /* 419 */
    "NtUserGetMenuItemRect", /* 420 */
    "NtUserGetMessage", /* 421 */
    "NtUserGetMouseMovePointsEx", /* 422 */
    "NtUserGetObjectInformation", /* 423 */
    "NtUserGetOpenClipboardWindow", /* 424 */
    "NtUserGetPriorityClipboardFormat", /* 425 */
    "NtUserGetProcessWindowStation", /* 426 */
    "NtUserGetRawInputBuffer", /* 427 */
    "NtUserGetRawInputData", /* 428 */
    "NtUserGetRawInputDeviceInfo", /* 429 */
    "NtUserGetRawInputDeviceList", /* 430 */
    "NtUserGetRegisteredRawInputDevices", /* 431 */
    "NtUserGetScrollBarInfo", /* 432 */
    "NtUserGetSystemMenu", /* 433 */
    "NtUserGetThreadDesktop", /* 434 */
    "NtUserGetThreadState", /* 435 */
    "NtUserGetTitleBarInfo", /* 436 */
    "NtUserGetUpdateRect", /* 437 */
    "NtUserGetUpdateRgn", /* 438 */
    "NtUserGetWindowDC", /* 439 */
    "NtUserGetWindowPlacement", /* 440 */
    "NtUserGetWOWClass", /* 441 */
    "NtUserHardErrorControl", /* 442 */
    "NtUserHideCaret", /* 443 */
    "NtUserHiliteMenuItem", /* 444 */
    "NtUserImpersonateDdeClientWindow", /* 445 */
    "NtUserInitialize", /* 446 */
    "NtUserInitializeClientPfnArrays", /* 447 */
    "NtUserInitTask", /* 448 */
    "NtUserInternalGetWindowText", /* 449 */
    "NtUserInvalidateRect", /* 450 */
    "NtUserInvalidateRgn", /* 451 */
    "NtUserIsClipboardFormatAvailable", /* 452 */
    "NtUserKillTimer", /* 453 */
    "NtUserLoadKeyboardLayoutEx", /* 454 */
    "NtUserLockWindowStation", /* 455 */
    "NtUserLockWindowUpdate", /* 456 */
    "NtUserLockWorkStation", /* 457 */
    "NtUserMapVirtualKeyEx", /* 458 */
    "NtUserMenuItemFromPoint", /* 459 */
    "NtUserMessageCall", /* 460 */
    "NtUserMinMaximize", /* 461 */
    "NtUserMNDragLeave", /* 462 */
    "NtUserMNDragOver", /* 463 */
    "NtUserModifyUserStartupInfoFlags", /* 464 */
    "NtUserMoveWindow", /* 465 */
    "NtUserNotifyIMEStatus", /* 466 */
    "NtUserNotifyProcessCreate ", /* 467 */
    "NtUserNotifyWinEvent", /* 468 */
    "NtUserOpenClipboard", /* 469 */
    "NtUserOpenDesktop", /* 470 */
    "NtUserOpenInputDesktop", /* 471 */
    "NtUserOpenWindowStation", /* 472 */
    "NtUserPaintDesktop", /* 473 */
    "NtUserPeekMessage", /* 474 */
    "NtUserPostMessage", /* 475 */
    "NtUserPostThreadMessage", /* 476 */
    "NtUserPrintWindow", /* 477 */
    "NtUserProcessConnect", /* 478 */
    "NtUserQueryInformationThread", /* 479 */
    "NtUserQueryInputContext", /* 480 */
    "NtUserQuerySendMessage", /* 481 */
    "NtUserQueryUserCounters", /* 482 */
    "NtUserQueryWindow ", /* 483 */
    "NtUserRealChildWindowFromPoint", /* 484 */
    "NtUserRealInternalGetMessage", /* 485 */
    "NtUserRealWaitMessageEx", /* 486 */
    "NtUserRedrawWindow", /* 487 */
    "NtUserRegisterClassExWOW", /* 488 */
    "NtUserRegisterUserApiHook", /* 489 */
    "NtUserRegisterHotKey", /* 490 */
    "NtUserRegisterRawInputDevices", /* 491 */
    "NtUserRegisterTasklist", /* 492 */
    "NtUserRegisterWindowMessage ", /* 493 */
    "NtUserRemoveMenu", /* 494 */
    "NtUserRemoveProp", /* 495 */
    "NtUserResolveDesktop", /* 496 */
    "NtUserResolveDesktopForWOW", /* 497 */
    "NtUserSBGetParms", /* 498 */
    "NtUserScrollDC", /* 499 */
    "NtUserScrollWindowEx", /* 500 */
    "NtUserSelectPalette", /* 501 */
    "NtUserSendInput", /* 502 */
    "NtUserSetActiveWindow", /* 503 */
    "NtUserSetAppImeLevel", /* 504 */
    "NtUserSetCapture", /* 505 */
    "NtUserSetClassLong", /* 506 */
    "NtUserSetClassWord", /* 507 */
    "NtUserSetClipboardData", /* 508 */
    "NtUserSetClipboardViewer", /* 509 */
    "NtUserSetConsoleReserveKeys", /* 510 */
    "NtUserSetCursor", /* 511 */
    "NtUserSetCursorContents", /* 512 */
    "NtUserSetCursorIconData", /* 513 */
    "NtUserSetDbgTag", /* 514 */
    "NtUserSetFocus", /* 515 */
    "NtUserSetImeHotKey", /* 516 */
    "NtUserSetImeInfoEx", /* 517 */
    "NtUserSetImeOwnerWindow", /* 518 */
    "NtUserSetInformationProcess ", /* 519 */
    "NtUserSetInformationThread", /* 520 */
    "NtUserSetInternalWindowPos", /* 521 */
    "NtUserSetKeyboardState", /* 522 */
    "NtUserSetLogonNotifyWindow ", /* 523 */
    "NtUserSetMenu", /* 524 */
    "NtUserSetMenuContextHelpId ", /* 525 */
    "NtUserSetMenuDefaultItem", /* 526 */
    "NtUserSetMenuFlagRtoL", /* 527 */
    "NtUserSetObjectInformation", /* 528 */
    "NtUserSetParent", /* 529 */
    "NtUserSetProcessWindowStation", /* 530 */
    "NtUserSetProp", /* 531 */
    "NtUserSetRipFlags", /* 532 */
    "NtUserSetScrollInfo", /* 533 */
    "NtUserSetShellWindowEx", /* 534 */
    "NtUserSetSysColors", /* 535 */
    "NtUserSetSystemCursor", /* 536 */
    "NtUserSetSystemMenu", /* 537 */
    "NtUserSetSystemTimer", /* 538 */
    "NtUserSetThreadDesktop", /* 539 */
    "NtUserSetThreadLayoutHandles", /* 540 */
    "NtUserSetThreadState", /* 541 */
    "NtUserSetTimer", /* 542 */
    "NtUserSetWindowFNID", /* 543 */
    "NtUserSetWindowLong ", /* 544 */
    "NtUserSetWindowPlacement", /* 545 */
    "NtUserSetWindowPos", /* 546 */
    "NtUserSetWindowRgn", /* 547 */
    "NtUserSetWindowsHookAW", /* 548 */
    "NtUserSetWindowsHookEx", /* 549 */
    "NtUserSetWindowStationUser", /* 550 */
    "NtUserSetWindowWord", /* 551 */
    "NtUserSetWinEventHook", /* 552 */
    "NtUserShowCaret", /* 553 */
    "NtUserShowScrollBar", /* 554 */
    "NtUserShowWindow", /* 555 */
    "NtUserShowWindowAsync", /* 556 */
    "NtUserSoundSentry", /* 557 */
    "NtUserSwitchDesktop", /* 558 */
    "NtUserSystemParametersInfo", /* 559 */
    "NtUserTestForInteractiveUser", /* 560 */
    "NtUserThunkedMenuInfo", /* 561 */
    "NtUserThunkedMenuItemInfo", /* 562 */
    "NtUserToUnicodeEx", /* 563 */
    "NtUserTrackMouseEvent", /* 564 */
    "NtUserTrackPopupMenuEx", /* 565 */
    "NtUserCalcMenuBar", /* 566 */
    "NtUserPaintMenuBar", /* 567 */
    "NtUserTranslateAccelerator", /* 568 */
    "NtUserTranslateMessage", /* 569 */
    "NtUserUnhookWindowsHookEx", /* 570 */
    "NtUserUnhookWinEvent", /* 571 */
    "NtUserUnloadKeyboardLayout", /* 572 */
    "NtUserUnlockWindowStation", /* 573 */
    "NtUserUnregisterClass", /* 574 */
    "NtUserUnregisterUserApiHook", /* 575 */
    "NtUserUnregisterHotKey", /* 576 */
    "NtUserUpdateInputContext", /* 577 */
    "NtUserUpdateInstance", /* 578 */
    "NtUserUpdateLayeredWindow", /* 579 */
    "NtUserGetLayeredWindowAttributes", /* 580 */
    "NtUserSetLayeredWindowAttributes", /* 581 */
    "NtUserUpdatePerUserSystemParameters", /* 582 */
    "NtUserUserHandleGrantAccess", /* 583 */
    "NtUserValidateHandleSecure", /* 584 */
    "NtUserValidateRect", /* 585 */
    "NtUserValidateTimerCallback", /* 586 */
    "NtUserVkKeyScanEx", /* 587 */
    "NtUserWaitForInputIdle", /* 588 */
    "NtUserWaitForMsgAndEvent", /* 589 */
    "NtUserWaitMessage", /* 590 */
    "NtUserWin32PoolAllocationStats", /* 591 */
    "NtUserWindowFromPoint", /* 592 */
    "NtUserYieldTask", /* 593 */
    "NtUserRemoteConnect", /* 594 */
    "NtUserRemoteRedrawRectangle", /* 595 */
    "NtUserRemoteRedrawScreen", /* 596 */
    "NtUserRemoteStopScreenUpdates ", /* 597 */
    "NtUserCtxDisplayIOCtl", /* 598 */
    "NtGdiEngAssociateSurface", /* 599 */
    "NtGdiEngCreateBitmap", /* 600 */
    "NtGdiEngCreateDeviceSurface", /* 601 */
    "NtGdiEngCreateDeviceBitmap", /* 602 */
    "NtGdiEngCreatePalette", /* 603 */
    "NtGdiEngComputeGlyphSet", /* 604 */
    "NtGdiEngCopyBits", /* 605 */
    "NtGdiEngDeletePalette", /* 606 */
    "NtGdiEngDeleteSurface", /* 607 */
    "NtGdiEngEraseSurface", /* 608 */
    "NtGdiEngUnlockSurface", /* 609 */
    "NtGdiEngLockSurface", /* 610 */
    "NtGdiEngBitBlt", /* 611 */
    "NtGdiEngStretchBlt", /* 612 */
    "NtGdiEngPlgBlt", /* 613 */
    "NtGdiEngMarkBandingSurface", /* 614 */
    "NtGdiEngStrokePath", /* 615 */
    "NtGdiEngFillPath", /* 616 */
    "NtGdiEngStrokeAndFillPath", /* 617 */
    "NtGdiEngPaint", /* 618 */
    "NtGdiEngLineTo", /* 619 */
    "NtGdiEngAlphaBlend", /* 620 */
    "NtGdiEngGradientFill", /* 621 */
    "NtGdiEngTransparentBlt", /* 622 */
    "NtGdiEngTextOut", /* 623 */
    "NtGdiEngStretchBltROP", /* 624 */
    "NtGdiXLATEOBJ_cGetPalette", /* 625 */
    "NtGdiXLATEOBJ_iXlate", /* 626 */
    "NtGdiXLATEOBJ_hGetColorTransform", /* 627 */
    "NtGdiCLIPOBJ_bEnum", /* 628 */
    "NtGdiCLIPOBJ_cEnumStart", /* 629 */
    "NtGdiCLIPOBJ_ppoGetPath", /* 630 */
    "NtGdiEngDeletePath", /* 631 */
    "NtGdiEngCreateClip", /* 632 */
    "NtGdiEngDeleteClip", /* 633 */
    "NtGdiBRUSHOBJ_ulGetBrushColor", /* 634 */
    "NtGdiBRUSHOBJ_pvAllocRbrush", /* 635 */
    "NtGdiBRUSHOBJ_pvGetRbrush", /* 636 */
    "NtGdiBRUSHOBJ_hGetColorTransform", /* 637 */
    "NtGdiXFORMOBJ_bApplyXform", /* 638 */
    "NtGdiXFORMOBJ_iGetXform", /* 639 */
    "NtGdiFONTOBJ_vGetInfo", /* 640 */
    "NtGdiFONTOBJ_pxoGetXform", /* 641 */
    "NtGdiFONTOBJ_cGetGlyphs", /* 642 */
    "NtGdiFONTOBJ_pifi", /* 643 */
    "NtGdiFONTOBJ_pfdg", /* 644 */
    "NtGdiFONTOBJ_pQueryGlyphAttrs", /* 645 */
    "NtGdiFONTOBJ_pvTrueTypeFontFile", /* 646 */
    "NtGdiFONTOBJ_cGetAllGlyphHandles", /* 647 */
    "NtGdiSTROBJ_bEnum", /* 648 */
    "NtGdiSTROBJ_bEnumPositionsOnly", /* 649 */
    "NtGdiSTROBJ_bGetAdvanceWidths", /* 650 */
    "NtGdiSTROBJ_vEnumStart", /* 651 */
    "NtGdiSTROBJ_dwGetCodePage ", /* 652 */
    "NtGdiPATHOBJ_vGetBounds", /* 653 */
    "NtGdiPATHOBJ_bEnum", /* 654 */
    "NtGdiPATHOBJ_vEnumStart", /* 655 */
    "NtGdiPATHOBJ_vEnumStartClipLines", /* 656 */
    "NtGdiPATHOBJ_bEnumClipLines", /* 657 */
    "NtGdiGetDhpdev", /* 658 */
    "NtGdiEngCheckAbort", /* 659 */
    "NtGdiHT_Get8BPPFormatPalette", /* 660 */
    "NtGdiHT_Get8BPPMaskPalette", /* 661 */
    "NtGdiUpdateTransform", /* 662 */
    "NtGdiSetPUMPDOBJ", /* 663 */
    "NtGdiBRUSHOBJ_DeleteRbrush", /* 664 */
    "NtGdiUnmapMemFont", /* 665 */
    "NtGdiDrawStream", /* 666 */
};


/* Windows system call related MACROs */
#define CREATE_SUSPENDED                  0x00000004

/* PspCidTable in Windows XP, to index EPROCESS by pid */
static const char XPPspCidTable[4] = { '\0', '\0', '\0', '\0' };