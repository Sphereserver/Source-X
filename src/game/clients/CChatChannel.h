/**
* @file CChatChannel.h
*
*/

#ifndef _INC_CCHATCHANNEL_H
#define _INC_CCHATCHANNEL_H

#include "../../common/sphere_library/CSObjListRec.h"
#include "../../common/sphere_library/CSString.h"
#include "../../common/sphere_library/sptr_containers.h"
#include "../../common/sphereproto.h"


class CChatChanMember;

class CChatChannel : public CSObjListRec
{
    // a number of clients can be attached to this chat channel.
private:
    friend class CChatChanMember;
    friend class CChat;
    CSString m_sName;
    CSString m_sPassword;
    bool m_fStatic;			// static channel created on server startup
    bool m_fVoiceDefault;	// give others voice by default.
public:
    static const char *m_sClassName;
    sl::unique_ptr_vector<CSString>             m_NoVoices;     // Current list of channel members with no voice
    sl::unique_ptr_vector<CSString>             m_Moderators;   // Current list of channel's moderators (may or may not be currently in the channel)
    sl::raw_ptr_view_vector<CChatChanMember>    m_Members;	    // Current list of members in this channel
private:
    void SetModerator(lpctstr pszName, bool fFlag = true);
    void SetVoice(lpctstr pszName, bool fFlag = true);
    void RenameChannel(CChatChanMember * pBy, lpctstr pszName);
    size_t FindMemberIndex( lpctstr pszName ) const;

public:
    explicit CChatChannel(lpctstr pszName, lpctstr pszPassword = nullptr, bool fStatic = false);

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
    void FillMembersList(CChatChanMember* pMember);
    void PrivateMessage(CChatChanMember* pFrom, lpctstr pszTo, lpctstr pszMsg = nullptr, CLanguageID lang = 0);
    void AddVoice(CChatChanMember* pByMember, lpctstr pszName);
    void RemoveVoice(CChatChanMember* pByMember, lpctstr pszName);
    void EnableDefaultVoice(lpctstr pszName);
    void DisableDefaultVoice(lpctstr pszName);
    void ToggleDefaultVoice(lpctstr pszName);
    void ToggleVoiceDefault(lpctstr  pszBy);
    void DisableVoiceDefault(lpctstr  pszBy);
    void EnableVoiceDefault(lpctstr  pszBy);
    void Emote(lpctstr pszBy, lpctstr pszMsg, CLanguageID lang = 0 );
    void WhoIs(lpctstr pszBy, lpctstr pszMember);
    void AddMember(CChatChanMember * pMember);
    void SendMember(CChatChanMember* pMember, CChatChanMember* pToMember = nullptr);
    void KickMember( CChatChanMember *pByMember, CChatChanMember * pMember );
    void Broadcast(CHATMSG_TYPE iType, lpctstr pszName = nullptr, lpctstr pszText = nullptr, CLanguageID lang = 0, bool fOverride = false);
    void RemoveMember(CChatChanMember * pMember);
    CChatChanMember* FindMember(lpctstr pszName) const;
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
};


#endif // _INC_CCHATCHANNEL_H
