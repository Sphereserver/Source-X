
// Actions specific to an NPC.

#include "../../common/CException.h"
#include "../clients/CClient.h"
#include "../CServerTime.h"
#include "../triggers.h"
#include "CCharNPC.h"


lpctstr const CCharNPC::sm_szLoadKeys[CNC_QTY+1] =
{
#define ADD(a,b) b,
#include "../../tables/CCharNpc_props.tbl"
#undef ADD
	NULL
};

void CChar::ClearNPC()
{
	ADDTOCALLSTACK("CChar::ClearNPC");
	if ( m_pNPC == NULL )
		return;

	delete m_pNPC;
	m_pNPC = NULL;
}

CChar * CChar::CreateNPC( CREID_TYPE baseID )	// static
{
	ADDTOCALLSTACK("CChar::CreateNPC");
	CChar * pChar = CreateBasic(baseID);
	ASSERT(pChar);
	pChar->NPC_LoadScript(true);
	pChar->NPC_CreateTrigger();

	return pChar;
}

CCharNPC::CCharNPC( CChar * pChar, NPCBRAIN_TYPE NPCBrain )
{
	UNREFERENCED_PARAMETER(pChar);
	m_Brain = NPCBrain;
	m_Home_Dist_Wander = INT16_MAX;	// as far as i want.
	m_Act_Motivation = 0;
	m_bonded = 0;
#ifndef _WIN32
	for (int i_tmpN=0;i_tmpN < MAX_NPC_PATH_STORAGE_SIZE;i_tmpN++)
	{
		m_nextX[i_tmpN] = 0;
		m_nextY[i_tmpN] = 0;
	}
#else
	memset(m_nextX, 0, sizeof(m_nextX));
	memset(m_nextY, 0, sizeof(m_nextY));
#endif
	m_timeRestock.Init();
}

CCharNPC::~CCharNPC()
{
}

bool CCharNPC::r_LoadVal( CChar * pChar, CScript &s )
{
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CNC_QTY ))
	{
		//Set as Strings
		case CNC_THROWDAM:
		case CNC_THROWOBJ:
		case CNC_THROWRANGE:
		{
			bool fQuoted = false;
			pChar->SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
		}
		break;
		//Set as numbers only
		case CNC_BONDED:
			m_bonded = (s.GetArgVal() > 0);
			if ( !g_Serv.IsLoading() )
				pChar->ResendTooltip();
			break;
		case CNC_FOLLOWERSLOTS:
			pChar->SetDefNum(s.GetKey(), s.GetArgVal(), false );
			break;
		case CNC_ACTPRI:
			m_Act_Motivation = (uchar)(s.GetArgVal());
			break;
		case CNC_NPC:
			m_Brain = static_cast<NPCBRAIN_TYPE>(s.GetArgVal());
			break;
		case CNC_HOMEDIST:
			if ( ! pChar->m_ptHome.IsValidPoint())
			{
				pChar->m_ptHome = pChar->GetTopPoint();
			}
			m_Home_Dist_Wander = (word)(s.GetArgVal());
			break;
		case CNC_NEED:
		case CNC_NEEDNAME:
		{
			tchar * pTmp = s.GetArgRaw();
			m_Need.Load(pTmp);
		}
		break;
		case CNC_SPEECH:
			return( m_Speech.r_LoadVal( s, RES_SPEECH ));
		case CNC_VENDCAP:
		{
			CItemContainer * pBank = pChar->GetBank();
			if ( pBank )
				pBank->m_itEqBankBox.m_Check_Restock = s.GetArgVal();
		}
		break;
		case CNC_VENDGOLD:
		{
			CItemContainer * pBank = pChar->GetBank();
			if ( pBank )
				pBank->m_itEqBankBox.m_Check_Amount = s.GetArgVal();
		}
		break;
		case CNC_SPELLADD:
		{
			int64 ppCmd[255];
			size_t count = Str_ParseCmds(s.GetArgStr(), ppCmd, CountOf(ppCmd));
			if (count < 1)
				return false;
			for (size_t i = 0; i < count; i++)
				Spells_Add(static_cast<SPELL_TYPE>(ppCmd[i]));
		}

		default:
			// Just ignore any player type stuff.
			if ( FindTableHeadSorted( s.GetKey(), CCharPlayer::sm_szLoadKeys, CPC_QTY ) >= 0 )
				return true;
			return(false );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CCharNPC::r_WriteVal( CChar * pChar, lpctstr pszKey, CSString & sVal )
{
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, CNC_QTY ))
	{

		//return as string or hex number or NULL if not set
		//On these ones, check BaseDef too if not found on dynamic
		case CNC_THROWDAM:
		case CNC_THROWOBJ:
		case CNC_THROWRANGE:
			sVal = pChar->GetDefStr(pszKey, false, true);
			break;
			//return as decimal number or 0 if not set
			//On these ones, check BaseDef if not found on dynamic
		case CNC_BONDED:
			sVal.FormatVal( m_bonded );
			break;
		case CNC_FOLLOWERSLOTS:
			sVal.FormatLLVal(pChar->GetDefNum(pszKey, true, true));
			break;
		case CNC_ACTPRI:
			sVal.FormatVal( m_Act_Motivation );
			break;
		case CNC_NPC:
			sVal.FormatVal( m_Brain );
			break;
		case CNC_HOMEDIST:
			sVal.FormatVal( m_Home_Dist_Wander );
			break;
		case CNC_NEED:
		{
			tchar *pszTmp = Str_GetTemp();
			m_Need.WriteKey( pszTmp );
			sVal = pszTmp;
		}
		break;
		case CNC_NEEDNAME:
		{
			tchar *pszTmp = Str_GetTemp();
			m_Need.WriteNameSingle( pszTmp );
			sVal = pszTmp;
		}
		break;
		case CNC_SPEECH:
			m_Speech.WriteResourceRefList( sVal );
			break;
		case CNC_VENDCAP:
		{
			CItemContainer * pBank = pChar->GetBank();
			if ( pBank )
				sVal.FormatVal( pBank->m_itEqBankBox.m_Check_Restock );
		}
		break;
		case CNC_VENDGOLD:
		{
			CItemContainer * pBank = pChar->GetBank();
			if ( pBank )
				sVal.FormatVal( pBank->m_itEqBankBox.m_Check_Amount );
		}
		break;
		default:
			if ( FindTableHeadSorted( pszKey, CCharPlayer::sm_szLoadKeys, CPC_QTY ) >= 0 )
			{
				sVal = "0";
				return true;
			}
			if ( FindTableSorted( pszKey, CClient::sm_szLoadKeys, CC_QTY ) >= 0 )
			{
				sVal = "0";
				return true;
			}
			return(false );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pChar);
	EXC_DEBUG_END;
	return false;
}

void CCharNPC::r_WriteChar( CChar * pChar, CScript & s )
{
	UNREFERENCED_PARAMETER(pChar);

	// This says we are an NPC.
	s.WriteKeyVal("NPC", m_Brain );

	if ( m_Home_Dist_Wander < INT16_MAX )
		s.WriteKeyVal( "HOMEDIST", m_Home_Dist_Wander );
	if ( m_Act_Motivation )
		s.WriteKeyHex( "ACTPRI", m_Act_Motivation );

	m_Speech.r_Write( s, "SPEECH" );
	if (m_bonded)
		s.WriteKeyVal("BONDED", m_bonded);

	if ( m_Need.GetResourceID().IsValidUID())
	{
		TemporaryString pszTmp;
		m_Need.WriteKey( pszTmp );
		s.WriteKey( "NEED", pszTmp );
	}
}

bool CCharNPC::IsVendor() const
{
	return( m_Brain == NPCBRAIN_HEALER || m_Brain == NPCBRAIN_BANKER || m_Brain == NPCBRAIN_VENDOR || m_Brain == NPCBRAIN_STABLE );
}

int CCharNPC::GetNpcAiFlags( const CChar *pChar ) const 
{
	CVarDefCont *pVar = pChar->GetKey( "OVERRIDE.NPCAI", true );
	if (pVar != NULL)
		return (int)(pVar->GetValNum());
	return g_Cfg.m_iNpcAi;
}

// Create an NPC from script.
void CChar::NPC_LoadScript( bool fRestock )
{
	ADDTOCALLSTACK("CChar::NPC_LoadScript");
	if ( m_pNPC == NULL )
		// Set a default brain type til we get the real one from scripts.
		SetNPCBrain(GetNPCBrain(false));	// should have a default brain. watch out for override vendor.

	CCharBase * pCharDef = Char_GetDef();

	// 1) CHARDEF trigger
	if ( m_pPlayer == NULL ) //	CHARDEF triggers (based on body type)
	{
		CChar * pChar = this->GetChar();
		if ( pChar != NULL )
		{
			CUID uidOldAct = pChar->m_Act_Targ;
			pChar->m_Act_Targ = GetUID();
			pChar->ReadScriptTrig(pCharDef, CTRIG_Create);
			pChar->m_Act_Targ = uidOldAct;
		}
	}
	//This remains untouched but moved after the chardef's section
	if (( fRestock ) && ( IsTrigUsed(TRIGGER_NPCRESTOCK) ))
		ReadScriptTrig(pCharDef, CTRIG_NPCRestock);

	CreateNewCharCheck();	//This one is giving stats, etc to the char, so we can read/set them in the next triggers.
}

// @Create trigger, NPC version
void CChar::NPC_CreateTrigger()
{
	ADDTOCALLSTACK("CChar::NPC_CreateTrigger");
	if (!m_pNPC)
		return;

	CCharBase *pCharDef = Char_GetDef();
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
	lpctstr pszTrigName = "@Create";
	CTRIG_TYPE iAction = (CTRIG_TYPE)FindTableSorted(pszTrigName, sm_szTrigName, CountOf(sm_szTrigName) - 1);

	// 2) TEVENTS
	for (size_t i = 0; i < pCharDef->m_TEvents.GetCount(); ++i)
	{
		CResourceLink * pLink = pCharDef->m_TEvents[i];
		if (!pLink || !pLink->HasTrigger(iAction))
			continue;
		CResourceLock s;
		if (!pLink->ResourceLock(s))
			continue;
		iRet = CScriptObj::OnTriggerScript(s, pszTrigName, this, 0);
		if (iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT)
			return;
	}

	// 4) EVENTSPET triggers
	for (size_t i = 0; i < g_Cfg.m_pEventsPetLink.GetCount(); ++i)
	{
		CResourceLink * pLink = g_Cfg.m_pEventsPetLink[i];
		if (!pLink || !pLink->HasTrigger(iAction))
			continue;
		CResourceLock s;
		if (!pLink->ResourceLock(s))
			continue;
		iRet = CScriptObj::OnTriggerScript(s, pszTrigName, this, 0);
		if (iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT)
			return;
	}
}
