#pragma once

#include <functional>

//---------------------------------------------------------------------------
// tTVPThreadPriority
//---------------------------------------------------------------------------
enum tTVPThreadPriority
{
    ttpIdle,
    ttpLowest,
    ttpLower,
    ttpNormal,
    ttpHigher,
    ttpHighest,
    ttpTimeCritical
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPThread
//---------------------------------------------------------------------------
class tTVPThread
{
    bool Terminated;
    bool Suspended;
    void* _impl;

    static int StartProc(void* arg);

public:
    tTVPThread(const char* name);
    virtual ~tTVPThread();

    bool GetTerminated() const { return Terminated; }
    bool IsRunning() { return !Suspended; }
    void Terminate();
    void StopThread();
    void Sleep(unsigned int milliseconds);
    bool IsCurrentThread();

protected:
    virtual void Execute() = 0;
    virtual void OnExit(){};

public:
    void WaitFor();
    tTVPThreadPriority GetPriority();
    void SetPriority(tTVPThreadPriority pri);
    void Resume();
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPThreadEvent
//---------------------------------------------------------------------------
class tTVPThreadEvent
{
    void* _impl;

public:
    tTVPThreadEvent();
    ~tTVPThreadEvent();
    void Set();
    bool WaitFor(int timeout);
};

/*[*/
const int TVPMaxThreadNum = 8;
typedef const std::function<void(int)>& TVP_THREAD_TASK_FUNC;
/*]*/

int TVPGetThreadNum();
void TVPExecThreadTask(int numThreads, TVP_THREAD_TASK_FUNC func);

//---------------------------------------------------------------------------
void TVPAddOnThreadExitEvent(const std::function<void()>& ev);
void TVPOnThreadExited();
//---------------------------------------------------------------------------

uint64_t TVPGetCurrentThreadID();
void TVPSleepFor(uint32_t ms);
//---------------------------------------------------------------------------