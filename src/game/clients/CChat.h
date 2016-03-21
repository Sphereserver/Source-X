
#pragma once
#ifndef _INC_CCHAT_H
#define _INC_CCHAT_H

#include "../common/CString.h"
#include "../common/CArray.h"
#include "../common/grayproto.h"


class CClient;
class CChatChannel;
class CChatChanMember;

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
	CChatChannel * FindChannel(LPCTSTR pszChannel) const;
public:
	static const char *m_sClassName;
	CChat();

private:
	CChat(const CChat& copy);
	CChat& operator=(const CChat& other);

public:
	CChatChannel * GetFirstChannel() const;

	void EventMsg( CClient * pClient, const NCHAR * pszText, int len, CLanguageID lang ); // Text from a client

	static bool IsValidName(LPCTSTR pszName, bool fPlayer);

	void SendDeleteChannel(CChatChannel * pChannel);
	void SendNewChannel(CChatChannel * pNewChannel);
	bool IsDuplicateChannelName(const char * pszName) const;

	void Broadcast(CChatChanMember * pFrom, LPCTSTR pszText, CLanguageID lang = 0, bool fOverride = false);
	void QuitChat(CChatChanMember * pClient);

	static void DecorateName(CGString & sName, const CChatChanMember * pMember = NULL, bool fSystem = false);
	static void GenerateChatName(CGString & sName, const CClient * pClient);
};

#endif // _INC_CCHAT_H
