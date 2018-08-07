/* $Id: tstGuestPropSvc.cpp 36412 2011-03-24 17:25:33Z vboxsync $ */
/** @file
 *
 * Testcase for the guest property service.
 */

/*
 * Copyright (C) 2008 Oracle Corporation
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
#include <VBox/HostServices/GuestPropertySvc.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#include <iprt/test.h>

/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
static RTTEST g_hTest = NIL_RTTEST;

using namespace guestProp;

extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad (VBOXHGCMSVCFNTABLE *ptable);

/** Simple call handle structure for the guest call completion callback */
struct VBOXHGCMCALLHANDLE_TYPEDEF
{
    /** Where to store the result code */
    int32_t rc;
};

/** Call completion callback for guest calls. */
static void callComplete(VBOXHGCMCALLHANDLE callHandle, int32_t rc)
{
    callHandle->rc = rc;
}

/**
 * Initialise the HGCM service table as much as we need to start the
 * service
 * @param  pTable the table to initialise
 */
void initTable(VBOXHGCMSVCFNTABLE *pTable, VBOXHGCMSVCHELPERS *pHelpers)
{
    pTable->cbSize = sizeof (VBOXHGCMSVCFNTABLE);
    pTable->u32Version = VBOX_HGCM_SVC_VERSION;
    pHelpers->pfnCallComplete = callComplete;
    pTable->pHelpers = pHelpers;
}

/**
 * A list of valid flag strings for testConvertFlags.  The flag conversion
 * functions should accept these and convert them from string to a flag type
 * and back without errors.
 */
struct flagStrings
{
    /** Flag string in a format the functions should recognise */
    const char *pcszIn;
    /** How the functions should output the string again */
    const char *pcszOut;
}
g_validFlagStrings[] =
{
    /* pcszIn,                                          pcszOut */
    { "  ",                                             "" },
    { "transient, ",                                    "TRANSIENT" },
    { "  rdOnLyHOST, transIENT  ,     READONLY    ",    "TRANSIENT, READONLY" },
    { " rdonlyguest",                                   "RDONLYGUEST" },
    { "rdonlyhost     ",                                "RDONLYHOST" },
    { "transient, transreset, rdonlyhost",              "TRANSIENT, RDONLYHOST, TRANSRESET" },
    { "transient, transreset, rdonlyguest",             "TRANSIENT, RDONLYGUEST, TRANSRESET" },     /* max length */
    { "rdonlyguest, rdonlyhost",                        "READONLY" },
    { "transient,   transreset, ",                      "TRANSIENT, TRANSRESET" }, /* Don't combine them ... */
    { "transreset, ",                                   "TRANSIENT, TRANSRESET" }, /* ... instead expand transreset for old adds. */
};

/**
 * A list of invalid flag strings for testConvertFlags.  The flag conversion
 * functions should reject these.
 */
const char *g_invalidFlagStrings[] =
{
    "RDONLYHOST,,",
    "  TRANSIENT READONLY"
};

/**
 * Test the flag conversion functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testConvertFlags()
{
    int rc = VINF_SUCCESS;
    char *pszFlagBuffer = (char *)RTTestGuardedAllocTail(g_hTest, MAX_FLAGS_LEN);

    RTPrintf("tstGuestPropSvc: Testing conversion of valid flags strings.\n");
    for (unsigned i = 0; i < RT_ELEMENTS(g_validFlagStrings) && RT_SUCCESS(rc); ++i)
    {
        uint32_t fFlags;
        rc = validateFlags(g_validFlagStrings[i].pcszIn, &fFlags);
        if (RT_FAILURE(rc))
            RTPrintf("tstGuestPropSvc: FAILURE - Failed to validate flag string '%s'.\n", g_validFlagStrings[i].pcszIn);
        if (RT_SUCCESS(rc))
        {
            rc = writeFlags(fFlags, pszFlagBuffer);
            if (RT_FAILURE(rc))
                RTPrintf("tstGuestPropSvc: FAILURE - Failed to convert flag string '%s' back to a string.\n",
                         g_validFlagStrings[i].pcszIn);
        }
        if (RT_SUCCESS(rc) && (strlen(pszFlagBuffer) > MAX_FLAGS_LEN - 1))
        {
            RTPrintf("tstGuestPropSvc: FAILURE - String '%s' converts back to a flag string which is too long.\n",
                     g_validFlagStrings[i].pcszIn);
            rc = VERR_TOO_MUCH_DATA;
        }
        if (RT_SUCCESS(rc) && (strcmp(pszFlagBuffer, g_validFlagStrings[i].pcszOut) != 0))
        {
            RTPrintf("tstGuestPropSvc: FAILURE - String '%s' converts back to '%s' instead of to '%s'\n",
                     g_validFlagStrings[i].pcszIn, pszFlagBuffer,
                     g_validFlagStrings[i].pcszOut);
            rc = VERR_PARSE_ERROR;
        }
    }
    if (RT_SUCCESS(rc))
    {
        RTPrintf("Testing rejection of invalid flags strings.\n");
        for (unsigned i = 0; i < RT_ELEMENTS(g_invalidFlagStrings) && RT_SUCCESS(rc); ++i)
        {
            uint32_t fFlags;
            /* This is required to fail. */
            if (RT_SUCCESS(validateFlags(g_invalidFlagStrings[i], &fFlags)))
            {
                RTPrintf("String '%s' was incorrectly accepted as a valid flag string.\n",
                         g_invalidFlagStrings[i]);
                rc = VERR_PARSE_ERROR;
            }
        }
    }
    if (RT_SUCCESS(rc))
    {
        uint32_t u32BadFlags = ALLFLAGS << 1;
        RTPrintf("Testing rejection of an invalid flags field.\n");
        /* This is required to fail. */
        if (RT_SUCCESS(writeFlags(u32BadFlags, pszFlagBuffer)))
        {
            RTPrintf("Flags 0x%x were incorrectly written out as '%.*s'\n",
                     u32BadFlags, MAX_FLAGS_LEN, pszFlagBuffer);
            rc = VERR_PARSE_ERROR;
        }
    }

    RTTestGuardedFree(g_hTest, pszFlagBuffer);
    return rc;
}

/**
 * List of property names for testSetPropsHost.
 */
const char *apcszNameBlock[] =
{
    "test/name/",
    "test name",
    "TEST NAME",
    "/test/name",
    NULL
};

/**
 * List of property values for testSetPropsHost.
 */
const char *apcszValueBlock[] =
{
    "test/value/",
    "test value",
    "TEST VALUE",
    "/test/value",
    NULL
};

/**
 * List of property timestamps for testSetPropsHost.
 */
uint64_t au64TimestampBlock[] =
{
    0, 999, 999999, UINT64_C(999999999999), 0
};

/**
 * List of property flags for testSetPropsHost.
 */
const char *apcszFlagsBlock[] =
{
    "",
    "readonly, transient",
    "RDONLYHOST",
    "RdOnlyGuest",
    NULL
};

/**
 * Test the SET_PROPS_HOST function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testSetPropsHost(VBOXHGCMSVCFNTABLE *ptable)
{
    int rc = VINF_SUCCESS;
    RTPrintf("Testing the SET_PROPS_HOST call.\n");
    if (!VALID_PTR(ptable->pfnHostCall))
    {
        RTPrintf("Invalid pfnHostCall() pointer\n");
        rc = VERR_INVALID_POINTER;
    }
    if (RT_SUCCESS(rc))
    {
        VBOXHGCMSVCPARM paParms[4];
        paParms[0].setPointer ((void *) apcszNameBlock, 0);
        paParms[1].setPointer ((void *) apcszValueBlock, 0);
        paParms[2].setPointer ((void *) au64TimestampBlock, 0);
        paParms[3].setPointer ((void *) apcszFlagsBlock, 0);
        rc = ptable->pfnHostCall(ptable->pvService, SET_PROPS_HOST, 4,
                                 paParms);
        if (RT_FAILURE(rc))
            RTPrintf("SET_PROPS_HOST call failed with rc=%Rrc\n", rc);
    }
    return rc;
}

/** Result strings for zeroth enumeration test */
static const char *pcchEnumResult0[] =
{
    "test/name/\0test/value/\0""0\0",
    "test name\0test value\0""999\0TRANSIENT, READONLY",
    "TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST",
    "/test/name\0/test/value\0""999999999999\0RDONLYGUEST",
    NULL
};

/** Result string sizes for zeroth enumeration test */
static const uint32_t cchEnumResult0[] =
{
    sizeof("test/name/\0test/value/\0""0\0"),
    sizeof("test name\0test value\0""999\0TRANSIENT, READONLY"),
    sizeof("TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST"),
    sizeof("/test/name\0/test/value\0""999999999999\0RDONLYGUEST"),
    0
};

/**
 * The size of the buffer returned by the zeroth enumeration test -
 * the - 1 at the end is because of the hidden zero terminator
 */
static const uint32_t cchEnumBuffer0 =
sizeof("test/name/\0test/value/\0""0\0\0"
"test name\0test value\0""999\0TRANSIENT, READONLY\0"
"TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST\0"
"/test/name\0/test/value\0""999999999999\0RDONLYGUEST\0\0\0\0\0") - 1;

/** Result strings for first and second enumeration test */
static const char *pcchEnumResult1[] =
{
    "TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST",
    "/test/name\0/test/value\0""999999999999\0RDONLYGUEST",
    NULL
};

/** Result string sizes for first and second enumeration test */
static const uint32_t cchEnumResult1[] =
{
    sizeof("TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST"),
    sizeof("/test/name\0/test/value\0""999999999999\0RDONLYGUEST"),
    0
};

/**
 * The size of the buffer returned by the first enumeration test -
 * the - 1 at the end is because of the hidden zero terminator
 */
static const uint32_t cchEnumBuffer1 =
sizeof("TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST\0"
"/test/name\0/test/value\0""999999999999\0RDONLYGUEST\0\0\0\0\0") - 1;

static const struct enumStringStruct
{
    /** The enumeration pattern to test */
    const char *pcszPatterns;
    /** The size of the pattern string */
    const uint32_t cchPatterns;
    /** The expected enumeration output strings */
    const char **ppcchResult;
    /** The size of the output strings */
    const uint32_t *pcchResult;
    /** The size of the buffer needed for the enumeration */
    const uint32_t cchBuffer;
}
enumStrings[] =
{
    {
        "", sizeof(""),
        pcchEnumResult0,
        cchEnumResult0,
        cchEnumBuffer0
    },
    {
        "/*\0?E*", sizeof("/*\0?E*"),
        pcchEnumResult1,
        cchEnumResult1,
        cchEnumBuffer1
    },
    {
        "/*|?E*", sizeof("/*|?E*"),
        pcchEnumResult1,
        cchEnumResult1,
        cchEnumBuffer1
    }
};

/**
 * Test the ENUM_PROPS_HOST function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testEnumPropsHost(VBOXHGCMSVCFNTABLE *ptable)
{
    int rc = VINF_SUCCESS;
    RTPrintf("Testing the ENUM_PROPS_HOST call.\n");
    if (!VALID_PTR(ptable->pfnHostCall))
    {
        RTPrintf("Invalid pfnHostCall() pointer\n");
        rc = VERR_INVALID_POINTER;
    }
    for (unsigned i = 0; RT_SUCCESS(rc) && i < RT_ELEMENTS(enumStrings);
         ++i)
    {
        char buffer[2048];
        VBOXHGCMSVCPARM paParms[3];
        paParms[0].setPointer ((void *) enumStrings[i].pcszPatterns,
                               enumStrings[i].cchPatterns);
        paParms[1].setPointer ((void *) buffer,
                               enumStrings[i].cchBuffer - 1);
        AssertBreakStmt(sizeof(buffer) > enumStrings[i].cchBuffer,
                        rc = VERR_INTERNAL_ERROR);
        if (RT_SUCCESS(rc))
        {
            /* This should fail as the buffer is too small. */
            int rc2 = ptable->pfnHostCall(ptable->pvService, ENUM_PROPS_HOST,
                                          3, paParms);
            if (rc2 != VERR_BUFFER_OVERFLOW)
            {
                RTPrintf("ENUM_PROPS_HOST returned %Rrc instead of VERR_BUFFER_OVERFLOW on too small buffer, pattern number %d\n", rc2, i);
                rc = VERR_BUFFER_OVERFLOW;
            }
            else
            {
                uint32_t cchBufferActual;
                rc = paParms[2].getUInt32 (&cchBufferActual);
                if (RT_SUCCESS(rc) && cchBufferActual != enumStrings[i].cchBuffer)
                {
                    RTPrintf("ENUM_PROPS_HOST requested a buffer size of %lu instead of %lu for pattern number %d\n", cchBufferActual, enumStrings[i].cchBuffer, i);
                    rc = VERR_OUT_OF_RANGE;
                }
                else if (RT_FAILURE(rc))
                    RTPrintf("ENUM_PROPS_HOST did not return the required buffer size properly for pattern %d\n", i);
            }
        }
        if (RT_SUCCESS(rc))
        {
            paParms[1].setPointer ((void *) buffer, enumStrings[i].cchBuffer);
            rc = ptable->pfnHostCall(ptable->pvService, ENUM_PROPS_HOST,
                                      3, paParms);
            if (RT_FAILURE(rc))
                RTPrintf("ENUM_PROPS_HOST call failed for pattern %d with rc=%Rrc\n", i, rc);
            else
                /* Look for each of the result strings in the buffer which was returned */
                for (unsigned j = 0; RT_SUCCESS(rc) && enumStrings[i].ppcchResult[j] != NULL;
                     ++j)
                {
                    bool found = false;
                    for (unsigned k = 0; !found && k <   enumStrings[i].cchBuffer
                                                       - enumStrings[i].pcchResult[j];
                         ++k)
                        if (memcmp(buffer + k, enumStrings[i].ppcchResult[j],
                            enumStrings[i].pcchResult[j]) == 0)
                            found = true;
                    if (!found)
                    {
                        RTPrintf("ENUM_PROPS_HOST did not produce the expected output for pattern %d\n",
                                 i);
                        rc = VERR_UNRESOLVED_ERROR;
                    }
                }
        }
    }
    return rc;
}

/**
 * Set a property by calling the service
 * @returns the status returned by the call to the service
 *
 * @param   pTable      the service instance handle
 * @param   pcszName    the name of the property to set
 * @param   pcszValue   the value to set the property to
 * @param   pcszFlags   the flag string to set if one of the SET_PROP[_HOST]
 *                      commands is used
 * @param   isHost      whether the SET_PROP[_VALUE]_HOST commands should be
 *                      used, rather than the guest ones
 * @param   useSetProp  whether SET_PROP[_HOST] should be used rather than
 *                      SET_PROP_VALUE[_HOST]
 */
int doSetProperty(VBOXHGCMSVCFNTABLE *pTable, const char *pcszName,
                  const char *pcszValue, const char *pcszFlags, bool isHost,
                  bool useSetProp)
{
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };
    int command = SET_PROP_VALUE;
    if (isHost)
    {
        if (useSetProp)
            command = SET_PROP_HOST;
        else
            command = SET_PROP_VALUE_HOST;
    }
    else if (useSetProp)
        command = SET_PROP;
    VBOXHGCMSVCPARM paParms[3];
    /* Work around silly constant issues - we ought to allow passing
     * constant strings in the hgcm parameters. */
    char szName[MAX_NAME_LEN];
    char szValue[MAX_VALUE_LEN];
    char szFlags[MAX_FLAGS_LEN];
    RTStrPrintf(szName, sizeof(szName), "%s", pcszName);
    RTStrPrintf(szValue, sizeof(szValue), "%s", pcszValue);
    RTStrPrintf(szFlags, sizeof(szFlags), "%s", pcszFlags);
    paParms[0].setPointer (szName, (uint32_t)strlen(szName) + 1);
    paParms[1].setPointer (szValue, (uint32_t)strlen(szValue) + 1);
    paParms[2].setPointer (szFlags, (uint32_t)strlen(szFlags) + 1);
    if (isHost)
        callHandle.rc = pTable->pfnHostCall(pTable->pvService, command,
                                            useSetProp ? 3 : 2, paParms);
    else
        pTable->pfnCall(pTable->pvService, &callHandle, 0, NULL, command,
                        useSetProp ? 3 : 2, paParms);
    return callHandle.rc;
}


/** Array of properties for testing SET_PROP_HOST and _GUEST. */
static const struct
{
    /** Property name */
    const char *pcszName;
    /** Property value */
    const char *pcszValue;
    /** Property flags */
    const char *pcszFlags;
    /** Should this be set as the host or the guest? */
    bool isHost;
    /** Should we use SET_PROP or SET_PROP_VALUE? */
    bool useSetProp;
    /** Should this succeed or be rejected with VERR_PERMISSION_DENIED? */
    bool isAllowed;
}
setProperties[] =
{
    { "Red", "Stop!", "transient", false, true, true },
    { "Amber", "Caution!", "", false, false, true },
    { "Green", "Go!", "readonly", true, true, true },
    { "Blue", "What on earth...?", "", true, false, true },
    { "/test/name", "test", "", false, true, false },
    { "TEST NAME", "test", "", true, true, false },
    { "Green", "gone out...", "", false, false, false },
    { "Green", "gone out...", "", true, false, false },
    { NULL, NULL, NULL, false, false, false }
};

/**
 * Test the SET_PROP, SET_PROP_VALUE, SET_PROP_HOST and SET_PROP_VALUE_HOST
 * functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testSetProp(VBOXHGCMSVCFNTABLE *pTable)
{
    int rc = VINF_SUCCESS;
    RTPrintf("Testing the SET_PROP, SET_PROP_VALUE, SET_PROP_HOST and SET_PROP_VALUE_HOST calls.\n");
    for (unsigned i = 0; RT_SUCCESS(rc) && (setProperties[i].pcszName != NULL);
         ++i)
    {
        rc = doSetProperty(pTable, setProperties[i].pcszName,
                           setProperties[i].pcszValue,
                           setProperties[i].pcszFlags,
                           setProperties[i].isHost,
                           setProperties[i].useSetProp);
        if (setProperties[i].isAllowed && RT_FAILURE(rc))
            RTPrintf("Setting property '%s' failed with rc=%Rrc.\n",
                     setProperties[i].pcszName, rc);
        else if (   !setProperties[i].isAllowed
                 && (rc != VERR_PERMISSION_DENIED))
        {
            RTPrintf("Setting property '%s' returned %Rrc instead of VERR_PERMISSION_DENIED.\n",
                     setProperties[i].pcszName, rc);
            rc = VERR_IPE_UNEXPECTED_STATUS;
        }
        else
            rc = VINF_SUCCESS;
    }
    return rc;
}

/**
 * Delete a property by calling the service
 * @returns the status returned by the call to the service
 *
 * @param   pTable    the service instance handle
 * @param   pcszName  the name of the property to delete
 * @param   isHost    whether the DEL_PROP_HOST command should be used, rather
 *                    than the guest one
 */
int doDelProp(VBOXHGCMSVCFNTABLE *pTable, const char *pcszName, bool isHost)
{
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };
    int command = DEL_PROP;
    if (isHost)
        command = DEL_PROP_HOST;
    VBOXHGCMSVCPARM paParms[1];
    /* Work around silly constant issues - we ought to allow passing
     * constant strings in the hgcm parameters. */
    char szName[MAX_NAME_LEN];
    RTStrPrintf(szName, sizeof(szName), "%s", pcszName);
    paParms[0].setPointer (szName, (uint32_t)strlen(szName) + 1);
    if (isHost)
        callHandle.rc = pTable->pfnHostCall(pTable->pvService, command,
                                            1, paParms);
    else
        pTable->pfnCall(pTable->pvService, &callHandle, 0, NULL, command,
                        1, paParms);
    return callHandle.rc;
}

/** Array of properties for testing DEL_PROP_HOST and _GUEST. */
static const struct
{
    /** Property name */
    const char *pcszName;
    /** Should this be set as the host or the guest? */
    bool isHost;
    /** Should this succeed or be rejected with VERR_PERMISSION_DENIED? */
    bool isAllowed;
}
delProperties[] =
{
    { "Red", false, true },
    { "Amber", true, true },
    { "Red2", false, true },
    { "Amber2", true, true },
    { "Green", false, false },
    { "Green", true, false },
    { "/test/name", false, false },
    { "TEST NAME", true, false },
    { NULL, false, false }
};

/**
 * Test the DEL_PROP, and DEL_PROP_HOST functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testDelProp(VBOXHGCMSVCFNTABLE *pTable)
{
    int rc = VINF_SUCCESS;
    RTPrintf("Testing the DEL_PROP and DEL_PROP_HOST calls.\n");
    for (unsigned i = 0; RT_SUCCESS(rc) && (delProperties[i].pcszName != NULL);
         ++i)
    {
        rc = doDelProp(pTable, delProperties[i].pcszName,
                       delProperties[i].isHost);
        if (delProperties[i].isAllowed && RT_FAILURE(rc))
            RTPrintf("Deleting property '%s' failed with rc=%Rrc.\n",
                     delProperties[i].pcszName, rc);
        else if (   !delProperties[i].isAllowed
                 && (rc != VERR_PERMISSION_DENIED)
                )
        {
            RTPrintf("Deleting property '%s' returned %Rrc instead of VERR_PERMISSION_DENIED.\n",
                     delProperties[i].pcszName, rc);
            rc = VERR_IPE_UNEXPECTED_STATUS;
        }
        else
            rc = VINF_SUCCESS;
    }
    return rc;
}

/** Array of properties for testing GET_PROP_HOST. */
static const struct
{
    /** Property name */
    const char *pcszName;
    /** What value/flags pattern do we expect back? */
    const char *pcchValue;
    /** What size should the value/flags array be? */
    uint32_t cchValue;
    /** Should this property exist? */
    bool exists;
    /** Do we expect a particular timestamp? */
    bool hasTimestamp;
    /** What timestamp if any do ex expect? */
    uint64_t u64Timestamp;
}
getProperties[] =
{
    { "test/name/", "test/value/\0", sizeof("test/value/\0"), true, true, 0 },
    { "test name", "test value\0TRANSIENT, READONLY",
      sizeof("test value\0TRANSIENT, READONLY"), true, true, 999 },
    { "TEST NAME", "TEST VALUE\0RDONLYHOST", sizeof("TEST VALUE\0RDONLYHOST"),
      true, true, 999999 },
    { "/test/name", "/test/value\0RDONLYGUEST",
      sizeof("/test/value\0RDONLYGUEST"), true, true, UINT64_C(999999999999) },
    { "Green", "Go!\0READONLY", sizeof("Go!\0READONLY"), true, false, 0 },
    { "Blue", "What on earth...?\0", sizeof("What on earth...?\0"), true,
      false, 0 },
    { "Red", "", 0, false, false, 0 },
    { NULL, NULL, 0, false, false, 0 }
};

/**
 * Test the GET_PROP_HOST function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testGetProp(VBOXHGCMSVCFNTABLE *pTable)
{
    int rc = VINF_SUCCESS, rc2 = VINF_SUCCESS;
    RTPrintf("Testing the GET_PROP_HOST call.\n");
    for (unsigned i = 0; RT_SUCCESS(rc) && (getProperties[i].pcszName != NULL);
         ++i)
    {
        VBOXHGCMSVCPARM paParms[4];
        /* Work around silly constant issues - we ought to allow passing
         * constant strings in the hgcm parameters. */
        char szName[MAX_NAME_LEN] = "";
        char szBuffer[MAX_VALUE_LEN + MAX_FLAGS_LEN];
        AssertBreakStmt(sizeof(szBuffer) >= getProperties[i].cchValue,
                        rc = VERR_INTERNAL_ERROR);
        RTStrPrintf(szName, sizeof(szName), "%s", getProperties[i].pcszName);
        paParms[0].setPointer (szName, (uint32_t)strlen(szName) + 1);
        paParms[1].setPointer (szBuffer, sizeof(szBuffer));
        rc2 = pTable->pfnHostCall(pTable->pvService, GET_PROP_HOST, 4,
                                  paParms);
        if (getProperties[i].exists && RT_FAILURE(rc2))
        {
            RTPrintf("Getting property '%s' failed with rc=%Rrc.\n",
                     getProperties[i].pcszName, rc2);
            rc = rc2;
        }
        else if (!getProperties[i].exists && (rc2 != VERR_NOT_FOUND))
        {
            RTPrintf("Getting property '%s' returned %Rrc instead of VERR_NOT_FOUND.\n",
                     getProperties[i].pcszName, rc2);
            rc = VERR_IPE_UNEXPECTED_STATUS;
        }
        if (RT_SUCCESS(rc) && getProperties[i].exists)
        {
            uint32_t u32ValueLen;
            rc = paParms[3].getUInt32 (&u32ValueLen);
            if (RT_FAILURE(rc))
                RTPrintf("Failed to get the size of the output buffer for property '%s'\n",
                         getProperties[i].pcszName);
            if (   RT_SUCCESS(rc)
                && (memcmp(szBuffer, getProperties[i].pcchValue,
                           getProperties[i].cchValue) != 0)
               )
            {
                RTPrintf("Unexpected result '%.*s' for property '%s', expected '%.*s'.\n",
                         u32ValueLen, szBuffer, getProperties[i].pcszName,
                         getProperties[i].cchValue, getProperties[i].pcchValue);
                rc = VERR_UNRESOLVED_ERROR;
            }
            if (RT_SUCCESS(rc) && getProperties[i].hasTimestamp)
            {
                uint64_t u64Timestamp;
                rc = paParms[2].getUInt64 (&u64Timestamp);
                if (RT_FAILURE(rc))
                    RTPrintf("Failed to get the timestamp for property '%s'\n",
                             getProperties[i].pcszName);
                if (   RT_SUCCESS(rc)
                    && (u64Timestamp != getProperties[i].u64Timestamp)
                   )
                {
                    RTPrintf("Bad timestamp %llu for property '%s', expected %llu.\n",
                             u64Timestamp, getProperties[i].pcszName,
                             getProperties[i].u64Timestamp);
                    rc = VERR_UNRESOLVED_ERROR;
                }
            }
        }
    }
    return rc;
}

/** Array of properties for testing GET_PROP_HOST. */
static const struct
{
    /** Buffer returned */
    const char *pchBuffer;
    /** What size should the buffer be? */
    uint32_t cchBuffer;
}
getNotifications[] =
{
    { "Red\0Stop!\0TRANSIENT", sizeof("Red\0Stop!\0TRANSIENT") },
    { "Amber\0Caution!\0", sizeof("Amber\0Caution!\0") },
    { "Green\0Go!\0READONLY", sizeof("Green\0Go!\0READONLY") },
    { "Blue\0What on earth...?\0", sizeof("Blue\0What on earth...?\0") },
    { "Red\0\0", sizeof("Red\0\0") },
    { "Amber\0\0", sizeof("Amber\0\0") },
    { NULL, 0 }
};

/**
 * Test the GET_NOTIFICATION function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testGetNotification(VBOXHGCMSVCFNTABLE *pTable)
{
    int rc = VINF_SUCCESS;
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };
    char achBuffer[MAX_NAME_LEN + MAX_VALUE_LEN + MAX_FLAGS_LEN];
    static char szPattern[] = "";

    RTPrintf("Testing the GET_NOTIFICATION call.\n");
    uint64_t u64Timestamp;
    uint32_t u32Size = 0;
    VBOXHGCMSVCPARM paParms[4];

    /* Test "buffer too small" */
    u64Timestamp = 1;
    paParms[0].setPointer ((void *) szPattern, sizeof(szPattern));
    paParms[1].setUInt64 (u64Timestamp);
    paParms[2].setPointer ((void *) achBuffer, getNotifications[0].cchBuffer - 1);
    pTable->pfnCall(pTable->pvService, &callHandle, 0, NULL,
                    GET_NOTIFICATION, 4, paParms);
    if (   callHandle.rc != VERR_BUFFER_OVERFLOW
        || RT_FAILURE(paParms[3].getUInt32 (&u32Size))
        || u32Size != getNotifications[0].cchBuffer
       )
    {
        RTPrintf("Getting notification for property '%s' with a too small buffer did not fail correctly.\n",
                 getNotifications[0].pchBuffer);
        rc = VERR_UNRESOLVED_ERROR;
    }

    /* Test successful notification queries.  Start with an unknown timestamp
     * to get the oldest available notification. */
    u64Timestamp = 1;
    for (unsigned i = 0; RT_SUCCESS(rc) && (getNotifications[i].pchBuffer != NULL);
         ++i)
    {
        paParms[0].setPointer ((void *) szPattern, sizeof(szPattern));
        paParms[1].setUInt64 (u64Timestamp);
        paParms[2].setPointer ((void *) achBuffer, sizeof(achBuffer));
        pTable->pfnCall(pTable->pvService, &callHandle, 0, NULL,
                        GET_NOTIFICATION, 4, paParms);
        if (   RT_FAILURE(callHandle.rc)
            || (i == 0 && callHandle.rc != VWRN_NOT_FOUND)
            || RT_FAILURE(paParms[1].getUInt64 (&u64Timestamp))
            || RT_FAILURE(paParms[3].getUInt32 (&u32Size))
            || u32Size != getNotifications[i].cchBuffer
            || memcmp(achBuffer, getNotifications[i].pchBuffer, u32Size) != 0
           )
        {
            RTPrintf("Failed to get notification for property '%s' (rc=%Rrc).\n",
                     getNotifications[i].pchBuffer, rc);
            rc = VERR_UNRESOLVED_ERROR;
        }
    }
    return rc;
}

/** Parameters for the asynchronous guest notification call */
struct asyncNotification_
{
    /** Call parameters */
    VBOXHGCMSVCPARM aParms[4];
    /** Result buffer */
    char chBuffer[MAX_NAME_LEN + MAX_VALUE_LEN + MAX_FLAGS_LEN];
    /** Return value */
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle;
} asyncNotification;

/**
 * Set up the test for the asynchronous GET_NOTIFICATION function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int setupAsyncNotification(VBOXHGCMSVCFNTABLE *pTable)
{
    int rc = VINF_SUCCESS;

    RTPrintf("Testing the asynchronous GET_NOTIFICATION call with no notifications are available.\n");
    uint64_t u64Timestamp = 0;
    uint32_t u32Size = 0;
    static char szPattern[] = "";

    asyncNotification.aParms[0].setPointer ((void *) szPattern, sizeof(szPattern));
    asyncNotification.aParms[1].setUInt64 (u64Timestamp);
    asyncNotification.aParms[2].setPointer ((void *) asyncNotification.chBuffer,
                                            sizeof(asyncNotification.chBuffer));
    asyncNotification.callHandle.rc = VINF_HGCM_ASYNC_EXECUTE;
    pTable->pfnCall(pTable->pvService, &asyncNotification.callHandle, 0, NULL,
                    GET_NOTIFICATION, 4, asyncNotification.aParms);
    if (RT_FAILURE(asyncNotification.callHandle.rc))
    {
        RTPrintf("GET_NOTIFICATION call failed, rc=%Rrc.\n", asyncNotification.callHandle.rc);
        rc = VERR_UNRESOLVED_ERROR;
    }
    else if (asyncNotification.callHandle.rc != VINF_HGCM_ASYNC_EXECUTE)
    {
        RTPrintf("GET_NOTIFICATION call completed when no new notifications should be available.\n");
        rc = VERR_UNRESOLVED_ERROR;
    }
    return rc;
}

/**
 * Test the asynchronous GET_NOTIFICATION function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testAsyncNotification(VBOXHGCMSVCFNTABLE *pTable)
{
    int rc = VINF_SUCCESS;
    uint64_t u64Timestamp;
    uint32_t u32Size;
    if (   asyncNotification.callHandle.rc != VINF_SUCCESS
        || RT_FAILURE(asyncNotification.aParms[1].getUInt64 (&u64Timestamp))
        || RT_FAILURE(asyncNotification.aParms[3].getUInt32 (&u32Size))
        || u32Size != getNotifications[0].cchBuffer
        || memcmp(asyncNotification.chBuffer, getNotifications[0].pchBuffer, u32Size) != 0
       )
    {
        RTPrintf("Asynchronous GET_NOTIFICATION call did not complete as expected, rc=%Rrc\n",
                 asyncNotification.callHandle.rc);
        rc = VERR_UNRESOLVED_ERROR;
    }
    return rc;
}

/** Array of properties for testing SET_PROP_HOST and _GUEST with the
 * READONLYGUEST global flag set. */
static const struct
{
    /** Property name */
    const char *pcszName;
    /** Property value */
    const char *pcszValue;
    /** Property flags */
    const char *pcszFlags;
    /** Should this be set as the host or the guest? */
    bool isHost;
    /** Should we use SET_PROP or SET_PROP_VALUE? */
    bool useSetProp;
    /** Should this succeed or be rejected with VERR_ (NOT VINF_!)
     * PERMISSION_DENIED?  The global check is done after the property one. */
    bool isAllowed;
}
setPropertiesROGuest[] =
{
    { "Red", "Stop!", "transient", false, true, true },
    { "Amber", "Caution!", "", false, false, true },
    { "Green", "Go!", "readonly", true, true, true },
    { "Blue", "What on earth...?", "", true, false, true },
    { "/test/name", "test", "", false, true, true },
    { "TEST NAME", "test", "", true, true, true },
    { "Green", "gone out...", "", false, false, false },
    { "Green", "gone out....", "", true, false, false },
    { NULL, NULL, NULL, false, false, true }
};

/**
 * Set the global flags value by calling the service
 * @returns the status returned by the call to the service
 *
 * @param   pTable  the service instance handle
 * @param   eFlags  the flags to set
 */
int doSetGlobalFlags(VBOXHGCMSVCFNTABLE *pTable, ePropFlags eFlags)
{
    VBOXHGCMSVCPARM paParm;
    paParm.setUInt32(eFlags);
    int rc = pTable->pfnHostCall(pTable->pvService, SET_GLOBAL_FLAGS_HOST,
                                 1, &paParm);
    if (RT_FAILURE(rc))
    {
        char szFlags[MAX_FLAGS_LEN];
        if (RT_FAILURE(writeFlags(eFlags, szFlags)))
            RTPrintf("Failed to set the global flags.\n");
        else
            RTPrintf("Failed to set the global flags \"%s\".\n",
                     szFlags);
    }
    return rc;
}

/**
 * Test the SET_PROP, SET_PROP_VALUE, SET_PROP_HOST and SET_PROP_VALUE_HOST
 * functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testSetPropROGuest(VBOXHGCMSVCFNTABLE *pTable)
{
    int rc = VINF_SUCCESS;
    RTPrintf("Testing the SET_PROP, SET_PROP_VALUE, SET_PROP_HOST and SET_PROP_VALUE_HOST calls with READONLYGUEST set globally.\n");
    rc = VBoxHGCMSvcLoad(pTable);
    if (RT_FAILURE(rc))
        RTPrintf("Failed to start the HGCM service.\n");
    if (RT_SUCCESS(rc))
        rc = doSetGlobalFlags(pTable, RDONLYGUEST);
    for (unsigned i = 0; RT_SUCCESS(rc) && (setPropertiesROGuest[i].pcszName != NULL);
         ++i)
    {
        rc = doSetProperty(pTable, setPropertiesROGuest[i].pcszName,
                           setPropertiesROGuest[i].pcszValue,
                           setPropertiesROGuest[i].pcszFlags,
                           setPropertiesROGuest[i].isHost,
                           setPropertiesROGuest[i].useSetProp);
        if (setPropertiesROGuest[i].isAllowed && RT_FAILURE(rc))
            RTPrintf("Setting property '%s' to '%s' failed with rc=%Rrc.\n",
                     setPropertiesROGuest[i].pcszName,
                     setPropertiesROGuest[i].pcszValue, rc);
        else if (   !setPropertiesROGuest[i].isAllowed
                 && (rc != VERR_PERMISSION_DENIED))
        {
            RTPrintf("Setting property '%s' to '%s' returned %Rrc instead of VERR_PERMISSION_DENIED.\n",
                     setPropertiesROGuest[i].pcszName,
                     setPropertiesROGuest[i].pcszValue, rc);
            rc = VERR_IPE_UNEXPECTED_STATUS;
        }
        else if (   !setPropertiesROGuest[i].isHost
                 && setPropertiesROGuest[i].isAllowed
                 && (rc != VINF_PERMISSION_DENIED))
        {
            RTPrintf("Setting property '%s' to '%s' returned %Rrc instead of VINF_PERMISSION_DENIED.\n",
                     setPropertiesROGuest[i].pcszName,
                     setPropertiesROGuest[i].pcszValue, rc);
            rc = VERR_IPE_UNEXPECTED_STATUS;
        }
        else
            rc = VINF_SUCCESS;
    }
    if (RT_FAILURE(pTable->pfnUnload(pTable->pvService)))
        RTPrintf("Failed to unload the HGCM service.\n");
    return rc;
}

/** Array of properties for testing DEL_PROP_HOST and _GUEST with
 * READONLYGUEST set globally. */
static const struct
{
    /** Property name */
    const char *pcszName;
    /** Should this be deleted as the host (or the guest)? */
    bool isHost;
    /** Should this property be created first?  (As host, obviously) */
    bool shouldCreate;
    /** And with what flags? */
    const char *pcszFlags;
    /** Should this succeed or be rejected with VERR_ (NOT VINF_!)
     * PERMISSION_DENIED?  The global check is done after the property one. */
    bool isAllowed;
}
delPropertiesROGuest[] =
{
    { "Red", true, true, "", true },
    { "Amber", false, true, "", true },
    { "Red2", true, false, "", true },
    { "Amber2", false, false, "", true },
    { "Red3", true, true, "READONLY", false },
    { "Amber3", false, true, "READONLY", false },
    { "Red4", true, true, "RDONLYHOST", false },
    { "Amber4", false, true, "RDONLYHOST", true },
    { NULL, false, false, "", false }
};

/**
 * Test the DEL_PROP, and DEL_PROP_HOST functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
int testDelPropROGuest(VBOXHGCMSVCFNTABLE *pTable)
{
    int rc = VINF_SUCCESS;
    RTPrintf("Testing the DEL_PROP and DEL_PROP_HOST calls with RDONLYGUEST set globally.\n");
    rc = VBoxHGCMSvcLoad(pTable);
    if (RT_FAILURE(rc))
        RTPrintf("Failed to start the HGCM service.\n");
    if (RT_SUCCESS(rc))
        rc = doSetGlobalFlags(pTable, RDONLYGUEST);
    for (unsigned i = 0;    RT_SUCCESS(rc)
                         && (delPropertiesROGuest[i].pcszName != NULL); ++i)
    {
        if (RT_SUCCESS(rc) && delPropertiesROGuest[i].shouldCreate)
            rc = doSetProperty(pTable, delPropertiesROGuest[i].pcszName,
                               "none", delPropertiesROGuest[i].pcszFlags,
                               true, true);
        rc = doDelProp(pTable, delPropertiesROGuest[i].pcszName,
                       delPropertiesROGuest[i].isHost);
        if (delPropertiesROGuest[i].isAllowed && RT_FAILURE(rc))
            RTPrintf("Deleting property '%s' failed with rc=%Rrc.\n",
                     delPropertiesROGuest[i].pcszName, rc);
        else if (   !delPropertiesROGuest[i].isAllowed
                 && (rc != VERR_PERMISSION_DENIED)
                )
        {
            RTPrintf("Deleting property '%s' returned %Rrc instead of VERR_PERMISSION_DENIED.\n",
                     delPropertiesROGuest[i].pcszName, rc);
            rc = VERR_IPE_UNEXPECTED_STATUS;
        }
        else if (   !delPropertiesROGuest[i].isHost
                 && delPropertiesROGuest[i].shouldCreate
                 && delPropertiesROGuest[i].isAllowed
                 && (rc != VINF_PERMISSION_DENIED))
        {
            RTPrintf("Deleting property '%s' as guest returned %Rrc instead of VINF_PERMISSION_DENIED.\n",
                     delPropertiesROGuest[i].pcszName, rc);
            rc = VERR_IPE_UNEXPECTED_STATUS;
        }
        else
            rc = VINF_SUCCESS;
    }
    if (RT_FAILURE(pTable->pfnUnload(pTable->pvService)))
        RTPrintf("Failed to unload the HGCM service.\n");
    return rc;
}

int main(int argc, char **argv)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    RTEXITCODE          rcExit;

    rcExit  = RTTestInitAndCreate("tstGuestPropSvc", &g_hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(g_hTest);

/** @todo convert the rest of this testcase. */
    initTable(&svcTable, &svcHelpers);

    if (RT_FAILURE(testConvertFlags()))
        return 1;
    /* The function is inside the service, not HGCM. */
    if (RT_FAILURE(VBoxHGCMSvcLoad(&svcTable)))
    {
        RTPrintf("Failed to start the HGCM service.\n");
        return 1;
    }
    if (RT_FAILURE(testSetPropsHost(&svcTable)))
        return 1;
    if (RT_FAILURE(testEnumPropsHost(&svcTable)))
        return 1;
    /* Set up the asynchronous notification test */
    if (RT_FAILURE(setupAsyncNotification(&svcTable)))
        return 1;
    if (RT_FAILURE(testSetProp(&svcTable)))
        return 1;
    RTPrintf("Checking the data returned by the asynchronous notification call.\n");
    /* Our previous notification call should have completed by now. */
    if (RT_FAILURE(testAsyncNotification(&svcTable)))
        return 1;
    if (RT_FAILURE(testDelProp(&svcTable)))
        return 1;
    if (RT_FAILURE(testGetProp(&svcTable)))
        return 1;
    if (RT_FAILURE(testGetNotification(&svcTable)))
        return 1;
    if (RT_FAILURE(svcTable.pfnUnload(svcTable.pvService)))
    {
        RTPrintf("Failed to unload the HGCM service.\n");
        return 1;
    }
    if (RT_FAILURE(testSetPropROGuest(&svcTable)))
        return 1;
    if (RT_FAILURE(testDelPropROGuest(&svcTable)))
        return 1;

    return RTTestSummaryAndDestroy(g_hTest);
}
