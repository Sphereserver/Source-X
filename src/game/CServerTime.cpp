
#include "CObjBase.h"
#include "CServerTime.h"
#include "CWorld.h"


int64 CServerTime::GetTimeRaw() const
{
	if ( m_lPrivateTime < 0 )
		return 0;

	return m_lPrivateTime;
}

int64 CServerTime::GetTimeDiff( const CServerTime & time ) const
{
	return ( m_lPrivateTime - time.m_lPrivateTime );
}

void CServerTime::Init()
{
	m_lPrivateTime = 0;
}

void CServerTime::InitTime( int64 lTimeBase )
{
	if ( lTimeBase < 0 )
		lTimeBase = 0;

	m_lPrivateTime = lTimeBase;
}

bool CServerTime::IsTimeValid() const
{
	return ( m_lPrivateTime > 0 ? true : false );
}

CServerTime CServerTime::operator+( int64 iTimeDiff ) const
{
	CServerTime time;
	time.m_lPrivateTime = m_lPrivateTime + iTimeDiff;
	if ( time.m_lPrivateTime < 0 )
		time.m_lPrivateTime = 0;

	return time;
}

CServerTime CServerTime::operator-( int64 iTimeDiff ) const
{
	CServerTime time;
	time.m_lPrivateTime = m_lPrivateTime - iTimeDiff;
	if ( time.m_lPrivateTime < 0 )
		time.m_lPrivateTime = 0;

	return time;
}

static CServerTime GetCurrentTime()
{
	return g_World.GetCurrentTime();
}//inlined in CWorld.h
