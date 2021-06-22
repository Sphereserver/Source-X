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
	CSTime() noexcept;
	CSTime(time_t time) noexcept;
	CSTime(const CSTime& timeSrc) noexcept;

	CSTime(struct tm time) noexcept;
	CSTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1);

	const CSTime& operator=(const CSTime& timeSrc) noexcept;
	const CSTime& operator=(time_t t) noexcept;

	// Operators
	bool operator<=(time_t t) const noexcept;
	bool operator==(time_t t) const noexcept;
	bool operator!=(time_t t) const noexcept;

	// Attributes
	time_t GetTime() const noexcept;
	int GetYear() const noexcept;
	int GetMonth() const noexcept;
	int GetDay() const noexcept;
	int GetHour() const noexcept;
	int GetMinute() const noexcept;

	// Operations
	lpctstr Format(lpctstr pszFormat) const;
	lpctstr FormatGmt(lpctstr pszFormat) const;

	// non CTime operations.
	bool Read(tchar* pVal);
	void Init() noexcept;
	bool IsTimeValid() const noexcept;
	int GetDaysTotal() const noexcept;

private:
	struct tm* GetLocalTm(struct tm* ptm = nullptr) const noexcept;
};

#endif // _INC_CTIME_H
