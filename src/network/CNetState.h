/**
* @file CNetState.h
* @brief Network status (client info, etc) and i/o manager for a single network connection.
*/

#ifndef _INC_CNETSTATE_H
#define _INC_CNETSTATE_H

#include "../common/sphere_library/CSQueue.h"
#include "../common/sphereproto.h"
#include "../sphere/containers.h"
#include "CSocket.h"
#include "packet.h"

#ifdef _LIBEV
    #include "linuxev.h"
#endif


#ifdef DEBUGPACKETS
    #define DEBUGNETWORK(_x_)	g_Log.EventDebug _x_;
#else
    #define DEBUGNETWORK(_x_)	if ( g_Cfg.m_iDebugFlags & DEBUGF_NETWORK ) { g_Log.EventDebug _x_; }
#endif


class CClient;
class CNetworkThread;

class CNetState
{
protected:
    int m_id; // net id
    CSocket m_socket; // socket
    CClient* m_client; // client
    CSocketAddress m_peerAddress; // client address
    CNetworkThread* m_parent;

    volatile bool m_isInUse;		// is currently in use
    volatile bool m_isReadClosed;	// is closed by read thread
    volatile bool m_isWriteClosed;	// is closed by write thread
    volatile bool m_needsFlush;		// does data need to be flushed

    bool m_seeded;	// is seed received
    dword m_seed;	// client seed
    bool m_newseed; // is the client using new seed

    bool m_useAsync;				// is this socket using asynchronous sends
    volatile bool m_isSendingAsync; // is a packet currently being sent asynchronously?
#ifdef _LIBEV
    // non-windows uses ev_io for async operations
    struct ev_io m_eventWatcher;
#elif defined(_WIN32)
    // windows uses winsock for async operations
    WSABUF m_bufferWSA;			// Winsock Async Buffer
    WSAOVERLAPPED m_overlapped; // Winsock Overlapped structure
#endif

    typedef ThreadSafeQueue<Packet*> PacketQueue;
    typedef ThreadSafeQueue<PacketSend*> PacketSendQueue;
    typedef ThreadSafeQueue<PacketTransaction*> PacketTransactionQueue;

    struct
    {
        PacketTransactionQueue queue[PacketSend::PRI_QTY];	// packet queue
        PacketSendQueue asyncQueue;		// async packet queue
        CSQueueBytes bytes;				// byte queue

        PacketTransaction* currentTransaction;			// transaction currently being processed
        ExtendedPacketTransaction* pendingTransaction;	// transaction being built
    } m_outgoing; // outgoing data

    struct
    {
        Packet* buffer; // received data
        PacketQueue rawPackets; // raw data packets
        Packet* rawBuffer;		// received data
    } m_incoming; // incoming data

    int m_packetExceptions;     // number of packet exceptions
    int64 _iInByteCounter;    // number of bytes received by this client since the last legitimacy check
    int64 _iOutByteCounter;   // number of bytes sent     by this client since the last legitimacy check

public:
    GAMECLIENT_TYPE m_clientType;	// type of client
    dword m_clientVersion;			// client version (encryption)
    dword m_reportedVersion;		// client version (reported)
    byte m_sequence;				// movement sequence

public:
    explicit CNetState(int id);
    ~CNetState(void);

private:
    CNetState(const CNetState& copy);
    CNetState& operator=(const CNetState& other);

public:
    int id(void) const { return m_id; };	// returns ID of the client
    void setId(int id) { m_id = id; };		// changes ID of the client
    void clear(void);						// clears state
    void clearQueues(void);					// clears outgoing data queues

    void init(SOCKET socket, CSocketAddress addr);		// initialized socket
    bool isInUse(const CClient* client = nullptr) const volatile; // does this socket still belong to this/a client?
    bool hasPendingData(void) const;			// is there any data waiting to be sent?
    bool canReceive(PacketSend* packet) const;	// can the state receive the given packet?

    void detectAsyncMode(void);
    void setAsyncMode(bool isAsync) { m_useAsync = isAsync; };	// set asynchronous mode
    bool isAsyncMode(void) const { return m_useAsync; };		// get asyncronous mode
#ifdef _LIBEV
    struct ev_io* iocb(void) { return &m_eventWatcher; };		// get io callback
#endif
    bool isSendingAsync(void) const volatile { return m_isSendingAsync; };				// get if async packeet is being sent
    void setSendingAsync(bool isSending) volatile { m_isSendingAsync = isSending; };	// set if async packet is being sent

    GAMECLIENT_TYPE getClientType(void) const { return m_clientType; };	// determined client type
    dword getCryptVersion(void) const { return m_clientVersion; };		// version as determined by encryption
    dword getReportedVersion(void) const { return m_reportedVersion; }; // version as reported by client

    void markReadClosed(void) volatile;		// mark socket as closed by read thread
    void markWriteClosed(void) volatile;	// mark socket as closed by write thread
    bool isClosing(void) const volatile { return m_isReadClosed || m_isWriteClosed; }	// is the socket closing?
    bool isClosed(void) const volatile { return m_isReadClosed && m_isWriteClosed; }	// is the socket closed?
    bool isReadClosed(void) const volatile { return m_isReadClosed; }	// is the socket closed by read-thread?
    bool isWriteClosed(void) const volatile { return m_isWriteClosed; }	// is the socket closed by write-thread?

    void markFlush(bool needsFlush) volatile; // mark socket as needing a flush
    bool needsFlush(void) const volatile { return m_needsFlush; } // does the socket need to be flushed?

    CClient* getClient(void) const { return m_client; } // get linked client

    bool isClient3D(void) const { return m_clientType == CLIENTTYPE_3D; }; // is this a 3D client?
    bool isClientKR(void) const { return m_clientType == CLIENTTYPE_KR; }; // is this a KR client?
    bool isClientEnhanced(void) const { return m_clientType == CLIENTTYPE_EC; }; // is this an Enhanced client?

    bool isCryptVersion(dword version) const { return m_clientVersion && m_clientVersion >= version; };			// check the minimum crypt version
    bool isReportedVersion(dword version) const { return m_reportedVersion && m_reportedVersion >= version; };	// check the minimum reported verson
    bool isClientVersion(dword version) const { return isCryptVersion(version) || isReportedVersion(version); } // check the minimum client version
    bool isCryptLessVersion(dword version) const { return m_clientVersion && m_clientVersion < version; };		// check the maximum crypt version
    bool isReportedLessVersion(dword version) const { return m_reportedVersion && m_reportedVersion < version; };	// check the maximum reported version
    bool isClientLessVersion(dword version) const { return isCryptLessVersion(version) || isReportedLessVersion(version); } // check the maximum client version

    void beginTransaction(int priority);	// begin a transaction for grouping packets
    void endTransaction(void);				// end transaction

    friend class CNetworkManager;
    friend class CNetworkThread;
    friend class CNetworkInput;
    friend class CNetworkOutput;
#ifdef _LIBEV
    friend class LinuxEv;
#endif

    void setParentThread(CNetworkThread* parent) { m_parent = parent; }
    CNetworkThread* getParentThread(void) const { return m_parent; }

    friend class CClient;
    friend class ClientIterator;
    friend class SafeClientIterator;
};


#endif // _INC_CNETSTATE_H
