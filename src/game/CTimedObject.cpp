#include "../sphere/ProfileTask.h"
#include "../sphere/threads.h"
#include "CWorldGameTime.h"
#include "CWorldTickingList.h"
#include "CTimedObject.h"


CTimedObject::CTimedObject(PROFILE_TYPE profile)
{
    _profileType = profile;
    _fIsSleeping = false;
    _iTimeout = 0;
}

CTimedObject::~CTimedObject()
{
    ADDTOCALLSTACK("CTimedObject::~CTimedObject");
    //if (_iTimeout > 0)
    //{
        CWorldTickingList::DelObjSingle(this);
    //}
}


void CTimedObject::GoAwake()
{
    ADDTOCALLSTACK("CTimedObject::GoAwake");
    /*
    * if the timeout did expire then it got ignored on it's tick and removed from the tick's map so we add it again,
    * otherwise it's not needed since the timer is already there.
    */
    if ((_iTimeout > 0) && (_iTimeout < CWorldGameTime::GetCurrentTime().GetTimeRaw()))
    {
        SetTimeout(1);  // set to 1 msec to tick it ASAP.
    }
    _fIsSleeping = false;
}

bool CTimedObject::OnTick()
{
    CWorldTickingList::DelObjSingle(this);  // this also sets _iTimeOut = 0;
    return true;
}


void CTimedObject::SetTimeout(int64 iDelayInMsecs)
{
    ADDTOCALLSTACK("CTimedObject::SetTimeout");

    const ProfileTask timersTask(PROFILE_TIMERS); // profile the settimeout proccess.
    if (IsDeleted()) //prevent deleted objects from setting new timers to avoid nullptr calls
    {
        return;
    }

    /*
    * Setting the new timer:
    *   Values lower than 0 just clear the timer (Note that this must happen after the 'if (_iTimeout > 0)' 
    *       check deleting this object from tick's map) to clear it's timer.
    *   New timer will be the current server's time (not CPU's time, just the server's one) + the given delay.
    *   Adding the object to the tick's map.
    */
    THREAD_UNIQUE_LOCK_SET;
    if (iDelayInMsecs < 0)
    {
        CWorldTickingList::DelObjSingle(this);
    }
    else
    {
        CWorldTickingList::AddObjSingle(CWorldGameTime::GetCurrentTime().GetTimeRaw() + iDelayInMsecs, this); // Adding this object to the tick's list.
    }
}


void CTimedObject::SetTimeoutS(int64 iSeconds)
{
    SetTimeout(iSeconds * MSECS_PER_SEC);
}

void CTimedObject::SetTimeoutD(int64 iTenths)
{
    SetTimeout(iTenths * MSECS_PER_TENTH);
}

int64 CTimedObject::GetTimerDiff() const
{
    // How long till this will expire ?
    return _iTimeout - CWorldGameTime::GetCurrentTime().GetTimeRaw();
}

int64 CTimedObject::GetTimerAdjusted() const
{
    // RETURN: time in msecs from now.
    if (!IsTimerSet())
        return -1;

    const int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;

    return (iDiffInMsecs);
}

// We shouldn't really use this
/*
int64 CTimedObject::GetTimerTAdjusted() const
{
    // RETURN: time in ticks from now.
    if (!IsTimerSet())
        return -1;
    int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;
    return (iDiffInMsecs / MSECS_PER_TICK);
}
*/

int64 CTimedObject::GetTimerDAdjusted() const
{
    // RETURN: time in tenths of second from now.
    if (!IsTimerSet())
        return -1;

    const int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;

    return (iDiffInMsecs / MSECS_PER_TENTH);
}

int64 CTimedObject::GetTimerSAdjusted() const
{
    // RETURN: time in seconds from now.
    if (!IsTimerSet())
        return -1;

    const int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;

    return (iDiffInMsecs / MSECS_PER_SEC);
}
