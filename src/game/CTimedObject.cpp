#include "../sphere/ProfileTask.h"
#include "../sphere/threads.h"
#include "../CException.h"
#include "CWorldGameTime.h"
#include "CWorldTickingList.h"
#include "CTimedObject.h"


CTimedObject::CTimedObject(PROFILE_TYPE profile)
{
    _profileType = profile;
    _fIsSleeping = true;
    _iTimeout = 0;
}

CTimedObject::~CTimedObject()
{
    EXC_TRY("Cleanup in destructor");

    ADDTOCALLSTACK("CTimedObject::~CTimedObject");
    //if (_iTimeout > 0)
    //{
        CWorldTickingList::DelObjSingle(this);
    //}

    EXC_CATCH;
}


void CTimedObject::_GoAwake()
{
    ADDTOCALLSTACK("CTimedObject::_GoAwake");
    /*
    * if the timeout did expire then it got ignored on it's tick and removed from the tick's map so we add it again,
    * otherwise it's not needed since the timer is already there.
    */
    if ((_iTimeout > 0) && (_iTimeout < CWorldGameTime::GetCurrentTime().GetTimeRaw()))
    {
        _SetTimeout(1);  // set to 1 msec to tick it ASAP.
    }
    _fIsSleeping = false;
}

bool CTimedObject::_CanTick() const
{
    //ADDTOCALLSTACK_INTENSIVE("CTimedObject::_CanTick");
    return _IsSleeping();
}

bool CTimedObject::CanTick() const
{
    //ADDTOCALLSTACK_INTENSIVE("CTimedObject::CanTick");
    THREAD_SHARED_LOCK_RETURN(_CanTick());
}

bool CTimedObject::OnTick()
{
    ADDTOCALLSTACK("CTimedObject::OnTick");
    THREAD_UNIQUE_LOCK_RETURN(_OnTick());
}

void CTimedObject::_SetTimeout(int64 iDelayInMsecs)
{
    ADDTOCALLSTACK("CTimedObject::_SetTimeout");
    // Assume we have the mutex already locked here

    const ProfileTask timersTask(PROFILE_TIMERS); // profile the settimeout proccess.
    if (_IsDeleted()) //prevent deleted objects from setting new timers to avoid nullptr calls
    {
        //CWorldTickingList::DelObjSingle(this); // This should already by done upon object deletion.
        return;
    }

    /*
    * Setting the new timer:
    *   Values lower than 0 just clear the timer (Note that this must happen after the 'if (_iTimeout > 0)'
    *       check deleting this object from tick's map) to clear it's timer.
    *   New timer will be the current server's time (not CPU's time, just the server's one) + the given delay.
    *   Adding the object to the tick's map.
    */
    if (iDelayInMsecs < 0)
    {
        CWorldTickingList::DelObjSingle(this);
        _SetTimeoutRaw(0);
    }
    else
    {
        const int64 iNewTimeout = CWorldGameTime::GetCurrentTime().GetTimeRaw() + iDelayInMsecs;
        CWorldTickingList::AddObjSingle(iNewTimeout, this, false); // Adding this object to the ticks list.
        _SetTimeoutRaw(iNewTimeout);
    }
}


void CTimedObject::SetTimeout(int64 iDelayInMsecs)
{
    ADDTOCALLSTACK("CTimedObject::SetTimeout");
    THREAD_UNIQUE_LOCK_SET;
    _SetTimeout(iDelayInMsecs);
}

// SetTimeout variants call the right virtual for SetTimeout
void CTimedObject::_SetTimeoutS(int64 iSeconds)
{
    _SetTimeout(iSeconds * MSECS_PER_SEC);
}
void CTimedObject::SetTimeoutS(int64 iSeconds)
{
    SetTimeout(iSeconds * MSECS_PER_SEC);
}

void CTimedObject::_SetTimeoutD(int64 iTenths)
{
    _SetTimeout(iTenths * MSECS_PER_TENTH);
}
void CTimedObject::SetTimeoutD(int64 iTenths)
{
    SetTimeout(iTenths * MSECS_PER_TENTH);
}

int64 CTimedObject::_GetTimerDiff() const noexcept
{
    return _iTimeout - CWorldGameTime::GetCurrentTime().GetTimeRaw();
}

int64 CTimedObject::_GetTimerAdjusted() const noexcept
{
    // How long till this will expire ?
    // RETURN: time in msecs from now.
    if (!_IsTimerSet())
        return -1;

    const int64 iDiffInMsecs = _GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;

    return iDiffInMsecs;
}
int64 CTimedObject::GetTimerAdjusted() const noexcept
{
    THREAD_SHARED_LOCK_RETURN(_GetTimerAdjusted());
}

int64 CTimedObject::_GetTimerDAdjusted() const noexcept
{
    // RETURN: time in tenths of second from now.
    if (!_IsTimerSet())
        return -1;

    const int64 iDiffInMsecs = _GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;

    return (iDiffInMsecs / MSECS_PER_TENTH);
}
int64 CTimedObject::GetTimerDAdjusted() const noexcept
{
    THREAD_SHARED_LOCK_RETURN(_GetTimerDAdjusted());
}

int64 CTimedObject::_GetTimerSAdjusted() const noexcept
{
    // RETURN: time in seconds from now.
    if (!_IsTimerSet())
        return -1;

    const int64 iDiffInMsecs = _GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;

    return (iDiffInMsecs / MSECS_PER_SEC);
}
int64 CTimedObject::GetTimerSAdjusted() const noexcept
{
    THREAD_SHARED_LOCK_RETURN(_GetTimerSAdjusted());
}


void CTimedObject::GoSleep()
{
    THREAD_UNIQUE_LOCK_SET;
    _GoSleep();
}

void CTimedObject::GoAwake()
{
    THREAD_UNIQUE_LOCK_SET;
    _GoAwake(); // Call virtuals!
}

PROFILE_TYPE CTimedObject::GetProfileType() const noexcept
{
    THREAD_SHARED_LOCK_RETURN(CTimedObject::_GetProfileType());
}

void CTimedObject::ClearTimeout() noexcept
{
    THREAD_UNIQUE_LOCK_SET;
    CTimedObject::_ClearTimeout();
}

bool CTimedObject::IsSleeping() const noexcept
{
    THREAD_SHARED_LOCK_RETURN(CTimedObject::_IsSleeping());
}

bool CTimedObject::IsTimerSet() const noexcept
{
    THREAD_SHARED_LOCK_RETURN(CTimedObject::_IsTimerSet());
}

bool CTimedObject::IsTimerExpired() const noexcept
{
    THREAD_SHARED_LOCK_RETURN(CTimedObject::_IsTimerExpired());
}
