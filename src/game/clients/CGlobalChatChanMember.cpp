
#include "../CServer.h"
#include "CClient.h"
#include "CGlobalChatChanMember.h"

CGlobalChatChanMember::CGlobalChatChanMember() noexcept :
    m_fVisible(false)
{
}


void CGlobalChatChanMember::SetJID(lpctstr pszJID)
{
	m_strJID = pszJID;
}

lpctstr CGlobalChatChanMember::GetJID() const
{
	return m_strJID.GetBuffer();
}

void CGlobalChatChanMember::SetVisible(bool fNewStatus)
{
	m_fVisible = fNewStatus;
}

bool CGlobalChatChanMember::IsVisible() const
{
	return m_fVisible;
}
