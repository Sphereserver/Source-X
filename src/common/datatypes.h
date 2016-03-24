/**
* @file datatypes.h
* @brief Sphere standard data types.
*/

// If windows.h was included the api string types were declared, so let's declare our aliases.
#if defined(_WINDOWS_) && !defined(_INC_DATATYPES_H_STR)
	/* IMPORTANT: for Windows-specific code, especially when using Windows API calls,
	we should use WinAPI data types (so, for example, dword instead of dword) */
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
/*  -- Macros from <cinttypes> we should use when formatting numbers inside strings --
	--  (like in printf/sprintf/etc) to achieve best cross-platform support			--
	Examples:	(u: unsigned - x: hex - d: decimal)
	PRIu8		//for byte
	PRIu16		//for word
	PRIu32		//for dword
	PRId64		//for long long
*/

typedef unsigned char			uchar;
typedef unsigned short			ushort;
typedef unsigned int			uint;
//typedef unsigned long			ulong;	// long and ulong shouldn't be used...
typedef long long				llong;
typedef unsigned long long		ullong;

/* To be used when coding packets */
typedef uint8_t			byte;		// 8 bits
typedef uint16_t		word;		// 16 bits
typedef uint32_t		dword;		// 32 bits

/*			Use the following types only if you know *why* you need them.
These types have a specified size, while standard data types (short, int, etc) haven't:
for example, sizeof(int) is definied to be >= sizeof(short) and <= sizeof(long), so it hasn't a fixed size;
this concept is valid for both non unix and unix OS.
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


#ifdef _WIN32
	
	// Since on MinGW %llx and %llu throw warnings...
	#ifdef __MINGW__
		#undef  PRIu64
		#undef  PRIx64
		#define PRIu64 "l64"
		#define PRIx64 "l64x"
	#endif

#else	//	assume unix if !_WIN32

	#include <wchar.h>
	#ifdef UNICODE
		typedef	wchar_t			tchar;
	#else
		typedef char			tchar;
	#endif
	typedef wchar_t				wchar;
	typedef wchar_t *			lpwstr;
	typedef const wchar_t *		lpcwstr;
	typedef	char *				lpstr;
	typedef	const char *		lpcstr;
	typedef	tchar *				lptstr;
	typedef const tchar *		lpctstr;

#endif // _WIN32

#endif // _INC_DATATYPES_H
