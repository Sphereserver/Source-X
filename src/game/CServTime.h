
#pragma once
#ifndef _INC_CSERVTIME_H
#define _INC_CSERVTIME_H

#include "../common/graycom.h"


class CServTime
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
	int64 GetTimeDiff( const CServTime & time ) const;
	void Init();
	void InitTime( int64 lTimeBase );
	bool IsTimeValid() const;
	CServTime operator+( int64 iTimeDiff ) const;
	CServTime operator-( int64 iTimeDiff ) const;
	int64 operator-( CServTime time ) const;
	bool operator==(CServTime time) const;
	bool operator!=(CServTime time) const;
	bool operator<(CServTime time) const;
	bool operator>(CServTime time) const;
	bool operator<=(CServTime time) const;
	bool operator>=(CServTime time) const;
	void SetCurrentTime();
	static CServTime GetCurrentTime();
};


#endif // _INC_CSERVTIME_H
