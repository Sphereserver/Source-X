//
// Ctime.cpp
//
// Replace the MFC CTime function. Must be usable with file system.
//

#include "../game/CLog.h"
#include "../sphere/threads.h"
#include "./sphere_library/CString.h"
#include "CTime.h"
#include "spherecom.h"

#ifndef _WIN32
#include <sys/time.h>

llong GetTickCount()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (llong) (((llong) tv.tv_sec * 1000) + ((llong) tv.tv_usec/1000));
}
#endif


//**************************************************************
// -CGTime - absolute time

CGTime::CGTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
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

CGTime::CGTime( struct tm atm )
{
	m_time = mktime(&atm);
}

CGTime CGTime::GetCurrentTime()	// static
{
	// return the current system time
	return CGTime(::time(NULL));
}

struct tm* CGTime::GetLocalTm(struct tm* ptm) const
{
	if (ptm != NULL)
	{
		struct tm* ptmTemp = localtime(&m_time);
		if (ptmTemp == NULL)
			return NULL;    // indicates the m_time was not initialized!

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
	ASSERT(pszTemp != NULL);
	ASSERT(pszFormat != NULL);
	ASSERT(ptmTemp != NULL);

#ifdef _WIN32
	// on windows we need to set the invalid parameter handler, or else the program will terminate when a bad format is encountered
	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = static_cast<_invalid_parameter_handler>(invalidParameterHandler);
#ifndef __MINGW32__
	oldHandler = _set_invalid_parameter_handler(newHandler);
#endif  // __MINGW32__
	try
	{
#endif

	if (strftime( pszTemp, maxTimeBufferSize, pszFormat, ptmTemp) == 0)
		pszTemp[0] = '\0';

#ifdef _WIN32
	}
	catch (...)
	{
		// since VS2010, it seems an exception gets thrown for invalid format strings too
		pszTemp[0] = '\0';
	}

	// restore previous parameter handler

#ifndef __MINGW32__
	_set_invalid_parameter_handler(oldHandler);
#endif  // __MINGW32__
#endif
}

lpctstr CGTime::Format(lpctstr pszFormat) const
{
	tchar * pszTemp = Str_GetTemp();

	if ( pszFormat == NULL )
	{
		pszFormat = "%Y/%m/%d %H:%M:%S";
	}

	struct tm* ptmTemp = localtime(&m_time);
	if (ptmTemp == NULL )
	{
		pszTemp[0] = '\0';
		return( pszTemp );
	}

	FormatDateTime(pszTemp, pszFormat, ptmTemp);
	return pszTemp;
}

lpctstr CGTime::FormatGmt(lpctstr pszFormat) const
{
	tchar * pszTemp = Str_GetTemp();
	if ( pszFormat == NULL )
	{
		pszFormat = "%a, %d %b %Y %H:%M:%S GMT";
	}

	struct tm* ptmTemp = gmtime(&m_time);
	if (ptmTemp == NULL )
	{
		pszTemp[0] = '\0';
		return( pszTemp );
	}

	FormatDateTime(pszTemp, pszFormat, ptmTemp);
	return pszTemp;
}

//**************************************************************

bool CGTime::Read(tchar *pszVal)
{
	ADDTOCALLSTACK("CGTime::Read");
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
	atm.tm_year = ATOI(ppCmds[0]) - 1900;
	atm.tm_mon = ATOI(ppCmds[1]) - 1;
	atm.tm_mday = ATOI(ppCmds[2]);
	atm.tm_hour = ATOI(ppCmds[3]);
	atm.tm_min = ATOI(ppCmds[4]);
	atm.tm_sec = ATOI(ppCmds[5]);
	m_time = mktime(&atm);

	return true;
}

CGTime::CGTime()
{
	m_time = 0;
}

CGTime::CGTime(time_t time)
{
	m_time = time;
}

CGTime::CGTime(const CGTime& timeSrc)
{
	m_time = timeSrc.m_time;
}

const CGTime& CGTime::operator=(const CGTime& timeSrc)
{
	m_time = timeSrc.m_time;
	return *this;
}

const CGTime& CGTime::operator=(time_t t)
{
	m_time = t;
	return *this;
}

bool CGTime::operator<=( time_t t ) const
{
	return( m_time <= t );
}

bool CGTime::operator==( time_t t ) const
{
	return( m_time == t );
}

bool CGTime::operator!=( time_t t ) const
{
	return( m_time != t );
}

time_t CGTime::GetTime() const
{
	return m_time;
}

int CGTime::GetYear() const
{
	return (GetLocalTm(NULL)->tm_year) + 1900;
}

int CGTime::GetMonth() const       // month of year (1 = Jan)
{
	return GetLocalTm(NULL)->tm_mon + 1;
}

int CGTime::GetDay() const         // day of month
{
	return GetLocalTm(NULL)->tm_mday;
}

int CGTime::GetHour() const
{
	return GetLocalTm(NULL)->tm_hour;
}

int CGTime::GetMinute() const
{
	return GetLocalTm(NULL)->tm_min;
}

void CGTime::Init()
{
	m_time = -1;
}

bool CGTime::IsTimeValid() const
{
	return(( m_time && m_time != -1 ) ? true : false );
}

int CGTime::GetDaysTotal() const
{
	// Needs to be more consistant than accurate. just for compares.
	return(( GetYear() * 366) + (GetMonth()*31) + GetDay() );
}