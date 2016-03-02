#include "graysvr.h"


INT64 CServTime::GetTimeRaw() const
{
	if ( m_lPrivateTime < 0 )
		return 0;

	return m_lPrivateTime;
}
INT64 CServTime::GetTimeDiff( const CServTime & time ) const
{
	return( m_lPrivateTime - time.m_lPrivateTime );
}
void CServTime::Init()
{
	m_lPrivateTime = 0;
}
void CServTime::InitTime( INT64 lTimeBase )
{
	if ( lTimeBase < 0 )
		lTimeBase = 0;

	m_lPrivateTime = lTimeBase;
}
bool CServTime::IsTimeValid() const
{
	return( m_lPrivateTime > 0 ? true : false );
}
CServTime CServTime::operator+( INT64 iTimeDiff ) const
{
	CServTime time;
	time.m_lPrivateTime = m_lPrivateTime + iTimeDiff;
	if ( time.m_lPrivateTime < 0 )
		time.m_lPrivateTime = 0;

	return( time );
}
CServTime CServTime::operator-( INT64 iTimeDiff ) const
{
	CServTime time;
	time.m_lPrivateTime = m_lPrivateTime - iTimeDiff;
	if ( time.m_lPrivateTime < 0 )
		time.m_lPrivateTime = 0;

	return( time );
}
INT64 CServTime::operator-( CServTime time ) const
{
	return(m_lPrivateTime-time.m_lPrivateTime);
}
bool CServTime::operator==(CServTime time) const
{
	return(m_lPrivateTime==time.m_lPrivateTime);
}
bool CServTime::operator!=(CServTime time) const
{
	return(m_lPrivateTime!=time.m_lPrivateTime);
}
bool CServTime::operator<(CServTime time) const
{
	return(m_lPrivateTime<time.m_lPrivateTime);
}
bool CServTime::operator>(CServTime time) const
{
	return(m_lPrivateTime>time.m_lPrivateTime);
}
bool CServTime::operator<=(CServTime time) const
{
	return(m_lPrivateTime<=time.m_lPrivateTime);
}
bool CServTime::operator>=(CServTime time) const
{
	return(m_lPrivateTime>=time.m_lPrivateTime);
}
void CServTime::SetCurrentTime()
{
	m_lPrivateTime = GetCurrentTime().m_lPrivateTime;
}
static CServTime GetCurrentTime()
{
	return( g_World.GetCurrentTime());
}//inlined in CWorld.h
