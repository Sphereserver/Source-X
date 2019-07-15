/**
* @file CSTime.
* @brief Custom time management.
*/

#ifndef _INC_CSTIME_H
#define _INC_CSTIME_H

#ifdef _WIN32
    #include <time.h>
#else
    #include <sys/time.h>
#endif
#include "../common.h"


#ifdef _WIN32
    #if (defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0600))
	    // We don't have GetSupportedTickCount on Windows versions previous to Vista. We need to check for overflows
	    //  (which occurs every 49.7 days of continuous running of the server, if measured with GetTickCount, every 7 years
	    //	with GetSupportedTickCount) manually every time we compare two values.
	    inline llong GetSupportedTickCount() { return (llong)GetTickCount(); }
    #else
	    inline llong GetSupportedTickCount() { return (llong)GetTickCount64(); }
    #endif
#endif

llong GetPreciseSysTime();


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
	struct tm* GetLocalTm(struct tm* ptm = nullptr) const;

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
