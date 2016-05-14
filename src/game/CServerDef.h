//
// CServerDef.h
//

#pragma once
#ifndef _INC_CSERVERDEF_H
#define _INC_CSERVERDEF_H

#include "sphere_library/CSTime.h"
#include "../common/CEncrypt.h"
#include "../common/CScriptObj.h"
#include "../common/CSocket.h"
#include "CServerTime.h"


class CTextConsole;

enum SERV_STAT_TYPE
{
	SERV_STAT_CLIENTS,	// How many clients does it say it has ? (use as % full)
	SERV_STAT_CHARS,
	SERV_STAT_ITEMS,
	SERV_STAT_ACCOUNTS,
	SERV_STAT_MEM,		// virtual
	SERV_STAT_QTY
};

enum ACCAPP_TYPE	// types of new account applications.
{
	ACCAPP_Closed = 0,	// 0=Closed. Not accepting more.
	ACCAPP_Unused1,
	ACCAPP_Free,		// 2=Anyone can just log in and create a full account.
	ACCAPP_GuestAuto,	// You get to be a guest and are automatically sent email with u're new password.
	ACCAPP_GuestTrial,	// You get to be a guest til u're accepted for full by an Admin.
	ACCAPP_Unused5,
	ACCAPP_Unspecified,	// Not specified.
	ACCAPP_Unused7,
	ACCAPP_Unused8,
	ACCAPP_QTY
};

class CServerDef : public CScriptObj
{
	static lpctstr const sm_szLoadKeys[];

private:
	CSString m_sName;	// What the name should be. Fill in from ping.
	CServerTime  m_timeLastValid;	// Last valid poll time in CServerTime::GetCurrentTime()
	CSTime	m_dateLastValid;

	CServerTime  m_timeCreate;	// When added to the list ? 0 = at start up.

	// Status read from returned string.
	CSString m_sStatus;	// last returned status string.

	// statistics
	size_t m_stStat[ SERV_STAT_QTY ];

public:
	static const char *m_sClassName;
	CSocketAddress m_ip;	// socket and port.
	CSString m_sClientVersion;
	CCrypt m_ClientVersion;

	// Breakdown the string. or filled in locally.
	char m_TimeZone;	// Hours from GMT. +5=EST
	CSString m_sEMail;		// Admin email address.
	CSString m_sURL;			// URL for the server.
	CSString m_sLang;
	ACCAPP_TYPE m_eAccApp;	// types of new account applications.

public:
	CServerDef( lpctstr pszName, CSocketAddressIP dwIP );

private:
	CServerDef(const CServerDef& copy);
	CServerDef& operator=(const CServerDef& other);

public:
	lpctstr GetStatus() const
	{
		return(m_sStatus);
	}

	size_t StatGet( SERV_STAT_TYPE i ) const;

	void StatInc( SERV_STAT_TYPE i )
	{
		ASSERT( i>=0 && i<SERV_STAT_QTY );
		m_stStat[i]++;
	}
	void StatDec( SERV_STAT_TYPE i )
	{
		ASSERT( i>=0 && i<SERV_STAT_QTY );
		m_stStat[i]--;
	}
	void SetStat( SERV_STAT_TYPE i, dword dwVal )
	{
		ASSERT( i>=0 && i<SERV_STAT_QTY );
		m_stStat[i] = dwVal;
	}

	lpctstr GetName() const { return( m_sName ); }
	void SetName( lpctstr pszName );

	virtual int64 GetAgeHours() const;

	bool IsSame( const CServerDef * pServNew ) const
	{
		UNREFERENCED_PARAMETER(pServNew);
		return true;
	}

	void SetValidTime();
	int64 GetTimeSinceLastValid() const;

	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( lpctstr pKey, CSString &sVal, CTextConsole * pSrc = NULL );

	bool IsConnected() const
	{
		return( m_timeLastValid.IsTimeValid() );
	}

	void SetCryptVersion(void)
	{
		m_ClientVersion.SetClientVer(m_sClientVersion.GetPtr());
	}
};

#endif // _INC_CSERVERDEF_H