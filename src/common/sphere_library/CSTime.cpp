//
// CSTime.cpp
//
// Replace the MFC CTime function. Must be usable with file system.
//

#include "../../sphere/threads.h"
#include "sstring.h"
#include "CSTime.h"
#include <cmath>
//#include <chrono>


// Windows epoch is January 1, 1601 (start of Gregorian calendar cycle)
// Unix epoch is January 1, 1970 (adjustment in "ticks" 100 nanosecond)
//#define UNIX_TICKS_PER_SECOND 10000000 //a tick is 100ns
#if _WIN32
//#   define UNIX_TIME_START 0x019DB1DED53E8000LL     // January 1, 1970 (start of Unix epoch) in "ticks"
//#   define WINDOWS_UNIX_EPOCH_OFFSET 11644473600    // (number of seconds between January 1, 1601 and January 1, 1970).
#endif

#ifdef _WIN32
//#   include <sysinfoapi.h>
#   if (defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0600))
	    // We don't have GetSupportedTickCount on Windows versions previous to Vista/Windows Server 2008. We need to check for overflows
	    //  (which occurs every 49.7 days of continuous running of the server, if measured with GetTickCount, every 7 years
	    //	with GetSupportedTickCount) manually every time we compare two values.
#       if MSVC_COMPILER
#           pragma warning(push)
#           pragma warning(disable: 28159)
#       endif
        // Precision should be in the order of 10-16 ms.
	    static inline llong GetSupportedTickCount() noexcept { return (llong)GetTickCount(); }
#   else
	    static inline llong GetSupportedTickCount() noexcept { return (llong)GetTickCount64(); }
#   endif
#   if MSVC_COMPILER
#       pragma warning(pop)
#   endif
#endif


//**************************************************************
// -CSTime - monotonic time

// More precision requires more CPU time!
llong CSTime::GetMonotonicSysTimeNano() noexcept // static
{
#ifdef _WIN32
    // From Windows documentation:
    //	On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
    // Since i think no one will run Sphere on a pre XP os, we can avoid checking for overflows, in case QueryPerformanceCounter fails.
    LARGE_INTEGER liQPCStart;
    if (!QueryPerformanceCounter(&liQPCStart))
        return GetSupportedTickCount() * 1000; // GetSupportedTickCount has only millisecond precision
    return (llong)((liQPCStart.QuadPart * 1.0e9) / _kllTimeProfileFrequency);

#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (llong)((ts.tv_sec * (llong)1.0e9) + (llong)ts.tv_nsec); // microseconds
#endif
}


// More precision requires more CPU time!
llong CSTime::GetMonotonicSysTimeMicro() noexcept // static
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
#   if   defined(CLOCK_MONOTONIC_RAW)
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#   else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#   endif
	return (llong)((ts.tv_sec * (llong)1.0e6) + (llong)round(ts.tv_nsec / 1.0e3)); // microseconds
#endif
}

llong CSTime::GetMonotonicSysTimeMilli() noexcept // static
{
#ifdef _WIN32
    // Max precision under Windows, but slowest...
	LARGE_INTEGER liQPCStart;
	if (!QueryPerformanceCounter(&liQPCStart))
		return GetSupportedTickCount();
	return (llong)((liQPCStart.QuadPart * 1.0e3) / _kllTimeProfileFrequency);

/*
    // Less precision, but faster (needs sysinfoapi.h).
    FILETIME ft;
//#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
//    GetSystemTimePreciseAsFileTime(&ft);  // This might actually be slower...
//#else
    GetSystemTimeAsFileTime(&ft);   // from sysinfoapi.h
    // GetSystemTimeAsFileTime might have a resolution between 55ms or 10ms, not good...
    // This blog post (https://devblogs.microsoft.com/oldnewthing/20170921-00/?p=97057) highlights the timeBeginPeriod function,
    //  it "should" make it work with a higher resolution, but at which cost? That should be eventually benchmarked, if we want to go this way...
//#endif

    // The suggested way to do it (but IntelliSense warns about li being uninitialized...)
    //ULARGE_INTEGER li;
    //li.LowPart = ft.dwLowDateTime;
    //li.HighPart = ft.dwHighDateTime;
    //unsigned long long valueAsHns = li.QuadPart;

    // The other way, which makes IntelliSense shut up.
    const unsigned long long valueAsHns = (unsigned long long)ft.dwLowDateTime | ((unsigned long long)ft.dwHighDateTime << 32u);
    const unsigned long long valueAsUs = valueAsHns/10;
    const unsigned long long valueAsMs = valueAsUs/1000;
    return valueAsMs;
*/

#else
	struct timespec ts;
#   if   defined(CLOCK_MONOTONIC_COARSE)
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
#   elif defined(CLOCK_MONOTONIC_RAW)
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#   else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#   endif
	return (llong)((ts.tv_sec * (llong)1.0e3) + (llong)round(ts.tv_nsec / 1.0e6)); // milliseconds
#endif
}


// Wall clock time

CSTime CSTime::GetCurrentTime()	noexcept // static
{
	// return the current system time
	return CSTime(::time(nullptr));
}

/*
CSTime::CSTime( struct tm atm ) noexcept
{
	m_time = mktime(&atm);
}
*/

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

static std::tm safe_localtime(const time_t t) noexcept
{
    std::tm atm {};

    // Standard C localtime is not thread-safe. We need alternatives.
    // https://stackoverflow.com/questions/38034033/c-localtime-this-function-or-variable-may-be-unsafe

#if defined(__unix__) || defined(__APPLE__) || defined(_POSIX_VERSION)
    localtime_r(&t, &atm);
#elif defined(MSVC_RUNTIME) || defined(__MINGW32__)
    localtime_s(&atm, &t);
#elif defined(__STDC_LIB_EXT1__)
    localtime_s(&t, &atm);
#else

    // To be tested! C++20 way.
    // https://stackoverflow.com/questions/61190884/current-time-and-date-in-c20-days
    static_assert(false, "Untested yet. If needed, test it and if it's fine delete this static_assert");

    using namespace std::chrono;
    const auto current_zoned_time = zoned_time{current_zone(), system_clock::from_time_t(t)};
    const auto local_time_point = current_zoned_time.get_local_time();
    //const auto local_duration = local_time_point.time_since_epoch();

    // Get a local time_point with days precision
    const auto ld = floor<days>(local_time_point);

    // Convert local days-precision time_point to a local {y, m, d} calendar
    const year_month_day ymd{ld};

    // Split time since local midnight into {h, m, s, subseconds}
    const hh_mm_ss hms{local_time_point - ld};

    // get_info(): Returns a std::chrono::time_zone::transition_info object, which provides information about whether DST is currently active.
    const bool is_dst = (current_zoned_time.get_info().save != std::chrono::seconds{0});

    atm.tm_sec  = (int)hms.seconds().count();
    atm.tm_min  = (int)hms.minutes().count();
    atm.tm_hour = (int)hms.hours().count();
    atm.tm_mday = (int)(uint)ymd.day();
    atm.tm_mon  = (int)(uint)ymd.month() - 1;
    atm.tm_year = (int)ymd.year() - 1900;
    atm.tm_isdst= (int)is_dst;
#endif

    return atm;
}

static std::tm safe_gmtime(const time_t t) noexcept
{
    std::tm atm {};

    // gmtime is in Coordinated Universal Time (UTC), while localtime is in your timezone
    // Standard C gmtime is not thread-safe. We need alternatives.

#if defined(__unix__) || defined(__APPLE__) || defined(_POSIX_VERSION)
    gmtime_r(&t, &atm);
#elif defined(MSVC_RUNTIME) || defined(__MINGW32__)
    gmtime_s(&atm, &t);
#elif defined(__STDC_LIB_EXT1__)
    gmtime_s(&t, &atm);
#else
    static_assert(false, "This platform doesn't look to have a thread-safe gmtime function?");
#endif

    return atm;
}

static std::tm safe_localtime_unoffset(std::tm atm) noexcept
{
    	atm.tm_year += 1900;
	atm.tm_mon += 1;
    return atm;
}

std::tm CSTime::GetLocalTm() const noexcept
{
	return safe_localtime(m_time);
}

std::tm CSTime::GetLocalTmPlain() const noexcept
{
	return safe_localtime_unoffset(safe_localtime(m_time));
}

////////////////////////////////////////////////////////////////////////////
// String formatting

#ifndef maxTimeBufferSize
	#define maxTimeBufferSize 128
#endif

#if defined(_WIN32) && defined (MSVC_COMPILER)
static void SPHERE_CDECL invalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, uint line, uintptr_t pReserved)
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
#ifdef MSVC_COMPILER
	// on windows we need to set the invalid parameter handler, or else the program will terminate when a bad format is encountered
    _invalid_parameter_handler newHandler, oldHandler;
	newHandler = static_cast<_invalid_parameter_handler>(invalidParameterHandler);
	oldHandler = _set_invalid_parameter_handler(newHandler);
#endif // MSVC_COMPILER
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

#ifdef MSVC_COMPILER
	// restore previous parameter handler
	_set_invalid_parameter_handler(oldHandler);
#endif // MSVC_COMPILER
#endif // _WIN32
}

lpctstr CSTime::Format(lpctstr pszFormat) const
{
	tchar * pszTemp = Str_GetTemp();

	if ( pszFormat == nullptr )
		pszFormat = "%Y/%m/%d %H:%M:%S";

	const std::tm ptmTemp = safe_localtime(m_time);

	FormatDateTime(pszTemp, pszFormat, &ptmTemp);
	return pszTemp;
}

lpctstr CSTime::FormatGmt(lpctstr pszFormat) const
{
	tchar * pszTemp = Str_GetTemp();
	if ( pszFormat == nullptr )
		pszFormat = "%a, %d %b %Y %H:%M:%S GMT";

	const std::tm ptmTemp = safe_gmtime(m_time);

	FormatDateTime(pszTemp, pszFormat, &ptmTemp);
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

CSTime::CSTime() noexcept :
    m_time(0)
{
}

CSTime::CSTime(time_t time) noexcept :
    m_time(time)
{
}

CSTime::CSTime(const CSTime& timeSrc) noexcept :
    m_time(timeSrc.m_time)
{
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
    //
    // TODO: Is this on Windows defined since January 1, 1601 ?
	return m_time;
}

int CSTime::GetYear() const noexcept
{
	return (GetLocalTm().tm_year) + 1900;
}

int CSTime::GetMonth() const noexcept       // month of year (1 = Jan)
{
	return GetLocalTm().tm_mon + 1;
}

int CSTime::GetDay() const noexcept         // day of month
{
	return GetLocalTm().tm_mday;
}

int CSTime::GetHour() const noexcept
{
	return GetLocalTm().tm_hour;
}

int CSTime::GetMinute() const noexcept
{
	return GetLocalTm().tm_min;
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
