/**
* @file CServerTime.h
*
*/

#ifndef _INC_CSERVTIME_H
#define _INC_CSERVTIME_H

#include "../common/datatypes.h"	// here we need only the numeric data types

/*
* A time stamp in the server/game world, starting from server's Creation (not from the Epoch).
* The time is advanced in each server's tick, so all the code executed in this tick will use
* the same time value, to adjust timers and so.
*/
struct CServerTime
{
	#define TICKS_PER_SEC   10      // Amount of ticks to advance in a second.
    #define TENTHS_PER_SEC  10      // Tenths in a second (backwards).
    #define MSECS_PER_SEC   1000    // Milliseconds in a second (to avoid magic numbers).

    #define MSECS_PER_TENTH (int64)100
    #define MSECS_PER_TICK  (int64)(MSECS_PER_SEC / TICKS_PER_SEC) // Milliseconds lapse between one tick and another.

	static const char *m_sClassName;
	int64 m_llPrivateTime;

    inline CServerTime();
    inline CServerTime(int64 iTimeInMilliseconds);

    void Init();
    void InitTime(int64 iTimeBase);
    bool IsTimeValid() const;
	int64 GetTimeRaw() const;

	CServerTime operator+(int64 iTimeDiff) const;
	CServerTime operator-(int64 iTimeDiff) const;
	inline int64 operator-(CServerTime time) const;
	inline bool operator==(CServerTime time) const;
	inline bool operator!=(CServerTime time) const;
	inline bool operator<(CServerTime time) const;
	inline bool operator>(CServerTime time) const;
	inline bool operator<=(CServerTime time) const;
	inline bool operator>=(CServerTime time) const;
    inline int64 GetTimeDiff( const CServerTime & time ) const;
	inline void SetCurrentTime();

    #undef GetCurrentTime
	static CServerTime GetCurrentTime();
};


/* Inline Methods Definitions */

CServerTime::CServerTime()
{
    Init();
}
CServerTime::CServerTime(int64 iTimeInMilliseconds)
{
    InitTime(iTimeInMilliseconds);
}

int64 CServerTime::GetTimeDiff( const CServerTime & time ) const
{
    return ( m_llPrivateTime - time.m_llPrivateTime );
}

int64 CServerTime::operator-(CServerTime time) const
{
	return (m_llPrivateTime - time.m_llPrivateTime);
}
bool CServerTime::operator==(CServerTime time) const
{
	return (m_llPrivateTime == time.m_llPrivateTime);
}
bool CServerTime::operator!=(CServerTime time) const
{
	return (m_llPrivateTime != time.m_llPrivateTime);
}
bool CServerTime::operator<(CServerTime time) const
{
	return (m_llPrivateTime < time.m_llPrivateTime);
}
bool CServerTime::operator>(CServerTime time) const
{
	return (m_llPrivateTime > time.m_llPrivateTime);
}
bool CServerTime::operator<=(CServerTime time) const
{
	return (m_llPrivateTime <= time.m_llPrivateTime);
}
bool CServerTime::operator>=(CServerTime time) const
{
	return (m_llPrivateTime >= time.m_llPrivateTime);
}
void CServerTime::SetCurrentTime()
{
	m_llPrivateTime = GetCurrentTime().m_llPrivateTime;
}


#endif // _INC_CSERVTIME_H
