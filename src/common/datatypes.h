#pragma once
#ifndef DATATYPES_H

#define DATATYPES_H
#include <stdint.h>

typedef long long				LLONG;
typedef unsigned long long		ULLONG;


#ifdef _WIN32

	#include <windows.h>		// because <minwindef.h> can't be directly included

	/*
		Use the following types only if you know that you need them.
		These types have a specified size, while standard data types (short, int, etc) haven't:
		for example, sizeof(int) is definied to be >= sizeof(short) and <= sizeof(long), so it hasn't a fixed size;
		nevertheless, most of the times using int instead of INT32 would suit you.
		This is valid for both non unix and unix OS.
	*/
	#define INT8				__int8
	#define INT16				__int16
	#define INT32				__int32
	#define INT64				__int64
	#define UINT8				unsigned __int8
	#define UINT16				unsigned __int16
	#define UINT32				unsigned __int32
	#define UINT64				unsigned __int64

#else	//	assume unix if !_WIN32

	#undef BYTE
	#undef WORD
	#undef DWORD
	#undef UINT
	#undef LONGLONG
	#undef ULONGLONG
	#undef LONG
	#undef INT8
	#undef INT16
	#undef INT32
	#undef INT64
	#undef UINT8
	#undef UINT16
	#undef UINT32
	#undef UINT64
	#undef BOOL
	#undef PUINT
	
	typedef uint8_t				BYTE;		// 8 bits
	typedef uint16_t			WORD;		// 16 bits
	typedef uint32_t			DWORD;		// 32 bits
	typedef unsigned int		UINT;
	typedef long long			LONGLONG; 	// this must be 64 bits
	typedef unsigned long long	ULONGLONG;
	typedef long				LONG;		// never use long! use int or long long!
	typedef	int8_t				INT8;
	typedef	int16_t				INT16;
	typedef	int32_t				INT32;
	typedef	int64_t				INT64;
	typedef	uint8_t				UINT8;
	typedef	uint16_t			UINT16;
	typedef	uint32_t			UINT32;
	typedef	uint64_t			UINT64;
	typedef	unsigned short		BOOL;
	typedef unsigned int *		PUINT;

	#include <wchar.h>
	#ifdef UNICODE
		typedef	wchar_t			TCHAR;
	#else
		typedef char			TCHAR;
	#endif
	typedef wchar_t				WCHAR;
	typedef	const TCHAR *		LPCSTR;
	typedef const TCHAR *		LPCTSTR;
	typedef	TCHAR *				LPSTR;
	typedef	TCHAR *				LPTSTR;

#endif

#endif
