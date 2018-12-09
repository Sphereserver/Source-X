/**
* @file CCTimedObject.h
*
*/

#ifndef _INC_CCTIMEDOBJECT_H
#define _INC_CCTIMEDOBJECT_H

#include "../CComponent.h"
#include "../../sphere/ProfileData.h"

class CCTimedObject
{
private:
    THREAD_CMUTEX_DEF;
    int64 _timeout;
    PROFILE_TYPE _profileType;
    bool _fIsSleeping;

    /**
    * @fn virtual void CCTimedObject::ClearTimeout();
    *
    * @brief clears the timeout.
    *
    * Should not be used outside the tick's loop, use SetTimeout(0) instead.
    */
    friend class CWorld;
    virtual void ClearTimeout();

public:
    CCTimedObject(PROFILE_TYPE profile);
    virtual ~CCTimedObject();
    inline bool IsSleeping() const;
    inline virtual void GoSleep();
    virtual void GoAwake();
    /**
    * @fm PROFILE_TYPE CCTimedObject::GetProfileType();
    *
    * @brief returns the profiler type.
    *
    * @return the type.
    */
    inline PROFILE_TYPE GetProfileType() const;

    /**
     * @fn  virtual bool CCTimedObject::OnTick() = 0;
     *
     * @brief   Executes the tick action.
     *
     * @return  true if it succeeds, false if it fails.
     */
    virtual bool OnTick();

    /*
    * @fn virtual bool CCTimedObject::IsDeleted() const = 0;
    *
    * @brief Check if IsDeleted();
    *
    * @return true if it's deleted.
    */
    virtual bool IsDeleted() const = 0;

    /**
     * @fn  virtual void CCTimedObject::SetTimer( int64 iDelayInMsecs );
     *
     * @brief   &lt; Raw timer.
     *
     * @param   iDelayInMsecs   Zero-based index of the delay in milliseconds.
     */
    void SetTimer(int64 iDelayInMsecs);

    /**
     * @fn  virtual void CCTimedObject::SetTimeout( int64 iDelayInMsecs );
     *
     * @brief   &lt; Timer.
     *
     * @param   iDelayInMsecs   Zero-based index of the delay in milliseconds.
     */
    virtual void SetTimeout(int64 iDelayInMsecs);

    /**
    * @fn  virtual void CObjBase::SetTimeoutS( int64 iDelayInSecs );
    *
    * @brief   &lt; Timer.
    *
    * @param   iDelayInSecs   Zero-based index of the delay in seconds.
    */
    void SetTimeoutS(int64 iSeconds);

    /**
    * @fn  virtual void CObjBase::SetTimeoutT( int64 iDelayInTicks );
    *
    * @brief   &lt; Timer.
    *
    * @param   iDelayInTicks   Zero-based index of the delay in ticks.
    */
    void SetTimeoutT(int64 iTicks);

    /**
    * @fn  virtual void CObjBase::SetTimeoutD( int64 iDelayInTenths );
    *
    * @brief   &lt; Timer.
    *
    * @param   iDelayInTenths   Zero-based index of the delay in tenths of second.
    */
    void SetTimeoutD(int64 iTenths);

    /**
     * @fn  bool CObjBase::IsTimerSet() const;
     *
     * @brief   Query if this object is timer set.
     *
     * @return  true if timer set, false if not.
     */
    inline bool IsTimerSet() const;

    /**
     * @fn  int64 CObjBase::GetTimerDiff() const;
     *
     * @brief   Gets timer difference between current time and stored time.
     *
     * @return  The timer difference.
     */
    int64 GetTimerDiff() const;

    /**
     * @fn  bool CObjBase::IsTimerExpired() const;
     *
     * @brief   Query if this object is timer expired.
     *
     * @return  true if timer expired, false if not.
     */
    inline bool IsTimerExpired() const;

    /**
     * @fn  int64 CObjBase::GetTimerAdjusted() const;
     *
     * @brief   Gets timer.
     *
     * @return  The timer adjusted.
     */
    int64 GetTimerAdjusted() const;

    /**
     * @fn  int64 CObjBase::GetTimerTAdjusted() const;
     *
     * @brief   Gets timer in ticks.
     *
     * @return  The timer t adjusted.
     */
    int64 GetTimerTAdjusted() const;

    /**
    * @fn  int64 CObjBase::GetTimerDAdjusted() const;
    *
    * @brief    Gets timer in tenths of seconds.
    *
    * @return  The timer d adjusted.
    */
    int64 GetTimerDAdjusted() const;

    /**
    * @fn  int64 CObjBase::GetTimerSAdjusted() const;
    *
    * @brief    Gets timer in seconds.
    *
    * @return   The timer s adjusted.
    */
    int64 GetTimerSAdjusted() const;
};

bool CCTimedObject::IsSleeping() const
{
    return _fIsSleeping;
}

void CCTimedObject::GoSleep()
{
    _fIsSleeping = true;
}

bool CCTimedObject::IsTimerSet() const
{
    return _timeout > 0;
}

bool CCTimedObject::IsTimerExpired() const
{
    return (GetTimerDiff() <= 0);
}

PROFILE_TYPE CCTimedObject::GetProfileType() const
{
    return _profileType;
}

#endif //_INC_CCTIMEDOBJECT_H