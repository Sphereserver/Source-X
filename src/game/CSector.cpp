
#include "../common/CException.h"
#include "../common/CLog.h"
#include "../sphere/ProfileTask.h"
#include "../sphere/ProfileData.h"
#include "chars/CChar.h"
#include "chars/CCharNPC.h"
#include "clients/CClient.h"
#include "components/CCSpawn.h"
#include "components/CCItemDamageable.h"
#include "items/CItem.h"
#include "CObjBase.h"
#include "CSector.h"
#include "CWorld.h"
#include "spheresvr.h"
#include "triggers.h"

//////////////////////////////////////////////////////////////////
// -CSector

CSector::CSector() : CTimedObject(PROFILE_SECTORS)
{
	m_ListenItems = 0;

	m_RainChance = 0;		// 0 to 100%
	m_ColdChance = 0;		// Will be snow if rain chance success.
	SetDefaultWeatherChance();

	m_dwFlags = 0;
	m_fSaveParity = false;
    GoSleep();    // Every sector is sleeping at start, they only awake when any player enter (this eases the load at startup).
}

CSector::~CSector()
{
	ASSERT( ! GetClientsNumber());
}

enum SC_TYPE
{
    SC_CANSLEEP,
	SC_CLIENTS,
	SC_COLDCHANCE,
	SC_COMPLEXITY,
	SC_FLAGS,
	SC_ISDARK,
	SC_ISNIGHTTIME,
    SC_ISSLEEPING,
    SC_ITEMCOUNT,
	SC_LIGHT,
	SC_LOCALTIME,
	SC_LOCALTOD,
	SC_NUMBER,
	SC_RAINCHANCE,
	SC_SEASON,
	SC_WEATHER,
	SC_QTY
};

lpctstr const CSector::sm_szLoadKeys[SC_QTY+1] =
{
    "CANSLEEP",
	"CLIENTS",
	"COLDCHANCE",
	"COMPLEXITY",
	"FLAGS",
	"ISDARK",
	"ISNIGHTTIME",
    "ISSLEEPING",
	"ITEMCOUNT",
	"LIGHT",
	"LOCALTIME",
	"LOCALTOD",
	"NUMBER",
	"RAINCHANCE",
	"SEASON",
	"WEATHER",
	nullptr
};

bool CSector::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallParent);
    UNREFERENCED_PARAMETER(fNoCallChildren);
	ADDTOCALLSTACK("CSector::r_WriteVal");
	EXC_TRY("WriteVal");

	static constexpr CValStr sm_ComplexityTitles[] =
	{
		{ "HIGH", INT32_MIN },	// speech can be very complex if low char count
		{ "MEDIUM", 5 },
		{ "LOW", 10 },
		{ nullptr, INT32_MAX }
	};

    SC_TYPE key = (SC_TYPE)FindTableHeadSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
	switch ( key )
	{
        case SC_CANSLEEP:
            {
                ptcKey += 8;
                const bool fCheckAdjacents = Exp_GetVal(ptcKey);
                sVal.FormatBVal(CanSleep(fCheckAdjacents));
                return true;
            }
		case SC_CLIENTS:
			sVal.FormatSTVal(m_Chars_Active.GetClientsNumber());
			return true;
		case SC_COLDCHANCE:
			sVal.FormatVal( GetColdChance());
			return true;
		case SC_COMPLEXITY:
			if ( ptcKey[10] == '.' )
			{
				ptcKey += 11;
				sVal = !strcmpi( ptcKey, sm_ComplexityTitles->FindName((int)GetCharComplexity()) ) ? "1" : "0";
				return true;
			}
			sVal.FormatSTVal( GetCharComplexity() );
			return true;
		case SC_FLAGS:
			sVal.FormatHex(m_dwFlags);
			return true;
		case SC_LIGHT:
			sVal.FormatVal(GetLight());
			return true;
		case SC_LOCALTIME:
			sVal = GetLocalGameTime();
			return true;
		case SC_LOCALTOD:
			sVal.FormatVal( GetLocalTime());
			return true;
		case SC_NUMBER:
			sVal.FormatVal(m_index);
			return true;
		case SC_ISDARK:
			sVal.FormatVal( IsDark() );
			return true;
		case SC_ISNIGHTTIME:
			{
				const int iMinutes = GetLocalTime();
				sVal = ( iMinutes < 7*60 || iMinutes > (9+12)*60 ) ? "1" : "0";
			}
			return true;
        case SC_ISSLEEPING:
            sVal.FormatVal(IsSleeping());
            return true;
		case SC_RAINCHANCE:
			sVal.FormatVal( GetRainChance());
			return true;
		case SC_ITEMCOUNT:
			sVal.FormatSTVal(GetItemComplexity());
			return true;
		case SC_SEASON:
			sVal.FormatVal((int)GetSeason());
			return true;
		case SC_WEATHER:
			sVal.FormatVal((int)GetWeather());
			return true;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

void CSector::GoSleep()
{
    ADDTOCALLSTACK("CSector::Sleep");
    const ProfileTask charactersTask(PROFILE_TIMERS);
    CTimedObject::GoSleep();

    CChar * pCharNext = nullptr;
    CChar * pChar = static_cast <CChar*>(m_Chars_Active.GetHead());
    for (; pChar != nullptr; pChar = pCharNext)
    {
		pCharNext = pChar->GetNext();
        if (!pChar->IsSleeping())
            pChar->GoSleep();
    }

    CItem * pItemNext = nullptr;
    CItem * pItem = static_cast <CItem*>(m_Items_Timer.GetHead());
    for (; pItem != nullptr; pItem = pItemNext)
    {
		pItemNext = pItem->GetNext();
        if (!pItem->IsSleeping())
			pItem->GoSleep();
    }
    pItemNext = nullptr;
    pItem = static_cast <CItem*>(m_Items_Inert.GetHead());
    for (; pItem != nullptr; pItem = pItemNext)
    {
        pItemNext = pItem->GetNext();
		if (!pItem->IsSleeping())
	        pItem->GoSleep();
    }
}

void CSector::GoAwake()
{
    ADDTOCALLSTACK("CSector::GoAwake");
    const ProfileTask charactersTask(PROFILE_TIMERS);
    CTimedObject::GoAwake();  // Awake it first, otherwise other things won't work.

    CChar * pCharNext = nullptr;
    CChar * pChar = static_cast <CChar*>(m_Chars_Active.GetHead());
    for (; pChar != nullptr; pChar = pCharNext)
    {
        pCharNext = pChar->GetNext();
        if (pChar->IsSleeping())
            pChar->GoAwake();
    }

    pChar = static_cast<CChar*>(m_Chars_Disconnect.GetHead());
    for (; pChar != nullptr; pChar = pCharNext)
    {
        pCharNext = pChar->GetNext();
        if (pChar->IsSleeping())
            pChar->GoAwake();
    }

    CItem * pItemNext = nullptr;
    CItem * pItem = static_cast <CItem*>(m_Items_Timer.GetHead());
    for (; pItem != nullptr; pItem = pItemNext)
    {
        pItemNext = pItem->GetNext();
		if (pItem->IsSleeping())
        	pItem->GoAwake();
    }
    pItem = static_cast <CItem*>(m_Items_Inert.GetHead());
    for (; pItem != nullptr; pItem = pItemNext)
    {
        pItemNext = pItem->GetNext();
        if (pItem->IsSleeping())
			pItem->GoAwake();
    }

    /*
    * Awake adjacent sectors when awaking this one to avoid the effect
    * of NPCs being stop until you enter the sector, or all the spawns
    * generating NPCs at once.
    */
    static CSector *pCentral = nullptr;   // do this only for the awaken sector
    if (!pCentral)
    {
        pCentral = this;  
        for (int i = 0; i < (int)DIR_QTY; ++i)
        {
            CSector *pSector = GetAdjacentSector((DIR_TYPE)i);
            if (pSector && pSector->IsSleeping())
            {
                pSector->GoAwake();
            }
        }
        pCentral = nullptr;
    }
    OnTick();   // Unknown time passed, make the sector tick now to reflect any possible environ changes.
}

bool CSector::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CSector::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		case SC_COLDCHANCE:
			SetWeatherChance( false, s.HasArgs() ? s.GetArgVal() : -1 );
			return true;
		case SC_FLAGS:
			m_dwFlags = s.GetArgVal();
			return true;
		case SC_LIGHT:
			if ( g_Cfg.m_bAllowLightOverride )
				SetLight( (s.HasArgs()) ? s.GetArgVal() : -1 );
			else
				g_Log.EventWarn("AllowLightOverride flag is disabled in sphere.ini, so sector's LIGHT property wasn't set\n");
			return true;
		case SC_RAINCHANCE:
			SetWeatherChance( true, s.HasArgs() ? s.GetArgVal() : -1 );
			return true;
		case SC_SEASON:
			SetSeason(s.HasArgs() ? static_cast<SEASON_TYPE>(s.GetArgVal()) : SEASON_Summer);
			return (true);
		case SC_WEATHER:
			SetWeather(s.HasArgs() ? static_cast<WEATHER_TYPE>(s.GetArgVal()) : WEATHER_DRY);
			return true;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

enum SEV_TYPE
{
	#define ADD(a,b) SEV_##a,
	#include "../tables/CSector_functions.tbl"
	#undef ADD
	SEV_QTY
};

lpctstr const CSector::sm_szVerbKeys[SEV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CSector_functions.tbl"
	#undef ADD
	nullptr
};

bool CSector::r_Verb( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys)-1 );
	switch (index)
	{
		case SEV_ALLCHARS:		// "ALLCHARS"
			v_AllChars( s, pSrc );
			break;
		case SEV_ALLCHARSIDLE:	// "ALLCHARSIDLE"
			v_AllCharsIdle( s, pSrc );
			break;
		case SEV_ALLCLIENTS:	// "ALLCLIENTS"
			v_AllClients( s, pSrc );
			break;
		case SEV_ALLITEMS:		// "ALLITEMS"
			v_AllItems( s, pSrc );
			break;
        case SEV_AWAKE:
            if (!IsSleeping())
            {
                break;
            }
            GoAwake();
            break;
		case SEV_DRY:	// "DRY"
			SetWeather( WEATHER_DRY );
			break;
		case SEV_LIGHT:
			if ( g_Cfg.m_bAllowLightOverride )
				SetLight( (s.HasArgs()) ? s.GetArgVal() : -1 );
			else
				g_Log.EventWarn("AllowLightOverride flag is disabled in sphere.ini, so sector's LIGHT property wasn't set\n");
			break;
		case SEV_RAIN:
			SetWeather(s.HasArgs() ? static_cast<WEATHER_TYPE>(s.GetArgVal()) : WEATHER_RAIN);
			break;
		case SEV_RESPAWN:
			( toupper( s.GetArgRaw()[0] ) == 'A' ) ? g_World.RespawnDeadNPCs() : RespawnDeadNPCs();
			break;
		case SEV_RESTOCK:	// x
			// set restock time of all vendors in World, set the respawn time of all spawns in World.
			( toupper( s.GetArgRaw()[0] ) == 'A' ) ? g_World.Restock() : Restock();
			break;
		case SEV_SEASON:
			SetSeason(static_cast<SEASON_TYPE>(s.GetArgVal()));
			break;
        case SEV_SLEEP:
            {
                if (IsSleeping())
                {
                    break;
                }
                if (!s.HasArgs())// with no args it will check if it can sleep before, to avoid possible problems.
                {
                    if (!CanSleep(true))
                    {
                        break;
                    }
                }
                GoSleep();
            }
            break;
		case SEV_SNOW:
			SetWeather( WEATHER_SNOW );
			break;
		default:
			return( CScriptObj::r_Verb( s, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

void CSector::r_Write()
{
	ADDTOCALLSTACK_INTENSIVE("CSector::r_Write");
	if ( m_fSaveParity == g_World.m_fSaveParity )
		return; // already saved.
	CPointMap pt = GetBasePoint();

	m_fSaveParity = g_World.m_fSaveParity;
	bool bHeaderCreated = false;

	if ( m_dwFlags > 0)
	{
		g_World.m_FileWorld.WriteSection("SECTOR %d,%d,0,%d", pt.m_x, pt.m_y, pt.m_map );
		g_World.m_FileWorld.WriteKeyHex("FLAGS", m_dwFlags);
		bHeaderCreated = true;
	}

	if (g_Cfg.m_bAllowLightOverride && IsLightOverriden())
	{
		if ( bHeaderCreated == false )
		{
			g_World.m_FileWorld.WriteSection("SECTOR %d,%d,0,%d", pt.m_x, pt.m_y, pt.m_map);
			bHeaderCreated = true;
		}

		g_World.m_FileWorld.WriteKeyVal("LIGHT", GetLight());
	}

	if (!g_Cfg.m_fNoWeather && (IsRainOverriden() || IsColdOverriden()))
	{
		if ( bHeaderCreated == false )
		{
			g_World.m_FileWorld.WriteSection("SECTOR %d,%d,0,%d", pt.m_x, pt.m_y, pt.m_map);
			bHeaderCreated = true;
		}

		if ( IsRainOverriden() )
			g_World.m_FileWorld.WriteKeyVal("RAINCHANCE", GetRainChance());

		if ( IsColdOverriden() )
			g_World.m_FileWorld.WriteKeyVal("COLDCHANCE", GetColdChance());
	}

	if (GetSeason() != SEASON_Summer)
	{
		if ( bHeaderCreated == false )
			g_World.m_FileWorld.WriteSection("SECTOR %d,%d,0,%d", pt.m_x, pt.m_y, pt.m_map);

		g_World.m_FileWorld.WriteKeyVal("SEASON", GetSeason());
	}

	// Chars in the sector.
	CChar *pCharNext = nullptr;
	for ( CChar *pChar = static_cast<CChar*>(m_Chars_Active.GetHead()); pChar != nullptr; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		pChar->r_WriteParity(pChar->m_pPlayer ? g_World.m_FilePlayers : g_World.m_FileWorld);
	}

	// Inactive Client Chars, ridden horses and dead NPCs (NOTE: Push inactive player chars out to the account files here?)
	for ( CChar *pChar = static_cast<CChar*>(m_Chars_Disconnect.GetHead()); pChar != nullptr; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		pChar->r_WriteParity(pChar->m_pPlayer ? g_World.m_FilePlayers : g_World.m_FileWorld);
	}

	// Items on the ground.
	CItem *pItemNext = nullptr;
	for ( CItem *pItem = static_cast<CItem*>(m_Items_Inert.GetHead()); pItem != nullptr; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
        if (pItem->IsTypeMulti())
        {
            pItem->r_WriteSafe(g_World.m_FileMultis);
        }
        else if (!pItem->IsAttr(ATTR_STATIC))
        {
            pItem->r_WriteSafe(g_World.m_FileWorld);
        }
	}

	for ( CItem *pItem = static_cast<CItem*>(m_Items_Timer.GetHead()); pItem != nullptr; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
        if (pItem->IsTypeMulti())
        {
            pItem->r_WriteSafe(g_World.m_FileMultis);
        }
        else if (!pItem->IsAttr(ATTR_STATIC))
        {
            pItem->r_WriteSafe(g_World.m_FileWorld);
        }
	}
}

bool CSector::v_AllChars( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::v_AllChars");
	CScript script(s.GetArgStr());
	script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// Index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
	script.m_iLineNum = s.m_iLineNum;						// Line in the script file where Key/Arg were read
	CChar * pChar = nullptr;
	bool fRet = false;

	// Loop through all the characters in m_Chars_Active.
	// We should start at the end incase some are removed during the loop.
	size_t i = m_Chars_Active.GetCount();
	while ( i > 0 )
	{
		pChar = static_cast <CChar*>(m_Chars_Active.GetAt(--i));

		// Check that a character was returned and keep looking if not.
		if (pChar == nullptr)
			continue;

		// Execute the verb on the character
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllCharsIdle( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::v_AllCharsIdle");
	CScript script(s.GetArgStr());
	script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// Index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
	script.m_iLineNum = s.m_iLineNum;						// Line in the script file where Key/Arg were read
	CChar * pChar = nullptr;
	bool fRet = false;

	// Loop through all the characters in m_Chars_Disconnect.
	// We should start at the end incase some are removed during the loop.
	size_t i = m_Chars_Disconnect.GetCount();
	while ( i > 0 )
	{
		pChar = static_cast <CChar*>(m_Chars_Disconnect.GetAt(--i));

		// Check that a character was returned and keep looking if not.
		if (pChar == nullptr)
			continue;

		// Execute the verb on the character
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllItems( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::v_AllItems");
	CScript script(s.GetArgStr());
	script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// Index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
	script.m_iLineNum = s.m_iLineNum;						// Line in the script file where Key/Arg were read
	CItem * pItem = nullptr;
	bool fRet = false;

	// Loop through all the items in m_Items_Timer.
	// We should start at the end incase items are removed during the loop.
	size_t i = m_Items_Timer.GetCount();
	while ( i > 0 )
	{
		// Get the next item
		pItem = static_cast <CItem*>(m_Items_Timer.GetAt(--i));

		// Check that an item was returned and keep looking if not.
		if (pItem == nullptr)
			continue;

		// Execute the verb on the item
		fRet |= pItem->r_Verb(script, pSrc);
	}

	// Loop through all the items in m_Items_Inert.
	// We should start at the end incase items are removed during the loop.
	i = m_Items_Inert.GetCount();
	while ( i > 0 )
	{
		// Get the next item.
		pItem = static_cast <CItem*>(m_Items_Inert.GetAt(--i));

		// Check that an item was returned and keep looking if not.
		if (pItem == nullptr)
			continue;

		// Execute the verb on the item
		fRet |= pItem->r_Verb(script, pSrc);
	}
	return fRet;
}

bool CSector::v_AllClients( CScript & s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CSector::v_AllClients");
	CScript script(s.GetArgStr());
	script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// Index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
	script.m_iLineNum = s.m_iLineNum;						// Line in the script file where Key/Arg were read
	CChar * pChar = nullptr;
	bool fRet = false;

	// Loop through all the characters in m_Chars_Active.
	// We should start at the end incase some are removed during the loop.
	size_t i = m_Chars_Active.GetCount();
	while ( i > 0 )
	{
		pChar = static_cast <CChar*>(m_Chars_Active.GetAt(--i));

		// Check that a character was returned and keep looking if not.
		if (pChar == nullptr)
			continue;

		// Check that the character is a client (we only want to affect clients with this)
		if ( ! pChar->IsClient())
			continue;

		// Execute the verb on the client
		fRet |= pChar->r_Verb(script, pSrc);
	}
	return fRet;
}

int CSector::GetLocalTime() const
{
	ADDTOCALLSTACK("CSector::GetLocalTime");
	//	Get local time of the day (in minutes)
	CPointMap pt = GetBasePoint();
	int64 iLocalTime = g_World.GetGameWorldTime();

	if ( !g_Cfg.m_bAllowLightOverride )
	{
		iLocalTime += ( pt.m_x * 24*60 ) / g_MapList.GetX(pt.m_map);
	}
	else
	{
		// Time difference between adjacent sectors in minutes
		int iSectorTimeDiff = (24*60) / g_MapList.GetSectorCols(pt.m_map);

		// Calculate the # of columns between here and Castle Britannia ( x = 1400 )
		//int iSectorOffset = ( pt.m_x / g_MapList.GetX(pt.m_map) ) - ( (24*60) / g_MapList.GetSectorSize(pt.m_map));
		int iSectorOffset = ( pt.m_x / g_MapList.GetSectorSize(pt.m_map));

		// Calculate the time offset from global time
		int iTimeOffset = iSectorOffset * iSectorTimeDiff;

		// Calculate the local time
		iLocalTime += iTimeOffset;
	}
	return (iLocalTime % (24*60));
}

lpctstr CSector::GetLocalGameTime() const
{
	ADDTOCALLSTACK("CSector::GetLocalGameTime");
	return CServerTime::GetTimeMinDesc(GetLocalTime());
}

bool CSector::IsMoonVisible(uint iPhase, int iLocalTime) const
{
	ADDTOCALLSTACK("CSector::IsMoonVisible");
	// When is moon rise and moon set ?
	iLocalTime %= (24*60);		// just to make sure (day time in minutes) 1440
	switch (iPhase)
	{
		case 0:	// new moon
			return( (iLocalTime > 360) && (iLocalTime < 1080));
		case 1:	// waxing crescent
			return( (iLocalTime > 540) && (iLocalTime < 1270));
		case 2:	// first quarter
			return( iLocalTime > 720 );
		case 3:	// waxing gibbous
			return( (iLocalTime < 180) || (iLocalTime > 900));
		case 4:	// full moon
			return( (iLocalTime < 360) || (iLocalTime > 1080));
		case 5:	// waning gibbous
			return( (iLocalTime < 540) || (iLocalTime > 1270));
		case 6:	// third quarter
			return( iLocalTime < 720 );
		case 7:	// waning crescent
			return( (iLocalTime > 180) && (iLocalTime < 900));
		default: // How'd we get here?
			return false;
	}
}

byte CSector::GetLightCalc( bool fQuickSet ) const
{
	ADDTOCALLSTACK("CSector::GetLightCalc");
	// What is the light level default here in this sector.

	if ( g_Cfg.m_bAllowLightOverride && IsLightOverriden() )
		return m_Env.m_Light;

	if ( IsInDungeon() )
		return (uchar)(g_Cfg.m_iLightDungeon);

	int localtime = GetLocalTime();

	if ( !g_Cfg.m_bAllowLightOverride )
	{
		//	Normalize time:
		//	convert	0=midnight	.. (23*60)+59=midnight
		//	to		0=day		.. 12*60=night
		if ( localtime < 12*60 )
			localtime = 12*60 - localtime;
		else
			localtime -= 12*60;

		//	0...	y	...lightnight
		//	0...	x	...12*60
		int iTargLight = ((localtime * ( g_Cfg.m_iLightNight - g_Cfg.m_iLightDay ))/(12*60) + g_Cfg.m_iLightDay);

		if ( iTargLight < LIGHT_BRIGHT )
			iTargLight = LIGHT_BRIGHT;
		if ( iTargLight > LIGHT_DARK )
			iTargLight = LIGHT_DARK;

		return (uchar)(iTargLight);
	}

	int hour = ( localtime / ( 60)) % 24;
	bool fNight = ( hour < 6 || hour > 12+8 );	// Is it night or day ?
	int iTargLight = (fNight) ? g_Cfg.m_iLightNight : g_Cfg.m_iLightDay;	// Target light level.

	// Check for clouds...if it is cloudy, then we don't even need to check for the effects of the moons...
	if ( GetWeather())
	{
		// Clouds of some sort...
		if (fNight)
			iTargLight += ( Calc_GetRandVal( 2 ) + 1 );	// 1-2 light levels darker if cloudy at night
		else
			iTargLight += ( Calc_GetRandVal( 4 ) + 1 );	// 1-4 light levels darker if cloudy during the day.
	}

	if ( fNight )
	{
		// Factor in the effects of the moons
		// Trammel
		uint iTrammelPhase = g_World.GetMoonPhase( false );
		// Check to see if Trammel is up here...

		if ( IsMoonVisible( iTrammelPhase, localtime ))
		{
static const byte sm_TrammelPhaseBrightness[] =
{
	0, // New Moon
	TRAMMEL_FULL_BRIGHTNESS / 4,	// Crescent Moon
	TRAMMEL_FULL_BRIGHTNESS / 2, 	// Quarter Moon
	( TRAMMEL_FULL_BRIGHTNESS * 3) / 4, // Gibbous Moon
	TRAMMEL_FULL_BRIGHTNESS,		// Full Moon
	( TRAMMEL_FULL_BRIGHTNESS * 3) / 4, // Gibbous Moon
	TRAMMEL_FULL_BRIGHTNESS / 2, 	// Quarter Moon
	TRAMMEL_FULL_BRIGHTNESS / 4		// Crescent Moon
};
			ASSERT( iTrammelPhase < CountOf(sm_TrammelPhaseBrightness));
			iTargLight -= sm_TrammelPhaseBrightness[iTrammelPhase];
		}

		// Felucca
		uint iFeluccaPhase = g_World.GetMoonPhase( true );
		if ( IsMoonVisible( iFeluccaPhase, localtime ))
		{
static const byte sm_FeluccaPhaseBrightness[] =
{
	0, // New Moon
	FELUCCA_FULL_BRIGHTNESS / 4,	// Crescent Moon
	FELUCCA_FULL_BRIGHTNESS / 2, 	// Quarter Moon
	( FELUCCA_FULL_BRIGHTNESS * 3) / 4, // Gibbous Moon
	FELUCCA_FULL_BRIGHTNESS,		// Full Moon
	( FELUCCA_FULL_BRIGHTNESS * 3) / 4, // Gibbous Moon
	FELUCCA_FULL_BRIGHTNESS / 2, 	// Quarter Moon
	FELUCCA_FULL_BRIGHTNESS / 4		// Crescent Moon
};
			ASSERT( iFeluccaPhase < CountOf(sm_FeluccaPhaseBrightness));
			iTargLight -= sm_FeluccaPhaseBrightness[iFeluccaPhase];
		}
	}

	if ( iTargLight < LIGHT_BRIGHT )
		iTargLight = LIGHT_BRIGHT;
	if ( iTargLight > LIGHT_DARK )
		iTargLight = LIGHT_DARK;

	if ( fQuickSet || m_Env.m_Light == iTargLight )		// Initializing the sector
		return (uchar)(iTargLight);

	// Gradual transition to global light level.
	if ( m_Env.m_Light > iTargLight )
		return ( m_Env.m_Light - 1 );
	else
		return ( m_Env.m_Light + 1 );
}

void CSector::SetLightNow( bool fFlash )
{
	ADDTOCALLSTACK("CSector::SetLightNow");
	// Set the light level for all the CClients here.

	CChar * pChar = static_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != nullptr; pChar = pChar->GetNext())
	{
		if ( pChar->IsStatFlag( STATF_DEAD | STATF_NIGHTSIGHT ))
			continue;

		if ( pChar->IsClient())
		{
			CClient * pClient = pChar->GetClient();
			ASSERT(pClient);

			if ( fFlash )	// This does not seem to work predicably ! too fast?
			{
				byte bPrvLight = pChar->m_LocalLight;
				pChar->m_LocalLight = LIGHT_BRIGHT;	// full bright.
				pClient->addLight();
				pChar->m_LocalLight = bPrvLight;	// back to previous.
			}
			pClient->addLight();
		}

		// don't fire trigger when server is loading or light is flashing
		if (( ! g_Serv.IsLoading() && fFlash == false ) && ( IsTrigUsed(TRIGGER_ENVIRONCHANGE) ))
		{
			pChar->OnTrigger( CTRIG_EnvironChange, pChar );
		}
	}
}

void CSector::SetLight( int light )
{
	ADDTOCALLSTACK("CSector::SetLight");
	// GM set light level command
	// light =LIGHT_BRIGHT, LIGHT_DARK=dark

	if ( light < LIGHT_BRIGHT || light > LIGHT_DARK )
	{
		m_Env.m_Light &= ~LIGHT_OVERRIDE;
		m_Env.m_Light = (byte) GetLightCalc( true );
	}
	else
		m_Env.m_Light = (byte) ( light | LIGHT_OVERRIDE );

	SetLightNow(false);
}

void CSector::SetDefaultWeatherChance()
{
	ADDTOCALLSTACK("CSector::SetDefaultWeatherChance");
	CPointMap pt = GetBasePoint();
	byte iPercent = (byte)(IMulDiv( pt.m_y, 100, g_MapList.GetY(pt.m_map) ));	// 100 = south
	if ( iPercent < 50 )
	{
		// Anywhere north of the Britain Moongate is a good candidate for snow
		m_ColdChance = 1 + ( 49 - iPercent ) * 2;
		m_RainChance = 15;
	}
	else
	{
		// warmer down south
		m_ColdChance = 1;
		// rain more likely down south.
		m_RainChance = 15 + ( iPercent - 50 ) / 10;
	}
}

WEATHER_TYPE CSector::GetWeatherCalc() const
{
	ADDTOCALLSTACK("CSector::GetWeatherCalc");

	// There is no weather in dungeons (or when the Sphere.ini setting prevents weather)
	if ( IsInDungeon() || g_Cfg.m_fNoWeather )
		return( WEATHER_DRY );

	// Rain chance also controls the chance of snow. If it isn't possible to rain then it cannot snow either
	int iPercentRoll = Calc_GetRandVal( 100 );
	if ( iPercentRoll < GetRainChance() )
	{
		// It is precipitating... but is it rain or snow?
		if ( GetColdChance() && Calc_GetRandVal(100) <= GetColdChance()) // Should it actually snow here?
			return WEATHER_SNOW;
		return WEATHER_RAIN;
	}
	// It is not precipitating, but is it cloudy or dry?
	if ( ( iPercentRoll / 2) < GetRainChance() )
		return WEATHER_CLOUDY;
	return( WEATHER_DRY );
}

void CSector::SetWeather( WEATHER_TYPE w )
{
	ADDTOCALLSTACK("CSector::SetWeather");
	// Set the immediate weather type.
	// 0=dry, 1=rain or 2=snow.

	if ( w == m_Env.m_Weather )
		return;

	m_Env.m_Weather = w;

	CChar * pChar = static_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != nullptr; pChar = pChar->GetNext())
	{
		if ( pChar->IsClient())
			pChar->GetClient()->addWeather( w );

		if ( IsTrigUsed(TRIGGER_ENVIRONCHANGE) )
			pChar->OnTrigger( CTRIG_EnvironChange, pChar );
	}
}

void CSector::SetSeason( SEASON_TYPE season )
{
	ADDTOCALLSTACK("CSector::SetSeason");
	// Set the season type.

	if ( season == m_Env.m_Season )
		return;

	m_Env.m_Season = season;

	CChar * pChar = static_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != nullptr; pChar = pChar->GetNext())
	{
		if ( pChar->IsClient() )
			pChar->GetClient()->addSeason(season);

		if ( IsTrigUsed(TRIGGER_ENVIRONCHANGE) )
			pChar->OnTrigger(CTRIG_EnvironChange, pChar);
	}
}

void CSector::SetWeatherChance( bool fRain, int iChance )
{
	ADDTOCALLSTACK("CSector::SetWeatherChance");
	// Set via the client.
	// Transfer from snow to rain does not work ! must be DRY first.

	if ( iChance > 100 )
		iChance = 100;
	if ( iChance < 0 )
	{
		// just set back to defaults.
		SetDefaultWeatherChance();
	}
	else if ( fRain )
	{
		m_RainChance = (uchar)(iChance | LIGHT_OVERRIDE);
	}
	else
	{
		m_ColdChance = (uchar)(iChance | LIGHT_OVERRIDE);
	}

	// Recalc the weather immediatly.
	SetWeather( GetWeatherCalc());
}

void CSector::OnHearItem( CChar * pChar, lpctstr pszText )
{
	ADDTOCALLSTACK("CSector::OnHearItem");
	// report to any of the items that something was said.

	ASSERT(m_ListenItems);

	CItem * pItemNext;
	CItem * pItem = static_cast <CItem*>( m_Items_Timer.GetHead());
	for ( ; pItem != nullptr; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->OnHear( pszText, pChar );
	}
	pItem = static_cast <CItem*>( m_Items_Inert.GetHead());
	for ( ; pItem != nullptr; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->OnHear( pszText, pChar );
	}
}

void CSector::MoveItemToSector( CItem * pItem, bool fActive )
{
	ADDTOCALLSTACK("CSector::MoveItemToSector");
	// remove from previous list and put in new.
	// May just be setting a timer. SetTimer or MoveTo()
	ASSERT( pItem );
    if (IsSleeping())
    {
        if (CanSleep(true))
        {
            pItem->GoSleep();
        }
        else
        {
            GoAwake();
            if (pItem->IsSleeping())
                pItem->GoAwake();
        }
    }
    else
    {
        if (pItem->IsSleeping())
            pItem->GoAwake();
    }
	if ( fActive )
		m_Items_Timer.AddItemToSector( pItem );
	else
		m_Items_Inert.AddItemToSector( pItem );
}

bool CSector::MoveCharToSector( CChar * pChar )
{
	ADDTOCALLSTACK("CSector::MoveCharToSector");
	// Move a CChar into this CSector.
    ASSERT(pChar);

	if ( IsCharActiveIn(pChar) )
		return false;	// already here

	// Check my save parity vs. this sector's
	if ( pChar->IsStatFlag( STATF_SAVEPARITY ) != m_fSaveParity )
	{
		if ( g_World.IsSaving())
		{
			if ( m_fSaveParity == g_World.m_fSaveParity )
			{
				// Save out the CChar now. the sector has already been saved.
				if ( pChar->m_pPlayer )
					pChar->r_WriteParity(g_World.m_FilePlayers);
				else
					pChar->r_WriteParity(g_World.m_FileWorld);
			}
			else
			{
				// We need to write out this CSector now. (before adding client)
				r_Write();
			}
		}
	}

    m_Chars_Active.AddCharActive(pChar);	// remove from previous spot.
    if (IsSleeping())
    {
        CClient *pClient = pChar->GetClient();
        if (pClient)    // A client just entered
        {
            GoAwake();    // Awake the sector and the chars inside (so, also pChar)
            ASSERT(!pChar->IsSleeping());
        }
        else if (!pChar->IsSleeping())    // An NPC entered, but the sector is sleeping
        {
            pChar->GoSleep(); // then make the NPC sleep too.
        }
    }
    else
    {
        if (pChar->IsSleeping())
        {
            pChar->GoAwake();
        }
    }
	
	return true;
}

bool CSector::CanSleep(bool fCheckAdjacents) const
{
	ADDTOCALLSTACK_INTENSIVE("CSector::CanSleep");
	if ( (g_Cfg._iSectorSleepDelay == 0) || IsFlagSet(SECF_NoSleep) )
		return false;	// never sleep
    if (m_Chars_Active.GetClientsNumber() > 0)
        return false;	// has at least one client, no sleep
	if ( IsFlagSet(SECF_InstaSleep) )
		return true;	// no active client inside, instant sleep
    
    if (fCheckAdjacents)
    {
        for (int i = 0; i < (int)DIR_QTY; ++i)// Check for adjacent's sectors sleeping allowance.
        {
            const CSector *pAdjacent = GetAdjacentSector((DIR_TYPE)i);    // set this as the last sector to avoid this code in the adjacent one and return if it can sleep or not instead of searching its adjacents.
            /*
            * Only check if this sector exist and it's not the last checked (sectors in the edges of the map doesn't have adjacent on those directions)
            * && Only check if the sector isn't sleeping (IsSleeping()) and then check if CanSleep().
            */
            if (!pAdjacent)
            {
                continue;
            }
            if (!pAdjacent->IsSleeping() || !pAdjacent->CanSleep(false))
            {
                return false;   // assume the base sector can't sleep.
            }
        }
    }

	//default behaviour;
	return ((g_World.GetCurrentTime().GetTimeRaw() - GetLastClientTime()) > g_Cfg._iSectorSleepDelay); // Sector Sleep timeout.
}

void CSector::SetSectorWakeStatus()
{
	ADDTOCALLSTACK("CSector::SetSectorWakeStatus");
	// Ships may enter a sector before it's riders ! ships need working timers to move !
	m_Chars_Active.m_timeLastClient = g_World.GetCurrentTime().GetTimeRaw();
    if (IsSleeping())
    {
        GoAwake();
    }
}

void CSector::Close()
{
	ADDTOCALLSTACK("CSector::Close");
	// Clear up all dynamic data for this sector.
	m_Items_Timer.Clear();
	m_Items_Inert.Clear();
	m_Chars_Active.Clear();
	m_Chars_Disconnect.Clear();

	// These are resource type things.
	// m_Teleports.RemoveAll();
	// m_RegionLinks.Empty();	// clear the link list.
}

void CSector::RespawnDeadNPCs()
{
	ADDTOCALLSTACK("CSector::RespawnDeadNPCs");
	// skip sectors in unsupported maps
	if ( !g_MapList.IsMapSupported(m_map) )
        return;

	// Respawn dead NPC's
	CChar * pCharNext;
	CChar * pChar = static_cast <CChar *>( m_Chars_Disconnect.GetHead());
	for ( ; pChar != nullptr; pChar = pCharNext )
	{
		pCharNext = pChar->GetNext();
		if ( ! pChar->m_pNPC )
			continue;
		if ( ! pChar->m_ptHome.IsValidPoint())
			continue;
		if ( ! pChar->IsStatFlag( STATF_DEAD ))
			continue;

		// Restock them with npc stuff.
		pChar->NPC_LoadScript(true);

		// Res them back to their "home".
		ushort uiDist = pChar->m_pNPC->m_Home_Dist_Wander;
		pChar->MoveNear( pChar->m_ptHome, uiDist );
		pChar->NPC_CreateTrigger(); //Removed from NPC_LoadScript() and triggered after char placement
		pChar->Spell_Resurrection();
	}
}

void CSector::Restock()
{
    ADDTOCALLSTACK("CSector::Restock");
    // ARGS: iTime = time in seconds
    // set restock time of all vendors in Sector.
    // set the respawn time of all spawns in Sector.

    CChar * pCharNext;
    CChar * pChar = dynamic_cast <CChar*>(m_Chars_Active.GetHead());
    for (; pChar; pChar = pCharNext)
    {
        pCharNext = pChar->GetNext();
        if (pChar->m_pNPC)
        {
            pChar->NPC_Vendor_Restock(true);
        }
    }

    CItem * pItemNext;
    CItem * pItem = dynamic_cast <CItem*>(m_Items_Timer.GetHead());
    for (; pItem; pItem = pItemNext)
    {
        pItemNext = pItem->GetNext();
        if (pItem->IsType(IT_SPAWN_ITEM) || pItem->IsType(IT_SPAWN_CHAR) || pItem->IsType(IT_SPAWN_CHAMPION))
        {
            CCSpawn *pSpawn = pItem->GetSpawn();
            if (pSpawn)
            {
                pSpawn->OnTickComponent();
            }
        }
    }
}

bool CSector::OnTick()
{
	ADDTOCALLSTACK("CSector::OnTick");
	/*Ticking sectors from CWorld
    * Timer is automatically updated at the end with a 30 seconds default delay
    * Any return before it will threat this CSector as Sleep and will make it
    * not tick again until a new player enters (WARNING: even if there are
    * players already inside).
    */

	EXC_TRY("Tick");

	//	do not tick sectors on maps not supported by server
	if ( !g_MapList.IsMapSupported(m_map) )
		return true;

    EXC_SET_BLOCK("light change");
	// Check for light change before putting the sector to sleep, since in other case the
	// world light levels will be shitty
	bool fEnvironChange = false;
	bool fLightChange = false;

	// check for local light level change ?
	byte bLightPrv = m_Env.m_Light;
	m_Env.m_Light = GetLightCalc( false );
	if ( m_Env.m_Light != bLightPrv )
	{
		fEnvironChange = true;
		fLightChange = true;
	}

	EXC_SET_BLOCK("sector sleeping?");
	// Put the sector to sleep if no clients been here in a while.
    const bool fCanSleep = CanSleep(true);	
	if (fCanSleep && (g_Cfg._iSectorSleepDelay > 0))
	{
        if (!IsSleeping())
        {
            GoSleep();
        }
		return true;
	}


	EXC_SET_BLOCK("sound effects");
	// random weather noises and effects.
	SOUND_TYPE sound = 0;
	bool fWeatherChange = false;
	int iRegionPeriodic = 0;

	WEATHER_TYPE weatherprv = m_Env.m_Weather;
	if ( ! Calc_GetRandVal( 30 ))	// change less often
	{
		m_Env.m_Weather = GetWeatherCalc();
		if ( weatherprv != m_Env.m_Weather )
		{
			fWeatherChange = true;
			fEnvironChange = true;
		}
	}

	// Random area noises. Only do if clients about.
	if ( GetClientsNumber() > 0 )
	{
		iRegionPeriodic = 2;

		static const SOUND_TYPE sm_SfxRain[] = { 0x10, 0x11 };
		static const SOUND_TYPE sm_SfxWind[] = { 0x14, 0x15, 0x16 };
		static const SOUND_TYPE sm_SfxThunder[] = { 0x28, 0x29 , 0x206 };

		// Lightning ?	// wind, rain,
		switch ( GetWeather() )
		{
			case WEATHER_CLOUDY:
				break;

			case WEATHER_SNOW:
				if ( ! Calc_GetRandVal(5) )
					sound = sm_SfxWind[ Calc_GetRandVal( CountOf( sm_SfxWind )) ];
				break;

			case WEATHER_RAIN:
				{
					int iVal = Calc_GetRandVal(30);
					if ( iVal < 5 )
					{
						// Mess up the light levels for a sec..
						LightFlash();
						sound = sm_SfxThunder[ Calc_GetRandVal( CountOf( sm_SfxThunder )) ];
					}
					else if ( iVal < 10 )
						sound = sm_SfxRain[ Calc_GetRandVal( CountOf( sm_SfxRain )) ];
					else if ( iVal < 15 )
						sound = sm_SfxWind[ Calc_GetRandVal( CountOf( sm_SfxWind )) ];
				}
				break;

			default:
				break;
		}
	}

    // Check environ changes and inform clients of it.
	const ProfileTask charactersTask(PROFILE_CHARS);

	CChar * pCharNext = nullptr;
	CChar * pChar = static_cast <CChar*>( m_Chars_Active.GetHead());
	for ( ; pChar != nullptr; pChar = pCharNext )
	{
		EXC_TRYSUB("TickChar");

        ASSERT(pChar);
		pCharNext = pChar->GetNext();

		if (fEnvironChange && ( IsTrigUsed(TRIGGER_ENVIRONCHANGE) ))
			pChar->OnTrigger(CTRIG_EnvironChange, pChar);

		if ( pChar->IsClient())
		{
			CClient * pClient = pChar->GetClient();
			ASSERT( pClient );
			if ( sound )
				pClient->addSound(sound, pChar);

			if ( fLightChange && ! pChar->IsStatFlag( STATF_DEAD | STATF_NIGHTSIGHT ))
				pClient->addLight();

			if ( fWeatherChange )
				pClient->addWeather(GetWeather());

			if ( iRegionPeriodic && pChar->m_pArea )
			{
				if ( ( iRegionPeriodic == 2 ) && IsTrigUsed(TRIGGER_REGPERIODIC))
				{
					pChar->m_pArea->OnRegionTrigger( pChar, RTRIG_REGPERIODIC );
					--iRegionPeriodic;
				}
				if ( IsTrigUsed(TRIGGER_CLIPERIODIC) )
					pChar->m_pArea->OnRegionTrigger( pChar, RTRIG_CLIPERIODIC );
			}
		}

		EXC_CATCHSUB("Sector");

		EXC_DEBUGSUB_START;
		CPointMap pt = GetBasePoint();
		g_Log.EventDebug("#0 char 0%x '%s'\n", (dword)(pChar->GetUID()), pChar->GetName());
		g_Log.EventDebug("#0 sector #%d [%d,%d,%d,%d]\n", GetIndex(),  pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		EXC_DEBUGSUB_END;
	}

	EXC_CATCH;

    SetTimeoutS(30);  // Sector is Awake, make it tick after 30 seconds.

	EXC_DEBUG_START;
	CPointMap pt = GetBasePoint();
	g_Log.EventError("#4 sector #%d [%hd,%hd,%hhd,%hhu]\n", GetIndex(), pt.m_x, pt.m_y, pt.m_z, pt.m_map);
	EXC_DEBUG_END;
    return true;
}


SEASON_TYPE CSector::GetSeason() const
{
	return m_Env.m_Season;
}

// Weather
WEATHER_TYPE CSector::GetWeather() const	// current weather.
{
	return m_Env.m_Weather;
}

bool CSector::IsRainOverriden() const
{
	return(( m_RainChance & LIGHT_OVERRIDE ) ? true : false );
}

byte CSector::GetRainChance() const
{
	return( m_RainChance &~ LIGHT_OVERRIDE );
}

bool CSector::IsColdOverriden() const
{
	return(( m_ColdChance & LIGHT_OVERRIDE ) ? true : false );
}

byte CSector::GetColdChance() const
{
	return( m_ColdChance &~ LIGHT_OVERRIDE );
}

// Light
bool CSector::IsLightOverriden() const
{
	return(( m_Env.m_Light & LIGHT_OVERRIDE ) ? true : false );
}

byte CSector::GetLight() const
{
	return( m_Env.m_Light &~ LIGHT_OVERRIDE );
}

bool CSector::IsDark() const
{
	return( GetLight() > 6 );
}

bool CSector::IsNight() const
{
	int iMinutes = GetLocalTime();
	return( iMinutes < 7*60 || iMinutes > (9+12)*60 );
}

void CSector::LightFlash()
{
	SetLightNow( true );
}

size_t CSector::GetItemComplexity() const
{
	return m_Items_Timer.GetCount() + m_Items_Inert.GetCount();
}

bool CSector::IsItemInSector( const CItem * pItem ) const
{
	if ( !pItem )
		return false;

	return pItem->GetParent() == &m_Items_Inert ||
		pItem->GetParent() == &m_Items_Timer;
}

void CSector::AddListenItem()
{
	m_ListenItems++;
}

void CSector::RemoveListenItem()
{
	m_ListenItems--;
}

bool CSector::HasListenItems() const
{
	return m_ListenItems ? true : false;
}

bool CSector::IsCharActiveIn( const CChar * pChar ) //const
{
	// assume the char is active (not disconnected)
	return( pChar->GetParent() == &m_Chars_Active );
}

bool CSector::IsCharDisconnectedIn( const CChar * pChar ) //const
{
	// assume the char is active (not disconnected)
	return( pChar->GetParent() == &m_Chars_Disconnect );
}

size_t CSector::GetCharComplexity() const
{
	return( m_Chars_Active.GetCount());
}

size_t CSector::GetInactiveChars() const
{
	return( m_Chars_Disconnect.GetCount());
}

size_t CSector::GetClientsNumber() const
{
	return( m_Chars_Active.GetClientsNumber());
}

int64 CSector::GetLastClientTime() const
{
	return( m_Chars_Active.m_timeLastClient );
}

