
#include "../../common/CException.h"
#include "../../common/CUIDExtra.h"
#include "../chars/CChar.h"
#include "../../common/CLog.h"
#include "../CWorld.h"
#include "CItemStone.h"

//////////
// -CStoneMember

enum STMM_TYPE
{
#define ADD(a,b) STMM_##a,
#include "../../tables/CStoneMember_props.tbl"
#undef ADD
	STMM_QTY
};

lpctstr const CStoneMember::sm_szLoadKeys[STMM_QTY+1] =
{
#define ADD(a,b) b,
#include "../../tables/CStoneMember_props.tbl"
#undef ADD
	NULL
};

enum STMMV_TYPE
{
#define ADD(a,b) STMMV_##a,
#include "../../tables/CStoneMember_functions.tbl"
#undef ADD
	STMMV_QTY
};

lpctstr const CStoneMember::sm_szVerbKeys[STMMV_QTY+1] =
{
#define ADD(a,b) b,
#include "../../tables/CStoneMember_functions.tbl"
#undef ADD
	NULL
};

CStoneMember* CStoneMember::GetNext() const
{
	ADDTOCALLSTACK("CStoneMember::GetNext");
	return static_cast <CStoneMember *>( CSObjListRec::GetNext());
}

CUID CStoneMember::GetLinkUID() const { return m_uidLinkTo; }

STONEPRIV_TYPE CStoneMember::GetPriv() const { return m_iPriv; }
void CStoneMember::SetPriv(STONEPRIV_TYPE iPriv) { m_iPriv = iPriv; }
bool CStoneMember::IsPrivMaster() const { return m_iPriv == STONEPRIV_MASTER; }
bool CStoneMember::IsPrivMember() const { return( m_iPriv == STONEPRIV_MASTER || m_iPriv == STONEPRIV_MEMBER ); }

// If the member is really a war flag (STONEPRIV_ENEMY)
void CStoneMember::SetWeDeclared(bool f)
{
	ADDTOCALLSTACK("CStoneMember::SetWeDeclared");
	m_Enemy.m_fWeDeclared = f;
}
bool CStoneMember::GetWeDeclared() const
{
	ADDTOCALLSTACK("CStoneMember::GetWeDeclared");
	return ( m_Enemy.m_fWeDeclared ) ? true : false;
}
void CStoneMember::SetTheyDeclared(bool f)
{
	ADDTOCALLSTACK("CStoneMember::SetTheyDeclared");
	m_Enemy.m_fTheyDeclared = f;
}
bool CStoneMember::GetTheyDeclared() const
{
	ADDTOCALLSTACK("CStoneMember::GetTheyDeclared");
	return ( m_Enemy.m_fTheyDeclared ) ? true : false;
}

// Member
bool CStoneMember::IsAbbrevOn() const { return ( m_Member.m_fAbbrev ) ? true : false; }
void CStoneMember::ToggleAbbrev() { m_Member.m_fAbbrev = !m_Member.m_fAbbrev; }
void CStoneMember::SetAbbrev(bool mode) { m_Member.m_fAbbrev = mode; }

lpctstr CStoneMember::GetTitle() const
{
	ADDTOCALLSTACK("CStoneMember::GetTitle");
	return( m_sTitle );
}
void CStoneMember::SetTitle( lpctstr pTitle )
{
	ADDTOCALLSTACK("CStoneMember::SetTitle");
	m_sTitle = pTitle;
}

CUID CStoneMember::GetLoyalToUID() const
{
	ADDTOCALLSTACK("CStoneMember::GetLoyalToUID");
	return( m_uidLoyalTo );
}

int CStoneMember::GetAccountGold() const
{
	ADDTOCALLSTACK("CStoneMember::GetAccountGold");
	return( m_Member.m_iAccountGold );
}
void CStoneMember::SetAccountGold( int iGold )
{
	ADDTOCALLSTACK("CStoneMember::SetAccountGold");
	m_Member.m_iAccountGold = iGold;
}

bool CStoneMember::r_GetRef( lpctstr & pszKey, CScriptObj * & pRef )
{
	UNREFERENCED_PARAMETER(pszKey);
	UNREFERENCED_PARAMETER(pRef);
	return false;
}

bool CStoneMember::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CStoneMember::r_Verb");
	EXC_TRY("Verb");

	ASSERT(pSrc);
	
	lpctstr pszKey = s.GetKey();
	int index = FindTableSorted( pszKey, sm_szVerbKeys, CountOf(sm_szVerbKeys)-1 );
	if ( index < 0 )
	{
		if ( r_LoadVal(s) ) // if it's successful all ok, else go on verb.
		{
			return true;
		}
	}

	if ( GetLinkUID().IsChar() )
	{
		CScriptObj *pRef = GetLinkUID().CharFind();
		return pRef->r_Verb( s, pSrc );
	}
	else if ( GetLinkUID().IsItem() )
	{
		CScriptObj *pRef = GetLinkUID().ItemFind();
		return pRef->r_Verb( s, pSrc );
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}


bool CStoneMember::r_LoadVal( CScript & s ) // Load an item Script
{
	ADDTOCALLSTACK("CStoneMember::r_LoadVal");
	EXC_TRY("LoadVal");

	STMM_TYPE iIndex = (STMM_TYPE) FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );

	if ( GetLinkUID().IsChar() )
	{
		switch ( iIndex )
		{
			case STMM_ACCOUNTGOLD:
				SetAccountGold(s.GetArgVal());
				break;

			case STMM_LOYALTO:
				{
					CUID uid = s.GetArgVal();
					SetLoyalTo(uid.CharFind());
				}
				break;

			case STMM_PRIV:
				SetPriv(static_cast<STONEPRIV_TYPE>(s.GetArgVal()));
				break;

			case STMM_TITLE:
				SetTitle(s.GetArgStr());
				break;

			case STMM_SHOWABBREV:
				SetAbbrev( s.GetArgVal() ? 1 : 0 );
				break;

			default:
				return false;
		}
	} 
	else if ( GetLinkUID().IsItem() )
	{
		switch ( iIndex )
		{
			case STMM_GUILD_THEYALLIANCE:
				break;

			case STMM_GUILD_THEYWAR:
				SetTheyDeclared(s.GetArgVal() ? true : false);
				break;

			case STMM_GUILD_WEALLIANCE:
				break;

			case STMM_GUILD_WEWAR:
				SetWeDeclared(s.GetArgVal() ? true : false);
				break;

			default:
				return false;
		}
	}


	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}


bool CStoneMember::r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CStoneMember::r_WriteVal");
	EXC_TRY("WriteVal");

	STMM_TYPE iIndex = (STMM_TYPE) FindTableSorted( pszKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );

	if ( GetLinkUID().IsChar() )
	{
		switch ( iIndex )
		{
			case STMM_ACCOUNTGOLD:
				sVal.FormatVal(GetAccountGold());
				break;
			case STMM_ISMASTER:
				sVal.FormatVal(IsPrivMaster());
				break;
			case STMM_ISMEMBER:
				sVal.FormatVal(IsPrivMember());
				break;
			case STMM_LOYALTO:
				sVal.FormatHex(GetLoyalToUID());
				break;
			case STMM_PRIV:
				sVal.FormatVal(GetPriv());
				break;
			case STMM_PRIVNAME:
				sVal = GetPrivName();
				break;
			case STMM_TITLE:
				sVal = GetTitle();
				break;
			case STMM_SHOWABBREV:
				sVal.FormatVal(IsAbbrevOn());
				break;
			case STMM_ISCANDIDATE:
				sVal.FormatVal((GetPriv() == STONEPRIV_CANDIDATE) ? 1 : 0);
				break;
			default:
				{
					CScriptObj *pRef = GetLinkUID().CharFind();
					return pRef->r_WriteVal(pszKey,sVal,pSrc);
				}
		}
	}
	else if ( GetLinkUID().IsItem() )
	{
		switch ( iIndex )
		{
			case STMM_GUILD_ISENEMY:
				sVal.FormatVal(GetWeDeclared() && GetTheyDeclared());
				break;
			case STMM_GUILD_THEYWAR:
				sVal.FormatVal(GetTheyDeclared());
				break;
			case STMM_GUILD_WEWAR:
				sVal.FormatVal(GetWeDeclared());
				break;
			case STMM_GUILD_WEALLIANCE:
				sVal.FormatVal(0);
				break;
			case STMM_GUILD_THEYALLIANCE:
				sVal.FormatVal(0);
				break;
			default:
				{
					CScriptObj *pRef = GetLinkUID().ItemFind();
					return pRef->r_WriteVal(pszKey,sVal,pSrc);
				}
		}
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

CStoneMember::CStoneMember( CItemStone * pStone, CUID uid, STONEPRIV_TYPE iType, lpctstr pTitle, CUID loyaluid, bool fVal1, bool fVal2, int nAccountGold)
{
	m_uidLinkTo = uid;
	m_sTitle = pTitle;
	m_iPriv = iType;
	m_uidLoyalTo = loyaluid;

	// union.
	if ( iType == STONEPRIV_ENEMY )
	{
		m_Enemy.m_fTheyDeclared = fVal1;
		m_Enemy.m_fWeDeclared = fVal2;
	}
	else
	{
		m_Member.m_fAbbrev = fVal1;
		m_Member.m_iVoteTally = fVal2;		// Temporary space to calculate votes.
	}

	m_Member.m_iAccountGold = nAccountGold;

	if ( ! g_Serv.IsLoading() && pStone->GetMemoryType())
	{
		CChar * pChar = uid.CharFind();
		if ( pChar != NULL )
		{
			pChar->Memory_AddObjTypes(pStone, (word)(pStone->GetMemoryType()));
			if ( pStone->IsTopLevel())
			{
				pChar->m_ptHome = pStone->GetTopPoint();	// Our new home.
			}
		}
	}

	pStone->InsertTail( this );
}

CStoneMember::~CStoneMember()
{
	CItemStone * pStone = GetParentStone();
	if ( ! pStone )
		return;

	RemoveSelf();

	if ( m_iPriv == STONEPRIV_ENEMY )
	{
		// same as declaring peace.
		CItemStone * pStoneEnemy = dynamic_cast <CItemStone *>( GetLinkUID().ItemFind());
		if ( pStoneEnemy != NULL )
		{
			pStoneEnemy->TheyDeclarePeace( pStone, true );
		}
	}
	else if ( pStone->GetMemoryType())
	{
		// If we remove a char with good loyalty we may have changed the vote count.
		pStone->ElectMaster();

		CChar * pChar = GetLinkUID().CharFind();
		if ( pChar )
		{
			pChar->Memory_ClearTypes((word)(pStone->GetMemoryType())); 	// Make them forget they were ever in this guild
		}
	}
}

CItemStone * CStoneMember::GetParentStone() const
{
	ADDTOCALLSTACK("CStoneMember::GetParentStone");
	return dynamic_cast <CItemStone *>( GetParent());
}

lpctstr CStoneMember::GetPrivName() const
{
	ADDTOCALLSTACK("CStoneMember::GetPrivName");
	STONEPRIV_TYPE iPriv = GetPriv();

	TemporaryString sDefname;
	sprintf(sDefname, "STONECONFIG_PRIVNAME_PRIVID-%d", (int)(iPriv));
	
	CVarDefCont * pResult = g_Exp.m_VarDefs.GetKey(sDefname);
	if (pResult)
		return pResult->GetValStr();
	else
		pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_PRIVNAME_PRIVUNK");

	return ( pResult == NULL ) ? "" : pResult->GetValStr();
}

bool CStoneMember::SetLoyalTo( const CChar * pCharLoyal )
{
	ADDTOCALLSTACK("CStoneMember::SetLoyalTo");
	CChar * pCharMe = GetLinkUID().CharFind();
	if ( pCharMe == NULL )	// on shutdown
		return false;

	m_uidLoyalTo = GetLinkUID();	// set to self for default.
	if ( pCharLoyal == NULL )
		return true;

	if ( ! IsPrivMember())
	{
		// non members can't vote
		pCharMe->SysMessage("Candidates aren't elligible to vote.");
		return false;
	}

	CItemStone * pStone = GetParentStone();
	if ( !pStone )
		return false;

	CStoneMember * pNewLoyalTo = pStone->GetMember(pCharLoyal);
	if ( pNewLoyalTo == NULL || ! pNewLoyalTo->IsPrivMember())
	{
		// you can't vote for candidates
		pCharMe->SysMessage( "Can only vote for full members.");
		return false;
	}

	m_uidLoyalTo = pCharLoyal->GetUID();

	// new vote tally
	pStone->ElectMaster();
	return true;
}

//////////
// -CItemStone

CItemStone::CItemStone( ITEMID_TYPE id, CItemBase * pItemDef ) : CItem( id, pItemDef )
{
	m_itStone.m_iAlign = STONEALIGN_STANDARD;
	g_World.m_Stones.Add( this );
}

CItemStone::~CItemStone()
{
	SetAmount(0);	// Tell everyone we are deleting.
	DeletePrepare();	// Must remove early because virtuals will fail in child destructor.

	// Remove this stone from the links of guilds in the world
	g_World.m_Stones.RemovePtr( this );

	// all members are deleted automatically.
	Empty();	// do this manually to preserve the parents type cast
}

MEMORY_TYPE CItemStone::GetMemoryType() const
{
	ADDTOCALLSTACK("CItemStone::GetMemoryType");
	switch ( GetType() )
	{
		case IT_STONE_TOWN: return( MEMORY_TOWN );
		case IT_STONE_GUILD: return( MEMORY_GUILD );
		default: return( MEMORY_NONE ); // Houses have no memories.
	}
}

lpctstr CItemStone::GetCharter(uint iLine) const
{
	ASSERT(iLine<CountOf(m_sCharter));
	return(  m_sCharter[iLine] );
}

void CItemStone::SetCharter( uint iLine, lpctstr pCharter )
{
	ASSERT(iLine<CountOf(m_sCharter));
	m_sCharter[iLine] = pCharter;
}

lpctstr CItemStone::GetWebPageURL() const
{
	return( m_sWebPageURL );
}

void CItemStone::SetWebPage( lpctstr pWebPage )
{
	m_sWebPageURL = pWebPage;
}

STONEALIGN_TYPE CItemStone::GetAlignType() const
{
	return m_itStone.m_iAlign;
}

void CItemStone::SetAlignType(STONEALIGN_TYPE iAlign)
{
	m_itStone.m_iAlign = iAlign;
}

lpctstr CItemStone::GetAbbrev() const
{
	return( m_sAbbrev );
}

void CItemStone::SetAbbrev( lpctstr pAbbrev )
{
	m_sAbbrev = pAbbrev;
}

lpctstr CItemStone::GetTypeName() const
{
	ADDTOCALLSTACK("CItemStone::GetTypeName");
	CVarDefCont * pResult = NULL;
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

	if ( pResult == NULL )
		pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_TYPENAME_UNK");

	return ( pResult == NULL ) ? "" : pResult->GetValStr();
}

void CItemStone::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemStone::r_Write");
	CItem::r_Write( s );
	s.WriteKeyVal( "ALIGN", GetAlignType());
	if ( ! m_sAbbrev.IsEmpty())
	{
		s.WriteKey( "ABBREV", m_sAbbrev );
	}

	TemporaryString pszTemp;
	for ( uint i = 0; i < CountOf(m_sCharter); ++i )
	{
		if ( ! m_sCharter[i].IsEmpty())
		{
			sprintf(pszTemp, "CHARTER%u", i);
			s.WriteKey(pszTemp, m_sCharter[i] );
		}
	}

	if ( ! m_sWebPageURL.IsEmpty())
	{
		s.WriteKey( "WEBPAGE", GetWebPageURL() );
	}

	// s.WriteKey( "//", "uid,title,priv,loyaluid,abbr&theydecl,wedecl");

	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if (pMember->GetLinkUID().IsValidUID()) // To protect against characters that were deleted!
		{
			s.WriteKeyFormat( "MEMBER",
				"0%x,%s,%i,0%x,%i,%i,%i",
				(dword) pMember->GetLinkUID() | (pMember->GetLinkUID().IsItem() ? UID_F_ITEM : 0),
				static_cast<lpctstr>(pMember->GetTitle()),
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

	TemporaryString sDefname;
	if ( GetType() == IT_STONE_GUILD )
		sprintf(sDefname, "GUILDCONFIG_ALIGN_%d", iAlign);
	else if ( GetType() == IT_STONE_TOWN )
		sprintf(sDefname, "TOWNSCONFIG_ALIGN_%d", iAlign);
	else
		return "";

	lpctstr sRes = g_Exp.m_VarDefs.GetKeyStr(sDefname);
	return ( sRes == NULL ) ? "" : sRes;
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
	NULL
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
	NULL
};

bool CItemStone::r_GetRef( lpctstr & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CItemStone::r_GetRef");
	if ( !strnicmp("member.", pszKey, 7) )
	{
		pszKey = pszKey + 7;
		if ( !pszKey[0] )
			return false;

		int nNumber = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());

		for ( int i = 0; pMember != NULL; pMember = pMember->GetNext() )
		{
			if ( !pMember->GetLinkUID().IsChar() ) 
				continue;

			if ( nNumber == i )
			{
				pRef = pMember; 
				return true;
			}

			i++;
		}
	}
	else if ( !strnicmp("memberfromuid.", pszKey, 14) )
	{
		pszKey = pszKey + 14;
		if ( !pszKey[0] )
			return false;

		CUID pMemberUid = Exp_GetDWVal(pszKey);
		SKIP_SEPARATORS(pszKey);

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
	else if ( !strnicmp("guild.", pszKey, 6) )
	{
		pszKey = pszKey + 6;
		if ( !pszKey[0] )
			return false;

		int nNumber = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());

		for ( int i = 0; pMember != NULL; pMember = pMember->GetNext() )
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
	else if ( !strnicmp("guildfromuid.", pszKey, 13) )
	{
		pszKey = pszKey + 13;
		if ( !pszKey[0] )
			return false;

		CUID pGuildUid = Exp_GetDWVal(pszKey);
		SKIP_SEPARATORS(pszKey);

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

	return CItem::r_GetRef( pszKey, pRef );
}

bool CItemStone::r_LoadVal( CScript & s ) // Load an item Script
{
	ADDTOCALLSTACK("CItemStone::r_LoadVal");
	EXC_TRY("LoadVal");

	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		case STC_ABBREV: // "ABBREV"
			m_sAbbrev = s.GetArgStr();
			return true;
		case STC_ALIGN: // "ALIGN"
			SetAlignType(static_cast<STONEALIGN_TYPE>(s.GetArgVal()));
			return true;
		case STC_MasterUid:
			{
				if ( s.HasArgs() )
				{
					CUID pNewMasterUid = (dword) s.GetArgVal();
					CChar * pChar = pNewMasterUid.CharFind();
					if ( !pChar )
					{
						DEBUG_ERR(( "MASTERUID called on non char 0%x uid.\n", (dword)pNewMasterUid ));
						return false;
					}

					CStoneMember * pNewMaster = GetMember( pChar );
					if ( !pNewMaster )
					{
						DEBUG_ERR(( "MASTERUID called on char 0%x (%s) that is not a valid member of stone with 0x%x uid.\n", (dword)pNewMasterUid, pChar->GetName(), (dword)GetUID() ));
						return false;
					}

					CStoneMember * pMaster = GetMasterMember();
					if ( pMaster )
					{
						if ( pMaster->GetLinkUID() == pNewMasterUid )
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
			size_t Arg_Qty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, CountOf( Arg_ppCmd ), "," );
			if (Arg_Qty < 1) // must at least provide the member uid
				return false;

			new CStoneMember(
				this,
				ahextoi(Arg_ppCmd[0]),													// Member's UID
				Arg_Qty > 2 ? static_cast<STONEPRIV_TYPE>(ATOI(Arg_ppCmd[2])) : STONEPRIV_CANDIDATE,	// Members priv level (use as a type)
				Arg_Qty > 1 ? Arg_ppCmd[1] : "",										// Title
				ahextoi(Arg_ppCmd[3]),													// Member is loyal to this
				Arg_Qty > 4 ? (ATOI( Arg_ppCmd[4] ) != 0) : 0,							// Paperdoll stone abbreviation (also if they declared war)
				Arg_Qty > 5 ? (ATOI( Arg_ppCmd[5] ) != 0) : 0,							// If we declared war
				Arg_Qty > 6 ? ATOI( Arg_ppCmd[6] ) : 0);								// AccountGold
			}
			return true;
		case STC_WEBPAGE: // "WEBPAGE"
			m_sWebPageURL = s.GetArgStr();
			return true;
	}

	if ( s.IsKeyHead( sm_szLoadKeys[STC_CHARTER], 7 ))
	{
		uint i = ATOI(s.GetKey() + 7);
		if ( i >= CountOf(m_sCharter))
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

bool CItemStone::r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemStone::r_WriteVal");
	EXC_TRY("WriteVal");
	CChar * pCharSrc = pSrc->GetChar();

	if ( !strnicmp("member.",pszKey,7) )
	{
		lpctstr pszCmd = pszKey + 7;

		if ( !strnicmp("COUNT",pszCmd,5) )
		{
			pszCmd = pszCmd + 5;

			int i = 0;
			CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());

			if ( *pszCmd )
			{
				SKIP_ARGSEP(pszCmd);
				STONEPRIV_TYPE iPriv = static_cast<STONEPRIV_TYPE>(Exp_GetVal(pszCmd));

				for (; pMember != NULL; pMember = pMember->GetNext())
				{
					if ( !pMember->GetLinkUID().IsChar() )
						continue;

					if ( pMember->GetPriv() != iPriv )
						continue;

					i++;
				}
			}
			else
			{
				for (; pMember != NULL; pMember = pMember->GetNext())
				{
					if (!pMember->GetLinkUID().IsChar()) 
						continue;

					i++;
				}
			}

			sVal.FormatVal(i);
			return true;
		}
		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
		sVal.FormatVal(0);

		for ( int i = 0 ; pMember != NULL; pMember = pMember->GetNext() )
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
	else if ( !strnicmp("memberfromuid.", pszKey, 14) )
	{
		lpctstr pszCmd = pszKey + 14;
		sVal.FormatVal(0);

		if ( !pszCmd[0] )
			return true;

		CUID pMemberUid = Exp_GetDWVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CChar * pMemberChar = pMemberUid.CharFind();
		if ( pMemberChar )
		{
			CStoneMember * pMemberGuild = GetMember( pMemberChar );
			if ( pMemberGuild )
			{
				return pMemberGuild->r_WriteVal(pszCmd, sVal, pSrc);
			}
		}

		return true;
	}
	else if ( !strnicmp("guild.",pszKey,6) )
	{
		lpctstr pszCmd = pszKey + 6;

		if ( !strnicmp("COUNT",pszCmd,5) )
		{
			pszCmd = pszCmd + 5;

			int i = 0;
			CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());

			if ( *pszCmd )
			{
				SKIP_ARGSEP(pszCmd);
				int iToCheck = Exp_GetVal(pszCmd);

				for (; pMember != NULL; pMember = pMember->GetNext())
				{
					if ( pMember->GetLinkUID().IsChar() )
						continue;

					if ( ( iToCheck == 1 ) && ( pMember->GetWeDeclared() && !pMember->GetTheyDeclared() ) )
						i++;
					else if ( ( iToCheck == 2 ) && ( !pMember->GetWeDeclared() && pMember->GetTheyDeclared() ) )
						i++;
					else if ( ( iToCheck == 3 ) && ( pMember->GetWeDeclared() && pMember->GetTheyDeclared() ) )
						i++;
				}
			}
			else
			{
				for (; pMember != NULL; pMember = pMember->GetNext())
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

		CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
		sVal.FormatVal(0);

		for ( int i = 0 ; pMember != NULL; pMember = pMember->GetNext() )
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
	else if ( !strnicmp("guildfromuid.", pszKey, 13) )
	{
		lpctstr pszCmd = pszKey + 13;
		sVal.FormatVal(0);

		if ( !pszCmd[0] )
			return true;

		CUID pGuildUid = Exp_GetDWVal(pszCmd);
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
	else if ( !strnicmp(sm_szLoadKeys[STC_CHARTER], pszKey, 7) )
	{
		lpctstr pszCmd = pszKey + 7;
		uint i = ATOI(pszCmd);
		if ( i >= CountOf(m_sCharter))
			sVal = "";
		else
			sVal = m_sCharter[i];

		return true;
	}


	STC_TYPE iIndex = (STC_TYPE) FindTableSorted( pszKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );

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
		case STC_AbbreviationToggle:
			{
				CStoneMember * pMember = GetMember(pCharSrc);
				CVarDefCont * pResult = NULL;

				if ( pMember == NULL )
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
		case STC_AlignType:
			sVal = GetAlignName();
			return true;

		case STC_LoyalTo:
			{
				CStoneMember * pMember = GetMember(pCharSrc);
				CVarDefCont * pResult = NULL;

				if ( pMember == NULL )
				{
					pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_NONMEMBER");
				}
				else
				{
					CChar * pLoyalTo = pMember->GetLoyalToUID().CharFind();
					if ((pLoyalTo == NULL) || (pLoyalTo == pCharSrc ))
					{
						pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_YOURSELF");
					}
					else
					{
						sVal = pLoyalTo->GetName();
						return true;
					}
				}

				sVal = pResult ? pResult->GetValStr() : "";
			}
			return true;
	
		case STC_Master:
			{
				CChar * pMaster = GetMaster();
				sVal = (pMaster) ? pMaster->GetName() : g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_PENDVOTE");
			}
			return true;
	
		case STC_MasterGenderTitle:
			{
				CChar * pMaster = GetMaster();
				if ( pMaster == NULL )
					sVal = ""; // If no master (vote pending)
				else if ( pMaster->Char_GetDef()->IsFemale())
					sVal = g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_MASTERGENDERFEMALE");
				else
					sVal = g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_MASTERGENDERMALE");
			}
			return true;
	
		case STC_MasterTitle:
			{
				CStoneMember * pMember = GetMasterMember();
				sVal = (pMember) ? pMember->GetTitle() : "";
			}
			return true;
	
		case STC_MasterUid:
			{
				CChar * pMaster = GetMaster();
				if ( pMaster )
					sVal.FormatHex( (dword) pMaster->GetUID() );
				else
					sVal.FormatHex( (dword) 0 );
			}
			return true;
			
		default:
			return( CItem::r_WriteVal( pszKey, sVal, pSrc ));
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

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys)-1 );
	if ( index < 0 )
	{
		return ( CItem::r_Verb( s, pSrc ));
	}

	CChar * pCharSrc = pSrc->GetChar();
	CStoneMember * pMember = GetMember(pCharSrc);

	switch ( index )
	{
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
							pMember = static_cast <CStoneMember *>(GetHead());
							SKIP_ARGSEP(pszArgs);
							CScript script(pszArgs);
							script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// Index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
							script.m_iLineNum = s.m_iLineNum;						// Line in the script file where Key/Arg were read

							for (; pMember != NULL; pMember = pMember->GetNext())
							{
								if ( pMember->GetLinkUID().IsChar() )
									continue;

								if ( !iFlags )
									pMember->r_Verb(script, pSrc);
								else if ( ( iFlags == 1 ) && ( pMember->GetWeDeclared() && !pMember->GetTheyDeclared() ) )
									pMember->r_Verb(script, pSrc);
								else if ( ( iFlags == 2 ) && ( !pMember->GetWeDeclared() && pMember->GetTheyDeclared() ) )
									pMember->r_Verb(script, pSrc);
								else if ( ( iFlags == 3 ) && ( pMember->GetWeDeclared() && pMember->GetTheyDeclared() ) )
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
							pMember = static_cast <CStoneMember *>(GetHead());
							SKIP_ARGSEP(pszArgs);
							CScript script(s.GetKey(), pszArgs);
							script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// Index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
							script.m_iLineNum = s.m_iLineNum;						// Line in the script file where Key/Arg were read

							for (; pMember != NULL; pMember = pMember->GetNext())
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
				CUID pMemberUid = s.GetArgVal();
				CChar * pMemberChar = pMemberUid.CharFind();
				if ( pMemberChar )
					AddRecruit( pMemberChar, STONEPRIV_CANDIDATE );
			}
			break;
		case ISV_CHANGEALIGN:
			if ( s.HasArgs())
			{
				SetAlignType(static_cast<STONEALIGN_TYPE>(s.GetArgVal()));
				tchar *pszMsg = Str_GetTemp();
				sprintf(pszMsg, "%s is now a %s %s\n", GetName(), GetAlignName(), GetTypeName());
				Speak(pszMsg);
			}
			break;
		case ISV_DECLAREPEACE:
			if ( s.HasArgs())
			{
				CUID pMemberUid = s.GetArgVal();
				WeDeclarePeace(pMemberUid);
			}
			break;
		case ISV_DECLAREWAR:
			if ( s.HasArgs())
			{
				CUID pMemberUid = s.GetArgVal();
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
					int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, CountOf(piCmd));
					if ( iArgQty == 2 )
					{
						CUID pGuildUid = (uint)(piCmd[0]);
						bool bWeDeclared = (piCmd[1] != 0);
						CItem * pEnemyItem = pGuildUid.ItemFind();
						if ( pEnemyItem && (pEnemyItem->IsType(IT_STONE_GUILD) || pEnemyItem->IsType(IT_STONE_TOWN)) )
						{
							CStoneMember * pMemberGuild = GetMember( pEnemyItem );
							if ( !pMemberGuild )
								pMemberGuild = new CStoneMember(this, pGuildUid, STONEPRIV_ENEMY);

							if ( bWeDeclared )
								pMemberGuild->SetWeDeclared(true);
							else
								pMemberGuild->SetTheyDeclared(true);
						}
					}
				}
			} break;
		case ISV_JOINASMEMBER:
			if ( s.HasArgs())
			{
				CUID pMemberUid = s.GetArgVal();
				CChar * pMemberChar = pMemberUid.CharFind();
				if ( pMemberChar )
				{
					AddRecruit( pMemberChar, STONEPRIV_MEMBER );
				}
			}
			break;
		case ISV_RESIGN:
			if ( s.HasArgs())
			{
				CUID pMemberUid = s.GetArgVal();
				CChar * pMemberChar = pMemberUid.CharFind();
				if ( pMemberChar )
				{
					CStoneMember * pMemberGuild = GetMember( pMemberChar );
					if ( pMemberGuild )
						delete pMemberGuild;
				}
			}
			else
			{
				if ( pMember != NULL )
					delete pMember;
			}
			break;
		case ISV_TOGGLEABBREVIATION:
			{
				CUID pMemberUid = pMember->GetLinkUID();
				if ( s.HasArgs() )
					pMemberUid = s.GetArgVal();

				CChar * pMemberChar = pMemberUid.CharFind();
				if ( pMemberChar )
				{
					CStoneMember * pMemberGuild = GetMember( pMemberChar );
					if ( pMemberGuild )
					{
						pMemberGuild->ToggleAbbrev();
						pMemberChar->ResendTooltip();
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
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->GetPriv() == STONEPRIV_MASTER )
			return pMember;
	}
	return NULL;
}

bool CItemStone::IsPrivMember( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CItemStone::IsPrivMember");
	const CStoneMember * pMember = GetMember(pChar);
	if ( pMember == NULL )
		return false;
	return( pMember->IsPrivMember());
}

CChar * CItemStone::GetMaster() const
{
	ADDTOCALLSTACK("CItemStone::GetMaster");
	CStoneMember * pMember = GetMasterMember();
	if ( pMember == NULL )
		return NULL;
	return pMember->GetLinkUID().CharFind();
}

CStoneMember * CItemStone::GetMember( const CObjBase * pObj ) const
{
	ADDTOCALLSTACK("CItemStone::GetMember");
	// Get member info for this char/item (if it has member info)
	if (!pObj)
		return NULL;
	CUID otherUID = pObj->GetUID();
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->GetLinkUID() == otherUID )
			return pMember;
	}
	return NULL;
}

bool CItemStone::NoMembers() const
{
	ADDTOCALLSTACK("CItemStone::NoMembers");
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
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
		return NULL;
	}

	tchar * z = Str_GetTemp();
	const CItemStone * pStone = pChar->Guild_Find( GetMemoryType());
	if ( pStone && pStone != this )
	{
		sprintf(z, "%s appears to belong to %s. Must resign previous %s", pChar->GetName(), pStone->GetName(), GetTypeName());
		Speak(z);
		return NULL;
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
			sprintf(z, "%s is already %s %s.", pChar->GetName(), pMember->GetPrivName(), GetName());
			Speak(z);
			return NULL;
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

	sprintf(z, "%s is now %s %s", pChar->GetName(), pMember->GetPrivName(), GetName());
	Speak(z);
	return pMember;
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
	CStoneMember * pMaster = NULL;

	// Validate the items and Clear the votes field
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->GetPriv() == STONEPRIV_MASTER )
		{
			pMaster = pMember;	// find current master.
		}
		else if ( pMember->GetPriv() != STONEPRIV_MEMBER )
		{
			continue;
		}
		pMember->m_Member.m_iVoteTally = 0;
		iCountMembers++;
	}

	// Now tally the votes.
	pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( ! pMember->IsPrivMember())
			continue;

		CChar * pCharVote = pMember->GetLoyalToUID().CharFind();
		if ( pCharVote != NULL )
		{
			CStoneMember * pMemberVote = GetMember( pCharVote );
			if ( pMemberVote != NULL )
			{
				pMemberVote->m_Member.m_iVoteTally ++;
				continue;
			}
		}

		// not valid to vote for. change to self.
		pMember->SetLoyalTo(NULL);
		// Assume I voted for myself.
		pMember->m_Member.m_iVoteTally ++;
	}

	// Find who won.
	bool fTie = false;
	CStoneMember * pMemberHighest = NULL;
	pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( ! pMember->IsPrivMember())
			continue;
		if ( pMemberHighest == NULL )
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
		pMember = static_cast <CStoneMember *>(GetHead());
		for (; pMember != NULL; pMember = pMember->GetNext())
		{
			WeDeclarePeace(pMember->GetLinkUID(), true);
		}
	}
}

bool CItemStone::IsUniqueName( lpctstr pName ) // static
{
	ADDTOCALLSTACK("CItemStone::IsUniqueName");
	for ( size_t i = 0; i < g_World.m_Stones.GetCount(); i++ )
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

	if ( GetAmount()==0 || g_Serv.m_iExitFlag )	// no reason to elect new if the stone is dead.
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
				if ( pChar == NULL )
					break;
				if ( pChar->Guild_Find( GetMemoryType()) != this )
					break;
			}
			return true;
		case STONEPRIV_ENEMY:
			{
				CItemStone * pEnemyStone = dynamic_cast <CItemStone *>( pMember->GetLinkUID().ItemFind());
				if ( pEnemyStone == NULL )
					break;
				CStoneMember * pEnemyMember = pEnemyStone->GetMember(this);
				if ( pEnemyMember == NULL )
					break;
				if ( pMember->GetWeDeclared() && ! pEnemyMember->GetTheyDeclared())
					break;
				if ( pMember->GetTheyDeclared() && ! pEnemyMember->GetWeDeclared())
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
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	while ( pMember != NULL )
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
	if ( pStone == NULL )
		return false;

	CScriptTriggerArgs Args;
	Args.m_pO1 = const_cast<CItemStone *>(pStone);
	enum TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;

	if ( const_cast<CItemStone *>(this)->r_Call("f_stonesys_internal_isalliedwith", &g_Serv, &Args, NULL, &tr) )
	{
		if ( tr == TRIGRET_RET_FALSE )
		{
			return false;
		}
		else if ( tr == TRIGRET_RET_TRUE )
		{
			return true;
		}
	}

	return( GetAlignType() != STONEALIGN_STANDARD &&
		GetAlignType() == pStone->GetAlignType());
}

bool CItemStone::IsAtWarWith( const CItemStone * pEnemyStone ) const
{
	ADDTOCALLSTACK("CItemStone::IsAtWarWith");
	// Boths guild shave declared war on each other.

	if ( pEnemyStone == NULL )
		return false;

	CScriptTriggerArgs Args;
	Args.m_pO1 = const_cast<CItemStone *>(pEnemyStone);
	enum TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;

	if ( const_cast<CItemStone *>(this)->r_Call("f_stonesys_internal_isatwarwith", &g_Serv, &Args, NULL, &tr) )
	{
		if ( tr == TRIGRET_RET_FALSE )
		{
			return false;
		}
		else if ( tr == TRIGRET_RET_TRUE )
		{
			return true;
		}
	}

	// Just based on align type.
	if ( IsType(IT_STONE_GUILD) &&
		GetAlignType() != STONEALIGN_STANDARD &&
		pEnemyStone->GetAlignType() != STONEALIGN_STANDARD )
	{
		return( GetAlignType() != pEnemyStone->GetAlignType());
	}

	// we have declared or they declared.
	CStoneMember * pEnemyMember = GetMember(pEnemyStone);
	if (pEnemyMember) // Ok, we might be at war
	{
		if ( pEnemyMember->GetTheyDeclared() && pEnemyMember->GetWeDeclared())
			return true;
	}

	return false;

}

void CItemStone::AnnounceWar( const CItemStone * pEnemyStone, bool fWeDeclare, bool fWar )
{
	ADDTOCALLSTACK("CItemStone::AnnounceWar");
	// Announce we are at war or peace.

	ASSERT(pEnemyStone);

	bool fAtWar = IsAtWarWith(pEnemyStone);

	tchar *pszTemp = Str_GetTemp();
	int len = sprintf( pszTemp, (fWar) ? "%s %s declared war on %s." : "%s %s requested peace with %s.",
		(fWeDeclare) ? "You" : pEnemyStone->GetName(),
		(fWeDeclare) ? "have" : "has",
		(fWeDeclare) ? pEnemyStone->GetName() : "You" );

	if ( fAtWar )
	{
		sprintf( pszTemp+len, " War is ON!" );
	}
	else if ( fWar )
	{
		sprintf( pszTemp+len, " War is NOT yet on." );
	}
	else
	{
		sprintf( pszTemp+len, " War is OFF." );
	}

	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		CChar * pChar = pMember->GetLinkUID().CharFind();
		if ( pChar == NULL )
			continue;
		if ( ! pChar->IsClient())
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
		if ( pMember->GetWeDeclared())
			return true;
	}
	else // They haven't, make a record of this
	{
		pMember = new CStoneMember( this, pEnemyStone->GetUID(), STONEPRIV_ENEMY );
	}
	pMember->SetWeDeclared(true);

	// Now inform the other stone
	// See if they have already declared war on us
	CStoneMember * pEnemyMember = pEnemyStone->GetMember(this);
	if (!pEnemyMember) // Not yet it seems
		pEnemyMember = new CStoneMember( pEnemyStone, GetUID(), STONEPRIV_ENEMY );

	pEnemyMember->SetTheyDeclared(true);
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

	bool fPrevTheyDeclared = pEnemyMember->GetTheyDeclared();

	if (!pEnemyMember->GetWeDeclared() || fForcePeace) // If we're not at war with them, delete this record
		delete pEnemyMember;
	else
		pEnemyMember->SetTheyDeclared(false);

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
	if (!pMember->GetTheyDeclared() || fForcePeace) // If they're not at war with us, delete this record
		delete pMember;
	else
		pMember->SetWeDeclared(false);

	pEnemyStone->TheyDeclarePeace( this, fForcePeace );
}

void CItemStone::SetTownName()
{
	ADDTOCALLSTACK("CItemStone::SetTownName");
	// For town stones.
	if ( ! IsTopLevel())
		return;
	CRegionBase * pArea = GetTopPoint().GetRegion(( IsType(IT_STONE_TOWN)) ? REGION_TYPE_AREA : REGION_TYPE_ROOM );
	if ( pArea )
	{
		pArea->SetName( GetIndividualName());
	}
}

bool CItemStone::MoveTo(CPointMap pt, bool bForceFix)
{
	ADDTOCALLSTACK("CItemStone::MoveTo");
	// Put item on the ground here.
	if ( IsType(IT_STONE_TOWN) )
	{
		SetTownName();
	}
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
