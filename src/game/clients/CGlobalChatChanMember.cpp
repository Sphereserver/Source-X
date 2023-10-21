
#include "../CServer.h"
#include "CClient.h"
#include "CGlobalChatChanMember.h"

CGlobalChatChanMember::CGlobalChatChanMember()
{
	m_pszJID = Str_GetTemp();
	m_fVisible = false;
}

CGlobalChatChanMember::~CGlobalChatChanMember()
{
}

void CGlobalChatChanMember::SetJID(lpctstr pszJID)
{
	m_pszJID = pszJID;
}

lpctstr CGlobalChatChanMember::GetJID() const
{
	return m_pszJID;
}

void CGlobalChatChanMember::SetVisible(bool fNewStatus)
{
	m_fVisible = fNewStatus;
}

bool CGlobalChatChanMember::IsVisible() const
{
	return m_fVisible;
}