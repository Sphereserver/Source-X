/**
* @file CChatChanMember.h
*
*/

#ifndef _INC_CCHATCHANMEMBER_H
#define _INC_CCHATCHANMEMBER_H

#include "../../common/sphere_library/CSObjArray.h"
#include "../../common/sphere_library/CSString.h"
#include "../../common/sphereproto.h"


class CClient;

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
    CSObjArray<CSString *> m_IgnoredMembers;	// Player's list of ignored members
private:
    friend class CChatChannel;
    friend class CChat;
    // friend CClient;
    bool GetWhoIs() const { return m_fAllowWhoIs; }
    void SetWhoIs(bool fAllowWhoIs) { m_fAllowWhoIs = fAllowWhoIs; }
    bool IsReceivingAllowed() const { return m_fReceiving; }
    lpctstr GetChatName() const;

    size_t FindIgnoringIndex( lpctstr pszName) const;

protected:
    void SetChatActive();
    void SetChatInactive();
public:
    CChatChanMember();
    virtual ~CChatChanMember();

private:
    CChatChanMember(const CChatChanMember& copy);
    CChatChanMember& operator=(const CChatChanMember& other);

public:
    CClient * GetClientActive();
    const CClient * GetClientActive() const;
    bool IsChatActive() const;
    void SetReceiving(bool fOnOff);
    void ToggleReceiving();

    void PermitWhoIs();
    void ForbidWhoIs();
    void ToggleWhoIs();

    CChatChannel * GetChannel() const;
    void SetChannel(CChatChannel * pChannel);
    void SendChatMsg( CHATMSG_TYPE iType, lpctstr pszName1 = nullptr, lpctstr pszName2 = nullptr, CLanguageID lang = 0 );
    void RenameChannel(lpctstr pszName);

    void Ignore(lpctstr pszName);
    void DontIgnore(lpctstr pszName);
    void ToggleIgnore(lpctstr pszName);
    void ClearIgnoreList();
    bool IsIgnoring(lpctstr pszName) const;
};

#endif // _INC_CCHATCHANMEMBER_H


