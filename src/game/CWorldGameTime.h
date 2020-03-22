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
	static CServerTime GetCurrentTime();

	static int64 GetCurrentTimeInGameMinutes(int64 basetime);	// return game world minutes
	static int64 GetCurrentTimeInGameMinutes();
	
	static uint GetMoonPhase( bool fMoonIndex = false );
	static int64 GetNextNewMoon( bool fMoonIndex );
};


#endif // _INC_CWORLDGAMETIME_H
