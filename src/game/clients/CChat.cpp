#include "../../network/CClientIterator.h"
#include "../chars/CChar.h"
#include "CChat.h"
#include "CChatChannel.h"
#include "CChatChanMember.h"
#include "CClient.h"


void CChat::EventMsg( CClient * pClient, const nachar * pszText, int len, CLanguageID lang ) // Text from a client
{
	ADDTOCALLSTACK("CChat::EventMsg");
	// ARGS:
	//  len = length of the pszText string in nchar's.
	//

	CChatChanMember * pMe = pClient;
	ASSERT(pMe);

	// newer clients do not send the 'chat button' packet, leading to all kinds of problems
	// with the client not being initialised properly for chat (e.g. blank name and exceptions
	// when leaving a chat room) - if chat is not active then we must simulate the chat button
	// event before processing the chat message
	if (pMe->IsChatActive() == false)
	{
		// simulate the chat button being clicked
		pClient->Event_ChatButton(nullptr);

		// if chat isn't active now then cancel processing the event
		if (pMe->IsChatActive() == false)
			return;
	}

	CChatChannel * pChannel =  pMe->GetChannel();

	tchar szText[MAX_TALK_BUFFER * 2];
	CvtNETUTF16ToSystem( szText, sizeof(szText), pszText, len );

	// The 1st character is a command byte, join channel, private message someone, etc etc
	tchar * szMsg = szText+1;
	switch ( szText[0] )
	{
	case 'a':	// a = client typed a plain message in the text entry area
	{
		// Check for a chat command here
		if (szMsg[0] == '/')
		{
			DoCommand(pMe, szMsg + 1);
			break;
		}
		if (!pChannel)
		{
not_in_a_channel:
			pMe->SendChatMsg(CHATMSG_MustBeInAConference);
			return;
		}
		// Not a chat command, must be speech
		pChannel->MemberTalk(pMe, szMsg, lang);
		break;
	};
	case 'A':	// A = change the channel password
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->ChangePassword(pMe, szMsg);
		break;
	};
	case 'b':	// b = client joining an existing channel
	{
		// Look for second double quote to separate channel from password
		size_t i = 1;
		for (; szMsg[i] != '\0'; i++)
			if (szMsg[i] == '"')
				break;
		szMsg[i] = '\0';
		tchar * pszPassword = szMsg + i + 1;
		if (pszPassword[0] == ' ') // skip leading space if any
			pszPassword++;
		JoinChannel( pMe, szMsg + 1, pszPassword);
		break;
	};
	case 'c':	// c = client creating (and joining) new channel
	{
		tchar * pszPassword = nullptr;
		size_t iMsgLength = strlen(szMsg);
		for (size_t i = 0; i < iMsgLength; i++)
		{
			if (szMsg[i] == '{') // OK, there's a password here
			{
				szMsg[i] = 0;
				pszPassword = szMsg + i + 1;
				size_t iPasswordLength = strlen(pszPassword);
				for (i = 0; i < iPasswordLength; i++)
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
		CreateJoinChannel(pMe, szMsg, pszPassword);
		break;
	};
	case 'd':	// d = (/rename x) rename conference
	{
		if (!pChannel)
			goto not_in_a_channel;
		pMe->RenameChannel(szMsg);
		break;
	};
	case 'e':	// e = Send a private message to ....
	{
		if (!pChannel)
			goto not_in_a_channel;
		tchar buffer[2048];
		Str_CopyLimitNull(buffer, szMsg, sizeof(buffer));
		// Separate the recipient from the message (look for a space)
		size_t i = 0;
		size_t bufferLength = strlen(buffer);
		for (; i < bufferLength; i++)
		{
			if (buffer[i] == ' ')
			{
				buffer[i] = '\0';
				break;
			}
		}
		pChannel->SendPrivateMessage(pMe, buffer, buffer+i+1);
		break;
	};
	case 'f':	// f = (+ignore) ignore this person
	{
		pMe->Ignore(szMsg);
		break;
	};
	case 'g':	// g = (-ignore) don't ignore this person
	{
		pMe->DontIgnore(szMsg);
		break;
	};
	case 'h':	// h = toggle ignoring this person
	{
		pMe->ToggleIgnore(szMsg);
		break;
	};
	case 'i':	// i = grant speaking privs to this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->GrantVoice(pMe, szMsg);
		break;
	};
	case 'j':	// j = remove speaking privs from this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->RevokeVoice(pMe, szMsg);
		break;
	};
	case 'k':	// k = (/voice) toggle voice status
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->ToggleVoice(pMe, szMsg);
		break;
	};
	case 'l':	// l = grant moderator status to this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->GrantModerator(pMe, szMsg);
		break;
	};
	case 'm':	// m = remove moderator status from this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->RevokeModerator(pMe, szMsg);
		break;
	};
	case 'n':	// m = toggle the moderator status for this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->ToggleModerator(pMe, szMsg);
		break;
	}
	case 'o':	// o = turn on receiving private messages
	{
		pMe->SetReceiving(true);
		break;
	}
	case 'p':	// p = turn off receiving private messages
	{
		pMe->SetReceiving(false);
		break;
	}
	case 'q':	// q = toggle receiving messages
	{
		pMe->ToggleReceiving();
		break;
	};
	case 'r':	// r = (+showname) turn on showing character name
	{
		pMe->PermitWhoIs();
		break;
	};
	case 's':	// s = (-showname) turn off showing character name
	{
		pMe->ForbidWhoIs();
		break;
	};
	case 't':	// t = toggle showing character name
	{
		pMe->ToggleWhoIs();
		break;
	};
	case 'u':	// u = who is this player
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->WhoIs(pMe->GetChatName(), szMsg);
		break;
	};
	case 'v':	// v = kick this person out of the conference
	{
		if (!pChannel)
			goto not_in_a_channel;

		CChatChanMember * pMember = pChannel->FindMember(szMsg);
		if (!pMember)
		{
			pMe->SendChatMsg(CHATMSG_NoPlayer, szMsg);
			break;
		}

		pChannel->KickMember(pMe, pMember);
		// If noone is left, tell the chat system to
		// delete it from memory (you can kick yourself)
		if (pChannel->m_Members.size() <= 0) // Kicked self
		{
			DeleteChannel(pChannel);
		}
		break;
	};
	case 'X':	// X = client quit chat
		QuitChat(pClient);
		break;
	case 'w':	// w = (+defaultvoice) make moderators be the only ones with a voice by default
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->DisableVoiceDefault(pMe->GetChatName());
		break;
	case 'x':	// x = (-defaultvoice) give everyone a voice by default
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->EnableVoiceDefault(pMe->GetChatName());
		break;
	case 'y':	// y = (/defaultvoice) toggle
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->ToggleVoiceDefault(pMe->GetChatName());
		break;
	case 'z':	// z = emote
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->Emote(pMe->GetChatName(), szMsg, lang );
		break;
	};
}

void CChat::QuitChat(CChatChanMember * pClient)
{
	ADDTOCALLSTACK("CChat::QuitChat");
	// Are we in a channel now?
	CChatChannel * pCurrentChannel = pClient->GetChannel();
	// Remove from old channel (if any)
	if (pCurrentChannel)
	{
		// Remove myself from the channels list of members
		pCurrentChannel->RemoveMember( pClient );

		// Am I the last one here? Delete it from all other clients?
		if (pCurrentChannel->m_Members.size() <= 0)
		{
			// If noone is left, tell the chat system to delete it from memory
			DeleteChannel(pCurrentChannel);
		}
	}
	// Now tell the chat system you left

	pClient->SetChatInactive();
}

void CChat::DoCommand(CChatChanMember * pBy, lpctstr szMsg)
{
	ADDTOCALLSTACK("CChat::DoCommand");
	static lpctstr constexpr sm_szCmd_Chat[] =
	{
		"ALLKICK",
		"BC",
		"BCALL",
		"CHATSOK",
		"CLEARIGNORE",
		"KILLCHATS",
		"NOCHATS",
		"SYSMSG",
		"WHEREIS"
	};

	ASSERT(pBy != nullptr);
	ASSERT(szMsg != nullptr);

	tchar buffer[2048];
	Str_CopyLimitNull(buffer, szMsg, sizeof(buffer));

	tchar * pszCommand = buffer;
	tchar * pszText = nullptr;
	size_t iCommandLength = strlen(pszCommand);
	for (size_t i = 0; i < iCommandLength; i++)
	{
		ASSERT( i<ARRAY_COUNT(buffer));
		if (pszCommand[i] == ' ')
		{
			pszCommand[i] = 0;
			pszText = pszCommand + i + 1;
		}
	}

	CSString sFrom;
	CChatChannel * pChannel = pBy->GetChannel();
	CClient * pByClient = pBy->GetClientActive();
	ASSERT(pByClient != nullptr);

	switch ( FindTableSorted( pszCommand, sm_szCmd_Chat, ARRAY_COUNT(sm_szCmd_Chat)))
	{
		case 0: // "ALLKICK"
		{
			if (pChannel == nullptr)
			{
				pBy->SendChatMsg(CHATMSG_MustBeInAConference);
				return;
			}

			if (!pChannel->IsModerator(pBy->GetChatName()))
			{
				pBy->SendChatMsg(CHATMSG_MustHaveOps);
				return;
			}

			pChannel->KickAll(pBy);
			DecorateName(sFrom, nullptr, true);
			pBy->SendChatMsg(CHATMSG_PlayerTalk, sFrom, "All members have been kicked!", "");
			return;
		}
		case 1: // "BC"
		{
			if ( ! pByClient->IsPriv( PRIV_GM ))
			{
	need_gm_privs:
				DecorateName(sFrom, nullptr, true);
				pBy->SendChatMsg(CHATMSG_PlayerTalk, sFrom, "You need to have GM privs to use this command.");
				return;
			}

			if (pszText)
				Broadcast(pBy, pszText);
			return;
		}
		case 2: // "BCALL"
		{
			if ( ! pByClient->IsPriv( PRIV_GM ))
				goto need_gm_privs;

			if (pszText)
				Broadcast(pBy, pszText, "", true);
			return;
		}
		case 3: // "CHATSOK"
		{
			if ( ! pByClient->IsPriv( PRIV_GM ))
				goto need_gm_privs;

			if (!m_fChatsOK)
			{
				m_fChatsOK = true;
				Broadcast(nullptr, "Conference creation is enabled.");
			}
			return;
		}
		case 4: // "CLEARIGNORE"
		{
			pBy->ClearIgnoreList();
			return;
		}
		case 5: // "KILLCHATS"
		{
			if ( ! pByClient->IsPriv( PRIV_GM ))
				goto need_gm_privs;

			KillChannels();
			return;
		}
		case 6: // "NOCHATS"
		{
			if ( ! pByClient->IsPriv( PRIV_GM ))
				goto need_gm_privs;

			if (m_fChatsOK)
			{
				Broadcast(nullptr, "Conference creation is now disabled.");
				m_fChatsOK = false;
			}
			return;
		}
		case 7: // "SYSMSG"
		{
			if ( ! pByClient->IsPriv( PRIV_GM ))
				goto need_gm_privs;

			if (pszText)
				Broadcast(nullptr, pszText, "", true);
			return;
		}
		case 8:	// "WHEREIS"
		{
			if (pszText)
				WhereIs(pBy, pszText);
			return;
		}
		default:
		{
			tchar *pszMsg = Str_GetTemp();
			snprintf(pszMsg, STR_TEMPLENGTH, "Unknown command: '%s'", pszCommand);

			DecorateName(sFrom, nullptr, true);
			pBy->SendChatMsg(CHATMSG_PlayerTalk, sFrom, pszMsg);
			return;
		}
	}
}

void CChat::KillChannels()
{
	ADDTOCALLSTACK("CChat::KillChannels");
	CChatChannel * pChannel = GetFirstChannel();
	// First /kick everyone
	for ( ; pChannel != nullptr; pChannel = pChannel->GetNext())
		pChannel->KickAll();
	m_Channels.ClearContainer();
}

void CChat::WhereIs(CChatChanMember * pBy, lpctstr pszName ) const
{
	ADDTOCALLSTACK("CChat::WhereIs");
	
	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( ! strcmp( pClient->GetChatName(), pszName))
			continue;

		tchar *pszMsg = Str_GetTemp();
		if (! pClient->IsChatActive() || !pClient->GetChannel())
			snprintf(pszMsg, STR_TEMPLENGTH, "%s is not currently in a conference.", pszName);
		else
			snprintf(pszMsg, STR_TEMPLENGTH, "%s is in conference '%s'.", pszName, pClient->GetChannel()->GetName());
		CSString sFrom;
		DecorateName(sFrom, nullptr, true);
		pBy->SendChatMsg(CHATMSG_PlayerTalk, sFrom, pszMsg);
		return;
	}

	pBy->SendChatMsg(CHATMSG_NoPlayer, pszName);
}

void CChat::DeleteChannel(CChatChannel * pChannel)
{
	ADDTOCALLSTACK("CChat::DeleteChannel");
	SendDeleteChannel(pChannel);	// tell everyone about it first.
	delete pChannel;
}

void CChat::SendDeleteChannel(CChatChannel * pChannel)
{
	ADDTOCALLSTACK("CChat::SendDeleteChannel");
	// Send a delete channel name message to all clients using the chat system
	
	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( ! pClient->IsChatActive())
			continue;
		pClient->addChatSystemMessage(CHATMSG_RemoveChannelName, pChannel->GetName());
	}
}

bool CChat::IsValidName( lpctstr pszName, bool fPlayer ) // static
{
	ADDTOCALLSTACK("CChat::IsValidName");
	// Channels can have spaces, but not player names
	if (strlen(pszName) < 1)
		return false;
	if (!strncmp(pszName, "SYSTEM",6))
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

void CChat::SendNewChannel(CChatChannel * pNewChannel)
{
	ADDTOCALLSTACK("CChat::SendNewChannel");
	// Send this new channel name to all clients using the chat system
	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( ! pClient->IsChatActive())
			continue;
		pClient->addChatSystemMessage(CHATMSG_SendChannelName, pNewChannel->GetName(), pNewChannel->GetModeString());
	}
}

void CChat::DecorateName(CSString &sName, const CChatChanMember * pMember, bool fSystem) // static
{
	ADDTOCALLSTACK("CChat::DecorateName");
	CChatChannel * pChannel = nullptr;
	if (pMember)
		pChannel = pMember->GetChannel();
	// 0 = yellow
	// 1 = purple
	// 2 = blue
	// 3 = purple
	// 4 = white
	// 5 = green
	int iResult = 0;
	if (!pMember || !pChannel) // Must be a system command if these are invalid
	{
		if (fSystem)
			iResult = 5;
		else
			iResult = 4;
	}
	else if (pChannel->IsModerator(pMember->GetChatName()))
		iResult = 1;
	else if (!pChannel->HasVoice(pMember->GetChatName()))
		iResult = 2;

	if (!pMember || !pChannel)
		sName.Format("%i%s", iResult, "SYSTEM");
	else
		sName.Format("%i%s", iResult, static_cast<lpctstr>(pMember->GetChatName()));
}

void CChat::GenerateChatName(CSString &sName, const CClient * pClient) // static
{
	if (pClient == nullptr)
		return;

	// decide upon 'base' name
	lpctstr pszName = nullptr;
	if (pClient->GetChar() != nullptr)
		pszName = pClient->GetChar()->GetName();

	if (pszName == nullptr)
		return;

	// try the base name
	CSString sTempName(pszName);
	if (g_Accounts.Account_FindChat(sTempName.GetBuffer()) != nullptr)
	{
		sTempName.Clear();

		// append (n) to the name to make it unique
		for (uint attempts = 2; attempts <= g_Accounts.Account_GetCount(); attempts++)
		{
			sTempName.Format("%s (%u)", pszName, attempts);
			if (g_Accounts.Account_FindChat(static_cast<lpctstr>(sTempName)) == nullptr)
				break;

			sTempName.Clear();
		}
	}

	// copy name to output
	sName.Copy(sTempName.GetBuffer());
}

void CChat::Broadcast(CChatChanMember *pFrom, lpctstr pszText, CLanguageID lang, bool fOverride)
{
	ADDTOCALLSTACK("CChat::Broadcast");
	ClientIterator it;
	for (CClient *pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if (!pClient->IsChatActive())
			continue;
		if (fOverride || pClient->IsReceivingAllowed())
		{
			CSString sName;
			DecorateName(sName, pFrom, fOverride);
			pClient->SendChatMsg(CHATMSG_PlayerTalk, sName, pszText, lang);
		}
	}
}

void CChat::CreateJoinChannel(CChatChanMember * pByMember, lpctstr pszName, lpctstr pszPassword)
{
	ADDTOCALLSTACK("CChat::CreateJoinChannel");
	if ( ! IsValidName( pszName, false ))
	{
		pByMember->GetClientActive()->addChatSystemMessage( CHATMSG_InvalidConferenceName );
	}
	else if (IsDuplicateChannelName(pszName))
	{
		pByMember->GetClientActive()->addChatSystemMessage( CHATMSG_AlreadyAConference );
	}
	else
	{
		if ( CreateChannel(pszName, ((pszPassword != nullptr) ? pszPassword : ""), pByMember))
			JoinChannel(pByMember, pszName, ((pszPassword != nullptr) ? pszPassword : ""));
	}
}

bool CChat::CreateChannel(lpctstr pszName, lpctstr pszPassword, CChatChanMember * pMember)
{
	ADDTOCALLSTACK("CChat::CreateChannel");
	if (!m_fChatsOK)
	{
		CSString sName;
		DecorateName(sName, nullptr, true);
		pMember->SendChatMsg(CHATMSG_PlayerTalk, sName, "Conference creation is disabled.");
		return false;
	}
	CChatChannel * pChannel = new CChatChannel( pszName, pszPassword );
	m_Channels.InsertContentTail( pChannel );
	pChannel->SetModerator(pMember->GetChatName());
	// Send all clients with an open chat window the new channel name
	SendNewChannel(pChannel);
	return true;
}

bool CChat::JoinChannel(CChatChanMember * pMember, lpctstr pszChannel, lpctstr pszPassword)
{
	ADDTOCALLSTACK("CChat::JoinChannel");
	ASSERT(pMember != nullptr);
	CClient * pMemberClient = pMember->GetClientActive();
	ASSERT(pMemberClient != nullptr);

	// Are we in a channel now?
	CChatChannel * pCurrentChannel = pMember->GetChannel();
	if (pCurrentChannel != nullptr)
	{
		// Is it the same channel as the one I'm already in?
		if (strcmp(pszChannel, pCurrentChannel->GetName()) == 0)
		{
			// Tell them and return
			pMember->SendChatMsg(CHATMSG_AlreadyInConference, pszChannel);
			return false;
		}
	}

	CChatChannel * pNewChannel = FindChannel(pszChannel);
	if (pNewChannel == nullptr)
	{
		pMemberClient->addChatSystemMessage(CHATMSG_NoConference, pszChannel );
		return false;
	}

	// If there's a password, is it the correct one?
	if (strcmp(pNewChannel->GetPassword(), pszPassword) != 0)
	{
		pMemberClient->addChatSystemMessage(CHATMSG_IncorrectPassword);
		return false;
	}

	// Leave the old channel 1st
	// Remove from old channel (if any)
	if (pCurrentChannel != nullptr)
	{
		// Remove myself from the channels list of members
		pCurrentChannel->RemoveMember(pMember);

		// If noone is left, tell the chat system to delete it from memory
		if (pCurrentChannel->m_Members.size() <= 0)
		{
			// Am I the last one here? Delete it from all other clients?
			DeleteChannel(pCurrentChannel);
		}

		// Since we left, clear all members from our client that might be in our list from the channel we just left
		pMemberClient->addChatSystemMessage(CHATMSG_ClearMemberList);
	}

	// Now join a new channel
	// Add all the members of the channel to the clients list of channel participants
	pNewChannel->SendMembers(pMember);

	// Add ourself to the channels list of members
	if (!pNewChannel->AddMember(pMember))
		return false;

	// Set the channel name title bar
	pMemberClient->addChatSystemMessage(CHATMSG_UpdateChannelBar, pszChannel);

	// Now send out my name to all clients in this channel
	pNewChannel->SendThisMember(pMember);
	return true;
}

CChatChannel * CChat::FindChannel(lpctstr pszChannel) const
{
	CChatChannel * pChannel = GetFirstChannel();
	for ( ; pChannel != nullptr; pChannel = pChannel->GetNext())
	{
		if (strcmp(pChannel->GetName(), pszChannel) == 0)
			break;
	}
	return pChannel;
}

CChat::CChat()
{
	m_fChatsOK = true;
}

CChatChannel * CChat::GetFirstChannel() const
{
	return static_cast <CChatChannel *>(m_Channels.GetContainerHead());
}

bool CChat::IsDuplicateChannelName(const char * pszName) const
{
	return FindChannel(pszName) != nullptr;
}


