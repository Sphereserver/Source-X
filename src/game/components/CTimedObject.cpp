
#include "CTimedObject.h"
#include "../../sphere/ProfileTask.h"
#include "../CWorld.h"



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

bool CTimedObject::IsSleeping()
{
    return _fIsSleeping;
}

void CTimedObject::Sleep()
{
    _fIsSleeping = true;
}

void CTimedObject::Awake()
{
    /*
    * if the timeout did expire then it got ignored on it's tick and removed from the tick's list add it again,
    * otherwise it's not needed since the timer is already there
    */
    if (_timeout > 0)
    {
        SetTimeout(_timeout);
    }
    _fIsSleeping = false;
}

PROFILE_TYPE CTimedObject::GetProfileType()
{
    return _profileType;
}

bool CTimedObject::OnTick()
{
    ClearTimeout();
    return true;
}

void CTimedObject::Delete(bool bForce)
{
    ADDTOCALLSTACK("CTimedObject::Delete");
    UNREFERENCED_PARAMETER(bForce);
    if (_timeout > 0)
    {
        g_World.DelTimedObject(_timeout, this);
    }
}

void CTimedObject::SetTimeout(int64 iDelayInMsecs)
{
    ADDTOCALLSTACK("CTimedObject::SetTimeout");
    ProfileTask timersTask(PROFILE_TIMERS); // profile the settimeout proccess.
    if (g_Serv.IsLoading() || IsSleeping())
    {
        _timeout = CServerTime::GetCurrentTime().GetTimeRaw() - iDelayInMsecs;   // get the diff in msecs between the saved timeout and the current time
    }
    // if there's a timer already, clear it before setting the new one.
    else if (_timeout > 0)    // should not happen during Serv.Load()
    {
        g_World.DelTimedObject(_timeout, this);
    }
    CObjBase *pObj = dynamic_cast<CObjBase*>(this); // TODO: create a method on this class to create a good check for each type of object.
    if (pObj && pObj->IsDeleted()) //prevent deleted objects from setting new timers
    {
        return;
    }
    // Set delay to -1 = never timeout.
    if (iDelayInMsecs < 0)
    {
        _timeout = 0;
    }
    else
    {
        _timeout = g_World.GetCurrentTime().GetTimeRaw() + iDelayInMsecs;   // Setting the new Timeout value
        g_World.AddTimedObject(_timeout, this); // Adding this object to the tick's list.
    }
}


void CTimedObject::SetTimeoutS(int64 iSeconds)
{
    SetTimeout(iSeconds * MSECS_PER_SEC);
}

void CTimedObject::SetTimeoutT(int64 iTicks)
{
    SetTimeout(iTicks * MSECS_PER_TICK);
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

int64 CTimedObject::GetTimerTAdjusted() const
{
    // RETURN: time in ticks from now.
    if (!IsTimerSet())
        return -1;
    int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;
    return (iDiffInMsecs / TICKS_PER_SEC);
}

int64 CTimedObject::GetTimerDAdjusted() const
{
    // RETURN: time in tenths of second from now.
    if (!IsTimerSet())
        return -1;
    int64 iDiffInMsecs = GetTimerDiff();
    if (iDiffInMsecs < 0)
        return 0;
    return (iDiffInMsecs / TENTHS_PER_SEC);
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