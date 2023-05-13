/**
* @file CWorldComm.h
*
*/

#ifndef _INC_CWORLDCOMM_H
#define _INC_CWORLDCOMM_H

#include "../common/common.h"
#include "../common/sphereproto.h"
#include "uo_files/uofiles_enums.h"
#include "uo_files/uofiles_types.h"

class CObjBaseTemplate;


class CWorldComm
{
public:
	static const char* m_sClassName;

	static void Speak(const CObjBaseTemplate* pSrc, lpctstr pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD);
	static void SpeakUNICODE(const CObjBaseTemplate* pSrc, const nachar* pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang);

	static void Broadcast(lpctstr pMsg);
	static void __cdecl Broadcastf(lpctstr pMsg, ...) __printfargs(1, 2);
};


#endif // _INC_CWORLDCOMM_H
