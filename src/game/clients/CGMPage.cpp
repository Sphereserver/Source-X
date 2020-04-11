
#include "../../common/CException.h"
#include "../chars/CChar.h"
#include "../CWorld.h"
#include "../CWorldGameTime.h"
#include "CClient.h"
#include "CGMPage.h"

//////////////////////////////////////////////////////////////////
// -CGMPage

CGMPage::CGMPage( lpctstr pszAccount ) :
	m_sAccount( pszAccount )
{
	m_pGMClient = nullptr;
	m_timePage = CWorldGameTime::GetCurrentTime().GetTimeRaw();
	// Put at the end of the list.
	g_World.m_GMPages.InsertContentTail( this );
}

CGMPage::~CGMPage()
{
	ClearGMHandler();
}

lpctstr CGMPage::GetName() const
{
	return( m_sAccount );
}

lpctstr CGMPage::GetReason() const
{
	return( m_sReason );
}

void CGMPage::SetReason( lpctstr pszReason )
{
	m_sReason = pszReason;
}

CClient * CGMPage::FindGMHandler() const
{
	return( m_pGMClient );
}

int64 CGMPage::GetAge() const
{
	ADDTOCALLSTACK("CGMPage::GetAge");
	// How old in seconds.
	return (CWorldGameTime::GetCurrentTime().GetTimeDiff( m_timePage ) / MSECS_PER_SEC);
}

void CGMPage::ClearGMHandler()
{
	if ( m_pGMClient != nullptr)	// break the link to the client.
	{
		ASSERT( m_pGMClient->m_pGMPage == this );
		m_pGMClient->m_pGMPage = nullptr;
	}

	m_pGMClient = nullptr;
}

void CGMPage::SetGMHandler(CClient* pClient)
{
	if ( m_pGMClient == pClient )
		return;

	if ( m_pGMClient != nullptr)	// break the link to the previous client.
		ClearGMHandler();

	m_pGMClient = pClient;
	if ( m_pGMClient != nullptr )
		m_pGMClient->m_pGMPage = this;
}

void CGMPage::r_Write( CScript & s ) const
{
	ADDTOCALLSTACK_INTENSIVE("CGMPage::r_Write");
	s.WriteSection( "GMPAGE %s", GetName());
	s.WriteKey( "REASON", GetReason());
	s.WriteKeyHex( "TIME", GetAge());
	s.WriteKey( "P", m_ptOrigin.WriteUsed());
}

enum GC_TYPE
{
	GC_ACCOUNT,
	GC_P,
	GC_REASON,
	GC_STATUS,
	GC_TIME,
	GC_QTY
};

lpctstr const CGMPage::sm_szLoadKeys[GC_QTY+1] =
{
	"ACCOUNT",
	"P",
	"REASON",
	"STATUS",
	"TIME",
	nullptr
};

bool CGMPage::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
	ADDTOCALLSTACK("CGMPage::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 ))
	{
	case GC_ACCOUNT:
		sVal = GetName();
		break;
	case GC_P:	// "P"
		sVal = m_ptOrigin.WriteUsed();
		break;
	case GC_REASON:	// "REASON"
		sVal = GetReason();
		break;
	case GC_STATUS:
		sVal = GetAccountStatus();
		break;
	case GC_TIME:	// "TIME"
		sVal.FormatLLHex( GetAge() );
		break;
	default:
		return (fNoCallParent ? false : CScriptObj::r_WriteVal( ptcKey, sVal, pSrc, false ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CGMPage::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CGMPage::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 ))
	{
	case GC_P:	// "P"
		m_ptOrigin.Read( s.GetArgStr());
		break;
	case GC_REASON:	// "REASON"
		SetReason( s.GetArgStr());
		break;
	case GC_TIME:	// "TIME"
		m_timePage = CWorldGameTime::GetCurrentTime().GetTimeRaw() - ( s.GetArgLLVal() * MSECS_PER_SEC);
		break;
	default:
		return( CScriptObj::r_LoadVal( s ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

CAccount * CGMPage::FindAccount() const
{
	ADDTOCALLSTACK("CGMPage::FindAccount");
	return( g_Accounts.Account_Find( m_sAccount ));
}

lpctstr CGMPage::GetAccountStatus() const
{
	ADDTOCALLSTACK("CGMPage::GetAccountStatus");
	CClient * pClient = FindAccount()->FindClient();
	if ( pClient==nullptr )
		return "OFFLINE";
	else if ( pClient->GetChar() == nullptr )
		return "LOGIN";
	else
		return pClient->GetChar()->GetName();
}

CGMPage * CGMPage::GetNext() const
{
	return( static_cast <CGMPage*>( CSObjListRec::GetNext()));
}
