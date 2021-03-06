/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape Portable Runtime (NSPR).
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "primpl.h"

#if defined(WIN95)
/*
** Some local variables report warnings on Win95 because the code paths 
** using them are conditioned on HAVE_CUSTOME_USER_THREADS.
** The pragma suppresses the warning.
** 
*/
#pragma warning(disable : 4101)
#endif


extern PRLock *_pr_sleeplock;  /* allocated and initialized in prinit */
/* 
** Routines common to both native and user threads.
**
**
** Clean up a thread object, releasing all of the attached data. Do not
** free the object itself (it may not have been malloc'd)
*/
void _PR_CleanupThread(PRThread *thread)
{
    /* Free up per-thread-data */
    _PR_DestroyThreadPrivate(thread);

    /* Free any thread dump procs */
    if (thread->dumpArg) {
        PR_DELETE(thread->dumpArg);
    }
    thread->dump = 0;

    PR_DELETE(thread->errorString);
    thread->errorStringSize = 0;
    thread->errorStringLength = 0;
    thread->environment = NULL;
}

PR_IMPLEMENT(PRStatus) PR_Yield()
{
    static PRBool warning = PR_TRUE;
    if (warning) warning = _PR_Obsolete(
        "PR_Yield()", "PR_Sleep(PR_INTERVAL_NO_WAIT)");
    return (PR_Sleep(PR_INTERVAL_NO_WAIT));
}

/*
** Make the current thread sleep until "timeout" ticks amount of time
** has expired. If "timeout" is PR_INTERVAL_NO_WAIT then the call is
** equivalent to a yield. Waiting for an infinite amount of time is
** allowed in the expectation that another thread will interrupt().
**
** A single lock is used for all threads calling sleep. Each caller
** does get its own condition variable since each is expected to have
** a unique 'timeout'.
*/
PR_IMPLEMENT(PRStatus) PR_Sleep(PRIntervalTime timeout)
{
    PRStatus rv = PR_SUCCESS;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (PR_INTERVAL_NO_WAIT == timeout)
    {
        /*
        ** This is a simple yield, nothing more, nothing less.
        */
        PRIntn is;
        PRThread *me = PR_GetCurrentThread();
        PRUintn pri = me->priority;
        _PRCPU *cpu = _PR_MD_CURRENT_CPU();

        if ( _PR_IS_NATIVE_THREAD(me) ) _PR_MD_YIELD();
        else
        {
            _PR_INTSOFF(is);
            _PR_RUNQ_LOCK(cpu);
            if (_PR_RUNQREADYMASK(cpu) >> pri) {
                me->cpu = cpu;
                me->state = _PR_RUNNABLE;
                _PR_ADD_RUNQ(me, cpu, pri);
                _PR_RUNQ_UNLOCK(cpu);

                PR_LOG(_pr_sched_lm, PR_LOG_MIN, ("PR_Yield: yielding"));
                _PR_MD_SWITCH_CONTEXT(me);
                PR_LOG(_pr_sched_lm, PR_LOG_MIN, ("PR_Yield: done"));

                _PR_FAST_INTSON(is);
            }
            else
            {
                _PR_RUNQ_UNLOCK(cpu);
                _PR_INTSON(is);
            }
        }
    }
    else
    {
        /*
        ** This is waiting for some finite period of time.
        ** A thread in this state is interruptible (PR_Interrupt()),
        ** but the lock and cvar used are local to the implementation
        ** and not visible to the caller, therefore not notifiable.
        */
        PRCondVar *cv;
        PRIntervalTime timein;

        timein = PR_IntervalNow();
        cv = PR_NewCondVar(_pr_sleeplock);
        PR_ASSERT(cv != NULL);
        PR_Lock(_pr_sleeplock);
        do
        {
            PRIntervalTime delta = PR_IntervalNow() - timein;
            if (delta > timeout) break;
            rv = PR_WaitCondVar(cv, timeout - delta);
        } while (rv == PR_SUCCESS);
        PR_Unlock(_pr_sleeplock);
        PR_DestroyCondVar(cv);
    }
    return rv;
}

PR_IMPLEMENT(PRUint32) PR_GetThreadID(PRThread *thread)
{
    return thread->id;
}

PR_IMPLEMENT(PRThreadPriority) PR_GetThreadPriority(const PRThread *thread)
{
    return (PRThreadPriority) thread->priority;
}

PR_IMPLEMENT(PRThread *) PR_GetCurrentThread()
{
    if (!_pr_initialized) _PR_ImplicitInitialization();
    return _PR_MD_CURRENT_THREAD();
}

/*
** Set the interrupt flag for a thread. The thread will be unable to
** block in i/o functions when this happens. Also, any PR_Wait's in
** progress will be undone. The interrupt remains in force until
** PR_ClearInterrupt is called.
*/
PR_IMPLEMENT(PRStatus) PR_Interrupt(PRThread *thread)
{
#ifdef _PR_GLOBAL_THREADS_ONLY
    PRCondVar *victim;

    _PR_THREAD_LOCK(thread);
    thread->flags |= _PR_INTERRUPT;
    victim = thread->wait.cvar;
    _PR_THREAD_UNLOCK(thread);
    if ((NULL != victim) && (!(thread->flags & _PR_INTERRUPT_BLOCKED))) {
        int haveLock = (victim->lock->owner == _PR_MD_CURRENT_THREAD());

        if (!haveLock) PR_Lock(victim->lock);
        PR_NotifyAllCondVar(victim);
        if (!haveLock) PR_Unlock(victim->lock);
    }
    return PR_SUCCESS;
#else  /* ! _PR_GLOBAL_THREADS_ONLY */
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

            if (!_PR_IS_NATIVE_THREAD(me))
            	_PR_INTSOFF(is);

            _PR_THREAD_LOCK(thread);
            thread->flags |= _PR_INTERRUPT;
        switch (thread->state) {
                case _PR_COND_WAIT:
                        /*
                         * call is made with thread locked;
                         * on return lock is released
                         */
						if (!(thread->flags & _PR_INTERRUPT_BLOCKED))
                        	_PR_NotifyLockedThread(thread);
                        break;
                case _PR_IO_WAIT:
                        /*
                         * Need to hold the thread lock when calling
                         * _PR_Unblock_IO_Wait().  On return lock is
                         * released. 
                         */
#if defined(XP_UNIX) || defined(WINNT) || defined(WIN16)
						if (!(thread->flags & _PR_INTERRUPT_BLOCKED))
                        	_PR_Unblock_IO_Wait(thread);
#else
                        _PR_THREAD_UNLOCK(thread);
#endif
                        break;
                case _PR_RUNNING:
                case _PR_RUNNABLE:
                case _PR_LOCK_WAIT:
                default:
                            _PR_THREAD_UNLOCK(thread);
                        break;
        }
            if (!_PR_IS_NATIVE_THREAD(me))
            	_PR_INTSON(is);
            return PR_SUCCESS;
#endif  /* _PR_GLOBAL_THREADS_ONLY */
}

/*
** Clear the interrupt flag for self.
*/
PR_IMPLEMENT(void) PR_ClearInterrupt()
{
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

        if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);
    _PR_THREAD_LOCK(me);
         me->flags &= ~_PR_INTERRUPT;
    _PR_THREAD_UNLOCK(me);
        if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSON(is);
}

PR_IMPLEMENT(void) PR_BlockInterrupt()
{
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);
    _PR_THREAD_LOCK(me);
    _PR_THREAD_BLOCK_INTERRUPT(me);
    _PR_THREAD_UNLOCK(me);
    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSON(is);
}  /* PR_BlockInterrupt */

PR_IMPLEMENT(void) PR_UnblockInterrupt()
{
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);
    _PR_THREAD_LOCK(me);
    _PR_THREAD_UNBLOCK_INTERRUPT(me);
    _PR_THREAD_UNLOCK(me);
    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSON(is);
}  /* PR_UnblockInterrupt */

/*
** Return the thread stack pointer of the given thread.
*/
PR_IMPLEMENT(void *) PR_GetSP(PRThread *thread)
{
        return (void *)_PR_MD_GET_SP(thread);
}

PR_IMPLEMENT(void*) GetExecutionEnvironment(PRThread *thread)
{
        return thread->environment;
}

PR_IMPLEMENT(void) SetExecutionEnvironment(PRThread *thread, void *env)
{
        thread->environment = env;
}


PR_IMPLEMENT(PRInt32) PR_GetThreadAffinityMask(PRThread *thread, PRUint32 *mask)
{
#ifdef HAVE_THREAD_AFFINITY
    return _PR_MD_GETTHREADAFFINITYMASK(thread, mask);
#else

#if defined(XP_MAC)
#pragma unused (thread, mask)
#endif

    return 0;
#endif
}

PR_IMPLEMENT(PRInt32) PR_SetThreadAffinityMask(PRThread *thread, PRUint32 mask )
{
#ifdef HAVE_THREAD_AFFINITY
#ifndef IRIX
    return _PR_MD_SETTHREADAFFINITYMASK(thread, mask);
#else
	return 0;
#endif
#else

#if defined(XP_MAC)
#pragma unused (thread, mask)
#endif

    return 0;
#endif
}

/* This call is thread unsafe if another thread is calling SetConcurrency()
 */
PR_IMPLEMENT(PRInt32) PR_SetCPUAffinityMask(PRUint32 mask)
{
#ifdef HAVE_THREAD_AFFINITY
    PRCList *qp;
    extern PRUint32 _pr_cpu_affinity_mask;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    _pr_cpu_affinity_mask = mask;

    qp = _PR_CPUQ().next;
    while(qp != &_PR_CPUQ()) {
        _PRCPU *cpu;

        cpu = _PR_CPU_PTR(qp);
        PR_SetThreadAffinityMask(cpu->thread, mask);

        qp = qp->next;
    }
#endif

#if defined(XP_MAC)
#pragma unused (mask)
#endif

    return 0;
}

PRUint32 _pr_recycleThreads = 0;
PR_IMPLEMENT(void) PR_SetThreadRecycleMode(PRUint32 count)
{
    _pr_recycleThreads = count;
}

PR_IMPLEMENT(PRThread*) PR_CreateThreadGCAble(PRThreadType type,
                                     void (*start)(void *arg),
                                     void *arg,
                                     PRThreadPriority priority,
                                     PRThreadScope scope,
                                     PRThreadState state,
                                     PRUint32 stackSize)
{
    return _PR_CreateThread(type, start, arg, priority, scope, state, 
                            stackSize, _PR_GCABLE_THREAD);
}

#ifdef SOLARIS
PR_IMPLEMENT(PRThread*) PR_CreateThreadBound(PRThreadType type,
                                     void (*start)(void *arg),
                                     void *arg,
                                     PRUintn priority,
                                     PRThreadScope scope,
                                     PRThreadState state,
                                     PRUint32 stackSize)
{
    return _PR_CreateThread(type, start, arg, priority, scope, state, 
                            stackSize, _PR_BOUND_THREAD);
}
#endif


PR_IMPLEMENT(PRThread*) PR_AttachThreadGCAble(
    PRThreadType type, PRThreadPriority priority, PRThreadStack *stack)
{
#ifdef XP_MAC
#pragma unused (type, priority, stack)
#endif
    /* $$$$ not sure how to finese this one */
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}

PR_IMPLEMENT(void) PR_SetThreadGCAble()
{
    if (!_pr_initialized) _PR_ImplicitInitialization();
    PR_Lock(_pr_activeLock);
        _PR_MD_CURRENT_THREAD()->flags |= _PR_GCABLE_THREAD;
        PR_Unlock(_pr_activeLock);        
}

PR_IMPLEMENT(void) PR_ClearThreadGCAble()
{
    if (!_pr_initialized) _PR_ImplicitInitialization();
    PR_Lock(_pr_activeLock);
        _PR_MD_CURRENT_THREAD()->flags &= (~_PR_GCABLE_THREAD);
        PR_Unlock(_pr_activeLock);
}

PR_IMPLEMENT(PRThreadScope) PR_GetThreadScope(const PRThread *thread)
{
#ifdef XP_MAC
#pragma unused( thread )
#endif
    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (_PR_IS_NATIVE_THREAD(thread)) {
    	return (thread->flags & _PR_BOUND_THREAD) ? PR_GLOBAL_BOUND_THREAD :
										PR_GLOBAL_THREAD;
    } else
        return PR_LOCAL_THREAD;
}

PR_IMPLEMENT(PRThreadType) PR_GetThreadType(const PRThread *thread)
{
    return (thread->flags & _PR_SYSTEM) ? PR_SYSTEM_THREAD : PR_USER_THREAD;
}

PR_IMPLEMENT(PRThreadState) PR_GetThreadState(const PRThread *thread)
{
    return (NULL == thread->term) ? PR_UNJOINABLE_THREAD : PR_JOINABLE_THREAD;
}  /* PR_GetThreadState */
