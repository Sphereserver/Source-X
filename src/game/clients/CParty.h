
#pragma once
#ifndef _INC_CPARTY_H
#define _INC_CPARTY_H

#include "../common/sphere_library/CString.h"
#include "../common/sphere_library/CArray.h"
#include "../common/CRect.h"
#include "../common/CScriptObj.h"
#include "../common/CVarDefMap.h"
#include "../common/sphereproto.h"
#include "../chars/CCharRefArray.h"
#include "../CServTime.h"


class PacketSend;

class CPartyDef : public CGObListRec, public CScriptObj
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
	CGString m_sName;
	CVarDefMap m_TagDefs;
	CVarDefMap m_BaseDefs;		// New Variable storage system
	CGString m_pSpeechFunction;

public:
	lpctstr GetDefStr( lpctstr pszKey, bool fZero = false ) const;
	int64 GetDefNum( lpctstr pszKey, bool fZero = false ) const;
	void SetDefNum(lpctstr pszKey, int64 iVal, bool fZero = true);
	void SetDefStr(lpctstr pszKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true);
	void DeleteDef(lpctstr pszKey);

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
	static bool AcceptEvent( CChar * pCharAccept, CUID uidInviter, bool bForced = false );
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
	bool MessageEvent( CUID uidDst, CUID uidSrc, const NCHAR * pText, int ilenmsg );
	// void MessageAll( CUID uidSrc, const NCHAR * pText, int ilenmsg );
	// bool MessageMember( CUID uidDst, CUID uidSrc, const NCHAR * pText, int ilenmsg );
	// Sysmessage sending wrappers
	void SysMessageAll( lpctstr pText );

	// Commands
	bool Disband( CUID uidMaster );
	bool RemoveMember( CUID uidRemove, CUID uidCommand );
	void AcceptMember( CChar * pChar );
	void SetLootFlag( CChar * pChar, bool fSet );
	bool GetLootFlag( const CChar * pChar );
	bool SetMaster( CChar * pChar );

	// -------------------------------

	lpctstr GetName() const { return static_cast<lpctstr>(m_sName); }
	bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	bool r_WriteVal( lpctstr pszKey, CGString & sVal, CTextConsole * pSrc );
	bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
	bool r_LoadVal( CScript & s );
	bool r_Load( CScript & s );
};


#endif // _INC_CPARTY_H
