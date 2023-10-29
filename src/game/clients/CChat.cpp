#include "../../network/CClientIterator.h"
#include "../chars/CChar.h"
#include "../CServer.h"
#include "CChat.h"
#include "CChatChannel.h"
#include "CChatChanMember.h"
#include "CClient.h"

bool CChat::CreateChannel(lpctstr pszName, lpctstr pszPassword, CChatChanMember* pMember)
{
	ADDTOCALLSTACK("CChat::CreateChannel");

	if (pMember)
	{
		CClient* pClient = pMember->GetClientActive();
		ASSERT(pClient);
		ASSERT(pClient->GetChar());
		if (!pClient->IsPriv(PRIV_GM) && !(g_Cfg.m_iChatFlags & CHATF_CHANNELCREATION))
		{
			CSString sName;
			FormatName(sName, nullptr, true);
			pMember->SendChatMsg(CHATMSG_PlayerTalk, sName, " Channel creation is disabled.");
			return false;
		}

		if (!IsValidName(pszName, false))
		{
			pMember->SendChatMsg(CHATMSG_InvalidConferenceName);
			return false;
		}
		else if (g_Serv.m_Chats.FindChannel(pszName))
		{
			pMember->SendChatMsg(CHATMSG_AlreadyAConference);
			return false;
		}
	}

	auto &pChannel = m_Channels.emplace_back(std::make_unique<CChatChannel>(pszName, pszPassword, !pMember));
	if (pMember && (g_Cfg.m_iChatFlags & CHATF_CHANNELMODERATION))
		pChannel->SetModerator(pMember->GetChatName());
	BroadcastAddChannel(pChannel.get());
	return true;
}

bool CChat::IsChannel(const CChatChannel* pChannel) const
{
	ADDTOCALLSTACK("CChat::IsChannel");
	return m_Channels.has_ptr(pChannel);
}

void CChat::DeleteChannel(CChatChannel* pChannel)
{
	ADDTOCALLSTACK("CChat::DeleteChannel");

	if (pChannel->m_fStatic)
		return;

	BroadcastRemoveChannel(pChannel);
	m_Channels.erase_element(pChannel);
}

void CChat::JoinChannel(lpctstr pszChannel, lpctstr pszPassword, CChatChanMember* pMember)
{
	ADDTOCALLSTACK("CChat::JoinChannel");

	ASSERT(pMember);
	CClient* pMemberClient = pMember->GetClientActive();
	ASSERT(pMemberClient);

	CChatChannel* pNewChannel = FindChannel(pszChannel);
	if (!pNewChannel)
	{
		pMemberClient->addChatSystemMessage(CHATMSG_NoConference, pszChannel);
		return;
	}
	pszChannel = pNewChannel->GetName();

	CChatChannel* pCurrentChannel = pMember->GetChannel();
	if (pCurrentChannel && (pCurrentChannel == pNewChannel))
	{
		pMember->SendChatMsg(CHATMSG_AlreadyInConference, pszChannel);
		return;
	}

	if (!pNewChannel->m_sPassword.IsEmpty() && (!pszPassword || (strcmp(pNewChannel->GetPassword(), pszPassword) != 0)))
	{
		if (pMemberClient->m_fUseNewChatSystem)
		{
			CSString sName;
			FormatName(sName, nullptr, true);
			pMember->SendChatMsg(CHATMSG_PlayerTalk, sName, " Your client version can't join channels with password.");
		}
		else
			pMemberClient->addChatSystemMessage(CHATMSG_IncorrectPassword);
		return;
	}

	// Leave current channel (if any)
	if (pCurrentChannel)
		pCurrentChannel->RemoveMember(pMember);

	// Join the new channel
	pNewChannel->AddMember(pMember);
	pNewChannel->SendMember(pMember);
	pMemberClient->addChatSystemMessage(CHATCMD_JoinedChannel, pszChannel);
	if (!pMemberClient->m_fUseNewChatSystem)
		pNewChannel->FillMembersList(pMember);	// fill the members list on this client
}

void CChat::Action(CClient* pClient, const nachar* pszText, int len, CLanguageID lang)
{
	ADDTOCALLSTACK("CChat::Action");
	// ARGS:
	//  len = length of the pszText string in NCHAR's.

	if (!(g_Cfg.m_iFeatureT2A & FEATURE_T2A_CHAT))
		return;

	CChatChanMember* pMe = pClient;
	CChatChannel* pChannel = pMe->GetChannel();

	tchar szFullText[MAX_TALK_BUFFER];
	CvtNETUTF16ToSystem(szFullText, sizeof(szFullText), pszText, len);

	tchar* szMsg = szFullText + 1;
	switch (szFullText[0])	// the 1st character is a command byte (join channel, leave channel, etc)
	{
	case CHATACT_ChangeChannelPassword:		// client shortcut: /pw
	{
		if (!pChannel)
			goto NoConference;

		pChannel->ChangePassword(pMe, szMsg);
		break;
	}
	case CHATACT_LeaveChannel:
	{
		if (!pChannel)
			goto NoConference;

		pChannel->RemoveMember(pMe);
		break;
	}
	case CHATACT_LeaveChat:
	{
		if (pChannel)
			pChannel->RemoveMember(pMe);
		break;
	}
	case CHATACT_ChannelMessage:
	{
		if (pChannel)
		{
			pChannel->MemberTalk(pMe, pClient->m_fUseNewChatSystem ? szFullText : szMsg, lang);
			break;
		}
	NoConference:
		pMe->SendChatMsg(CHATMSG_MustBeInAConference);
		return;
	}
	case CHATACT_JoinChannel:				// client shortcut: /conf
	{
		// Look for second double quote to separate channel from password
		size_t i = 1;
		for (; szMsg[i] != '\0'; ++i)
		{
			if (szMsg[i] == '"')
				break;
		}
		szMsg[i] = '\0';
		tchar* pszPassword = szMsg + i + 1;
		if (pszPassword[0] == ' ')	// skip whitespaces
			pszPassword += 1;
		JoinChannel(szMsg + 1, pszPassword, pMe);
		break;
	}
	case CHATACT_CreateChannel:				// client shortcut: /newconf
	{
		tchar* pszPassword = nullptr;
		size_t iMsgLength = strlen(szMsg);
		for (size_t i = 0; i < iMsgLength; ++i)
		{
			if (szMsg[i] == '{')	// there's a password here
			{
				szMsg[i] = 0;
				pszPassword = szMsg + i + 1;
				size_t iPasswordLength = strlen(pszPassword);
				for (i = 0; i < iPasswordLength; ++i)
				{
					if (pszPassword[i] == '}')
					{
						pszPassword[i] = '\0';
						break;
					}
				}
				break;
			}
		}
		if (CreateChannel(szMsg, pszPassword, pMe))
			JoinChannel(szMsg, pszPassword, pMe);
		break;
	}
	case CHATACT_RenameChannel:				// client shortcut: /rename
	{
		if (!pChannel)
			goto NoConference;

		pMe->RenameChannel(szMsg);
		break;
	}
	case CHATACT_PrivateMessage:			// client shortcut: /msg
	{
		if (!pChannel)
			goto NoConference;

		// Split the recipient from the message (look for a space)
		tchar buffer[2048];
		strcpy(buffer, szMsg);
		size_t bufferLength = strlen(buffer);
		size_t i = 0;
		for (; i < bufferLength; ++i)
		{
			if (buffer[i] == ' ')
			{
				buffer[i] = '\0';
				break;
			}
		}
		pChannel->PrivateMessage(pMe, buffer, buffer + i + 1, lang);
		break;
	}
	case CHATACT_AddIgnore:					// client shortcut: +ignore
	{
		pMe->AddIgnore(szMsg);
		break;
	}
	case CHATACT_RemoveIgnore:				// client shortcut: -ignore
	{
		pMe->RemoveIgnore(szMsg);
		break;
	}
	case CHATACT_ToggleIgnore:				// client shortcut: /ignore
	{
		pMe->ToggleIgnore(szMsg);
		break;
	}
	case CHATACT_AddVoice:					// client shortcut: +voice
	{
		if (!pChannel)
			goto NoConference;

		pChannel->AddVoice(pMe, szMsg);
		break;
	}
	case CHATACT_RemoveVoice:				// client shortcut: -voice
	{
		if (!pChannel)
			goto NoConference;

		pChannel->RemoveVoice(pMe, szMsg);
		break;
	}
	case CHATACT_ToggleVoice:				// client shortcut: /voice
	{
		if (!pChannel)
			goto NoConference;

		pChannel->ToggleVoice(pMe, szMsg);
		break;
	}
	case CHATACT_AddModerator:				// client shortcut: +ops
	{
		if (!pChannel)
			goto NoConference;

		pChannel->GrantModerator(pMe, szMsg);
		break;
	}
	case CHATACT_RemoveModerator:			// client shortcut: -ops
	{
		if (!pChannel)
			goto NoConference;

		pChannel->RevokeModerator(pMe, szMsg);
		break;
	}
	case CHATACT_ToggleModerator:			// client shortcut: /ops
	{
		if (!pChannel)
			goto NoConference;

		pChannel->ToggleModerator(pMe, szMsg);
		break;
	}
	case CHATACT_EnablePrivateMessages:		// client shortcut: +receive
	{
		pMe->SetReceiving(true);
		break;
	}
	case CHATACT_DisablePrivateMessages:	// client shortcut: -receive
	{
		pMe->SetReceiving(false);
		break;
	}
	case CHATACT_TogglePrivateMessages:		// client shortcut: /receive
	{
		pMe->ToggleReceiving();
		break;
	}
	case CHATACT_ShowCharacterName:			// client shortcut: +showname
	{
		pMe->ShowCharacterName();
		break;
	}
	case CHATACT_HideCharacterName:			// client shortcut: -showname
	{
		pMe->HideCharacterName();
		break;
	}
	case CHATACT_ToggleCharacterName:		// client shortcut: /showname
	{
		pMe->ToggleCharacterName();
		break;
	}
	case CHATACT_WhoIs:						// client shortcut: /whois
	{
		if (!pChannel)
			goto NoConference;

		pChannel->WhoIs(pMe->GetChatName(), szMsg);
		break;
	}
	case CHATACT_Kick:						// client shortcut: /kick
	{
		if (!pChannel)
			goto NoConference;

		CChatChanMember* pMember = pChannel->FindMember(szMsg);
		if (pMember)
			pChannel->KickMember(pMe, pMember);
		else
			pMe->SendChatMsg(CHATMSG_NoPlayer, szMsg);
		break;
	}
	case CHATACT_EnableDefaultVoice:		// client shortcut: +defaultvoice
	{
		if (!pChannel)
			goto NoConference;

		pChannel->EnableDefaultVoice(pMe->GetChatName());
		break;
	}
	case CHATACT_DisableDefaultVoice:		// client shortcut: -defaultvoice
	{
		if (!pChannel)
			goto NoConference;

		pChannel->DisableDefaultVoice(pMe->GetChatName());
		break;
	}
	case CHATACT_ToggleDefaultVoice:		// client shortcut: /defaultvoice
	{
		if (!pChannel)
			goto NoConference;

		pChannel->ToggleDefaultVoice(pMe->GetChatName());
		break;
	}
	case CHATACT_EmoteMessage:				// client shortcut: /emote or /em
	{
		if (!pChannel)
			goto NoConference;

		pChannel->Emote(pMe->GetChatName(), szMsg, lang);
		break;
	}
	}
}

void CChat::QuitChat(CChatChanMember* pClient)
{
	ADDTOCALLSTACK("CChat::QuitChat");
	// Remove from old channel (if any)

	CChatChannel* pCurrentChannel = pClient->GetChannel();
	if (pCurrentChannel)
		pCurrentChannel->RemoveMember(pClient);
}

void CChat::FormatName(CSString& sName, const CChatChanMember* pMember, bool fSystem)
{
	ADDTOCALLSTACK("CChat::FormatName");
	// Format chat name with proper color
	// 0 = Yellow (user)
	// 1 = Purple (moderator)
	// 2 = Blue (muted)
	// 3 = Purple (unused?)
	// 4 = White (me)
	// 5 = Green (system)

	int iColor = 0;
	if (pMember)
	{
		CChatChannel* pChannel = pMember->GetChannel();
		if (pChannel)
		{
			lpctstr pszName = pMember->GetChatName();
			if (pChannel->IsModerator(pszName))
				iColor = 1;
			else if (!pChannel->HasVoice(pszName))
				iColor = 2;

			sName.Format("%d%s", iColor, pszName);
			return;
		}
	}

	iColor = fSystem ? 5 : 4;
	sName.Format("%d%s", iColor, "SYSTEM");
}

bool CChat::IsValidName( lpctstr pszName, bool fPlayer ) // static
{
	ADDTOCALLSTACK("CChat::IsValidName");

	// Channels can have spaces, but not player names
	if ((strlen(pszName) < 1) || g_Cfg.IsObscene(pszName) || (strcmpi(pszName, "SYSTEM") == 0))
		return false;

	size_t length = strlen(pszName);
	for (size_t i = 0; i < length; i++)
	{
		if ( pszName[i] == ' ' )
		{
			if (fPlayer)
				return false;
			continue;
		}
		if ( ! isalnum(pszName[i]))
			return false;
	}
	return true;
}

void CChat::Broadcast(CChatChanMember *pFrom, lpctstr pszText, CLanguageID lang, bool fOverride)
{
	ADDTOCALLSTACK("CChat::Broadcast");

	CSString sName;
	FormatName(sName, pFrom, fOverride);

	ClientIterator it;
	for (CClient *pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if (!pClient->IsChatActive())
			continue;
		if (fOverride || pClient->IsReceivingAllowed())
		{
			pClient->SendChatMsg(CHATMSG_PlayerTalk, sName, pszText, lang);
		}
	}
}

void CChat::BroadcastAddChannel(CChatChannel* pChannel)
{
	ADDTOCALLSTACK("CChat::BroadcastAddChannel");
	// Send 'add channel' message to all clients

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
		pClient->addChatSystemMessage(CHATCMD_AddChannel, pChannel->GetName(), pClient->m_fUseNewChatSystem ? nullptr : pChannel->GetPassword());
}

void CChat::BroadcastRemoveChannel(CChatChannel* pChannel)
{
	ADDTOCALLSTACK("CChat::BroadcastRemoveChannel");
	// Send 'delete channel' message to all clients

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
		pClient->addChatSystemMessage(CHATCMD_RemoveChannel, pChannel->GetName());
}

CChatChannel * CChat::FindChannel(lpctstr pszChannel) const
{
	for (auto const& pChannel : m_Channels)
	{
		if (strcmp(pChannel->GetName(), pszChannel) == 0)
			return pChannel.get();
	}
	return nullptr;
}


