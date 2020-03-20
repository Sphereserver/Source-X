#include "../../sphere/ProfileTask.h"
#include "CWorld.h"
#include "CTimedObject.h"


void CTimedObject::ClearTimeout()
{
    _timeout = 0;
}

CTimedObject::CTimedObject(PROFILE_TYPE profile)
{
    _profileType = profile;
    _fIsSleeping = false;
    _timeout = 0;
}

CTimedObject::~CTimedObject()
{
    ADDTOCALLSTACK("CTimedObject::~CTimedObject");
    //if (_timeout > 0)
    //{
        g_World._Ticker.DelTimedObject(this);
    //}
}

void CTimedObject::Delete()
{
    ADDTOCALLSTACK("CTimedObject::Delete");
    //if (_timeout > 0)
    //{
        g_World._Ticker.DelTimedObject(this);
    //}
}

void CTimedObject::GoAwake()
{
    /*
    * if the timeout did expire then it got ignored on it's tick and removed from the tick's map so we add it again,
    * otherwise it's not needed since the timer is already there.
    */
    if ((_timeout > 0) && (_timeout < CServerTime::GetCurrentTime().GetTimeRaw()))
    {
        SetTimeout(1);  // set to 1msec to tick it ASAP.
    }
    _fIsSleeping = false;
}

bool CTimedObject::OnTick()
{
    ClearTimeout();
    return true;
}

void CTimedObject::SetTimer(int64 iDelayInMsecs)
{
    /*
    * Only called from CObjBase::r_LoadVal when server is loading to set the raw timer
    * instead of doing conversions to msecs.
    */
    THREAD_CMUTEX.lock();
    _timeout = iDelayInMsecs;
    THREAD_CMUTEX.unlock();
}

void CTimedObject::SetTimeout(int64 iDelayInMsecs)
{
    ADDTOCALLSTACK("CTimedObject::SetTimeout");
    const ProfileTask timersTask(PROFILE_TIMERS); // profile the settimeout proccess.
    
    /*
    * Setting the new timer must remove any entry from the current world tick's map
    * Should never happen if g_Serv.IsLoading()
    */
    //if (_timeout > 0)
    //{
        g_World._Ticker.DelTimedObject(this);
    //}
    if (IsDeleted()) //prevent deleted objects from setting new timers to avoid nullptr calls
    {
        return;
    }

    /*
    * Setting the new timer:
    *   Values lower than 0 just clear the timer (Note that this must happen after the 'if (_timeout > 0)' 
    *       check deleting this object from tick's map) to clear it's timer.
    *   New timer will be the current server's time (not CPU's time, just the server's one) + the given delay.
    *   Adding the object to the tick's map.
    */
    if (iDelayInMsecs < 0)
    {
        SetTimer(0);
    }
    else
    {
        SetTimer(CServerTime::GetCurrentTime().GetTimeRaw() + iDelayInMsecs);   // Setting the new Timeout value
        g_World._Ticker.AddTimedObject(_timeout, this); // Adding this object to the tick's list.
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
    return g_World.GetTimeDiff(_timeout);
}

int64 CTimedObject::GetTimerAdjusted() const
{
    // RETURN: time in msecs from now.
    if (!IsTimerSet())
        return -1;
    int64 iDiffInMsecs = GetTimerDiff();
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
    int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;
    return (iDiffInMsecs / MSECS_PER_TENTH);
}

int64 CTimedObject::GetTimerSAdjusted() const
{
    // RETURN: time in seconds from now.
    if (!IsTimerSet())
        return -1;
    int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;
    return (iDiffInMsecs / MSECS_PER_SEC);
}
