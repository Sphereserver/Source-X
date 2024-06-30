#include <cstdio>
#include "../common/sphere_library/CSString.h"
#include "../game/CScriptProfiler.h"
#include "ProfileData.h"
#include "threads.h"

ProfileData::ProfileData() noexcept
{
	// we don't want to use SetActive here because ADDTOCALLSTACK will cause an infinite loop

	memset(m_AverageTimes, 0, sizeof(m_AverageTimes));
	memset(m_CurrentTimes, 0, sizeof(m_CurrentTimes));
	memset(m_PreviousTimes, 0, sizeof(m_PreviousTimes));
	memset(m_EnabledProfiles, 0, sizeof(m_EnabledProfiles));

	m_iActiveWindowSeconds = 10 * 1000; // expressed in milliseconds
	m_iAverageCount = 1;

	llong llTicksStart;
	TIME_PROFILE_START;

	m_CurrentTime = llTicksStart;
	m_CurrentTask = PROFILE_IDLE;
	m_TimeTotal = 0;
    m_iMapTaskCounter = 0;
}

int ProfileData::GetActiveWindow() const noexcept
{
    return m_iActiveWindowSeconds / 1000;
}

void ProfileData::SetActive(int iSampleSec)
{
	ADDTOCALLSTACK("ProfileData::SetActive");

	memset(m_AverageTimes, 0, sizeof(m_AverageTimes));
	memset(m_CurrentTimes, 0, sizeof(m_CurrentTimes));
	memset(m_PreviousTimes, 0, sizeof(m_PreviousTimes));

	m_iActiveWindowSeconds = iSampleSec * 1000; // expressed in milliseconds;
	m_iAverageCount		= 1;

	if (m_iActiveWindowSeconds == 0)
		return;

	llong llTicksStart;
	TIME_PROFILE_START;

	m_CurrentTime = llTicksStart;
	m_CurrentTask = PROFILE_OVERHEAD;
	m_TimeTotal = 0;
}

void ProfileData::Start(PROFILE_TYPE id)
{
	// ADDTOCALLSTACK("ProfileData::Start"); // CPU intensive
	if (( id >= PROFILE_TIME_QTY ) || !m_iActiveWindowSeconds )
		return;

	// ensure profile is enabled
	EnableProfile(id);

	// Stop prev task.
	if ( m_TimeTotal >= m_iActiveWindowSeconds )
	{
		for ( int i = 0; i < PROFILE_DATA_QTY; ++i )
		{
			if ( m_iAverageCount < 4 )
			{
				m_AverageTimes[i].m_Time = m_CurrentTimes[i].m_Time;
				m_AverageTimes[i].m_iCount = m_CurrentTimes[i].m_iCount;
			}
			else
			{
				m_AverageTimes[i].m_Time	= (((m_AverageTimes[i].m_Time * 90) + (m_PreviousTimes[i].m_Time * 10)) / 100);
				m_AverageTimes[i].m_iCount	= (((m_AverageTimes[i].m_iCount * 95) + (m_PreviousTimes[i].m_iCount * 10)) / 100);
			}
		}

		for ( int i = PROFILE_DATA_QTY; i < PROFILE_QTY; ++i )
		{
			m_AverageTimes[i].m_Time	+= m_CurrentTimes[i].m_Time;
			m_AverageTimes[i].m_iCount	+= m_CurrentTimes[i].m_iCount;
		}

		++m_iAverageCount;

		memcpy( m_PreviousTimes, m_CurrentTimes, sizeof( m_PreviousTimes ));
		memset( m_CurrentTimes, 0, sizeof( m_CurrentTimes ));
		m_TimeTotal = 0;
	}

	// Get the current precise time.
	llong llTicksStart;
	TIME_PROFILE_START;

	// accumulate the time for this task.
	llong llDiff = ( llTicksStart - m_CurrentTime );
    if ((llDiff < 0) && (llDiff > -10)) {
        // The timer might not be very very precise.
        llDiff = 0;
    }
    else {
        ASSERT(llDiff >= 0);
    }

    //if (m_CurrentTask == PROFILE_MAP) {
    //    llDiff *= 10;
    //}

	m_TimeTotal += llDiff;
    ASSERT(m_TimeTotal >= 0);
	m_CurrentTimes[m_CurrentTask].m_Time += llDiff;
	++ m_CurrentTimes[m_CurrentTask].m_iCount;

	// We are now on to the new task.
	m_CurrentTime = llTicksStart;
	m_CurrentTask = id;
}

void ProfileData::Count(PROFILE_TYPE id, dword dwVal)
{
	ADDTOCALLSTACK("ProfileData::Count");

	ASSERT( id >= PROFILE_TIME_QTY && id < PROFILE_QTY );
	m_CurrentTimes[id].m_Time += dwVal;
	++ m_CurrentTimes[id].m_iCount;
}

void ProfileData::EnableProfile(PROFILE_TYPE id) noexcept
{
	if (id >= PROFILE_QTY)
		return;

	m_EnabledProfiles[id] = true;
}

bool ProfileData::IsEnabled(PROFILE_TYPE id) const noexcept
{
	if (id > PROFILE_QTY)
		return false;

	if (id < PROFILE_QTY)
		return m_EnabledProfiles[id];

	// check all profiles
	for (int i = PROFILE_OVERHEAD; i < PROFILE_QTY; ++i)
	{
		if (IsEnabled( PROFILE_TYPE(i) ))
			return true;
	}

	return false;
}

PROFILE_TYPE ProfileData::GetCurrentTask() const noexcept
{
	return m_CurrentTask;
}

lpctstr ProfileData::GetName(PROFILE_TYPE id) const noexcept
{
	static lpctstr constexpr sm_pszProfileName[PROFILE_QTY] =
	{
		"IDLE",
		"OVERHEAD",
		"NETWORK_RX",
		"CLIENTS",
		"NETWORK_TX",
		"CHARS",
		"ITEMS",
		"MAP",
        "MULTIS",
		"NPC_AI",
		"SCRIPTS",
        "SECTORS",
        "SHIPS",
        "TIMEDFUNCTIONS",
        "TIMERS",
		"DATA_TX",
		"DATA_RX",
		"FAULTS"
	};

	return (id < PROFILE_QTY) ? sm_pszProfileName[id] : "";
}

lpctstr ProfileData::GetDescription(PROFILE_TYPE id) const
{
	ADDTOCALLSTACK("ProfileData::GetDesc");
	ASSERT(id < PROFILE_QTY);
	const int iCount = m_PreviousTimes[id].m_iCount;
	tchar* ptcTmp = Str_GetTemp();

	if ( id >= PROFILE_DATA_QTY )
	{
		snprintf(ptcTmp, Str_TempLength(), "%lld (total: %lld) instances", m_PreviousTimes[id].m_Time, m_AverageTimes[id].m_Time);
	}
	else if ( id >= PROFILE_TIME_QTY )
	{
		snprintf(ptcTmp, Str_TempLength(), "%lld (avg: %lld) bytes", m_PreviousTimes[id].m_Time, m_AverageTimes[id].m_Time);
	}
	else
	{
		snprintf(ptcTmp, Str_TempLength(), "%.4fs  avg: %.4fs  [samples:%8i  avg:%7i]  adjusted runtime: %is",
			(m_PreviousTimes[id].m_Time    / 1000.0),
			(m_AverageTimes[id].m_Time     / 1000.0),
			iCount,
			m_AverageTimes[id].m_iCount,
			m_iAverageCount );
	}

	return ptcTmp;
}
