#include "CSocket.h"
#include "spherecom.h"

//		***		***			***
//
//		CSocketAddressIP
//
//		***		***			***

CSocketAddressIP::CSocketAddressIP()
{
	s_addr = INADDR_BROADCAST;
}
CSocketAddressIP::CSocketAddressIP( dword dwIP )
{
	s_addr = dwIP;
}

CSocketAddressIP::CSocketAddressIP(const char *ip)
{
	s_addr = inet_addr(ip);
}

bool CSocketAddressIP::operator==( const CSocketAddressIP & ip ) const
{
	return( IsSameIP( ip ) );
}

dword CSocketAddressIP::GetAddrIP() const
{
	return( s_addr );
}

void CSocketAddressIP::SetAddrIP( dword dwIP )
{
	s_addr = dwIP;
}

lpctstr CSocketAddressIP::GetAddrStr() const
{
	return inet_ntoa( *this );
}

void CSocketAddressIP::SetAddrStr( lpctstr pszIP )
{
	// NOTE: This must be in 1.2.3.4 format.
	s_addr = inet_addr( pszIP );
}

bool CSocketAddressIP::IsValidAddr() const
{
	// 0 and 0xffffffff=INADDR_NONE
	return( s_addr != INADDR_ANY && s_addr != INADDR_BROADCAST );
}

bool CSocketAddressIP::IsLocalAddr() const
{
	return( s_addr == 0 || s_addr == SOCKET_LOCAL_ADDRESS );
}

bool CSocketAddressIP::IsMatchIP( const CSocketAddressIP & ip ) const
{
	byte		ip1	[4];
	byte		ip2	[4];

	memcpy( ip1, (void*) &ip.s_addr,	4 );
	memcpy( ip2, (void*) &s_addr,		4 );

	for ( int i = 0; i < 4; i++ )
	{
		if ( ip1[i] == 255 || ip2[i] == 255 || ip1[i] == ip2[i] )
			continue;
		return false;
	}
	return true;
}

bool CSocketAddressIP::IsSameIP( const CSocketAddressIP & ip ) const
{
	return (ip.s_addr == s_addr);
}

bool CSocketAddressIP::SetHostStruct( const struct hostent * pHost )
{
	// Set the ip from the address name we looked up.
	if ( pHost == NULL ||
		pHost->h_addr_list == NULL ||
		pHost->h_addr == NULL )	// can't resolve the address.
	{
		return false;
	}
	SetAddrIP( *((dword*)( pHost->h_addr ))); // 0.1.2.3
	return true;
}

bool CSocketAddressIP::SetHostStr( lpctstr pszHostName )
{
	// try to resolve the host name with DNS for the true ip address.
	if ( pszHostName[0] == '\0' )
		return false;
	if ( IsDigit( pszHostName[0] ))
	{
		SetAddrStr( pszHostName ); // 0.1.2.3
		return true;
	}
	// NOTE: This is a blocking call !!!!
	return SetHostStruct( gethostbyname( pszHostName ));
}

//		***		***			***
//
//		CSocketAddress
//
//		***		***			***

CSocketAddress::CSocketAddress()
{
	// s_addr = INADDR_BROADCAST;
	m_port = 0;
}

CSocketAddress::CSocketAddress( in_addr dwIP, word uPort )
{
	s_addr = dwIP.s_addr;
	m_port = uPort;
}

CSocketAddress::CSocketAddress( CSocketAddressIP ip, word uPort )
{
	s_addr = ip.GetAddrIP();
	m_port = uPort;
}

CSocketAddress::CSocketAddress( dword dwIP, word uPort )
{
	s_addr = dwIP;
	m_port = uPort;
}

CSocketAddress::CSocketAddress( const sockaddr_in & SockAddrIn )
{
	SetAddrPort( SockAddrIn );
}

bool CSocketAddress::operator==( const CSocketAddress & SockAddr ) const
{
	return( GetAddrIP() == SockAddr.GetAddrIP() && GetPort() == SockAddr.GetPort() );
}

CSocketAddress & CSocketAddress::operator = ( const struct sockaddr_in & SockAddrIn )
{
	SetAddrPort(SockAddrIn);
	return( *this );
}

bool CSocketAddress::operator==( const struct sockaddr_in & SockAddrIn ) const
{
	return( GetAddrIP() == SockAddrIn.sin_addr.s_addr && GetPort() == ntohs( SockAddrIn.sin_port ) );
}

word CSocketAddress::GetPort() const
{
	return( m_port );
}

void CSocketAddress::SetPort( word wPort )
{
	m_port = wPort;
}

void CSocketAddress::SetPortStr( lpctstr pszPort )
{
	m_port = (word)(ATOI(pszPort));
}

bool CSocketAddress::SetPortExtStr( tchar * pszIP )
{
	// assume the port is at the end of the line.
	tchar * pszPort = strchr( pszIP, ',' );
	if ( pszPort == NULL )
	{
		pszPort = strchr( pszIP, ':' );
		if ( pszPort == NULL )
			return false;
	}

	SetPortStr( pszPort + 1 );
	*pszPort = '\0';
	return true;
}

// Port and address together.
bool CSocketAddress::SetHostPortStr( lpctstr pszIP )
{
	// NOTE: This is a blocking call !!!!
	tchar szIP[256];
	strncpy( szIP, pszIP, sizeof(szIP));
	SetPortExtStr( szIP );
	return SetHostStr( szIP );
}

// compare to sockaddr_in

struct sockaddr_in CSocketAddress::GetAddrPort() const
{
	struct sockaddr_in SockAddrIn;
	SockAddrIn.sin_family = AF_INET;
	SockAddrIn.sin_addr.s_addr = s_addr;
	SockAddrIn.sin_port = htons(m_port);
	return( SockAddrIn );
}
void CSocketAddress::SetAddrPort( const struct sockaddr_in & SockAddrIn )
{
	s_addr = SockAddrIn.sin_addr.s_addr;
	m_port = ntohs( SockAddrIn.sin_port );
}

//		***		***			***
//
//		CSSocket
//
//		***		***			***


CSSocket::CSSocket()
{
	Clear();
}

CSSocket::CSSocket( SOCKET socket )	// accept case.
{
	m_hSocket = socket;
}

CSSocket::~CSSocket()
{
	Close();
}

void CSSocket::SetSocket(SOCKET socket)
{
	Close();
	m_hSocket = socket;
}

void CSSocket::Clear()
{
	// Transfer the socket someplace else.
	m_hSocket = INVALID_SOCKET;
}

int CSSocket::GetLastError(bool bUseErrno)
{
#ifdef _WIN32
	UNREFERENCED_PARAMETER(bUseErrno);
	return( WSAGetLastError() );
#else
	return( !bUseErrno ? h_errno : errno );	// WSAGetLastError()
#endif
}

bool CSSocket::IsOpen() const
{
	return( m_hSocket != INVALID_SOCKET );
}

SOCKET CSSocket::GetSocket() const
{
	return( m_hSocket );
}

bool CSSocket::Create()
{
	return( Create( AF_INET, SOCK_STREAM, IPPROTO_TCP ) );
}

bool CSSocket::Create( int iAf, int iType, int iProtocol )
{
	ASSERT( ! IsOpen());
	m_hSocket = socket( iAf, iType, iProtocol );
	return( IsOpen());
}

int CSSocket::Bind( struct sockaddr_in * pSockAddrIn )
{
	return bind( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), sizeof(*pSockAddrIn));
}

int CSSocket::Bind( const CSocketAddress & SockAddr )
{
	struct sockaddr_in SockAddrIn = SockAddr.GetAddrPort();
	if ( SockAddr.IsLocalAddr())
	{
		SockAddrIn.sin_addr.s_addr = INADDR_ANY;	// use all addresses.
	}
	return( Bind( &SockAddrIn ));
}

int CSSocket::Listen( int iMaxBacklogConnections )
{
	return( listen( m_hSocket, iMaxBacklogConnections ));
}

int CSSocket::Connect( struct sockaddr_in * pSockAddrIn )
{
	// RETURN: 0 = success, else SOCKET_ERROR
	return connect( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), sizeof(*pSockAddrIn));
}

int CSSocket::Connect( const CSocketAddress & SockAddr )
{
	struct sockaddr_in SockAddrIn = SockAddr.GetAddrPort();
	return( Connect( &SockAddrIn ));
}

int CSSocket::Connect( const struct in_addr & ip, word wPort )
{
	CSocketAddress SockAddr( ip.s_addr, wPort );
	return( Connect( SockAddr ));
}

int CSSocket::Connect( lpctstr pszHostName, word wPort )
{
	CSocketAddress SockAddr;
	SockAddr.SetHostStr( pszHostName );
	SockAddr.SetPort( wPort );
	return( Connect( SockAddr ));
}

SOCKET CSSocket::Accept( struct sockaddr_in * pSockAddrIn ) const
{
	int len = sizeof(struct sockaddr_in);
	return accept( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), reinterpret_cast<socklen_t *>(&len));
}

SOCKET CSSocket::Accept( CSocketAddress & SockAddr ) const
{
	// RETURN: Error = hSocketClient < 0 || hSocketClient == INVALID_SOCKET 
	struct sockaddr_in SockAddrIn;
	SOCKET hSocket = Accept( &SockAddrIn );
	SockAddr.SetAddrPort( SockAddrIn );
	return( hSocket );
}

int CSSocket::Send( const void * pData, int len ) const
{
	// RETURN: length sent
	return( send( m_hSocket, static_cast<const char *>(pData), len, 0 ));
}

int CSSocket::Receive( void * pData, int len, int flags )
{
	// RETURN: length, <= 0 is closed or error.
	// flags = MSG_PEEK or MSG_OOB
	return( recv( m_hSocket, static_cast<char *>(pData), len, flags ));
}

int CSSocket::GetSockName( struct sockaddr_in * pSockAddrIn ) const
{
	// Get the address of the near end. (us)
	// RETURN: 0 = success
	int len = sizeof( *pSockAddrIn );
	return( getsockname( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), reinterpret_cast<socklen_t *>(&len) ));
}

CSocketAddress CSSocket::GetSockName() const
{
	struct sockaddr_in SockAddrIn;
	int iRet = GetSockName( &SockAddrIn );
	if ( iRet )
	{
		return( CSocketAddress( INADDR_BROADCAST, 0 ));	// invalid.
	}
	else
	{
		return( CSocketAddress( SockAddrIn ));
	}
}

int CSSocket::GetPeerName( struct sockaddr_in * pSockAddrIn ) const
{
	// Get the address of the far end.
	// RETURN: 0 = success
	int len = sizeof( *pSockAddrIn );
	return( getpeername( m_hSocket, reinterpret_cast<struct sockaddr *>(pSockAddrIn), reinterpret_cast<socklen_t *>(&len) ));
}

CSocketAddress CSSocket::GetPeerName( ) const
{
	struct sockaddr_in SockAddrIn;
	int iRet = GetPeerName( &SockAddrIn );
	if ( iRet )
	{
		return( CSocketAddress( INADDR_BROADCAST, 0 ));	// invalid.
	}
	else
	{
		return( CSocketAddress( SockAddrIn ));
	}
}

int CSSocket::SetSockOpt( int nOptionName, const void * optval, int optlen, int nLevel ) const
{
	// level = SOL_SOCKET and IPPROTO_TCP.
	return( setsockopt( m_hSocket, nLevel, nOptionName, reinterpret_cast<const char FAR *>(optval), optlen ));
}

int CSSocket::GetSockOpt( int nOptionName, void * optval, int * poptlen, int nLevel ) const
{
	return( getsockopt( m_hSocket, nLevel, nOptionName, reinterpret_cast<char FAR *>(optval), reinterpret_cast<socklen_t *>(poptlen)));
}

#ifdef _WIN32
	int CSSocket::IOCtlSocket(int icmd, dword * pdwArgs )
	{
		return ioctlsocket( m_hSocket, icmd, (DWORD*)pdwArgs );
	}

	int CSSocket::SendAsync( LPWSABUF lpBuffers, dword dwBufferCount, LPDWORD lpNumberOfBytesSent, dword dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine ) const
	{
		 // RETURN: length sent
		 return( WSASend( m_hSocket, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine ));
	}
	
	void CSSocket::ClearAsync()
	{
     	// TO BE CALLED IN CClient destructor !!!
		CancelIo(reinterpret_cast<HANDLE>(m_hSocket));
		SleepEx(1, TRUE);
	}

#else
	int CSSocket::IOCtlSocket( int icmd, int iVal )	// LINUX ?
	{
		return fcntl( m_hSocket, icmd, iVal );
	}

	int CSSocket::GetIOCtlSocketFlags( void )
	{
		return fcntl( m_hSocket, F_GETFL );
	}
#endif

void CSSocket::SetNonBlocking(bool bEnable)
{
#ifdef _WIN32
	DWORD lVal = bEnable? 1 : 0;	// 0 =  block
	ioctlsocket(m_hSocket, FIONBIO, &lVal);
#else
	if (bEnable)
		fcntl(m_hSocket, F_SETFL, GetIOCtlSocketFlags()|O_NONBLOCK);
	else
		fcntl(m_hSocket, F_SETFL, GetIOCtlSocketFlags()&~O_NONBLOCK);
#endif
}

void CSSocket::Close()
{
	if ( ! IsOpen())
		return;

	CSSocket::CloseSocket( m_hSocket );
	Clear();
}

void CSSocket::CloseSocket( SOCKET hClose )
{
	shutdown( hClose, 2 );
#ifdef _WIN32
	closesocket( hClose );
#else
	close( hClose ); // SD_BOTH
#endif
}

short CSSocket::GetProtocolIdByName( lpctstr pszName )
{
	protoent * ppe;

	ppe = getprotobyname(pszName);
	if ( !ppe )
		return 0;

	return ppe->p_proto;
}
