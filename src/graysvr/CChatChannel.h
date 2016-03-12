#ifndef _INC_CCHATCHANNEL_H
#define _INC_CCHATCHANNEL_H

#include "../common/grayproto.h"
#include "../common/CArray.h"
#include "../common/CString.h"

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
    void SetModerator(LPCTSTR pszName, bool fFlag = true);
    void SetVoice(LPCTSTR pszName, bool fFlag = true);
    void RenameChannel(CChatChanMember * pBy, LPCTSTR pszName);
    size_t FindMemberIndex( LPCTSTR pszName ) const;

public:
    explicit CChatChannel(LPCTSTR pszName, LPCTSTR pszPassword = NULL);

private:
    CChatChannel(const CChatChannel& copy);
    CChatChannel& operator=(const CChatChannel& other);

public:
    CChatChannel* GetNext() const;
    LPCTSTR GetName() const;
    LPCTSTR GetModeString() const;
    LPCTSTR GetPassword() const;
    void SetPassword( LPCTSTR pszPassword);
    bool IsPassworded() const;

    bool GetVoiceDefault()  const;
    void SetVoiceDefault(bool fVoiceDefault);
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
    CChatChanMember * FindMember(LPCTSTR pszName) const;
    bool RemoveMember(LPCTSTR pszName);
    void SetName(LPCTSTR pszName);
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


#endif //_INC_CCHATCHANNEL_H
