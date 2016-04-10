
#pragma once
#ifndef _INC_CCHATCHANNEL_H
#define _INC_CCHATCHANNEL_H

#include "../common/sphereproto.h"
#include "../common/sphere_library/CArray.h"
#include "../common/sphere_library/CString.h"


class CChatChanMember;

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
    void SetModerator(lpctstr pszName, bool fFlag = true);
    void SetVoice(lpctstr pszName, bool fFlag = true);
    void RenameChannel(CChatChanMember * pBy, lpctstr pszName);
    size_t FindMemberIndex( lpctstr pszName ) const;

public:
    explicit CChatChannel(lpctstr pszName, lpctstr pszPassword = NULL);

private:
    CChatChannel(const CChatChannel& copy);
    CChatChannel& operator=(const CChatChannel& other);

public:
    CChatChannel* GetNext() const;
    lpctstr GetName() const;
    lpctstr GetModeString() const;
    lpctstr GetPassword() const;
    void SetPassword( lpctstr pszPassword);
    bool IsPassworded() const;

    bool GetVoiceDefault()  const;
    void SetVoiceDefault(bool fVoiceDefault);
    void ToggleVoiceDefault(lpctstr  pszBy);
    void DisableVoiceDefault(lpctstr  pszBy);
    void EnableVoiceDefault(lpctstr  pszBy);
    void Emote(lpctstr pszBy, lpctstr pszMsg, CLanguageID lang = 0 );
    void WhoIs(lpctstr pszBy, lpctstr pszMember);
    bool AddMember(CChatChanMember * pMember);
    void KickMember( CChatChanMember *pByMember, CChatChanMember * pMember );
    void Broadcast(CHATMSG_TYPE iType, lpctstr pszName, lpctstr pszText, CLanguageID lang = 0, bool fOverride = false);
    void SendThisMember(CChatChanMember * pMember, CChatChanMember * pToMember = NULL);
    void SendMembers(CChatChanMember * pMember);
    void RemoveMember(CChatChanMember * pMember);
    CChatChanMember * FindMember(lpctstr pszName) const;
    bool RemoveMember(lpctstr pszName);
    void SetName(lpctstr pszName);
    bool IsModerator(lpctstr pszName) const;
    bool HasVoice(lpctstr pszName) const;

    void MemberTalk(CChatChanMember * pByMember, lpctstr pszText, CLanguageID lang );
    void ChangePassword(CChatChanMember * pByMember, lpctstr pszPassword);
    void GrantVoice(CChatChanMember * pByMember, lpctstr pszName);
    void RevokeVoice(CChatChanMember * pByMember, lpctstr pszName);
    void ToggleVoice(CChatChanMember * pByMember, lpctstr pszName);
    void GrantModerator(CChatChanMember * pByMember, lpctstr pszName);
    void RevokeModerator(CChatChanMember * pByMember, lpctstr pszName);
    void ToggleModerator(CChatChanMember * pByMember, lpctstr pszName);
    void SendPrivateMessage(CChatChanMember * pFrom, lpctstr pszTo, lpctstr  pszMsg);
    void KickAll(CChatChanMember * pMember = NULL);
};


#endif // _INC_CCHATCHANNEL_H
