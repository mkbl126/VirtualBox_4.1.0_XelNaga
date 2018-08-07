/* $Id: VBoxVideoWinDbg.cpp 36867 2011-04-28 07:27:03Z vboxsync $ */

/** @file
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

#include <windows.h>
#define KDEXT_64BIT
#include <wdbgexts.h>

#define VBOXVWD_VERSION_MAJOR 1
#define VBOXVWD_VERSION_MINOR 1

static EXT_API_VERSION g_VBoxVWDVersion = {
        VBOXVWD_VERSION_MAJOR,
        VBOXVWD_VERSION_MINOR,
        EXT_API_VERSION_NUMBER64,
        0
};

/**
 * DLL entry point.
 */
BOOL WINAPI DllMain(HINSTANCE hInstance,
                    DWORD     dwReason,
                    LPVOID    lpReserved)
{
    BOOL bOk = TRUE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            break;
        }

        default:
            break;
    }
    return bOk;
}

/* note: need to name it this way to make dprintf & other macros defined in wdbgexts.h work */
WINDBG_EXTENSION_APIS64 ExtensionApis = {0};
USHORT SavedMajorVersion;
USHORT SavedMinorVersion;

#ifdef __cplusplus
extern "C"
{
#endif
LPEXT_API_VERSION WDBGAPI ExtensionApiVersion();
VOID WDBGAPI CheckVersion();
VOID WDBGAPI WinDbgExtensionDllInit(PWINDBG_EXTENSION_APIS64 lpExtensionApis, USHORT MajorVersion, USHORT MinorVersion);
#ifdef __cplusplus
}
#endif

LPEXT_API_VERSION WDBGAPI ExtensionApiVersion()
{
    return &g_VBoxVWDVersion;
}

VOID WDBGAPI CheckVersion()
{
    return;
}

VOID WDBGAPI WinDbgExtensionDllInit(PWINDBG_EXTENSION_APIS64 lpExtensionApis, USHORT MajorVersion, USHORT MinorVersion)
{
    ExtensionApis = *lpExtensionApis;
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
}

DECLARE_API(help)
{
    dprintf("**** VirtulBox Video Driver debugging extension ****\n"
            " The following commands are supported: \n"
            " !ms - save memory (video data) to clipboard \n"
            "  usage: !ms <virtual memory address> <width> <height> [bitsPerPixel (default is 32)] [pitch (default is ((width * bpp + 7) >> 3) + 3) & ~3)]\n");
}

DECLARE_API(ms)
{
    ULONG64 u64Mem;
    ULONG64 u64Width;
    ULONG64 u64Height;
    ULONG64 u64Bpp = 32;
    ULONG64 u64NumColors = 3;
    ULONG64 u64Pitch;
    ULONG64 u64DefaultPitch;
    PCSTR pExpr = args;

    if (!pExpr) { dprintf("address not specified\n"); return; }
    if (!GetExpressionEx(pExpr, &u64Mem, &pExpr)) { dprintf("error evaluating address\n"); return; }
    if (!u64Mem) { dprintf("address value can not be NULL\n"); return; }

    if (!pExpr) { dprintf("width not specified\n"); return; }
    if (!GetExpressionEx(pExpr, &u64Width, &pExpr)) { dprintf("error evaluating width\n"); return; }
    if (!u64Width) { dprintf("width value can not be NULL\n"); return; }

    if (!pExpr) { dprintf("height not specified\n"); return; }
    if (!GetExpressionEx(pExpr, &u64Height, &pExpr)) { dprintf("error evaluating height\n"); return; }
    if (!u64Height) { dprintf("height value can not be NULL\n"); return; }

    if (pExpr && GetExpressionEx(pExpr, &u64NumColors, &pExpr))
    {
        if (!u64NumColors) { dprintf("Num Colors value can not be NULL\n"); return; }
    }

    if (pExpr && GetExpressionEx(pExpr, &u64Bpp, &pExpr))
    {
        if (!u64Bpp) { dprintf("bpp value can not be NULL\n"); return; }
    }

    u64DefaultPitch = (((((u64Width * u64Bpp) + 7) >> 3) + 3) & ~3ULL);
    if (pExpr && GetExpressionEx(pExpr, &u64Pitch, &pExpr))
    {
        if (u64Pitch < u64DefaultPitch) { dprintf("pitch value can not be less than (%I)\n", u64DefaultPitch); return; }
    }
    else
    {
        u64Pitch = u64DefaultPitch;
    }

    dprintf("processing data for address(0x%p), width(%d), height(%d), bpp(%d), pitch(%d)...\n",
                u64Mem, (UINT)u64Width, (UINT)u64Height, (UINT)u64Bpp, (UINT)u64Pitch);

    ULONG64 cbSize = u64DefaultPitch * u64Height;
    PVOID pvBuf = malloc(cbSize);
    if (pvBuf)
    {
        ULONG uRc = 0;
        if(u64DefaultPitch == u64Pitch)
        {
            ULONG cbRead = 0;
            dprintf("reading the entire memory buffer...\n");
            uRc = ReadMemory(u64Mem, pvBuf, cbSize, &cbRead);
            if (!uRc)
            {
                dprintf("Failed to read the memory buffer of size(%I)\n", cbSize);
            }
            else if (cbRead != cbSize)
            {
                dprintf("the actual number of bytes read(%I) no equal the requested size(%I)\n", cbRead, cbSize);
                uRc = 0;
            }

        }
        else
        {
            ULONG64 u64Offset = u64Mem;
            char* pvcBuf = (char*)pvBuf;
            ULONG64 i;
            dprintf("reading memory by chunks since custom pitch is specified...\n");
            for (i = 0; i < u64Height; ++i, u64Offset+=u64Pitch, pvcBuf+=u64DefaultPitch)
            {
                ULONG cbRead = 0;
                uRc = ReadMemory(u64Offset, pvcBuf, u64DefaultPitch, &cbRead);
                if (!uRc)
                {
                    dprintf("Failed to read the memory buffer of size(%I), chunk(%I)\n", u64DefaultPitch, i);
                    break;
                }
                else if (cbRead != u64DefaultPitch)
                {
                    dprintf("the actual number of bytes read(%I) no equal the requested size(%I), chunk(%I)\n", cbRead, u64DefaultPitch, i);
                    uRc = 0;
                    break;
                }
            }
        }

        if (uRc)
        {
            switch (u64Bpp)
            {
                case 32:
                case 24:
                case 16:
                    if (u64NumColors != 3)
                    {
                        dprintf("WARNING: unsupported number colors: (%d)\n", u64NumColors);
                    }
                    break;
                case 8:
                    {
                        if (u64NumColors == 1)
                        {
                            ULONG64 cbSize32 = u64DefaultPitch * 4 * u64Height;
                            PVOID pvBuf32 = malloc(cbSize32);
                            if (pvBuf32)
                            {
                                byte* pByteBuf32 = (byte*)pvBuf32;
                                byte* pByteBuf = (byte*)pvBuf;
                                memset(pvBuf32, 0, cbSize32);
                                for (UINT i = 0; i < u64Height; ++i)
                                {
                                    for (UINT j = 0; j < u64Width; ++j)
                                    {
                                        pByteBuf32[0] = pByteBuf[0];
                                        pByteBuf32[1] = pByteBuf[0];
                                        pByteBuf32[2] = pByteBuf[0];
                                        pByteBuf32 += 4;
                                        pByteBuf += 1;
                                    }
                                }
                                free(pvBuf);
                                pvBuf = pvBuf32;
                                u64DefaultPitch *= 4;
                                u64Bpp *= 4;
                            }
                        }
                        else
                        {
                            dprintf("WARNING: unsupported number colors: (%d)\n", u64NumColors);
                        }
                    }
                    break;
            }
            BITMAP Bmp = {0};
            HBITMAP hBmp;
            dprintf("read memory succeeded..\n");
            Bmp.bmType = 0;
            Bmp.bmWidth = (LONG)u64Width;
            Bmp.bmHeight = (LONG)u64Height;
            Bmp.bmWidthBytes = (LONG)u64DefaultPitch;
            Bmp.bmPlanes = 1;
            Bmp.bmBitsPixel = (WORD)u64Bpp;
            Bmp.bmBits = (LPVOID)pvBuf;
            hBmp = CreateBitmapIndirect(&Bmp);
            if (hBmp)
            {
                if (OpenClipboard(GetDesktopWindow()))
                {
                    if (EmptyClipboard())
                    {
                        if (SetClipboardData(CF_BITMAP, hBmp))
                        {
                            dprintf("succeeded!! You can now do <ctrl>+v in your favourite image editor\n");
                        }
                        else
                        {
                            DWORD winEr = GetLastError();
                            dprintf("SetClipboardData failed, err(%I)\n", winEr);
                        }
                    }
                    else
                    {
                        DWORD winEr = GetLastError();
                        dprintf("EmptyClipboard failed, err(%I)\n", winEr);
                    }

                    CloseClipboard();
                }
                else
                {
                    DWORD winEr = GetLastError();
                    dprintf("OpenClipboard failed, err(%I)\n", winEr);
                }

                DeleteObject(hBmp);
            }
            else
            {
                DWORD winEr = GetLastError();
                dprintf("CreateBitmapIndirect failed, err(%I)\n", winEr);
            }
        }
        else
        {
            dprintf("read memory failed\n");
        }
        free(pvBuf);
    }
    else
    {
        dprintf("failed to allocate memory buffer of size(%I)\n", cbSize);
    }
}
