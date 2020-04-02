#include "../common/CException.h"
#include "../sphere/threads.h"
#include "CWorldClock.h"
#include <ctime>
#include <chrono>

static int64 GetSystemClock()
{
	// Return system wall-clock using high resolution value (milliseconds)
	const auto timeMaxResolution = std::chrono::high_resolution_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(timeMaxResolution).count();
}

// --

void CWorldClock::InitTime(int64 iTimeBase)
{
	ADDTOCALLSTACK("CWorldClock::InitTime");
	m_Clock_SysPrev = GetSystemClock();
	m_timeClock.InitTime(iTimeBase + MSECS_PER_TICK);
	_iCurTick = 0;
}

void CWorldClock::Init()
{
	ADDTOCALLSTACK("CWorldClock::Init");
	m_Clock_SysPrev = GetSystemClock();
	m_timeClock = 0;
	_iCurTick = 0;
}

bool CWorldClock::Advance()
{
	ADDTOCALLSTACK("CWorldClock::Advance");
	const int64 Clock_Sys = GetSystemClock();
	const int64 iTimeDiff = Clock_Sys - m_Clock_SysPrev;
	if (iTimeDiff == 0)
		return false;
	if (iTimeDiff < 0)
	{
		// System clock has changed forward
		// Just wait until next cycle and it should be ok
		g_Log.Event(LOGL_WARN, "System clock has changed forward (daylight saving change, etc). This may cause strange behavior on some objects.\n");
		m_Clock_SysPrev = Clock_Sys;
		return false;
	}

	m_Clock_SysPrev = Clock_Sys;
	const CServerTime Clock_New = m_timeClock + iTimeDiff;

	// CServerTime is signed (it's now int64)!
	// NOTE: This will overflow after 292 millions or so years of run time, good luck!
	if (Clock_New < m_timeClock)
	{
		g_Log.Event(LOGL_WARN, "System clock has changed backward (daylight saving change, etc). This may cause strange behavior on some objects.\n");
		m_timeClock = Clock_New;
		return false;
	}

	m_timeClock = Clock_New;
	// Maths here are done with MSECs precision, if proceed we advance a server's TICK.
	const CServerTime curTime = GetCurrentTime();
	if (m_nextTickTime <= curTime)
	{
		m_nextTickTime = curTime + MSECS_PER_TICK;	// Next hit time.
		++_iCurTick;
	}
	return true;
}
