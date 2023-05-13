/**
* @file CChat.h
*
*/

#ifndef _INC_CCHAT_H
#define _INC_CCHAT_H

#include "../../common/sphere_library/CSString.h"
#include "../../common/sphere_library/CSObjList.h"
#include "../../common/sphereproto.h"


class CClient;
class CChatChannel;
class CChatChanMember;

class CChat
{
	// All the chat channels.
private:
	bool m_fChatsOK;	// allowed to create new chats ?
	CSObjList m_Channels;		// CChatChannel // List of chat channels.
private:
	void DoCommand(CChatChanMember * pBy, lpctstr szMsg);
	void DeleteChannel(CChatChannel * pChannel);
	void WhereIs(CChatChanMember * pBy, lpctstr pszName) const;
	void KillChannels();
	bool JoinChannel(CChatChanMember * pMember, lpctstr pszChannel, lpctstr pszPassword);
	bool CreateChannel(lpctstr pszName, lpctstr pszPassword, CChatChanMember * pMember);
	void CreateJoinChannel(CChatChanMember * pByMember, lpctstr pszName, lpctstr pszPassword);
	CChatChannel * FindChannel(lpctstr pszChannel) const;
public:
	static const char *m_sClassName;
	CChat();

private:
	CChat(const CChat& copy);
	CChat& operator=(const CChat& other);

public:
	CChatChannel * GetFirstChannel() const;

	void EventMsg( CClient * pClient, const nachar * pszText, int len, CLanguageID lang ); // Text from a client

	static bool IsValidName(lpctstr pszName, bool fPlayer);

	void SendDeleteChannel(CChatChannel * pChannel);
	void SendNewChannel(CChatChannel * pNewChannel);
	bool IsDuplicateChannelName(const char * pszName) const;

	void Broadcast(CChatChanMember * pFrom, lpctstr pszText, CLanguageID lang = 0, bool fOverride = false);
	void QuitChat(CChatChanMember * pClient);

	static void DecorateName(CSString & sName, const CChatChanMember * pMember = nullptr, bool fSystem = false);
	static void GenerateChatName(CSString & sName, const CClient * pClient);
};


#endif // _INC_CCHAT_H
