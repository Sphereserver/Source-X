
#pragma once
#ifndef CTIME_H
#define CTIME_H

#include <time.h>
#include "common.h"

#ifndef _WIN32
	LONGLONG GetTickCount();
#endif


class CGTime	// similar to the MFC CTime and CTimeSpan or COleDateTime
{
	// Get time stamp in the real world. based on struct tm
#undef GetCurrentTime
private:
	time_t m_time;
public:
	static const char *m_sClassName;

	// Constructors
	static CGTime GetCurrentTime();

	CGTime();
	CGTime(time_t time);
	CGTime(const CGTime& timeSrc);

	CGTime( struct tm time );
	CGTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1);

	const CGTime& operator=(const CGTime& timeSrc);

	const CGTime& operator=(time_t t);

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
	LPCTSTR Format(LPCTSTR pszFormat) const;
	LPCTSTR FormatGmt(LPCTSTR pszFormat) const;

	// non CTime operations.
	bool Read( TCHAR * pVal );
	void Init();
	bool IsTimeValid() const;
	int GetDaysTotal() const;
};

#endif // CTIME_H
