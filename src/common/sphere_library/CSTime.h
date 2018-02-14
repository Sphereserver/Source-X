/**
* @file CSTime.
* @brief Custom time management.
*/

#pragma once
#ifndef _INC_CSTIME_H
#define _INC_CSTIME_H

#include <time.h>
#include "../common.h"

#if !defined(_WIN32)
	llong GetTickCount64();
#elif (defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0600))
	// we don't have GetTickCount64 on older Windows versions, but since we use it only to calculate
	//  the difference between two near ticks, we won't overflow anyways
	inline llong GetTickCount64() { return (llong)GetTickCount(); }
#endif


class CSTime	// similar to the MFC CTime and CTimeSpan or COleDateTime
{
	// Get time stamp in the real world. based on struct tm
#undef GetCurrentTime
private:
	time_t m_time;
public:
	static const char *m_sClassName;

	// Constructors
	static CSTime GetCurrentTime();

	CSTime();
	CSTime(time_t time);
	CSTime(const CSTime& timeSrc);

	CSTime( struct tm time );
	CSTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1);

	const CSTime& operator=(const CSTime& timeSrc);

	const CSTime& operator=(time_t t);

	bool operator<=( time_t t ) const;
	bool operator==( time_t t ) const;
	bool operator!=( time_t t ) const;

	time_t GetTime() const;

	// Attributes
	struct tm* GetLocalTm(struct tm* ptm = NULL) const;

	int GetYear() const;
	int GetMonth() const;
	int GetDay() const;
	int GetHour() const;
	int GetMinute() const;

	// Operations
	lpctstr Format(lpctstr pszFormat) const;
	lpctstr FormatGmt(lpctstr pszFormat) const;

	// non CTime operations.
	bool Read( tchar * pVal );
	void Init();
	bool IsTimeValid() const;
	int GetDaysTotal() const;
};

#endif // _INC_CTIME_H
