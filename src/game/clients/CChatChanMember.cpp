
#include "../CServer.h"
#include "CChatChannel.h"
#include "CChatChanMember.h"
#include "CClient.h"

CChatChanMember::CChatChanMember()
{
    m_fChatActive = false;
    m_pChannel = nullptr;
    m_fReceiving = true;
    m_fAllowWhoIs = true;
}

CChatChanMember::~CChatChanMember()
{
    if ( IsChatActive()) // Are we chatting currently ?
    {
        g_Serv.m_Chats.QuitChat(this);
    }
}

CChatChannel * CChatChanMember::GetChannel() const
{
    return m_pChannel;
}

void CChatChanMember::SetChannel(CChatChannel * pChannel)
{
    m_pChannel = pChannel;
}

bool CChatChanMember::IsChatActive() const
{
    return m_fChatActive;
}

void CChatChanMember::SetReceiving(bool fOnOff)
{
    if (m_fReceiving != fOnOff)
        ToggleReceiving();
}

void CChatChanMember::addChatWindow()
{
    ADDTOCALLSTACK("CChatMember::addChatWindow");
    // Called from Event_ChatButton

    CClient* pClient = GetClientActive();
    if (!pClient || (!pClient->m_fUseNewChatSystem && IsChatActive()))
        return;

    // Open chat window (old chat system only)
    // On new chat system this is not needed because the chat button is hardcoded on client-side, and
    // PacketChatButton packet is sent by client after login complete only to get initial channel list
    if (!pClient->m_fUseNewChatSystem)
        pClient->addChatSystemMessage(CHATCMD_OpenChatWindow, pClient->m_fUseNewChatSystem ? nullptr : GetChatName());

    // Send channel names
    for (auto const& pChannel : g_Serv.m_Chats.m_Channels)
    {
        pClient->addChatSystemMessage(CHATCMD_AddChannel, pChannel->m_sName, pClient->m_fUseNewChatSystem ? nullptr : pChannel->GetPassword());
        if ((g_Cfg.m_iChatFlags & CHATF_AUTOJOIN) && pChannel->m_fStatic && !GetChannel())
            g_Serv.m_Chats.JoinChannel(pChannel->m_sName, nullptr, this);
    }
}

size_t CChatChanMember::FindIgnoringIndex(lpctstr pszName) const
{
    ADDTOCALLSTACK("CChatChanMember::FindIgnoringIndex");
    for ( size_t i = 0; i < m_IgnoredMembers.size(); i++)
    {
        if (m_IgnoredMembers[i]->Compare(pszName) == 0)
            return i;
    }
    return sl::scont_bad_index();
}

void CChatChanMember::Ignore(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChanMember::Ignore");
    if (!IsIgnoring(pszName))
        ToggleIgnore(pszName);
    else
        SendChatMsg(CHATMSG_AlreadyIgnoringPlayer, pszName);
}

void CChatChanMember::DontIgnore(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChanMember::DontIgnore");
    if (IsIgnoring(pszName))
        ToggleIgnore(pszName);
    else
        SendChatMsg(CHATMSG_NotIgnoring, pszName);
}

void CChatChanMember::ToggleIgnore(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChanMember::ToggleIgnore");
    size_t i = FindIgnoringIndex( pszName );
    if ( i != sl::scont_bad_index() )
    {
        ASSERT( m_IgnoredMembers.IsValidIndex(i) );
        m_IgnoredMembers.erase_at(i);

        SendChatMsg(CHATMSG_NoLongerIgnoring, pszName);

        // Resend the un ignored member to the client's local list of members (but only if they are currently in the same channel!)
        if (m_pChannel)
        {
            CChatChanMember * pMember = m_pChannel->FindMember(pszName);
            if (pMember)
                m_pChannel->SendMember(pMember, this);
        }
    }
    else
    {
        CSString * name = new CSString(pszName);
        m_IgnoredMembers.push_back(name);
        SendChatMsg(CHATMSG_NowIgnoring, pszName); // This message also takes the ignored person off the clients local list of channel members
    }
}

void CChatChanMember::ClearIgnoreList()
{
    ADDTOCALLSTACK("CChatChanMember::ClearIgnoreList");
    for (size_t i = 0; i < m_IgnoredMembers.size(); ++i)
    {
        m_IgnoredMembers.erase_at(i);
    }
    SendChatMsg(CHATMSG_NoLongerIgnoringAnyone);
}

void CChatChanMember::RenameChannel(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChanMember::RenameChannel");
    CChatChannel * pChannel = GetChannel();
    if (!pChannel)
        SendChatMsg(CHATMSG_MustBeInAConference);
    else if (!pChannel->IsModerator(pszName))
        SendChatMsg(CHATMSG_MustHaveOps);
    else
        pChannel->RenameChannel(this, pszName);
}

void CChatChanMember::SendChatMsg(CHATMSG_TYPE iType, lpctstr pszName1, lpctstr pszName2, CLanguageID lang )
{
    ADDTOCALLSTACK("CChatChanMember::SendChatMsg");
    GetClientActive()->addChatSystemMessage(iType, pszName1, pszName2, lang );
}

void CChatChanMember::ToggleReceiving()
{
    ADDTOCALLSTACK("CChatChanMember::ToggleReceiving");
    m_fReceiving = !m_fReceiving;
    GetClientActive()->addChatSystemMessage((m_fReceiving) ? CHATMSG_ReceivingPrivate : CHATMSG_NoLongerReceivingPrivate);
}

void CChatChanMember::PermitWhoIs()
{
    ADDTOCALLSTACK("CChatChanMember::PermitWhoIs");
    if (GetWhoIs())
        return;
    SetWhoIs(true);
    SendChatMsg(CHATMSG_ShowingName);
}

void CChatChanMember::ForbidWhoIs()
{
    ADDTOCALLSTACK("CChatChanMember::ForbidWhoIs");
    if (!GetWhoIs())
        return;
    SetWhoIs(false);
    SendChatMsg(CHATMSG_NotShowingName);
}

void CChatChanMember::ToggleWhoIs()
{
    ADDTOCALLSTACK("CChatChanMember::ToggleWhoIs");
    SetWhoIs(!GetWhoIs());
    SendChatMsg((GetWhoIs() == true) ? CHATMSG_ShowingName : CHATMSG_NotShowingName);
}

CClient * CChatChanMember::GetClientActive()
{
    ADDTOCALLSTACK("CChatChanMember::GetClientActive");
    return( static_cast <CClient*>( this ));
}

const CClient * CChatChanMember::GetClientActive() const
{
    ADDTOCALLSTACK("CChatChanMember::GetClientActive");
    return( static_cast <const CClient*>( this ));
}

lpctstr CChatChanMember::GetChatName() const
{
    ADDTOCALLSTACK("CChatChanMember::GetChatName");
    const CClient *pClient = GetClientActive();

    if (pClient)
        return(pClient->GetAccount()->m_sChatName);
    return "";
}

bool CChatChanMember::IsIgnoring(lpctstr pszName) const
{
    ADDTOCALLSTACK("CChatChanMember::IsIgnoring");
    return( FindIgnoringIndex(pszName) != sl::scont_bad_index() );
}

void CChatChanMember::HideCharacterName()
{
    ADDTOCALLSTACK("CChatMember::HideCharacterName");
    if (!m_fAllowWhoIs)
        return;

    m_fAllowWhoIs = false;
    SendChatMsg(CHATMSG_NotShowingName);
}

void CChatChanMember::ToggleCharacterName()
{
    ADDTOCALLSTACK("CChatMember::ToggleCharacterName");
    m_fAllowWhoIs = !m_fAllowWhoIs;
    SendChatMsg(m_fAllowWhoIs ? CHATMSG_ShowingName : CHATMSG_NotShowingName);
}

void CChatChanMember::ShowCharacterName()
{
    ADDTOCALLSTACK("CChatMember::ShowCharacterName");
    if (m_fAllowWhoIs)
        return;

    m_fAllowWhoIs = true;
    SendChatMsg(CHATMSG_ShowingName);
}

void CChatChanMember::AddIgnore(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatMember::AddIgnore");
    if (IsIgnoring(pszName))
        SendChatMsg(CHATMSG_AlreadyIgnoringPlayer, pszName);
    else
        ToggleIgnore(pszName);
}

void CChatChanMember::RemoveIgnore(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatMember::RemoveIgnore");
    if (!IsIgnoring(pszName))
        SendChatMsg(CHATMSG_NotIgnoring, pszName);
    else
        ToggleIgnore(pszName);
}
