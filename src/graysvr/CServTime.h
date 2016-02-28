#ifndef _INC_CSERVTIME_H
#define _INC_CSERVTIME_H

#include "../common/graycom.h"
class CServTime
{
#undef GetCurrentTime
#define TICK_PER_SEC 10
#define TENTHS_PER_SEC 1
	// A time stamp in the server/game world.
public:
	static const char *m_sClassName;
	INT64 m_lPrivateTime;
public:
	INT64 GetTimeRaw() const
	{
		if ( m_lPrivateTime < 0 )
			return 0;

		return m_lPrivateTime;
	}
	INT64 GetTimeDiff( const CServTime & time ) const
	{
		return( m_lPrivateTime - time.m_lPrivateTime );
	}
	void Init()
	{
		m_lPrivateTime = 0;
	}
	void InitTime( INT64 lTimeBase )
	{
		if ( lTimeBase < 0 )
			lTimeBase = 0;

		m_lPrivateTime = lTimeBase;
	}
	bool IsTimeValid() const
	{
		return( m_lPrivateTime > 0 ? true : false );
	}
	CServTime operator+( INT64 iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime + iTimeDiff;
		if ( time.m_lPrivateTime < 0 )
			time.m_lPrivateTime = 0;

		return( time );
	}
	CServTime operator-( INT64 iTimeDiff ) const
	{
		CServTime time;
		time.m_lPrivateTime = m_lPrivateTime - iTimeDiff;
		if ( time.m_lPrivateTime < 0 )
			time.m_lPrivateTime = 0;

		return( time );
	}
	INT64 operator-( CServTime time ) const
	{
		return(m_lPrivateTime-time.m_lPrivateTime);
	}
	bool operator==(CServTime time) const
	{
		return(m_lPrivateTime==time.m_lPrivateTime);
	}
	bool operator!=(CServTime time) const
	{
		return(m_lPrivateTime!=time.m_lPrivateTime);
	}
	bool operator<(CServTime time) const
	{
		return(m_lPrivateTime<time.m_lPrivateTime);
	}
	bool operator>(CServTime time) const
	{
		return(m_lPrivateTime>time.m_lPrivateTime);
	}
	bool operator<=(CServTime time) const
	{
		return(m_lPrivateTime<=time.m_lPrivateTime);
	}
	bool operator>=(CServTime time) const
	{
		return(m_lPrivateTime>=time.m_lPrivateTime);
	}
	void SetCurrentTime()
	{
		m_lPrivateTime = GetCurrentTime().m_lPrivateTime;
	}
	static CServTime GetCurrentTime();
};
#endif // _INC_CSERVTIME_H
