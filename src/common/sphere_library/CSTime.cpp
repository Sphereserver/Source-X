//
// CSTime.cpp
//
// Replace the MFC CTime function. Must be usable with file system.
//

#include <cmath>
#include "CSTime.h"
#include "CSString.h"
#include "../../common/CLog.h"
#include "../../sphere/threads.h"


#ifdef _WIN32
    #if (defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0600))
	    // We don't have GetSupportedTickCount on Windows versions previous to Vista. We need to check for overflows
	    //  (which occurs every 49.7 days of continuous running of the server, if measured with GetTickCount, every 7 years
	    //	with GetSupportedTickCount) manually every time we compare two values.

        // Precision is in the order of 10-16 ms.
	    static inline llong GetSupportedTickCount() noexcept { return (llong)GetTickCount(); }
    #else
	    static inline llong GetSupportedTickCount() noexcept { return (llong)GetTickCount64(); }
    #endif
#endif


//**************************************************************
// -CSTime - absolute time

llong CSTime::GetPreciseSysTimeMicro() noexcept // static
{
#ifdef _WIN32
	// From Windows documentation:
	//	On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
	// Since i think no one will run Sphere on a pre XP os, we can avoid checking for overflows, in case QueryPerformanceCounter fails.
	LARGE_INTEGER liQPCStart;
	if (!QueryPerformanceCounter(&liQPCStart))
		return GetSupportedTickCount() * 1000; // GetSupportedTickCount has only millisecond precision
	return (llong)((liQPCStart.QuadPart * 1.0e6) / _kllTimeProfileFrequency);
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (llong)((ts.tv_sec * (llong)1.0e6) + (llong)round(ts.tv_nsec / 1.0e3)); // microseconds
#endif
}

llong CSTime::GetPreciseSysTimeMilli() noexcept // static
{
#ifdef _WIN32
	LARGE_INTEGER liQPCStart;
	if (!QueryPerformanceCounter(&liQPCStart))
		return GetSupportedTickCount();
	return (llong)((liQPCStart.QuadPart * 1.0e3) / _kllTimeProfileFrequency);
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (llong)((ts.tv_sec * (llong)1.0e3) + (llong)round(ts.tv_nsec / 1.0e6)); // milliseconds
#endif
}

CSTime CSTime::GetCurrentTime()	noexcept // static
{
	// return the current system time
	return CSTime(::time(nullptr));
}


CSTime::CSTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
			   int nDST) noexcept
{
	struct tm atm;
	atm.tm_sec = nSec;
	atm.tm_min = nMin;
	atm.tm_hour = nHour;
	atm.tm_mday = nDay;
	atm.tm_mon = nMonth - 1;        // tm_mon is 0 based
	atm.tm_year = nYear - 1900;     // tm_year is 1900 based
	atm.tm_isdst = nDST;
	m_time = mktime(&atm);
}

CSTime::CSTime( struct tm atm ) noexcept
{
	m_time = mktime(&atm);
}

struct tm* CSTime::GetLocalTm(struct tm* ptm) const noexcept
{
	if (ptm != nullptr)
	{
		struct tm* ptmTemp = localtime(&m_time);
		if (ptmTemp == nullptr)
			return nullptr;    // indicates the m_time was not initialized!

		*ptm = *ptmTemp;
		return ptm;
	}
	else
		return localtime(&m_time);
}

////////////////////////////////////////////////////////////////////////////
// String formatting

#ifndef maxTimeBufferSize
	#define maxTimeBufferSize 128
#endif

#if defined(_WIN32) && defined (_MSC_VER)
static void __cdecl invalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, uint line, uintptr_t pReserved)
{
	// bad format has been specified
	UnreferencedParameter(expression);
	UnreferencedParameter(function);
	UnreferencedParameter(file);
	UnreferencedParameter(line);
	UnreferencedParameter(pReserved);
	DEBUG_ERR(("Invalid time format specified.\n"));
}
#endif

static void FormatDateTime(tchar * pszTemp, lpctstr pszFormat, const struct tm * ptmTemp)
{
	ASSERT(pszTemp != nullptr);
	ASSERT(pszFormat != nullptr);
	ASSERT(ptmTemp != nullptr);

#ifdef _WIN32
#ifdef _MSC_VER
	// on windows we need to set the invalid parameter handler, or else the program will terminate when a bad format is encountered
    _invalid_parameter_handler newHandler, oldHandler;
	newHandler = static_cast<_invalid_parameter_handler>(invalidParameterHandler);
	oldHandler = _set_invalid_parameter_handler(newHandler);
#endif // _MSC_VER
	try
	{
#endif // _WIN32

	// only for ASCII strings
	if (strftime( pszTemp, maxTimeBufferSize, pszFormat, ptmTemp) == 0)
		pszTemp[0] = '\0';

#ifdef _WIN32
	}
	catch (...)
	{
		// since VS2010, it seems an exception gets thrown for invalid format strings too
		pszTemp[0] = '\0';
	}

#ifdef _MSC_VER
	// restore previous parameter handler
	_set_invalid_parameter_handler(oldHandler);
#endif // _MSC_VER
#endif // _WIN32
}

lpctstr CSTime::Format(lpctstr pszFormat) const
{
	tchar * pszTemp = Str_GetTemp();

	if ( pszFormat == nullptr )
		pszFormat = "%Y/%m/%d %H:%M:%S";

	struct tm* ptmTemp = localtime(&m_time);
	if (ptmTemp == nullptr )
	{
		pszTemp[0] = '\0';
		return( pszTemp );
	}

	FormatDateTime(pszTemp, pszFormat, ptmTemp);
	return pszTemp;
}

lpctstr CSTime::FormatGmt(lpctstr pszFormat) const
{
	tchar * pszTemp = Str_GetTemp();
	if ( pszFormat == nullptr )
		pszFormat = "%a, %d %b %Y %H:%M:%S GMT";

	struct tm* ptmTemp = gmtime(&m_time);
	if (ptmTemp == nullptr )
	{
		pszTemp[0] = '\0';
		return( pszTemp );
	}

	FormatDateTime(pszTemp, pszFormat, ptmTemp);
	return pszTemp;
}

//**************************************************************

bool CSTime::Read(tchar *pszVal)
{
	ADDTOCALLSTACK("CSTime::Read");
	// Read the full date format.

	tchar *ppCmds[10];
	size_t iQty = Str_ParseCmds( pszVal, ppCmds, ARRAY_COUNT(ppCmds), "/,: \t");
	if ( iQty < 6 )
		return false;

	struct tm atm;
	atm.tm_wday = 0;    // days since Sunday - [0,6]
	atm.tm_yday = 0;    // days since January 1 - [0,365]
	atm.tm_isdst = 0;   // daylight savings time flag

	// Saves: "1999/8/1 14:30:18"
	atm.tm_year = atoi(ppCmds[0]) - 1900;
	atm.tm_mon = atoi(ppCmds[1]) - 1;
	atm.tm_mday = atoi(ppCmds[2]);
	atm.tm_hour = atoi(ppCmds[3]);
	atm.tm_min = atoi(ppCmds[4]);
	atm.tm_sec = atoi(ppCmds[5]);
	m_time = mktime(&atm);

	return true;
}

CSTime::CSTime() noexcept
{
	m_time = 0;
}

CSTime::CSTime(time_t time) noexcept
{
	m_time = time;
}

CSTime::CSTime(const CSTime& timeSrc) noexcept
{
	m_time = timeSrc.m_time;
}

const CSTime& CSTime::operator=(const CSTime& timeSrc) noexcept
{
	m_time = timeSrc.m_time;
	return *this;
}

const CSTime& CSTime::operator=(time_t t) noexcept
{
	m_time = t;
	return *this;
}

bool CSTime::operator<=( time_t t ) const noexcept
{
	return( m_time <= t );
}

bool CSTime::operator==( time_t t ) const noexcept
{
	return( m_time == t );
}

bool CSTime::operator!=( time_t t ) const noexcept
{
	return( m_time != t );
}

time_t CSTime::GetTime() const noexcept
{
	// Although not defined by the C standard, this is almost always an integral value holding the number of seconds 
	//  (not counting leap seconds) since 00:00, Jan 1 1970 UTC, corresponding to UNIX time.
	return m_time;
}

int CSTime::GetYear() const noexcept
{
	return (GetLocalTm(nullptr)->tm_year) + 1900;
}

int CSTime::GetMonth() const noexcept       // month of year (1 = Jan)
{
	return GetLocalTm(nullptr)->tm_mon + 1;
}

int CSTime::GetDay() const noexcept         // day of month
{
	return GetLocalTm(nullptr)->tm_mday;
}

int CSTime::GetHour() const noexcept
{
	return GetLocalTm(nullptr)->tm_hour;
}

int CSTime::GetMinute() const noexcept
{
	return GetLocalTm(nullptr)->tm_min;
}

void CSTime::Init() noexcept
{
	m_time = -1;
}

bool CSTime::IsTimeValid() const noexcept
{
	return (( m_time && m_time != -1 ) ? true : false );
}

int CSTime::GetDaysTotal() const noexcept
{
	// Needs to be more consistant than accurate. just for compares.
	return (( GetYear() * 366) + (GetMonth()*31) + GetDay() );
}
