#include "../../common/CException.h"
#include "../../common/CLog.h"
#include "../chars/CChar.h"
#include "../CServer.h"
#include "CStoneMember.h"


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
	nullptr
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
	nullptr
};

CStoneMember* CStoneMember::GetNext() const
{
	ADDTOCALLSTACK("CStoneMember::GetNext");
	return static_cast <CStoneMember *>( CSObjListRec::GetNext() );
}

CUID CStoneMember::GetLinkUID() const
{
	return m_uidLinkTo;
}

STONEPRIV_TYPE CStoneMember::GetPriv() const
{
	//ADDTOCALLSTACK("CStoneMember::GetPriv");
	return m_iPriv;
}
void CStoneMember::SetPriv(STONEPRIV_TYPE iPriv)
{
	//ADDTOCALLSTACK("CStoneMember::SetPriv");
	m_iPriv = iPriv;
}
bool CStoneMember::IsPrivMaster() const
{
	//ADDTOCALLSTACK("CStoneMember::IsPrivMaster");
	return (m_iPriv == STONEPRIV_MASTER);
}
bool CStoneMember::IsPrivMember() const
{
	//ADDTOCALLSTACK("CStoneMember::IsPrivMember");
	return ( (m_iPriv == STONEPRIV_MASTER) || (m_iPriv == STONEPRIV_MEMBER) );
}

// If the member is really a alliance flag (STONEPRIV_ALLY)
void CStoneMember::SetWeDeclaredAlly(bool f)
{
	//ADDTOCALLSTACK("CStoneMember::SetWeDeclaredAlly");
	m_Ally.m_fWeDeclared = f;
}
bool CStoneMember::GetWeDeclaredAlly() const
{
	//ADDTOCALLSTACK("CStoneMember::GetWeDeclaredAlly");
	return m_Ally.m_fWeDeclared ? true : false;
}
void CStoneMember::SetTheyDeclaredAlly(bool f)
{
	//ADDTOCALLSTACK("CStoneMember::SetTheyDeclaredAlly");
	m_Ally.m_fTheyDeclared = f;
}
bool CStoneMember::GetTheyDeclaredAlly() const
{
	//ADDTOCALLSTACK("CStoneMember::GetTheyDeclaredAlly");
	return m_Ally.m_fTheyDeclared ? true : false;
}

// If the member is really a war flag (STONEPRIV_ENEMY)
void CStoneMember::SetWeDeclaredWar(bool f)
{
	//ADDTOCALLSTACK("CStoneMember::SetWeDeclaredWar");
	m_Enemy.m_fWeDeclared = f;
}
bool CStoneMember::GetWeDeclaredWar() const
{
	//ADDTOCALLSTACK("CStoneMember::GetWeDeclaredWar");
	return ( m_Enemy.m_fWeDeclared ) ? true : false;
}
void CStoneMember::SetTheyDeclaredWar(bool f)
{
	//ADDTOCALLSTACK("CStoneMember::SetTheyDeclaredWar");
	m_Enemy.m_fTheyDeclared = f;
}
bool CStoneMember::GetTheyDeclaredWar() const
{
	//ADDTOCALLSTACK("CStoneMember::GetTheyDeclaredWar");
	return ( m_Enemy.m_fTheyDeclared ) ? true : false;
}

bool CStoneMember::IsAbbrevOn() const
{
	//ADDTOCALLSTACK("CStoneMember::IsAbbrevOn");
	return ( m_Member.m_fAbbrev ) ? true : false;
}
void CStoneMember::ToggleAbbrev()
{
	//ADDTOCALLSTACK("CStoneMember::ToggleAbbrev");
	m_Member.m_fAbbrev = !m_Member.m_fAbbrev;
}
void CStoneMember::SetAbbrev(bool mode)
{
	//ADDTOCALLSTACK("CStoneMember::SetAbbrev");
	m_Member.m_fAbbrev = mode;
}

lpctstr CStoneMember::GetTitle() const
{
	//ADDTOCALLSTACK("CStoneMember::GetTitle");
	return m_sTitle;
}
void CStoneMember::SetTitle( lpctstr pTitle )
{
	//ADDTOCALLSTACK("CStoneMember::SetTitle");
	m_sTitle = pTitle;
}

CUID CStoneMember::GetLoyalToUID() const
{
	//ADDTOCALLSTACK("CStoneMember::GetLoyalToUID");
	return m_UIDLoyalTo;
}

int CStoneMember::GetAccountGold() const
{
	//ADDTOCALLSTACK("CStoneMember::GetAccountGold");
	return m_Member.m_iAccountGold;
}
void CStoneMember::SetAccountGold( int iGold )
{
	//ADDTOCALLSTACK("CStoneMember::SetAccountGold");
	m_Member.m_iAccountGold = iGold;
}

bool CStoneMember::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	UnreferencedParameter(ptcKey);
	UnreferencedParameter(pRef);
	return false;
}

bool CStoneMember::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CStoneMember::r_Verb");
	EXC_TRY("Verb");

	ASSERT(pSrc);

	lpctstr ptcKey = s.GetKey();
	int index = FindTableSorted( ptcKey, sm_szVerbKeys, ARRAY_COUNT(sm_szVerbKeys)-1 );
	if ( index < 0 )
	{
		if ( r_LoadVal(s) ) // if it's successful all ok, else go on verb.
			return true;
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

	STMM_TYPE iIndex = (STMM_TYPE) FindTableSorted( s.GetKey(), sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );

	if ( GetLinkUID().IsChar() )
	{
		switch ( iIndex )
		{
			case STMM_ACCOUNTGOLD:
				SetAccountGold(s.GetArgVal());
				break;

			case STMM_LOYALTO:
				SetLoyalTo( CUID(s.GetArgVal()).CharFind() );
			break;

			case STMM_PRIV:
				SetPriv( (STONEPRIV_TYPE)s.GetArgVal() );
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
				SetTheyDeclaredAlly(s.GetArgVal() ? true : false);
				break;

			case STMM_GUILD_THEYWAR:
				SetTheyDeclaredWar(s.GetArgVal() ? true : false);
				break;

			case STMM_GUILD_WEALLIANCE:
				SetWeDeclaredAlly(s.GetArgVal() ? true : false);
				break;

			case STMM_GUILD_WEWAR:
				SetWeDeclaredWar(s.GetArgVal() ? true : false);
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


bool CStoneMember::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallChildren);
	ADDTOCALLSTACK("CStoneMember::r_WriteVal");
	EXC_TRY("WriteVal");

	STMM_TYPE iIndex = (STMM_TYPE) FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );

	if ( GetLinkUID().IsChar() )
	{
		switch ( iIndex )
		{
			case STMM_ISCANDIDATE:
				sVal.FormatVal((GetPriv() == STONEPRIV_CANDIDATE) ? 1 : 0);
				break;
			case STMM_ISMASTER:
				sVal.FormatVal(IsPrivMaster());
				break;
			case STMM_ISMEMBER:
				sVal.FormatVal(IsPrivMember());
				break;
			case STMM_ACCOUNTGOLD:
				sVal.FormatVal(GetAccountGold());
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
			default:
                if (!fNoCallParent)
			    {
				    CScriptObj *pRef = GetLinkUID().CharFind();
				    return pRef->r_WriteVal(ptcKey,sVal,pSrc);
			    }
		}
	}
	else if ( GetLinkUID().IsItem() )
	{
		switch ( iIndex )
		{
			case STMM_GUILD_ISALLY:
				sVal.FormatVal(GetWeDeclaredAlly() && GetTheyDeclaredAlly());
				break;
			case STMM_GUILD_ISENEMY:
				sVal.FormatVal(GetWeDeclaredWar() && GetTheyDeclaredWar());
				break;
			case STMM_GUILD_THEYALLIANCE:
				sVal.FormatVal(GetTheyDeclaredAlly());
				break;
			case STMM_GUILD_THEYWAR:
				sVal.FormatVal(GetTheyDeclaredWar());
				break;
			case STMM_GUILD_WEALLIANCE:
				sVal.FormatVal(GetWeDeclaredAlly());
				break;
			case STMM_GUILD_WEWAR:
				sVal.FormatVal(GetWeDeclaredWar());
				break;
			default:
                if (!fNoCallParent)
			    {
				    CScriptObj *pRef = GetLinkUID().ItemFind();
				    return pRef->r_WriteVal(ptcKey,sVal,pSrc);
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
	m_UIDLoyalTo = loyaluid;

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
		if ( pChar != nullptr )
		{
			pChar->Memory_AddObjTypes(pStone, (word)(pStone->GetMemoryType()));
			if ( pStone->IsTopLevel())
			{
				pChar->m_ptHome = pStone->GetTopPoint();	// Our new home.
			}
		}
	}

	pStone->InsertContentTail( this );
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
		if ( pStoneEnemy != nullptr )
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

	TemporaryString tsDefname;
	snprintf(tsDefname.buffer(), tsDefname.capacity(), "STONECONFIG_PRIVNAME_PRIVID-%d", (int)iPriv);

	CVarDefCont * pResult = g_Exp.m_VarDefs.GetKey(tsDefname);
	if (pResult)
		return pResult->GetValStr();
	else
		pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_PRIVNAME_PRIVUNK");

	return ( pResult == nullptr ) ? "" : pResult->GetValStr();
}

bool CStoneMember::SetLoyalTo( const CChar * pCharLoyal )
{
	ADDTOCALLSTACK("CStoneMember::SetLoyalTo");
	CChar * pCharMe = GetLinkUID().CharFind();
	if ( pCharMe == nullptr )	// on shutdown
		return false;

	m_UIDLoyalTo = GetLinkUID();	// set to self for default.
	if ( pCharLoyal == nullptr )
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

	CStoneMember * pNewLOYALTO = pStone->GetMember(pCharLoyal);
	if ( pNewLOYALTO == nullptr || ! pNewLOYALTO->IsPrivMember())
	{
		// you can't vote for candidates
		pCharMe->SysMessage( "Can only vote for full members.");
		return false;
	}

	m_UIDLoyalTo = pCharLoyal->GetUID();

	// new vote tally
	pStone->ElectMaster();
	return true;
}
