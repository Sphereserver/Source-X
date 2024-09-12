/**
* @file CGlobalChatChanMember.h
*
*/

#ifndef _INC_CGLOBALCHATCHANMEMBER_H
#define _INC_CGLOBALCHATCHANMEMBER_H

#include "../../common/sphere_library/CSString.h"

class CClient;


class CGlobalChatChanMember		// This is member of CClient
{
public:
	static const char* m_sClassName;

	CGlobalChatChanMember() noexcept;
	virtual ~CGlobalChatChanMember() noexcept;

private:
	CSString m_strJID;	// client Jabber ID
	bool m_fVisible;	// client visibility status (online/offline)

public:
	void SetJID(lpctstr pszJID);
	lpctstr GetJID() const;
	void SetVisible(bool fNewStatus);
	bool IsVisible() const;

	CGlobalChatChanMember(const CGlobalChatChanMember& copy) = delete;
	CGlobalChatChanMember& operator=(const CGlobalChatChanMember& other) = delete;
};

#endif // _INC_CGLOBALCHATCHANMEMBER_H


