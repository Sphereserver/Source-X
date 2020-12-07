/**
* @file CSocket.h
* 
*/

#ifndef _INC_CSOCKET_H
#define _INC_CSOCKET_H

#include "../common/common.h"
#ifdef _WIN32
	#include <winsock2.h>		// this needs to be included after common.h, which sets some defines and then includes windows.h, since winsock2.h needs windows.h
	typedef int socklen_t;
#else	
	// else assume LINUX
	#include <sys/socket.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <signal.h>
	#include <fcntl.h>

	// Compatibility stuff.
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR    (-1)
	#define SOCKET			int
	#define TCP_NODELAY		0x0001

#endif	// _WIN32


#ifdef _WIN32
#	define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); closesocket(_x_); }
#else
#	define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); close(_x_); }
#endif

void AddSocketToSet(fd_set& fds, SOCKET socket, int& count);


struct CSocketAddressIP : public in_addr
{
	// Just the ip address. Not the port.
#define SOCKET_LOCAL_ADDRESS 0x0100007f
	// INADDR_ANY              (u_long)0x00000000
	// INADDR_LOOPBACK         0x7f000001
	// INADDR_BROADCAST        (u_long)0xffffffff
	// INADDR_NONE             0xffffffff

public:
	CSocketAddressIP();
	explicit CSocketAddressIP( dword dwIP );
	explicit CSocketAddressIP( const char *ip );

public:
	dword GetAddrIP() const;
	void SetAddrIP( dword dwIP );
	lpctstr GetAddrStr() const;
	void SetAddrStr( lpctstr pszIP );
	bool IsValidAddr() const;
	bool IsLocalAddr() const;

	bool IsSameIP( const CSocketAddressIP & ip ) const;
	bool IsMatchIP( const CSocketAddressIP & ip ) const;

	bool SetHostStruct( const struct hostent * pHost );

	bool SetHostStr( lpctstr pszHostName );
	bool operator==( const CSocketAddressIP & ip ) const;
};

struct CSocketAddress : public CSocketAddressIP
{
	// IP plus port.
	// similar to sockaddr_in but without the waste.
	// use this instead.
private:
	word m_port;
public:
	CSocketAddress();
	CSocketAddress( in_addr dwIP, word uPort );
	CSocketAddress( CSocketAddressIP ip, word uPort );
	CSocketAddress( dword dwIP, word uPort );
	explicit CSocketAddress( const sockaddr_in & SockAddrIn );
	
	bool operator==( const CSocketAddress & SockAddr ) const;
	CSocketAddress & operator = ( const struct sockaddr_in & SockAddrIn );
	bool operator==( const struct sockaddr_in & SockAddrIn ) const;

	// compare to sockaddr_in
	struct sockaddr_in GetAddrPort() const;
	void SetAddrPort( const struct sockaddr_in & SockAddrIn );
	// Just the port.
	word GetPort() const;
	void SetPort( word wPort );
	void SetPortStr( lpctstr pszPort );
	bool SetPortExtStr( tchar * pszIP );
	// Port and address together.
	bool SetHostPortStr( lpctstr pszIP );
};

class CSocket
{
private:
	SOCKET  m_hSocket;	// socket connect handle
	
	void Clear();
	
public:
	static const char *m_sClassName;

	CSocket();
	explicit CSocket( SOCKET socket );
	~CSocket();

private:
	CSocket(const CSocket& copy);
	CSocket& operator=(const CSocket& other);

public:
	static int GetLastError(bool bUseErrno = false);
	bool IsOpen() const;

	void SetSocket(SOCKET socket);
	SOCKET GetSocket() const;

	bool Create();
	bool Create( int iAf, int iType, int iProtocol );
	int Bind( struct sockaddr_in * pSockAddrIn );
	int Bind( const CSocketAddress & SockAddr );
	int Listen( int iMaxBacklogConnections = SOMAXCONN );
	int Connect( struct sockaddr_in * pSockAddrIn );
	int Connect( const CSocketAddress & SockAddr );
	int Connect( const struct in_addr & ip, word wPort );
	int Connect( lpctstr pszHostName, word wPort );
	SOCKET Accept( struct sockaddr_in * pSockAddrIn ) const;
	SOCKET Accept( CSocketAddress & SockAddr ) const;
	int Send( const void * pData, int len ) const;
	int Receive( void * pData, int len, int flags = 0 );

	int GetSockName( struct sockaddr_in * pSockAddrIn ) const;
	CSocketAddress GetSockName() const;

	int GetPeerName( struct sockaddr_in * pSockAddrIn ) const;
	CSocketAddress GetPeerName( ) const;

	int SetSockOpt( int nOptionName, const void* optval, int optlen, int nLevel = SOL_SOCKET ) const;
	int GetSockOpt( int nOptionName, void* optval, int * poptlen, int nLevel = SOL_SOCKET ) const;

#ifdef _WIN32
	int IOCtlSocket(int icmd, DWORD * pdwArgs );
	int SendAsync( LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine ) const;
	void ClearAsync();
#else
	int IOCtlSocket(int icmd, int iVal );
	int GetIOCtlSocketFlags( void );
#endif

	int SetNonBlocking(bool bEnable = true);
	void Close();

	static void CloseSocket( SOCKET hClose );
	static short GetProtocolIdByName( lpctstr pszName );
};

#endif // _INC_CSOCKET_H
