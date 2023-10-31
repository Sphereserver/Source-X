#include "../network/CClientIterator.h"
#include "../common/CLog.h"
#include "chars/CChar.h"
#include "clients/CClient.h"
#include "CWorldComm.h"


static const SOUND_TYPE sm_Sounds_Ghost[] =
{
	SOUND_GHOST_1,
	SOUND_GHOST_2,
	SOUND_GHOST_3,
	SOUND_GHOST_4,
	SOUND_GHOST_5
};

void CWorldComm::Speak( const CObjBaseTemplate * pSrc, lpctstr pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font ) // static
{
	ADDTOCALLSTACK("CWorldComm::Speak");
	if ( !pszText || !pszText[0] )
		return;

	bool fSpeakAsGhost = false;
	if ( pSrc )
	{
		if ( pSrc->IsChar() )
		{
			const CChar *pSrcChar = static_cast<const CChar *>(pSrc);
			ASSERT(pSrcChar);

			// Are they dead? Garble the text. unless we have SpiritSpeak
			fSpeakAsGhost = pSrcChar->IsSpeakAsGhost();
		}
	}
	else
		mode = TALKMODE_BROADCAST;

	//CSString sTextUID;
	//CSString sTextName;	// name labelled text.
	CSString sTextGhost;	// ghost speak.

	// For things
	bool fCanSee = false;
	const CChar * pChar = nullptr;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next(), fCanSee = false, pChar = nullptr)
	{
		if ( ! pClient->CanHear( pSrc, mode ) )
			continue;

		tchar * myName = Str_GetTemp();

		lpctstr pszSpeak = pszText;
		pChar = pClient->GetChar();

		if ( pChar != nullptr )
		{
			fCanSee = pChar->CanSee(pSrc);

			if ( fSpeakAsGhost && !pChar->CanUnderstandGhost() )
			{
				if ( sTextGhost.IsEmpty() )
				{
					sTextGhost = pszText;
					for ( int i = 0; i < sTextGhost.GetLength(); ++i )
					{
						if ( sTextGhost[i] != ' ' &&  sTextGhost[i] != '\t' )
							sTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
					}
				}
				pszSpeak = sTextGhost;
				pClient->addSound( sm_Sounds_Ghost[ Calc_GetRandVal( ARRAY_COUNT( sm_Sounds_Ghost )) ], pSrc );
			}

			if ( !fCanSee && pSrc )
			{
				//if ( sTextName.IsEmpty() )
				//{
				//	sTextName.Format("<%s>", pSrc->GetName());
				//}
				//myName = sTextName;
				if ( !*myName )
					snprintf(myName, Str_TempLength(), "<%s>", pSrc->GetName());
			}
		}

		if ( ! fCanSee && pSrc && pClient->IsPriv( PRIV_HEARALL|PRIV_DEBUG ))
		{
			//if ( sTextUID.IsEmpty() )
			//{
			//	sTextUID.Format("<%s [%x]>", pSrc->GetName(), (dword)pSrc->GetUID());
			//}
			//myName = sTextUID;
			if ( !*myName )
				snprintf(myName, Str_TempLength(), "<%s [%x]>", pSrc->GetName(), (dword)pSrc->GetUID());
		}

		if (*myName)
			pClient->addBarkParse( pszSpeak, pSrc, wHue, mode, font, false, myName );
		else
			pClient->addBarkParse( pszSpeak, pSrc, wHue, mode, font );
	}
}

void CWorldComm::SpeakUNICODE( const CObjBaseTemplate * pSrc, const nachar * pwText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang ) // static
{
	ADDTOCALLSTACK("CWorldComm::SpeakUNICODE");
	bool fSpeakAsGhost = false;
	const CChar * pSrcChar = nullptr;

	if ( pSrc )
	{
		if ( pSrc->IsChar() )
		{
			pSrcChar = static_cast<const CChar *>(pSrc);
			ASSERT(pSrcChar);

			// Are they dead? Garble the text. unless we have SpiritSpeak
			fSpeakAsGhost = pSrcChar->IsSpeakAsGhost();
		}
	}
	else
		mode = TALKMODE_BROADCAST;

	if (mode != TALKMODE_SPELL)
	{
		if (pSrcChar && pSrcChar->m_SpeechHueOverride)
		{
			// This hue overwriting part for ASCII text isn't done in CWorld::Speak but in CClient::AddBarkParse.
			// If a specific hue is not given, use SpeechHueOverride.
			wHue = pSrcChar->m_SpeechHueOverride;
		}
	}

	nachar wTextUID[MAX_TALK_BUFFER];	// uid labelled text.
	wTextUID[0] = '\0';
	nachar wTextName[MAX_TALK_BUFFER];	// name labelled text.
	wTextName[0] = '\0';
	nachar wTextGhost[MAX_TALK_BUFFER];	// ghost speak.
	wTextGhost[0] = '\0';

	// For things
	bool fCanSee = false;
	const CChar * pChar = nullptr;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next(), fCanSee = false, pChar = nullptr)
	{
		if ( ! pClient->CanHear( pSrc, mode ) )
			continue;

		const nachar * pwSpeak = pwText;
		pChar = pClient->GetChar();

		if ( pChar != nullptr )
		{
			// Cansee?
			fCanSee = pChar->CanSee( pSrc );

			if ( fSpeakAsGhost && ! pChar->CanUnderstandGhost() )
			{
				if ( wTextGhost[0] == '\0' )	// Garble ghost.
				{
					size_t i;
					for ( i = 0; i < MAX_TALK_BUFFER - 1 && pwText[i]; ++i )
					{
						if ( pwText[i] != ' ' && pwText[i] != '\t' )
							wTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
						else
							wTextGhost[i] = pwText[i];
					}
					wTextGhost[i] = '\0';
				}
				pwSpeak = wTextGhost;
				pClient->addSound( sm_Sounds_Ghost[ Calc_GetRandVal( ARRAY_COUNT( sm_Sounds_Ghost )) ], pSrc );
			}

			// Must label the text.
			if ( ! fCanSee && pSrc )
			{
				if ( wTextName[0] == '\0' )
				{
					CSString sTextName;
					sTextName.Format("<%s>", pSrc->GetName());
					int iLen = CvtSystemToNETUTF16( wTextName, ARRAY_COUNT(wTextName), sTextName, -1 );
					if ( wTextGhost[0] != '\0' )
					{
						for ( int i = 0; wTextGhost[i] != '\0' && iLen < MAX_TALK_BUFFER - 1; i++, iLen++ )
							wTextName[iLen] = wTextGhost[i];
					}
					else
					{
						for ( int i = 0; pwText[i] != 0 && iLen < MAX_TALK_BUFFER - 1; i++, iLen++ )
							wTextName[iLen] = pwText[i];
					}
					wTextName[iLen] = '\0';
				}
				pwSpeak = wTextName;
			}
		}

		if ( ! fCanSee && pSrc && pClient->IsPriv( PRIV_HEARALL|PRIV_DEBUG ))
		{
			if ( wTextUID[0] == '\0' )
			{
				tchar * pszMsg = Str_GetTemp();
				snprintf(pszMsg, Str_TempLength(), "<%s [%x]>", pSrc->GetName(), (dword)pSrc->GetUID());
				int iLen = CvtSystemToNETUTF16( wTextUID, ARRAY_COUNT(wTextUID), pszMsg, -1 );
				for ( int i = 0; pwText[i] && iLen < MAX_TALK_BUFFER - 1; i++, iLen++ )
					wTextUID[iLen] = pwText[i];
				wTextUID[iLen] = '\0';
			}
			pwSpeak = wTextUID;
		}

		pClient->addBarkUNICODE( pwSpeak, pSrc, wHue, mode, font, lang );
	}
}

void CWorldComm::Broadcast(lpctstr pMsg) // static
{
	// System broadcast in bold text
	ADDTOCALLSTACK("CWorldComm::Broadcast");
	Speak( nullptr, pMsg, HUE_TEXT_DEF, TALKMODE_BROADCAST, FONT_BOLD );
}

void __cdecl CWorldComm::Broadcastf(lpctstr pMsg, ...) // static
{
	// System broadcast in bold text
	ADDTOCALLSTACK("CWorldComm::Broadcastf");
	TemporaryString tsTemp;
	va_list vargs;
	va_start(vargs, pMsg);
	vsnprintf(tsTemp.buffer(), tsTemp.capacity(), pMsg, vargs);
	va_end(vargs);
	Broadcast(tsTemp.buffer());
}
