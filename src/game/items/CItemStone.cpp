
#include "../../common/CException.h"
#include "../../common/CLog.h"
#include "../../common/CScriptTriggerArgs.h"
#include "../chars/CChar.h"
#include "../CServer.h"
#include "../CWorld.h"
#include "CItemStone.h"
#include "CItemMulti.h"
#include "CItemShip.h"


CItemStone::CItemStone( ITEMID_TYPE id, CItemBase * pItemDef ) :
    CTimedObject(PROFILE_ITEMS),
    CItem( id, pItemDef )
{
	EXC_TRY("Constructor");

	m_itStone.m_iAlign = STONEALIGN_STANDARD;
    g_World.m_Stones.emplace_back(this);
    _pMultiStorage = new CMultiStorage(CUID());
    _iMaxShips = g_Cfg._iMaxShipsGuild;
    _iMaxHouses = g_Cfg._iMaxHousesGuild;

	EXC_CATCH;
}

CItemStone::~CItemStone()
{
	EXC_TRY("Cleanup in destructor");

	SetAmount(0);	// Tell everyone we are deleting.
	DeletePrepare();	// Must remove early because virtuals will fail in child destructor.

    delete _pMultiStorage;
	// all members are deleted automatically.
	ClearContainer();	// do this manually to preserve the parents type cast

	EXC_CATCH;
}

MEMORY_TYPE CItemStone::GetMemoryType() const
{
	ADDTOCALLSTACK("CItemStone::GetMemoryType");
	switch ( GetType() )
	{
		case IT_STONE_TOWN:
			return MEMORY_TOWN;
		case IT_STONE_GUILD:
			return MEMORY_GUILD;
		default:
			return MEMORY_NONE; // Houses have no memories.
	}
}

lpctstr CItemStone::GetCharter(uint iLine) const
{
	ASSERT(iLine<ARRAY_COUNT(m_sCharter));
	return m_sCharter[iLine];
}

void CItemStone::SetCharter( uint iLine, lpctstr pCharter )
{
	ASSERT(iLine<ARRAY_COUNT(m_sCharter));
	m_sCharter[iLine] = pCharter;
}

lpctstr CItemStone::GetWebPageURL() const
{
	return m_sWebPageURL;
}

void CItemStone::SetWebPageURL( lpctstr pWebPageURL )
{
	m_sWebPageURL = pWebPageURL;
}

STONEALIGN_TYPE CItemStone::GetAlignType() const
{
	return m_itStone.m_iAlign;
}

void CItemStone::SetALIGNTYPE(STONEALIGN_TYPE iAlign)
{
	m_itStone.m_iAlign = iAlign;
}

lpctstr CItemStone::GetAbbrev() const
{
	return m_sAbbrev;
}

void CItemStone::SetAbbrev( lpctstr pAbbrev )
{
	m_sAbbrev = pAbbrev;
}

lpctstr CItemStone::GetTypeName() const
{
	ADDTOCALLSTACK("CItemStone::GetTypeName");
	CVarDefCont * pResult = nullptr;
	switch ( GetType() )
	{
		case IT_STONE_GUILD:
			pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_TYPENAME_GUILD");
			break;
		case IT_STONE_TOWN:
			pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_TYPENAME_TOWN");
			break;
		default:
			break;
	}

	if ( pResult == nullptr )
		pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_TYPENAME_UNK");

	return ( pResult == nullptr ) ? "" : pResult->GetValStr();
}

void CItemStone::r_Write( CScript & s )
{
	ADDTOCALLSTACK_DEBUG("CItemStone::r_Write");
	CItem::r_Write( s );
	s.WriteKeyVal( "ALIGN", GetAlignType());
	if ( ! m_sAbbrev.IsEmpty())
		s.WriteKeyStr( "ABBREV", m_sAbbrev.GetBuffer() );

	TemporaryString tsTemp;
	for ( uint i = 0; i < ARRAY_COUNT(m_sCharter); ++i )
	{
		if ( ! m_sCharter[i].IsEmpty())
		{
			snprintf(tsTemp.buffer(), tsTemp.capacity(), "CHARTER%u", i);
			s.WriteKeyStr(tsTemp.buffer(), m_sCharter[i].GetBuffer() );
		}
	}

	if ( ! m_sWebPageURL.IsEmpty())
		s.WriteKeyStr( "WEBPAGE", GetWebPageURL() );

	// s.WriteKeyVal( "//", "uid,title,priv,loyaluid,abbr&theydecl,wedecl");

	CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());
	for ( ; pMember != nullptr; pMember = pMember->GetNext())
	{
		if (pMember->GetLinkUID().IsValidUID()) // To protect against characters that were deleted!
		{
			s.WriteKeyFormat( "MEMBER",
				"0%x,%s,%i,0%x,%i,%i,%i",
				(dword) pMember->GetLinkUID() | (pMember->GetLinkUID().IsItem() ? UID_F_ITEM : 0),
				pMember->GetTitle(),
				pMember->GetPriv(),
				(dword)(pMember->GetLoyalToUID()),
				pMember->m_UnDef.m_Val1,
				pMember->m_UnDef.m_Val2,
				pMember->GetAccountGold());
		}
	}
}

lpctstr CItemStone::GetAlignName() const
{
	ADDTOCALLSTACK("CItemStone::GetAlignName");
	int iAlign = GetAlignType();

	TemporaryString tsDefname;
	if ( GetType() == IT_STONE_GUILD )
		snprintf(tsDefname.buffer(), tsDefname.capacity(), "GUILDCONFIG_ALIGN_%d", iAlign);
	else if ( GetType() == IT_STONE_TOWN )
		snprintf(tsDefname.buffer(), tsDefname.capacity(), "TOWNSCONFIG_ALIGN_%d", iAlign);
	else
		return "";

	lpctstr sRes = g_Exp.m_VarDefs.GetKeyStr(tsDefname);
	return ( sRes == nullptr ) ? "" : sRes;
}

enum STC_TYPE
{
#define ADD(a,b) STC_##a,
#include "../../tables/CItemStone_props.tbl"
#undef ADD
	STC_QTY
};

lpctstr const CItemStone::sm_szLoadKeys[STC_QTY+1] =
{
#define ADD(a,b) b,
#include "../../tables/CItemStone_props.tbl"
#undef ADD
	nullptr
};

enum ISV_TYPE
{
#define ADD(a,b) ISV_##a,
#include "../../tables/CItemStone_functions.tbl"
#undef ADD
	ISV_QTY
};

lpctstr const CItemStone::sm_szVerbKeys[ISV_QTY+1] =
{
#define ADD(a,b) b,
#include "../../tables/CItemStone_functions.tbl"
#undef ADD
	nullptr
};

bool CItemStone::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CItemStone::r_GetRef");
	if ( !strnicmp("member.", ptcKey, 7) )
	{
		ptcKey = ptcKey + 7;
		if ( !ptcKey[0] )
			return false;

		int nNumber = Exp_GetVal(ptcKey);
		SKIP_SEPARATORS(ptcKey);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());

		for ( int i = 0; pMember != nullptr; pMember = pMember->GetNext() )
		{
			if ( !pMember->GetLinkUID().IsChar() ) 
				continue;

			if ( nNumber == i )
			{
				pRef = pMember; 
				return true;
			}

			++i;
		}
	}
	else if ( !strnicmp("memberfromuid.", ptcKey, 14) )
	{
		ptcKey = ptcKey + 14;
		if ( !ptcKey[0] )
			return false;

		CUID pMemberUid(Exp_GetDWVal(ptcKey));
		SKIP_SEPARATORS(ptcKey);

		CChar * pMemberChar = pMemberUid.CharFind();
		if ( pMemberChar )
		{
			CStoneMember * pMemberGuild = GetMember( pMemberChar );
			if ( pMemberGuild )
			{
				pRef = pMemberGuild; 
				return true;
			}
		}
	}
	else if ( !strnicmp("guild.", ptcKey, 6) )
	{
		ptcKey = ptcKey + 6;
		if ( !ptcKey[0] )
			return false;

		int nNumber = Exp_GetVal(ptcKey);
		SKIP_SEPARATORS(ptcKey);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());

		for ( int i = 0; pMember != nullptr; pMember = pMember->GetNext() )
		{
			if ( pMember->GetLinkUID().IsChar() ) 
				continue;

			if ( nNumber == i )
			{
				pRef = pMember;
				return true;
			}

			i++;
		}
	}
	else if ( !strnicmp("guildfromuid.", ptcKey, 13) )
	{
		ptcKey = ptcKey + 13;
		if ( !ptcKey[0] )
			return false;

		CUID pGuildUid(Exp_GetDWVal(ptcKey));
		SKIP_SEPARATORS(ptcKey);

		CItem * pMemberGuild = pGuildUid.ItemFind();
		if ( pMemberGuild )
		{
			CStoneMember * pGuild = GetMember( pMemberGuild );
			if ( pGuild )
			{
				pRef = pGuild; 
				return true;
			}
		}
	}
    else if (!strnicmp("house.", ptcKey, 6))
    {
        ptcKey = ptcKey + 6;
        if (!ptcKey[0])
            return false;

        int16 nNumber = (int16)Exp_GetVal(ptcKey);
        SKIP_SEPARATORS(ptcKey);

        if (nNumber < GetMultiStorage()->GetHouseCountReal())
        {
            pRef = static_cast<CItemMulti*>(GetMultiStorage()->GetHouseAt(nNumber).ItemFind());
            return true;
        }
    }
    else if (!strnicmp("ship.", ptcKey, 5))
    {
        ptcKey = ptcKey + 5;
        if (!ptcKey[0])
            return false;

        int16 nNumber = (int16)Exp_GetVal(ptcKey);
        SKIP_SEPARATORS(ptcKey);

        if (nNumber < GetMultiStorage()->GetShipCountReal())
        {
            pRef = static_cast<CItemShip*>(GetMultiStorage()->GetShipAt(nNumber).ItemFind());
            return true;
        }
    }

	return CItem::r_GetRef( ptcKey, pRef );
}

bool CItemStone::r_LoadVal( CScript & s ) // Load an item Script
{
	ADDTOCALLSTACK("CItemStone::r_LoadVal");
	EXC_TRY("LoadVal");

	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
	{
		case STC_ABBREV: // "ABBREV"
			m_sAbbrev = s.GetArgStr();
			return true;
        case STC_ADDHOUSE:
        case STC_ADDSHIP:
        {
            GetMultiStorage()->AddMulti(((CUID)s.GetArgDWVal()), HP_GUILD);
            return true;
        }
		case STC_ALIGN: // "ALIGN"
			SetALIGNTYPE(static_cast<STONEALIGN_TYPE>(s.GetArgVal()));
			return true;
		case STC_MASTERUID:
			{
				if ( s.HasArgs() )
				{
					CUID pNewMASTERUID(s.GetArgDWVal());
					CChar * pChar = pNewMASTERUID.CharFind();
					if ( !pChar )
					{
						DEBUG_ERR(( "MASTERUID called on non char 0%x uid.\n", (dword)pNewMASTERUID ));
						return false;
					}

					CStoneMember * pNewMaster = GetMember( pChar );
					if ( !pNewMaster )
					{
						DEBUG_ERR(( "MASTERUID called on char 0%x (%s) that is not a valid member of stone with 0x%x uid.\n", (dword)pNewMASTERUID, pChar->GetName(), (dword)GetUID() ));
						return false;
					}

					CStoneMember * pMaster = GetMasterMember();
					if ( pMaster )
					{
						if ( pMaster->GetLinkUID() == pNewMASTERUID )
							return true;

						pMaster->SetPriv(STONEPRIV_MEMBER);
						//pMaster->SetLoyalTo(pChar);
					}

					//pNewMaster->SetLoyalTo(pChar);
					pNewMaster->SetPriv(STONEPRIV_MASTER);
				}
				else
				{
					DEBUG_ERR(( "MASTERUID called without arguments.\n" ));
					return false;
				}
			}
			return true;
		case STC_MEMBER: // "MEMBER"
			{
			tchar *Arg_ppCmd[8];		// Maximum parameters in one line
			size_t Arg_Qty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, ARRAY_COUNT( Arg_ppCmd ), "," );
			if (Arg_Qty < 1) // must at least provide the member uid
				return false;

			new CStoneMember(
				this,
				CUID(ahextoi(Arg_ppCmd[0])), 											// Member's UID
				Arg_Qty > 2 ? (STONEPRIV_TYPE)(atoi(Arg_ppCmd[2])) : STONEPRIV_CANDIDATE,// Members priv level (use as a type)
				Arg_Qty > 1 ? Arg_ppCmd[1] : "",										// Title
				CUID(ahextoi(Arg_ppCmd[3])),											// Member is loyal to this
				Arg_Qty > 4 ? (atoi( Arg_ppCmd[4] ) != 0) : 0,							// Paperdoll stone abbreviation (also if they declared war)
				Arg_Qty > 5 ? (atoi( Arg_ppCmd[5] ) != 0) : 0,							// If we declared war
				Arg_Qty > 6 ? atoi( Arg_ppCmd[6] ) : 0);								// AccountGold
			}
			return true;
		case STC_WEBPAGE: // "WEBPAGE"
			m_sWebPageURL = s.GetArgStr();
			return true;
        case STC_MAXHOUSES:
            _iMaxHouses = s.GetArg16Val();
            return true;
        case STC_MAXSHIPS:
            _iMaxShips = s.GetArg16Val();
            return true;
	}

	if ( s.IsKeyHead( sm_szLoadKeys[STC_CHARTER], 7 ))
	{
		uint i = atoi(s.GetKey() + 7);
		if ( i >= ARRAY_COUNT(m_sCharter))
			return false;
		m_sCharter[i] = s.GetArgStr();
		return true;
	}
	
	return CItem::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CItemStone::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallChildren);
	ADDTOCALLSTACK("CItemStone::r_WriteVal");
	EXC_TRY("WriteVal");
	CChar * pCharSrc = pSrc->GetChar();

	if ( !strnicmp("member.",ptcKey,7) )
	{
		lpctstr pszCmd = ptcKey + 7;

		if ( !strnicmp("COUNT",pszCmd,5) )
		{
			pszCmd = pszCmd + 5;

			int i = 0;
			CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());

			if ( *pszCmd )
			{
				SKIP_ARGSEP(pszCmd);
				STONEPRIV_TYPE iPriv = STONEPRIV_TYPE(Exp_GetVal(pszCmd));

				for (; pMember != nullptr; pMember = pMember->GetNext())
				{
					if ( !pMember->GetLinkUID().IsChar() )
						continue;

					if ( pMember->GetPriv() != iPriv )
						continue;

					++i;
				}
			}
			else
			{
				for (; pMember != nullptr; pMember = pMember->GetNext())
				{
					if (!pMember->GetLinkUID().IsChar()) 
						continue;

					++i;
				}
			}

			sVal.FormatVal(i);
			return true;
		}
		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());
		sVal.FormatVal(0);

		for ( int i = 0 ; pMember != nullptr; pMember = pMember->GetNext() )
		{
			if (!pMember->GetLinkUID().IsChar()) 
				continue;
				
			if ( nNumber == i )
			{
				if (!pszCmd[0]) 
					return true;

				return pMember->r_WriteVal(pszCmd, sVal, pSrc);
			}

			i++;
		}

		return true;
	}
	else if ( !strnicmp("memberfromuid.", ptcKey, 14) )
	{
		lpctstr pszCmd = ptcKey + 14;
		sVal.FormatVal(0);

		if ( !pszCmd[0] )
			return true;

		CUID pMemberUid(Exp_GetDWVal(pszCmd));
		SKIP_SEPARATORS(pszCmd);

		CChar * pMemberChar = pMemberUid.CharFind();
		if ( pMemberChar )
		{
			CStoneMember * pMemberGuild = GetMember( pMemberChar );
			if ( pMemberGuild )
				return pMemberGuild->r_WriteVal(pszCmd, sVal, pSrc);
		}

		return true;
	}
	else if ( !strnicmp("guild.",ptcKey,6) )
	{
		lpctstr pszCmd = ptcKey + 6;

		if ( !strnicmp("COUNT",pszCmd,5) )
		{
			pszCmd = pszCmd + 5;

			int i = 0;
			CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());

			if ( *pszCmd )
			{
				SKIP_ARGSEP(pszCmd);
				int iToCheck = Exp_GetVal(pszCmd);

				for (; pMember != nullptr; pMember = pMember->GetNext())
				{
					if ( pMember->GetLinkUID().IsChar() )
						continue;

					if ( ( iToCheck == 1 ) && ( pMember->GetWeDeclaredWar() && !pMember->GetTheyDeclaredWar() ) )
						i++;
					else if ( ( iToCheck == 2 ) && ( !pMember->GetWeDeclaredWar() && pMember->GetTheyDeclaredWar() ) )
						i++;
					else if ( ( iToCheck == 3 ) && ( pMember->GetWeDeclaredWar() && pMember->GetTheyDeclaredWar() ) )
						i++;
				}
			}
			else
			{
				for (; pMember != nullptr; pMember = pMember->GetNext())
				{
					if (pMember->GetLinkUID().IsChar()) 
						continue;

					i++;
				}
			}

			sVal.FormatVal(i);
			return true;
		}
		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());
		sVal.FormatVal(0);

		for ( int i = 0 ; pMember != nullptr; pMember = pMember->GetNext() )
		{
			if (pMember->GetLinkUID().IsChar()) 
				continue;
				
			if ( nNumber == i )
			{
				if (!pszCmd[0]) 
					return true;

				return pMember->r_WriteVal(pszCmd, sVal, pSrc);
			}

			i++;
		}

		return true;
	}
	else if ( !strnicmp("guildfromuid.", ptcKey, 13) )
	{
		lpctstr pszCmd = ptcKey + 13;
		sVal.FormatVal(0);

		if ( !pszCmd[0] )
			return true;

		CUID pGuildUid(Exp_GetDWVal(pszCmd));
		SKIP_SEPARATORS(pszCmd);

		CItem * pMemberGuild = pGuildUid.ItemFind();
		if ( pMemberGuild )
		{
			CStoneMember * pGuild = GetMember( pMemberGuild );
			if ( pGuild )
			{
				return pGuild->r_WriteVal(pszCmd, sVal, pSrc);
			}
		}

		return true;
	}
	else if ( !strnicmp(sm_szLoadKeys[STC_CHARTER], ptcKey, 7) )
	{
		lpctstr pszCmd = ptcKey + 7;
		uint i = atoi(pszCmd);
		if ( i >= ARRAY_COUNT(m_sCharter))
			sVal.Clear();
		else
			sVal = m_sCharter[i];

		return true;
	}


	STC_TYPE iIndex = (STC_TYPE) FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );

	switch ( iIndex )
	{
		case STC_ABBREV: // "ABBREV"
			sVal = m_sAbbrev;
			return true;
		case STC_ALIGN:
			sVal.FormatVal( GetAlignType());
			return true;
		case STC_WEBPAGE: // "WEBPAGE"
			sVal = GetWebPageURL();
			return true;
        case STC_MAXHOUSES:
            sVal.Format16Val(_iMaxHouses);
            return true;
        case STC_MAXSHIPS:
            sVal.Format16Val(_iMaxShips);
            return true;
        case STC_HOUSES:
            sVal.Format16Val(GetMultiStorage()->GetHouseCountReal());
            return true;
        case STC_SHIPS:
            sVal.Format16Val(GetMultiStorage()->GetShipCountReal());
            return true;
		case STC_ABBREVIATIONTOGGLE:
			{
				CStoneMember * pMember = GetMember(pCharSrc);
				CVarDefCont * pResult = nullptr;

				if ( pMember == nullptr )
				{
					pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_NONMEMBER");
				}
				else
				{
					pResult = pMember->IsAbbrevOn() ? g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_ABBREVON") :
								g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_ABBREVOFF");
				}

				sVal = pResult ? pResult->GetValStr() : "";
			}
			return true;
		case STC_ALIGNTYPE:
			sVal = GetAlignName();
			return true;

		case STC_LOYALTO:
			{
				CStoneMember * pMember = GetMember(pCharSrc);
				CVarDefCont * pResult = nullptr;

				if ( pMember == nullptr )
				{
					pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_NONMEMBER");
				}
				else
				{
					CChar * pLOYALTO = pMember->GetLoyalToUID().CharFind();
					if ((pLOYALTO == nullptr) || (pLOYALTO == pCharSrc ))
					{
						pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_YOURSELF");
					}
					else
					{
						sVal = pLOYALTO->GetName();
						return true;
					}
				}

				sVal = pResult ? pResult->GetValStr() : "";
			}
			return true;
	
		case STC_MASTER:
			{
				CChar * pMaster = GetMaster();
				sVal = (pMaster) ? pMaster->GetName() : g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_PENDVOTE");
			}
			return true;
	
		case STC_MASTERGENDERTITLE:
			{
				CChar * pMaster = GetMaster();
				if ( pMaster == nullptr )
					sVal.Clear(); // If no master (vote pending)
				else if ( pMaster->Char_GetDef()->IsFemale())
					sVal = g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_MASTERGENDERFEMALE");
				else
					sVal = g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_MASTERGENDERMALE");
			}
			return true;
	
		case STC_MASTERTITLE:
			{
				CStoneMember * pMember = GetMasterMember();
				sVal = (pMember) ? pMember->GetTitle() : "";
			}
			return true;
	
		case STC_MASTERUID:
			{
				CChar * pMaster = GetMaster();
				if ( pMaster )
					sVal.FormatHex( (dword) pMaster->GetUID() );
				else
					sVal.FormatHex( (dword) 0 );
			}
			return true;
			
		default:
			return (fNoCallParent ? false : CItem::r_WriteVal( ptcKey, sVal, pSrc ));
	}

	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemStone::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CItemStone::r_Verb");
	EXC_TRY("Verb");
	// NOTE:: ONLY CALL this from CChar::r_Verb !!!
	// Little to no security checking here !!!

	ASSERT(pSrc);

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, ARRAY_COUNT(sm_szVerbKeys)-1 );
	if ( index < 0 )
		return CItem::r_Verb( s, pSrc );

	CChar * pCharSrc = pSrc->GetChar();
	CStoneMember * pMember = GetMember(pCharSrc);

	switch ( index )
	{
        case ISV_DELHOUSE:
        case ISV_DELSHIP:
        {
            GetMultiStorage()->DelMulti((CUID)s.GetArgDWVal());
            return true;
        }
		case ISV_ALLGUILDS:
			{
				if ( s.HasArgs() )
				{
					tchar * pszArgs = s.GetArgRaw();
					int iFlags = Exp_GetVal(pszArgs);

					if ( iFlags < 0 )
					{
						g_Log.EventError("ItemStone::AllGuilds invalid parameter '%i'.\n", iFlags);
						return false;
					}
					else
					{
						if ( pszArgs[0] != '\0' )
						{
							pMember = static_cast <CStoneMember *>(GetContainerHead());
							SKIP_ARGSEP(pszArgs);
							CScript script(pszArgs);
							script.CopyParseState(s);

							for (; pMember != nullptr; pMember = pMember->GetNext())
							{
								if ( pMember->GetLinkUID().IsChar() )
									continue;

								if ( !iFlags )
									pMember->r_Verb(script, pSrc);
								else if ( ( iFlags == 1 ) && ( pMember->GetWeDeclaredWar() && !pMember->GetTheyDeclaredWar() ) )
									pMember->r_Verb(script, pSrc);
								else if ( ( iFlags == 2 ) && ( !pMember->GetWeDeclaredWar() && pMember->GetTheyDeclaredWar() ) )
									pMember->r_Verb(script, pSrc);
								else if ( ( iFlags == 3 ) && ( pMember->GetWeDeclaredWar() && pMember->GetTheyDeclaredWar() ) )
									pMember->r_Verb(script, pSrc);
							}
						}
						else
						{
							g_Log.EventError("ItemStone::AllGuilds empty args.\n");
							return false;
						}
					}
				}
			}
			break;
		case ISV_ALLMEMBERS:
			{
				if ( s.HasArgs() )
				{
					tchar * pszArgs = s.GetArgRaw();
					int iFlags = Exp_GetVal(pszArgs);

					if (( iFlags < -1 ) || ( iFlags > STONEPRIV_ACCEPTED ))
					{
						g_Log.EventError("ItemStone::AllMembers invalid parameter '%i'.\n", iFlags);
						return false;
					}
					else
					{
						if ( pszArgs[0] != '\0' )
						{
							pMember = static_cast <CStoneMember *>(GetContainerHead());
							SKIP_ARGSEP(pszArgs);
							CScript script(pszArgs);
							script.CopyParseState(s);

							for (; pMember != nullptr; pMember = pMember->GetNext())
							{
								if ( !pMember->GetLinkUID().IsChar() )
									continue;

								if ( iFlags == -1 )
									pMember->r_Verb(script, pSrc);
								else if ( pMember->GetPriv() == static_cast<STONEPRIV_TYPE>(iFlags) )
									pMember->r_Verb(script, pSrc);
							}
						}
						else
						{
							g_Log.EventError("ItemStone::AllMembers empty args.\n");
							return false;
						}
					}
				}
			}
			break;
		case ISV_APPLYTOJOIN:
			if ( s.HasArgs())
			{
				CChar * pMemberChar = CUID::CharFindFromUID(s.GetArgDWVal());
				if ( pMemberChar )
					AddRecruit( pMemberChar, STONEPRIV_CANDIDATE );
			}
			break;
		case ISV_CHANGEALIGN:
			if ( s.HasArgs())
			{
				SetALIGNTYPE(STONEALIGN_TYPE(s.GetArgVal()));
				tchar *pszMsg = Str_GetTemp();
				snprintf(pszMsg, SCRIPT_MAX_LINE_LEN, "%s is now a %s %s\n", GetName(), GetAlignName(), GetTypeName());
				Speak(pszMsg);
			}
			break;
		case ISV_DECLAREPEACE:
			if ( s.HasArgs())
			{
				CUID pMemberUid(s.GetArgDWVal());
				WeDeclarePeace(pMemberUid);
			}
			break;
		case ISV_DECLAREWAR:
			if ( s.HasArgs())
			{
				CUID pMemberUid(s.GetArgDWVal());
				CItem * pEnemyItem = pMemberUid.ItemFind();
				if ( pEnemyItem )
				{
					CItemStone * pNewEnemy = dynamic_cast<CItemStone*>(pEnemyItem);
					if ( pNewEnemy )
						WeDeclareWar(pNewEnemy);
				}
			}
			break;
		case ISV_ELECTMASTER:
			ElectMaster();
			break;
		// Need to change FixWeirdness or rewrite how cstonemember guilds are handled.
		case ISV_INVITEWAR:
			{
				if ( s.HasArgs() )
				{
					int64 piCmd[2];
					int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd));
					if ( iArgQty == 2 )
					{
						const CUID uidGuild((dword)piCmd[0]);
						bool fWeDeclared = (piCmd[1] != 0);
						CItem * pEnemyItem = uidGuild.ItemFind();
						if ( pEnemyItem && (pEnemyItem->IsType(IT_STONE_GUILD) || pEnemyItem->IsType(IT_STONE_TOWN)) )
						{
							CStoneMember * pMemberGuild = GetMember( pEnemyItem );
							if ( !pMemberGuild )
								pMemberGuild = new CStoneMember(this, uidGuild, STONEPRIV_ENEMY);

							if (fWeDeclared)
								pMemberGuild->SetWeDeclaredWar(true);
							else
								pMemberGuild->SetTheyDeclaredWar(true);
						}
					}
				}
			} break;
		case ISV_JOINASMEMBER:
			if ( s.HasArgs())
			{
				const CUID uidMember(s.GetArgDWVal());
				CChar * pMemberChar = uidMember.CharFind();
				if ( pMemberChar )
				{
					AddRecruit( pMemberChar, STONEPRIV_MEMBER );
				}
			}
			break;
		case ISV_RESIGN:
			if ( s.HasArgs())
			{
				const CUID uidMember(s.GetArgDWVal());
				CChar* pMemberChar = uidMember.CharFind();
				if ( pMemberChar )
				{
					CStoneMember * pMemberGuild = GetMember( pMemberChar );
					if ( pMemberGuild )
						delete pMemberGuild;
				}
			}
			else
			{
				if ( pMember != nullptr )
					delete pMember;
			}
			break;
		case ISV_TOGGLEABBREVIATION:
			{
				if (!s.HasArgs() && !pMember)
					return false;

				const CUID uidMember(s.HasArgs() ? s.GetArgDWVal() : pMember->GetLinkUID());
				CChar * pMemberChar = uidMember.CharFind();
				if ( pMemberChar )
				{
					CStoneMember * pMemberGuild = GetMember( pMemberChar );
					if ( pMemberGuild )
					{
						pMemberGuild->ToggleAbbrev();
						pMemberChar->UpdatePropertyFlag();
					}
				}
			}
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

CStoneMember * CItemStone::GetMasterMember() const
{
	ADDTOCALLSTACK("CItemStone::GetMasterMember");
	CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());
	for ( ; pMember != nullptr; pMember = pMember->GetNext())
	{
		if ( pMember->GetPriv() == STONEPRIV_MASTER )
			return pMember;
	}
	return nullptr;
}

bool CItemStone::IsPrivMember( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CItemStone::IsPrivMember");
	const CStoneMember * pMember = GetMember(pChar);
	if ( pMember == nullptr )
		return false;
	return( pMember->IsPrivMember());
}

CChar * CItemStone::GetMaster() const
{
	ADDTOCALLSTACK("CItemStone::GetMaster");
	CStoneMember * pMember = GetMasterMember();
	if ( pMember == nullptr )
		return nullptr;
	return pMember->GetLinkUID().CharFind();
}

CStoneMember * CItemStone::GetMember( const CObjBase * pObj ) const
{
	ADDTOCALLSTACK("CItemStone::GetMember");
	// Get member info for this char/item (if it has member info)
	if (!pObj)
		return nullptr;
	CUID otherUID = pObj->GetUID();
	CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());
	for ( ; pMember != nullptr; pMember = pMember->GetNext())
	{
		if ( pMember->GetLinkUID() == otherUID )
			return pMember;
	}
	return nullptr;
}

bool CItemStone::NoMembers() const
{
	ADDTOCALLSTACK("CItemStone::NoMembers");
	CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());
	for ( ; pMember != nullptr; pMember = pMember->GetNext())
	{
		if ( pMember->IsPrivMember())
			return false;
	}
	return true;
}

CStoneMember * CItemStone::AddRecruit(const CChar * pChar, STONEPRIV_TYPE iPriv, bool bFull)
{
	ADDTOCALLSTACK("CItemStone::AddRecruit");
	// CLIMODE_TARG_STONE_RECRUIT
	// Set as member or candidate.

	if ( !pChar || !pChar->m_pPlayer )
	{
		Speak( "Only players can be members!");
		return nullptr;
	}

	tchar * z = Str_GetTemp();
	const CItemStone * pStone = pChar->Guild_Find( GetMemoryType());
	if ( pStone && pStone != this )
	{
		snprintf(z, SCRIPT_MAX_LINE_LEN, "%s appears to belong to %s. Must resign previous %s", pChar->GetName(), pStone->GetName(), GetTypeName());
		Speak(z);
		return nullptr;
	}

	if ( IsType(IT_STONE_TOWN) && IsAttr(ATTR_OWNED) && iPriv == STONEPRIV_CANDIDATE )
	{
		// instant member.
		iPriv = STONEPRIV_MEMBER;
	}

	CStoneMember * pMember = GetMember(pChar);
	if ( pMember )
	{
		// I'm already a member of some sort.
		if ( pMember->GetPriv() == iPriv || iPriv == STONEPRIV_CANDIDATE )
		{
			snprintf(z, SCRIPT_MAX_LINE_LEN, "%s is already %s %s.", pChar->GetName(), pMember->GetPrivName(), GetName());
			Speak(z);
			return nullptr;
		}

		pMember->SetPriv( iPriv );
	}
	else
	{
		pMember = new CStoneMember(this, pChar->GetUID(), iPriv);

		if ( bFull )	// full join means becoming a full member already
		{
			pMember->SetPriv(STONEPRIV_MEMBER);
			pMember->SetAbbrev(true);
		}
	}

	if ( pMember->IsPrivMember())
	{
		pMember->SetLoyalTo(pChar);
		ElectMaster();	// just in case this is the first.
	}

	snprintf(z, SCRIPT_MAX_LINE_LEN, "%s is now %s %s", pChar->GetName(), pMember->GetPrivName(), GetName());
	Speak(z);
	return pMember;
}

CMultiStorage * CItemStone::GetMultiStorage()
{
    return _pMultiStorage;
}

void CItemStone::ElectMaster()
{
	ADDTOCALLSTACK("CItemStone::ElectMaster");
	// Check who is loyal to who and find the new master.
	if ( GetAmount() == 0 )
		return;	// no reason to elect new if the stone is dead.

	int iResultCode = FixWeirdness();	// try to eliminate bad members.
	if ( iResultCode )
	{
		// The stone is bad ?
		// iResultCode
	}

	int iCountMembers = 0;
	CStoneMember * pMaster = nullptr;

	// Validate the items and Clear the votes field
	CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());
    CStoneMember * pMemberNext = nullptr;
	for ( ; pMember != nullptr; pMember = pMemberNext)
	{
        pMemberNext = pMember->GetNext();
		if ( pMember->GetPriv() == STONEPRIV_MASTER )
		{
			pMaster = pMember;	// find current master.
		}
		else if ( pMember->GetPriv() != STONEPRIV_MEMBER )
		{
			continue;
		}
		pMember->m_Member.m_iVoteTally = 0;
		++iCountMembers;
	}

	// Now tally the votes.
	pMemberNext = nullptr, pMember = static_cast <CStoneMember *>(GetContainerHead());
	for ( ; pMember != nullptr; pMember = pMemberNext)
	{
        pMemberNext = pMember->GetNext();
		if ( ! pMember->IsPrivMember())
			continue;

		CChar * pCharVote = pMember->GetLoyalToUID().CharFind();
		if ( pCharVote != nullptr )
		{
			CStoneMember * pMemberVote = GetMember( pCharVote );
			if ( pMemberVote != nullptr )
			{
				++ pMemberVote->m_Member.m_iVoteTally;
				continue;
			}
		}

		// not valid to vote for. change to self.
		pMember->SetLoyalTo(nullptr);
		// Assume I voted for myself.
		++ pMember->m_Member.m_iVoteTally;
	}

	// Find who won.
	bool fTie = false;
	CStoneMember * pMemberHighest = nullptr;
    pMemberNext = nullptr, pMember = static_cast <CStoneMember *>(GetContainerHead());
	for ( ; pMember != nullptr; pMember = pMemberNext)
	{
        pMemberNext = pMember->GetNext();
		if ( ! pMember->IsPrivMember())
			continue;
		if ( pMemberHighest == nullptr )
		{
			pMemberHighest = pMember;
			continue;
		}
		if ( pMember->m_Member.m_iVoteTally == pMemberHighest->m_Member.m_iVoteTally )
		{
			fTie = true;
		}
		if ( pMember->m_Member.m_iVoteTally > pMemberHighest->m_Member.m_iVoteTally )
		{
			fTie = false;
			pMemberHighest = pMember;
		}
	}

	// In the event of a tie, leave the current master as is
	if ( ! fTie && pMemberHighest )
	{
		if (pMaster)
			pMaster->SetPriv(STONEPRIV_MEMBER);
		pMemberHighest->SetPriv(STONEPRIV_MASTER);
	}

	if ( ! iCountMembers )
	{
		// No more members, declare peace (by force)
        pMemberNext = nullptr, pMember = static_cast <CStoneMember *>(GetContainerHead());
		for (; pMember != nullptr; pMember = pMemberNext)
		{
            pMemberNext = pMember->GetNext();
			WeDeclarePeace(pMember->GetLinkUID(), true);
		}
	}
}

bool CItemStone::IsUniqueName( lpctstr pName ) // static
{
	ADDTOCALLSTACK("CItemStone::IsUniqueName");
	for ( size_t i = 0; i < g_World.m_Stones.size(); ++i )
	{
		if ( ! strcmpi( pName, g_World.m_Stones[i]->GetName()))
			return false;
	}
	return true;
}

bool CItemStone::CheckValidMember( CStoneMember * pMember )
{
	ADDTOCALLSTACK("CItemStone::CheckValidMember");
	ASSERT(pMember);
	ASSERT( pMember->GetParent() == this );

	if ( (GetAmount() == 0) || g_Serv.GetExitFlag() )	// no reason to elect new if the stone is dead.
		return true;	// we are deleting anyhow.

	switch ( pMember->GetPriv())
	{
		case STONEPRIV_MASTER:
		case STONEPRIV_MEMBER:
		case STONEPRIV_CANDIDATE:
		case STONEPRIV_ACCEPTED:
			if ( GetMemoryType())
			{
				// Make sure the member has a memory that links them back here.
				CChar * pChar = pMember->GetLinkUID().CharFind();
				if ( pChar == nullptr )
					break;
				if ( pChar->Guild_Find( GetMemoryType()) != this )
					break;
			}
			return true;
		case STONEPRIV_ENEMY:
			{
				CItemStone * pEnemyStone = dynamic_cast <CItemStone *>( pMember->GetLinkUID().ItemFind());
				if ( pEnemyStone == nullptr )
					break;
				CStoneMember * pEnemyMember = pEnemyStone->GetMember(this);
				if ( pEnemyMember == nullptr )
					break;
				if ( pMember->GetWeDeclaredWar() && ! pEnemyMember->GetTheyDeclaredWar())
					break;
				if ( pMember->GetTheyDeclaredWar() && ! pEnemyMember->GetWeDeclaredWar())
					break;
			}
			return true;

		default:
			break;
	}

	// just delete this member. (it is mislinked)
	DEBUG_ERR(( "Stone UID=0%x has mislinked member uid=0%x\n", 
		(dword) GetUID(), (dword) pMember->GetLinkUID()));
	return false;
}

int CItemStone::FixWeirdness()
{
	ADDTOCALLSTACK("CItemStone::FixWeirdness");
	// Check all my members. Make sure all wars are reciprocated and members are flaged.

	int iResultCode = CItem::FixWeirdness();
	if ( iResultCode )
	{
		return( iResultCode );
	}

	bool fChanges = false;
	CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());
	while ( pMember != nullptr )
	{
		CStoneMember * pMemberNext = pMember->GetNext();
		if ( ! CheckValidMember(pMember))
		{
			IT_TYPE oldtype = GetType();
			SetAmount(0);	// turn off validation for now. we don't want to delete other members.
			delete pMember;
			SetAmount(1);	// turn off validation for now. we don't want to delete other members.
			SetType( oldtype );
			fChanges = true;
		}
		pMember = pMemberNext;
	}

	if ( fChanges )
	{
		ElectMaster();	// May have changed the vote count.
	}
	return 0;
}

bool CItemStone::IsAlliedWith( const CItemStone * pStone) const
{
	ADDTOCALLSTACK("CItemStone::IsAlliedWith");
	if ( pStone == nullptr )
		return false;

	CScriptTriggerArgs Args;
	Args.m_pO1 = const_cast<CItemStone *>(pStone);
	enum TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;

	if ( const_cast<CItemStone *>(this)->r_Call("f_stonesys_internal_isalliedwith", &g_Serv, &Args, nullptr, &tr) )
	{
		if ( tr == TRIGRET_RET_FALSE )
			return false;
		else if ( tr == TRIGRET_RET_TRUE )
			return true;
	}

	// we have declared or they declared.
	CStoneMember * pAllyMember = GetMember(pStone);
	if ( pAllyMember ) // Ok, we might be ally
	{
		if ( pAllyMember->m_iPriv == STONEPRIV_ALLY && pAllyMember->GetTheyDeclaredAlly() && pAllyMember->GetWeDeclaredAlly() )
			return true;
	}

	// Just based on align type.
	/*
	if ( (GetAlignType() != STONEALIGN_STANDARD) && (GetAlignType() == pStone->GetAlignType()) )
		return true;
	*/

	return false;
}

bool CItemStone::IsAtWarWith( const CItemStone * pEnemyStone ) const
{
	ADDTOCALLSTACK("CItemStone::IsAtWarWith");
	// Boths guild shave declared war on each other.

	if ( pEnemyStone == nullptr )
		return false;

	CScriptTriggerArgs Args;
	Args.m_pO1 = const_cast<CItemStone *>(pEnemyStone);
	enum TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;

	if ( const_cast<CItemStone *>(this)->r_Call("f_stonesys_internal_isatwarwith", &g_Serv, &Args, nullptr, &tr) )
	{
		if ( tr == TRIGRET_RET_FALSE )
			return false;
		else if ( tr == TRIGRET_RET_TRUE )
			return true;
	}

	// we have declared or they declared.
	CStoneMember * pEnemyMember = GetMember(pEnemyStone);
	if (pEnemyMember) // Ok, we might be at war
	{
		if ( pEnemyMember->m_iPriv == STONEPRIV_ENEMY && pEnemyMember->GetTheyDeclaredWar() && pEnemyMember->GetWeDeclaredWar())
			return true;
	}

	// Just based on align type.
	/*
	if ( (GetAlignType() != STONEALIGN_STANDARD) &&	(pEnemyStone->GetAlignType() != STONEALIGN_STANDARD) )
		return ( GetAlignType() != pEnemyStone->GetAlignType() );
	*/

	return false;
}

void CItemStone::AnnounceWar( const CItemStone * pEnemyStone, bool fWeDeclare, bool fWar )
{
	ADDTOCALLSTACK("CItemStone::AnnounceWar");
	// Announce we are at war or peace.

	ASSERT(pEnemyStone);

	bool fAtWar = IsAtWarWith(pEnemyStone);

	tchar *pszTemp = Str_GetTemp();
	int len = snprintf( pszTemp, Str_TempLength(), (fWar) ? "%s %s declared war on %s." : "%s %s requested peace with %s.",
		(fWeDeclare) ? "You" : pEnemyStone->GetName(),
		(fWeDeclare) ? "have" : "has",
		(fWeDeclare) ? pEnemyStone->GetName() : "You" );

	if ( fAtWar )
		snprintf( pszTemp+len, Str_TempLength() - len, " War is ON!" );
	else if ( fWar )
		snprintf( pszTemp+len, Str_TempLength() - len, " War is NOT yet on." );
	else
		snprintf( pszTemp+len, Str_TempLength() - len, " War is OFF." );

	CStoneMember * pMember = static_cast <CStoneMember *>(GetContainerHead());
	for ( ; pMember != nullptr; pMember = pMember->GetNext())
	{
		CChar * pChar = pMember->GetLinkUID().CharFind();
		if ( pChar == nullptr )
			continue;
		if ( ! pChar->IsClientActive())
			continue;
		pChar->SysMessage( pszTemp );
	}
}

bool CItemStone::WeDeclareWar(CItemStone * pEnemyStone)
{
	ADDTOCALLSTACK("CItemStone::WeDeclareWar");
	if (!pEnemyStone)
		return false;

	// See if they've already declared war on us
	CStoneMember * pMember = GetMember(pEnemyStone);
	if ( pMember )
	{
		if ( pMember->GetWeDeclaredWar())
			return true;
	}
	else // They haven't, make a record of this
	{
		pMember = new CStoneMember( this, pEnemyStone->GetUID(), STONEPRIV_ENEMY );
	}
	pMember->SetWeDeclaredWar(true);

	// Now inform the other stone
	// See if they have already declared war on us
	CStoneMember * pEnemyMember = pEnemyStone->GetMember(this);
	if (!pEnemyMember) // Not yet it seems
		pEnemyMember = new CStoneMember( pEnemyStone, GetUID(), STONEPRIV_ENEMY );

	pEnemyMember->SetTheyDeclaredWar(true);
	return true;
}

void CItemStone::TheyDeclarePeace( CItemStone* pEnemyStone, bool fForcePeace )
{
	ADDTOCALLSTACK("CItemStone::TheyDeclarePeace");
	// Now inform the other stone
	// Make sure we declared war on them
	CStoneMember * pEnemyMember = GetMember(pEnemyStone);
	if ( ! pEnemyMember )
		return;

	bool fPrevTheyDeclared = pEnemyMember->GetTheyDeclaredWar();

	if (!pEnemyMember->GetWeDeclaredWar() || fForcePeace) // If we're not at war with them, delete this record
		delete pEnemyMember;
	else
		pEnemyMember->SetTheyDeclaredWar(false);

	if ( ! fPrevTheyDeclared )
		return;
}

void CItemStone::WeDeclarePeace(CUID uid, bool fForcePeace)
{
	ADDTOCALLSTACK("CItemStone::WeDeclarePeace");
	CItemStone * pEnemyStone = dynamic_cast <CItemStone*>( uid.ItemFind());
	if (!pEnemyStone)
		return;

	CStoneMember * pMember = GetMember(pEnemyStone);
	if ( ! pMember ) // No such member
		return;

	// Set my flags on the subject.
	if (!pMember->GetTheyDeclaredWar() || fForcePeace) // If they're not at war with us, delete this record
		delete pMember;
	else
		pMember->SetWeDeclaredWar(false);

	pEnemyStone->TheyDeclarePeace( this, fForcePeace );
}

void CItemStone::SetTownName()
{
	ADDTOCALLSTACK("CItemStone::SetTownName");
	// For town stones.
	if ( ! IsTopLevel())
		return;
	CRegion * pArea = GetTopPoint().GetRegion(( IsType(IT_STONE_TOWN)) ? REGION_TYPE_AREA : REGION_TYPE_ROOM );
	if ( pArea )
		pArea->SetName( GetIndividualName());
}

bool CItemStone::MoveTo(const CPointMap& pt, bool bForceFix)
{
	ADDTOCALLSTACK("CItemStone::MoveTo");
	// Put item on the ground here.
	if ( IsType(IT_STONE_TOWN) )
		SetTownName();
	return CItem::MoveTo(pt, bForceFix);
}

bool CItemStone::SetName( lpctstr pszName )
{
	ADDTOCALLSTACK("CItemStone::SetName");
	// If this is a town then set the whole regions name.
	if ( !CItem::SetName(pszName) )
		return false;
	if ( IsTopLevel() && IsType(IT_STONE_TOWN) )
		SetTownName();

	return true;
}
