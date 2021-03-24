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
	int64 _iTickCur;            // Current TICK count of the server from its first start.
	CServerTime _timeClock;     // SERVER TIME on the current game loop cycle (CWorld::_OnTick method), used to advance the ticks.
	int64 _iSysClock_Prev;	    // REAL WORLD TIME (in milliseconds) of the last game loop cycle.
	CServerTime	_timeNextTick;	// SERVER TIME we'll run the next tick on (to do sector and other stuff).

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

	inline void AdvanceTick() noexcept
	{
		++_iTickCur;
	}

#undef GetCurrentTime
	inline const CServerTime& GetCurrentTime() const noexcept // in milliseconds
	{
		return _timeClock;
	}

	inline int64 GetCurrentTick() const noexcept
	{
		return _iTickCur;
	}

private:
	// To update the internal system clock after a WorldSave or Resync
	friend class CWorld;

	static int64 GetSystemClock() noexcept;
};

#endif // _INC_CWORLDCLOCK_H
