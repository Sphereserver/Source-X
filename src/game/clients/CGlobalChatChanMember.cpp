
#include "CGlobalChatChanMember.h"

CGlobalChatChanMember::CGlobalChatChanMember() noexcept :
    m_fVisible(false)
{
}

CGlobalChatChanMember::~CGlobalChatChanMember() noexcept = default;

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
