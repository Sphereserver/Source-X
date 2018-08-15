
#include "CWorld.h"
#include "CServerTime.h"

int64 CServerTime::GetTimeRaw() const
{
	if ( m_llPrivateTime < 0 )
		return 0;

	return m_llPrivateTime;
}

void CServerTime::Init()
{
	m_llPrivateTime = 0;
}

void CServerTime::InitTime( int64 llTimeBase )
{
	if ( llTimeBase < 0 )
		llTimeBase = 0;

	m_llPrivateTime = llTimeBase;
}

bool CServerTime::IsTimeValid() const
{
	return ( m_llPrivateTime > 0 ? true : false );
}

CServerTime CServerTime::operator+( int64 llTimeDiff ) const
{
	CServerTime time;
	time.m_llPrivateTime = m_llPrivateTime + llTimeDiff;
	if ( time.m_llPrivateTime < 0 )
		time.m_llPrivateTime = 0;

	return time;
}

CServerTime CServerTime::operator-( int64 llTimeDiff ) const
{
	CServerTime time;
	time.m_llPrivateTime = m_llPrivateTime - llTimeDiff;
	if ( time.m_llPrivateTime < 0 )
		time.m_llPrivateTime = 0;

	return time;
}

CServerTime CServerTime::GetCurrentTime()	// static
{
    return g_World.GetCurrentTime();
}
