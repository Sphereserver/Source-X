/**
* @file CTimedObject.h
*
*/

#ifndef _INC_CTIMEDOBJECT_H
#define _INC_CTIMEDOBJECT_H

#include "../sphere/ProfileData.h"


class CTimedObject
{
    friend class CWorldTick;

private:
    THREAD_CMUTEX_DEF;
    int64 _timeout;
    PROFILE_TYPE _profileType;
    bool _fIsSleeping;

    /**
    * @brief clears the timeout.
    * Should not be used outside the tick's loop, use SetTimeout(0) instead.
    */
    virtual void ClearTimeout();

public:
    CTimedObject(PROFILE_TYPE profile);
    virtual ~CTimedObject();
    void Delete();

    inline bool IsSleeping() const;
    inline virtual void GoSleep();

    virtual void GoAwake();

    /**
    * @brief returns the profiler type.
    * @return the type.
    */
    inline PROFILE_TYPE GetProfileType() const;

    /**
     * @brief   Executes the tick action.
     * @return  true if it succeeds, false if it fails.
     */
    virtual bool OnTick();

    /*
    * @brief Check if IsDeleted();
    * @return true if it's deleted.
    */
    virtual bool IsDeleted() const = 0;

    /**
     * @brief   &lt; Raw timer.
     * @param   iDelayInMsecs   Zero-based index of the delay in milliseconds.
     */
    void SetTimer(int64 iDelayInMsecs);

    /**
     * @brief   &lt; Timer.
     * @param   iDelayInMsecs   Zero-based index of the delay in milliseconds.
     */
    virtual void SetTimeout(int64 iDelayInMsecs);

    /**
    * @brief   &lt; Timer.
    * @param   iDelayInSecs   Zero-based index of the delay in seconds.
    */
    void SetTimeoutS(int64 iSeconds);

    /**
    * @brief   &lt; Timer.
    * @param   iDelayInTenths   Zero-based index of the delay in tenths of second.
    */
    void SetTimeoutD(int64 iTenths);

    /**
     * @brief   Query if this object is timer set.
     * @return  true if timer set, false if not.
     */
    inline bool IsTimerSet() const;

    /**
     * @brief   Gets timer difference between current time and stored time.
     * @return  The timer difference.
     */
    int64 GetTimerDiff() const;

    /**
     * @brief   Query if this object is timer expired.
     * @return  true if timer expired, false if not.
     */
    inline bool IsTimerExpired() const;

    /**
     * @brief   Gets timer (in milliseconds).
     * @return  The adjusted timer.
     */
    int64 GetTimerAdjusted() const;

    /**
     * @brief   Gets timer in ticks.
     * @return  The adjusted timer.
     */
    //int64 GetTimerTAdjusted() const;

    /**
    * @brief   Gets timer in tenths of seconds.
    * @return  The adjusted timer.
    */
    int64 GetTimerDAdjusted() const;

    /**
    * @brief    Gets timer in seconds.
    * @return   The adjusted timer.
    */
    int64 GetTimerSAdjusted() const;
};


/* Inlined methods are defined here */

bool CTimedObject::IsSleeping() const
{
    return _fIsSleeping;
}

void CTimedObject::GoSleep()
{
    _fIsSleeping = true;
}

bool CTimedObject::IsTimerSet() const
{
    return _timeout > 0;
}

bool CTimedObject::IsTimerExpired() const
{
    return (GetTimerDiff() <= 0);
}

PROFILE_TYPE CTimedObject::GetProfileType() const
{
    return _profileType;
}

#endif //_INC_CTIMEDOBJECT_H