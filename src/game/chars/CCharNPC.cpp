// Actions specific to an NPC.

#include <flat_containers/flat_set.hpp>
#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../clients/CClient.h"
#include "../CServer.h"
#include "../triggers.h"
#include "CCharNPC.h"


lpctstr const CCharNPC::sm_szLoadKeys[CNC_QTY+1] =
{
#define ADD(a,b) b,
#include "../../tables/CCharNpc_props.tbl"
#undef ADD
	nullptr
};

void CChar::ClearNPC()
{
	ADDTOCALLSTACK("CChar::ClearNPC");
	if ( m_pNPC == nullptr )
		return;

	delete m_pNPC;
	m_pNPC = nullptr;
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
	UnreferencedParameter(pChar);
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
	m_timeRestock = 0;
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
		case CNC_THROWDAMTYPE:
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
				pChar->UpdatePropertyFlag();
			break;
		case CNC_FOLLOWERSLOTS:
			pChar->SetDefNum(s.GetKey(), s.GetArgVal(), false );
			break;
		case CNC_ACTPRI:
			m_Act_Motivation = (uchar)(s.GetArgVal());
			break;
		case CNC_NPC:
			m_Brain = NPCBRAIN_TYPE(s.GetArgVal());
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
			const int count = Str_ParseCmds(s.GetArgStr(), ppCmd, ARRAY_COUNT(ppCmd));
			if (count < 1)
				return false;
			for (int i = 0; i < count; ++i)
				Spells_Add((SPELL_TYPE)(ppCmd[i]));
		}
		break;

		default:
			// Just ignore any player type stuff.
			if ( FindTableHeadSorted( s.GetKey(), CCharPlayer::sm_szLoadKeys, CPC_QTY ) >= 0 )
				return true;
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CCharNPC::r_WriteVal( CChar * pChar, lpctstr ptcKey, CSString & sVal )
{
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( ptcKey, sm_szLoadKeys, CNC_QTY ))
	{

		//return as string or hex number or nullptr if not set
		//On these ones, check BaseDef too if not found on dynamic
		case CNC_THROWDAM:
		case CNC_THROWDAMTYPE:
		case CNC_THROWOBJ:
		case CNC_THROWRANGE:
			sVal = pChar->GetDefStr(ptcKey, false, true);
			break;
			//return as decimal number or 0 if not set
			//On these ones, check BaseDef if not found on dynamic
		case CNC_BONDED:
			sVal.FormatVal( m_bonded );
			break;
		case CNC_FOLLOWERSLOTS:
			sVal.FormatLLVal(pChar->GetDefNum(ptcKey, true));
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
			m_Need.WriteKey( pszTmp, Str_TempLength() );
			sVal = pszTmp;
		}
		break;
		case CNC_NEEDNAME:
		{
			tchar *pszTmp = Str_GetTemp();
			m_Need.WriteNameSingle( pszTmp, Str_TempLength() );
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
			if ( FindTableHeadSorted( ptcKey, CCharPlayer::sm_szLoadKeys, CPC_QTY ) >= 0 )
			{
				sVal = "0";
				return true;
			}
			if ( FindTableSorted( ptcKey, CClient::sm_szLoadKeys, CC_QTY ) >= 0 )
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
	UnreferencedParameter(pChar);

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
		TemporaryString tsTemp;
		m_Need.WriteKey(tsTemp.buffer(), tsTemp.capacity());
		s.WriteKeyStr( "NEED", tsTemp.buffer());
	}
}

bool CCharNPC::IsVendor() const
{
	return ( (m_Brain == NPCBRAIN_HEALER) || (m_Brain == NPCBRAIN_BANKER) || (m_Brain == NPCBRAIN_VENDOR) || (m_Brain == NPCBRAIN_STABLE) );
}

int CCharNPC::GetNpcAiFlags( const CChar *pChar ) const 
{
	CVarDefCont *pVar = pChar->GetKey("OVERRIDE.NPCAI", true );
	if (pVar != nullptr)
		return (int)(pVar->GetValNum());
	return g_Cfg.m_iNpcAi;
}

// Create an NPC from script.
void CChar::NPC_LoadScript( bool fRestock )
{
	ADDTOCALLSTACK("CChar::NPC_LoadScript");
	if (!m_pNPC)
	{
		// Set a default brain type til we get the real one from scripts.
		// should have a default brain. watch out for override vendor.
		SetNPCBrain(GetNPCBrainAuto());	
	}					

	CCharBase * pCharDef = Char_GetDef();

    CChar * pChar = this->GetChar();
    if (pChar != nullptr)
    {
	    // 1) CHARDEF trigger
	    if ( m_pPlayer == nullptr ) //	CHARDEF triggers (based on body type)
	    {
		    CUID uidOldAct = pChar->m_Act_UID;
		    pChar->m_Act_UID = GetUID();
		    pChar->ReadScriptReducedTrig(pCharDef, CTRIG_Create);
		    pChar->m_Act_UID = uidOldAct;
	    }
	    //This remains untouched but moved after the chardef's section
	    if ( fRestock && IsTrigUsed(TRIGGER_NPCRESTOCK) )
            pChar->ReadScriptReducedTrig(pCharDef, CTRIG_NPCRestock);
    }
	CreateNewCharCheck();	//This one is giving stats, etc to the char, so we can read/set them in the next triggers.
}

// @Create trigger, NPC version
void CChar::NPC_CreateTrigger()
{
	ADDTOCALLSTACK("CChar::NPC_CreateTrigger");
	ASSERT(m_pNPC);

	fc::vector_set<const CResourceLink*> executedEvents;
	CCharBase *pCharDef = Char_GetDef();

	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
	lpctstr pszTrigName = "@Create";
	CTRIG_TYPE iAction = (CTRIG_TYPE)FindTableSorted(pszTrigName, sm_szTrigName, ARRAY_COUNT(sm_szTrigName) - 1);	

	// 2) TEVENTS
	for (size_t i = 0; i < pCharDef->m_TEvents.size(); ++i)
	{
		CResourceLink * pLink = pCharDef->m_TEvents[i].GetRef();
		if (!pLink || !pLink->HasTrigger(iAction) || (executedEvents.find(pLink) != executedEvents.end()))
			continue;

		CResourceLock s;
		if (!pLink->ResourceLock(s))
			continue;

		executedEvents.emplace(pLink);
		iRet = CScriptObj::OnTriggerScript(s, pszTrigName, this, 0);
		if (iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT)
			return;
	}

	// 4) EVENTSPET triggers
	for (size_t i = 0; i < g_Cfg.m_pEventsPetLink.size(); ++i)
	{
		CResourceLink * pLink = g_Cfg.m_pEventsPetLink[i].GetRef();
		if (!pLink || !pLink->HasTrigger(iAction) || (executedEvents.find(pLink) != executedEvents.end()))
			continue;

		CResourceLock s;
		if (!pLink->ResourceLock(s))
			continue;

		executedEvents.emplace(pLink);
		iRet = CScriptObj::OnTriggerScript(s, pszTrigName, this, 0);
		if (iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT)
			return;
	}
}
