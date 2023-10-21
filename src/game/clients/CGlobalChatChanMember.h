/**
* @file CGlobalChatChanMember.h
*
*/

#ifndef _INC_CGLOBALCHATCHANMEMBER_H
#define _INC_CGLOBALCHATCHANMEMBER_H

class CClient;

class CGlobalChatChanMember		// This is member of CClient
{
public:
	static const char* m_sClassName;

	CGlobalChatChanMember();
	virtual ~CGlobalChatChanMember();

private:
	lpctstr m_pszJID;	// client Jabber ID
	bool m_fVisible;	// client visibility status (online/offline)

public:
	void SetJID(lpctstr pszJID);
	lpctstr GetJID() const;
	void SetVisible(bool fNewStatus);
	bool IsVisible() const;

private:
	CGlobalChatChanMember(const CGlobalChatChanMember& copy);
	CGlobalChatChanMember& operator=(const CGlobalChatChanMember& other);
};

#endif // _INC_CGLOBALCHATCHANMEMBER_H


