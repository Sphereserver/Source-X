
#include "../../common/CException.h"
#include "../chars/CChar.h"
#include "../CWorld.h"
#include "../CWorldGameTime.h"
#include "CClient.h"
#include "CGMPage.h"

//////////////////////////////////////////////////////////////////
// -CGMPage

CGMPage::CGMPage( lpctstr pszAccount )
{
	m_pClientHandling = nullptr;
	m_sAccount = pszAccount;
	m_sReason = nullptr;
	m_time = CWorldGameTime::GetCurrentTime().GetTimeRaw();

	g_World.m_GMPages.InsertContentTail(this);		// put it at the end of the list
}

CGMPage::~CGMPage()
{
	ClearHandler();
}

void CGMPage::SetHandler(CClient* pClient)
{
	ADDTOCALLSTACK("CGMPage::SetHandler");
	if (!pClient || (pClient == m_pClientHandling))
		return;

	if (m_pClientHandling)
		ClearHandler();

	m_pClientHandling = pClient;
	m_pClientHandling->m_pGMPage = this;
}

void CGMPage::ClearHandler()
{
	ADDTOCALLSTACK("CGMPage::ClearHandler");
	if (m_pClientHandling)
	{
		m_pClientHandling->m_pGMPage = nullptr;
		m_pClientHandling = nullptr;
	}
}

void CGMPage::r_Write(CScript& s) const
{
	ADDTOCALLSTACK_INTENSIVE("CGMPage::r_Write");
	s.WriteSection("GMPAGE %s", GetName());
	s.WriteKeyHex("CHARUID", m_uidChar.GetObjUID());
	s.WriteKeyStr("P", m_pt.WriteUsed());
	s.WriteKeyStr("REASON", m_sReason.GetBuffer());
	s.WriteKeyVal("TIME", CWorldGameTime::GetCurrentTime().GetTimeDiff(m_time) / MSECS_PER_SEC);
}

enum GC_TYPE
{
	GC_ACCOUNT,
	GC_CHARUID,
	GC_DELETE,
	GC_HANDLED,
	GC_P,
	GC_REASON,
	GC_TIME,
	GC_QTY
};

lpctstr const CGMPage::sm_szLoadKeys[GC_QTY + 1] =
{
	"ACCOUNT",
	"CHARUID",
	"DELETE",
	"HANDLED",
	"P",
	"REASON",
	"TIME",
	nullptr
};


bool CGMPage::r_WriteVal(lpctstr pszKey, CSString &sVal, CTextConsole *pSrc, bool fNoCallParent, bool fNoCallChildren)
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
    UNREFERENCED_PARAMETER(fNoCallParent);
	ADDTOCALLSTACK("CGMPage::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1) )
	{
		case GC_ACCOUNT:
			sVal = GetName();
			break;
		case GC_CHARUID:
			sVal.FormatHex(m_uidChar);
			break;
		case GC_HANDLED:
			sVal.FormatHex((m_pClientHandling && m_pClientHandling->GetChar()) ? (dword)m_pClientHandling->GetChar()->GetUID() : (dword)0 );
			break;
		case GC_P:
			sVal = m_pt.WriteUsed();
			break;
		case GC_REASON:
			sVal = m_sReason;
			break;
		case GC_TIME:
			sVal.FormatLLVal(CWorldGameTime::GetCurrentTime().GetTimeDiff(m_time) / MSECS_PER_SEC);
			break;
		default:
			return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	// EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}


bool CGMPage::r_LoadVal(CScript& s)
{
	ADDTOCALLSTACK("CGMPage::r_LoadVal");
	EXC_TRY("LoadVal");
	switch (FindTableSorted(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1))
	{
	case GC_CHARUID:
		m_uidChar.SetObjUID(s.GetArgDWVal());
		break;
	case GC_DELETE:
		delete this;
		break;
	case GC_HANDLED:
	{
		CChar* pChar = CUID::CharFindFromUID(s.GetArgDWVal());
		if (pChar && pChar->IsClientActive())
			SetHandler(pChar->GetClientActive());
		else
			ClearHandler();
		break;
	}
	case GC_P:
		m_pt.Read(s.GetArgStr());
		break;
	case GC_REASON:
		m_sReason = s.GetArgStr();
		break;
	case GC_TIME:
		m_time = CWorldGameTime::GetCurrentTime() - (s.GetArgLLVal() * MSECS_PER_SEC);
		break;
	default:
		return CScriptObj::r_LoadVal(s);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
