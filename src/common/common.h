/**
* @file common.h
* @brief Header that should be included by every file.
*/

#ifndef _INC_COMMON_H
#define _INC_COMMON_H


#define SPHERE_DEF_PORT			2593
#define SPHERE_FILE				"sphere"	// file name prefix
#define SPHERE_TITLE			"SphereServer-eXperimental"
#define SPHERE_SCRIPT			".scp"
#define SCRIPT_MAX_LINE_LEN		4096		// default size.


#include <cstdlib>

#ifdef _WIN32
	#include "os_windows.h"
#endif
#include "datatypes.h"
#ifndef _WIN32
	#include "os_unix.h"
#endif

// use to indicate that a function uses printf-style arguments, allowing GCC
// to validate the format string and arguments:
// a = 1-based index of format string
// b = 1-based index of arguments
// (note: add 1 to index for non-static class methods because 'this' argument
// is inserted in position 1)
#ifdef _MSC_VER
	#define __printfargs(a,b)
#else
	#ifdef _WIN32
		#define __printfargs(a,b) __attribute__ ((format(gnu_printf, a, b)))
	#else
		#define __printfargs(a,b) __attribute__ ((format(printf, a, b)))
	#endif
#endif

#ifdef UNICODE
	#define IsDigit(c)			iswdigit((wint_t)c)
	#define IsSpace(c)			iswspace((wint_t)c)
	#define IsAlpha(c)			iswalpha((wint_t)c)
	#define IsAlnum(c)			iswalnum((wint_t)c)
#else
	#define IsDigit(c)			isdigit((int)(c & 0xFF))	// It was: (uchar)c
	#define IsSpace(c)			isspace((int)(c & 0xFF))
	#define IsAlpha(c)			isalpha((int)(c & 0xFF))
	#define IsAlnum(c)			isalnum((int)(c & 0xFF))
#endif

#define IsNegative(c)		((c < 0)?1:0)
#define MulDiv(a,b,c)		(((((int)(a)*(int)(b)) + (int)(c / 2)) / (int)(c)) - (IsNegative((int)(a)*(int)(b))))
#define MulDivLL(a,b,c)		(((((llong)(a)*(llong)(b)) + (llong)(c / 2)) / (llong)(c)) - (IsNegative((llong)(a)*(llong)(b))))
#define MulMulDiv(a,b,c)	(((a)*(b))/(c))
#define minimum(x,y)		((x)<(y)?(x):(y))
#define maximum(x,y)		((x)>(y)?(x):(y))
#define medium(x,y,z)		((x)>(y)?(x):((z)<(y)?(z):(y)))
#define CountOf(a)			(sizeof(a)/sizeof((a)[0]))
#define sign(n) (((n) < 0) ? -1 : (((n) > 0) ? 1 : 0))
//#define abs(n) (((n) < 0) ? (-(n)) : (n))


/* These macros are uppercase for conformity to windows.h macros */
#define MAKEDWORD(low, high) ((dword)(((word)low) | (((dword)((word)high)) << 16)))

// Desguise an id as a pointer.
#ifndef MAKEINTRESOURCE			// typically on unix
	#define MAKEINTRESOURCEA(i) ((lpstr)((size_t)((word)i)))
	#define MAKEINTRESOURCEW(i) ((lpwstr)((size_t)((word)i)))
	#ifdef UNICODE
		#define MAKEINTRESOURCE  MAKEINTRESOURCEW
	#else
		#define MAKEINTRESOURCE  MAKEINTRESOURCEA
	#endif
#endif
#define ISINTRESOURCE(r)	((((size_t)r) >> 16) == 0)
#define GETINTRESOURCE(r)	(((size_t)r)&0xFFFF)

#ifndef ASSERT
	#ifdef _DEBUG
		extern void Assert_Fail(const char * pExp, const char *pFile, long long llLine);
		#define ASSERT(exp)			if ( !(exp) )	Assert_Fail(#exp, __FILE__, __LINE__);
	#else
		#define ASSERT(exp)			(void)0
	#endif
#endif
/* End of macros section */

typedef uint	ERROR_CODE;
typedef THREAD_ENTRY_RET(_cdecl * PTHREAD_ENTRY_PROC)(void *);


#endif	// _INC_COMMON_H
