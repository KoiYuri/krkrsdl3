//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Thread base class
//---------------------------------------------------------------------------

#include "tjsCommHead.h"
#include "Platform.h"
#include "PlatformThread.h"
#include "PlatformMutex.h"

#include "TVPMsg.h"
#include "TVPDebug.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

//---------------------------------------------------------------------------
// tTVPThread : a wrapper class for thread
//---------------------------------------------------------------------------
#define THR_IMPL ((TVPThreadImpl*)_impl)
struct TVPThreadImpl
{
    SDL_Thread* thread;
    SDL_Mutex* mutex;
    SDL_Condition* cond;
};
tTVPThread::tTVPThread(const char* name)
{
    Terminated = false;
    Suspended = true;

    _impl = new TVPThreadImpl;
    THR_IMPL->mutex = SDL_CreateMutex();
    THR_IMPL->cond = SDL_CreateCondition();
    THR_IMPL->thread = SDL_CreateThread(StartProc, name, this);
    if (!THR_IMPL->thread)
    {
        TVPThrowInternalError;
    }
}
//---------------------------------------------------------------------------
tTVPThread::~tTVPThread()
{
    if (!Terminated)
        Terminate();
    delete THR_IMPL;
    SDL_DestroyCondition(THR_IMPL->cond);
    SDL_DestroyMutex(THR_IMPL->mutex);
    _impl = NULL;
}
//---------------------------------------------------------------------------
void tTVPThread::Terminate()
{
    Terminated = true;
}
void tTVPThread::StopThread()
{
    Terminated = true;
    SDL_BroadcastCondition(THR_IMPL->cond);
    SDL_WaitThread(THR_IMPL->thread, nullptr);
}
//---------------------------------------------------------------------------
void tTVPThread::Sleep(unsigned int milliseconds)
{
    if (IsCurrentThread())
    {
        SDL_LockMutex(THR_IMPL->mutex);
        SDL_WaitConditionTimeout(THR_IMPL->cond, THR_IMPL->mutex, (int)milliseconds);
        SDL_UnlockMutex(THR_IMPL->mutex);
    }
    else
    {
        SDL_Delay(milliseconds);
    }
}
//---------------------------------------------------------------------------
bool tTVPThread::IsCurrentThread()
{
    return (SDL_GetCurrentThreadID() == SDL_GetThreadID(THR_IMPL->thread));
}
//---------------------------------------------------------------------------
int tTVPThread::StartProc(void* arg)
{
    tTVPThread* _this = static_cast<tTVPThread*>(arg);
    TVPThreadImpl* impl = (TVPThreadImpl*)_this->_impl;
    if (!impl)
        return 0;

    // 等待恢复
    if (_this->Suspended)
    {
        SDL_LockMutex(impl->mutex);
        SDL_WaitCondition(impl->cond, impl->mutex);
        SDL_UnlockMutex(impl->mutex);
    }
    _this->Execute();
    _this->OnExit();
    _this->Terminated = false;
    TVPOnThreadExited();

    return 0;
}
//---------------------------------------------------------------------------
void tTVPThread::WaitFor()
{
    SDL_WaitThread(THR_IMPL->thread, nullptr);
}
//---------------------------------------------------------------------------
tTVPThreadPriority tTVPThread::GetPriority()
{
    // do nothing
    return ttpNormal;
}
//---------------------------------------------------------------------------
void tTVPThread::SetPriority(tTVPThreadPriority pri)
{
    // do nothing
}
//---------------------------------------------------------------------------
void tTVPThread::Resume()
{
    Suspended = false;
    SDL_SignalCondition(THR_IMPL->cond);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPThreadEvent
//---------------------------------------------------------------------------
#define EVT_IMPL ((TVPThreadEventImpl*)_impl)
struct TVPThreadEventImpl
{
    SDL_Condition* cond;
    SDL_Mutex* mutex;
};

tTVPThreadEvent::tTVPThreadEvent()
{
    _impl = new TVPThreadEventImpl;
    EVT_IMPL->cond = SDL_CreateCondition();
    EVT_IMPL->mutex = SDL_CreateMutex();
}

tTVPThreadEvent::~tTVPThreadEvent()
{
    SDL_DestroyCondition(EVT_IMPL->cond);
    SDL_DestroyMutex(EVT_IMPL->mutex);
    delete EVT_IMPL;
}

void tTVPThreadEvent::Set()
{
    SDL_LockMutex(EVT_IMPL->mutex);
    SDL_SignalCondition(EVT_IMPL->cond);
    SDL_UnlockMutex(EVT_IMPL->mutex);
}
//---------------------------------------------------------------------------
bool tTVPThreadEvent::WaitFor(int timeout)
{
    // wait for event;
    // returns true if the event is set, otherwise (when timed out) returns false.

    SDL_LockMutex(EVT_IMPL->mutex);
    bool result;
    if (timeout != 0)
        result = SDL_WaitConditionTimeout(EVT_IMPL->cond, EVT_IMPL->mutex, (int)timeout);
    else
    {
        SDL_WaitCondition(EVT_IMPL->cond, EVT_IMPL->mutex);
        result = true;
    }
    SDL_UnlockMutex(EVT_IMPL->mutex);
    return result;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
int TVPDrawThreadNum = 1;

static std::vector<tjs_int> TVPProcesserIdList;
static tjs_int TVPThreadTaskNum, TVPThreadTaskCount;

//---------------------------------------------------------------------------
int TVPGetThreadNum(void)
{
    tjs_int threadNum = TVPDrawThreadNum ? TVPDrawThreadNum : TVPGetProcessorNum();
    threadNum = std::min(threadNum, TVPMaxThreadNum);
    return threadNum;
}

//---------------------------------------------------------------------------
void TVPExecThreadTask(int numThreads, TVP_THREAD_TASK_FUNC func)
{
    if (numThreads == 1)
    {
        func(0);
        return;
    }

#pragma omp parallel for schedule(static)
    for (int i = 0; i < numThreads; ++i)
        func(i);
}
//---------------------------------------------------------------------------

// xp3filter cleaner
std::vector<std::function<void()>> _OnThreadExitedEvents;

void TVPOnThreadExited()
{
    for (const auto& ev : _OnThreadExitedEvents)
    {
        ev();
    }
}

void TVPAddOnThreadExitEvent(const std::function<void()>& ev)
{
    _OnThreadExitedEvents.emplace_back(ev);
}

uint64_t TVPGetCurrentThreadID()
{
    return SDL_GetCurrentThreadID();
}

void TVPSleepFor(uint32_t ms)
{
    SDL_Delay(ms);
}

//---------------------------------------------------------------------------
// tTJSCriticalSection
//---------------------------------------------------------------------------
struct tTJSCriticalSectionImpl
{
    SDL_Mutex* _mutex;
    SDL_ThreadID _tid;

    tTJSCriticalSectionImpl() : _mutex(SDL_CreateMutex()), _tid(0) {}
    ~tTJSCriticalSectionImpl() { SDL_DestroyMutex(_mutex); }

    bool lock()
    {
        SDL_ThreadID id = SDL_GetCurrentThreadID();
        if (_tid == id)
            return false;

        SDL_LockMutex(_mutex);
        _tid = id;
        return true;
    }
    void unlock()
    {
        _tid = 0;
        SDL_UnlockMutex(_mutex);
    }
};

bool tTJSCriticalSection::lock()
{
    return _impl->lock();
}
void tTJSCriticalSection::unlock()
{
    _impl->unlock();
}
tTJSCriticalSection::tTJSCriticalSection()
{
    _impl = new tTJSCriticalSectionImpl;
}
tTJSCriticalSection::~tTJSCriticalSection()
{
    delete _impl;
}

//---------------------------------------------------------------------------
// tTJSCriticalSectionHolder
//---------------------------------------------------------------------------
tTJSCriticalSectionHolder::tTJSCriticalSectionHolder(tTJSCriticalSection& cs)
{
    if (cs.lock())
    {
        _cs = &cs;
    }
    else
    {
        _cs = nullptr;
    }
}
tTJSCriticalSectionHolder::~tTJSCriticalSectionHolder()
{
    if (_cs)
        _cs->unlock();
}

//---------------------------------------------------------------------------
// tTJSUniqueLock
//---------------------------------------------------------------------------
tTJSUniqueLock::tTJSUniqueLock(tTJSCriticalSection& cs) : owns(true)
{
    if (cs.lock())
    {
        _cs = &cs;
    }
    else
    {
        _cs = nullptr;
    }
}
tTJSUniqueLock::~tTJSUniqueLock()
{
    if (owns && _cs)
        _cs->unlock();
}
void tTJSUniqueLock::unlock()
{
    if (owns)
    {
        owns = false;
        _cs->unlock();
    }
}
void tTJSUniqueLock::lock()
{
    if (!owns)
    {
        owns = true;
        _cs->lock();
    }
}

//---------------------------------------------------------------------------
// tTVPCondition
//---------------------------------------------------------------------------
tTVPCondition::tTVPCondition() : _impl(SDL_CreateCondition())
{
}
tTVPCondition::~tTVPCondition()
{
    SDL_DestroyCondition((SDL_Condition*)_impl);
}
void tTVPCondition::notify_one()
{
    SDL_SignalCondition((SDL_Condition*)_impl);
}
void tTVPCondition::notify_all()
{
    SDL_BroadcastCondition((SDL_Condition*)_impl);
}
void tTVPCondition::Wait(tTJSCriticalSection& cs)
{
    SDL_WaitCondition((SDL_Condition*)_impl, (SDL_Mutex*)cs._impl->_mutex);
}
bool tTVPCondition::WaitFor(tTJSCriticalSection& cs, unsigned int ms)
{
    return SDL_WaitConditionTimeout((SDL_Condition*)_impl, (SDL_Mutex*)cs._impl->_mutex, (int)ms);
}

//---------------------------------------------------------------------------
// tTJSSpinLock
//---------------------------------------------------------------------------
void tTJSSpinLock::lock()
{
    SDL_LockSpinlock(&splock);
}
void tTJSSpinLock::unlock()
{
    SDL_UnlockSpinlock(&splock);
}
tTJSSpinLock::tTJSSpinLock()
{
    unlock();
}

tTJSSpinLockHolder::tTJSSpinLockHolder(tTJSSpinLock& lock)
{
    lock.lock();
    Lock = &lock;
}
tTJSSpinLockHolder::~tTJSSpinLockHolder()
{
    if (Lock)
    {
        Lock->unlock();
    }
}
//---------------------------------------------------------------------------