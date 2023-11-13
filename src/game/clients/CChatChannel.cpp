
#include "../chars/CChar.h"
#include "../CServer.h"
#include "CChatChannel.h"
#include "CChatChanMember.h"
#include "CClient.h"


CChatChannel::CChatChannel(lpctstr pszName, lpctstr pszPassword, bool fStatic)
{
    m_sName = pszName;
    m_sPassword = pszPassword;
    m_fVoiceDefault = true;
    m_fStatic = fStatic;
}

lpctstr CChatChannel::GetName() const
{
    return m_sName;
}

lpctstr CChatChannel::GetModeString() const
{
    // (client needs this) "0" = not passworded, "1" = passworded
    return (IsPassworded() ? "1" : "0");
}

lpctstr CChatChannel::GetPassword() const
{
    return m_sPassword;
}

void CChatChannel::SetPassword( lpctstr pszPassword)
{
    m_sPassword = pszPassword;
}

bool CChatChannel::IsPassworded() const
{
    return !m_sPassword.IsEmpty();
}

void CChatChannel::WhoIs(lpctstr pszBy, lpctstr pszMember)
{
    ADDTOCALLSTACK("CChatChannel::WhoIs");
    CChatChanMember * pBy = FindMember(pszBy);
    CChatChanMember * pMember = FindMember(pszMember);
    CChar * pChar = pMember? pMember->GetClientActive()->GetChar() : nullptr;
    if (!pMember||!pChar)
    {
        pBy->SendChatMsg(CHATMSG_NoPlayer, pszMember);
    }
    else if (pMember->GetWhoIs())
    {
        pBy->SendChatMsg(CHATMSG_PlayerKnownAs, pszMember, pChar->GetName());
    }
    else
    {
        pBy->SendChatMsg(CHATMSG_PlayerIsAnonymous, pszMember);
    }
}

void CChatChannel::Emote(lpctstr pszBy, lpctstr pszMsg, CLanguageID lang )
{
    ADDTOCALLSTACK("CChatChannel::Emote");
    if (HasVoice(pszBy))
        Broadcast(CHATMSG_PlayerEmote, pszBy, pszMsg, lang );
    else
        FindMember(pszBy)->SendChatMsg(CHATMSG_RevokedSpeaking);
}

void CChatChannel::ToggleVoiceDefault(lpctstr  pszBy)
{
    ADDTOCALLSTACK("CChatChannel::ToggleVoiceDefault");
    if (!IsModerator(pszBy))
    {
        FindMember(pszBy)->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }
    if (GetVoiceDefault())
        Broadcast(CHATMSG_ModeratorsSpeakDefault, "", "");
    else
        Broadcast(CHATMSG_SpeakingByDefault, "", "");
    SetVoiceDefault(!GetVoiceDefault());
}

void CChatChannel::DisableVoiceDefault(lpctstr  pszBy)
{
    ADDTOCALLSTACK("CChatChannel::DisableVoiceDefault");
    if (GetVoiceDefault())
        ToggleVoiceDefault(pszBy);
}

void CChatChannel::EnableVoiceDefault(lpctstr  pszBy)
{
    ADDTOCALLSTACK("CChatChannel::EnableVoiceDefault");
    if (!GetVoiceDefault())
        ToggleVoiceDefault(pszBy);
}

void CChatChannel::SendPrivateMessage(CChatChanMember * pFrom, lpctstr pszTo, lpctstr pszMsg)
{
    ADDTOCALLSTACK("CChatChannel::SendPrivateMessage");
    CChatChanMember * pTo = FindMember(pszTo);
    if (!pTo)
    {
        pFrom->SendChatMsg(CHATMSG_NoPlayer, pszTo);
        return;
    }
    if (!pTo->IsReceivingAllowed())
    {
        pFrom->SendChatMsg(CHATMSG_PlayerNotReceivingPrivate, pszTo);
        return;
    }
    // Can always send private messages to moderators (but only if they are receiving)
    bool fHasVoice = HasVoice(pFrom->GetChatName());
    if ( !fHasVoice && !IsModerator(pszTo))
    {
        pFrom->SendChatMsg(CHATMSG_RevokedSpeaking);
        return;
    }

    if (pTo->IsIgnoring(pFrom->GetChatName())) // See if ignoring you
    {
        pFrom->SendChatMsg(CHATMSG_PlayerIsIgnoring, pszTo);
        return;
    }

    CSString sName;
    g_Serv.m_Chats.FormatName(sName, pFrom);
    // Echo to the sending client so they know the message went out
    pFrom->SendChatMsg(CHATMSG_PlayerPrivate, sName, pszMsg);
    // If the sending and receiving are different send it out to the receiver
    if (pTo != pFrom)
        pTo->SendChatMsg(CHATMSG_PlayerPrivate, sName, pszMsg);
}

void CChatChannel::RenameChannel(CChatChanMember* pBy, lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::RenameChannel");

    if (!g_Serv.m_Chats.IsValidName(pszName, false))
    {
        pBy->SendChatMsg(CHATMSG_InvalidConferenceName);
        return;
    }
    if (g_Serv.m_Chats.FindChannel(pszName))
    {
        pBy->SendChatMsg(CHATMSG_AlreadyAConference);
        return;
    }

    Broadcast(CHATMSG_ConferenceRenamed, GetName(), pszName);
    g_Serv.m_Chats.BroadcastRemoveChannel(this);
    m_sName = pszName;

    Broadcast(CHATCMD_JoinedChannel, pszName);
    g_Serv.m_Chats.BroadcastAddChannel(this);
}

void CChatChannel::SendMember(CChatChanMember* pMember, CChatChanMember* pToMember)
{
    ADDTOCALLSTACK("CChatChannel::SendMember");
    CSString sName;
    g_Serv.m_Chats.FormatName(sName, pMember);

    CClient* pClient = nullptr;
    if (pToMember)
    {
        // If pToMember is specified, send only to this member
        pClient = pToMember->GetClientActive();
        if (pClient && pClient->m_fUseNewChatSystem)
            return;
        if (pToMember->IsIgnoring(pMember->GetChatName()))
            return;
        pToMember->SendChatMsg(CHATCMD_AddMemberToChannel, sName);
    }
    else
    {
        // If no pToMember is specified, send to all members
        for (size_t i = 0; i < m_Members.size(); ++i)
        {
            pClient = m_Members[i]->GetClientActive();
            if (pClient && pClient->m_fUseNewChatSystem)
                continue;
            if (m_Members[i]->IsIgnoring(pMember->GetChatName()))
                continue;
            m_Members[i]->SendChatMsg(CHATCMD_AddMemberToChannel, sName);
        }
    }
}

void CChatChannel::RemoveMember(CChatChanMember * pMember)
{
    ADDTOCALLSTACK("CChatChannel::RemoveMember");

    for ( size_t i = 0; i < m_Members.size(); )
    {
        // Tell the other clients in this channel (if any) you are leaving (including yourself)
        CClient * pClient = m_Members[i]->GetClientActive();

        if ( pClient == nullptr )		//	auto-remove offline clients
        {
            m_Members[i]->SetChannel(nullptr);
            m_Members.erase(m_Members.begin() + i);
            continue;
        }

        if (!pClient->m_fUseNewChatSystem)
            pClient->addChatSystemMessage(CHATCMD_RemoveMemberFromChannel, pMember->GetChatName());

        if (m_Members[i] == pMember)	// disjoin
        {
            pClient->addChatSystemMessage(pClient->m_fUseNewChatSystem ? CHATCMD_LeftChannel : CHATCMD_ClearMembers, static_cast<lpctstr>(m_sName));
            m_Members.erase(m_Members.begin() + i);
            break;
        }

        ++i;
    }

    // Delete the channel if there's no members left
    if (m_Members.size() <= 0)
        g_Serv.m_Chats.DeleteChannel(this);

    // Update our persona
    pMember->SetChannel(nullptr);
}

CChatChanMember* CChatChannel::FindMember(lpctstr pszName) const
{
    ADDTOCALLSTACK("CChatChannel::FindMember");
    size_t i = FindMemberIndex( pszName );
    if ( i == sl::scont_bad_index() )
        return nullptr;

    return m_Members[i].get();
}

bool CChatChannel::RemoveMemberByName(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::RemoveMemberByName");
    CChatChanMember* pMember = FindMember(pszName);
    if ( pMember == nullptr )
        return false;

    RemoveMember(pMember);
    return true;
}

void CChatChannel::SetName(lpctstr pszName)
{
    m_sName = pszName;
}


bool CChatChannel::IsModerator(lpctstr pszMember) const
{
    ADDTOCALLSTACK("CChatChannel::IsModerator");
    for (size_t i = 0; i < m_Moderators.size(); ++i)
    {
        if (m_Moderators[i]->CompareNoCase(pszMember) == 0)
            return true;
    }
    return false;
}

bool CChatChannel::HasVoice(lpctstr pszMember) const
{
    ADDTOCALLSTACK("CChatChannel::HasVoice");
    for (size_t i = 0; i < m_NoVoices.size(); ++i)
    {
        if (m_NoVoices[i]->Compare(pszMember) == 0)
            return false;
    }
    return true;
}

void CChatChannel::SetModerator(lpctstr pszMember, bool fFlag)
{
    ADDTOCALLSTACK("CChatChannel::SetModerator");
    // See if they are already a moderator
    for (size_t i = 0; i < m_Moderators.size(); ++i)
    {
        if (m_Moderators[i]->CompareNoCase(pszMember) == 0)
        {
            if (fFlag == false)
                m_Moderators.erase_index(i);
            return;
        }
    }
    if (fFlag)
    {
        m_Moderators.emplace_back(std::make_unique<CSString>(pszMember));
    }
}

void CChatChannel::KickMember(CChatChanMember* pByMember, CChatChanMember* pMember)
{
    ADDTOCALLSTACK("CChatChannel::KickMember");
    ASSERT(pMember);

    lpctstr pszByName = "SYSTEM";
    if (pByMember) // If nullptr, then an ADMIN or a GM did it
    {
        pszByName = pByMember->GetChatName();
        if (!IsModerator(pszByName))
        {
            pByMember->SendChatMsg(CHATMSG_MustHaveOps);
            return;
        }
    }

    lpctstr pszName = pMember->GetChatName();
    // Kicking this person...remove from list of moderators first
    if (IsModerator(pszName))
    {
        SetModerator(pszName, false);
        SendMember(pMember);
        Broadcast(CHATMSG_PlayerNoLongerModerator, pszName, "");
        pMember->SendChatMsg(CHATMSG_RemovedListModerators, pszByName);
    }

    // Remove them from the channels list of members
    RemoveMember(pMember);
    if (!g_Serv.m_Chats.IsChannel(this)) // The channel got removed because there's no members left
        return;

    pMember->SendChatMsg(CHATMSG_ModeratorHasKicked, pszByName);
    pMember->SendChatMsg(CHATCMD_ClearMembers);
    Broadcast(CHATMSG_PlayerIsKicked, pszName);
}

void CChatChannel::AddMember(CChatChanMember * pMember)
{
    ADDTOCALLSTACK("CChatChannel::AddMember");
    pMember->SetChannel(this);
    m_Members.emplace_back(pMember);
    // See if only moderators have a voice by default
    lpctstr pszName = pMember->GetChatName();
    if (!IsModerator(pszName))
    {
        if (!GetVoiceDefault())
            SetVoice(pszName);

        // GMs always have moderation privs
        CClient* pClient = pMember->GetClientActive();
        if (pClient && pClient->IsPriv(PRIV_GM))
            SetModerator(pszName);
    }
}

void CChatChannel::SetVoice(lpctstr pszName, bool fFlag)
{
    ADDTOCALLSTACK("CChatChannel::SetVoice");
    // See if they have no voice already
    for (size_t i = 0; i < m_NoVoices.size(); ++i)
    {
        if (m_NoVoices[i]->Compare(pszName) == 0)
        {
            if (fFlag == true)
                m_NoVoices.erase_index(i);
            return;
        }
    }
    if (fFlag == false)
    {
        m_NoVoices.emplace_back(std::make_unique<CSString>(pszName));
        return;
    }
}

void CChatChannel::MemberTalk(CChatChanMember * pByMember, lpctstr pszText, CLanguageID lang )
{
    ADDTOCALLSTACK("CChatChannel::MemberTalk");
    // Do I have a voice?
    if (!HasVoice(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_RevokedSpeaking);
        return;
    }
    Broadcast(CHATMSG_PlayerTalk, pByMember->GetChatName(), pszText, lang );
}

void CChatChannel::ChangePassword(CChatChanMember * pByMember, lpctstr pszPassword)
{
    ADDTOCALLSTACK("CChatChannel::ChangePassword");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->GetClientActive()->addChatSystemMessage(CHATMSG_MustHaveOps);
    }
    else
    {
        SetPassword(pszPassword);
        g_Serv.m_Chats.BroadcastAddChannel(pByMember->GetChannel());
        Broadcast(CHATMSG_PasswordChanged, "","");
    }
}

void CChatChannel::Broadcast(CHATMSG_TYPE iType, lpctstr pszName, lpctstr pszText, CLanguageID lang, bool fOverride )
{
    ADDTOCALLSTACK("CChatChannel::Broadcast");
    CSString sName;
    CChatChanMember *pSendingMember = FindMember(pszName);

    if (iType >= CHATMSG_PlayerTalk && iType <= CHATMSG_PlayerPrivate) // Only chat, emote, and privates get a color status number
        g_Serv.m_Chats.FormatName(sName, pSendingMember, fOverride);
    else
        sName = pszName;

    for (size_t i = 0; i < m_Members.size(); i++)
    {
        // Check to see if the recipient is ignoring messages from the sender
        // Just pass over it if it's a regular talk message
        if (!m_Members[i]->IsIgnoring(pszName))
        {
            m_Members[i]->SendChatMsg(iType, sName, pszText, lang );
        }

            // If it's a private message, then tell the sender the recipient is ignoring them
        else if (iType == CHATMSG_PlayerPrivate)
        {
            pSendingMember->SendChatMsg(CHATMSG_PlayerIsIgnoring, m_Members[i]->GetChatName());
        }
    }
}

void CChatChannel::GrantVoice(CChatChanMember * pByMember, lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::GrantVoice");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }
    CChatChanMember * pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    if (HasVoice(pszName))
        return;
    SetVoice(pszName, true);
    SendMember(pMember); // Update the color
    pMember->SendChatMsg(CHATMSG_ModeratorGrantSpeaking, pByMember->GetChatName());
    Broadcast(CHATMSG_PlayerNowSpeaking, pszName, "", "");
}

void CChatChannel::RevokeVoice(CChatChanMember * pByMember, lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::RevokeVoice");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }
    CChatChanMember * pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    if (!HasVoice(pszName))
        return;
    SetVoice(pszName, false);
    SendMember(pMember); // Update the color
    pMember->SendChatMsg(CHATMSG_ModeratorRemovedSpeaking, pByMember->GetChatName());
    Broadcast(CHATMSG_PlayerNoSpeaking, pszName, "", "");
}

void CChatChannel::ToggleVoice(CChatChanMember * pByMember, lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::ToggleVoice");
    if (!HasVoice(pszName)) // (This also returns true if this person is not in the channel)
        GrantVoice(pByMember, pszName); // this checks and reports on membership
    else
        RevokeVoice(pByMember, pszName); // this checks and reports on membership
}

size_t CChatChannel::FindMemberIndex(lpctstr pszName) const
{
    ADDTOCALLSTACK("CChatChannel::FindMemberIndex");
    for (size_t i = 0; i < m_Members.size(); ++i)
    {
        if ( strcmp( m_Members[i]->GetChatName(), pszName) == 0)
            return i;
    }
    return sl::scont_bad_index();
}

void CChatChannel::GrantModerator(CChatChanMember * pByMember, lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::GrantModerator");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }

    CChatChanMember * pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    if (IsModerator(pMember->GetChatName()))
        return;

    SetModerator(pszName, true);
    SendMember(pMember); // Update the color
    Broadcast(CHATMSG_PlayerIsAModerator, pMember->GetChatName(), "", "");
    pMember->SendChatMsg(CHATMSG_YouAreAModerator, pByMember->GetChatName());
}

void CChatChannel::RevokeModerator(CChatChanMember * pByMember, lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::RevokeModerator");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }

    CChatChanMember * pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    if (!IsModerator(pMember->GetChatName()))
        return;

    SetModerator(pszName, false);
    SendMember(pMember); // Update the color
    Broadcast(CHATMSG_PlayerNoLongerModerator, pMember->GetChatName(), "", "");
    pMember->SendChatMsg(CHATMSG_RemovedListModerators, pByMember->GetChatName());
}

void CChatChannel::ToggleModerator(CChatChanMember * pByMember, lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::ToggleModerator");
    if (!IsModerator(pszName))
        GrantModerator(pByMember, pszName);
    else
        RevokeModerator(pByMember, pszName);
}

bool CChatChannel::GetVoiceDefault()  const
{
    return m_fVoiceDefault;
}

void CChatChannel::SetVoiceDefault(bool fVoiceDefault)
{
    m_fVoiceDefault = fVoiceDefault;
}

void CChatChannel::FillMembersList(CChatChanMember* pMember)
{
    ADDTOCALLSTACK("CChatChannel::FillMembersList");
    for (size_t i = 0; i < m_Members.size(); ++i)
    {
        if (pMember->IsIgnoring(m_Members[i]->GetChatName()))
            continue;

        CSString sName;
        g_Serv.m_Chats.FormatName(sName, m_Members[i].get());
        pMember->SendChatMsg(CHATCMD_AddMemberToChannel, sName);
    }
}

void CChatChannel::PrivateMessage(CChatChanMember* pFrom, lpctstr pszTo, lpctstr pszMsg, CLanguageID lang)
{
    ADDTOCALLSTACK("CChatChannel::PrivateMessage");
    CChatChanMember* pTo = FindMember(pszTo);
    if (!pTo)
    {
        pFrom->SendChatMsg(CHATMSG_NoPlayer, pszTo);
        return;
    }
    if (!pTo->IsReceivingAllowed())
    {
        pFrom->SendChatMsg(CHATMSG_PlayerNotReceivingPrivate, pszTo);
        return;
    }

    // Members without voice can't send private messages on channel, but they still allowed to send private messages to moderators
    if (!HasVoice(pFrom->GetChatName()) && !IsModerator(pszTo))
    {
        pFrom->SendChatMsg(CHATMSG_RevokedSpeaking);
        return;
    }

    if (pTo->IsIgnoring(pFrom->GetChatName()))
    {
        pFrom->SendChatMsg(CHATMSG_PlayerIsIgnoring, pszTo);
        return;
    }

    CSString sName;
    g_Serv.m_Chats.FormatName(sName, pFrom);
    pFrom->SendChatMsg(CHATMSG_PlayerPrivate, sName, pszMsg, lang);
    if (pTo != pFrom)
        pTo->SendChatMsg(CHATMSG_PlayerPrivate, sName, pszMsg, lang);
}

void CChatChannel::AddVoice(CChatChanMember* pByMember, lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::AddVoice");

    lpctstr pszByName = pByMember->GetChatName();
    if (!IsModerator(pszByName))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }

    CChatChanMember* pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    pszName = pMember->GetChatName();	// fix case-sensitive mismatch
    if (HasVoice(pszName))
        return;

    SetVoice(pszName);
    SendMember(pMember);	// update name color
    pMember->SendChatMsg(CHATMSG_ModeratorGrantSpeaking, pszByName);
    Broadcast(CHATMSG_PlayerNowSpeaking, pszName);
}

void CChatChannel::RemoveVoice(CChatChanMember* pByMember, lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::RemoveVoice");

    lpctstr pszByName = pByMember->GetChatName();
    if (!IsModerator(pszByName))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }

    CChatChanMember* pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    pszName = pMember->GetChatName();	// fix case-sensitive mismatch
    if (!HasVoice(pszName))
        return;

    SetVoice(pszName, true);
    SendMember(pMember);	// update name color
    pMember->SendChatMsg(CHATMSG_ModeratorRemovedSpeaking, pszByName);
    Broadcast(CHATMSG_PlayerNoSpeaking, pszName);
}

void CChatChannel::EnableDefaultVoice(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::EnableDefaultVoice");
    if (!GetVoiceDefault())
        ToggleDefaultVoice(pszName);
}

void CChatChannel::DisableDefaultVoice(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::DisableDefaultVoice");
    if (GetVoiceDefault())
        ToggleDefaultVoice(pszName);
}

void CChatChannel::ToggleDefaultVoice(lpctstr pszName)
{
    ADDTOCALLSTACK("CChatChannel::ToggleDefaultVoice");
    if (!IsModerator(pszName))
    {
        FindMember(pszName)->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }
    m_fVoiceDefault = !m_fVoiceDefault;
    Broadcast(m_fVoiceDefault ? CHATMSG_SpeakingByDefault : CHATMSG_ModeratorsSpeakDefault);
}
