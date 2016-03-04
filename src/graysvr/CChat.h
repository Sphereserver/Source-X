#ifndef _INC_CCHAT_H
#define _INC_CCHAT_H

#include "../common/CString.h"
#include "../common/CArray.h"
#include "../common/grayproto.h"

class CClient;
class CChatChannel;

class CChatChanMember
{
	// This is a member of the CClient.
private:
	bool m_fChatActive;
	bool m_fReceiving;
	bool m_fAllowWhoIs;
	CChatChannel * m_pChannel;	// I can only be a member of one chan at a time.
public:
	static const char *m_sClassName;
	CGObArray< CGString * > m_IgnoredMembers;	// Player's list of ignored members
private:
	friend class CChatChannel;
	friend class CChat;
	// friend CClient;
	bool GetWhoIs() const { return m_fAllowWhoIs; }
	void SetWhoIs(bool fAllowWhoIs) { m_fAllowWhoIs = fAllowWhoIs; }
	bool IsReceivingAllowed() const { return m_fReceiving; }
	LPCTSTR GetChatName() const;

	size_t FindIgnoringIndex( LPCTSTR pszName) const;

protected:
	void SetChatActive();
	void SetChatInactive();
public:
	CChatChanMember()
	{
		m_fChatActive = false;
		m_pChannel = NULL;
		m_fReceiving = true;
		m_fAllowWhoIs = true;
	}

	virtual ~CChatChanMember();

private:
	CChatChanMember(const CChatChanMember& copy);
	CChatChanMember& operator=(const CChatChanMember& other);

public:
	CClient * GetClient();
	const CClient * GetClient() const;

	bool IsChatActive() const
	{
		return( m_fChatActive );
	}

	void SetReceiving(bool fOnOff)
	{
		if (m_fReceiving != fOnOff)
			ToggleReceiving();
	}
	void ToggleReceiving();

	void PermitWhoIs();
	void ForbidWhoIs();
	void ToggleWhoIs();

	CChatChannel * GetChannel() const { return m_pChannel; }
	void SetChannel(CChatChannel * pChannel) { m_pChannel = pChannel; }
	void SendChatMsg( CHATMSG_TYPE iType, LPCTSTR pszName1 = NULL, LPCTSTR pszName2 = NULL, CLanguageID lang = 0 );
	void RenameChannel(LPCTSTR pszName);

	void Ignore(LPCTSTR pszName);
	void DontIgnore(LPCTSTR pszName);
	void ToggleIgnore(LPCTSTR pszName);
	void ClearIgnoreList();
	bool IsIgnoring(LPCTSTR pszName) const
	{
		return( FindIgnoringIndex( pszName ) != m_IgnoredMembers.BadIndex() );
	}
};

class CChatChannel : public CGObListRec
{
	// a number of clients can be attached to this chat channel.
private:
	friend class CChatChanMember;
	friend class CChat;
	CGString m_sName;
	CGString m_sPassword;
	bool m_fVoiceDefault;	// give others voice by default.
public:
	static const char *m_sClassName;
	CGObArray< CGString * > m_NoVoices;// Current list of channel members with no voice
	CGObArray< CGString * > m_Moderators;// Current list of channel's moderators (may or may not be currently in the channel)
	CGPtrTypeArray< CChatChanMember* > m_Members;	// Current list of members in this channel
private:
	void SetModerator(LPCTSTR pszName, bool fFlag = true);
	void SetVoice(LPCTSTR pszName, bool fFlag = true);
	void RenameChannel(CChatChanMember * pBy, LPCTSTR pszName);
	size_t FindMemberIndex( LPCTSTR pszName ) const;

public:
	explicit CChatChannel(LPCTSTR pszName, LPCTSTR pszPassword = NULL)
	{
		m_sName = pszName;
		m_sPassword = pszPassword;
		m_fVoiceDefault = true;
	};

private:
	CChatChannel(const CChatChannel& copy);
	CChatChannel& operator=(const CChatChannel& other);

public:
	CChatChannel* GetNext() const
	{
		return( static_cast <CChatChannel *>( CGObListRec::GetNext()));
	}
	LPCTSTR GetName() const
	{
		return( m_sName );
	}
	LPCTSTR GetModeString() const
	{
		// (client needs this) "0" = not passworded, "1" = passworded
		return(( IsPassworded()) ? "1" : "0" );
	}

	LPCTSTR GetPassword() const
	{
		return( m_sPassword );
	}
	void SetPassword( LPCTSTR pszPassword)
	{
		m_sPassword = pszPassword;
		return;
	}
	bool IsPassworded() const
	{
		return ( !m_sPassword.IsEmpty());
	}

	bool GetVoiceDefault()  const { return m_fVoiceDefault; }
	void SetVoiceDefault(bool fVoiceDefault) { m_fVoiceDefault = fVoiceDefault; }
	void ToggleVoiceDefault(LPCTSTR  pszBy);
	void DisableVoiceDefault(LPCTSTR  pszBy);
	void EnableVoiceDefault(LPCTSTR  pszBy);
	void Emote(LPCTSTR pszBy, LPCTSTR pszMsg, CLanguageID lang = 0 );
	void WhoIs(LPCTSTR pszBy, LPCTSTR pszMember);
	bool AddMember(CChatChanMember * pMember);
	void KickMember( CChatChanMember *pByMember, CChatChanMember * pMember );
	void Broadcast(CHATMSG_TYPE iType, LPCTSTR pszName, LPCTSTR pszText, CLanguageID lang = 0, bool fOverride = false);
	void SendThisMember(CChatChanMember * pMember, CChatChanMember * pToMember = NULL);
	void SendMembers(CChatChanMember * pMember);
	void RemoveMember(CChatChanMember * pMember);
	CChatChanMember * FindMember(LPCTSTR pszName) const
	{
		size_t i = FindMemberIndex( pszName );
		if ( i == m_Members.BadIndex() )
			return NULL;
		return m_Members[i];
	}
	bool RemoveMember(LPCTSTR pszName)
	{
		CChatChanMember * pMember = FindMember(pszName);
		if ( pMember == NULL )
			return false;
		RemoveMember(pMember);
		return true;
	}
	void SetName(LPCTSTR pszName)
	{
		m_sName = pszName;
	}
	bool IsModerator(LPCTSTR pszName) const;
	bool HasVoice(LPCTSTR pszName) const;

	void MemberTalk(CChatChanMember * pByMember, LPCTSTR pszText, CLanguageID lang );
	void ChangePassword(CChatChanMember * pByMember, LPCTSTR pszPassword);
	void GrantVoice(CChatChanMember * pByMember, LPCTSTR pszName);
	void RevokeVoice(CChatChanMember * pByMember, LPCTSTR pszName);
	void ToggleVoice(CChatChanMember * pByMember, LPCTSTR pszName);
	void GrantModerator(CChatChanMember * pByMember, LPCTSTR pszName);
	void RevokeModerator(CChatChanMember * pByMember, LPCTSTR pszName);
	void ToggleModerator(CChatChanMember * pByMember, LPCTSTR pszName);
	void SendPrivateMessage(CChatChanMember * pFrom, LPCTSTR pszTo, LPCTSTR  pszMsg);
	void KickAll(CChatChanMember * pMember = NULL);
};

class CChat
{
	// All the chat channels.
private:
	bool m_fChatsOK;	// allowed to create new chats ?
	CGObList m_Channels;		// CChatChannel // List of chat channels.
private:
	void DoCommand(CChatChanMember * pBy, LPCTSTR szMsg);
	void DeleteChannel(CChatChannel * pChannel);
	void WhereIs(CChatChanMember * pBy, LPCTSTR pszName) const;
	void KillChannels();
	bool JoinChannel(CChatChanMember * pMember, LPCTSTR pszChannel, LPCTSTR pszPassword);
	bool CreateChannel(LPCTSTR pszName, LPCTSTR pszPassword, CChatChanMember * pMember);
	void CreateJoinChannel(CChatChanMember * pByMember, LPCTSTR pszName, LPCTSTR pszPassword);
	CChatChannel * FindChannel(LPCTSTR pszChannel) const
	{
		CChatChannel * pChannel = GetFirstChannel();
		for ( ; pChannel != NULL; pChannel = pChannel->GetNext())
		{
			if (strcmp(pChannel->GetName(), pszChannel) == 0)
				break;
		}
		return pChannel;
	};
public:
	static const char *m_sClassName;
	CChat()
	{
		m_fChatsOK = true;
	}

private:
	CChat(const CChat& copy);
	CChat& operator=(const CChat& other);

public:
	CChatChannel * GetFirstChannel() const
	{
		return STATIC_CAST <CChatChannel *>(m_Channels.GetHead());
	}

	void EventMsg( CClient * pClient, const NCHAR * pszText, int len, CLanguageID lang ); // Text from a client

	static bool IsValidName(LPCTSTR pszName, bool fPlayer);

	void SendDeleteChannel(CChatChannel * pChannel);
	void SendNewChannel(CChatChannel * pNewChannel);
	bool IsDuplicateChannelName(const char * pszName) const
	{
		return FindChannel(pszName) != NULL;
	}

	void Broadcast(CChatChanMember * pFrom, LPCTSTR pszText, CLanguageID lang = 0, bool fOverride = false);
	void QuitChat(CChatChanMember * pClient);

	static void DecorateName(CGString & sName, const CChatChanMember * pMember = NULL, bool fSystem = false);
	static void GenerateChatName(CGString & sName, const CClient * pClient);
};

#endif // _INC_CCHAT_H
