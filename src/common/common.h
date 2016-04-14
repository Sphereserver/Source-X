// always for __cplusplus
// I try to compile in several different environments.
// 1. DOS command line or windows (_WIN32	by compiler or _INC_WINDOWS in windows.h)
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
#endif

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

typedef THREAD_ENTRY_RET ( _cdecl * PTHREAD_ENTRY_PROC )(void *);

#define SCRIPT_MAX_LINE_LEN 4096	// default size.

#define IsDigit(c)			isdigit((uchar)c)
#define IsSpace(c)			isspace((uchar)c)
#define IsAlpha(c)			isalpha((uchar)c)
#define IsNegative(c)		((c < 0)?1:0)
#define MulMulDiv(a,b,c)	(((a)*(b))/(c))
#define MulDivLL(a,b,c)		(((((llong)(a)*(llong)(b))+(c / 2))/(c))-(IsNegative((llong)(a)*(llong)(b))))

#ifndef MAKEDWORD
	#define MAKEDWORD(low, high) ((dword)(((word)(low)) | (((dword)((word)(high))) << 16)))
#endif

#ifndef COUNTOF
	#define COUNTOF(a)	(sizeof(a)/sizeof((a)[0]))
#endif

typedef uint	ERROR_CODE;

#ifndef minimum
	#define minimum(x,y)	((x)<(y)?(x):(y))
#endif
#ifndef maximum
	#define maximum(x,y)	((x)>(y)?(x):(y))
#endif

#define medium(x,y,z)		((x)>(y)?(x):((z)<(y)?(z):(y)))

#endif	// _INC_COMMON_H
