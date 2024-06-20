/**
* @file CServerTime.h
* @brief A time stamp in the server/game world, starting from server's Creation (not from the Epoch).
*/

#ifndef _INC_CSERVERTIME_H
#define _INC_CSERVERTIME_H

#include "../common/common.h"


struct CServerTime
{
	#define TICKS_PER_SEC   (int64)10      // Amount of ticks to advance in a second.

    #define TENTHS_PER_SEC  (int64)10      // Tenths in a second (backwards).
    #define MSECS_PER_SEC   (int64)1000    // Milliseconds in a second (to avoid magic numbers).

    #define MSECS_PER_TENTH (int64)100
    #define MSECS_PER_TICK  (int64)(MSECS_PER_SEC / TICKS_PER_SEC) // Milliseconds lapse between one server tick and another.


	static const char *m_sClassName;
	int64 m_llPrivateTime; // in milliseconds


    inline CServerTime() noexcept;
    inline CServerTime(int64 iTimeInMilliseconds) noexcept;

    inline void Init() noexcept;
    inline void InitTime(int64 iTimeBase) noexcept;
    inline bool IsTimeValid() const noexcept;

	/*
	* @brief Get the time value in milliseconds.
	*/
	inline int64 GetTimeRaw() const noexcept;

	CServerTime operator+(int64 iTimeDiff) const noexcept;
	CServerTime operator-(int64 iTimeDiff) const noexcept;
	inline int64 operator-(const CServerTime& time) const noexcept;
	inline bool operator==(const CServerTime& time) const noexcept;
	inline bool operator!=(const CServerTime& time) const noexcept;
	inline bool operator<(const CServerTime& time) const noexcept;
	inline bool operator>(const CServerTime& time) const noexcept;
	inline bool operator<=(const CServerTime& time) const noexcept;
	inline bool operator>=(const CServerTime& time) const noexcept;
    inline int64 GetTimeDiff(const CServerTime & time) const noexcept;

    static lpctstr GetTimeMinDesc(int iMinutes);
};


/* Inline Methods Definitions */

CServerTime::CServerTime() noexcept
{
    Init();
}
CServerTime::CServerTime(int64 iTimeInMilliseconds) noexcept
{
    InitTime(iTimeInMilliseconds);
}

int64 CServerTime::GetTimeRaw() const noexcept
{
    return (m_llPrivateTime < 0) ? 0 : m_llPrivateTime;
}

void CServerTime::Init() noexcept
{
    m_llPrivateTime = 0;
}

void CServerTime::InitTime(int64 llTimeBase) noexcept
{
    m_llPrivateTime = (llTimeBase < 0) ? 0 : llTimeBase;
}


bool CServerTime::IsTimeValid() const noexcept
{
    return bool(m_llPrivateTime > 0);
}

int64 CServerTime::GetTimeDiff(const CServerTime & time) const noexcept
{
    return ( m_llPrivateTime - time.m_llPrivateTime );
}

int64 CServerTime::operator-(const CServerTime& time) const noexcept
{
	return (m_llPrivateTime - time.m_llPrivateTime);
}
bool CServerTime::operator==(const CServerTime& time) const noexcept
{
	return (m_llPrivateTime == time.m_llPrivateTime);
}
bool CServerTime::operator!=(const CServerTime& time) const noexcept
{
	return (m_llPrivateTime != time.m_llPrivateTime);
}
bool CServerTime::operator<(const CServerTime& time) const noexcept
{
	return (m_llPrivateTime < time.m_llPrivateTime);
}
bool CServerTime::operator>(const CServerTime& time) const noexcept
{
	return (m_llPrivateTime > time.m_llPrivateTime);
}
bool CServerTime::operator<=(const CServerTime& time) const noexcept
{
	return (m_llPrivateTime <= time.m_llPrivateTime);
}
bool CServerTime::operator>=(const CServerTime& time) const noexcept
{
	return (m_llPrivateTime >= time.m_llPrivateTime);
}


#endif // _INC_CSERVERTIME_H
