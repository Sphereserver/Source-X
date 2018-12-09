
#include "CCTimedObject.h"
#include "../../sphere/ProfileTask.h"
#include "../CWorld.h"



void CCTimedObject::ClearTimeout()
{
    _timeout = 0;
}

CCTimedObject::CCTimedObject(PROFILE_TYPE profile)
{
    _profileType = profile;
    _fIsSleeping = false;
    _timeout = 0;
}

CCTimedObject::~CCTimedObject()
{
    ADDTOCALLSTACK("CCTimedObject::~CCTimedObject");
    //if (_timeout > 0)
    //{
        g_World.DelTimedObject(this);
    //}
}

void CCTimedObject::GoAwake()
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

bool CCTimedObject::OnTick()
{
    ClearTimeout();
    return true;
}

void CCTimedObject::SetTimer(int64 iDelayInMsecs)
{
    /*
    * Only called from CObjBase::r_LoadVal when server is loading to set the raw timer
    * instead of doing conversions to msecs.
    */
    THREAD_CMUTEX.lock();
    _timeout = iDelayInMsecs;
    THREAD_CMUTEX.unlock();
}

void CCTimedObject::SetTimeout(int64 iDelayInMsecs)
{
    ADDTOCALLSTACK("CCTimedObject::SetTimeout");
    ProfileTask timersTask(PROFILE_TIMERS); // profile the settimeout proccess.
    
    /*
    * Setting the new timer must remove any entry from the current world tick's map
    * Should never happen if g_Serv.IsLoading()
    */
    //if (_timeout > 0)
    //{
        g_World.DelTimedObject(this);
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
        g_World.AddTimedObject(_timeout, this); // Adding this object to the tick's list.
    }
}


void CCTimedObject::SetTimeoutS(int64 iSeconds)
{
    SetTimeout(iSeconds * MSECS_PER_SEC);
}

void CCTimedObject::SetTimeoutT(int64 iTicks)
{
    SetTimeout(iTicks * MSECS_PER_TICK);
}

void CCTimedObject::SetTimeoutD(int64 iTenths)
{
    SetTimeout(iTenths * MSECS_PER_TENTH);
}

int64 CCTimedObject::GetTimerDiff() const
{
    // How long till this will expire ?
    return g_World.GetTimeDiff(_timeout);
}

int64 CCTimedObject::GetTimerAdjusted() const
{
    // RETURN: time in msecs from now.
    if (!IsTimerSet())
        return -1;
    int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;
    return (iDiffInMsecs);
}

int64 CCTimedObject::GetTimerTAdjusted() const
{
    // RETURN: time in ticks from now.
    if (!IsTimerSet())
        return -1;
    int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;
    return (iDiffInMsecs / MSECS_PER_TICK);
}

int64 CCTimedObject::GetTimerDAdjusted() const
{
    // RETURN: time in tenths of second from now.
    if (!IsTimerSet())
        return -1;
    int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;
    return (iDiffInMsecs / MSECS_PER_TENTH);
}

int64 CCTimedObject::GetTimerSAdjusted() const
{
    // RETURN: time in seconds from now.
    if (!IsTimerSet())
        return -1;
    int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;
    return (iDiffInMsecs / MSECS_PER_SEC);
}