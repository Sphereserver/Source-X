/**
* @file CServerTime.h
*
*/

#ifndef _INC_CSERVTIME_H
#define _INC_CSERVTIME_H

#include "../common/datatypes.h"	// only the numeric data types


struct CServerTime
{
	// A time stamp in the server/game world, starting from server's Creation (not from the Epoch).

	#define TICK_PER_SEC    4   // how much ticks are in a second
    #define MSECS_PER_TICK     (1000 / TICK_PER_SEC) // how much milliseconds are between a tick and another

	static const char *m_sClassName;
	int64 m_llPrivateTime;

    inline CServerTime();
    inline CServerTime(int64 iTimeInMilliseconds);

    void Init();
    void InitTime( int64 iTimeBase );
    bool IsTimeValid() const;
	int64 GetTimeRaw() const;

	CServerTime operator+( int64 iTimeDiff ) const;
	CServerTime operator-( int64 iTimeDiff ) const;
	inline int64 operator-(CServerTime time ) const;
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
