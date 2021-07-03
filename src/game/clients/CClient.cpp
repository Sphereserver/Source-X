
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../../network/CClientIterator.h"
#include "../../network/CNetworkManager.h"
#include "../../network/CIPHistoryManager.h"
#include "../../network/send.h"
#include "../../network/packet.h"
#include "../chars/CChar.h"
#include "../components/CCSpawn.h"
#include "../items/CItemMultiCustom.h"
#include "../CServer.h"
#include "../CWorld.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../spheresvr.h"
#include "../triggers.h"
#include "CParty.h"
#include "CClient.h"


/////////////////////////////////////////////////////////////////
// -CClient stuff.

CClient::CClient(CNetState* state)
{
	// This may be a web connection or Telnet ?
	m_net = state;
	SetConnectType( CONNECT_UNK );	// don't know what sort of connect this is yet.

	// update ip history
	HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
	++ history.m_connecting;
	++ history.m_connected;

	m_Crypt.SetClientVer( g_Serv.m_ClientVersion );
	m_pAccount = nullptr;
	m_pChar = nullptr;
	m_pGMPage = nullptr;

	m_timeLogin = 0;
	m_timeLastEvent = CWorldGameTime::GetCurrentTime().GetTimeRaw();
	m_timeLastEventWalk = CWorldGameTime::GetCurrentTime().GetTimeRaw();
	m_timeNextEventWalk = 0;

	m_iWalkStepCount = 0;
	m_iWalkTimeAvg	= 500;
	m_timeWalkStep = CSTime::GetPreciseSysTimeMilli();
	m_lastDir = 0;

    _fShowPublicHouseContent = true;

	m_Targ_Timeout = 0;
	m_Targ_Mode = CLIMODE_SETUP_CONNECTING;
	m_Prompt_Mode = CLIMODE_NORMAL;

	m_tmSetup.m_dwIP = 0;
	m_tmSetup.m_iConnect = 0;
	m_tmSetup.m_bNewSeed = false;

	m_Env.SetInvalid();

	g_Log.Event(LOGM_CLIENTS_LOG, "%x:Client connected [Total:%" PRIuSIZE_T "]. IP='%s'. (Connecting/Connected: %d/%d).\n",
		GetSocketID(), g_Serv.StatGet(SERV_STAT_CLIENTS), GetPeerStr(), history.m_connecting, history.m_connected);

	m_zLastMessage[0] = 0;
	m_zLastObjMessage[0] = 0;
	m_tNextPickup = 0;

    m_BfAntiCheat = {};
    m_ScreenSize = {};
	m_pPopupPacket = nullptr;
	m_pHouseDesign = nullptr;
	m_fUpdateStats = 0;

    m_timeLastSkillThrowing = 0;
    m_pSkillThrowingTarg = nullptr;
	m_SkillThrowingAnimID = ITEMID_NOTHING;
	m_SkillThrowingAnimHue = 0;
	m_SkillThrowingAnimRender = 0;
}


CClient::~CClient()
{
	EXC_TRY("Cleanup in destructor");

	ADDTOCALLSTACK("CClient::~CClient");

	// update ip history
	HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
	if ( GetConnectType() != CONNECT_GAME )
		--history.m_connecting;
	--history.m_connected;

	const bool fWasChar = ( m_pChar != nullptr );

	CharDisconnect();	// am i a char in game ?

	if (m_pGMPage)
		m_pGMPage->ClearHandler();

	// Clear containers (CTAG and TOOLTIP)
	m_TagDefs.Clear();

	CAccount * pAccount = GetAccount();
	if ( pAccount )
	{
		pAccount->OnLogout(this, fWasChar);
		m_pAccount = nullptr;
	}

	if (m_pPopupPacket != nullptr)
	{
		delete m_pPopupPacket;
		m_pPopupPacket = nullptr;
	}

	if (m_net->isClosed() == false)
		g_Log.EventError("Client being deleted without being safely removed from the network system\n");

	EXC_CATCH;
}

bool CClient::CanInstantLogOut() const
{
	ADDTOCALLSTACK("CClient::CanInstantLogOut");
	if ( g_Serv.IsLoading())	// or exiting.
		return true;
	if ( ! g_Cfg.m_iClientLingerTime )
		return true;
	if ( GetPrivLevel() > PLEVEL_Player )
		return true;
	if ( m_pChar == nullptr )
		return true;
	if ( m_pChar->IsStatFlag(STATF_DEAD))
		return true;

	const CRegionWorld * pArea = m_pChar->GetRegion();
	if ( pArea == nullptr )
		return true;
	if ( pArea->IsFlag( REGION_FLAG_INSTA_LOGOUT ))
		return true;

	const CRegion * pRoom = m_pChar->GetRoom(); //Allows Room flag to work!
	if ( pRoom && pRoom->IsFlag( REGION_FLAG_INSTA_LOGOUT )) //sanity check for null rooms // Can C++ guarantee short-circuit evaluation for CRegion ?
		return true;

	return false;
}

void CClient::CharDisconnect()
{
	ADDTOCALLSTACK("CClient::CharDisconnect");
	// Disconnect the CChar from the client.
	// Even tho the CClient might stay active.
	if ( !m_pChar )
		return;
	int64 iLingerTime = g_Cfg.m_iClientLingerTime / MSECS_PER_SEC;

	Announce(false);
	bool fCanInstaLogOut = CanInstantLogOut();

	//	stoned chars cannot logout if they are not privileged of course
	if ( m_pChar->IsStatFlag(STATF_STONE) )
	{
		iLingerTime = 60*60;	// 1 hour of linger time
		fCanInstaLogOut = false;
	}

	//	we are not a client anymore
	if ( IsChatActive() )
		g_Serv.m_Chats.QuitChat(this);

	if ( m_pHouseDesign )
		m_pHouseDesign->EndCustomize(true);

	if ( IsTrigUsed(TRIGGER_LOGOUT) )
	{
		CScriptTriggerArgs Args(iLingerTime, fCanInstaLogOut);
		m_pChar->OnTrigger(CTRIG_LogOut, m_pChar, &Args);
		iLingerTime = (int)(Args.m_iN1);
		fCanInstaLogOut = (Args.m_iN2 != 0);
	}

	if ( iLingerTime <= 0 )
		fCanInstaLogOut = true;

	// log out immediately ? (test before ClientDetach())
	if ( ! fCanInstaLogOut )
	{
		// become an NPC for a little while
		CItem * pItemChange = CItem::CreateBase( ITEMID_RHAND_POINT_W );
		ASSERT(pItemChange);
		pItemChange->SetName("Client Linger");
		pItemChange->SetType(IT_EQ_CLIENT_LINGER);
		pItemChange->SetTimeoutS(iLingerTime);
		m_pChar->LayerAdd(pItemChange, LAYER_FLAG_ClientLinger);
	}
	else
	{
		// remove me from other clients screens now.
		m_pChar->SetDisconnected();
	}
    m_pChar->ClientDetach();	// we are not a client any more.

    // Gump memory cleanup, we don't want them on logged out players
    m_mapOpenedGumps.clear();

    // Layer dragging, moving it to backpack
    CItem * pItemDragging = m_pChar->LayerFind(LAYER_DRAGGING);
    if ( pItemDragging )
        m_pChar->ItemBounce(pItemDragging);

	m_pChar = nullptr;
}

void CClient::SysMessage( lpctstr pszMsg ) const // System message (In lower left corner)
{
	ADDTOCALLSTACK("CClient::SysMessage");
	// Diff sorts of clients.
	if ( !pszMsg || !*pszMsg ) return;

	switch ( GetConnectType() )
	{
		case CONNECT_TELNET:
		case CONNECT_AXIS:
			{
				if ( *pszMsg == '\0' )
                    return;

				new PacketTelnet(this, pszMsg);
			}
			return;
		case CONNECT_UOG:
			{
				if ( *pszMsg == '\0' )
                    return;

				new PacketTelnet(this, pszMsg, true);
			}
			return;
		case CONNECT_CRYPT:
		case CONNECT_LOGIN:
		case CONNECT_GAME:
			const_cast <CClient*>(this)->addSysMessage(pszMsg);
			return;

		case CONNECT_HTTP:
			const_cast <CClient*>(this)->m_Targ_Text = pszMsg;
			return;

		default:
			return;
	}
}

void CClient::Announce( bool fArrive ) const
{
	ADDTOCALLSTACK("CClient::Announce");
	if ( !GetAccount() || !GetChar() || !GetChar()->m_pPlayer )
		return;

	// We have logged in or disconnected.
	// Annouce my arrival or departure.
	tchar *pszMsg = Str_GetTemp();
	if ( (g_Cfg.m_iArriveDepartMsg == 2) && (GetPrivLevel() > PLEVEL_Player) )		// notify of GMs
	{
		lpctstr zTitle = m_pChar->Noto_GetFameTitle();
		snprintf(pszMsg, STR_TEMPLENGTH, "@231 STAFF: %s%s logged %s.", zTitle, m_pChar->GetName(), (fArrive ? "in" : "out"));
	}
	else if ( g_Cfg.m_iArriveDepartMsg == 1 )		// notify of players
	{
		const CRegion *pRegion = m_pChar->GetTopPoint().GetRegion(REGION_TYPE_AREA);
		snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_ARRDEP_1),
			m_pChar->GetName(),
			fArrive ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_ARRDEP_2) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_ARRDEP_3),
			pRegion != nullptr ? pRegion->GetName() : g_Serv.GetName());
	}
	if ( pszMsg )
	{
		ClientIterator it;
		for (CClient *pClient = it.next(); pClient != nullptr; pClient = it.next())
		{
			if ( (pClient == this) || (GetPrivLevel() > pClient->GetPrivLevel()) )
				continue;
			pClient->SysMessage(pszMsg);
		}

	}

	// Check murder decay timer
	CItem *pMurders = m_pChar->LayerFind(LAYER_FLAG_Murders);
	if ( pMurders )
	{
		if ( fArrive )	// on client login, set active timer on murder memory
			pMurders->SetTimeoutS(pMurders->m_itEqMurderCount.m_dwDecayBalance);
		else			// or make it inactive on logout
		{
			pMurders->m_itEqMurderCount.m_dwDecayBalance = (dword)(pMurders->GetTimerSAdjusted());
			pMurders->SetTimeout(-1);
		}
	}
	else if ( fArrive )
		m_pChar->Noto_Murder();		// if there's no murder memory found, check if we need a new memory
}

////////////////////////////////////////////////////

bool CClient::CanSee( const CObjBaseTemplate * pObj ) const
{
	ADDTOCALLSTACK("CClient::CanSee");
	// Can player see item b
	if ( !m_pChar || !pObj )
		return false;

	if ( pObj->IsChar() )
	{
		const CChar *pChar = static_cast<const CChar*>(pObj);
		if ( pChar->IsDisconnected() )
        {
            if( !IsPriv(PRIV_ALLSHOW) )
                return false;
            //dont show pet when is ridden (cause double)
            else if ( pChar->IsStatFlag(STATF_PET) && pChar->IsStatFlag(STATF_RIDDEN) )
                return false;
        }
	}
	return m_pChar->CanSee( pObj );
}

bool CClient::CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const
{
	ADDTOCALLSTACK("CClient::CanHear");
	// can we hear this text or sound.

	if ( !IsConnectTypePacket() )
		return false;
	if ( (mode == TALKMODE_BROADCAST) || !pSrc )
		return true;
	if ( !m_pChar )
		return false;

	if ( IsPriv(PRIV_HEARALL) &&
		pSrc->IsChar() &&
		( mode == TALKMODE_SAY || mode == TALKMODE_WHISPER || mode == TALKMODE_YELL ) ) // HEARALL works only for this talkmodes
	{
		const CChar * pCharSrc = dynamic_cast <const CChar*> ( pSrc );
		ASSERT(pCharSrc);
		if ( pCharSrc->IsClientActive() && (pCharSrc->GetPrivLevel() <= GetPrivLevel()) )
			return true;
	}

	return m_pChar->CanHear( pSrc, mode );
}

////////////////////////////////////////////////////


void CClient::addTargetVerb( lpctstr pszCmd, lpctstr ptcArg )
{
	ADDTOCALLSTACK("CClient::addTargetVerb");
	// Target a verb at some object .

	ASSERT(pszCmd);
	GETNONWHITESPACE(pszCmd);
	SKIP_SEPARATORS(pszCmd);

	if ( !strlen(pszCmd) )
		pszCmd = ptcArg;

	if ( pszCmd == ptcArg )
	{
		GETNONWHITESPACE(pszCmd);
		SKIP_SEPARATORS(pszCmd);
		ptcArg = "";
	}

	// priv here
	PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( pszCmd );
	if ( ilevel > GetPrivLevel() )
		return;

	m_Targ_Text.Format( "%s%s%s", pszCmd, ( ptcArg[0] && pszCmd[0] ) ? " " : "", ptcArg );
	tchar * pszMsg = Str_GetTemp();
	snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_TARGET_COMMAND), m_Targ_Text.GetBuffer());
	addTarget(CLIMODE_TARG_OBJ_SET, pszMsg);
}

void CClient::addTargetFunctionMulti( lpctstr pszFunction, ITEMID_TYPE itemid, HUE_TYPE color, bool fAllowGround )
{
	ADDTOCALLSTACK("CClient::addTargetFunctionMulti");
	// Target a verb at some object .
	ASSERT(pszFunction);
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPARATORS(pszFunction);

	m_Targ_Text = pszFunction;
	if ( CItemBase::IsID_Multi( itemid ))	// a multi we get from Multi.mul
	{
		SetTargMode(CLIMODE_TARG_OBJ_FUNC);
		new PacketAddTarget(this, fAllowGround ? PacketAddTarget::Ground : PacketAddTarget::Object,
            CLIMODE_TARG_OBJ_FUNC, PacketAddTarget::None, itemid, color);
	}
	addTargetFunction( pszFunction, fAllowGround, false );
}

void CClient::addTargetFunction( lpctstr pszFunction, bool fAllowGround, bool fCheckCrime )
{
	ADDTOCALLSTACK("CClient::addTargetFunction");
	// Target a verb at some object .
	ASSERT(pszFunction);
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPARATORS(pszFunction);

	m_Targ_Text = pszFunction;
	addTarget( CLIMODE_TARG_OBJ_FUNC, nullptr, fAllowGround, fCheckCrime );
}

void CClient::addPromptConsoleFunction( lpctstr pszFunction, lpctstr pszSysmessage, bool bUnicode )
{
	ADDTOCALLSTACK("CClient::addPromptConsoleFunction");
	// Target a verb at some object .
	ASSERT(pszFunction);
	m_Prompt_Text = pszFunction;
	addPromptConsole( CLIMODE_PROMPT_SCRIPT_VERB, pszSysmessage, CUID(), CUID(), bUnicode );
}


enum CLIR_TYPE
{
	CLIR_ACCOUNT,
	CLIR_GMPAGEP,
	CLIR_HOUSEDESIGN,
	CLIR_PARTY,
	CLIR_TARG,
	CLIR_TARGPROP,
	CLIR_TARGPRV,
	CLIR_QTY
};

lpctstr const CClient::sm_szRefKeys[CLIR_QTY+1] =
{
	"ACCOUNT",
	"GMPAGEP",
	"HOUSEDESIGN",
	"PARTY",
	"TARG",
	"TARGPROP",
	"TARGPRV",
	nullptr
};

bool CClient::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CClient::r_GetRef");
	int i = FindTableHeadSorted( ptcKey, sm_szRefKeys, CountOf(sm_szRefKeys)-1 );
	if ( i >= 0 )
	{
		ptcKey += strlen( sm_szRefKeys[i] );
		SKIP_SEPARATORS(ptcKey);
		switch (i)
		{
			case CLIR_ACCOUNT:
				if ( ptcKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = GetAccount();
				return true;
			case CLIR_GMPAGEP:
				pRef = m_pGMPage;
				return true;
			case CLIR_HOUSEDESIGN:
				pRef = m_pHouseDesign;
				return true;
			case CLIR_PARTY:
				if (CChar* pCharThis = GetChar())
				{
					if (!strnicmp(ptcKey, "CREATE", 7))
					{
						if (pCharThis->m_pParty)
							return false;

						lpctstr oldKey = ptcKey;
						ptcKey += 7;

						// Do i want to send the "joined" message to the party members?
						bool fSendMsgs = (Exp_GetSingle(ptcKey) != 0) ? true : false;

						// Add all the UIDs to the party
						for (int ip = 0; ip < 10; ++ip)
						{
							SKIP_ARGSEP(ptcKey);
							CChar* pChar = CUID::CharFindFromUID(Exp_GetDWSingle(ptcKey));
							if (!pChar)
								continue;
							if (!pChar->IsClientActive())
								continue;
							CPartyDef::AcceptEvent(pChar, pChar->GetUID(), true, fSendMsgs);

							if (*ptcKey == '\0')
								break;
						}
						ptcKey = oldKey;	// Restoring back to real ptcKey, so we don't get errors for giving an uid instead of PDV_CREATE.
					}
					if (!pCharThis->m_pParty)
						return false;
					pRef = pCharThis->m_pParty;
					return true;
				}
				return false;
			case CLIR_TARG:
				pRef = m_Targ_UID.ObjFind();
				return true;
			case CLIR_TARGPRV:
				pRef = m_Targ_Prv_UID.ObjFind();
				return true;
			case CLIR_TARGPROP:
				pRef = m_Prop_UID.ObjFind();
				return true;
		}
	}

	return CScriptObj::r_GetRef( ptcKey, pRef );
}

lpctstr const CClient::sm_szLoadKeys[CC_QTY+1] = // static
{
	#define ADD(a,b) b,
	#include "../../tables/CClient_props.tbl"
	#undef ADD
	nullptr
};

lpctstr const CClient::sm_szVerbKeys[CV_QTY+1] =	// static
{
	#define ADD(a,b) b,
	#include "../../tables/CClient_functions.tbl"
	#undef ADD
	nullptr
};

bool CClient::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
	ADDTOCALLSTACK("CClient::r_WriteVal");
	EXC_TRY("WriteVal");
	
	if ( !strnicmp("CTAG", ptcKey, 4) )
	{
        bool fCtag = false;
        bool fZero = false;
        if (ptcKey[4] == '.')   //	CTAG.xxx - client tag
        {
            fCtag = true;
            ptcKey += 5;
        }
        else if ((ptcKey[4] == '0') && (ptcKey[5] == '.'))  //	CTAG0.xxx - client tag
        {
            fCtag = true;
            fZero = true;
            ptcKey += 6;
        }
        if (fCtag)
        {
            sVal = m_TagDefs.GetKeyStr(ptcKey, fZero);
            return true;
        }
	}

    int index;
	if ( !strnicmp( "TARGP", ptcKey, 5 ) && ( ptcKey[5] == '\0' || ptcKey[5] == '.' ) )
		index = CC_TARGP;
	else if ( !strnicmp( "SCREENSIZE", ptcKey, 10 ) && ( ptcKey[10] == '\0' || ptcKey[10] == '.' ) )
		index = CC_SCREENSIZE;
	else if ( !strnicmp( "REPORTEDCLIVER", ptcKey, 14 ) && ( ptcKey[14] == '\0' || ptcKey[14] == '.' ) )
		index = CC_REPORTEDCLIVER;
	else
		index = FindTableSorted( ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 );

	switch (index)
	{
		case CC_ALLMOVE:
			sVal.FormatVal( IsPriv( PRIV_ALLMOVE ));
			break;
		case CC_ALLSHOW:
			sVal.FormatVal( IsPriv( PRIV_ALLSHOW ));
			break;
		case CC_CLIENTIS3D:
			sVal.FormatVal( GetNetState()->isClient3D() );
			break;
		case CC_CLIENTISKR:
			sVal.FormatVal( GetNetState()->isClientKR() );
			break;
		case CC_CLIENTISENHANCED:
			sVal.FormatVal( GetNetState()->isClientEnhanced() );
			break;
		case CC_CLIENTVERSION:
			{
				char szVersion[ 128 ];
				sVal = m_Crypt.WriteClientVer( szVersion, sizeof(szVersion) );
			}
			break;
		case CC_DEBUG:
			sVal.FormatVal( IsPriv( PRIV_DEBUG ));
			break;
		case CC_DETAIL:
			sVal.FormatVal( IsPriv( PRIV_DETAIL ));
			break;
		case CC_GM:	// toggle your GM status on/off
			sVal.FormatVal( IsPriv( PRIV_GM ));
			break;
		case CC_HEARALL:
			sVal.FormatVal( IsPriv( PRIV_HEARALL ));
			break;
		case CC_LASTEVENT:
			sVal.FormatLLVal( m_timeLastEvent );
			break;
        case CC_LASTEVENTWALK:
            sVal.FormatLLVal( m_timeLastEventWalk );
            break;
		case CC_PRIVSHOW:
			// Show my priv title.
			sVal.FormatVal( ! IsPriv( PRIV_PRIV_NOSHOW ));
			break;
		case CC_REPORTEDCLIVER:
			{
				ptcKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE(ptcKey);

				dword iCliVer = GetNetState()->getReportedVersion();
				if ( ptcKey[0] == '\0' )
				{
					// Return full version string (eg: 5.0.2d)
					tchar ptcVersion[128];
					sVal = CCrypto::WriteClientVerString(iCliVer, ptcVersion, sizeof(ptcVersion));
				}
				else
				{
					// Return raw version number (eg: 5.0.2d = 5000204)
					sVal.FormatUVal(iCliVer);
				}
			}
			break;
		case CC_SCREENSIZE:
			{
				if ( ptcKey[10] == '.' )
				{
					ptcKey += strlen(sm_szLoadKeys[index]);
					SKIP_SEPARATORS(ptcKey);

					if ( !strnicmp("X", ptcKey, 1) )
						sVal.Format( "%u", m_ScreenSize.x );
					else if ( !strnicmp("Y", ptcKey, 1) )
						sVal.Format( "%u", m_ScreenSize.y );
					else
						return false;
				}
				else
					sVal.Format( "%u,%u", m_ScreenSize.x, m_ScreenSize.y );
			} break;
		case CC_TARG:
			sVal.FormatHex(m_Targ_UID);
			break;
		case CC_TARGP:
			if ( ptcKey[5] == '.' )
			{
				return m_Targ_p.r_WriteVal( ptcKey+6, sVal );
			}
			sVal = m_Targ_p.WriteUsed();
			break;
		case CC_TARGPROP:
			sVal.FormatHex(m_Prop_UID);
			break;
		case CC_TARGPRV:
			sVal.FormatHex(m_Targ_Prv_UID);
			break;
		case CC_TARGTXT:
			sVal = m_Targ_Text;
			break;
		default:
            //Special case: if fNoCallParent = true, call CScriptObj::r_WriteVal with fNoCallChildren = true, to check only for the ref
			return CScriptObj::r_WriteVal( ptcKey, sVal, pSrc, false, fNoCallParent );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CClient::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CClient::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( GetAccount() == nullptr )
		return false;

	lpctstr ptcKey = s.GetKey();
    if (!strnicmp("CTAG", ptcKey, 4))
    {
        if ((ptcKey[4] == '.') || (ptcKey[4] == '0'))
        {
            const bool fZero = ptcKey[4] == '0';
            ptcKey = ptcKey + (fZero ? 6 : 5);
            bool fQuoted = false;
            lpctstr ptcArg = s.GetArgStr(&fQuoted);
            m_TagDefs.SetStr(ptcKey, fQuoted, ptcArg, fZero);
            return true;
        }
    }

	switch ( FindTableSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 ))
	{
		case CC_ALLMOVE:
			addRemoveAll(true, false);
			GetAccount()->TogPrivFlags( PRIV_ALLMOVE, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
				GetChar()->SysMessage( IsPriv(PRIV_ALLMOVE)? "Allmove ON" : "Allmove OFF" );
			addPlayerView(CPointMap());
			break;
		case CC_ALLSHOW:
			addRemoveAll(false, true);
			GetAccount()->TogPrivFlags( PRIV_ALLSHOW, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
                GetChar()->SysMessage( IsPriv(PRIV_ALLSHOW)? "Allshow ON" : "Allshow OFF" );
			addPlayerView(CPointMap());
			break;
		case CC_DEBUG:
			addRemoveAll(true, false);
			GetAccount()->TogPrivFlags( PRIV_DEBUG, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
                GetChar()->SysMessage( IsPriv(PRIV_DEBUG)? "Debug ON" : "Debug OFF" );
			addPlayerView(CPointMap());
			break;
		case CC_DETAIL:
			GetAccount()->TogPrivFlags( PRIV_DETAIL, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
                GetChar()->SysMessage( IsPriv(PRIV_DETAIL)? "Detail ON" : "Detail OFF" );
			break;
		case CC_GM: // toggle your GM status on/off
			if ( GetPrivLevel() >= PLEVEL_GM )
			{
				GetAccount()->TogPrivFlags( PRIV_GM, s.GetArgStr() );
                GetChar()->UpdatePropertyFlag();
				if ( IsSetOF( OF_Command_Sysmsgs ) )
                    GetChar()->SysMessage( IsPriv(PRIV_GM)? "GM ON" : "GM OFF" );
			}
			break;
		case CC_HEARALL:
			GetAccount()->TogPrivFlags( PRIV_HEARALL, s.GetArgStr() );
			if ( IsSetOF( OF_Command_Sysmsgs ) )
                GetChar()->SysMessage( IsPriv(PRIV_HEARALL)? "Hearall ON" : "Hearall OFF" );
			break;
		case CC_PRIVSHOW:
			// Hide my priv title.
			if ( GetPrivLevel() >= PLEVEL_Counsel )
			{
				if ( ! s.HasArgs())
					GetAccount()->TogPrivFlags( PRIV_PRIV_NOSHOW, nullptr );
				else if ( s.GetArgVal() )
					GetAccount()->ClearPrivFlags( PRIV_PRIV_NOSHOW );
				else
					GetAccount()->SetPrivFlags( PRIV_PRIV_NOSHOW );

                GetChar()->UpdatePropertyFlag();
				if ( IsSetOF( OF_Command_Sysmsgs ) )
                    GetChar()->SysMessage( IsPriv(PRIV_PRIV_NOSHOW)? "Privshow OFF" : "Privshow ON" );
			}
			break;

		case CC_TARG:
			m_Targ_UID.SetObjUID(s.GetArgVal());
			break;
		case CC_TARGP:
			m_Targ_p.Read( s.GetArgRaw());
			if ( !m_Targ_p.IsValidPoint() )
			{
				m_Targ_p.ValidatePoint();
				SysMessagef( "Invalid point: %s", s.GetArgStr() );
			}
			break;
		case CC_TARGPROP:
			m_Prop_UID.SetObjUID(s.GetArgVal());
			break;
		case CC_TARGPRV:
			m_Targ_Prv_UID.SetObjUID(s.GetArgVal());
			break;
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

bool CClient::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CClient::r_Verb");
	ASSERT(pSrc);

	// NOTE: This can be called directly from a RES_WEBPAGE script.
	//  So do not assume we are a game client !
	// NOTE: Mostly called from CChar::r_Verb
	// NOTE: Little security here so watch out for dangerous scripts !

	EXC_TRY("Verb-Special");
	lpctstr ptcKey = s.GetKey();

	// Old ver
	if ( s.IsKeyHead( "SET", 3 ) && ( g_Cfg.m_Functions.ContainsKey( ptcKey ) == false ) )
	{
		PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( "SET" );
		if ( ilevel > GetPrivLevel() )
			return false;

		ASSERT( m_pChar );
		addTargetVerb( ptcKey+3, s.GetArgRaw());
		return true;
	}

	if ( toupper( ptcKey[0] ) == 'X' && ( g_Cfg.m_Functions.ContainsKey( ptcKey ) == false ) )
	{
		PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( "SET" );
		if ( ilevel > GetPrivLevel() )
			return false;

		// Target this command verb on some other object.
		ASSERT( m_pChar );
		addTargetVerb( ptcKey+1, s.GetArgRaw());
		return true;
	}

	EXC_SET_BLOCK("Verb-Statement");
	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys)-1 );
	switch (index)
	{
		case CV_ADD:
			if ( s.HasArgs())
			{
				tchar *ppszArgs[2];
				size_t iQty = Str_ParseCmds(s.GetArgStr(), ppszArgs, CountOf(ppszArgs));

				if ( !IsValidGameObjDef(ppszArgs[0]) )
				{
					//g_Log.EventWarn("Invalid ADD argument '%s'\n", pszArgs);
					SysMessageDefault( DEFMSG_CMD_INVALID );
					return true;
				}

				CResourceID rid = g_Cfg.ResourceGetID( RES_QTY, ppszArgs[0]);
				m_tmAdd.m_id = rid.GetResIndex();
				if (iQty > 1)
				{
					m_tmAdd.m_vcAmount = (word)atoi(ppszArgs[1]);
					m_tmAdd.m_vcAmount = maximum(m_tmAdd.m_vcAmount, 1);
				}
				else
					m_tmAdd.m_vcAmount = 1;
				if ( (rid.GetResType() == RES_CHARDEF) || (rid.GetResType() == RES_SPAWN) )
				{
					m_Targ_Prv_UID.InitUID();
					return addTargetChars(CLIMODE_TARG_ADDCHAR, (CREID_TYPE)m_tmAdd.m_id, false);
				}
				else
				{
					return addTargetItems(CLIMODE_TARG_ADDITEM, (ITEMID_TYPE)m_tmAdd.m_id);
				}
			}
			else
			{
				if ( IsValidResourceDef( "D_ADD" ) )
					Dialog_Setup( CLIMODE_DIALOG, g_Cfg.ResourceGetIDType(RES_DIALOG, "D_ADD"), 0, this->GetChar() );
				else
					Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, "MENU_ADDITEM"));
			}
			break;
		case CV_ADDBUFF:
			{
				tchar * ppArgs[11];
				Str_ParseCmds( s.GetArgStr(), ppArgs, CountOf(ppArgs));

				int iArgs[4];
				for ( int idx = 0; idx < 4; ++idx )
				{
					if ( !IsStrNumeric(ppArgs[idx]))
					{
						DEBUG_ERR(("Invalid AddBuff argument number %u\n", idx+1));
						return true;
					}
					iArgs[idx] = Exp_GetVal( ppArgs[idx] );
				}
				if (iArgs[0] < BI_START || iArgs[0] > BI_QTY/* || iArgs[0] == 0x3EB || iArgs[0] == 0x3EC*/)
				{	// 0x3eb and 0x3ec among some others does not exists now, which doesn't mean they won't fill them and, since nothing happens when wrong id is sent, we can let them be sent.
					DEBUG_ERR(("Invalid AddBuff icon ID\n"));
					break;
				}

				lpctstr Args[7];
				uint ArgsCount = 0;
				for ( int i = 0; i < 7; ++i )
				{
					Args[i] = ppArgs[i + 4];
					if ( Args[i] != nullptr )
						++ArgsCount;
				}

				addBuff((BUFF_ICONS)iArgs[0], iArgs[1], iArgs[2], (word)(iArgs[3]), Args, ArgsCount);
			}
			break;
		case CV_REMOVEBUFF:
			{
				BUFF_ICONS IconId = (BUFF_ICONS)s.GetArgVal();
				if (IconId < BI_START || IconId > BI_QTY/* || IconId == 0x3EB || IconId == 0x3EC*/)
				{
					DEBUG_ERR(("Invalid RemoveBuff icon ID\n"));
					break;
				}
				removeBuff(IconId);
			}
			break;
		case CV_ADDCONTEXTENTRY:
			{
				tchar * ppLocArgs[20];
				if ( Str_ParseCmds(s.GetArgRaw(), ppLocArgs, CountOf(ppLocArgs), ",") > 4 )
				{
					DEBUG_ERR(("Bad AddContextEntry usage: Function takes maximum of 4 arguments!\n"));
					return true;
				}

				if (m_pPopupPacket == nullptr)
				{
					DEBUG_ERR(("Bad AddContextEntry usage: Not used under a @ContextMenuRequest/@itemContextMenuRequest trigger!\n"));
					return true;
				}

				for ( int i = 0; i < 4; i++ )
				{
					if ( i > 1 && IsStrEmpty(ppLocArgs[i]) )
						continue;

					if ( !IsStrNumeric(ppLocArgs[i]) )
					{
						DEBUG_ERR(("Bad AddContextEntry usage: Argument %d must be a number!\n", i+1));
						return true;
					}
				}

				int entrytag = Exp_GetVal(ppLocArgs[0]);
				if ( entrytag < 100 )
				{
					DEBUG_ERR(("Bad AddContextEntry usage: TextEntry < 100 is reserved for server usage!\n"));
					return true;
				}
				m_pPopupPacket->addOption((word)entrytag, Exp_GetDWVal(ppLocArgs[1]), Exp_GetWVal(ppLocArgs[2]), Exp_GetWVal(ppLocArgs[3]) );
			}
			break;
		case CV_ARROWQUEST:
			{
				int64 piVal[3];
				Str_ParseCmds( s.GetArgRaw(), piVal, CountOf(piVal));
				addArrowQuest( (int)piVal[0], (int)piVal[1], (int)piVal[2] );
#ifdef _ALPHASPHERE
				// todo: should use a proper container for these, since the arrows are lost
				// when the client logs out, and also newer clients support multiple
				// arrows
				if ( piVal[0] && piVal[1] && m_pChar )
				{
					m_pChar->SetKeyNum("ARROWQUEST_X", piVal[0]);
					m_pChar->SetKeyNum("ARROWQUEST_Y", piVal[1]);
				} else {
					m_pChar->DeleteKey("ARROWQUEST_X");
					m_pChar->DeleteKey("ARROWQUEST_Y");
				}
#endif
			}
			break;
		case CV_BADSPAWN:
			{
				//	Loop the world searching for bad spawns
                int iPos = s.GetArgVal();
                CCSpawn *pSpawn = CCSpawn::GetBadSpawn(iPos ? iPos : -1);
                if (pSpawn != nullptr)
                {
                    CItem* pSpawnItem = pSpawn->GetLink();
                    const CResourceID& rid = pSpawn->GetSpawnID();
                    const CPointMap& pt = pSpawnItem->GetTopPoint();
                    CUID spawnUID = pSpawnItem->GetUID();
                    m_pChar->Spell_Teleport(pt, true, false);
                    m_pChar->Update();
                    m_pChar->m_Act_UID = spawnUID;
                    SysMessagef("Bad spawn (0%x, id=%s). Set as ACT", (dword)spawnUID, g_Cfg.ResourceGetName(rid));
                }
                else
                {
                    SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NO_BAD_SPAWNS));
                }
			}
			break;
		case CV_BANKSELF: // open my own bank
			addBankOpen( m_pChar, static_cast<LAYER_TYPE>(s.GetArgVal()));
			break;
		case CV_CAST:
			{
				SPELL_TYPE spell = static_cast<SPELL_TYPE>(g_Cfg.ResourceGetIndexType(RES_SPELL, s.GetArgStr()));
				const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
				if (pSpellDef == nullptr)
					return true;

				CObjBase * pObjSrc = dynamic_cast<CObjBase *>(pSrc);

				if ( IsSetMagicFlags( MAGICF_PRECAST ) && !pSpellDef->IsSpellType( SPELLFLAG_NOPRECAST ) )
				{
					int skill;
					if (!pSpellDef->GetPrimarySkill(&skill, nullptr))
						return true;

					m_tmSkillMagery.m_iSpell = spell;	// m_atMagery.m_iSpell
					m_pChar->m_atMagery.m_iSpell = spell;
					if (pObjSrc != nullptr)
					{
						m_Targ_UID = pObjSrc->GetUID();	// default target.
						m_Targ_Prv_UID = pObjSrc->GetUID();
					}
					else
					{
						m_Targ_UID.InitUID();
						m_Targ_Prv_UID.InitUID();
					}
					m_pChar->Skill_Start((SKILL_TYPE)skill);
					break;
				}
				else
					Cmd_Skill_Magery(spell, pObjSrc);
			}
			break;

		case CV_CHANGEFACE:		// open 'face selection' dialog (enhanced clients only)
		{
			addGumpDialog(CLIMODE_DIALOG, nullptr, 0, nullptr, 0, 50, 50, m_pChar, CLIMODE_DIALOG_FACESELECTION);
			break;
		}

		case CV_CHARLIST:
		{
			if ( !PacketChangeCharacter::CanSendTo(GetNetState()) )
			{
				addSysMessage("The client in use doesn't support the CHARLIST command!");
				break;
			}

			// usually just a gm command
			new PacketChangeCharacter(this);

			CharDisconnect();	// since there is no undoing this in the client.
			SetTargMode( CLIMODE_SETUP_CHARLIST );
		}
		break;

		case CV_CTAGLIST:
			if ( !strnicmp( s.GetArgStr(), "log", 3 ) )
				pSrc = &g_Serv;
			m_TagDefs.DumpKeys(pSrc, "CTAG.");
			break;

		case CV_CLEARCTAGS:
			{
				if ( s.HasArgs() )
				{
					lpctstr pszArgs = s.GetArgStr();
					SKIP_SEPARATORS(pszArgs);
					m_TagDefs.ClearKeys(pszArgs);
				}
				else
				{
					m_TagDefs.ClearKeys();
				}
			} break;

		case CV_CLOSEPAPERDOLL:
			{
                const CChar *pChar = m_pChar;
				if ( s.HasArgs() )
				{
                    const CUID uid(s.GetArgDWVal());
					pChar = uid.CharFind();
				}
				if ( pChar )
					closeUIWindow(pChar, PacketCloseUIWindow::Paperdoll);
			}
			break;

		case CV_CLOSEPROFILE:
			{
				const CChar *pChar = m_pChar;
				if ( s.HasArgs() )
				{
                    const CUID uid(s.GetArgDWVal());
					pChar = uid.CharFind();
				}
				if ( pChar )
					closeUIWindow(pChar, PacketCloseUIWindow::Profile);
			}
			break;

		case CV_CLOSESTATUS:
			{
                const CChar *pChar = m_pChar;
				if ( s.HasArgs() )
				{
                    const CUID uid(s.GetArgDWVal());
					pChar = uid.CharFind();
				}
				if ( pChar )
					closeUIWindow(pChar, PacketCloseUIWindow::Status);
			}
			break;

        case CV_CODEXOFWISDOM:
        {
            int64 piArgs[2];
            size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piArgs, CountOf(piArgs));
            if ( iArgQty < 1 )
            {
                SysMessage("Usage: CODEXOFWISDOM TopicID [ForceOpen]");
                break;
            }

            addCodexOfWisdom((dword)(piArgs[0]), (bool)(piArgs[1]));
            break;
        }

		case CV_DYE:
			if ( s.HasArgs() )
			{
				CUID uid(s.GetArgVal());
				CObjBase *pObj = uid.ObjFind();
				if ( pObj )
					addDyeOption(pObj);
			}
			break;

		case CV_EVERBTARG:
			m_Prompt_Text = s.GetArgStr();
			addPromptConsole( CLIMODE_PROMPT_TARG_VERB, m_Targ_Text.IsEmpty() ? "Enter the verb" : "Enter the text",  m_Targ_UID );
			break;

		case CV_EXTRACT:
			// sort of like EXPORT but for statics.
			// Opposite of the "UNEXTRACT" command

			if ( ! s.HasArgs())
			{
				SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_EXTRACT_USAGE ) );
			}
			else
			{
				tchar * ppArgs[2];
				Str_ParseCmds( s.GetArgStr(), ppArgs, CountOf( ppArgs ));

				m_Targ_Text = ppArgs[0]; // Point at the options, if any
				m_tmTile.m_ptFirst.InitPoint(); // Clear this first
				m_tmTile.m_Code = CV_EXTRACT;	// set extract code.
				m_tmTile.m_id = Exp_GetVal(ppArgs[1]);	// extract id.
				addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_EXTRACT_AREA ), true );
			}
			break;

		case CV_UNEXTRACT:
			// Create item from script.
			// Opposite of the "EXTRACT" command
			if ( ! s.HasArgs())
			{
				SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_UNEXTRACT_USAGE ) );
			}
			else
			{
				tchar * ppArgs[2];
				Str_ParseCmds( s.GetArgStr(), ppArgs, CountOf( ppArgs ));

				m_Targ_Text = ppArgs[0]; // Point at the options, if any
				m_tmTile.m_ptFirst.InitPoint(); // Clear this first
				m_tmTile.m_Code = CV_UNEXTRACT;	// set extract code.
				m_tmTile.m_id = Exp_GetVal(ppArgs[1]);	// extract id.

				addTarget( CLIMODE_TARG_UNEXTRACT, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_MULTI_POS ), true );
			}
			break;

		case CV_GMPAGE:
			m_Targ_Text = s.GetArgStr();
			if ( !m_Targ_Text.IsEmpty() && !strnicmp( m_Targ_Text, "ADD ", 4 ) )
			{
				Event_PromptResp_GMPage(m_Targ_Text + 4);
				break;
			}
			addPromptConsole( CLIMODE_PROMPT_GM_PAGE_TEXT, g_Cfg.GetDefaultMsg( DEFMSG_GMPAGE_PROMPT ) );
			break;
		case CV_GOTARG: // go to my (preselected) target.
			{
				ASSERT(m_pChar);
				CObjBase * pObj = m_Targ_UID.ObjFind();
				if ( pObj != nullptr )
				{
					const CPointMap po(pObj->GetTopLevelObj()->GetTopPoint());
					CPointMap pnt = po;
					pnt.MoveN( DIR_W, 3 );
					dword dwBlockFlags = m_pChar->GetCanMoveFlags(m_pChar->GetCanFlags());
					pnt.m_z = CWorldMap::GetHeightPoint2( pnt, dwBlockFlags );	// ??? Get Area
					m_pChar->m_dirFace = pnt.GetDir( po, m_pChar->m_dirFace ); // Face the player
					m_pChar->Spell_Teleport( pnt, true, false );
				}
			}
			break;
		case CV_INFO:
			// We could also get ground tile info.
			addTarget( CLIMODE_TARG_OBJ_INFO, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_ITEM_INFO ), true, false );
			break;
		case CV_INFORMATION:
			SysMessage( g_Serv.GetStatusString( 0x22 ));
			SysMessage( g_Serv.GetStatusString( 0x24 ));
			break;
		case CV_LAST:
			// Fake Previous target.
			if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
			{
				ASSERT(m_pChar);
				CObjBase * pObj = m_pChar->m_Act_UID.ObjFind();
				if ( pObj != nullptr )
				{
					Event_Target(GetTargMode(), pObj->GetUID(), pObj->GetUnkPoint());
					addTargetCancel();
				}
				break;
			}
			return false;
		case CV_LINK:	// link doors
			m_Targ_UID.InitUID();
			addTarget( CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_LINK_ITEM ) );
			break;

        case CV_MAPWAYPOINT:
        {
            int64 piVal[2];
            Str_ParseCmds(s.GetArgRaw(), piVal, CountOf(piVal));

            CObjBase *pObj = static_cast<CUID>((dword)piVal[0]).ObjFind();
            if (pObj)
            {
                addMapWaypoint(pObj, (MAPWAYPOINT_TYPE)((dword)piVal[1]));
            }
            break;
        }

		case CV_MENU:
			Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, s.GetArgStr()));
			break;
		case CV_MIDILIST:
			{
				int64 piMidi[64];
				int64 iQty = Str_ParseCmds( s.GetArgStr(), piMidi, CountOf(piMidi) );
				if ( iQty > 0 )
				{
					addMusic( static_cast<MIDI_TYPE>(piMidi[ Calc_GetRandLLVal( iQty ) ]) );
				}
			}
			break;
		case CV_NUDGE:
			if ( ! s.HasArgs())
			{
				SysMessage( "Usage: NUDGE dx dy dz" );
			}
			else
			{
				m_Targ_Text = s.GetArgRaw();
				m_tmTile.m_ptFirst.InitPoint(); // Clear this first
				m_tmTile.m_Code = CV_NUDGE;
				addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_NUDGE_AREA ), true );
			}
			break;

		case CV_NUKE:
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_NUKE;	// set nuke code.
			addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_NUKE_AREA ), true );
			break;
		case CV_NUKECHAR:
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_NUKECHAR;	// set nuke code.
			addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_NUKE_CHAR_AREA ), true );
			break;

		case CV_OPENPAPERDOLL:
		{
			CChar *pChar = m_pChar;
			if ( s.HasArgs() )
				pChar = CUID::CharFindFromUID(s.GetArgVal());
			
			if ( pChar )
				addCharPaperdoll(pChar);
			break;
		}

		case CV_OPENTRADEWINDOW:
		{
			tchar *ppArgs[2];
			Str_ParseCmds(s.GetArgStr(), ppArgs, CountOf(ppArgs));

			CChar *pChar = nullptr;
			CItem *pItem = nullptr;
			if ( ppArgs[0] )
			{
				pChar = CUID::CharFindFromUID(Exp_GetDWVal(ppArgs[0]));
			}
			if ( ppArgs[1] )
			{
				pItem = CUID::ItemFindFromUID(Exp_GetDWVal(ppArgs[1]));
			}
			if (pChar)
			{
				Cmd_SecureTrade(pChar, pItem);
			}
			break;
		}

		case CV_REPAIR:
			addTarget( CLIMODE_TARG_REPAIR, g_Cfg.GetDefaultMsg( DEFMSG_SELECT_ITEM_REPAIR ) );
			break;
		case CV_FLUSH:
			g_NetworkManager.flush(GetNetState());
			break;
		case CV_RESEND:
			addReSync();
			break;
		case CV_SAVE:
			g_World.Save(s.GetArgVal() != 0);
			break;
		case CV_SCROLL:
			// put a scroll up.
			addScrollResource( s.GetArgStr(), SCROLL_TYPE_UPDATES );
			break;
		case CV_SENDPACKET:
			SendPacket( s.GetArgStr() );
			break;
		case CV_SELF:
			// Fake self target.
			if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
			{
				ASSERT(m_pChar);
				Event_Target(GetTargMode(), m_pChar->GetUID(), m_pChar->GetTopPoint());
				addTargetCancel();
				break;
			}
			return false;
		case CV_SHOWSKILLS:
			addSkillWindow((SKILL_TYPE)(g_Cfg.m_iMaxSkill)); // Reload the real skills
			break;
		case CV_SKILLMENU:				// Just put up another menu.
			Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, s.GetArgStr()));
			break;
		case CV_SKILLSELECT:
			Event_Skill_Use( g_Cfg.FindSkillKey( s.GetArgStr() ) );
			break;
		case CV_SUMMON:
		{
			ASSERT(m_pChar);
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Summon);
			if ( !pSpellDef )
				return false;

			m_pChar->m_Act_UID = m_pChar->GetUID();
			m_pChar->m_Act_Prv_UID = m_pChar->GetUID();

			if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
			{
				m_tmSkillMagery.m_iSpell = SPELL_Summon;
				m_tmSkillMagery.m_iSummonID = (CREID_TYPE)(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr()));

				lpctstr pPrompt = g_Cfg.GetDefaultMsg(DEFMSG_SELECT_MAGIC_TARGET);
				if ( !pSpellDef->m_sTargetPrompt.IsEmpty() )
					pPrompt = pSpellDef->m_sTargetPrompt;

				int64 SpellTimeout = g_Cfg.m_iSpellTimeout;
				if ( GetDefNum("SPELLTIMEOUT") )
					SpellTimeout = GetDefNum("SPELLTIMEOUT") * MSECS_PER_SEC; //tenths to msecs.

				addTarget(CLIMODE_TARG_SKILL_MAGERY, pPrompt, pSpellDef->IsSpellType(SPELLFLAG_TARG_XYZ), pSpellDef->IsSpellType(SPELLFLAG_HARM), SpellTimeout);
				break;
			}
			else
			{
				m_pChar->m_atMagery.m_iSpell = SPELL_Summon;
				m_pChar->m_atMagery.m_iSummonID = (CREID_TYPE)(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr()));

				if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
				{
					m_pChar->Spell_CastDone();
					break;
				}
				else
				{
					int skill;
					if ( !pSpellDef->GetPrimarySkill(&skill, nullptr) )
						return false;

					m_pChar->Skill_Start((SKILL_TYPE)skill);
				}
			}
			break;
		}
		case CV_SMSG:
		case CV_SYSMESSAGE:
			SysMessage( s.GetArgStr() );
			break;
		case CV_SYSMESSAGEF: //There is still an issue with numbers not resolving properly when %i,%d,or other numeric format code is in use
			{
				tchar * pszArgs[4];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, CountOf(pszArgs) );
				if ( iArgQty < 2 )
				{
					g_Log.EventError("SysMessagef with less than 1 args for the given text\n");
					return false;
				}
				if ( iArgQty > 4 )
				{
					g_Log.EventError("Too many arguments given to SysMessagef (max = text + 3\n");
					return false;
				}
				//strip quotes if any
				if ( *pszArgs[0] == '"' )
					++pszArgs[0];
				for (tchar * pEnd = pszArgs[0] + strlen( pszArgs[0] ) - 1; pEnd >= pszArgs[0]; --pEnd )
				{
					if ( *pEnd == '"' )
					{
						*pEnd = '\0';
						break;
					}
				}
				SysMessagef( pszArgs[0], pszArgs[1], pszArgs[2] ? pszArgs[2] : 0, pszArgs[3] ? pszArgs[3] : 0);
			}
			break;
		case CV_SMSGU:
		case CV_SYSMESSAGEUA:
			{
				tchar * pszArgs[5];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, CountOf(pszArgs) );
                if (iArgQty < 5)
                    return false;
				// Font and mode are actually ignored here, but they never made a difference
				// anyway.. I'd like to keep the syntax similar to SAYUA
                nchar szBuffer[MAX_TALK_BUFFER];
                CvtSystemToNUNICODE(szBuffer, CountOf(szBuffer), pszArgs[4], -1);
                addBarkUNICODE(szBuffer, nullptr, (HUE_TYPE)(Exp_GetVal(pszArgs[0])), TALKMODE_SAY, FONT_NORMAL, pszArgs[3]);
			}
			break;
		case CV_SMSGL:
		case CV_SYSMESSAGELOC:
			{
				tchar * ppArgs[256];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), ppArgs, CountOf(ppArgs), "," );
				if ( iArgQty > 1 )
				{
					int hue = -1;
					if ( atoi(ppArgs[0]) > 0 )
						hue = Exp_GetVal( ppArgs[0] );
					int iClilocId = Exp_GetVal( ppArgs[1] );

					if ( hue == -1 )
						hue = HUE_TEXT_DEF;

					CSString CArgs;
					for (int i = 2; i < iArgQty; ++i )
					{
						if ( CArgs.GetLength() )
							CArgs += "\t";
						CArgs += ( !strncmp(ppArgs[i], "NULL", 4) ? " " : ppArgs[i] );
					}

					addBarkLocalized(iClilocId, nullptr, (HUE_TYPE)(hue), TALKMODE_SAY, FONT_NORMAL, CArgs.GetBuffer());
				}
			}
			break;
		case CV_SMSGLEX:
		case CV_SYSMESSAGELOCEX:
			{
				tchar * ppArgs[256];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), ppArgs, CountOf(ppArgs), "," );
				if ( iArgQty > 2 )
				{
					int hue = -1;
					int affix = 0;
					if ( atoi(ppArgs[0]) > 0 )
						hue = Exp_GetVal( ppArgs[0] );
					int iClilocId = Exp_GetVal( ppArgs[1] );
					if ( ppArgs[2] )
						affix = Exp_GetVal( ppArgs[2] );

					if ( hue == -1 )
                        hue = HUE_TEXT_DEF;

					CSString CArgs;
					for (int i = 4; i < iArgQty; ++i )
					{
						if ( CArgs.GetLength() )
							CArgs += "\t";
						CArgs += ( !strncmp(ppArgs[i], "NULL", 4) ? " " : ppArgs[i] );
					}

					addBarkLocalizedEx( iClilocId, nullptr, (HUE_TYPE)(hue), TALKMODE_SAY, FONT_NORMAL, (AFFIX_TYPE)(affix), ppArgs[3], CArgs.GetBuffer() );
				}
			}
			break;
		case CV_TELE:
			Cmd_Skill_Magery(SPELL_Teleport, dynamic_cast<CObjBase *>(pSrc));
			break;
		case CV_TILE:
			if ( ! s.HasArgs() )
			{
				SysMessage( "Usage: TILE z-height item1 item2 itemX" );
			}
			else
			{
				m_Targ_Text = s.GetArgStr(); // Point at the options
				m_tmTile.m_ptFirst.InitPoint(); // Clear this first
				m_tmTile.m_Code = CV_TILE;
				addTarget( CLIMODE_TARG_TILE, "Pick 1st corner:", true );
			}
			break;
		case CV_VERSION:	// "SHOW VERSION"
			SysMessage(g_sServerDescription.c_str());
			break;
		case CV_WEBLINK:
			addWebLaunch( s.GetArgStr() );
			break;
		default:
			if ( r_LoadVal( s ) )
			{
				CSString sVal;
				if ( r_WriteVal( s.GetKey(), sVal, pSrc ))
				{
					// if ( !s.IsKeyHead( "CTAG.", 5 ) && !s.IsKeyHead( "CTAG0.", 6 ) ) // We don't want output related to ctag
					//	SysMessagef( "%s = %s", (lpctstr) s.GetKey(), (lpctstr) sVal );	// feedback on what we just did.

					return true;
				}
			}

			if (GetChar())
				return false;	// In this case, we were called by CChar::r_Verb, which calls also the other r_Verb virtual (or not) methods.

			return CScriptObj::r_Verb( s, pSrc );	// used in the case of web pages to access server level things..
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

int CClient::GetSocketID() const
{
	return m_net->id();
}

CSocketAddress &CClient::GetPeer()
{
	return m_net->m_peerAddress;
}

lpctstr CClient::GetPeerStr() const
{
	return m_net->m_peerAddress.GetAddrStr();
}
