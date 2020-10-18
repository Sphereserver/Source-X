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

// Similar to the MFC CTime and CTimeSpan or COleDateTime:
//  Get time stamp in the real world. based on struct tm
class CSTime
{
private:
#ifdef _WIN32
    #undef GetCurrentTime

	// Set once at server startup; used for Windows high-resolution timer
	friend class GlobalInitializer;
	static llong _kllTimeProfileFrequency;
#endif

	time_t m_time;

public:
	// Static methods
	static llong GetPreciseSysTimeMicro() noexcept;
	static llong GetPreciseSysTimeMilli() noexcept;

	static CSTime GetCurrentTime();

public:
	static const char* m_sClassName;

	// Constructors
	CSTime();
	CSTime(time_t time);
	CSTime(const CSTime& timeSrc);

	CSTime(struct tm time);
	CSTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1);

	const CSTime& operator=(const CSTime& timeSrc);
	const CSTime& operator=(time_t t);

	// Operators
	bool operator<=(time_t t) const;
	bool operator==(time_t t) const;
	bool operator!=(time_t t) const;

	// Attributes
	time_t GetTime() const;
	int GetYear() const;
	int GetMonth() const;
	int GetDay() const;
	int GetHour() const;
	int GetMinute() const;

	// Operations
	lpctstr Format(lpctstr pszFormat) const;
	lpctstr FormatGmt(lpctstr pszFormat) const;

	// non CTime operations.
	bool Read(tchar* pVal);
	void Init();
	bool IsTimeValid() const;
	int GetDaysTotal() const;

private:
	struct tm* GetLocalTm(struct tm* ptm = nullptr) const;
};

#endif // _INC_CTIME_H
