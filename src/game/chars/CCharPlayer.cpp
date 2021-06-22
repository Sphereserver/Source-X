
// Actions specific to a Player.

#include "../../common/resource/sections/CSkillClassDef.h"
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../clients/CClient.h"
#include "../items/CItemMulti.h"
#include "../CServer.h"
#include "../CWorld.h"
#include "../CWorldGameTime.h"
#include "CChar.h"
#include "CCharNPC.h"


lpctstr const CCharPlayer::sm_szLoadKeys[CPC_QTY+1] =
{
#define ADD(a,b) b,
#include "../../tables/CCharPlayer_props.tbl"
#undef ADD
	nullptr
};


CCharPlayer::CCharPlayer(CChar *pChar, CAccount *pAccount) :
	m_SkillLock{}, m_StatLock{},
	m_pAccount(pAccount)
{
	_iTimeLastUsed = _iTimeLastDisconnected = 0;

    m_SpeechHue = m_EmoteHue = 0;
	m_wDeaths = m_wMurders = 0;
	m_speedMode = 0;
	m_pflag = 0;
	m_fKrToolbarEnabled = false;
	m_LocalLight = 0;

	_iMaxHouses = g_Cfg._iMaxHousesPlayer;
	_iMaxShips = g_Cfg._iMaxShipsPlayer;
	_pMultiStorage = new CMultiStorage(pChar->GetUID());

	SetSkillClass(pChar, CResourceID(RES_SKILLCLASS));
}

CCharPlayer::~CCharPlayer()
{
	m_Speech.clear();

	CMultiStorage* pOldStorage = _pMultiStorage;
	_pMultiStorage = nullptr;
	delete pOldStorage;			// I need _pMultiStorage to be nullptr before CMultiStorage destructor is called
}

CAccount * CCharPlayer::GetAccount() const
{
	ASSERT( m_pAccount );
	return m_pAccount;
}

bool CCharPlayer::getKrToolbarStatus() const noexcept
{
	return m_fKrToolbarEnabled;
}

CMultiStorage* CCharPlayer::GetMultiStorage()
{
	return _pMultiStorage;
}

bool CCharPlayer::SetSkillClass( CChar * pChar, CResourceID rid )
{
	ADDTOCALLSTACK("CCharPlayer::SetSkillClass");
	CResourceDef * pDef = g_Cfg.ResourceGetDef(rid);
	if ( !pDef )
		return false;

	CSkillClassDef* pLink = static_cast <CSkillClassDef*>(pDef);
	if ( pLink == GetSkillClass() )
		return true;

	// Remove any previous skillclass from the Events block.
	size_t i = pChar->m_OEvents.FindResourceType(RES_SKILLCLASS);
	if ( i != SCONT_BADINDEX )
		pChar->m_OEvents.erase(pChar->m_OEvents.begin() + i);

	m_SkillClass.SetRef(pLink);

	// set it as m_Events block as well.
    pChar->m_OEvents.push_back(pLink);
	return true;
}

// This should always return NON-nullptr.
CSkillClassDef * CCharPlayer::GetSkillClass() const
{
	ADDTOCALLSTACK("CCharPlayer::GetSkillClass");

	CResourceLink * pLink = m_SkillClass.GetRef();
	if ( pLink == nullptr )
		return nullptr;
	return( static_cast <CSkillClassDef *>(pLink));
}

// only players can have skill locks.
SKILL_TYPE CCharPlayer::Skill_GetLockType( lpctstr ptcKey ) const
{
	ADDTOCALLSTACK("CCharPlayer::Skill_GetLockType");

	tchar szTmpKey[128];
    Str_CopyLimitNull( szTmpKey, ptcKey, CountOf(szTmpKey) );

	tchar * ppArgs[3];
	size_t i = Str_ParseCmds( szTmpKey, ppArgs, CountOf(ppArgs), ".[]" );
	if ( i <= 1 )
		return SKILL_NONE;

	if ( IsDigit( ppArgs[1][0] ))
		i = atoi( ppArgs[1] );
	else
		i = g_Cfg.FindSkillKey( ppArgs[1] );

	if ( i >= g_Cfg.m_iMaxSkill )
		return SKILL_NONE;
	return (SKILL_TYPE)i;
}

SKILLLOCK_TYPE CCharPlayer::Skill_GetLock( SKILL_TYPE skill ) const
{
	ASSERT( (skill >= 0) && ((size_t)skill < CountOf(m_SkillLock)));
	return static_cast<SKILLLOCK_TYPE>(m_SkillLock[skill]);
}

void CCharPlayer::Skill_SetLock( SKILL_TYPE skill, SKILLLOCK_TYPE state )
{
	ASSERT( (skill >= 0) && ((uint)skill < CountOf(m_SkillLock)));
	m_SkillLock[skill] = (uchar)(state);
}

// only players can have stat locks.
STAT_TYPE CCharPlayer::Stat_GetLockType( lpctstr ptcKey ) const
{
	ADDTOCALLSTACK("CCharPlayer::Stat_GetLockType");

	tchar szTmpKey[128];
    Str_CopyLimitNull( szTmpKey, ptcKey, CountOf(szTmpKey) );

	tchar * ppArgs[3];
	size_t i = Str_ParseCmds( szTmpKey, ppArgs, CountOf(ppArgs), ".[]" );
	if ( i <= 1 )
		return STAT_NONE;

	if ( IsDigit( ppArgs[1][0] ))
		i = atoi( ppArgs[1] );
	else
		i = g_Cfg.GetStatKey( ppArgs[1] );

	if ( i >= STAT_BASE_QTY )
		return STAT_NONE;
	return (STAT_TYPE)i;
}

SKILLLOCK_TYPE CCharPlayer::Stat_GetLock( STAT_TYPE stat ) const
{
	ASSERT( (stat >= 0) && ((uint)stat < CountOf(m_StatLock)));
	return (SKILLLOCK_TYPE)m_StatLock[stat];
}

void CCharPlayer::Stat_SetLock( STAT_TYPE stat, SKILLLOCK_TYPE state )
{
	ASSERT( (stat >= 0) && ((uint)stat < CountOf(m_StatLock)));
	m_StatLock[stat] = (uchar)state;
}

bool CCharPlayer::r_WriteVal( CChar * pChar, lpctstr ptcKey, CSString & sVal )
{
	ADDTOCALLSTACK("CCharPlayer::r_WriteVal");
	EXC_TRY("WriteVal");

	if ( !pChar || !GetAccount() )
		return false;

	if ( !strnicmp(ptcKey, "SKILLCLASS.", 11) )
	{
		CSkillClassDef* pSkillClass = GetSkillClass();
		ASSERT(pSkillClass);
		return pSkillClass->r_WriteVal(ptcKey + 11, sVal, pChar);
	}
	else if ( ( !strnicmp(ptcKey, "GUILD", 5) ) || ( !strnicmp(ptcKey, "TOWN", 4) ) )
	{
		const bool fIsGuild = !strnicmp(ptcKey, "GUILD", 5);
		ptcKey += fIsGuild ? 5 : 4;
		if ( *ptcKey == 0 )
		{
			CItemStone *pMyGuild = pChar->Guild_Find(fIsGuild ? MEMORY_GUILD : MEMORY_TOWN);
			if ( pMyGuild )
                sVal.FormatHex((dword)pMyGuild->GetUID());
			else
                sVal.FormatVal(0);
			return true;
		}
		else if ( *ptcKey == '.' )
		{
			ptcKey += 1;
			CItemStone *pMyGuild = pChar->Guild_Find(fIsGuild ? MEMORY_GUILD : MEMORY_TOWN);
			if ( pMyGuild )
                return pMyGuild->r_WriteVal(ptcKey, sVal, pChar);
		}
		return false;
	}

	switch ( FindTableHeadSorted( ptcKey, sm_szLoadKeys, CPC_QTY ))
	{
		case CPC_ACCOUNT:
			sVal = GetAccount()->GetName();
			return true;
        case CPC_SPEECHCOLOR:
            sVal.FormatWVal( m_SpeechHue );
            return true;
        case CPC_EMOTECOLOR:
        	sVal.FormatWVal( m_EmoteHue );
        	return true;
		case CPC_DEATHS:
			sVal.FormatVal( m_wDeaths );
			return true;
		case CPC_DSPEECH:
			m_Speech.WriteResourceRefList( sVal );
			return true;
		case CPC_KILLS:
			sVal.FormatVal( m_wMurders );
			return true;
		case CPC_KRTOOLBARSTATUS:
			sVal.FormatVal( m_fKrToolbarEnabled );
			return true;
		case CPC_ISDSPEECH:
			if ( ptcKey[9] != '.' )
				return false;
			ptcKey += 10;
			sVal = m_Speech.ContainsResourceName(RES_SPEECH, ptcKey) ? "1" : "0";
			return true;
		case CPC_LASTDISCONNECTED:
			sVal.FormatLLVal(CWorldGameTime::GetCurrentTime().GetTimeDiff(_iTimeLastDisconnected) / MSECS_PER_SEC);  //seconds
			return true;
		case CPC_LASTUSED:
			sVal.FormatLLVal( CWorldGameTime::GetCurrentTime().GetTimeDiff( _iTimeLastUsed ) / MSECS_PER_SEC );  //seconds
			return true;
		case CPC_LIGHT:
			sVal.FormatHex(m_LocalLight);
			return true;
		case CPC_PFLAG:
			sVal.FormatVal(m_pflag);
			return true;
		case CPC_PROFILE:
			{
				tchar szLine[SCRIPT_MAX_LINE_LEN-16];
				Str_MakeUnFiltered( szLine, m_sProfile, sizeof(szLine));
				sVal = szLine;
			}
			return true;
		case CPC_REFUSETRADES:
			{
				CVarDefCont * pVar = pChar->GetDefKey(ptcKey, true);
				sVal.FormatLLVal(pVar ? pVar->GetValNum() : 0);
			}
			return true;
		case CPC_SKILLCLASS:
			{
				CSkillClassDef* pSkillClass = GetSkillClass();
				ASSERT(pSkillClass);
				sVal = pSkillClass->GetResourceName();
			}
			return true;
		case CPC_SKILLLOCK:
			{
				// "SkillLock[alchemy]"
				SKILL_TYPE skill = Skill_GetLockType( ptcKey );
				if ( skill <= SKILL_NONE )
					return false;
				sVal.FormatVal( Skill_GetLock( skill ));
			} return true;
		case CPC_SPEEDMODE:
			sVal.FormatVal( m_speedMode );
			return true;
		case CPC_STATLOCK:
			{
				// "StatLock[str]"
				STAT_TYPE stat = Stat_GetLockType( ptcKey );
				if (( stat <= STAT_NONE ) || ( stat >= STAT_BASE_QTY ))
					return false;
				sVal.FormatVal( Stat_GetLock( stat ));
			} return true;

		case CPC_MAXHOUSES:
			sVal.FormatU8Val(_iMaxHouses);
			return true;
		case CPC_HOUSES:
			sVal.Format16Val(GetMultiStorage()->GetHouseCountReal());
			return true;
		case CPC_GETHOUSEPOS:
		{
			ptcKey += 11;
			sVal.Format16Val(GetMultiStorage()->GetHousePos((CUID)Exp_GetDWVal(ptcKey)));
			return true;
		}
		case CPC_MAXSHIPS:
		{
			sVal.FormatU8Val(_iMaxShips);
			return true;
		}
		case CPC_SHIPS:
		{
			sVal.Format16Val(GetMultiStorage()->GetShipCountReal());
			return true;
		}
		case CPC_GETSHIPPOS:
		{
			ptcKey += 11;
			sVal.Format16Val(GetMultiStorage()->GetShipPos((CUID)Exp_GetDWVal(ptcKey)));
			return true;
		}

		default:
			if ( FindTableSorted( ptcKey, CCharNPC::sm_szLoadKeys, CNC_QTY ) >= 0 )
			{
				sVal = "0";
				return true;
			}
			return false;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar);
	EXC_DEBUG_END;
	return false;
}

bool CCharPlayer::r_LoadVal( CChar * pChar, CScript &s )
{
	ADDTOCALLSTACK("CCharPlayer::r_LoadVal");
	EXC_TRY("LoadVal");

	lpctstr ptcKey = s.GetKey();

	if ( ( !strnicmp(ptcKey, "GUILD", 5) ) || ( !strnicmp(ptcKey, "TOWN", 4) ) )
	{
		bool bIsGuild = !strnicmp(ptcKey, "GUILD", 5);
		ptcKey += bIsGuild ? 5 : 4;
		if ( *ptcKey == '.' )
		{
			ptcKey += 1;
			CItemStone *pMyGuild = pChar->Guild_Find(bIsGuild ? MEMORY_GUILD : MEMORY_TOWN);
			if ( pMyGuild )
                return pMyGuild->r_SetVal(ptcKey, s.GetArgRaw());
		}
		return false;
	}

	switch ( FindTableHeadSorted( s.GetKey(), sm_szLoadKeys, CPC_QTY ))
	{

		case CPC_ADDHOUSE:
		{
			if (g_Serv.IsLoading()) //Prevent to load it from saves, it may cause a crash when accessing to a non-yet existant multi
			{
				break;
			}
			int64 piCmd[2];
			Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
			HOUSE_PRIV ePriv = HP_OWNER;
			if (piCmd[1] > 0 && piCmd[1] < HP_QTY)
			{
				ePriv = (HOUSE_PRIV)piCmd[1];
			}
			GetMultiStorage()->AddHouse(CUID((dword)piCmd[0]), ePriv);
			return true;
		}
		case CPC_DELHOUSE:
		{
			dword dwUID = s.GetArgDWVal();
			if (dwUID == UID_UNUSED)
			{
				GetMultiStorage()->ClearHouses();
			}
			else
			{
				GetMultiStorage()->DelHouse(CUID(dwUID));
			}
			return true;
		}
		case CPC_ADDSHIP:
		{
			if (g_Serv.IsLoading()) //Prevent to load it from saves, it may cause a crash when accessing to a non-yet existant multi
			{
				break;
			}
			int64 piCmd[2];
			Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
			HOUSE_PRIV ePriv = HP_OWNER;
			if (piCmd[1] > 0 && piCmd[1] < HP_QTY)
			{
				ePriv = (HOUSE_PRIV)piCmd[1];
			}
			GetMultiStorage()->AddShip(CUID((dword)piCmd[0]), ePriv);
			return true;
		}
		case CPC_DELSHIP:
		{
			dword dwUID = s.GetArgDWVal();
			if (dwUID == UINT_MAX)
			{
				GetMultiStorage()->ClearShips();
			}
			else
			{
				GetMultiStorage()->DelShip(CUID(dwUID));
			}
			return true;
		}

		case CPC_MAXHOUSES:
			_iMaxHouses = s.GetArgU8Val();
			return true;
		case CPC_MAXSHIPS:
			_iMaxShips = s.GetArgU8Val();
			return true;

		case CPC_LIGHT:
			m_LocalLight = s.GetArgBVal();
			if (pChar->IsClientActive())
				pChar->GetClientActive()->addLight();
			return true;

        case CPC_SPEECHCOLOR:
            m_SpeechHue = (HUE_TYPE)s.GetArgWVal();
            return true;
        case CPC_EMOTECOLOR:
        	m_EmoteHue = (HUE_TYPE)s.GetArgWVal();
        	return true;
		case CPC_DEATHS:
			m_wDeaths = (word)(s.GetArgVal());
			return true;
		case CPC_DSPEECH:
			return( m_Speech.r_LoadVal( s, RES_SPEECH ));
		case CPC_KILLS:
			m_wMurders = (word)(s.GetArgVal());
			pChar->NotoSave_Update();
			return true;
		case CPC_KRTOOLBARSTATUS:
			m_fKrToolbarEnabled = ( s.GetArgVal() != 0 );
			if ( pChar->IsClientActive() )
				pChar->GetClientActive()->addKRToolbar( m_fKrToolbarEnabled );
			return true;
		case CPC_LASTDISCONNECTED:
			_iTimeLastDisconnected = s.GetArgLLVal() * MSECS_PER_SEC;
			return true;
		case CPC_LASTUSED:
			_iTimeLastUsed = s.GetArgLLVal() * MSECS_PER_SEC;
			return true;
		case CPC_PFLAG:
			{
				m_pflag = s.GetArgVal();
			} return true;
		case CPC_PROFILE:
			m_sProfile = Str_MakeFiltered( s.GetArgStr());
			return true;
		case CPC_REFUSETRADES:
			pChar->SetDefNum(s.GetKey(), s.GetArgVal() > 0 ? 1 : 0, false);
			return true;
		case CPC_SKILLCLASS:
			return SetSkillClass( pChar, g_Cfg.ResourceGetIDType( RES_SKILLCLASS, s.GetArgStr()));
		case CPC_SKILLLOCK:
			{
				SKILL_TYPE skill = Skill_GetLockType( s.GetKey());
				if ( skill <= SKILL_NONE )
					return false;
				int bState = s.GetArgVal();
				if ( (bState < SKILLLOCK_UP) || (bState > SKILLLOCK_LOCK) )
					return false;
				Skill_SetLock(skill, (SKILLLOCK_TYPE)bState);
				if ( pChar->IsClientActive() )
					pChar->GetClientActive()->addSkillWindow(skill);
			} return true;
		case CPC_SPEEDMODE:
			{
				m_speedMode = s.GetArgBVal();
				pChar->UpdateSpeedMode();
			} return true;
		case CPC_STATLOCK:
			{
				STAT_TYPE stat = Stat_GetLockType( s.GetKey());
				if (( stat <= STAT_NONE ) || ( stat >= STAT_BASE_QTY ))
					return false;
				int bState = s.GetArgVal();
				if ( (bState < SKILLLOCK_UP) || (bState > SKILLLOCK_LOCK) )
					return false;
				Stat_SetLock(stat, (SKILLLOCK_TYPE)bState );
				if ( pChar->IsClientActive() )
					pChar->GetClientActive()->addStatusWindow(pChar);
			} return true;

		default:
			// Just ignore any NPC type stuff.
			if ( FindTableSorted( s.GetKey(), CCharNPC::sm_szLoadKeys, CNC_QTY ) >= 0 )
				return true;
			return false;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CCharPlayer::r_WriteChar( CChar * pChar, CScript & s )
{
	ADDTOCALLSTACK("CCharPlayer::r_WriteChar");
	UNREFERENCED_PARAMETER(pChar);
	EXC_TRY("r_WriteChar");

	const CAccount* pAccount = GetAccount();
	ASSERT (pAccount);

	s.WriteKeyStr("ACCOUNT", pAccount->GetName());

	if (_iTimeLastUsed > 0)
		s.WriteKeyVal("LASTUSED", _iTimeLastUsed);
	if (_iTimeLastDisconnected > 0)
		s.WriteKeyVal("LASTDISCONNECTED", _iTimeLastDisconnected);

	if ( m_wDeaths )
		s.WriteKeyVal( "DEATHS", m_wDeaths );
	if ( m_wMurders )
		s.WriteKeyVal( "KILLS", m_wMurders );
	if (CSkillClassDef* pSkillClass = GetSkillClass())
	{
		if (pSkillClass->GetResourceID().GetResIndex())
			s.WriteKeyStr("SKILLCLASS", pSkillClass->GetResourceName());
	}
	if ( m_pflag )
		s.WriteKeyVal( "PFLAG", m_pflag );
	if ( m_speedMode )
		s.WriteKeyVal("SPEEDMODE", m_speedMode);
	if (m_fKrToolbarEnabled && (pAccount->GetResDisp() >= RDS_KR))
		s.WriteKeyVal("KRTOOLBARSTATUS", m_fKrToolbarEnabled);
	if (m_LocalLight)
		s.WriteKeyHex("LIGHT", m_LocalLight);
    if ( m_SpeechHue )
        s.WriteKeyVal("SPEECHCOLOR", m_SpeechHue);
    if ( m_EmoteHue )
    	s.WriteKeyVal("EMOTECOLOR", m_EmoteHue);

	EXC_SET_BLOCK("saving dynamic speech");
	if (!m_Speech.empty())
	{
		CSString sVal;
		m_Speech.WriteResourceRefList( sVal );
		s.WriteKeyStr("DSPEECH", sVal.GetBuffer());
	}

	EXC_SET_BLOCK("saving profile");
	if ( ! m_sProfile.IsEmpty())
	{
		tchar szLine[SCRIPT_MAX_LINE_LEN - 16];
		Str_MakeUnFiltered( szLine, m_sProfile, sizeof(szLine));
		s.WriteKeyStr( "PROFILE", szLine );
	}

	tchar szTemp[64];

	EXC_SET_BLOCK("saving stats locks");
	for ( int x = 0; x < STAT_BASE_QTY; ++x)	// Don't write all lock states!
	{
		if ( ! m_StatLock[x] )
			continue;
		sprintf( szTemp, "StatLock[%d]", x );	// smaller storage space.
		s.WriteKeyVal( szTemp, m_StatLock[x] );
	}

	EXC_SET_BLOCK("saving skill locks");
	for ( uint j = 0; j < g_Cfg.m_iMaxSkill; ++j )	// Don't write all lock states!
	{
		ASSERT(j < CountOf(m_SkillLock));
		if ( ! m_SkillLock[j] )
			continue;
		sprintf( szTemp, "SkillLock[%d]", j );	// smaller storage space.
		s.WriteKeyVal( szTemp, m_SkillLock[j] );
	}

	if (_iMaxHouses != g_Cfg._iMaxHousesPlayer)
	{
		s.WriteKeyVal("MaxHouses", _iMaxHouses);
	}
	if (_iMaxShips != g_Cfg._iMaxShipsPlayer)
	{
		s.WriteKeyVal("MaxShips", _iMaxShips);
	}

	const CMultiStorage* pMultiStorage = GetMultiStorage();
	ASSERT(pMultiStorage);
	pMultiStorage->r_Write(s);

	EXC_CATCH;
}

enum CPV_TYPE	// Player char.
{
	#define ADD(a,b) CPV_##a,
	#include "../../tables/CCharPlayer_functions.tbl"
	#undef ADD
	CPV_QTY
};

lpctstr const CCharPlayer::sm_szVerbKeys[CPV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../../tables/CCharPlayer_functions.tbl"
	#undef ADD
	nullptr
};

// Execute command from script
bool CChar::Player_OnVerb( CScript &s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CChar::Player_OnVerb");
	if ( !m_pPlayer || !pSrc )
		return false;

	lpctstr ptcKey = s.GetKey();
	int cpVerb = FindTableSorted( ptcKey, CCharPlayer::sm_szVerbKeys, CountOf(CCharPlayer::sm_szVerbKeys)-1 );

	if ( cpVerb <= -1 )
	{
		if ( ( !strnicmp(ptcKey, "GUILD", 5) ) || ( !strnicmp(ptcKey, "TOWN", 4) ) )
		{
			bool bIsGuild = !strnicmp(ptcKey, "GUILD", 5);
			ptcKey += bIsGuild ? 5 : 4;
			if ( *ptcKey == '.' )
			{
				ptcKey += 1;
				CItemStone *pMyGuild = Guild_Find(bIsGuild ? MEMORY_GUILD : MEMORY_TOWN);
                if ( pMyGuild )
                {
					CScript script(ptcKey, s.GetArgRaw());
					script.CopyParseState(s);
                   	return pMyGuild->r_Verb(script, pSrc);
                }
			}
			return false;
		}
	}

	switch ( cpVerb )
	{
		case CPV_KICK: // "KICK" = kick and block the account
			return (IsClientActive() && GetClientActive()->addKick(pSrc));

		case CPV_PASSWORD:	// "PASSWORD"
		{
			// Set/Clear the password
			if ( pSrc != this )
			{
				if ( pSrc->GetPrivLevel() <= GetPrivLevel() || pSrc->GetPrivLevel() < PLEVEL_Admin )
				{
					pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_PRIV));
					return false;
				}
			}

			CAccount * pAccount = m_pPlayer->GetAccount();
			ASSERT(pAccount != nullptr);

			if ( !s.HasArgs() )
			{
				pAccount->ClearPassword();
				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_PASSCLEAR));
				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_PASSCLEAR_RELOG));
				g_Log.Event(LOGM_ACCOUNTS|LOGL_EVENT, "Account '%s', password cleared\n", pAccount->GetName());
			}
			else
			{
				if ( pAccount->SetPassword(s.GetArgStr()) )
				{
					SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_ACCEPTPASS));
					g_Log.Event(LOGM_ACCOUNTS|LOGL_EVENT, "Account '%s', password set to '%s'\n", pAccount->GetName(), s.GetArgStr());
					return true;
				}

				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_INVALIDPASS));
			}
			break;
		}

		default:
			return false;
	}

	return true;
}

