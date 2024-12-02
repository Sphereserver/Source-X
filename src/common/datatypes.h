/**
* @file datatypes.h
* @brief Sphere standard data types.
*/

// !!!!!!!!!!!!!!!!!! WARNING: do not include datatypes.h directly, but include common.h first!
// !!!!!!!!!!!!!!!!!!   If you include datatypes.h you won't have windows.h and its data types.

//----------------------------------------------------------------------------------------------


// If windows.h was included the api string types were declared, so let's declare our aliases.
// This is outside the main include guard since this file can be loaded multiple times, with windows.h included or not.
#if defined(_WINDOWS_) && !defined(_INC_DATATYPES_H_STR)
	/* IMPORTANT: for Windows-specific code, especially when using Windows API calls,
	we should use WinAPI data types (so, for example, DWORD instead of dword) */
	#define _INC_DATATYPES_H_STR
	typedef	TCHAR		tchar;
	typedef WCHAR		wchar;
	typedef LPWSTR		lpwstr;
	typedef LPCWSTR		lpcwstr;
	typedef	LPSTR		lpstr;
	typedef	LPCSTR		lpcstr;
	typedef	LPTSTR		lptstr;
	typedef LPCTSTR		lpctstr;
#endif

// no #pragma once
#ifndef _INC_DATATYPES_H
#define _INC_DATATYPES_H


#include <cinttypes>
/*  -- Macros from <cinttypes> we should use when formatting numbers inside strings		--
	--  (like in printf/sprintf/etc) to achieve best cross-platform/compiler support	--
	Examples:	(u: unsigned - x: hex - d: decimal)

	Mandatory:
	PRId64		// for long long
	PRIx64		// llong, hexadecimal
	PRIu64		// llong, unsigned

	Optional (for now):
	all of the others
*/

typedef float					realtype16;
typedef double					realtype32;
typedef long double				realtype64;
typedef realtype32				realtype;

typedef signed char              schar; // could need to be specified in some places. On x86 char is signed by default, but on ARM it's unsigned!
typedef unsigned char			uchar;
typedef unsigned short			ushort;
typedef unsigned int			uint;
//typedef unsigned long			ulong;	// long and ulong shouldn't be used.. can have different sizes on different platforms.
typedef long long				llong;
typedef unsigned long long		ullong;

/* To be used when coding packets */
typedef uint8_t		byte;		// 8 bits
typedef uint16_t		word;		// 16 bits
typedef uint32_t		dword;		// 32 bits

/*		Use the following types only if you know *why* you need them.
These types have a fixed, specified size, while standard data types (short, int, etc) haven't:
for example, sizeof(int) is definied to be >= sizeof(short) and <= sizeof(long), so it hasn't a fixed size;
this concept is valid for both non unix and unix OSes.
On the vast majority of platforms, base data types have well definied dimensions (for now):
	char: 1 byte - short: 2 bytes - int: 4 bytes - long: (4 on windows, 8 on unix) bytes - long long: 8 bytes
In the future they may change to higher values, so you should use fixed-size types only when you know that
this variable should not exceed a maximum value, and check whether or not it exceeds it; when an upper limit is
not required, standard types (int, short, etc) are better and they are/will be faster, depending on the circumstances. */
typedef	int8_t			int8;
typedef	int16_t			int16;
typedef	int32_t			int32;
typedef	int64_t			int64;
typedef	uint8_t			uint8;
typedef	uint16_t		uint16;
typedef	uint32_t		uint32;
typedef	uint64_t		uint64;


#ifndef _WIN32			//	assume unix if !_WIN32

	#include <wchar.h>
	//#ifdef UNICODE
	//	typedef	char16_t		tchar;
	//#else
		typedef char			tchar;
	//#endif
	typedef char16_t			wchar;
	typedef char16_t *			lpwstr;
	typedef const char16_t *	lpcwstr;
	typedef	char *				lpstr;
	typedef	const char *		lpcstr;
	typedef	tchar *				lptstr;
	typedef const tchar *		lpctstr;

	// printf format identifiers
	#define PRIuSIZE_T			"zu"		// linux uses %zu to format size_t
	#define PRIxSIZE_T			"zx"
	#define PRIdSSIZE_T			"zd"

#else	// _WIN32

	#ifdef _MSC_VER // already defined on MinGW
		#include <basetsd.h>
		typedef SSIZE_T				ssize_t;
	#endif // _MSC_VER

	// printf format identifiers
	#if !defined(_MSC_VER)
		#define PRIuSIZE_T			"zu"
		#define PRIxSIZE_T			"zx"
		#define PRIdSSIZE_T			"zd"
	#else
        // If using MSVC runtime and not GNU/MinGW.
		#define PRIuSIZE_T			"Iu"
		#define PRIxSIZE_T			"Ix"
		#define PRIdSSIZE_T			"Id"
	#endif

#endif // !_WIN32


#endif // _INC_DATATYPES_H
