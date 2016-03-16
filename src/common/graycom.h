#ifndef _INC_GRAYCOM_H
#define _INC_GRAYCOM_H
#pragma once

//---------------------------SYSTEM DEFINITIONS---------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#ifdef _WIN32
// NOTE: If we want a max number of sockets we must compile for it !
	#undef FD_SETSIZE
	#define FD_SETSIZE 1024 // for max of n users ! default = 64

	#ifndef STRICT
		#define STRICT			// strict conversion of handles and pointers.
	#endif	// STRICT

	#include <io.h>
	#include <winsock2.h>
	#include <windows.h>
	#include <dos.h>
	#include <conio.h>
	#include <sys/timeb.h>

	#define strcmpi		_strcmpi	// Non ANSI equiv functions ?
	#define strnicmp	_strnicmp

	extern const OSVERSIONINFO * GRAY_GetOSInfo();
#else	// _WIN32 else assume LINUX

	#include <sys/types.h>
	#include <sys/timeb.h>

	#define HANDLE			DWORD
	#define _cdecl
	#define __cdecl

	#define FAR
	#define E_FAIL			0x80004005

	#ifdef _BSD
		int getTimezone();
		#define _timezone		getTimezone()
	#else
		#define _timezone		timezone
	#endif

	#define IsBadReadPtr( p, len )		((p) == NULL)
	#define IsBadStringPtr( p, len )	((p) == NULL)
	#define Sleep(mSec)					usleep(mSec*1000)	// arg is microseconds = 1/1000000
	#define SleepEx(mSec, unused)		usleep(mSec*1000)	// arg is microseconds = 1/1000000

	#define strcmpi		strcasecmp
	#define strnicmp	strncasecmp
	#define _vsnprintf	vsnprintf
#endif // !_WIN32

#ifdef _DEBUG
	#ifndef ASSERT
		extern void Assert_CheckFail( const char * pExp, const char *pFile, long lLine );
		#define ASSERT(exp)			(void)( (exp) || (Assert_CheckFail(#exp, __FILE__, __LINE__), 0) )
	#endif	// ASSERT

	#ifndef STATIC_CAST
		#define STATIC_CAST dynamic_cast
	#endif

#else	// _DEBUG

	#ifndef ASSERT
		/*#ifndef _WIN32
			// In linux, if we get an access violation, an exception isn't thrown.  Instead, we get
			// a SIG_SEGV, and the process cores. The following code takes care of this for us.
			extern void Assert_CheckFail( const char * pExp, const char *pFile, long lLine );
			//matex3: is this still necessary? We have a SIG_SEGV handler nowaways.
			#define ASSERT(exp)			(void)( (exp) || (Assert_CheckFail(#exp, __FILE__, __LINE__), 0) )
		#else*/
			#define ASSERT(exp)
		/*#endif*/
	#endif	// ASSERT

	#ifndef STATIC_CAST
		#define STATIC_CAST static_cast
	#endif

#endif	// ! _DEBUG

#ifdef _WIN32
	#define ATOI atoi
	#define ITOA _itoa
	#define LTOA _ltoa
	#define STRREV _strrev
#else
	int ATOI( const char * str );
	char * ITOA(int value, char *string, int radix);
	char * LTOA(long value, char *string, int radix);
	void STRREV( char* string );
#endif

// Macro for fast NoCrypt Client version check
#define IsAosFlagEnabled( value )	( g_Cfg.m_iFeatureAOS & (value) )
#define IsResClient( value )		( GetAccount()->GetResDisp() >= (value) )

#define FEATURE_T2A_UPDATE 			0x01
#define FEATURE_T2A_CHAT 			0x02
#define FEATURE_LBR_UPDATE			0x01
#define FEATURE_LBR_SOUND			0x02

#define FEATURE_AOS_UPDATE_A		0x01	// AOS Monsters, Map, Skills
#define FEATURE_AOS_UPDATE_B		0x02	// Tooltip, Fightbook, Necro/paladin on creation, Single/Six char selection screen
#define FEATURE_AOS_POPUP			0x04	// PopUp Menus
#define FEATURE_AOS_DAMAGE			0x08

#define FEATURE_SE_UPDATE			0x01	// 0x00008 in 0xA9
#define FEATURE_SE_NINJASAM			0x02	// 0x00040 in feature

#define FEATURE_ML_UPDATE			0x01 	// 0x00100 on charlist and 0x0080 for feature to activate

#define FEATURE_KR_UPDATE			0x01	// 0x00200 in 0xA9 (KR crapness)
#define FEATURE_KR_CLIENTTYPE		0x02	// 0x00400 in 0xA9 (enables 0xE1 packet)

#define FEATURE_SA_UPDATE			0x01	// 0x10000 feature (unlock gargoyle character, housing items)
#define FEATURE_SA_MOVEMENT			0x02	// 0x04000 on charlist (new movement packets)

#define FEATURE_TOL_UPDATE			0x01	// 0x400000 feature
#define FEATURE_TOL_VIRTUALGOLD		0x02	// Not related to login flags

#define FEATURE_EXTRA_CRYSTAL		0x01	// 0x200 feature (unlock ML crystal items on house design)
#define FEATURE_EXTRA_GOTHIC		0x02	// 0x40000 feature (unlock SA gothic items on house design)
#define FEATURE_EXTRA_RUSTIC		0x04	// 0x80000 feature (unlock SA rustic items on house design)
#define FEATURE_EXTRA_JUNGLE		0x08	// 0x100000 feature (unlock TOL jungle items on house design)
#define FEATURE_EXTRA_SHADOWGUARD	0x10	// 0x200000 feature (unlock TOL shadowguard items on house design)


#endif	// _INC_GRAYCOM_H
