/**
* @file CServerTime.h
*
*/

#pragma once
#ifndef _INC_CSERVTIME_H
#define _INC_CSERVTIME_H

#include "../common/spherecom.h"


class CServerTime
{
	// A time stamp in the server/game world.

	#undef GetCurrentTime
	#define TICK_PER_SEC 10
	#define TENTHS_PER_SEC 1
	
public:
	static const char *m_sClassName;
	int64 m_lPrivateTime;
public:
	int64 GetTimeRaw() const;
	int64 GetTimeDiff( const CServerTime & time ) const;
	void Init();
	void InitTime( int64 lTimeBase );
	bool IsTimeValid() const;
	CServerTime operator+( int64 iTimeDiff ) const;
	CServerTime operator-( int64 iTimeDiff ) const;
	inline int64 operator-( CServerTime time ) const;
	inline bool operator==(CServerTime time) const;
	inline bool operator!=(CServerTime time) const;
	inline bool operator<(CServerTime time) const;
	inline bool operator>(CServerTime time) const;
	inline bool operator<=(CServerTime time) const;
	inline bool operator>=(CServerTime time) const;
	inline void SetCurrentTime();
	static CServerTime GetCurrentTime();
};


/* Inline Methods Definitions */

int64 CServerTime::operator-(CServerTime time) const
{
	return (m_lPrivateTime - time.m_lPrivateTime);
}
bool CServerTime::operator==(CServerTime time) const
{
	return (m_lPrivateTime == time.m_lPrivateTime);
}
bool CServerTime::operator!=(CServerTime time) const
{
	return (m_lPrivateTime != time.m_lPrivateTime);
}
bool CServerTime::operator<(CServerTime time) const
{
	return (m_lPrivateTime<time.m_lPrivateTime);
}
bool CServerTime::operator>(CServerTime time) const
{
	return (m_lPrivateTime>time.m_lPrivateTime);
}
bool CServerTime::operator<=(CServerTime time) const
{
	return (m_lPrivateTime <= time.m_lPrivateTime);
}
bool CServerTime::operator>=(CServerTime time) const
{
	return (m_lPrivateTime >= time.m_lPrivateTime);
}
void CServerTime::SetCurrentTime()
{
	m_lPrivateTime = GetCurrentTime().m_lPrivateTime;
}


#endif // _INC_CSERVTIME_H
