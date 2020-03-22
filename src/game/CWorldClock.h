/**
* @file CWorldClock.h
* @brief The time is advanced in each server's tick, so all the code executed in this tick will use the same time value, to adjust timers and so.
*/

#ifndef _INC_CWORLDCLOCK_H
#define _INC_CWORLDCLOCK_H

#include "CServerTime.h"

class CWorldClock
{
private:
	int64 _iCurTick;            // Current TICK count of the server from it's first start.
	CServerTime m_timeClock;    // Internal clock record, on msecs, used to advance the ticks.
	int64 m_Clock_SysPrev;	    // SERVER time (in milliseconds) of the last OnTick()
	CServerTime	m_nextTickTime;	// next time to do sector stuff.

public:
	static const char* m_sClassName;
	CWorldClock()
	{
		Init();
	}

private:
	CWorldClock(const CWorldClock& copy);
	CWorldClock& operator=(const CWorldClock& other);

public:
	void Init();
	void InitTime(int64 iTimeBase);
	bool Advance();
	inline void AdvanceTick()
	{
		++_iCurTick;
	}
	inline CServerTime GetCurrentTime() const // in milliseconds
	{
		return m_timeClock;
	}
	inline int64 GetCurrentTick() const
	{
		return _iCurTick;
	}
};

#endif // _INC_CWORLDCLOCK_H
