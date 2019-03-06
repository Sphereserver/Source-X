/**
* @file network.h
* @brief Networking stuff.
*/

#ifndef _INC_NETWORK_H
#define _INC_NETWORK_H

#include <deque>
#include "CSocket.h"
#include "packet.h"
#include "../common/common.h"
#include "../common/sphere_library/CSObjList.h"
#include "../common/sphere_library/CSQueue.h"
#include "../common/sphereproto.h"
#include "../sphere/containers.h"

#ifdef _LIBEV
	#include "../sphere/linuxev.h"
#endif

#define NETWORK_PACKETCOUNT		0x100	// number of unique packets
#define NETWORK_BUFFERSIZE		0xF000	// size of receive buffer
#define NETWORK_SEEDLEN_OLD		(sizeof( dword ))
#define NETWORK_SEEDLEN_NEW		(1 + (sizeof( dword ) * 5))

#define NETWORK_MAXPACKETS		g_Cfg.m_iNetMaxPacketsPerTick	// max packets to send per tick (per queue)
#define NETWORK_MAXPACKETLEN	g_Cfg.m_iNetMaxLengthPerTick	// max packet length to send per tick (per queue)
#define NETWORK_MAXQUEUESIZE	g_Cfg.m_iNetMaxQueueSize		// max packets to hold per queue (comment out for unlimited)
#define NETHISTORY_TTL			g_Cfg.m_iNetHistoryTTL			// time to remember an ip
#define NETHISTORY_MAXPINGS		g_Cfg.m_iNetMaxPings			// max 'pings' before blocking an ip
#define NETHISTORY_PINGDECAY	60								// time to decay 1 'ping'

#ifdef _MTNETWORK
	#define MTNETWORK_INPUT		// handle input in multithreaded mode
	#define MTNETWORK_OUTPUT	// handle output in multithreaded mode
#endif

#define NETWORK_DISCONNECTPRI	PacketSend::PRI_HIGHEST			// packet priorty to continue sending before closing sockets

#ifdef DEBUGPACKETS
	#define DEBUGNETWORK(_x_)	g_Log.EventDebug _x_;
#else
	#define DEBUGNETWORK(_x_)	if ( g_Cfg.m_iDebugFlags & DEBUGF_NETWORK ) { g_Log.EventDebug _x_; }
#endif

class CClient;
struct CSocketAddress;
#ifndef _MTNETWORK
	class NetworkIn;
#else
	class NetworkManager;
	class NetworkThread;
	class NetworkInput;
	class NetworkOutput;
    extern NetworkManager g_NetworkManager;
#endif
struct HistoryIP;
typedef std::deque<HistoryIP> IPHistoryList;

#if defined(_PACKETDUMP) || defined(_DUMPSUPPORT)
	void xRecordPacketData(const CClient* client, const byte* data, uint length, lpctstr heading);
	void xRecordPacket(const CClient* client, Packet* packet, lpctstr heading);
#else
	#define xRecordPacketData(_client_, _data_, _length, _heading_)
	#define xRecordPacket(_client_, _packet_, _heading_)
#endif

/***************************************************************************
 *
 *
 *	class NetState				Network status (client info, etc)
 *
 *
 ***************************************************************************/
class NetState
{
protected:
	int m_id; // net id
	CSocket m_socket; // socket
	CClient* m_client; // client
	CSocketAddress m_peerAddress; // client address
#ifdef _MTNETWORK
	NetworkThread* m_parent;
#endif

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
#ifdef _MTNETWORK
		PacketQueue rawPackets; // raw data packets
		Packet* rawBuffer;		// received data
#endif
	} m_incoming; // incoming data

	int m_packetExceptions; // number of packet exceptions

public:
	GAMECLIENT_TYPE m_clientType;	// type of client
	dword m_clientVersion;			// client version (encryption)
	dword m_reportedVersion;		// client version (reported)
	byte m_sequence;				// movement sequence

public:
	explicit NetState(int id);
	~NetState(void);

private:
	NetState(const NetState& copy);
	NetState& operator=(const NetState& other);

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
	
#ifndef _MTNETWORK
	friend class NetworkIn;
	friend class NetworkOut;
#else
	friend class NetworkManager;
	friend class NetworkThread;
	friend class NetworkInput;
	friend class NetworkOutput;

public:
	void setParentThread(NetworkThread* parent) { m_parent = parent; }
	NetworkThread* getParentThread(void) const { return m_parent; }
#endif
	friend class CClient;
	friend class ClientIterator;
	friend class SafeClientIterator;
#ifdef _LIBEV
	friend class LinuxEv;
#endif
};


/***************************************************************************
 *
 *
 *	struct HistoryIP			Simple interface for IP history maintainence
 *
 *
 ***************************************************************************/
struct HistoryIP
{
	CSocketAddressIP m_ip;
	int m_pings;
	int m_connecting;
	int m_connected;
	bool m_blocked;
	int m_ttl;
	int64 m_blockExpire;
	int m_pingDecay;

	void update(void);
	bool checkPing(void); // IP is blocked -or- too many pings to it?
	void setBlocked(bool isBlocked, int timeout = -1); // timeout in seconds
};


/***************************************************************************
 *
 *
 *	class IPHistoryManager		Holds IP records (bans, pings, etc)
 *
 *
 ***************************************************************************/
class IPHistoryManager
{
private:
	IPHistoryList m_ips;		// list of known ips
	int64 m_lastDecayTime;	// last decay time

public:
	IPHistoryManager(void);
	~IPHistoryManager(void);

private:
	IPHistoryManager(const IPHistoryManager& copy);
	IPHistoryManager& operator=(const IPHistoryManager& other);
		
public:
	void tick(void);	// period events

	HistoryIP& getHistoryForIP(const CSocketAddressIP& ip);	// get history for an ip
	HistoryIP& getHistoryForIP(const char* ip);				// get history for an ip
};


/***************************************************************************
 *
 *
 *	class PacketManager             Holds lists of packet handlers
 *
 *
 ***************************************************************************/
class PacketManager
{
private:
	Packet* m_handlers[NETWORK_PACKETCOUNT];	// standard packet handlers
	Packet* m_extended[NETWORK_PACKETCOUNT];	// extended packeet handlers (0xbf)
	Packet* m_encoded[NETWORK_PACKETCOUNT];		// encoded packet handlers (0xd7)

public:
	static const char* m_sClassName;
	PacketManager(void);
	virtual ~PacketManager(void);

private:
	PacketManager(const PacketManager& copy);
	PacketManager& operator=(const PacketManager& other);

public:
	void registerStandardPackets(void);	// register standard packet handlers

	void registerPacket(uint id, Packet* handler);		// register packet handler
	void registerExtended(uint id, Packet* handler);	// register extended packet handler
	void registerEncoded(uint id, Packet* handler);		// register encoded packet handler

	void unregisterPacket(uint id);		// remove packet handler
	void unregisterExtended(uint id);	// remove extended packet handler
	void unregisterEncoded(uint id);	// remove encoded packet handler

	Packet* getHandler(uint id) const;			// get handler for packet
	Packet* getExtendedHandler(uint id) const;	// get handler for extended packet
	Packet* getEncodedHandler(uint id) const;	// get handler for encoded packet
};


/***************************************************************************
 *
 *
 *	class ClientIterator		Works as client iterator getting the clients
 *
 *
 ***************************************************************************/
class ClientIterator
{
protected:
#ifndef _MTNETWORK
	const NetworkIn* m_network;			// network to iterate
#else
	const NetworkManager* m_network;	// network manager to iterate
#endif
	CClient* m_nextClient;

public:
#ifndef _MTNETWORK
	explicit ClientIterator(const NetworkIn* network = nullptr);
#else
	explicit ClientIterator(const NetworkManager* network = nullptr);
#endif
	~ClientIterator(void);

private:
	ClientIterator(const ClientIterator& copy);
	ClientIterator& operator=(const ClientIterator& other);

public:
	CClient* next(bool includeClosing = false); // finds next client
};

#ifndef _MTNETWORK
/***************************************************************************
 *
 *
 *	class SafeClientIterator		Works as client iterator getting the clients in a thread-safe way
 *
 *
 ***************************************************************************/
class SafeClientIterator
{
protected:
	const NetworkIn* m_network;
	int m_id;
	int m_max;

public:
	explicit SafeClientIterator(const NetworkIn* network = nullptr);
	~SafeClientIterator(void);

private:
	SafeClientIterator(const SafeClientIterator& copy);
	SafeClientIterator& operator=(const SafeClientIterator& other);

public:
	CClient* next(bool includeClosing = false); // finds next client
};


/***************************************************************************
 *
 *
 *	class NetworkIn				Networking thread used for receiving client data
 *
 *
 ***************************************************************************/
class NetworkIn : public AbstractSphereThread
{
private:
	int m_lastGivenSlot;		// last slot taken by client
	PacketManager m_packets;	// packet handlers
	IPHistoryManager m_ips;		// ip history

	byte* m_buffer;			// receive buffer
	byte* m_decryptBuffer;	// receive buffer for decryption

protected:
	NetState** m_states;	// client state pool
	int m_stateCount;		// client state count
	CSObjList m_clients;		// current list of clients (CClient)

public:
	static const char* m_sClassName;

	NetworkIn(void);
	virtual ~NetworkIn(void);

private:
	NetworkIn(const NetworkIn& copy);
	NetworkIn& operator=(const NetworkIn& other);

public:
	virtual void onStart(void);
	virtual void tick(void);
	
	const PacketManager& getPacketManager(void) const { return m_packets; }	// get packet manager
	IPHistoryManager& getIPHistoryManager(void) { return m_ips; }			// get ip history manager

	void acceptConnection(void); // accepts a new connection

protected:
	int checkForData(fd_set* storage); // executes network state request for new packets
	int getStateSlot(int startFrom = -1); // finds suitable random slot for client to take
	void periodic(void); // performs periodic actions
	void defragSlots(int fromSlot = 0); // moves used network slots to front

	friend class ClientIterator;
	friend class SafeClientIterator;
};


/***************************************************************************
 *
 *
 *	class NetworkOut			Networking thread used for queued sending outgoing packets
 *
 *
 ***************************************************************************/
class NetworkOut : public AbstractSphereThread
{
private:
	byte* m_encryptBuffer; // buffer for encryption

public:
	static const char* m_sClassName;

	NetworkOut(void);
	virtual ~NetworkOut(void);

private:
	NetworkOut(const NetworkOut& copy);
	NetworkOut& operator=(const NetworkOut& other);

public:
	virtual void tick(void);

	void schedule(const PacketSend* packet, bool appendTransaction); // schedule this packet to be sent
	void scheduleOnce(PacketSend* packet, bool appendTransaction); // schedule this packet to be sent MOVING it to the list
	void scheduleOnce(PacketTransaction* transaction); // schedule a transaction to be sent

	void flushAll(void); // forces immediate send of all packets for all clients
	void flush(CClient* client); // forces immediate send of all packets

protected:
	int proceedQueue(CClient* client, int priority); // send next set of packets with the specified priority (returns number of packets sent)
	int proceedQueueAsync(CClient* client); // send next set of asynchronous packets (returns number of packets sent, 1 max)
	void proceedQueueBytes(CClient* client); // send next set of bytes
	void proceedFlush(void); // flush data to pending sockets
	bool sendPacket(CClient* client, PacketSend* packet); // send packet to a client
	bool sendPacketNow(CClient* client, PacketSend* packet); // send packet to a client now
	int sendBytesNow(CClient* client, const byte* data, dword length); // send bytes to a client (returns number of bytes sent, < 0 for failure)

public:
	void onAsyncSendComplete(NetState* state, bool success); // handle completion of async send

	friend class SimplePacketTransaction;
	friend class ExtendedPacketTransaction;
};

extern NetworkIn g_NetworkIn;
extern NetworkOut g_NetworkOut;

#else //!_MTNETWORK

class NetworkManager;
class NetworkThread;
class NetworkInput;
class NetworkOutput;

typedef std::deque<NetworkThread*> NetworkThreadList;
typedef std::deque<NetState*> NetworkStateList;
	
/***************************************************************************
 *
 *
 *	class NetworkInput					Handles network input from clients
 *
 *
 ***************************************************************************/
class NetworkInput
{
private:
	NetworkThread* m_thread;	// owning network thread
	byte* m_receiveBuffer;		// buffer for received data
	byte* m_decryptBuffer;		// buffer for decrypted data

public:
	static const char* m_sClassName;
	NetworkInput(void);
	~NetworkInput(void);

private:
	NetworkInput(const NetworkInput& copy);
	NetworkInput& operator=(const NetworkInput& other);

public:
	void setOwner(NetworkThread* thread) { m_thread = thread; } // set owner thread
	bool processInput(void);									// process input from clients, returns true if work was done

private:
	bool checkForData(fd_set& fds);		// check for states which have pending data to read
	void receiveData();					// receive raw data for all sockets
	void processData();					// process received data for all sockets

	bool processData(NetState* state, Packet* buffer);				// process received data
	bool processUnknownClientData(NetState* state, Packet* buffer);	// process data from an unknown client type
	bool processOtherClientData(NetState* state, Packet* buffer);		// process data from a non-game client
	bool processGameClientData(NetState* state, Packet* buffer);		// process data from a game client
};

	
/***************************************************************************
 *
 *
 *	class NetworkOutput					Handles network output to clients
 *
 *
 ***************************************************************************/
class NetworkOutput
{
private:
	static inline size_t _failed_result(void) { return INTPTR_MAX; }

private:
	NetworkThread* m_thread;	// owning network thread
	byte* m_encryptBuffer;		// buffer for encrpyted data

public:
	static const char* m_sClassName;
	NetworkOutput(void);
	~NetworkOutput(void);

private:
	NetworkOutput(const NetworkOutput& copy);
	NetworkOutput& operator=(const NetworkOutput& other);

public:
	void setOwner(NetworkThread* thread) { m_thread = thread; }			// set owner thread
	bool processOutput(void);											// process output to clients, returns true if data was sent
	size_t flush(NetState* state);										// process all queues for a client
	void onAsyncSendComplete(NetState* state, bool success);			// notify that async operation completed

	static void QueuePacket(PacketSend* packet, bool appendTransaction);	// queue a packet for sending
	static void QueuePacketTransaction(PacketTransaction* transaction);		// queue a packet transaction for sending

private:
	void checkFlushRequests(void);										// check for clients who need data flushing
	size_t processPacketQueue(NetState* state, uint priority);	// process a client's packet queue
	size_t processAsyncQueue(NetState* state);							// process a client's async queue
	bool processByteQueue(NetState* state);								// process a client's byte queue

	bool sendPacket(NetState* state, PacketSend* packet);				// send packet to client (can be queued for async operation)
	bool sendPacketData(NetState* state, PacketSend* packet);			// send packet data to client
	size_t sendData(NetState* state, const byte* data, size_t length);	// send raw data to client
};


/***************************************************************************
 *
 *
 *	class NetworkManager            Network manager, handles incoming connections and
 *                                  spawns child threads to process i/o
 *
 *
 ***************************************************************************/
class NetworkManager
{
private:
	NetState** m_states;			// client state pool
	size_t m_stateCount;			// client state count
	size_t m_lastGivenSlot;			// last slot index assigned
	bool m_isThreaded;

    IPHistoryManager m_ips;			// ip history
	CSObjList m_clients;			// current list of clients (CClient)
	NetworkThreadList m_threads;	// list of network threads
	PacketManager m_packets;		// packet handlers

public:
	static const char* m_sClassName;
	NetworkManager(void);
	~NetworkManager(void);

private:
	NetworkManager(const NetworkManager& copy);
	NetworkManager& operator=(const NetworkManager& other);

public:
	void start(void);
	void stop(void);
	void tick(void);

	bool checkNewConnection(void);				// check if a new connection is waiting to be accepted
	void acceptNewConnection(void);				// accept a new connection

	void processAllInput(void);					// process network input (NOT THREADSAFE)
	void processAllOutput(void);				// process network output (NOT THREADSAFE)
	size_t flush(NetState * state);				// process all output for a client
	void flushAllClients(void);					// force each thread to flush output

public:
	const PacketManager& getPacketManager(void) const { return m_packets; }		// get packet manager
	IPHistoryManager& getIPHistoryManager(void) { return m_ips; }	// get ip history manager
	bool isThreaded(void) const { return m_isThreaded; } // are threads active
	inline bool isInputThreaded(void) const // is network input handled by thread
	{
#ifdef MTNETWORK_INPUT
		return m_isThreaded;
#else
		return false; // threaded input not supported
#endif
	}

	inline bool isOutputThreaded(void) const // is network output handled by thread
	{
#ifdef MTNETWORK_OUTPUT
		return m_isThreaded;
#else
		return false; // threaded output not supported
#endif
	}

private:
	void createNetworkThreads(size_t count);	// create n threads to handle client i/o
	NetworkThread* selectBestThread(void);		// select the most suitable thread for handling a new client
	void assignNetworkState(NetState* state);	// assign a state to a thread
	NetState* findFreeSlot(size_t start = INTPTR_MAX);	// find an unused slot for new client
		
	friend class ClientIterator;
	friend class NetworkThreadStateIterator;
	friend class NetworkThread;
	friend class NetworkInput;
	friend class NetworkOutput;
};


/***************************************************************************
 *
 *
 *	class NetworkThread             Handles i/o for a set of clients, owned
 *                                  by a single NetworkManager instance
 *
 *
 ***************************************************************************/
class NetworkThread : public AbstractSphereThread
{
private:
	NetworkManager& m_manager;					// parent network manager
	size_t m_id;								// network thread #

	NetworkStateList m_states;					// states controlled by this thread
	ThreadSafeQueue<NetState*> m_assignQueue;	// queue of states waiting to be taken by this thread

	NetworkInput m_input;		// handles data input
	NetworkOutput m_output;		// handles data output

public:
	size_t id(void) const { return m_id; }							// network thread #
	size_t getClientCount(void) const { return m_states.size(); }	// current number of clients controlled by thread

public:
	static const char* m_sClassName;
	NetworkThread(NetworkManager& manager, size_t id);
	virtual ~NetworkThread(void);

private:
	NetworkThread(const NetworkThread& copy);
	NetworkThread& operator=(const NetworkThread& other);

public:
	void assignNetworkState(NetState* state);	// assign a network state to this thread

public:
	void onAsyncSendComplete(NetState* state, bool success)
	{
		// notify that async operation completed
		m_output.onAsyncSendComplete(state, success);
	}

	void queuePacket(PacketSend* packet, bool appendTransaction)
	{
		// queue a packet for sending
		NetworkOutput::QueuePacket(packet, appendTransaction);
	}

	void queuePacketTransaction(PacketTransaction* transaction)
	{
		// queue a packet transaction for sending
		NetworkOutput::QueuePacketTransaction(transaction);
	}

	void processInput(void)
	{
		// process network input
		m_input.processInput();
	}

	void processOutput(void)
	{
		// process network output
		m_output.processOutput();
	}

	size_t flush(NetState * state)
	{
		// flush all output for a client
		return m_output.flush(state);
	}

public:
	virtual void onStart(void);
	virtual void tick(void);

	void flushAllClients(void);			// flush all output

private:
	void checkNewStates(void);			// check for states that have been assigned but not moved to our list
	void dropInvalidStates(void);		// check for states that don't belong to use anymore

public:
	friend class NetworkManager;
	friend class NetworkInput;
	friend class NetworkOutput;
	friend class NetworkThreadStateIterator;
};


/***************************************************************************
 *
 *
 *	class NetworkThreadStateIterator		Works as network state iterator getting the states
 *											for a thread, safely.
 *
 *
 ***************************************************************************/
class NetworkThreadStateIterator
{
protected:
	const NetworkThread* m_thread;	// network thread to iterate
	size_t m_nextIndex;				// next index to check
	bool m_safeAccess;				// whether to use safe access

public:
	explicit NetworkThreadStateIterator(const NetworkThread* thread);
	~NetworkThreadStateIterator(void);

private:
	NetworkThreadStateIterator(const NetworkThreadStateIterator& copy);
	NetworkThreadStateIterator& operator=(const NetworkThreadStateIterator& other);

public:
	NetState* next(void); // find next state
};


#endif //_MTNETWORK

#endif // _INC_NETWORK_H
