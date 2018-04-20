

#include "CChampion.h"	// predef header.
#include "chars/CChar.h"
#include "../common/CScript.h"
#include "../common/CResourceBase.h"
#include "../common/CException.h"

/*
* @file Champion.cpp
*
* @brief This file contains CItemChampion and CChampionDef classes declarations
*
* CItemChampion class manages a CItem making it to work as a Champion
* class is declared inside CObjBase.h
*
* CChampionDef class is used to store all the data placed in scripts under the [CHAMPION ] resource
* this data is read by CItemChampion to know Champion's type, npcs to spawn at each level, boss id....
* class is declared inside CResource.h
*/

CItemChampion::CItemChampion(ITEMID_TYPE id, CItemBase * pItemDef):	CItemMulti( id, pItemDef )
{
	ADDTOCALLSTACK("CItemChampion::CItemChampion");
    g_Log.EventDebug("Constructor 1\n");
	_fActive		= false;
	m_RedCandles	= 0;	
	m_WhiteCandles	= 0;	
	m_LevelMax		= 4;
	m_Level			= 0;
	m_iSpawnsMax	= 2368;
	m_iSpawnsCur	= 0;
	m_iDeathCount	= 0;
    m_iSpawnsNextWhite = 0;
    m_iSpawnsNextRed = 0;
};

CItemChampion::~CItemChampion()
{
	ADDTOCALLSTACK("CItemChampion::~CItemChampion");;
	Stop();
};

bool CItemChampion::OnTick()
{
	ADDTOCALLSTACK("CItemChampion::OnTick");
	if ( m_RedCandles > 1)
		m_RedCandles--;
	else
		Stop();
	SetTimeout(TICK_PER_SEC*60*10);	//10 minutes
	return true;
};

void CItemChampion::Init()
{
	ADDTOCALLSTACK("CItemChampion::Init");
	_fActive = true;
	SetLevel(1);
	for (unsigned char i = 0; i < m_iSpawnsNextWhite; i++)
		SpawnNPC();
};

void CItemChampion::Stop()
{
	ADDTOCALLSTACK("CItemChampion::Stop");
	KillChildren();
	_fActive = false;
	m_iSpawnsCur = 0;
	m_iDeathCount = 0;
	m_Level	= 0;
	//m_iSpawnsNext = 0;
};

void CItemChampion::Complete()
{
	ADDTOCALLSTACK("CItemChampion::Complete");
	Stop();	//Cleaning everything, just to be sure.
};

void CItemChampion::OnKill()
{
	ADDTOCALLSTACK("CItemChampion::OnKill");
	if ( m_iSpawnsNextWhite == 0 )
		AddWhiteCandle();
	else
		SpawnNPC();
}
void CItemChampion::SpawnNPC()
{
	ADDTOCALLSTACK("CItemChampion::SpawnNPC");

	if ( m_Level == 5)	// Already summoned the Champion, stop
		return;

    CResourceIDBase	rid = m_itSpawnChar.m_CharID;
	g_Log.EventDebug("SpawnNPC = %d\n",rid);
	CREID_TYPE pNpc = (CREID_TYPE)NULL;
	if (  m_RedCandles == 16 && m_iSpawnsNextWhite == 0) // Reached 16th red candle and spawned all the normal npcs, so next one should be the champion
	{
		pNpc = static_cast< CChampionDef* >( g_Cfg.ResourceGetDef( rid ) )->m_Champion;//Call to CChampionDef->m_Champion
	}
	else
	{
		m_iSpawnsNextWhite--;
		if ( rid.GetResType() == RES_CHAMPION )
			g_Log.EventDebug("champion!!!!!!!!! = %d - 0%lx - 0%lx\n", rid.GetResIndex(),rid.GetObjUID(),rid.GetPrivateUID());
		else
			g_Log.EventDebug("type = %d - %d - 0%lx - 0%lx\n", rid.GetResType(), rid.GetResIndex(),rid.GetObjUID(),rid.GetPrivateUID());
		CResourceDef * pDef = g_Cfg.ResourceGetDef( rid );
		if (!pDef)
		{
			g_Log.EventDebug( "nanai1\n" );
			return;
		}
		CChampionDef * pChamp =static_cast< CChampionDef* >(pDef);
		if (!pChamp)
		{
			g_Log.EventDebug( "nanai2\n" );
			return;
		}
        uint8 iLevel = m_Level;
        uint8 iRand = (uint8)Calc_GetRandVal2(1,4);
		while ( iRand > 0 && !pChamp->m_iSpawn[iLevel][iRand] )
			iRand--;	//Decrease iRand to 1 in case not all spots are filled
		pNpc = pChamp->m_iSpawn[iLevel][iRand];	//Find out the random npc.
		g_Log.EventDebug("pNpc = %s",rid.CharFind()->GetName());
		if ( !pNpc )	// At least one NPC per level should be added, check just in case.
			return;
	}
	CChar * pChar = CChar::CreateBasic( pNpc );
	if ( !pChar )
		return;
	pChar->SetTopPoint( GetTopPoint() );
	pChar->MoveNear( GetTopPoint(), m_itNormal.m_morep.m_z);
	AddObj( pChar->GetUID() );

};

/*[11:52:29] Kayla Bradshaw: @AddCandle
 ArgN1= 0 = no candle, 1 = Red candle, 2 = white candle
 ArgN2= Amount to add
@RemoveCandle
 ArgN1= 0 = no candle, 1 = Red candle, 2 = white candle
 ArgN2= Amount to remove


Just an idea for you and your code
[11:53:15] Kayla Bradshaw: Local.ID= could be what to use for a candle
*/
void CItemChampion::AddWhiteCandle( CUID uid )
{
	ADDTOCALLSTACK("CItemChampion::AddWhiteCandle");
	if ( m_WhiteCandles == 4 )
	{
		AddRedCandle();
		return;
	}
	else
		m_iSpawnsNextWhite = m_iSpawnsNextRed / 5;

	CItem * pCandle = NULL;
	if ( uid != static_cast<CUID>(UID_UNUSED) )
		pCandle = uid.ItemFind();

	m_WhiteCandles++;
	if ( !pCandle )
	{
		pCandle = CreateBase(ITEMID_SKULL_CANDLE);
		if (!pCandle)
			return;
		CPointMap pt = GetTopPoint();
		switch (m_WhiteCandles)
		{
			case 1:
				pt.MoveN(DIR_SW, 1);
				break;
			case 2:
				pt.MoveN(DIR_SE, 1);
				break;
			case 3:
				pt.MoveN(DIR_NW, 1);
				break;
			case 4:
				pt.MoveN(DIR_NE, 1);
				break;
			default:
				break;
		}
		pCandle->SetAttr(ATTR_MOVE_NEVER);
		pCandle->MoveTo(pt);
		pCandle->SetTopZ(pCandle->GetTopZ() + 4);
		pCandle->Update();
		pCandle->GenerateScript(NULL);
	}
	m_WhiteCandle[m_WhiteCandles] = pCandle;
	pCandle->m_uidLink = GetUID();	// Link it to the champion, so if it gets removed the candle will be removed too
};

void CItemChampion::AddRedCandle( CUID uid )
{
	ADDTOCALLSTACK("CItemChampion::AddRedCandle");
	CItem * pCandle = NULL;
	if ( uid != static_cast<CUID>(UID_UNUSED) )
		pCandle = uid.ItemFind();

	if ( m_RedCandles >= 14 && m_Level < 4)
		SetLevel(4);
	else if ( m_RedCandles >= 10 && m_Level < 3)
		SetLevel(3);
	else if ( m_RedCandles >= 6 && m_Level < 2)
		SetLevel(2);
	m_RedCandles++;
	m_iSpawnsNextWhite = m_iSpawnsNextRed / 5;
	if ( !g_Serv.IsLoading() )	// White candles may be created before red ones when restoring items from worldsave we must not remove them.
		ClearWhiteCandles();

	if ( !pCandle )
	{
		pCandle = CreateBase(ITEMID_SKULL_CANDLE);
		if (!pCandle)
			return;
		CPointMap pt = GetTopPoint();
		switch (m_RedCandles)
		{
			case 1:
				pt.MoveN(DIR_NW, 2);
				break;
			case 2:
				pt.MoveN(DIR_N, 2);
				pt.MoveN(DIR_W, 1);
				break;
			case 3:
				pt.MoveN(DIR_N, 2);
				break;
			case 4:
				pt.MoveN(DIR_N, 2);
				pt.MoveN(DIR_E, 1);
				break;
			case 5:
				pt.MoveN(DIR_NE, 2);
				break;
			case 6:
				pt.MoveN(DIR_E, 2);
				pt.MoveN(DIR_N, 1);
				break;
			case 7:
				pt.MoveN(DIR_E, 2);
				break;
			case 8:
				pt.MoveN(DIR_E, 2);
				pt.MoveN(DIR_S, 1);
				break;
			case 9:
				pt.MoveN(DIR_SE, 2);
				break;
			case 10:
				pt.MoveN(DIR_S, 2);
				pt.MoveN(DIR_E, 1);
				break;
			case 11:
				pt.MoveN(DIR_S, 2);
				break;
			case 12:
				pt.MoveN(DIR_S, 2);
				pt.MoveN(DIR_W, 1);
				break;
			case 13:
				pt.MoveN(DIR_SW, 2);
				break;
			case 14:
				pt.MoveN(DIR_W, 2);
				pt.MoveN(DIR_S, 1);
				break;
			case 15:
				pt.MoveN(DIR_W, 2);
				break;
			case 16:
				pt.MoveN(DIR_W, 2);
				pt.MoveN(DIR_N, 1);
				break;
			default:
				break;
		}
		pCandle->SetAttr(ATTR_MOVE_NEVER);
		pCandle->MoveTo(pt);
		pCandle->SetTopZ(pCandle->GetTopZ() + 4);
		pCandle->SetHue(static_cast<HUE_TYPE>(33));
		pCandle->Update();
		pCandle->GenerateScript(NULL);
	}
	m_RedCandle[m_RedCandles] = pCandle;
	pCandle->m_uidLink = GetUID();	// Link it to the champion, so if it gets removed the candle will be removed too

};

void CItemChampion::SetLevel( BYTE iLevel )
{
	ADDTOCALLSTACK("CItemChampion::SetLevel");
	if ( iLevel > 4 )
		iLevel = 4;
	else if ( iLevel < 1 )
		iLevel = 1;
	m_Level = iLevel;
	unsigned short iLevelMonsters = GetMonstersPerLevel( m_iSpawnsMax );
	m_itNormal.m_morep.m_x = GetCandlesPerLevel();
	unsigned short iRedMonsters = iLevelMonsters / m_itNormal.m_morep.m_x;
	unsigned short iWhiteMonsters = iRedMonsters / 5;
	m_iSpawnsNextWhite = iWhiteMonsters;
	m_iSpawnsNextRed = iRedMonsters;
	SetTimeout(TICK_PER_SEC*60*10);	//10 minutes
};

BYTE CItemChampion::GetCandlesPerLevel()
{
	ADDTOCALLSTACK("CItemChampion::GetCandlesPerLevel");
	switch (m_Level)
	{
	case 4:
		return 2;
	case 3:
		return 4;
	case 2:
		return 4;
	default:
	case 1:
		return 6;
	}
	return 2;
};

unsigned short CItemChampion::GetMonstersPerLevel( unsigned short iMonsters )
{
	ADDTOCALLSTACK("CItemChampion::GetMonstersPerLevel");
	switch (m_Level)
	{
		case 4:
			return (7 * iMonsters) / 100;
		case 3:
			return (13 * iMonsters) / 100;
		case 2:
			return (27 * iMonsters) / 100;
		default:
		case 1:
			return (53 * iMonsters) / 100;
	}
};

// Delete the last created white candle.
void CItemChampion::DelWhiteCandle()
{
	ADDTOCALLSTACK("CItemChampion::DelWhiteCandle");
	if ( m_WhiteCandles == 0 )
		return;
	CItem * pCandle = m_WhiteCandle[m_WhiteCandles];
	pCandle->Delete();
	m_WhiteCandles--;
	return;
};

// Delete the last created red candle.
void CItemChampion::DelRedCandle()
{
	ADDTOCALLSTACK("CItemChampion::DelRedCandle");
	if ( m_RedCandles == 0 )
		return;
	CItem * pCandle = m_RedCandle[m_RedCandles];
	pCandle->Delete();
	m_RedCandles--;
};

// Clear all white candles.
void CItemChampion::ClearWhiteCandles()
{
	ADDTOCALLSTACK("CItemChampion::ClearWhiteCandles");
	if ( m_WhiteCandles == 0 )
		return;
	BYTE iCount = 0;
	for ( CItem * pCandle = m_WhiteCandle[iCount]; pCandle != NULL; iCount++ )
	{
		pCandle->Delete();
		m_WhiteCandles--;
	}
};

// Clear all red candles.
void CItemChampion::ClearRedCandles()
{
	ADDTOCALLSTACK("CItemChampion::ClearRedCandles");
	if ( m_RedCandles == 0 )
		return;
	BYTE iCount = 0;
	for ( CItem * pCandle = m_RedCandle[iCount]; pCandle != NULL ; iCount++ )
	{
		pCandle->Delete();
		m_RedCandles--;
	}
};


// kill everything spawned from this spawn !
void CItemChampion::KillChildren()
{
	ADDTOCALLSTACK("CitemSpawn:KillChildren");
	WORD iTotal = static_cast<WORD>( m_itNormal.m_more2) ; //m_itSpawnItem doesn't have m_current, it uses more2 to set the amount of items spawned in each tick, so i'm using its amount to perform the loop
	if ( iTotal <= 0 )
		return;
	for ( WORD i = 0; i < iTotal; i++ )
	{
		if ( !m_obj[i].IsValidUID() )
			continue;
		CChar * pChar = m_obj[i].CharFind();
		if ( pChar )
		{
			pChar->Delete();
			m_obj[i].InitUID();
		}

	}
	m_itNormal.m_more2 = 0;
}
// Deleting one object from Spawn's memory, reallocating memory automatically.
void CItemChampion::DelObj( CUID uid )
{
	ADDTOCALLSTACK("CitemSpawn:DelObj");
	if ( !uid.IsValidUID() )
		return;
	for ( unsigned char i = 0; i < m_itNormal.m_more2; i++ )
	{
		if ( m_obj[i].IsValidUID() && m_obj[i] == uid )	// found this uid, proceeding to clear it
		{
			m_itNormal.m_more2--;	// found this UID in the spawn's list, decreasing CChar's count only in this case
			while ( m_obj[i + 1].IsValidUID() )			// searching for any entry higher than this one...
			{
				m_obj[i] = m_obj[i + 1];	// and moving it 1 position to keep values 'together'.
				i++;
			}
			m_obj[i].InitUID();				// Finished moving higher entries (if any) so we free the last entry.
			break;
		}
	}
	uid.CharFind()->m_uidSpawnItem.InitUID();
	uid.CharFind()->StatFlag_Clear(STATF_SPAWNED);
	CScript s("-e_spawn_champion");
	uid.CharFind()->m_OEvents.r_LoadVal( s, RES_EVENTS );	//removing event from the char
	OnKill();
}

// Storing one UID in Spawn's m_obj[]
void CItemChampion::AddObj( CUID uid )
{
	ADDTOCALLSTACK("CitemSpawn:AddObj");
	unsigned char iMax = GetAmount() > 0 ? static_cast<unsigned char>(GetAmount()) : 1;
	iMax += 1;	// We must give a +1 to create a 'free slot'
	if (!uid || !uid.CharFind()->m_pNPC)	// Only adding UIDs...
		return;
	if ( uid.CharFind()->m_uidSpawnItem.ItemFind() )	//... which doesn't have a SpawnItem already
		return;
	for ( unsigned char i = 0; i < iMax; i++ )
	{
		if ( m_obj[i] == uid )	// Not adding me again
			return;
		if ( !m_obj[i].CharFind() )
		{
			m_obj[i] = uid;
			m_itNormal.m_more2++;
			uid.CharFind()->StatFlag_Set(STATF_SPAWNED);
			uid.CharFind()->m_uidSpawnItem = GetUID();
			break;
		}
	}
	CScript s("+e_spawn_champion");
	uid.CharFind()->m_OEvents.r_LoadVal( s, RES_EVENTS );	//adding event to the char
}
// Returns the monster's 'level' according to the champion's level (red candles).
/*BYTE CItemChampion::GetPercentMonsters()
{
	switch (m_Level)
	{
		default:
		case 1:	
			return 38;
		case 2:
		case 3:
			return 25;
		case 4:
			return 12;
	}
};*/

// Returns the percentaje of monsters killed.
/*BYTE CItemChampion::GetCompletionMonsters()
{
	BYTE iPercent = (m_iDeathCount * 100) / m_iSpawnsMax;

	if (iPercent <= 53)
		return 1;
	else if ( iPercent <= 80 ) // 53 + 27
		return 2;
	else if ( iPercent <= 93 ) // 53 + 27 + 13
		return 3;
	else if ( iPercent <= 100 )
		return 4;
	return 1;
};*/

enum ICHMPL_TYPE
{
	ICHMPL_ADDOBJ,
	ICHMPL_ADDREDCANDLE,
	ICHMPL_ADDWHITECANDLE,
	ICHMPL_REDCANDLES,
	ICHMPL_SPAWNSCUR,
	ICHMPL_SPAWNSMAX,
	ICHMPL_WHITECANDLES,
	ICHMPL_QTY
};

LPCTSTR const CItemChampion::sm_szLoadKeys[ICHMPL_QTY + 1] =
{
	"ADDOBJ",
	"ADDREDCANDLE",
	"ADDWHITECANDLE",
	"REDCANDLES",
	"SPAWNSCUR",
	"SPAWNSMAX",
	"WHITECANDLES",
	NULL
};

enum ICHMPV_TYPE
{
	ICHMPV_ADDOBJ,
	ICHMPV_ADDREDCANDLE,
	ICHMPV_ADDSPAWN,
	ICHMPV_ADDWHITECANDLE,
	ICHMPV_DELOBJ,
	ICHMPV_DELREDCANDLE,
	ICHMPV_DELWHITECANDLE,
	ICHMPV_INIT,
	ICHMPV_MULTICREATE,
	ICHMPV_STOP,
	ICHMPV_QTY
};


LPCTSTR const CItemChampion::sm_szVerbKeys[ICHMPV_QTY + 1] =
{
	"ADDOBJ",
	"ADDREDCANDLE",
	"ADDSPAWN",
	"ADDWHITECANDLE",
	"DELOBJ",
	"DELREDCANDLE",
	"DELWHITECANDLE",
	"INIT",
	"MULTICREATE",
	"STOP",
	NULL
};

void CItemChampion::r_Write(CScript & s)
{
	ADDTOCALLSTACK("CItemChampion::r_Write");
	CItem::r_Write(s);
	if ( m_pRegion )
	{
		m_pRegion->r_WriteBody( s, "REGION." );
	}
	for ( BYTE i = 1 ; ; i++ )
	{
		CItem * pCandle = m_RedCandle[i];
		if ( !pCandle )
			break;
		s.WriteKeyHex("ADDREDCANDLE",pCandle->GetUID());
	}
	for ( BYTE i = 1 ; ; i++ )
	{
		CItem * pCandle = m_WhiteCandle[i];
		if ( !pCandle )
			break;
		s.WriteKeyHex("ADDREDCANDLE",pCandle->GetUID());
	}
	return;
};

bool CItemChampion::r_WriteVal(LPCTSTR pszKey, CSString & sVal, CTextConsole * pSrc) 
{
	ADDTOCALLSTACK("CItemChampion::r_WriteVal");

	int iCmd = FindTableSorted( pszKey, sm_szLoadKeys, (int)CountOf(sm_szLoadKeys) - 1 );
	switch (iCmd)
	{
		case ICHMPL_REDCANDLES:
			sVal.FormatVal(m_RedCandles);
			return true;
		case ICHMPL_WHITECANDLES:
			sVal.FormatVal(m_WhiteCandles);
			return true;
		case ICHMPL_SPAWNSCUR:
			sVal.FormatVal( m_iSpawnsCur );
			return true;
		case ICHMPL_SPAWNSMAX:
			sVal.FormatVal( m_iSpawnsMax);
			return true;
		default:
			break;
	}
	return CItemMulti::r_WriteVal(pszKey, sVal, pSrc);
};

bool CItemChampion::r_LoadVal(CScript & s)
{	
	ADDTOCALLSTACK("CItemChampion::r_LoadVal");
	EXC_TRY("LoadVal");
	int iCmd = FindTableSorted( s.GetKey(), sm_szLoadKeys, (int)CountOf(sm_szLoadKeys) - 1 );

	switch (iCmd)
	{
		case ICHMPL_ADDOBJ:
		{
			CUID uid = static_cast<CUID>(s.GetArgVal());
			if ( uid.ObjFind() )
				AddObj(uid);
			return true;
		}
		case ICHMPL_ADDREDCANDLE:
		{
			CUID uid = static_cast<CUID>(s.GetArgVal());
			AddRedCandle(uid);
			return true;
		}
		case ICHMPL_ADDWHITECANDLE:
		{
			CUID uid = static_cast<CUID>(s.GetArgVal());
			AddWhiteCandle(uid);
			return true;
		}
		case ICHMPL_SPAWNSCUR:
			m_iSpawnsCur = (uint16)s.GetArgVal();
			return true;
		case ICHMPL_SPAWNSMAX:
			m_iSpawnsMax = (uint16)s.GetArgVal();
			return true;
		default:
			return CItemMulti::r_LoadVal(s);
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
};

bool CItemChampion::r_GetRef(LPCTSTR & pszKey, CScriptObj * & pRef)
{
	ADDTOCALLSTACK("CItemChampion::r_GetRef");
	if ( !strnicmp(pszKey, "WHITECANDLE", 11) )
	{
		pszKey += 11;
		int iCandle = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		CItem * pCandle = m_WhiteCandle[iCandle];
		if (pCandle)
		{
			pRef = pCandle;
			return true;
		}
	}
	if ( !strnicmp(pszKey, "REDCANDLE", 9 ) )
	{
		pszKey += 9;
		int iCandle = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		CItem * pCandle = m_RedCandle[iCandle];
		if (pCandle)
		{
			pRef = pCandle;
			return true;
		}
	}
	if ( ! strnicmp( pszKey, "SPAWN", 5 ))
	{
		pszKey += 5;
		int i = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		if ( m_obj[i] )
			pRef = m_obj[i].ObjFind();
		return( true );
	}

	return( CItemMulti::r_GetRef( pszKey, pRef ));
};

bool CItemChampion::r_Verb(CScript & s, CTextConsole * pSrc)
{
	ADDTOCALLSTACK("CItemChampion::r_Verb");
	EXC_TRY("Verb");

	int iCmd = FindTableSorted( s.GetKey(), sm_szVerbKeys, (int)CountOf( sm_szVerbKeys )-1 );
	switch ( iCmd )
	{
		case ICHMPV_ADDOBJ:
		{
			CUID uid = static_cast<CUID>(s.GetArgVal());
			if ( uid.ObjFind() )
				AddObj(uid);
			return true;
		}
		case ICHMPV_ADDREDCANDLE:
		{
			CUID uid = static_cast<CUID>(s.GetArgVal());
			AddRedCandle(uid);
			return true;
		}
		case ICHMPV_ADDWHITECANDLE:
		{
			CUID uid = static_cast<CUID>(s.GetArgVal());
			AddWhiteCandle(uid);
			return true;
		}
		case ICHMPV_ADDSPAWN:
			SpawnNPC();
			return true;
		case ICHMPV_DELOBJ:
		{
			CUID uid = static_cast<CUID>(s.GetArgVal());
			if ( uid.ObjFind() )
				DelObj(uid);
			return true;
		}
		case ICHMPV_DELREDCANDLE:
			DelRedCandle();
			return true;
		case ICHMPV_DELWHITECANDLE:
			DelWhiteCandle();
			return true;
		case ICHMPV_INIT:
			Init();
			return true;
		case ICHMPV_MULTICREATE:
		{
			CUID	uid( s.GetArgVal() );
			CChar *	pCharSrc = uid.CharFind();
			Multi_Create( pCharSrc, 0 );
			return true;
		}
		case ICHMPV_STOP:
			Stop();
			return true;

		default:
			return CItem::r_Verb( s, pSrc );
	}

	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return true;
};


enum CHAMPIONDEF_TYPE
{
	CHAMPIONDEF_CHAMPION,	///< Champion ID: m_Champion.
	CHAMPIONDEF_DEFNAME,	///< Champion's DEFNAME
	CHAMPIONDEF_NAME,		///< Champion name: m_sName.
	CHAMPIONDEF_NPCGROUP,	///< Monster group / level: m_iSpawn[n][n].
	CHAMPIONDEF_SPAWNS,		///< Total amount of monsters: m_iSpawnsMax.
	CHAMPIONDEF_QTY
};

LPCTSTR const CChampionDef::sm_szLoadKeys[] =
{
	"CHAMPION",
	"DEFNAME",
	"NAME",
	"NPCGROUP",
	"SPAWNS",
	"TYPE",
	NULL
};


CChampionDef::CChampionDef(CResourceID rid) : CResourceLink(rid)
{
    ADDTOCALLSTACK("CChampionDef::CChampionDef");
    g_Log.EventDebug("Creado CChampionDef\n");
    ASSERT(_CrtCheckMemory());
    m_iType = 0;
    m_iSpawnsMax = 0;
    for (unsigned char iGroup = 0; iGroup < 5; iGroup++)
    {
        for (unsigned char iNpc = 0; iNpc < 5; iNpc++)
            m_iSpawn[iGroup][iNpc] = (CREID_TYPE)0;
    }
};

CChampionDef::~CChampionDef()
{
	ADDTOCALLSTACK("CChampionDef::~CChampionDef");
};

bool CChampionDef::r_WriteVal(LPCTSTR pszKey, CSString & sVal, CTextConsole * pSrc)
{
	ADDTOCALLSTACK("CChampionDef::r_WriteVal");
	CHAMPIONDEF_TYPE iCmd = (CHAMPIONDEF_TYPE) FindTableHeadSorted( pszKey, sm_szLoadKeys, CHAMPIONDEF_QTY );

	switch (iCmd)
	{
		case CHAMPIONDEF_CHAMPION:
			sVal = g_Cfg.ResourceGetName(CResourceID( RES_CHARDEF, m_Champion) );
			return true;
		case CHAMPIONDEF_DEFNAME:
			sVal = g_Cfg.ResourceGetName( GetResourceID() );
			return true;
		case CHAMPIONDEF_NAME:
			sVal.Format(m_sName);
			return true;
		case CHAMPIONDEF_NPCGROUP:
		{
			pszKey +=8;
			SKIP_SEPARATORS(pszKey);
            int8 iArg = (int8)Exp_GetSingle(pszKey);	//it reads both numbers given together, doing /10 will result in the right number to be used here
            int8 iGroup = iArg / 10;
			if ( iGroup > 4)
				return false;
            int8 iNpc = iArg % 10;
			if ( iNpc > 4)
				return false;
			if ( m_iSpawn[iGroup][iNpc] )
			{
				sVal = g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, m_iSpawn[iGroup][iNpc]));
				return true;
			}
			return false;
		}
		case CHAMPIONDEF_SPAWNS:
			return true;
		default:
			break;
	}
	return CResourceDef::r_WriteVal( pszKey, sVal, pSrc );
};

bool CChampionDef::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CChampionDef::r_LoadVal");
	LPCTSTR pszKey = s.GetKey();
	CHAMPIONDEF_TYPE iCmd = static_cast<CHAMPIONDEF_TYPE>( FindTableHeadSorted( pszKey, sm_szLoadKeys, (int)CountOf(sm_szLoadKeys) -1 ) );

	switch (iCmd)
	{
		case CHAMPIONDEF_CHAMPION:
            g_Log.EventDebug("loaded CHAMPION value.\n");
			m_Champion = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr()));
			return true;
		case CHAMPIONDEF_DEFNAME:
			return SetResourceName( s.GetArgStr());
		case CHAMPIONDEF_NAME:
			m_sName = s.GetArgStr();
			return true;
		case CHAMPIONDEF_NPCGROUP:
		{
			pszKey +=8;
            uint8 iGroup = (uint8)Exp_GetVal(pszKey);
			if ( iGroup > 4)
				return false;
			int64 piCmd[4];
			size_t iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, (int)CountOf(piCmd));
			uint8 iCount = 1;
			for (uint8 i = 0; i <= iArgQty; i++ )
			{
				CREID_TYPE pCharDef = static_cast<CREID_TYPE>(piCmd[i]);
				if ( pCharDef )
				{
					m_iSpawn[iGroup][iCount] = pCharDef;
					iCount++;
				}
			}
			return true;
		}
		case CHAMPIONDEF_SPAWNS:
			m_iSpawnsMax = (uint16)s.GetArgVal();
			return true;
		default:
			break;
	}
	return CResourceDef::r_LoadVal( s );
};