#include "../common/CException.h"
#include "../sphere/threads.h"
#include "CWorldClock.h"
#include <ctime>
#include <chrono>

int64 CWorldClock::GetSystemClock() noexcept // static
{
	// Return system wall-clock using high resolution value (milliseconds)
	const auto timeMaxResolution = std::chrono::high_resolution_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(timeMaxResolution).count();
}

// --

void CWorldClock::InitTime(int64 iTimeBase)
{
	ADDTOCALLSTACK("CWorldClock::InitTime");
	_iSysClock_Prev = GetSystemClock();
	_timeClock.InitTime(iTimeBase + MSECS_PER_TICK);
	_iTickCur = 0;
}

void CWorldClock::Init()
{
	ADDTOCALLSTACK("CWorldClock::Init");
	_iSysClock_Prev = GetSystemClock();
	_timeClock = 0;
	_iTickCur = 0;
}

bool CWorldClock::Advance()
{
	ADDTOCALLSTACK("CWorldClock::Advance");
	const int64 iSysClock_Cur = GetSystemClock();
	const int64 iTimeDiff = iSysClock_Cur - _iSysClock_Prev;

	if (iTimeDiff == 0)
		return false;

	if (iTimeDiff < 0)
	{
		// System clock has changed forward
		// Just wait until next cycle and it should be ok
		g_Log.Event(LOGL_WARN, "System clock has changed forward (daylight saving change, etc). This may cause strange behavior on some objects.\n");
		_iSysClock_Prev = iSysClock_Cur;
		return false;
	}

	_iSysClock_Prev = iSysClock_Cur;
	const CServerTime timeClock_New = _timeClock + iTimeDiff;

	// CServerTime is signed (it's now int64)!
	// NOTE: This will overflow after 292 millions or so years of run time, good luck!
	if (timeClock_New < _timeClock)
	{
		g_Log.Event(LOGL_WARN, "System clock has changed backward (daylight saving change, etc). This may cause strange behavior on some objects.\n");
		_timeClock = timeClock_New;
		return false;
	}

	_timeClock = timeClock_New;
	// Maths here are done with MSECs precision, if proceed we advance a server's TICK.
	const CServerTime timeCur = GetCurrentTime();
	if (_timeNextTick <= timeCur)
	{
		_timeNextTick = timeCur + MSECS_PER_TICK;	// Next hit time.
		++_iTickCur;
	}
	return true;
}
