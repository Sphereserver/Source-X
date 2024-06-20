/**
* @file ProfileData.h
*
*/

#ifndef _INC_PROFILEDATA_H
#define _INC_PROFILEDATA_H

#include "../common/common.h"

enum PROFILE_TYPE : uchar
{
	PROFILE_IDLE,		// Wait for stuff.
	PROFILE_OVERHEAD,	// In between stuff.
	PROFILE_NETWORK_RX,	// Just get client info and monitor new client requests. No processing.
	PROFILE_CLIENTS,	// Client processing.
	PROFILE_NETWORK_TX,	// sending network data
	PROFILE_CHARS,		// ticking characters
	PROFILE_ITEMS,		// ticking items
	PROFILE_MAP,		// reading map data
    PROFILE_MULTIS,     // Multi's stuff
	PROFILE_NPC_AI,		// processing npc ai
	PROFILE_SCRIPTS,	// running scripts
    PROFILE_SECTORS,    // sector stuff
    PROFILE_SHIPS,      // sips moving
    PROFILE_TIMEDFUNCTIONS, // TimerF
    PROFILE_TIMERS,
	PROFILE_TIME_QTY,

	// Qty of bytes. Not Time.
	PROFILE_DATA_TX = PROFILE_TIME_QTY, // network bytes sent
	PROFILE_DATA_RX,					// network bytes received
	PROFILE_DATA_QTY,

	PROFILE_STAT_FAULTS = PROFILE_DATA_QTY,	// exceptions raised

	PROFILE_QTY
};

class ProfileData
{
protected:
	struct ProfileDataRec
	{
		llong m_Time;	// accumulated time in msec.
		int m_iCount;	// how many passes made into this.
	};

	ProfileDataRec m_AverageTimes[PROFILE_QTY];
	ProfileDataRec m_PreviousTimes[PROFILE_QTY];
	ProfileDataRec m_CurrentTimes[PROFILE_QTY];
	bool m_EnabledProfiles[PROFILE_QTY];

	int m_iActiveWindowSeconds;	// The sample window size in seconds. 0=off
	int	m_iAverageCount;

	llong m_TimeTotal;			// Average this over a total time period.

	// Store the last time start time.
	PROFILE_TYPE  m_CurrentTask;	// What task are we currently processing ?
	llong m_CurrentTime;			// in milliseconds

    friend class ProfileTask;
    uint m_iMapTaskCounter;

public:
	ProfileData() noexcept;

	ProfileData(const ProfileData& copy) = delete;
	ProfileData& operator=(const ProfileData& other) = delete;

public:
	bool IsActive() const noexcept {
		return ( m_iActiveWindowSeconds > 0 ? true : false );
	}
    int GetActiveWindow() const noexcept;

	void SetActive(int iSampleSec);
	void Start(PROFILE_TYPE id);
	void Count(PROFILE_TYPE id, dword dwVal);
	void EnableProfile(PROFILE_TYPE id) noexcept;

	bool IsEnabled(PROFILE_TYPE id = PROFILE_QTY) const noexcept;
	PROFILE_TYPE GetCurrentTask() const noexcept;
	lpctstr GetName(PROFILE_TYPE id) const noexcept;
	lpctstr GetDescription(PROFILE_TYPE id) const;
};

#endif // _INC_PROFILEDATA_H
