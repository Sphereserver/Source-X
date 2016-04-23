/**
* @file ox_unix.h
* @brief Unix-specific declarations.
*/

#pragma once
#ifndef _INC_OS_UNIX_H
#define _INC_OS_UNIX_H

#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <setjmp.h>
#include <dlfcn.h>
#include <errno.h>
#include <aio.h>
#include <exception>
#include <cctype>
#include "sphere_library/CSTime.h"


//#define SLASH_PATH	"/"

#ifndef _MAX_PATH			// stdlib.h ?
	#define _MAX_PATH   260 	// max. length of full pathname
#endif

#define _cdecl

/*	thread-specific definitions  */
#define THREAD_ENTRY_RET		void *
#define STDFUNC_FILENO			fileno
#define STDFUNC_GETPID			getpid
#define STDFUNC_UNLINK			unlink
#define CRITICAL_SECTION		pthread_mutex_t
#define Sleep(mSec)				usleep(mSec*1000)	// arg is microseconds = 1/1000000
#define SleepEx(mSec, unused)	usleep(mSec*1000)	// arg is microseconds = 1/1000000
/*  */

#define ERROR_SUCCESS	0
#define UNREFERENCED_PARAMETER(P)	(void)(P)
#define HKEY_LOCAL_MACHINE			(( HKEY ) 0x80000002 )

#define MAKEWORD(low,high)	((word)(((byte)(low))|(((word)((byte)(high)))<<8)))
//#define MAKELONG(low,high)	((long)(((word)(low))|(((dword)((word)(high)))<<16)))
#define LOWORD(l)	((word)((dword)(l) & 0xffff))
#define HIWORD(l)	((word)((dword)(l) >> 16))
#define LOBYTE(w)	((byte)((dword)(w) &  0xff))
#define HIBYTE(w)	((byte)((dword)(w) >> 8))

#define sign(n) (((n) < 0) ? -1 : (((n) > 0) ? 1 : 0))
//#define abs(n) (((n) < 0) ? (-(n)) : (n))

// unix flushing works perfectly, so we do not need disabling buffer
#define	FILE_SETNOCACHE(_x_)
#define FILE_FLUSH(_x_)			fflush(_x_)


inline void _strupr( tchar * pszStr )
{
	// No portable UNIX/LINUX equiv to this.
	for ( ;pszStr[0] != '\0'; pszStr++ )
	{
		*pszStr = toupper( *pszStr );
	}
}

inline void _strlwr( tchar * pszStr )
{
	// No portable UNIX/LINUX equiv to this.
	for ( ;pszStr[0] != '\0'; pszStr++ )
	{
		*pszStr = tolower( *pszStr );
	}
}


#endif	// _INC_OS_UNIX_H
