
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../../common/CScriptObj.h"
#include "../../network/send.h"
#include "../chars/CChar.h"
#include "../CServer.h"
#include "../CWorld.h"
#include "../triggers.h"
#include "CClient.h"
#include "CParty.h"


//*****************************************************************
// -CPartyDef

CPartyDef::CPartyDef( CChar *pCharInvite, CChar *pCharAccept)
{
	// pChar1 = the master.
	ASSERT(pCharInvite);
	ASSERT(pCharAccept);
    pCharInvite->m_pParty = this;
    pCharAccept->m_pParty = this;
	AttachChar(pCharInvite);
	AttachChar(pCharAccept);
	SendAddList(nullptr);		// send full list to all
	m_sName.Format("Party_0%x", (dword)pCharAccept->GetUID());
    //UpdateWaypointAll(pCharInvite, PartyMember);
}

// ---------------------------------------------------------
size_t CPartyDef::AttachChar( CChar *pChar )
{
	ADDTOCALLSTACK("CPartyDef::AttachChar");
	// RETURN:
	//  index of the char in the group. BadIndex = not in group.
	size_t i = m_Chars.AttachChar(pChar);
	pChar->NotoSave_Update();
    UpdateWaypointAll(pChar, MAPWAYPOINT_PartyMember);
	return i;
}

size_t CPartyDef::DetachChar( CChar *pChar )
{
	ADDTOCALLSTACK("CPartyDef::DetachChar");
	// RETURN:
	//  index of the char in the group. BadIndex = not in group.
	size_t i = m_Chars.DetachChar(pChar);
	if ( i != SCONT_BADINDEX )
	{
        UpdateWaypointAll(pChar, MAPWAYPOINT_Remove);
		pChar->m_pParty = nullptr;
		pChar->DeleteKey("PARTY_LASTINVITE");
		pChar->DeleteKey("PARTY_LASTINVITETIME");
		pChar->NotoSave_Update();
	}
	return i;
}

bool CPartyDef::SetMaster( CChar *pNewMaster )
{
	if ( !pNewMaster )
		return false;
	else if ( !IsInParty(pNewMaster) || IsPartyMaster(pNewMaster) )
		return false;

	size_t i = m_Chars.InsertChar(pNewMaster, 0);
	SendAddList(nullptr);
	return (i == 0);
}

void CPartyDef::SetLootFlag( CChar *pChar, bool fSet )
{
	ADDTOCALLSTACK("CPartyDef::SetLootFlag");
	ASSERT(pChar);
	if ( IsInParty(pChar) )
	{
		pChar->SetKeyNum("PARTY_CANLOOTME", fSet);
		pChar->SysMessageDefault(fSet ? DEFMSG_PARTY_LOOT_ALLOW : DEFMSG_PARTY_LOOT_BLOCK);
	}
}

bool CPartyDef::GetLootFlag( const CChar *pChar )
{
	ADDTOCALLSTACK("CPartyDef::GetLootFlag");
	ASSERT(pChar);
	if ( IsInParty(pChar) )
		return (pChar->GetKeyNum("PARTY_CANLOOTME") != 0);

	return false;
}

// ---------------------------------------------------------
void CPartyDef::AddStatsUpdate( CChar *pChar, PacketSend *pPacket )
{
	ADDTOCALLSTACK("CPartyDef::AddStatsUpdate");
	size_t iQty = m_Chars.GetCharCount();
	if ( iQty <= 0 )
		return;

	for ( size_t i = 0; i < iQty; ++i )
	{
		CChar *pCharNow = m_Chars.GetChar(i).CharFind();
		if ( pCharNow && pCharNow != pChar )
		{
			if ( pCharNow->IsClientActive() && pCharNow->CanSee(pChar) )
				pPacket->send(pCharNow->GetClientActive());
		}
	}
}

// ---------------------------------------------------------
void CPartyDef::SysMessageAll( lpctstr pText )
{
	ADDTOCALLSTACK("CPartyDef::SysMessageAll");
	// SysMessage to all members of the party.
	size_t iQty = m_Chars.GetCharCount();
	for ( size_t i = 0; i < iQty; i++ )
	{
		CChar *pChar = m_Chars.GetChar(i).CharFind();
		pChar->SysMessage(pText);
	}
}

void CPartyDef::UpdateWaypointAll(CChar * pCharSrc, MAPWAYPOINT_TYPE type)
{
    ADDTOCALLSTACK("CPartyDef::UpdateWaypointAll");
    // Send pCharSrc map waypoint location to all party members (enhanced client only)
    size_t iQty = m_Chars.GetCharCount();
    if (iQty <= 0)
        return;

    CChar *pChar = nullptr;
    for (size_t i = 0; i < iQty; i++)
    {
        pChar = m_Chars.GetChar(i).CharFind();
        if (!pChar || !pChar->GetClientActive() || (pChar == pCharSrc))
            continue;
        pChar->GetClientActive()->addMapWaypoint(pCharSrc, type);
    }
}

// ---------------------------------------------------------
bool CPartyDef::SendMemberMsg( CChar *pCharDest, PacketSend *pPacket )
{
	ADDTOCALLSTACK("CPartyDef::SendMemberMsg");
	if ( !pCharDest )
	{
		SendAll(pPacket);
		return true;
	}

	// Weirdness check.
	if ( pCharDest->m_pParty != this )
	{
		if ( DetachChar(pCharDest) != SCONT_BADINDEX )	// this is bad!
			return false;
		return true;
	}
	if ( !m_Chars.IsCharIn(pCharDest) )
	{
		pCharDest->m_pParty = nullptr;
		return true;
	}

	if ( pCharDest->IsClientActive() )
	{
		CClient *pClient = pCharDest->GetClientActive();
		ASSERT(pClient);
		pPacket->send(pClient);
		if ( *pPacket->getData() == PARTYMSG_Remove )
			pClient->addReSync();
	}

	return true;
}

void CPartyDef::SendAll( PacketSend *pPacket )
{
	ADDTOCALLSTACK("CPartyDef::SendAll");
	// Send this to all members of the party.
	size_t iQty = m_Chars.GetCharCount();
	for ( size_t i = 0; i < iQty; i++ )
	{
		CChar *pChar = m_Chars.GetChar(i).CharFind();
		ASSERT(pChar);
		if ( !SendMemberMsg(pChar, pPacket) )
		{
			iQty--;
			i--;
		}
	}
}

// ---------------------------------------------------------
bool CPartyDef::SendAddList( CChar *pCharDest )
{
	ADDTOCALLSTACK("CPartyDef::SendAddList");

	if ( m_Chars.GetCharCount() <= 0 )
		return false;

	PacketPartyList cmd(&m_Chars);
	if (pCharDest)
		SendMemberMsg(pCharDest, &cmd);
	else
		SendAll(&cmd);

	return true;
}

bool CPartyDef::SendRemoveList( CChar *pCharRemove, bool bFor )
{
	ADDTOCALLSTACK("CPartyDef::SendRemoveList");

	if ( bFor )
	{
		PacketPartyRemoveMember cmd(pCharRemove, nullptr);
		SendMemberMsg(pCharRemove, &cmd);
	}
	else
	{
		if ( m_Chars.GetCharCount() <= 0 )
			return false;

		PacketPartyRemoveMember cmd(pCharRemove, &m_Chars);
		SendAll(&cmd);
	}

	return true;
}

// ---------------------------------------------------------
bool CPartyDef::MessageEvent( CUID uidDst, CUID uidSrc, const nchar *pText, int ilenmsg )
{
	ADDTOCALLSTACK("CPartyDef::MessageEvent");
	UNREFERENCED_PARAMETER(ilenmsg);
	if ( pText == nullptr )
		return false;
	if ( uidDst.IsValidUID() && !IsInParty(uidDst.CharFind()) )
		return false;

	CChar *pFrom = uidSrc.CharFind();
	CChar *pTo = nullptr;
	if ( uidDst != (dword)0 )
		pTo = uidDst.CharFind();

	tchar *szText = Str_GetTemp();
	CvtNUNICODEToSystem(szText, MAX_TALK_BUFFER, pText, MAX_TALK_BUFFER);

	if ( !m_pSpeechFunction.IsEmpty() )
	{
		TRIGRET_TYPE tr = TRIGRET_RET_FALSE;
		CScriptTriggerArgs Args;
		Args.m_iN1 = uidSrc.GetObjUID();
		Args.m_iN2 = uidDst.GetObjUID();
		Args.m_s1 = szText;
		Args.m_s1_buf_vec = szText;

		if ( r_Call(m_pSpeechFunction, &g_Serv, &Args, nullptr, &tr) )
		{
			if ( tr == TRIGRET_RET_TRUE )
				return false;
		}
	}

	if ( g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) )
		g_Log.Event(LOGM_PLAYER_SPEAK|LOGM_NOCONTEXT, "%x:'%s' Says '%s' in party to '%s'\n", pFrom->GetClientActive()->GetSocketID(), pFrom->GetName(), szText, pTo ? pTo->GetName() : "all");

	snprintf(szText, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_MSG), pText);
	PacketPartyChat cmd(pFrom, pText);

	if ( pTo )
		SendMemberMsg(pTo, &cmd);
	else
		SendAll(&cmd);

	return true;
}

// ---------------------------------------------------------
void CPartyDef::AcceptMember( CChar *pChar )
{
	ADDTOCALLSTACK("CPartyDef::AcceptMember");
	// This person has accepted to be part of the party.
	ASSERT(pChar);

	pChar->m_pParty = this;
	AttachChar(pChar);
	SendAddList(nullptr);
}

bool CPartyDef::RemoveMember( CUID uidRemove, CUID uidCommand )
{
	ADDTOCALLSTACK("CPartyDef::RemoveMember");
	// ARGS:
	//  uidRemove = Who is being removed.
	//  uidCommand = who removed this person (only the master or self can remove)
	//
	// NOTE: remove of the master will cause the party to disband.

	if ( m_Chars.GetCharCount() <= 0 )
		return false;

	CUID uidMaster = GetMaster();
	if ( (uidRemove != uidCommand) && (uidCommand != uidMaster) )
		return false;

	CChar *pCharRemove = uidRemove.CharFind();
	if ( !pCharRemove )
		return false;
	if ( !IsInParty(pCharRemove) )
		return false;
	if ( uidRemove == uidMaster )
		return Disband(uidMaster);

	CChar *pSrc = uidCommand.CharFind();
	if ( pSrc && IsTrigUsed(TRIGGER_PARTYREMOVE) )
	{
		CScriptTriggerArgs args;
		if ( pCharRemove->OnTrigger(CTRIG_PartyRemove, pSrc, &args) == TRIGRET_RET_TRUE )
			return false;
	}
	if ( IsTrigUsed(TRIGGER_PARTYLEAVE) )
	{
		if ( pCharRemove->OnTrigger(CTRIG_PartyLeave, pCharRemove, 0) == TRIGRET_RET_TRUE )
			return false;
	}

	// Remove it from the party
	SendRemoveList(pCharRemove, true);
	DetachChar(pCharRemove);
	pCharRemove->SysMessageDefault(DEFMSG_PARTY_LEAVE_2);

	tchar *pszMsg = Str_GetTemp();
	snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_LEAVE_1), pCharRemove->GetName());
	SysMessageAll(pszMsg);

	if ( m_Chars.GetCharCount() <= 1 )
	{
		// Disband the party
		SysMessageAll(g_Cfg.GetDefaultMsg(DEFMSG_PARTY_LEAVE_LAST_PERSON));
		return Disband(uidMaster);
	}

	return true;
}

bool CPartyDef::Disband( CUID uidMaster )
{
	ADDTOCALLSTACK("CPartyDef::Disband");
	// Make sure i am the master.
	if ( m_Chars.GetCharCount() <= 0 )
		return false;
	if ( GetMaster() != uidMaster )
		return false;

	CChar *pMaster = GetMaster().CharFind();
	if ( pMaster && IsTrigUsed(TRIGGER_PARTYDISBAND) )
	{
		CScriptTriggerArgs args;
		if ( pMaster->OnTrigger(CTRIG_PartyDisband, pMaster, &args) == TRIGRET_RET_TRUE )
			return false;
	}

	SysMessageAll(g_Cfg.GetDefaultMsg(DEFMSG_PARTY_DISBANDED));

	CChar *pSrc = uidMaster.CharFind();
	size_t iQty = m_Chars.GetCharCount();
	ASSERT(iQty > 0);
	for ( size_t i = iQty; i > 0; --i )
	{
		CChar *pChar = m_Chars.GetChar(i-1).CharFind();
		if ( !pChar )
			continue;

		if ( IsTrigUsed(TRIGGER_PARTYREMOVE) )
		{
			CScriptTriggerArgs args;
			args.m_iN1 = 1;
			pChar->OnTrigger(CTRIG_PartyRemove, pSrc, &args);
		}

		SendRemoveList(pChar, true);
		DetachChar(pChar);
	}

	delete this;	// should remove itself from the world list.
	return true;
}

// ---------------------------------------------------------
bool CPartyDef::DeclineEvent( CChar *pCharDecline, CUID uidInviter )	// static
{
	ADDTOCALLSTACK("CPartyDef::DeclineEvent");
	// This should happen after a timeout as well.
	// "You notify %s that you do not wish to join the party"

	CChar *pCharInviter = uidInviter.CharFind();
	if ( !pCharInviter || !pCharDecline || uidInviter == pCharDecline->GetUID() )
		return false;

	CVarDefCont *sTempVal = pCharInviter->m_TagDefs.GetKey("PARTY_LASTINVITE");
	if ( !sTempVal || (dword)sTempVal->GetValNum() != (dword)pCharDecline->GetUID() )
		return false;

	pCharInviter->DeleteKey("PARTY_LASTINVITE");

	tchar *sTemp = Str_GetTemp();
	snprintf(sTemp, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_DECLINE_2), pCharInviter->GetName());
	pCharDecline->SysMessage(sTemp);
	sTemp = Str_GetTemp();
	snprintf(sTemp, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_DECLINE_1), pCharDecline->GetName());
	pCharInviter->SysMessage(sTemp);
	return true;
}

bool CPartyDef::AcceptEvent( CChar *pCharAccept, CUID uidInviter, bool bForced, bool bSendMessages ) // static
{
	ADDTOCALLSTACK("CPartyDef::AcceptEvent");
	// We are accepting the invite to join a party
	// No security checks if bForced -> true !!!
	// Party master is only one that can add ! GetChar(0)

	CChar *pCharInviter = uidInviter.CharFind();
	if ( !pCharInviter || !pCharInviter->IsClientActive() || !pCharAccept || !pCharAccept->IsClientActive() || (pCharInviter == pCharAccept) )
		return false;

	CPartyDef *pParty = pCharInviter->m_pParty;
	if ( !bForced )
	{
		CVarDefCont *sTempVal = pCharInviter->m_TagDefs.GetKey("PARTY_LASTINVITE");
		if ( !sTempVal || (dword)sTempVal->GetValNum() != (dword)pCharAccept->GetUID() )
			return false;

		pCharInviter->DeleteKey("PARTY_LASTINVITE");

		if ( !pCharInviter->CanSee(pCharAccept) )
			return false;
	}

	if ( pCharAccept->m_pParty )	// already in a party
	{
		if ( pParty == pCharAccept->m_pParty )	// already in this party
			return true;

		if ( bForced )
		{
			pCharAccept->m_pParty->RemoveMember(pCharAccept->GetUID(), pCharAccept->GetUID());
			pCharAccept->m_pParty = nullptr;
		}
		else
			return false;
	}

	tchar *pszMsg = Str_GetTemp();
	snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_JOINED), pCharAccept->GetName());

	if ( !pParty )
	{
		// Create the party now.
		pParty = new CPartyDef(pCharInviter, pCharAccept);
		ASSERT(pParty);
		g_World.m_Parties.InsertContentHead(pParty);
		if (bSendMessages)
			pCharInviter->SysMessage(pszMsg);
	}
	else
	{
		// Add to existing party
		if ( pParty->IsPartyFull() || (!bForced && !pParty->IsPartyMaster(pCharInviter)) )
			return false;

		if (bSendMessages)
			pParty->SysMessageAll(pszMsg);	// tell everyone already in the party about this
		pParty->AcceptMember(pCharAccept);
	}

	if (bSendMessages)
		pCharAccept->SysMessageDefault(DEFMSG_PARTY_ADDED);
	return true;
}

// ---------------------------------------------------------
enum PDV_TYPE
{
	#define ADD(a,b) PDV_##a,
	#include "../../tables/CParty_functions.tbl"
	#undef ADD
	PDV_QTY
};

lpctstr const CPartyDef::sm_szVerbKeys[PDV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../../tables/CParty_functions.tbl"
	#undef ADD
	nullptr
};

enum PDC_TYPE
{
	#define ADD(a,b) PDC_##a,
	#include "../../tables/CParty_props.tbl"
	#undef ADD
	PDC_QTY
};

lpctstr const CPartyDef::sm_szLoadKeys[PDC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../../tables/CParty_props.tbl"
	#undef ADD
	nullptr
};

bool CPartyDef::r_GetRef( lpctstr &ptcKey, CScriptObj *&pRef )
{
	ADDTOCALLSTACK("CPartyDef::r_GetRef");
	if ( !strnicmp("MASTER.", ptcKey, 7) )
	{
		ptcKey += 7;
		CChar *pMaster = GetMaster().CharFind();
		if ( pMaster )
		{
			pRef = pMaster;
			return true;
		}
	}
	else if ( !strnicmp("MEMBER.", ptcKey, 7) )
	{
		ptcKey += 7;
		size_t nNumber = Exp_GetSTVal(ptcKey);
		SKIP_SEPARATORS(ptcKey);
		if ( !m_Chars.IsValidIndex(nNumber) )
			return false;

		CChar *pMember = m_Chars.GetChar(nNumber).CharFind();
		if ( pMember )
		{
			pRef = pMember;
			return true;
		}
	}
	return false;
}

bool CPartyDef::r_LoadVal( CScript &s )
{ 
	ADDTOCALLSTACK("CPartyDef::r_LoadVal");
	EXC_TRY("LoadVal");
	lpctstr ptcKey = s.GetKey();

	int index = FindTableHeadSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
	switch ( index )
	{
		case PDC_SPEECHFILTER:
		{
			if ( !s.HasArgs() )
				this->m_pSpeechFunction.Clear();
			else
			{
				lpctstr ptcArg = s.GetArgStr();
				CResourceLink *m_pTestEvent = dynamic_cast<CResourceLink *>(g_Cfg.ResourceGetDefByName(RES_FUNCTION, ptcArg));

				if ( !m_pTestEvent )
					return false;

				this->m_pSpeechFunction.Format("%s", ptcArg);
			}

		} break;

		case PDC_TAG0:
		case PDC_TAG:
		    {
            const bool fZero = (index == PDC_TAG0);
            ptcKey += (fZero ? 5 : 4);
            bool fQuoted = false;
            lpctstr ptcArg = s.GetArgStr(&fQuoted);
            m_TagDefs.SetStr(ptcKey, fQuoted, ptcArg, fZero);
		} break;
		
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CPartyDef::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole *pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallParent);
    UNREFERENCED_PARAMETER(fNoCallChildren);
	ADDTOCALLSTACK("CPartyDef::r_WriteVal");
	EXC_TRY("WriteVal");

	CScriptObj *pRef;
	if ( r_GetRef(ptcKey, pRef) )
	{
		if ( pRef == nullptr )		// good command but bad link.
		{
			sVal = "0";
			return true;
		}
		if ( ptcKey[0] == '\0' )	// we where just testing the ref.
		{
			CObjBase *pObj = dynamic_cast<CObjBase*>(pRef);
			if ( pObj )
				sVal.FormatHex((dword)pObj->GetUID());
			else
				sVal.FormatVal(1);
			return true;
		}
		return pRef->r_WriteVal(ptcKey, sVal, pSrc);
	}

	bool fZero = false;
	switch ( FindTableHeadSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1) )
	{
		case PDC_ISSAMEPARTYOF:
		{
			ptcKey += 13;
			GETNONWHITESPACE(ptcKey);
			if ( ptcKey[0] != '\0' )
			{
				CChar *pCharToCheck = CUID::CharFindFromUID(Exp_GetDWVal(ptcKey));
				sVal.FormatVal(pCharToCheck && (pCharToCheck->m_pParty == this));
			}
			else
				return false;
		} break;

		case PDC_MEMBERS:
			sVal.FormatSTVal(m_Chars.GetCharCount());
			break;

		case PDC_SPEECHFILTER:
			sVal = this->m_pSpeechFunction.IsEmpty() ? "" : this->m_pSpeechFunction;
			break;

		case PDC_TAG0:
			fZero = true;
			++ptcKey;
		case PDC_TAG:
		{
			if ( ptcKey[3] != '.' )
				return false;
			ptcKey += 4;
			sVal = m_TagDefs.GetKeyStr(ptcKey, fZero);
		} break;

		case PDC_TAGAT:
		{
			ptcKey += 5;	// eat the 'TAGAT'
			if ( *ptcKey == '.' )	// do we have an argument?
			{
				SKIP_SEPARATORS(ptcKey);
				size_t iQty = Exp_GetSTVal(ptcKey);
				if ( iQty >= m_TagDefs.GetCount() )
					return false;	// trying to get non-existant tag

				const CVarDefCont *pTagAt = m_TagDefs.GetAt(iQty);
				if ( !pTagAt )
					return false;	// trying to get non-existant tag

				SKIP_SEPARATORS(ptcKey);
				if ( !*ptcKey )
				{
					sVal.Format("%s=%s", pTagAt->GetKey(), pTagAt->GetValStr());
					return true;
				}
				else if ( !strnicmp(ptcKey, "KEY", 3) )
				{
					sVal = pTagAt->GetKey();
					return true;
				}
				else if ( !strnicmp(ptcKey, "VAL", 3) )
				{
					sVal = pTagAt->GetValStr();
					return true;
				}
			}
			return false;
		}

		case PDC_TAGCOUNT:
			sVal.FormatSTVal(m_TagDefs.GetCount());
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CPartyDef::r_Verb( CScript &s, CTextConsole *pSrc )
{
	ADDTOCALLSTACK("CPartyDef::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

	lpctstr ptcKey = s.GetKey();
	CScriptObj *pRef;
	if ( r_GetRef(ptcKey, pRef) )
	{
		if ( ptcKey[0] )
		{
			if ( !pRef )
				return true;
			CScript script(ptcKey, s.GetArgStr());
			script.CopyParseState(s);
			return pRef->r_Verb(script, pSrc);
		}
	}

	int iIndex = FindTableSorted(ptcKey, sm_szVerbKeys, CountOf(sm_szVerbKeys) - 1);
	switch ( iIndex )
	{
		case PDV_ADDMEMBER:
		case PDV_ADDMEMBERFORCED:
		{
			const bool fForced = (iIndex == PDV_ADDMEMBERFORCED);
			CUID toAdd(s.GetArgDWVal());
			CChar *pCharAdd = toAdd.CharFind();
			CChar *pCharMaster = GetMaster().CharFind();
			if ( !pCharAdd || IsInParty(pCharAdd) )
				return false;

			if ( pCharMaster && !fForced )
				pCharMaster->SetKeyNum("PARTY_LASTINVITE", toAdd.GetObjUID());
			
			return CPartyDef::AcceptEvent(pCharAdd, GetMaster(), fForced);
		} break;

		case PDV_CLEARTAGS:
		{
			lpctstr ptcArg = s.GetArgStr();
			SKIP_SEPARATORS(ptcArg);
			m_TagDefs.ClearKeys(ptcArg);
		} break;

		case PDV_CREATE:
			return true;

		case PDV_DISBAND:
			return Disband(GetMaster());

		case PDV_MESSAGE:
			break;

		case PDV_REMOVEMEMBER:
		{
			CUID toRemove;
			lpctstr ptcArg = s.GetArgStr();
			if ( *ptcArg == '@' )
			{
				++ptcArg;
				const size_t nMember = Exp_GetSTVal(ptcArg);
				if ( !m_Chars.IsValidIndex(nMember) )
					return false;

				toRemove = m_Chars.GetChar(nMember);
			}
			else
				toRemove.SetObjUID(s.GetArgDWVal());

			if ( toRemove.IsValidUID() )
				return RemoveMember(toRemove, GetMaster());

			return false;
		} break;

		case PDV_SETMASTER:
		{
			CUID newMaster;
			lpctstr ptcArg = s.GetArgStr();
			if ( *ptcArg == '@' )
			{
				++ptcArg;
				const size_t nMember = Exp_GetSTVal(ptcArg);
				if ( (nMember == 0) || !m_Chars.IsValidIndex(nMember) )
					return false;

				newMaster = m_Chars.GetChar(nMember);
			}
			else
				newMaster.SetObjUID(s.GetArgDWVal());

			if ( newMaster.IsValidUID() )
				return SetMaster(newMaster.CharFind());

			return false;
		} break;

		case PDV_SYSMESSAGE:
		{
			CUID toSysmessage;
			lpctstr ptcArg = s.GetArgStr();
			tchar *ptcUid = Str_GetTemp();
			int x = 0;

			if ( *ptcArg == '@' )
			{
				++ptcArg;
				if ( *ptcArg != '@' )
				{
					lpctstr __pszArg = ptcArg;
					while ( *ptcArg != ' ' )
					{
						++ptcArg;
						++x;
					}
                    Str_CopyLimitNull(ptcUid, __pszArg, ++x);

					const size_t nMember = Exp_GetSTVal(ptcUid);
					if ( !m_Chars.IsValidIndex(nMember) )
						return false;

					toSysmessage = m_Chars.GetChar(nMember);
				}
			}
			else
			{
				lpctstr __pszArg = ptcArg;
				while ( *ptcArg != ' ' )
				{
					++ptcArg;
					++x;
				}
                Str_CopyLimitNull(ptcUid, __pszArg, ++x);

				toSysmessage.SetObjUID(Exp_GetDWVal(ptcUid));
			}

			SKIP_SEPARATORS(ptcArg);

			if ( toSysmessage.IsValidUID() )
			{
				CChar *pSend = toSysmessage.CharFind();
				pSend->SysMessage(ptcArg);
			}
			else
				SysMessageAll(ptcArg);
		} break;

		case PDV_TAGLIST:
			m_TagDefs.DumpKeys(pSrc, "TAG.");
			break;

		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CPartyDef::r_Load( CScript &s )
{ 
	ADDTOCALLSTACK("CPartyDef::r_Load");
	UNREFERENCED_PARAMETER(s);
	return false; 
}

lpctstr CPartyDef::GetDefStr( lpctstr ptcKey, bool fZero ) const
{
	return m_BaseDefs.GetKeyStr( ptcKey, fZero );
}

int64 CPartyDef::GetDefNum( lpctstr ptcKey ) const
{
	return m_BaseDefs.GetKeyNum( ptcKey );
}

void CPartyDef::SetDefNum(lpctstr ptcKey, int64 iVal, bool fZero )
{
	m_BaseDefs.SetNum(ptcKey, iVal, fZero);
}

void CPartyDef::SetDefStr(lpctstr ptcKey, lpctstr pszVal, bool fQuoted, bool fZero )
{
	m_BaseDefs.SetStr(ptcKey, fQuoted, pszVal, fZero);
}

void CPartyDef::DeleteDef(lpctstr ptcKey)
{
	m_BaseDefs.DeleteKey(ptcKey);
}

bool CPartyDef::IsPartyFull() const
{
	return (m_Chars.GetCharCount() >= MAX_CHAR_IN_PARTY);
}

bool CPartyDef::IsInParty( const CChar * pChar ) const
{
	return m_Chars.IsCharIn( pChar );
}

bool CPartyDef::IsPartyMaster( const CChar * pChar ) const
{
	return (m_Chars.FindChar( pChar ) == 0);
}

CUID CPartyDef::GetMaster()
{
	return m_Chars.GetChar(0);
}