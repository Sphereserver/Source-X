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
	INT64 GetTimeRaw() const;
	INT64 GetTimeDiff( const CServTime & time ) const;
	void Init();
	void InitTime( INT64 lTimeBase );
	bool IsTimeValid() const;
	CServTime operator+( INT64 iTimeDiff ) const;
	CServTime operator-( INT64 iTimeDiff ) const;
	INT64 operator-( CServTime time ) const;
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
