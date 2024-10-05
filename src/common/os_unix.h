/**
* @file ox_unix.h
* @brief Unix-specific declarations.
*/

#ifndef _INC_OS_UNIX_H
#define _INC_OS_UNIX_H

//#include <limits.h>       // PATH_MAX
#include <unistd.h>			// usleep
#include <cctype>           // toupper, tolower


//#define SLASH_PATH	"/"


// Max. length of full pathname

/*
#if defined(PATH_MAX) // limits.h
#   if PATH_MAX <= 0
#       define SPHERE_MAX_PATH 260
#   elif PATH_MAX > 1024
#       define SPHERE_MAX_PATH 1024 // Define a "big enough" max value, just to avoid the need of enormous char buffers.
#   else
#       define SPHERE_MAX_PATH PATH_MAX
#   endif
#else
// If we want to keep compatibility with legacy Windows max path length.
#   define SPHERE_MAX_PATH 260
#endif
*/
// TODO: enable longer MAX_PATHs on both Linux and Windows, also checking that increasing that limit doesn't cause any buffer overflow.
#define SPHERE_MAX_PATH 260

//#define SPHERE_CDECL __attribute__((cdecl))
#define SPHERE_CDECL
#define E_FAIL			0x80004005	// exception code


/*  cross-platform functions macros  */

#define MAKEWORD(low,high)		((word)(((byte)(low))|(((word)((byte)(high)))<<8)))
//#define MAKELONG(low,high)	((long)(((word)(low))|(((dword)((word)(high)))<<16)))
#define LOWORD(l)		((word)((dword)(l) & 0xffff))
#define HIWORD(l)		((word)((dword)(l) >> 16))
#define LOBYTE(w)		((byte)((dword)(w) &  0xff))
#define HIBYTE(w)		((byte)((dword)(w) >> 8))

#ifdef _BSD
	int getTimezone();
    #define TIMEZONE		getTimezone()
#else
    #define TIMEZONE		timezone
#endif

/*  */

/*	file handling definitions  */
#define STDFUNC_FILENO			fileno
#define STDFUNC_UNLINK			unlink
// unix flushing works perfectly, so we do not need disabling buffer
#define	FILE_SETNOCACHE(_x_)
#define FILE_FLUSH(_x_)			fflush(_x_)

/*	thread-specific definitions  */
#define STDFUNC_GETPID			getpid
#define CRITICAL_SECTION		pthread_mutex_t

/* */
#define SLEEP(mSec)	usleep(mSec*1000)	// arg is microseconds = 1/1000000

/*  No portable UNIX/LINUX equiv to this.  */
inline void _strupr( tchar * pszStr ) noexcept
{
	for ( ; pszStr[0] != '\0'; ++pszStr )
		*pszStr = toupper( *pszStr );
}

inline void _strlwr( tchar * pszStr ) noexcept
{
	for ( ; pszStr[0] != '\0'; ++pszStr )
		*pszStr = tolower( *pszStr );
}
/*  */

#endif	// _INC_OS_UNIX_H
