#pragma once

//---------------------------------------------------------------------------
// tTJSCriticalSection ( implement on each platform for multi-threading support )
//---------------------------------------------------------------------------
struct tTJSCriticalSectionImpl;
struct tTJSCriticalSection
{
    tTJSCriticalSectionImpl* _impl;
    tTJSCriticalSection();
    ~tTJSCriticalSection();
    bool lock();
    void unlock();
};
typedef tTJSCriticalSection tTJSStaticCriticalSection;
//---------------------------------------------------------------------------
// tTJSCriticalSectionHolder
//---------------------------------------------------------------------------
class tTJSCriticalSectionHolder
{
    tTJSCriticalSection* _cs;

public:
    tTJSCriticalSectionHolder(tTJSCriticalSection& cs);
    ~tTJSCriticalSectionHolder();
};
typedef tTJSCriticalSectionHolder tTJSCSH;
//---------------------------------------------------------------------------
// tTJSUniqueLock
//---------------------------------------------------------------------------
class tTJSUniqueLock
{
    tTJSCriticalSection* _cs;
    bool owns;

public:
    tTJSUniqueLock(tTJSCriticalSection& cs);
    ~tTJSUniqueLock();
    void unlock();
    void lock();
};

//---------------------------------------------------------------------------
// tTVPCondition
//---------------------------------------------------------------------------
class tTVPCondition
{
    void* _impl;

public:
    tTVPCondition();
    ~tTVPCondition();
    void notify_one();
    void notify_all();
    void Wait(tTJSCriticalSection& cs);
    bool WaitFor(tTJSCriticalSection& cs, unsigned int ms);
};
//---------------------------------------------------------------------------
// SpinLock
//---------------------------------------------------------------------------
struct tTJSSpinLock
{
    int splock;
    tTJSSpinLock();
    void lock();
    void unlock();
};
class tTJSSpinLockHolder
{
    tTJSSpinLock* Lock;

public:
    tTJSSpinLockHolder(tTJSSpinLock& lock);

    ~tTJSSpinLockHolder();
};
//---------------------------------------------------------------------------