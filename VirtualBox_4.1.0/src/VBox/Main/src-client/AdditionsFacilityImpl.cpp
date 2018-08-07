/** @file
 *
 * VirtualBox COM class implementation
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

#include "AdditionsFacilityImpl.h"
#include "Global.h"

#include "AutoCaller.h"
#include "Logging.h"

/* static */
const AdditionsFacility::FacilityInfo AdditionsFacility::sFacilityInfo[8] =
{
    /* NOTE: We assume that unknown is always the first entry! */
    { "Unknown", AdditionsFacilityType_None, AdditionsFacilityClass_None },
    { "VirtualBox Base Driver", AdditionsFacilityType_VBoxGuestDriver, AdditionsFacilityClass_Driver },
    { "VirtualBox System Service", AdditionsFacilityType_VBoxService, AdditionsFacilityClass_Service },
    { "VirtualBox Desktop Integration", AdditionsFacilityType_VBoxTrayClient, AdditionsFacilityClass_Program },
    { "Seamless Mode", AdditionsFacilityType_Seamless, AdditionsFacilityClass_Feature },
    { "Graphics Mode", AdditionsFacilityType_Graphics, AdditionsFacilityClass_Feature },
};

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(AdditionsFacility)

HRESULT AdditionsFacility::FinalConstruct()
{
    LogFlowThisFunc(("\n"));
    return BaseFinalConstruct();
}

void AdditionsFacility::FinalRelease()
{
    LogFlowThisFuncEnter();
    uninit();
    BaseFinalRelease();
    LogFlowThisFuncLeave();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

HRESULT AdditionsFacility::init(Guest *aParent, AdditionsFacilityType_T enmFacility, AdditionsFacilityStatus_T enmStatus)
{
    LogFlowThisFunc(("aParent=%p\n", aParent));

    /* Enclose the state transition NotReady->InInit->Ready. */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    RTTimeNow(&mData.mLastUpdated);
    mData.mStatus = enmStatus;
    mData.mType = enmFacility;
    /** @todo mClass is not initialized here. */

    /* Confirm a successful initialization when it's the case. */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/**
 * Uninitializes the instance.
 * Called from FinalRelease().
 */
void AdditionsFacility::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady. */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
}

STDMETHODIMP AdditionsFacility::COMGETTER(ClassType)(AdditionsFacilityClass_T *aClass)
{
    LogFlowThisFuncEnter();

    CheckComArgOutPointerValid(aClass);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aClass = getClass();

    return S_OK;
}

STDMETHODIMP AdditionsFacility::COMGETTER(Name)(BSTR *aName)
{
    LogFlowThisFuncEnter();

    CheckComArgOutPointerValid(aName);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    Bstr(getName()).cloneTo(aName);

    return S_OK;
}

STDMETHODIMP AdditionsFacility::COMGETTER(LastUpdated)(LONG64 *aTimestamp)
{
    LogFlowThisFuncEnter();

    CheckComArgOutPointerValid(aTimestamp);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aTimestamp = getLastUpdated();

    return S_OK;
}

STDMETHODIMP AdditionsFacility::COMGETTER(Status)(AdditionsFacilityStatus_T *aStatus)
{
    LogFlowThisFuncEnter();

    CheckComArgOutPointerValid(aStatus);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aStatus = getStatus();

    return S_OK;
}

STDMETHODIMP AdditionsFacility::COMGETTER(Type)(AdditionsFacilityType_T *aType)
{
    LogFlowThisFuncEnter();

    CheckComArgOutPointerValid(aType);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aType = getType();

    return S_OK;
}

const AdditionsFacility::FacilityInfo &AdditionsFacility::typeToInfo(AdditionsFacilityType_T aType)
{
    for (size_t i = 0; i < RT_ELEMENTS(sFacilityInfo); ++i)
    {
        if (sFacilityInfo[i].mType == aType)
            return sFacilityInfo[i];
    }
    return sFacilityInfo[0]; /* Return unknown type. */
}

AdditionsFacilityClass_T AdditionsFacility::getClass() const
{
    return AdditionsFacility::typeToInfo(mData.mType).mClass;
}

Bstr AdditionsFacility::getName() const
{
    return AdditionsFacility::typeToInfo(mData.mType).mName;
}

LONG64 AdditionsFacility::getLastUpdated() const
{
    return RTTimeSpecGetMilli(&mData.mLastUpdated);
}

AdditionsFacilityStatus_T AdditionsFacility::getStatus() const
{
    return mData.mStatus;
}

AdditionsFacilityType_T AdditionsFacility::getType() const
{
    return mData.mType;
}

HRESULT AdditionsFacility::update(AdditionsFacilityStatus_T aStatus, RTTIMESPEC aTimestamp)
{
    mData.mStatus = aStatus;
    mData.mLastUpdated = aTimestamp;

    return S_OK;
}

