/**
* @file CServerTime.h
*
*/

#ifndef _INC_CSERVTIME_H
#define _INC_CSERVTIME_H

#include "../common/datatypes.h"	// only the numeric data types


class CServerTime
{
	// A time stamp in the server/game world.

	#undef GetCurrentTime
	#define TICK_PER_SEC 10
	#define TENTHS_PER_SEC 1
	
public:
	static const char *m_sClassName;
	int64 m_llPrivateTime;
public:
	int64 GetTimeRaw() const;
	inline int64 GetTimeDiff( const CServerTime & time ) const;
	void Init();
	void InitTime( int64 lTimeBase );
	bool IsTimeValid() const;
	CServerTime operator+( int64 iTimeDiff ) const;
	CServerTime operator-( int64 iTimeDiff ) const;
	inline int64 operator-(CServerTime time ) const;
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
