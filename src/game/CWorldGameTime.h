/**
* @file CWorldGameTime.h
*
*/

#ifndef _INC_CWORLDGAMETIME_H
#define _INC_CWORLDGAMETIME_H

#include "CServerTime.h"


class CWorldGameTime
{
public:
	static const char* m_sClassName;

	#undef GetCurrentTime
	static const CServerTime& GetCurrentTime() noexcept;

	static int64 GetCurrentTimeInGameMinutes(int64 basetime) noexcept;	// return game world minutes
	static int64 GetCurrentTimeInGameMinutes() noexcept;
	
	static uint GetMoonPhase( bool fMoonIndex = false );
	static int64 GetNextNewMoon( bool fMoonIndex );
};


#endif // _INC_CWORLDGAMETIME_H
