// Login and low level stuff for the client.

#include "../../common/crypto/CCrypto.h"
#include "../../common/crypto/CHuffman.h"
#include "../../common/sphere_library/CSFileList.h"
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../../network/CIPHistoryManager.h"
#include "../../network/CNetworkManager.h"
#include "../../network/send.h"
#include "../CServer.h"
#include "CClient.h"

#include <zlib/zlib.h>


/////////////////////////////////////////////////////////////////
// -CClient stuff.

uint CClient::xCompress( byte * pOutput, const byte * pInput, uint outLen, uint inLen ) // static
{
	ADDTOCALLSTACK("CClient::xCompress");
	// The game server will compress the outgoing data to the clients.
	return CHuffman::Compress( pOutput, pInput, outLen, inLen );
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

    auto _IsFullyConnectedType = [](const CONNECT_TYPE typ) -> bool {
        switch (typ)
        {
        case CONNECT_GAME:
        case CONNECT_HTTP:
        case CONNECT_TELNET:
        case CONNECT_UOG:
        case CONNECT_AXIS:
            return true;
        default:
            return false;
        }
    };

	if (_IsFullyConnectedType(iType) && !_IsFullyConnectedType(m_iConnectType))
	{
		HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
		-- history.m_connecting;
	}
	m_iConnectType = iType;

/*
	m_iConnectType = iType;
	if ( iType == CONNECT_GAME )
	{
		HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
		-- history.m_connecting;
	}
*/
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
	static lpctstr constexpr sm_Login_ErrMsg[] =
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
		"Encryption error: packet length does not match what was expected",
		"Encryption error: bad login packet or unknown encryption (encryption key missing in " SPHERE_FILE "Crypt.ini?)",
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

	if (code >= ARRAY_COUNT(sm_Login_ErrMsg))
		code = PacketLoginError::Other;

	g_Log.EventWarn( "%x:Bad Login %d. %s.\n", GetSocketID(), code, sm_Login_ErrMsg[(size_t)code] );

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

	if ( GetNetState()->m_clientVersionNumber || GetNetState()->m_reportedVersionNumber )	// only reply the packet to valid clients
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
		if ( !strnicmp(pszMsg, m_zLastMessage, SCRIPT_MAX_LINE_LEN) )
			return;

		Str_CopyLimitNull(m_zLastMessage, pszMsg, SCRIPT_MAX_LINE_LEN);
	}

	addBarkParse(pszMsg, nullptr, HUE_TEXT_DEF, TALKMODE_SAY);
}


void CClient::addWebLaunch( lpctstr pPage )
{
	ADDTOCALLSTACK("CClient::addWebLaunch");
	// Direct client to a web page
	if ( !pPage || !pPage[0] )
		return;
	SysMessageDefault(DEFMSG_WEB_BROWSER_START);
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
		ipAddr.SetAddrIP(m_net->m_socket.GetSockName().GetAddrIP());
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

		dwCustomerId = ::crc32(0L, Z_NULL, 0);
		dwCustomerId = ::crc32(dwCustomerId, reinterpret_cast<const Bytef *>(sCustomerID.GetBuffer()), (uInt)sCustomerID.GetLength());

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

bool CClient::OnRxConsole( const byte * pData, uint iLen )
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
					if ( (uint)(m_Targ_Text.GetLength()) > (sizeof(m_zLogin) - 1) )
					{
						SysMessage("Login:\n");
					}
					else
					{
						Str_CopyLimitNull(m_zLogin, m_Targ_Text, sizeof(m_zLogin));
						SysMessage("Password:\n");
					}
					m_Targ_Text.Clear();
				}
				else
				{
					CAccount * pAccount = g_Accounts.Account_Find(m_zLogin);
					if (( pAccount == nullptr ) || ( pAccount->GetPrivLevel() < PLEVEL_Admin ))
					{
						SysMessagef("%s\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_NOT_PRIV));
						m_Targ_Text.Clear();
						return false;
					}

					CSString sMsg;
					if ( LogIn(m_zLogin, m_Targ_Text, sMsg ) == PacketLoginError::Success )
					{
						m_Targ_Text.Clear();
						return OnRxConsoleLoginComplete();
					}
					else if ( ! sMsg.IsEmpty())
					{
						SysMessage( sMsg );
						return false;
					}
					m_Targ_Text.Clear();
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

bool CClient::OnRxAxis( const byte * pData, uint iLen )
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
					if ((uint)(m_Targ_Text.GetLength()) <= (sizeof(m_zLogin) - 1))
					{
						Str_CopyLimitNull(m_zLogin, m_Targ_Text, sizeof(m_zLogin));
					}
					m_Targ_Text.Clear();
				}
				else
				{
					CAccount * pAccount = g_Accounts.Account_Find(m_zLogin);
					if (( pAccount == nullptr ) || ( pAccount->GetPrivLevel() < PLEVEL_Counsel ))
					{
						SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_NOT_PRIV));
						m_Targ_Text.Clear();
						return false;
					}

					CSString sMsg;
					if ( LogIn(m_zLogin, m_Targ_Text, sMsg ) == PacketLoginError::Success )
					{
						m_Targ_Text.Clear();
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
								int iLength = FileRead.Read( szTmp, sizeof( szTmp ) );
								if ( iLength <= 0 )
									break;
								packet.setData((byte*)szTmp, (uint)iLength);
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
						SysMessagef("\"MSG:%s\"", sMsg.GetBuffer());
						return false;
					}
					m_Targ_Text.Clear();
				}
				return true;
			}
		}
	}
	return true;
}

bool CClient::OnRxPing( const byte * pData, uint iLen )
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

	g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Unknown/invalid ping data '0x%x' from %s (Len: %u)\n", GetSocketID(), pData[0], GetPeerStr(), iLen);
	return false;
}

bool CClient::OnRxWebPageRequest( byte * pRequest, size_t uiLen )
{
	ADDTOCALLSTACK("CClient::OnRxWebPageRequest");

    // Seems to be a web browser pointing at us ? typical stuff :
	if ( GetConnectType() != CONNECT_HTTP )
		return false;

    if (uiLen > HTTPREQ_MAX_SIZE)    // request too long
        goto httpreq_err_long;

	// ensure request is null-terminated (if the request is well-formed, we are overwriting a trailing \n here)
	pRequest[uiLen - 1] = '\0';

    uiLen = strlen(reinterpret_cast<const char*>(pRequest));
    if (uiLen > HTTPREQ_MAX_SIZE)     // too long request
    {
    httpreq_err_long:
        g_Log.EventWarn("%x:Client sent HTTP request of length %" PRIuSIZE_T" exceeding max length limit of %d, ignoring.\n", GetNetState()->id(), uiLen, HTTPREQ_MAX_SIZE);
        return false;
    }

	if ( !strpbrk( reinterpret_cast<const char *>(pRequest), " \t\012\015" ) )    // malformed request
		return false;

	tchar * ppLines[16];
	int iQtyLines = Str_ParseCmds(reinterpret_cast<char *>(pRequest), ppLines, ARRAY_COUNT(ppLines), "\r\n");
	if (( iQtyLines < 1 ) || ( iQtyLines >= 15 ))	// too long request
		return false;

	// Look for what they want to do with the connection.
	bool fKeepAlive = false;
	CSTime dateIfModifiedSince;
	tchar * pszReferer = nullptr;
	uint uiContentLength = 0;
	for ( int j = 1; j < iQtyLines; ++j )
	{
		tchar *pszArgs = Str_TrimWhitespace(ppLines[j]);
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
            std::optional<uint> iconv = Str_ToU(pszArgs, 10);
            if (!iconv.has_value())
                continue;

            uiContentLength = *iconv;
		}
		else if ( ! strnicmp( pszArgs, "If-Modified-Since:", 18 ))
		{
			// If-Modified-Since: Fri, 17 Dec 1999 14:59:20 GMT\r\n
			pszArgs += 18;
			dateIfModifiedSince.Read(pszArgs);
		}
	}

	tchar * ppRequest[4];
	int iQtyArgs = Str_ParseCmds(ppLines[0], ppRequest, ARRAY_COUNT(ppRequest), " ");
	if (( iQtyArgs < 2 ) || ( strlen(ppRequest[1]) >= _MAX_PATH ))
		return false;

	if ( strchr(ppRequest[1], '\r') || strchr(ppRequest[1], 0x0c) )
		return false;

	int iSocketRet = 0;

	// if the client hasn't requested a keep alive, we must act as if they had
	// when async networking is used, otherwise data may not be completely sent
	if ( fKeepAlive == false )
	{
		fKeepAlive = m_net->isAsyncMode();

		// must switch to a blocking socket when the connection is not being kept
		// alive, or else pending data will be lost when the socket shuts down

		if (fKeepAlive == false)
		{
			iSocketRet = m_net->m_socket.SetNonBlocking(false);
			if (iSocketRet)
				return false;
		}
	}

	linger llinger{};
	llinger.l_onoff = 1;
	llinger.l_linger = 500;	// in mSec
	iSocketRet = m_net->m_socket.SetSockOpt(SO_LINGER, reinterpret_cast<char *>(&llinger), sizeof(linger));
	CheckReportNetAPIErr(iSocketRet, "CClient::Webpage.SO_LINGER");
	if (iSocketRet)
		return false;

	int iSockFlag = 1;
	iSocketRet = m_net->m_socket.SetSockOpt(SO_KEEPALIVE, &iSockFlag, sizeof(iSockFlag));
	CheckReportNetAPIErr(iSocketRet, "CClient::Webpage.SO_KEEPALIVE");
	if (iSocketRet)
		return false;

	// disable NAGLE algorythm for data compression
	iSockFlag = 1;
	iSocketRet = m_net->m_socket.SetSockOpt(TCP_NODELAY, &iSockFlag, sizeof(iSockFlag), IPPROTO_TCP);
	CheckReportNetAPIErr(iSocketRet, "CClient::Webpage.TCP_NODELAY");
	if (iSocketRet)
		return false;

	if ( memcmp(ppLines[0], "POST", 4) == 0 )
	{
		if (uiContentLength > strlen(ppLines[iQtyLines-1]) )
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
			if ( pWebPage->ServPagePost(this, ppRequest[1], ppLines[iQtyLines-1], uiContentLength) )
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

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%x:HTTP Page Request '%s', alive=%d\n", GetSocketID(), szPageName, fKeepAlive);
        CWebPageDef::ServPage(this, szPageName, &dateIfModifiedSince);
		/*if ( CWebPageDef::ServPage(this, szPageName, &dateIfModifiedSince) )
		{
			if ( fKeepAlive )
				return true;
			return false;
		}*/
	}


	return false;
}

bool CClient::xProcessClientSetup( CEvent * pEvent, uint uiLen )
{
	ADDTOCALLSTACK("CClient::xProcessClientSetup");
	// If this is a login then try to process the data and figure out what client it is.
	// try to figure out which client version we are talking to.
	// (CEvent::ServersReq) or (CEvent::CharListReq)
	// NOTE: Anything else we get at this point is tossed !
	ASSERT( GetConnectType() == CONNECT_CRYPT );
	ASSERT( !m_Crypt.IsInit());
	ASSERT( pEvent != nullptr );
	ASSERT( uiLen > 0 );

	// Try all client versions on the msg.
	if ( !m_Crypt.Init( m_net->m_seed, pEvent->m_Raw, uiLen, GetNetState()->isClientKR() ) )
	{
		DEBUG_MSG(( "%x:Odd login message length %u?\n", GetSocketID(), uiLen ));
#ifdef _DEBUG
		xRecordPacketData(this, pEvent->m_Raw, uiLen, "client->server");
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

    ASSERT(uiLen <= sizeof(CEvent));
    std::unique_ptr<CEvent> bincopy = std::make_unique<CEvent>();		// in buffer. (from client)
    memcpy(bincopy->m_Raw, pEvent->m_Raw, uiLen);
	if (!m_Crypt.Decrypt( pEvent->m_Raw, bincopy->m_Raw, MAX_BUFFER, uiLen ))
    {
        g_Log.EventError("NET-IN: xProcessClientSetup failed (Decrypt).\n");
        return false;
    }

    byte lErr = PacketLoginError::EncUnknown;
	tchar szAccount[MAX_ACCOUNT_NAME_SIZE+3];

	switch ( pEvent->Default.m_Cmd )
	{
		case XCMD_ServersReq:
		{
			if ( uiLen < sizeof( pEvent->ServersReq ))
				return false;

			lErr = Login_ServerList( pEvent->ServersReq.m_acctname, pEvent->ServersReq.m_acctpass );
			if ( lErr == PacketLoginError::Success )
			{
				Str_GetBare( szAccount, pEvent->ServersReq.m_acctname, sizeof(szAccount)-1 );
				CAccount * pAcc = g_Accounts.Account_Find( szAccount );
				if (pAcc)
				{
                    if (m_Crypt.GetClientVerNumber())
                        pAcc->m_TagDefs.SetNum("clientversion", m_Crypt.GetClientVerNumber());
					if (GetNetState()->getReportedVersion())
                        pAcc->m_TagDefs.SetNum("reportedcliver", GetNetState()->getReportedVersion());
                    else
                        new PacketClientVersionReq(this); // client version 0 ? ask for it.
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
			if ( uiLen < sizeof( pEvent->CharListReq ))
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
						if ( (tmVerReported == 0) && (tmVer > 1'26'04'00) )
                        {   // if we send this packet to clients < 1.26.04.00 we'll desynchronize the stream and break the login process
							new PacketClientVersionReq(this);
                        }

						if ( tmVerReported != 0 )
						{
							GetNetState()->m_reportedVersionNumber = tmVerReported;
						}
						else if ( tmVer != 0 )
						{
							m_Crypt.SetClientVerFromNumber(tmVer, false);
							GetNetState()->m_clientVersionNumber = tmVer;
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

	xRecordPacketData(this, pEvent->m_Raw, uiLen, "client->server");

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
		if ( !g_Serv.m_ClientVersion.GetClientVerNumber() ) // Any Client allowed
			return true;

		if ( m_Crypt.GetEncryptionType() != ENC_NONE )
			return ( m_Crypt.GetClientVerNumber() == g_Serv.m_ClientVersion.GetClientVerNumber() );
		else
			return true;	// if unencrypted we check that later
	}
}
