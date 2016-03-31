// always for __cplusplus
// I try to compile in several different environments.
// 1. DOS command line or windows (_WINDOWS	by compiler or _INC_WINDOWS in windows.h)
// 2. MFC or not MFC  (__AFX_H__ in afx.h or _MFC_VER by compiler)
// 3. 16 bit or 32 bit (_WIN32 defined by compiler)
// 4. LINUX 32 bit

#pragma once
#ifndef _INC_COMMON_H
#define _INC_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <queue>
#include <deque>
#include <vector>
#include <stack>

#ifdef _WIN32
	#include "os_windows.h"
#endif
#include "datatypes.h"
#ifndef _WIN32
	#include "os_unix.h"
#endif // _WIN32

// use to indicate that a function uses printf-style arguments, allowing GCC
// to validate the format string and arguments:
// a = 1-based index of format string
// b = 1-based index of arguments
// (note: add 1 to index for non-static class methods because 'this' argument
// is inserted in position 1)
#ifdef __GNUC__
	#define __printfargs(a,b) __attribute__ ((format(printf, a, b)))
#else
	#define __printfargs(a,b)
#endif

#define IsDigit(c) isdigit((uchar)c)
#define IsSpace(c) isspace((uchar)c)
#define IsAlpha(c) isalpha((uchar)c)
#define IsNegative(c) ((c < 0)?1:0)

typedef THREAD_ENTRY_RET ( _cdecl * PTHREAD_ENTRY_PROC )(void *);

#define SPHERE_DEF_PORT	2593
#define SPHERE_FILE		"sphere"	// file name prefix
#define SPHERE_TITLE		"Sphere"
#define SPHERE_SCRIPT		".scp"

#define SCRIPT_MAX_LINE_LEN 4096	// default size.

#define IMULDIVDOWN(a,b,c)	(((a)*(b))/(c))
#define IMULDIV(a,b,c)		(((((llong)(a)*(llong)(b))+(c / 2))/(c))-(IsNegative((llong)(a)*(llong)(b))))

#ifndef MAKEDWORD
	#define MAKEDWORD(low, high) ((dword)(((word)(low)) | (((dword)((word)(high))) << 16)))
#endif

#ifndef COUNTOF
	#define COUNTOF(a)	(sizeof(a)/sizeof((a)[0]))
#endif

typedef uint	ERROR_CODE;

#define ISWHITESPACE(ch)			(IsSpace(ch)||((uchar)ch)==0xa0)	// IsSpace
#define GETNONWHITESPACE( pStr )	while ( ISWHITESPACE( (pStr)[0] )) { (pStr)++; }
#define _IS_SWITCH(c)				((c) == '-' || (c) == '/' )	// command line switch.

#define REMOVE_QUOTES( x )			\
{									\
	GETNONWHITESPACE( x );			\
	if ( *x == '"' )	++x;				\
		tchar * psX	= const_cast<tchar*>(strchr( x, '"' ));	\
	if ( psX )						\
		*psX	= '\0';				\
}

#ifndef minimum
	#define minimum(x,y)	((x)<(y)?(x):(y))
#endif
#ifndef maximum
	#define maximum(x,y)	((x)>(y)?(x):(y))
#endif

#define medium(x,y,z)		((x)>(y)?(x):((z)<(y)?(z):(y)))


// -----------------------------
//	Time measurement macroses
// -----------------------------

extern llong llTimeProfileFrequency;

#ifdef _WIN32
	#define	TIME_PROFILE_INIT	\
		llong llTicks(0), llTicksEnd
	#define	TIME_PROFILE_START	\
		if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicks)) llTicks = GetTickCount()
	#define TIME_PROFILE_END	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicksEnd)) llTicksEnd = GetTickCount()
#else
	#define	TIME_PROFILE_INIT	\
		llong llTicks(0), llTicksEnd
	#define	TIME_PROFILE_START	\
		llTicks = GetTickCount()
	#define TIME_PROFILE_END	llTicksEnd = GetTickCount();
#endif

#define TIME_PROFILE_GET_HI	((llTicksEnd - llTicks)/(llTimeProfileFrequency/1000))
#define	TIME_PROFILE_GET_LO	((((llTicksEnd - llTicks)*10000)/(llTimeProfileFrequency/1000))%10000)

// -----------------------------
//	CEventLog
// -----------------------------

enum LOGL_TYPE
{
	// critical level.
	LOGL_FATAL	= 1, 	// fatal error ! cannot continue.
	LOGL_CRIT	= 2, 	// critical. might not continue.
	LOGL_ERROR	= 3, 	// non-fatal errors. can continue.
	LOGL_WARN	= 4,	// strange.
	LOGL_EVENT	= 5,	// Misc major events.
	// subject matter. (severity level is first 4 bits, LOGL_EVENT)
	LOGM_INIT = 0x00100,		// start up messages.
	LOGM_NOCONTEXT = 0x20000,	// do not include context information
	LOGM_DEBUG = 0x40000		// debug kind of message with DEBUG: prefix
};

extern class CEventLog
{
	// Any text event stream. (destination is independant)
	// May include __LINE__ or __FILE__ macro as well ?

protected:
	virtual int EventStr(dword wMask, lpctstr pszMsg)
	{
		UNREFERENCED_PARAMETER(wMask);
		UNREFERENCED_PARAMETER(pszMsg);
		return 0;
	}
	virtual int VEvent(dword wMask, lpctstr pszFormat, va_list args);

public:
	int _cdecl Event( dword wMask, lpctstr pszFormat, ... ) __printfargs(3,4)
	{
		va_list vargs;
		va_start( vargs, pszFormat );
		int iret = VEvent( wMask, pszFormat, vargs );
		va_end( vargs );
		return( iret );
	}

	int _cdecl EventDebug(lpctstr pszFormat, ...) __printfargs(2,3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iret = VEvent(LOGM_NOCONTEXT|LOGM_DEBUG, pszFormat, vargs);
		va_end(vargs);
		return iret;
	}

	int _cdecl EventError(lpctstr pszFormat, ...) __printfargs(2,3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iret = VEvent(LOGL_ERROR, pszFormat, vargs);
		va_end(vargs);
		return iret;
	}

	int _cdecl EventWarn(lpctstr pszFormat, ...) __printfargs(2,3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iret = VEvent(LOGL_WARN, pszFormat, vargs);
		va_end(vargs);
		return iret;
	}

#ifdef _DEBUG
	int _cdecl EventEvent( lpctstr pszFormat, ... ) __printfargs(2,3)
	{
		va_list vargs;
		va_start( vargs, pszFormat );
		int iret = VEvent( LOGL_EVENT, pszFormat, vargs );
		va_end( vargs );
		return( iret );
	}
#endif //_DEBUG

	#define DEBUG_ERR(_x_)	g_pLog->EventError _x_

	#ifdef _DEBUG
		#define DEBUG_WARN(_x_)	g_pLog->EventWarn _x_
		#define DEBUG_MSG(_x_)	g_pLog->EventEvent _x_
		#define DEBUG_MYFLAG(_x_) g_pLog->Event _x_
	#else
		#define DEBUG_WARN(_x_)
		#define DEBUG_MSG(_x_)
		#define DEBUG_MYFLAG(_x_)
	#endif

public:
	CEventLog() { };

private:
	CEventLog(const CEventLog& copy);
	CEventLog& operator=(const CEventLog& other);
} * g_pLog;

// -----------------------------
//	CValStr
// -----------------------------

struct CValStr
{
	// Associate a val with a string.
	// Assume sorted values from min to max.
public:
	lpctstr m_pszName;
	int m_iVal;
public:
	void SetValues( int iVal, lpctstr pszName )
	{
		m_iVal = iVal;
		m_pszName = pszName;
	}
	lpctstr FindName( int iVal ) const;
	void SetValue( int iVal )
	{
		m_iVal = iVal;
	}
};

#endif	// _INC_COMMON_H
