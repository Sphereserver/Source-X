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
public:
	static const char *m_sClassName;
	CChat();
	CSObjList m_Channels;		// CChatChannel // List of chat channels.

private:
	CChat(const CChat& copy);
	CChat& operator=(const CChat& other);

public:
	CChatChannel * GetFirstChannel() const;

	void Action( CClient * pClient, const nachar * pszText, int len, CLanguageID lang ); // Text from a client

	static bool IsValidName(lpctstr pszName, bool fPlayer);

	void Broadcast(CChatChanMember * pFrom, lpctstr pszText, CLanguageID lang = 0, bool fOverride = false);
	void BroadcastAddChannel(CChatChannel* pChannel);
	void BroadcastRemoveChannel(CChatChannel* pChannel);
	void QuitChat(CChatChanMember * pClient);

	void JoinChannel(lpctstr pszChannel, lpctstr pszPassword, CChatChanMember* pMember = nullptr);
	void DeleteChannel(CChatChannel* pChannel);
	void FormatName(CSString & sName, const CChatChanMember * pMember = nullptr, bool fSystem = false);

	CChatChannel* FindChannel(lpctstr pszChannel) const;
	bool CreateChannel(lpctstr pszName, lpctstr pszPassword = nullptr, CChatChanMember* pMember = nullptr);
};


#endif // _INC_CCHAT_H
