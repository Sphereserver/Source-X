// Login and low level stuff for the client.

#include "../../common/crypto/CCrypto.h"
#include "../../common/sphere_library/CSFileList.h"
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../../network/network.h"
#include "../../network/send.h"
#include "../CServer.h"
#include "CClient.h"

#include "../../common/zlib/zlib.h"


CHuffman CClient::m_Comp;


/////////////////////////////////////////////////////////////////
// -CClient stuff.

size_t CClient::xCompress( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen ) // static
{
	ADDTOCALLSTACK("CClient::xCompress");
	// The game server will compress the outgoing data to the clients.
	return m_Comp.Compress( pOutput, pInput, outLen, inLen );
}

bool CClient::IsConnecting() const
{
	ADDTOCALLSTACK("CClient::IsConnecting");
	switch ( GetConnectType() )
	{
		case CONNECT_TELNET:
		case CONNECT_AXIS:
		case CONNECT_HTTP:
		case CONNECT_GAME:
			return false;

		default:
			return true;
	}
}

lpctstr CClient::GetConnectTypeStr(CONNECT_TYPE iType)
{
	switch (iType)
	{
		case CONNECT_NONE:		return "No connection";
		case CONNECT_UNK:		return "Just connected";
		case CONNECT_CRYPT:		return "ServerList or CharList?";
		case CONNECT_LOGIN:		return "ServerList";
		case CONNECT_GAME:		return "CharList/Game";
		case CONNECT_HTTP:		return "HTTP";
		case CONNECT_TELNET:	return "Telnet";
		case CONNECT_UOG:		return "UOG";
		case CONNECT_AXIS:		return "Axis";
		default:				return "Unknown";
	}
}

void CClient::SetConnectType( CONNECT_TYPE iType )
{
	ADDTOCALLSTACK("CClient::SetConnectType");
	m_iConnectType = iType;
	if ( iType == CONNECT_GAME )
	{
#ifndef _MTNETWORK
		HistoryIP& history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(GetPeer());
#else
		HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
#endif
		history.m_connecting--;
	}
}

//---------------------------------------------------------------------
// Push world display data to this client only.

bool CClient::addLoginErr(byte code)
{
	ADDTOCALLSTACK("CClient::addLoginErr");
	// code
	// 0 = no account
	// 1 = account used.
	// 2 = blocked.
	// 3 = no password
	// LOGIN_ERR_OTHER

	if (code == PacketLoginError::Success)
		return true;

	// console message to display for each login error code
	static lpctstr const sm_Login_ErrMsg[] =
	{
		"Account does not exist",
		"The account entered is already being used",
		"This account or IP is blocked",
		"The password entered is not correct",
		"Timeout / Wrong encryption / Unknown error",
		"Invalid client version. See the CLIENTVERSION setting in " SPHERE_FILE ".ini",
		"Invalid character selected (chosen character does not exist)",
		"AuthID is not correct. This normally means that the client did not log in via the login server",
		"The account details entered are invalid (username or password is too short, too long or contains invalid characters). This can sometimes be caused by incorrect/missing encryption keys",
		"The account details entered are invalid (username or password is too short, too long or contains invalid characters). This can sometimes be caused by incorrect/missing encryption keys",
		"Encryption error (packet length does not match what was expected)",
		"Encryption error (unknown encryption or bad login packet)",
		"Encrypted client not permitted. See the USECRYPT setting in " SPHERE_FILE ".ini",
		"Unencrypted client not permitted. See the USENOCRYPT setting in " SPHERE_FILE ".ini",
		"Another character on this account is already ingame",
		"Account is full. Cannot create a new character",
		"Character creation blocked.",
		"This IP is blocked",
		"The maximum number of clients has been reached. See the CLIENTMAX setting in " SPHERE_FILE ".ini",
		"The maximum number of guests has been reached. See the GUESTSMAX setting in " SPHERE_FILE ".ini",
		"The maximum number of password tries has been reached"
	};

	if (code >= CountOf(sm_Login_ErrMsg))
		code = PacketLoginError::Other;
	
	g_Log.EventWarn( "%x:Bad Login %d (%s)\n", GetSocketID(), code, sm_Login_ErrMsg[(size_t)(code)] );

	// translate the code into a code the client will understand
	switch (code)
	{
		case PacketLoginError::Invalid:
			code = PacketLoginError::Invalid;
			break;
		case PacketLoginError::InUse:
		case PacketLoginError::CharIdle:
			code = PacketLoginError::InUse;
			break;
		case PacketLoginError::Blocked:
		case PacketLoginError::BlockedIP:
		case PacketLoginError::MaxClients:
		case PacketLoginError::MaxGuests:
			code = PacketLoginError::Blocked;
			break;
		case PacketLoginError::BadPass:
		case PacketLoginError::BadAccount:
		case PacketLoginError::BadPassword:
			code = PacketLoginError::BadPass;
			break;
		case PacketLoginError::Other:
		case PacketLoginError::BadVersion:
		case PacketLoginError::BadCharacter:
		case PacketLoginError::BadAuthID:
		case PacketLoginError::BadEncLength:
		case PacketLoginError::EncCrypt:
		case PacketLoginError::EncNoCrypt:
		case PacketLoginError::TooManyChars:
		case PacketLoginError::MaxPassTries:
		case PacketLoginError::EncUnknown:
		default:
			code = PacketLoginError::Other;
			break;
	}

	if ( GetNetState()->m_clientVersion || GetNetState()->m_reportedVersion )	// only reply the packet to valid clients
		new PacketLoginError(this, static_cast<PacketLoginError::Reason>(code));
	GetNetState()->markReadClosed();
	return false;
}


void CClient::addSysMessage(lpctstr pszMsg) // System message (In lower left corner)
{
	ADDTOCALLSTACK("CClient::addSysMessage");
	if ( !pszMsg )
		return;

	if ( IsSetOF(OF_Flood_Protection) && ( GetPrivLevel() <= PLEVEL_Player )  )
	{
		if ( !strcmpi(pszMsg, m_zLastMessage) )
			return;

		if ( strlen(pszMsg) < SCRIPT_MAX_LINE_LEN )
			strcpy(m_zLastMessage, pszMsg);
	}

	addBarkParse(pszMsg, nullptr, HUE_TEXT_DEF, TALKMODE_SAY);
}


void CClient::addWebLaunch( lpctstr pPage )
{
	ADDTOCALLSTACK("CClient::addWebLaunch");
	// Direct client to a web page
	if ( !pPage || !pPage[0] )
		return;

	new PacketWebPage(this, pPage);
}

///////////////////////////////////////////////////////////////
// Login server.

bool CClient::addRelay( const CServerDef * pServ )
{
	ADDTOCALLSTACK("CClient::addRelay");
	EXC_TRY("addRelay");

	// Tell the client to play on this server.
	if ( !pServ )
		return false;

	CSocketAddressIP ipAddr = pServ->m_ip;

	if ( ipAddr.IsLocalAddr())	// local server address not yet filled in.
	{
		ipAddr = m_net->m_socket.GetSockName();
		DEBUG_MSG(( "%x:Login_Relay to %s\n", GetSocketID(), ipAddr.GetAddrStr() ));
	}

	if ( GetPeer().IsLocalAddr() || GetPeer().IsSameIP( ipAddr ))	// weird problem with client relaying back to self.
	{
		DEBUG_MSG(( "%x:Login_Relay loopback to server %s\n", GetSocketID(), ipAddr.GetAddrStr() ));
		ipAddr.SetAddrIP( SOCKET_LOCAL_ADDRESS );
	}

	EXC_SET_BLOCK("customer id");
	dword dwAddr = ipAddr.GetAddrIP();
	dword dwCustomerId = 0x7f000001;
	if ( g_Cfg.m_fUseAuthID )
	{
		CSString sCustomerID(pServ->GetName());
		sCustomerID.Add(GetAccount()->GetName());

		dwCustomerId = z_crc32(0L, Z_NULL, 0);
		dwCustomerId = z_crc32(dwCustomerId, reinterpret_cast<const z_Bytef *>(sCustomerID.GetPtr()), (z_uInt)sCustomerID.GetLength());

		GetAccount()->m_TagDefs.SetNum("customerid", dwCustomerId);
	}

	DEBUG_MSG(( "%x:Login_Relay to server %s with AuthId %u\n", GetSocketID(), ipAddr.GetAddrStr(), dwCustomerId ));

	EXC_SET_BLOCK("server relay packet");
	new PacketServerRelay(this, dwAddr, pServ->m_ip.GetPort(), dwCustomerId);
	
	m_Targ_Mode = CLIMODE_SETUP_RELAY;
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("account '%s'\n", GetAccount() ? GetAccount()->GetName() : "");
	EXC_DEBUG_END;
	return false;
}

bool CClient::Login_Relay( uint iRelay ) // Relay player to a selected IP
{
	ADDTOCALLSTACK("CClient::Login_Relay");
	// Client wants to be relayed to another server. XCMD_ServerSelect
	// iRelay = 0 = this local server.

	// Sometimes we get an extra 0x80 ???
	if ( iRelay >= 0x80 )
	{
		iRelay -= 0x80;
	}

	// >= 1.26.00 clients list Gives us a 1 based index for some reason.
	if ( iRelay > 0 )
		iRelay --;

	CServerRef pServ;
	if ( iRelay <= 0 )
	{
		pServ = &g_Serv;	// we always list ourself first.
	}
	else
	{
		iRelay --;
		pServ = g_Cfg.Server_GetDef(iRelay);
		if ( pServ == nullptr )
		{
			DEBUG_ERR(( "%x:Login_Relay BAD index! %u\n", GetSocketID(), iRelay ));
			return false;
		}
	}

	return addRelay( pServ );
}

byte CClient::Login_ServerList( const char * pszAccount, const char * pszPassword )
{
	ADDTOCALLSTACK("CClient::Login_ServerList");
	// XCMD_ServersReq
	// Initial login (Login on "loginserver", new format)
	// If the messages are garbled make sure they are terminated to correct length.

	tchar szAccount[MAX_ACCOUNT_NAME_SIZE+3];
	size_t iLenAccount = Str_GetBare( szAccount, pszAccount, sizeof(szAccount)-1 );
	if ( iLenAccount > MAX_ACCOUNT_NAME_SIZE )
		return( PacketLoginError::BadAccount );
	if ( iLenAccount != strlen(pszAccount))
		return( PacketLoginError::BadAccount );

	tchar szPassword[MAX_NAME_SIZE+3];
	size_t iLenPassword = Str_GetBare( szPassword, pszPassword, sizeof( szPassword )-1 );
	if ( iLenPassword > MAX_NAME_SIZE )
		return( PacketLoginError::BadPassword );
	if ( iLenPassword != strlen(pszPassword))
		return( PacketLoginError::BadPassword );

	// don't bother logging in yet.
	// Give the server list to everyone.
	// if ( LogIn( pszAccount, pszPassword ) )
	//   return( PacketLoginError::BadPass );
	CSString sMsg;
	byte lErr = LogIn( pszAccount, pszPassword, sMsg );
	if ( lErr != PacketLoginError::Success )
	{
		return( lErr );
	}

	new PacketServerList(this);

	m_Targ_Mode = CLIMODE_SETUP_SERVERS;
	return( PacketLoginError::Success );
}

//*****************************************

bool CClient::OnRxConsoleLoginComplete()
{
	ADDTOCALLSTACK("CClient::OnRxConsoleLoginComplete");
	if ( GetConnectType() != CONNECT_TELNET )
		return false;
	if ( !GetPeer().IsValidAddr() )
		return false;

	if ( GetPrivLevel() < PLEVEL_Admin )	// this really should not happen.
	{
		SysMessagef("%s\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_NO_ADMIN));
		return false;
	}

	SysMessagef("%s '%s' ('%s')\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_WELCOME_2), GetName(), GetPeerStr());
	return true;
}

bool CClient::OnRxConsole( const byte * pData, size_t iLen )
{
	ADDTOCALLSTACK("CClient::OnRxConsole");
	// A special console version of the client. (Not game protocol)
	if ( !iLen || ( GetConnectType() != CONNECT_TELNET ))
		return false;

	if ( IsSetEF( EF_AllowTelnetPacketFilter ) )
	{
		bool fFiltered = xPacketFilter(pData, iLen);
		if ( fFiltered )
			return fFiltered;
	}

	while ( iLen -- )
	{
		int iRet = OnConsoleKey( m_Targ_Text, *pData++, GetAccount() != nullptr );
		if ( ! iRet )
			return false;
		if ( iRet == 2 )
		{
			if ( GetAccount() == nullptr )
			{
				if ( !m_zLogin[0] )
				{
					if ( (uint)(m_Targ_Text.GetLength()) > (CountOf(m_zLogin) - 1) )
					{
						SysMessage("Login:\n");
					}
					else
					{
						strcpy(m_zLogin, m_Targ_Text);
						SysMessage("Password:\n");
					}
					m_Targ_Text.Empty();
				}
				else
				{
					CSString sMsg;

					CAccount * pAccount = g_Accounts.Account_Find(m_zLogin);
					if (( pAccount == nullptr ) || ( pAccount->GetPrivLevel() < PLEVEL_Admin ))
					{
						SysMessagef("%s\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_NOT_PRIV));
						m_Targ_Text.Empty();
						return false;
					}
					if ( LogIn(m_zLogin, m_Targ_Text, sMsg ) == PacketLoginError::Success )
					{
						m_Targ_Text.Empty();
						return OnRxConsoleLoginComplete();
					}
					else if ( ! sMsg.IsEmpty())
					{
						SysMessage( sMsg );
						return false;
					}
					m_Targ_Text.Empty();
				}
				return true;
			}
			else
			{
				iRet = g_Serv.OnConsoleCmd( m_Targ_Text, this );

				if (g_Cfg.m_fTelnetLog && GetPrivLevel() >= g_Cfg.m_iCommandLog)
					g_Log.Event(LOGM_GM_CMDS, "%x:'%s' commands '%s'=%d\n", GetSocketID(), GetName(), static_cast<lpctstr>(m_Targ_Text), iRet);
			}
		}
	}
	return true;
}

bool CClient::OnRxAxis( const byte * pData, size_t iLen )
{
	ADDTOCALLSTACK("CClient::OnRxAxis");
	if ( !iLen || ( GetConnectType() != CONNECT_AXIS ))
		return false;

	while ( iLen -- )
	{
		int iRet = OnConsoleKey( m_Targ_Text, *pData++, GetAccount() != nullptr );
		if ( ! iRet )
			return false;
		if ( iRet == 2 )
		{
			if ( GetAccount() == nullptr )
			{
				if ( !m_zLogin[0] )
				{
					if ( (uint)(m_Targ_Text.GetLength()) <= (CountOf(m_zLogin) - 1) )
						strcpy(m_zLogin, m_Targ_Text);
					m_Targ_Text.Empty();
				}
				else
				{
					CSString sMsg;

					CAccount * pAccount = g_Accounts.Account_Find(m_zLogin);
					if (( pAccount == nullptr ) || ( pAccount->GetPrivLevel() < PLEVEL_Counsel ))
					{
						SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_NOT_PRIV));
						m_Targ_Text.Empty();
						return false;
					}
					if ( LogIn(m_zLogin, m_Targ_Text, sMsg ) == PacketLoginError::Success )
					{
						m_Targ_Text.Empty();
						if ( GetPrivLevel() < PLEVEL_Counsel )
						{
							SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_NOT_PRIV));
							return false;
						}
						if (GetPeer().IsValidAddr())
						{
							CScriptTriggerArgs Args;
							Args.m_VarsLocal.SetStrNew("Account",GetName());
							Args.m_VarsLocal.SetStrNew("IP",GetPeer().GetAddrStr());
							TRIGRET_TYPE tRet = TRIGRET_RET_DEFAULT;
							r_Call("f_axis_preload", this, &Args, nullptr, &tRet);
							if ( tRet == TRIGRET_RET_FALSE )
								return false;
							if ( tRet == TRIGRET_RET_TRUE )
							{
								SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_DENIED));
								return false;
							}

							time_t dateChange;
							dword dwSize;
							if ( ! CSFileList::ReadFileInfo( "Axis.db", dateChange, dwSize ))
							{
								SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_INFO_ERROR));
								return false;
							}

							CSFile FileRead;
							if ( ! FileRead.Open( "Axis.db", OF_READ|OF_BINARY ))
							{
								SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_FILE_ERROR));
								return false;
							}

							tchar szTmp[8*1024];
							PacketWeb packet;
							for (;;)
							{
								size_t iLength = FileRead.Read( szTmp, sizeof( szTmp ) );
								if ( iLength <= 0 )
									break;
								packet.setData((byte*)szTmp, iLength);
								packet.send(this);
								dwSize -= (dword)iLength;
								if ( dwSize <= 0 )
									break;
							}
							return true;
						}
						return false;
					}
					else if ( ! sMsg.IsEmpty())
					{
						SysMessagef("\"MSG:%s\"", (lpctstr)sMsg);
						return false;
					}
					m_Targ_Text.Empty();
				}
				return true;
			}
		}
	}
	return true;
}

bool CClient::OnRxPing( const byte * pData, size_t iLen )
{
	ADDTOCALLSTACK("CClient::OnRxPing");
	// packet iLen < 5
	// UOMon should work like this.
	// RETURN: true = keep the connection open.
	if ( GetConnectType() != CONNECT_UNK )
		return false;

	if ( !iLen || iLen > 4 )
		return false;

	switch ( pData[0] )
	{
		// Remote Admin Console
		case '\x1':
		case ' ':
		{
			if ( (iLen > 1) &&
				 (iLen != 2 || pData[1] != '\n') &&
				 (iLen != 3 || pData[1] != '\r' || pData[2] != '\n') &&
				 (iLen != 3 || pData[1] != '\n' || pData[2] != '\0') )
				break;

			// enter into remote admin mode. (look for password).
			SetConnectType( CONNECT_TELNET );
			m_zLogin[0] = 0;
			SysMessagef("%s %s Admin Telnet\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_WELCOME_1), g_Serv.GetName());

			if ( g_Cfg.m_fLocalIPAdmin )
			{
				// don't bother logging in if local.

				if ( GetPeer().IsLocalAddr() )
				{
					CAccount * pAccount = g_Accounts.Account_Find("Administrator");
					if ( !pAccount )
						pAccount = g_Accounts.Account_Find("RemoteAdmin");
					if ( pAccount )
					{
						CSString sMsg;
						byte lErr = LogIn( pAccount, sMsg );
						if ( lErr != PacketLoginError::Success )
						{
							if ( lErr != PacketLoginError::Invalid )
								SysMessage( sMsg );
							return false;
						}
						return OnRxConsoleLoginComplete();
					}
				}
			}

			SysMessage("Login:\n");
			return true;
		}

		//Axis Connection
		case '@':
		{
			if ( (iLen > 1) &&
				 (iLen != 2 || pData[1] != '\n') &&
				 (iLen != 3 || pData[1] != '\r' || pData[2] != '\n') &&
				 (iLen != 3 || pData[1] != '\n' || pData[2] != '\0') )
				break;

			// enter into Axis mode. (look for password).
			SetConnectType( CONNECT_AXIS );
			m_zLogin[0] = 0;

			time_t dateChange;
			dword dwSize = 0;
			CSFileList::ReadFileInfo( "Axis.db", dateChange, dwSize );
			SysMessagef("%u",dwSize);
			return true;
		}

		// ConnectUO Status
		case 0xF1:
		{
			// ConnectUO sends a 4-byte packet when requesting status info
			// byte Cmd		(0xF1)
			// word Unk		(0x04)
			// byte SubCmd	(0xFF)

			if ( iLen != MAKEWORD( pData[2], pData[1] ) )
				break;

			if ( pData[3] != 0xFF )
				break;

			if ( g_Cfg.m_fCUOStatus == false )
			{
				g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:CUO Status request from %s has been rejected.\n", GetSocketID(), GetPeerStr());
				return false;
			}

			// enter 'remote admin mode'
			SetConnectType( CONNECT_TELNET );

			g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:CUO Status request from %s\n", GetSocketID(), GetPeerStr());

			SysMessage( g_Serv.GetStatusString( 0x25 ) );

			// exit 'remote admin mode'
			SetConnectType( CONNECT_UNK );
			return false;
		}

		// UOGateway Status
		case 0xFF:
		case 0x7F:
		case 0x22:
		{
			if ( iLen > 1 )
				break;

			if ( g_Cfg.m_fUOGStatus == false )
			{
				g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:UOG Status request from %s has been rejected.\n", GetSocketID(), GetPeerStr());
				return false;
			}

			// enter 'remote admin mode'
			SetConnectType( CONNECT_TELNET );

			g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:UOG Status request from %s\n", GetSocketID(), GetPeerStr());

			if (pData[0] == 0x7F)
				SetConnectType( CONNECT_UOG );

			SysMessage( g_Serv.GetStatusString( 0x22 ) );

			// exit 'remote admin mode'
			SetConnectType( CONNECT_UNK );
			return false;
		}
	}

	g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Unknown/invalid ping data '0x%x' from %s (Len: %" PRIuSIZE_T ")\n", GetSocketID(), pData[0], GetPeerStr(), iLen);
	return false;
}

bool CClient::OnRxWebPageRequest( byte * pRequest, size_t iLen )
{
	ADDTOCALLSTACK("CClient::OnRxWebPageRequest");
	// Seems to be a web browser pointing at us ? typical stuff :
	if ( GetConnectType() != CONNECT_HTTP )
		return false;

	// ensure request is null-terminated (if the request is well-formed, we are overwriting a trailing \n here)
	pRequest[iLen - 1] = '\0';

	if ( strlen(reinterpret_cast<char *>(pRequest)) > 1024 )			// too long request
		return false;

	if ( !strpbrk( reinterpret_cast<char *>(pRequest), " \t\012\015" ) )	// malformed request
		return false;

	tchar * ppLines[16];
	size_t iQtyLines = Str_ParseCmds(reinterpret_cast<char *>(pRequest), ppLines, CountOf(ppLines), "\r\n");
	if (( iQtyLines < 1 ) || ( iQtyLines >= 15 ))	// too long request
		return false;

	// Look for what they want to do with the connection.
	bool fKeepAlive = false;
	CSTime dateIfModifiedSince;
	tchar * pszReferer = nullptr;
	size_t stContentLength = 0;
	for ( size_t j = 1; j < iQtyLines; j++ )
	{
		tchar	*pszArgs = Str_TrimWhitespace(ppLines[j]);
		if ( !strnicmp(pszArgs, "Connection:", 11 ) )
		{
			pszArgs += 11;
			GETNONWHITESPACE(pszArgs);
			if ( !strnicmp(pszArgs, "Keep-Alive", 10) )
				fKeepAlive = true;
		}
		else if ( !strnicmp(pszArgs, "Referer:", 8) )
		{
			pszReferer = pszArgs+8;
		}
		else if ( !strnicmp(pszArgs, "Content-Length:", 15) )
		{
			pszArgs += 15;
			GETNONWHITESPACE(pszArgs);
			stContentLength = strtoul(pszArgs, nullptr, 10);
		}
		else if ( ! strnicmp( pszArgs, "If-Modified-Since:", 18 ))
		{
			// If-Modified-Since: Fri, 17 Dec 1999 14:59:20 GMT\r\n
			pszArgs += 18;
			dateIfModifiedSince.Read(pszArgs);
		}
	}

	tchar * ppRequest[4];
	size_t iQtyArgs = Str_ParseCmds(ppLines[0], ppRequest, CountOf(ppRequest), " ");
	if (( iQtyArgs < 2 ) || ( strlen(ppRequest[1]) >= _MAX_PATH ))
		return false;

	if ( strchr(ppRequest[1], '\r') || strchr(ppRequest[1], 0x0c) )
		return false;

	// if the client hasn't requested a keep alive, we must act as if they had
	// when async networking is used, otherwise data may not be completely sent
	if ( fKeepAlive == false )
	{
		fKeepAlive = m_net->isAsyncMode();

		// must switch to a blocking socket when the connection is not being kept
		// alive, or else pending data will be lost when the socket shuts down

		if ( fKeepAlive == false )
			m_net->m_socket.SetNonBlocking(false);
	}

	linger llinger;
	llinger.l_onoff = 1;
	llinger.l_linger = 500;	// in mSec
	m_net->m_socket.SetSockOpt(SO_LINGER, reinterpret_cast<char *>(&llinger), sizeof(linger));
	char nbool = true;
	m_net->m_socket.SetSockOpt(SO_KEEPALIVE, &nbool, sizeof(char));

	// disable NAGLE algorythm for data compression
	nbool = true;
	m_net->m_socket.SetSockOpt( TCP_NODELAY, &nbool, sizeof(char), IPPROTO_TCP);
	
	if ( memcmp(ppLines[0], "POST", 4) == 0 )
	{
		if ( stContentLength > strlen(ppLines[iQtyLines-1]) )
			return false;

		// POST /--WEBBOT-SELF-- HTTP/1.1
		// Referer: http://127.0.0.1:2593/spherestatus.htm
		// Content-Type: application/x-www-form-urlencoded
		// Host: 127.0.0.1:2593
		// Content-Length: 29
		// T1=stuff1&B1=Submit&T2=stuff2

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%x:HTTP Page Post '%s'\n", GetSocketID(), static_cast<lpctstr>(ppRequest[1]));

		CWebPageDef	*pWebPage = g_Cfg.FindWebPage(ppRequest[1]);
		if ( !pWebPage )
			pWebPage = g_Cfg.FindWebPage(pszReferer);
		if ( pWebPage )
		{
			if ( pWebPage->ServPagePost(this, ppRequest[1], ppLines[iQtyLines-1], stContentLength) )
			{
				if ( fKeepAlive )
					return true;
				return false;
			}
			return false;
		}
	}
	else if ( !memcmp(ppLines[0], "GET", 3) )
	{
		// GET /pagename.htm HTTP/1.1\r\n
		// If-Modified-Since: Fri, 17 Dec 1999 14:59:20 GMT\r\n
		// Host: localhost:2593\r\n
		// \r\n

		tchar szPageName[_MAX_PATH];
		if ( !Str_GetBare( szPageName, Str_TrimWhitespace(ppRequest[1]), sizeof(szPageName), "!\"#$%&()*,:;<=>?[]^{|}-+'`" ) )
			return false;

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%x:HTTP Page Request '%s', alive=%d\n", GetSocketID(), static_cast<lpctstr>(szPageName), fKeepAlive);
		if ( CWebPageDef::ServPage(this, szPageName, &dateIfModifiedSince) )
		{
			if ( fKeepAlive )
				return true;
			return false;
		}
	}


	return false;
}

bool CClient::xProcessClientSetup( CEvent * pEvent, size_t iLen )
{
	ADDTOCALLSTACK("CClient::xProcessClientSetup");
	// If this is a login then try to process the data and figure out what client it is.
	// try to figure out which client version we are talking to.
	// (CEvent::ServersReq) or (CEvent::CharListReq)
	// NOTE: Anything else we get at this point is tossed !
	ASSERT( GetConnectType() == CONNECT_CRYPT );
	ASSERT( !m_Crypt.IsInit());
	ASSERT( pEvent != nullptr );
	ASSERT( iLen > 0 );

	// Try all client versions on the msg.
	CEvent bincopy;		// in buffer. (from client)
	ASSERT( iLen <= sizeof(bincopy));
	memcpy( bincopy.m_Raw, pEvent->m_Raw, iLen );

	if ( !m_Crypt.Init( m_net->m_seed, bincopy.m_Raw, iLen, GetNetState()->isClientKR() ) )
	{
		DEBUG_MSG(( "%x:Odd login message length %" PRIuSIZE_T "?\n", GetSocketID(), iLen ));
#ifdef _DEBUG
		xRecordPacketData(this, (const byte *)pEvent, iLen, "client->server");
#endif
		addLoginErr( PacketLoginError::BadEncLength );
		return false;
	}
	
	GetNetState()->detectAsyncMode();
	SetConnectType( m_Crypt.GetConnectType() );

	if ( !xCanEncLogin() )
	{
		addLoginErr((uchar)((m_Crypt.GetEncryptionType() == ENC_NONE? PacketLoginError::EncNoCrypt : PacketLoginError::EncCrypt) ));
		return false;
	}
	else if ( m_Crypt.GetConnectType() == CONNECT_LOGIN && !xCanEncLogin(true) )
	{
		addLoginErr( PacketLoginError::BadVersion );
		return false;
	}
	
	byte lErr = PacketLoginError::EncUnknown;
	
	if (!m_Crypt.Decrypt( pEvent->m_Raw, bincopy.m_Raw, MAX_BUFFER, iLen ))
    {
        g_Log.EventError("NET-IN: xProcessClientSetup failed (Decrypt).\n");
        return false;
    }
	
	tchar szAccount[MAX_ACCOUNT_NAME_SIZE+3];

	switch ( pEvent->Default.m_Cmd )
	{
		case XCMD_ServersReq:
		{
			if ( iLen < sizeof( pEvent->ServersReq ))
				return false;

			lErr = Login_ServerList( pEvent->ServersReq.m_acctname, pEvent->ServersReq.m_acctpass );
			if ( lErr == PacketLoginError::Success )
			{
				Str_GetBare( szAccount, pEvent->ServersReq.m_acctname, sizeof(szAccount)-1 );
				CAccount * pAcc = g_Accounts.Account_Find( szAccount );
				if (pAcc)
				{
					pAcc->m_TagDefs.SetNum("clientversion", m_Crypt.GetClientVer());
					pAcc->m_TagDefs.SetNum("reportedcliver", GetNetState()->getReportedVersion());
				}
				else
				{
					// If i can't set the tag is better to stop login now
					lErr = PacketLoginError::Invalid;
				}
			}

			break;
		}

		case XCMD_CharListReq:
		{
			if ( iLen < sizeof( pEvent->CharListReq ))
				return false;

			lErr = Setup_ListReq( pEvent->CharListReq.m_acctname, pEvent->CharListReq.m_acctpass, true );
			if ( lErr == PacketLoginError::Success )
			{
				// pass detected client version to the game server to make valid cliver used
				Str_GetBare( szAccount, pEvent->CharListReq.m_acctname, sizeof(szAccount)-1 );
				CAccount * pAcc = g_Accounts.Account_Find( szAccount );
				if (pAcc)
				{
					dword tmSid = 0x7f000001;
					dword tmVer = (dword)(pAcc->m_TagDefs.GetKeyNum("clientversion"));
					dword tmVerReported = (dword)(pAcc->m_TagDefs.GetKeyNum("reportedcliver"));
					pAcc->m_TagDefs.DeleteKey("clientversion");
					pAcc->m_TagDefs.DeleteKey("reportedcliver");

					if ( g_Cfg.m_fUseAuthID )
					{
						tmSid = (dword)(pAcc->m_TagDefs.GetKeyNum("customerid"));
						pAcc->m_TagDefs.DeleteKey("customerid");
					}

					DEBUG_MSG(("%x:xProcessClientSetup for %s, with AuthId %u and CliVersion %u / CliVersionReported %u\n", GetSocketID(), pAcc->GetName(), tmSid, tmVer, tmVerReported));

					if ( tmSid != 0 && tmSid == pEvent->CharListReq.m_Account )
					{
						// request client version if the client has not reported it to server yet
						if ( (tmVerReported == 0) && (tmVer > 1260400) )
                        {   // if we send this packet to clients < 1.26.04.00 we'll desynchronize the stream and break the login process
							new PacketClientVersionReq(this);
                        }

						if ( tmVerReported != 0 )
						{
							GetNetState()->m_reportedVersion = tmVerReported;
						}
						else if ( tmVer != 0 )
						{
							m_Crypt.SetClientVerEnum(tmVer, false);
							GetNetState()->m_clientVersion = tmVer;
						}

						// client version change may toggle async mode, it's important to flush pending data to the client before this happens
						GetNetState()->detectAsyncMode();

						if ( !xCanEncLogin(true) )
							lErr = PacketLoginError::BadVersion;
					}
					else
					{
						lErr = PacketLoginError::BadAuthID;
					}
				}
				else
				{
					lErr = PacketLoginError::Invalid;
				}
			}

			break;
		}

#ifdef _DEBUG
		default:
		{
			DEBUG_ERR(("Unknown/bad packet to receive at this time: 0x%X\n", pEvent->Default.m_Cmd));
		}
#endif
	}
	
	xRecordPacketData(this, (const byte *)pEvent, iLen, "client->server");

	if ( lErr != PacketLoginError::Success )	// it never matched any crypt format.
	{
		addLoginErr( lErr );
	}

	return( lErr == PacketLoginError::Success );
}

bool CClient::xCanEncLogin(bool bCheckCliver)
{
	ADDTOCALLSTACK("CClient::xCanEncLogin");
	if ( !bCheckCliver )
	{
		if ( m_Crypt.GetEncryptionType() == ENC_NONE )
			return ( g_Cfg.m_fUsenocrypt ); // Server don't want no-crypt clients 
		
		return ( g_Cfg.m_fUsecrypt ); // Server don't want crypt clients
	}
	else
	{
		if ( !g_Serv.m_ClientVersion.GetClientVer() ) // Any Client allowed
			return true;
		
		if ( m_Crypt.GetEncryptionType() != ENC_NONE )
			return( m_Crypt.GetClientVer() == g_Serv.m_ClientVersion.GetClientVer() );
		else
			return true;	// if unencrypted we check that later
	}
}
