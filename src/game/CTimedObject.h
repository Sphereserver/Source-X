/**
* @file CTimedObject.h
*
*/

#ifndef _INC_CTIMEDOBJECT_H
#define _INC_CTIMEDOBJECT_H

#include "../sphere/ProfileData.h"

class CComponent;
class CWorldTicker;

class CTimedObject
{
    friend class CComponent;
    friend class CWorldTicker;

public:
    static const char* m_sClassName;

protected:
    THREAD_CMUTEX_DEF;

private:
    int64 _iTimeout;
    PROFILE_TYPE _profileType;
    bool _fIsSleeping;

    /**
    * @brief clears the timeout.
    * Should not be used outside the tick's loop, use SetTimeout(0) instead.
    */
protected:  inline  void _ClearTimeout() noexcept;
public:             void  ClearTimeout() noexcept;

public:
    CTimedObject(PROFILE_TYPE profile);
    virtual ~CTimedObject();

protected:  inline  bool _IsSleeping() const noexcept;
public:             bool IsSleeping() const noexcept;

protected:  inline  virtual void _GoSleep();
public:             virtual void GoSleep();

protected:  virtual void _GoAwake();
public:     virtual void  GoAwake();

    /**
    * @brief returns the type of ticking object.
    * @return the type.
    */
protected:  inline PROFILE_TYPE _GetProfileType() const noexcept;
public:     PROFILE_TYPE GetProfileType() const noexcept;

    /**
     * @brief   Determine if the object is in a "tickable" state.
    */
protected:  virtual bool _CanTick() const;  // TODO: locks need to be extended to derived classes
public:     virtual bool  CanTick() const;

    /**
     * @brief   Executes the tick action.
     * @return  true if it succeeds, false if it fails.
    */
protected:  virtual bool _OnTick() = 0;
public:     virtual bool  OnTick(); // = 0;

    /*
    * @brief Check if IsDeleted();
    * @return true if it's deleted.
    */
protected:  virtual bool _IsDeleted() const = 0;
public:     virtual bool  IsDeleted() const = 0;

protected:
    /**
     * @brief   &lt; Gets raw Timeout.
     * @return  Delay in milliseconds.
     */
    inline  int64 _GetTimeoutRaw() const noexcept;

private:
    /**
     * @brief   &lt; Set raw Timeout.
     * @param   iDelayInMsecs   Delay in milliseconds.
     */
    inline  void _SetTimeoutRaw(int64 iDelayInMsecs) noexcept;

protected:  virtual void _SetTimeout(int64 iDelayInMsecs);
public:     virtual void  SetTimeout(int64 iDelayInMsecs);

    /**
    * @brief   &lt; Timer.
    * @param   iDelayInSecs   Delay in seconds.
    */
protected:  void _SetTimeoutS(int64 iSeconds);
public:     void  SetTimeoutS(int64 iSeconds);

    /**
    * @brief   &lt; Timer.
    * @param   iDelayInTenths   Delay in tenths of second.
    */
protected:  void _SetTimeoutD(int64 iTenths);
public:     void  SetTimeoutD(int64 iTenths);

    /**
     * @brief   Query if this object is timer set.
     * @return  true if timer set, false if not.
     */
protected:  inline  bool _IsTimerSet() const noexcept;
public:             bool IsTimerSet() const noexcept;

    /**
     * @brief   Gets timer difference between current time and stored time.
     * @return  The timer difference.
     */
private:
    int64 _GetTimerDiff() const noexcept;

    /**
     * @brief   Query if this object is timer expired.
     * @return  true if timer expired, false if not.
     */
protected:  inline  bool _IsTimerExpired() const noexcept;
public:             bool IsTimerExpired() const noexcept;

    /**
     * @brief   Gets timer (in milliseconds).
     * @return  The adjusted timer.
    
    */
protected:  int64 _GetTimerAdjusted() const noexcept;
public:     int64  GetTimerAdjusted() const noexcept;

    /**
    * @brief   Gets timer in tenths of seconds.
    * @return  The adjusted timer.
    */
protected:  int64 _GetTimerDAdjusted() const noexcept;
public:     int64  GetTimerDAdjusted() const noexcept;

    /**
    * @brief    Gets timer in seconds.
    * @return   The adjusted timer.
    */
protected:  int64 _GetTimerSAdjusted() const noexcept;
public:     int64  GetTimerSAdjusted() const noexcept;
};


/* Inlined methods are defined here */

int64 CTimedObject::_GetTimeoutRaw() const noexcept
{
    return _iTimeout;
}

void CTimedObject::_SetTimeoutRaw(int64 iDelayInMsecs) noexcept
{
    _iTimeout = iDelayInMsecs;
}

void CTimedObject::_ClearTimeout() noexcept
{
    _iTimeout = 0;
}

bool CTimedObject::_IsSleeping() const noexcept
{
    return _fIsSleeping;
}

void CTimedObject::_GoSleep()
{
    _fIsSleeping = true;
}

bool CTimedObject::_IsTimerSet() const noexcept
{
    return (_iTimeout > 0);
}

bool CTimedObject::_IsTimerExpired() const noexcept
{
    return (_GetTimerDiff() <= 0);
}

PROFILE_TYPE CTimedObject::_GetProfileType() const noexcept
{
    return _profileType;
}

#endif //_INC_CTIMEDOBJECT_H