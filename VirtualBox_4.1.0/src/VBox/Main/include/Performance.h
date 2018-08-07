/* $Id: Performance.h 36842 2011-04-26 07:54:09Z vboxsync $ */
/** @file
 * VirtualBox Main - Performance Classes declaration.
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
#ifndef ___performance_h
#define ___performance_h

#include <VBox/com/defs.h>
#include <VBox/com/ptr.h>
#include <VBox/com/string.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/types.h>
#include <iprt/err.h>

#include <algorithm>
#include <functional> /* For std::fun_ptr in testcase */
#include <list>
#include <vector>

/* Forward decl. */
class Machine;

namespace pm
{
    /* CPU load is measured in 1/1000 of per cent. */
    const uint64_t PM_CPU_LOAD_MULTIPLIER = UINT64_C(100000);

    /* Sub Metrics **********************************************************/
    class CircularBuffer
    {
    public:
        CircularBuffer() : mData(0), mLength(0), mEnd(0), mWrapped(false) {};
        ~CircularBuffer() { if (mData) RTMemFree(mData); };
        void init(ULONG length);
        ULONG length();
        ULONG getSequenceNumber() { return mSequenceNumber; }
        void put(ULONG value);
        void copyTo(ULONG *data);
    private:
        ULONG *mData;
        ULONG  mLength;
        ULONG  mEnd;
        ULONG  mSequenceNumber;
        bool   mWrapped;
    };

    class SubMetric : public CircularBuffer
    {
    public:
        SubMetric(const char *name, const char *description)
        : mName(name), mDescription(description) {};
        void query(ULONG *data);
        const char *getName() { return mName; };
        const char *getDescription() { return mDescription; };
    private:
        const char *mName;
        const char *mDescription;
    };


    enum {
        COLLECT_NONE        = 0x0,
        COLLECT_CPU_LOAD    = 0x1,
        COLLECT_RAM_USAGE   = 0x2,
        COLLECT_GUEST_STATS = 0x4
    };
    typedef int HintFlags;
    typedef std::pair<RTPROCESS, HintFlags> ProcessFlagsPair;

    class CollectorHints
    {
    public:
        typedef std::list<ProcessFlagsPair> ProcessList;

        CollectorHints() : mHostFlags(COLLECT_NONE) {}
        void collectHostCpuLoad()
            { mHostFlags |= COLLECT_CPU_LOAD; }
        void collectHostRamUsage()
            { mHostFlags |= COLLECT_RAM_USAGE; }
        void collectHostRamVmm()
            { mHostFlags |= COLLECT_GUEST_STATS; }
        void collectProcessCpuLoad(RTPROCESS process)
            { findProcess(process).second |= COLLECT_CPU_LOAD; }
        void collectProcessRamUsage(RTPROCESS process)
            { findProcess(process).second |= COLLECT_RAM_USAGE; }
        void collectGuestStats(RTPROCESS process)
            { findProcess(process).second |= COLLECT_GUEST_STATS; }
        bool isHostCpuLoadCollected() const
            { return (mHostFlags & COLLECT_CPU_LOAD) != 0; }
        bool isHostRamUsageCollected() const
            { return (mHostFlags & COLLECT_RAM_USAGE) != 0; }
        bool isHostRamVmmCollected() const
            { return (mHostFlags & COLLECT_GUEST_STATS) != 0; }
        bool isProcessCpuLoadCollected(RTPROCESS process)
            { return (findProcess(process).second & COLLECT_CPU_LOAD) != 0; }
        bool isProcessRamUsageCollected(RTPROCESS process)
            { return (findProcess(process).second & COLLECT_RAM_USAGE) != 0; }
        bool isGuestStatsCollected(RTPROCESS process)
            { return (findProcess(process).second & COLLECT_GUEST_STATS) != 0; }
        void getProcesses(std::vector<RTPROCESS>& processes) const
        {
            processes.clear();
            processes.reserve(mProcesses.size());
            for (ProcessList::const_iterator it = mProcesses.begin(); it != mProcesses.end(); it++)
                processes.push_back(it->first);
        }
        const ProcessList& getProcessFlags() const
        {
            return mProcesses;
        }
    private:
        HintFlags   mHostFlags;
        ProcessList mProcesses;

        ProcessFlagsPair& findProcess(RTPROCESS process)
        {
            ProcessList::iterator it;
            for (it = mProcesses.begin(); it != mProcesses.end(); it++)
                if (it->first == process)
                    return *it;

            /* Not found -- add new */
            mProcesses.push_back(ProcessFlagsPair(process, COLLECT_NONE));
            return mProcesses.back();
        }
    };

    /* Guest Collector Classes  *********************************/
    class CollectorGuest
    {
    public:
        CollectorGuest(Machine *machine, RTPROCESS process);
        ~CollectorGuest();

        bool isUnregistered()   { return mUnregistered; };
        bool isEnabled()        { return mEnabled; };
        bool isValid()          { return mValid; };
        void invalidateStats()  { mValid = false; };
        void unregister()       { mUnregistered = true; };
        int updateStats();
        int enable();
        int disable();

        RTPROCESS getProcess()  { return mProcess; };
        ULONG getCpuUser()      { return mCpuUser; };
        ULONG getCpuKernel()    { return mCpuKernel; };
        ULONG getCpuIdle()      { return mCpuIdle; };
        ULONG getMemTotal()     { return mMemTotal; };
        ULONG getMemFree()      { return mMemFree; };
        ULONG getMemBalloon()   { return mMemBalloon; };
        ULONG getMemShared()    { return mMemShared; };
        ULONG getMemCache()     { return mMemCache; };
        ULONG getPageTotal()    { return mPageTotal; };
        ULONG getAllocVMM()     { return mAllocVMM; };
        ULONG getFreeVMM()      { return mFreeVMM; };
        ULONG getBalloonedVMM() { return mBalloonedVMM; };
        ULONG getSharedVMM()    { return mSharedVMM; };

    private:
        bool                 mUnregistered;
        bool                 mEnabled;
        bool                 mValid;
        Machine             *mMachine;
        RTPROCESS            mProcess;
        ComPtr<IConsole>     mConsole;
        ComPtr<IGuest>       mGuest;
        ULONG                mCpuUser;
        ULONG                mCpuKernel;
        ULONG                mCpuIdle;
        ULONG                mMemTotal;
        ULONG                mMemFree;
        ULONG                mMemBalloon;
        ULONG                mMemShared;
        ULONG                mMemCache;
        ULONG                mPageTotal;
        ULONG                mAllocVMM;
        ULONG                mFreeVMM;
        ULONG                mBalloonedVMM;
        ULONG                mSharedVMM;
    };

    typedef std::list<CollectorGuest*> CollectorGuestList;
    class CollectorGuestManager
    {
    public:
        CollectorGuestManager() : mVMMStatsProvider(NULL) {};
        ~CollectorGuestManager() { Assert(mGuests.size() == 0); };
        void registerGuest(CollectorGuest* pGuest);
        void unregisterGuest(CollectorGuest* pGuest);
        CollectorGuest *getVMMStatsProvider() { return mVMMStatsProvider; };
        void preCollect(CollectorHints& hints, uint64_t iTick);
        void destroyUnregistered();
    private:
        CollectorGuestList mGuests;
        CollectorGuest    *mVMMStatsProvider;
    };

    /* Collector Hardware Abstraction Layer *********************************/
    class CollectorHAL
    {
    public:
                 CollectorHAL() {};
        virtual ~CollectorHAL() { };
        virtual int preCollect(const CollectorHints& /* hints */, uint64_t /* iTick */) { return VINF_SUCCESS; }
        /** Returns averaged CPU usage in 1/1000th per cent across all host's CPUs. */
        virtual int getHostCpuLoad(ULONG *user, ULONG *kernel, ULONG *idle);
        /** Returns the average frequency in MHz across all host's CPUs. */
        virtual int getHostCpuMHz(ULONG *mhz);
        /** Returns the amount of physical memory in kilobytes. */
        virtual int getHostMemoryUsage(ULONG *total, ULONG *used, ULONG *available);
        /** Returns CPU usage in 1/1000th per cent by a particular process. */
        virtual int getProcessCpuLoad(RTPROCESS process, ULONG *user, ULONG *kernel);
        /** Returns the amount of memory used by a process in kilobytes. */
        virtual int getProcessMemoryUsage(RTPROCESS process, ULONG *used);

        /** Returns CPU usage counters in platform-specific units. */
        virtual int getRawHostCpuLoad(uint64_t *user, uint64_t *kernel, uint64_t *idle);
        /** Returns process' CPU usage counter in platform-specific units. */
        virtual int getRawProcessCpuLoad(RTPROCESS process, uint64_t *user, uint64_t *kernel, uint64_t *total);
    };

    extern CollectorHAL *createHAL();

    /* Base Metrics *********************************************************/
    class BaseMetric
    {
    public:
        BaseMetric(CollectorHAL *hal, const char *name, ComPtr<IUnknown> object)
            : mPeriod(0), mLength(0), mHAL(hal), mName(name), mObject(object),
              mLastSampleTaken(0), mEnabled(false), mUnregistered(false) {};
        virtual ~BaseMetric() {};

        virtual void init(ULONG period, ULONG length) = 0;
        virtual void preCollect(CollectorHints& hints, uint64_t iTick) = 0;
        virtual void collect() = 0;
        virtual const char *getUnit() = 0;
        virtual ULONG getMinValue() = 0;
        virtual ULONG getMaxValue() = 0;
        virtual ULONG getScale() = 0;

        bool collectorBeat(uint64_t nowAt);

        void enable()  { mEnabled = true; };
        void disable() { mEnabled = false; };
        void unregister() { mUnregistered = true; };

        bool isUnregistered() { return mUnregistered; };
        bool isEnabled() { return mEnabled; };
        ULONG getPeriod() { return mPeriod; };
        ULONG getLength() { return mLength; };
        const char *getName() { return mName; };
        ComPtr<IUnknown> getObject() { return mObject; };
        bool associatedWith(ComPtr<IUnknown> object) { return mObject == object; };

    protected:
        ULONG           mPeriod;
        ULONG           mLength;
        CollectorHAL    *mHAL;
        const char      *mName;
        ComPtr<IUnknown> mObject;
        uint64_t         mLastSampleTaken;
        bool             mEnabled;
        bool             mUnregistered;
    };

    class BaseGuestMetric : public BaseMetric
    {
    public:
        BaseGuestMetric(CollectorGuest *cguest, const char *name, ComPtr<IUnknown> object)
            : BaseMetric(NULL, name, object), mCGuest(cguest) {};
    protected:
        CollectorGuest *mCGuest;
    };

    class HostCpuLoad : public BaseMetric
    {
    public:
        HostCpuLoad(CollectorHAL *hal, ComPtr<IUnknown> object, SubMetric *user, SubMetric *kernel, SubMetric *idle)
        : BaseMetric(hal, "CPU/Load", object), mUser(user), mKernel(kernel), mIdle(idle) {};
        ~HostCpuLoad() { delete mUser; delete mKernel; delete mIdle; };

        void init(ULONG period, ULONG length);

        void collect();
        const char *getUnit() { return "%"; };
        ULONG getMinValue() { return 0; };
        ULONG getMaxValue() { return PM_CPU_LOAD_MULTIPLIER; };
        ULONG getScale() { return PM_CPU_LOAD_MULTIPLIER / 100; }

    protected:
        SubMetric *mUser;
        SubMetric *mKernel;
        SubMetric *mIdle;
    };

    class HostCpuLoadRaw : public HostCpuLoad
    {
    public:
        HostCpuLoadRaw(CollectorHAL *hal, ComPtr<IUnknown> object, SubMetric *user, SubMetric *kernel, SubMetric *idle)
        : HostCpuLoad(hal, object, user, kernel, idle), mUserPrev(0), mKernelPrev(0), mIdlePrev(0) {};

        void preCollect(CollectorHints& hints, uint64_t iTick);
        void collect();
    private:
        uint64_t mUserPrev;
        uint64_t mKernelPrev;
        uint64_t mIdlePrev;
    };

    class HostCpuMhz : public BaseMetric
    {
    public:
        HostCpuMhz(CollectorHAL *hal, ComPtr<IUnknown> object, SubMetric *mhz)
        : BaseMetric(hal, "CPU/MHz", object), mMHz(mhz) {};
        ~HostCpuMhz() { delete mMHz; };

        void init(ULONG period, ULONG length);
        void preCollect(CollectorHints& /* hints */, uint64_t /* iTick */) {}
        void collect();
        const char *getUnit() { return "MHz"; };
        ULONG getMinValue() { return 0; };
        ULONG getMaxValue() { return INT32_MAX; };
        ULONG getScale() { return 1; }
    private:
        SubMetric *mMHz;
    };

    class HostRamUsage : public BaseMetric
    {
    public:
        HostRamUsage(CollectorHAL *hal, ComPtr<IUnknown> object, SubMetric *total, SubMetric *used, SubMetric *available)
        : BaseMetric(hal, "RAM/Usage", object), mTotal(total), mUsed(used), mAvailable(available) {};
        ~HostRamUsage() { delete mTotal; delete mUsed; delete mAvailable; };

        void init(ULONG period, ULONG length);
        void preCollect(CollectorHints& hints, uint64_t iTick);
        void collect();
        const char *getUnit() { return "kB"; };
        ULONG getMinValue() { return 0; };
        ULONG getMaxValue() { return INT32_MAX; };
        ULONG getScale() { return 1; }
    private:
        SubMetric *mTotal;
        SubMetric *mUsed;
        SubMetric *mAvailable;
    };

    class HostRamVmm : public BaseMetric
    {
    public:
        HostRamVmm(CollectorGuestManager *gm, ComPtr<IUnknown> object, SubMetric *allocVMM, SubMetric *freeVMM, SubMetric *balloonVMM, SubMetric *sharedVMM)
            : BaseMetric(NULL, "RAM/VMM", object), mCollectorGuestManager(gm),
            mAllocVMM(allocVMM), mFreeVMM(freeVMM), mBalloonVMM(balloonVMM), mSharedVMM(sharedVMM),
            mAllocCurrent(0), mFreeCurrent(0), mBalloonedCurrent(0), mSharedCurrent(0) {};
        ~HostRamVmm() { delete mAllocVMM; delete mFreeVMM; delete mBalloonVMM; delete mSharedVMM; };

        void init(ULONG period, ULONG length);
        void preCollect(CollectorHints& hints, uint64_t iTick);
        void collect();
        const char *getUnit() { return "kB"; };
        ULONG getMinValue() { return 0; };
        ULONG getMaxValue() { return INT32_MAX; };
        ULONG getScale() { return 1; }

    private:
        CollectorGuestManager *mCollectorGuestManager;
        SubMetric             *mAllocVMM;
        SubMetric             *mFreeVMM;
        SubMetric             *mBalloonVMM;
        SubMetric             *mSharedVMM;
        ULONG                  mAllocCurrent;
        ULONG                  mFreeCurrent;
        ULONG                  mBalloonedCurrent;
        ULONG                  mSharedCurrent;
    };

    class MachineCpuLoad : public BaseMetric
    {
    public:
        MachineCpuLoad(CollectorHAL *hal, ComPtr<IUnknown> object, RTPROCESS process, SubMetric *user, SubMetric *kernel)
        : BaseMetric(hal, "CPU/Load", object), mProcess(process), mUser(user), mKernel(kernel) {};
        ~MachineCpuLoad() { delete mUser; delete mKernel; };

        void init(ULONG period, ULONG length);
        void collect();
        const char *getUnit() { return "%"; };
        ULONG getMinValue() { return 0; };
        ULONG getMaxValue() { return PM_CPU_LOAD_MULTIPLIER; };
        ULONG getScale() { return PM_CPU_LOAD_MULTIPLIER / 100; }
    protected:
        RTPROCESS  mProcess;
        SubMetric *mUser;
        SubMetric *mKernel;
    };

    class MachineCpuLoadRaw : public MachineCpuLoad
    {
    public:
        MachineCpuLoadRaw(CollectorHAL *hal, ComPtr<IUnknown> object, RTPROCESS process, SubMetric *user, SubMetric *kernel)
        : MachineCpuLoad(hal, object, process, user, kernel), mHostTotalPrev(0), mProcessUserPrev(0), mProcessKernelPrev(0) {};

        void preCollect(CollectorHints& hints, uint64_t iTick);
        void collect();
    private:
        uint64_t mHostTotalPrev;
        uint64_t mProcessUserPrev;
        uint64_t mProcessKernelPrev;
    };

    class MachineRamUsage : public BaseMetric
    {
    public:
        MachineRamUsage(CollectorHAL *hal, ComPtr<IUnknown> object, RTPROCESS process, SubMetric *used)
        : BaseMetric(hal, "RAM/Usage", object), mProcess(process), mUsed(used) {};
        ~MachineRamUsage() { delete mUsed; };

        void init(ULONG period, ULONG length);
        void preCollect(CollectorHints& hints, uint64_t iTick);
        void collect();
        const char *getUnit() { return "kB"; };
        ULONG getMinValue() { return 0; };
        ULONG getMaxValue() { return INT32_MAX; };
        ULONG getScale() { return 1; }
    private:
        RTPROCESS  mProcess;
        SubMetric *mUsed;
    };


    class GuestCpuLoad : public BaseGuestMetric
    {
    public:
        GuestCpuLoad(CollectorGuest *cguest, ComPtr<IUnknown> object, SubMetric *user, SubMetric *kernel, SubMetric *idle)
            : BaseGuestMetric(cguest, "Guest/CPU/Load", object), mUser(user), mKernel(kernel), mIdle(idle) {};
        ~GuestCpuLoad() { delete mUser; delete mKernel; delete mIdle; };

        void init(ULONG period, ULONG length);
        void preCollect(CollectorHints& hints, uint64_t iTick);
        void collect();
        const char *getUnit() { return "%"; };
        ULONG getMinValue() { return 0; };
        ULONG getMaxValue() { return PM_CPU_LOAD_MULTIPLIER; };
        ULONG getScale() { return PM_CPU_LOAD_MULTIPLIER / 100; }
    protected:
        SubMetric *mUser;
        SubMetric *mKernel;
        SubMetric *mIdle;
    };

    class GuestRamUsage : public BaseGuestMetric
    {
    public:
        GuestRamUsage(CollectorGuest *cguest, ComPtr<IUnknown> object, SubMetric *total, SubMetric *free, SubMetric *balloon, SubMetric *shared, SubMetric *cache, SubMetric *pagedtotal)
            : BaseGuestMetric(cguest, "Guest/RAM/Usage", object), mTotal(total), mFree(free), mBallooned(balloon), mCache(cache), mPagedTotal(pagedtotal), mShared(shared) {};
        ~GuestRamUsage() { delete mTotal; delete mFree; delete mBallooned; delete mShared; delete mCache; delete mPagedTotal; };

        void init(ULONG period, ULONG length);
        void preCollect(CollectorHints& hints, uint64_t iTick);
        void collect();
        const char *getUnit() { return "kB"; };
        ULONG getMinValue() { return 0; };
        ULONG getMaxValue() { return INT32_MAX; };
        ULONG getScale() { return 1; }
    private:
        SubMetric *mTotal, *mFree, *mBallooned, *mCache, *mPagedTotal, *mShared;
    };

    /* Aggregate Functions **************************************************/
    class Aggregate
    {
    public:
        virtual ULONG compute(ULONG *data, ULONG length) = 0;
        virtual const char *getName() = 0;
    };

    class AggregateAvg : public Aggregate
    {
    public:
        virtual ULONG compute(ULONG *data, ULONG length);
        virtual const char *getName();
    };

    class AggregateMin : public Aggregate
    {
    public:
        virtual ULONG compute(ULONG *data, ULONG length);
        virtual const char *getName();
    };

    class AggregateMax : public Aggregate
    {
    public:
        virtual ULONG compute(ULONG *data, ULONG length);
        virtual const char *getName();
    };

    /* Metric Class *********************************************************/
    class Metric
    {
    public:
        Metric(BaseMetric *baseMetric, SubMetric *subMetric, Aggregate *aggregate) :
            mName(subMetric->getName()), mBaseMetric(baseMetric), mSubMetric(subMetric), mAggregate(aggregate)
        {
            if (mAggregate)
            {
                mName.append(":");
                mName.append(mAggregate->getName());
            }
        }

        ~Metric()
        {
            delete mAggregate;
        }
        bool associatedWith(ComPtr<IUnknown> object) { return getObject() == object; };

        const char *getName() { return mName.c_str(); };
        ComPtr<IUnknown> getObject() { return mBaseMetric->getObject(); };
        const char *getDescription()
            { return mAggregate ? "" : mSubMetric->getDescription(); };
        const char *getUnit() { return mBaseMetric->getUnit(); };
        ULONG getMinValue() { return mBaseMetric->getMinValue(); };
        ULONG getMaxValue() { return mBaseMetric->getMaxValue(); };
        ULONG getPeriod() { return mBaseMetric->getPeriod(); };
        ULONG getLength()
            { return mAggregate ? 1 : mBaseMetric->getLength(); };
        ULONG getScale() { return mBaseMetric->getScale(); }
        void query(ULONG **data, ULONG *count, ULONG *sequenceNumber);

    private:
        RTCString mName;
        BaseMetric *mBaseMetric;
        SubMetric  *mSubMetric;
        Aggregate  *mAggregate;
    };

    /* Filter Class *********************************************************/

    class Filter
    {
    public:
        Filter(ComSafeArrayIn(IN_BSTR, metricNames),
               ComSafeArrayIn(IUnknown * , objects));
        static bool patternMatch(const char *pszPat, const char *pszName,
                                 bool fSeenColon = false);
        bool match(const ComPtr<IUnknown> object, const RTCString &name) const;
    private:
        void init(ComSafeArrayIn(IN_BSTR, metricNames),
                  ComSafeArrayIn(IUnknown * , objects));

        typedef std::pair<const ComPtr<IUnknown>, const RTCString> FilterElement;
        typedef std::list<FilterElement> ElementList;

        ElementList mElements;

        void processMetricList(const com::Utf8Str &name, const ComPtr<IUnknown> object);
    };
}
#endif /* ___performance_h */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
