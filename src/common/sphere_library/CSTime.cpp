//
// CSime.cpp
//
// Replace the MFC CTime function. Must be usable with file system.
//

#include "CSTime.h"
#include "CSString.h"
#include "../../common/CLog.h"
#include "../../sphere/threads.h"


llong GetPreciseSysTime()	// 64 bits tick count
{
#ifdef _WIN32
    // From Windows documentation:
    //	On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
    // Since i think no one will run Sphere on a pre XP os, we can avoid checking for overflows, in case QueryPerformanceCounter fails.
    LARGE_INTEGER liQPCStart;
    if (!QueryPerformanceCounter(&liQPCStart))
        return GetSupportedTickCount();
    return liQPCStart.QuadPart;
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	// return (llong)((ts.tv_sec * (llong)1.0e6) + (llong)round(ts.tv_nsec / 1.0e6)); // milliseconds
    return (llong)((ts.tv_sec * (llong)1.0e9) + ts.tv_nsec); // nanoseconds
#endif
}


//**************************************************************
// -CSTime - absolute time

CSTime::CSTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
			   int nDST)
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

CSTime::CSTime( struct tm atm )
{
	m_time = mktime(&atm);
}

CSTime CSTime::GetCurrentTime()	// static
{
	// return the current system time
	return CSTime(::time(nullptr));
}

struct tm* CSTime::GetLocalTm(struct tm* ptm) const
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

#ifdef _WIN32
void __cdecl invalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, uint line, uintptr_t pReserved)
{
	// bad format has been specified
	UNREFERENCED_PARAMETER(expression);
	UNREFERENCED_PARAMETER(function);
	UNREFERENCED_PARAMETER(file);
	UNREFERENCED_PARAMETER(line);
	UNREFERENCED_PARAMETER(pReserved);
	DEBUG_ERR(("Invalid time format specified.\n"));
}
#endif

void FormatDateTime(tchar * pszTemp, lpctstr pszFormat, const struct tm * ptmTemp)
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
	size_t iQty = Str_ParseCmds( pszVal, ppCmds, CountOf(ppCmds), "/,: \t");
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

CSTime::CSTime()
{
	m_time = 0;
}

CSTime::CSTime(time_t time)
{
	m_time = time;
}

CSTime::CSTime(const CSTime& timeSrc)
{
	m_time = timeSrc.m_time;
}

const CSTime& CSTime::operator=(const CSTime& timeSrc)
{
	m_time = timeSrc.m_time;
	return *this;
}

const CSTime& CSTime::operator=(time_t t)
{
	m_time = t;
	return *this;
}

bool CSTime::operator<=( time_t t ) const
{
	return( m_time <= t );
}

bool CSTime::operator==( time_t t ) const
{
	return( m_time == t );
}

bool CSTime::operator!=( time_t t ) const
{
	return( m_time != t );
}

time_t CSTime::GetTime() const
{
	return m_time;
}

int CSTime::GetYear() const
{
	return (GetLocalTm(nullptr)->tm_year) + 1900;
}

int CSTime::GetMonth() const       // month of year (1 = Jan)
{
	return GetLocalTm(nullptr)->tm_mon + 1;
}

int CSTime::GetDay() const         // day of month
{
	return GetLocalTm(nullptr)->tm_mday;
}

int CSTime::GetHour() const
{
	return GetLocalTm(nullptr)->tm_hour;
}

int CSTime::GetMinute() const
{
	return GetLocalTm(nullptr)->tm_min;
}

void CSTime::Init()
{
	m_time = -1;
}

bool CSTime::IsTimeValid() const
{
	return (( m_time && m_time != -1 ) ? true : false );
}

int CSTime::GetDaysTotal() const
{
	// Needs to be more consistant than accurate. just for compares.
	return (( GetYear() * 366) + (GetMonth()*31) + GetDay() );
}
