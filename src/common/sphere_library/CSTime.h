/**
* @file CSString.
* @brief Custom time management.
*/

#pragma once
#ifndef _INC_CSTIME_H
#define _INC_CSTIME_H

#include <time.h>
#include "../common.h"

#ifndef _WIN32
	llong GetTickCount();
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
