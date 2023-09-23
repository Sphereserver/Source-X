/**
* @file CParty.h
*
*/

#ifndef _INC_CPARTY_H
#define _INC_CPARTY_H

#include "../../common/sphere_library/CSString.h"
#include "../../common/sphere_library/CSObjListRec.h"
#include "../../common/CRect.h"
#include "../../common/CScriptObj.h"
#include "../../common/CVarDefMap.h"
#include "../../common/sphereproto.h"
#include "../chars/CCharRefArray.h"


class PacketSend;

class CPartyDef : public CSObjListRec, public CScriptObj
{
	// a list of characters in the party.
#define MAX_CHAR_IN_PARTY 10

public:
	static const char *m_sClassName;
	static lpctstr const sm_szVerbKeys[];
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szLoadKeysM[];
private:
	CCharRefArray m_Chars;
	CSString m_sName;
	CVarDefMap m_TagDefs;
	CVarDefMap m_BaseDefs;		// New Variable storage system
	CSString m_pSpeechFunction;

public:
	lpctstr GetDefStr( lpctstr ptcKey, bool fZero = false ) const;
	int64 GetDefNum( lpctstr ptcKey ) const;
	void SetDefNum(lpctstr ptcKey, int64 iVal, bool fZero = true);
	void SetDefStr(lpctstr ptcKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true);
	void DeleteDef(lpctstr ptcKey);

private:
	bool SendMemberMsg( CChar * pCharDest, PacketSend * pPacket );
	void SendAll( PacketSend * pPacket );
	// List manipulation
	size_t AttachChar( CChar * pChar );
	size_t DetachChar( CChar * pChar );

public:
	CPartyDef( CChar * pCharInvite, CChar * pCharAccept );

private:
	CPartyDef(const CPartyDef& copy);
	CPartyDef& operator=(const CPartyDef& other);

public:
	static bool AcceptEvent( CChar * pCharAccept, CUID uidInviter, bool bForced = false, bool bSendMessages = true);
	static bool DeclineEvent( CChar * pCharDecline, CUID uidInviter );

	bool IsPartyFull() const;
	bool IsInParty( const CChar * pChar ) const;
	bool IsPartyMaster( const CChar * pChar ) const;
	CUID GetMaster();

	// Refresh status for party members
	void AddStatsUpdate( CChar * pChar, PacketSend * pPacket );
	// List sending wrappers
	bool SendRemoveList( CChar * pCharRemove, bool bFor );
	bool SendAddList( CChar * pCharDest );
	// Party message sending wrappers
	bool MessageEvent( CUID uidDst, CUID uidSrc, const nachar* pText, int ilenmsg );
	// void MessageAll( CUID uidSrc, const nchar * pText, int ilenmsg );
	// bool MessageMember( CUID uidDst, CUID uidSrc, const nchar * pText, int ilenmsg );
	// Sysmessage sending wrappers
	void SysMessageAll( lpctstr pText );
    void UpdateWaypointAll(CChar *pCharSrc, MAPWAYPOINT_TYPE type);

	// Commands
	bool Disband( CUID uidMaster );
	bool RemoveMember( CUID uidRemove, CUID uidCommand );
	void AcceptMember( CChar * pChar );
	void SetLootFlag( CChar * pChar, bool fSet );
	bool GetLootFlag( const CChar * pChar );
	bool SetMaster( CChar * pChar );

	// -------------------------------

	virtual lpctstr GetName() const override {
	    return static_cast<lpctstr>(m_sName);
	    }
	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
    virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
    virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override; // Execute command from script
    virtual bool r_LoadVal( CScript & s ) override;
    virtual bool r_Load( CScript & s ) override;
};


#endif // _INC_CPARTY_H
