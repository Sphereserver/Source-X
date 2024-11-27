/**
* @file CNetworkManager.h
* @brief Coordinates input/output network threads (handles incoming connections and spawns child threads to process i/o)
*/

#ifndef _INC_CNETWORKMANAGER_H
#define _INC_CNETWORKMANAGER_H

#include "../common/sphere_library/CSObjList.h"
#include "CIPHistoryManager.h"
#include "CPacketManager.h"


class CNetState;
class CNetworkThread;

class CNetworkManager
{
private:
    CNetState** m_states;			// client state pool
    int  m_stateCount;				// client state count
    int  m_lastGivenSlot;			// last slot index assigned
    bool m_isThreaded;

    typedef std::deque<CNetworkThread*> NetworkThreadList;
    NetworkThreadList m_threads;	// list of network threads
    IPHistoryManager m_ips;			// ip history
    CSObjList m_clients;			// current list of clients (CClient)
    PacketManager m_packets;		// packet handlers

public:
    static const char* m_sClassName;
    CNetworkManager(void);
    ~CNetworkManager(void);

private:
    CNetworkManager(const CNetworkManager& copy);
    CNetworkManager& operator=(const CNetworkManager& other);

public:
    void start(void);
    void stop(void);
    void tick(void);

    bool checkNewConnection(void);				// check if a new connection is waiting to be accepted
    void acceptNewConnection(void);				// accept a new connection

    void processAllInput(void);					// process network input (NOT THREADSAFE)
    void processAllOutput(void);				// process network output (NOT THREADSAFE)
    size_t flush(CNetState* state);				// process all output for a client
    void flushAllClients(void);					// force each thread to flush output

public:
    inline const PacketManager& getPacketManager(void) const noexcept { return m_packets; }		// get packet manager
    inline IPHistoryManager& getIPHistoryManager(void) noexcept { return m_ips; }	// get ip history manager
    inline bool isThreaded(void) const noexcept { return m_isThreaded; } // are threads active
    inline bool isInputThreaded(void) const noexcept // is network input handled by thread
    {
        return m_isThreaded;
    }

    inline bool isOutputThreaded(void) const noexcept // is network output handled by thread
    {
        return m_isThreaded;
    }

private:
    void createNetworkThreads(size_t count);	// create n threads to handle client i/o
    CNetworkThread* selectBestThread(void);		// select the most suitable thread for handling a new client
    void assignNetworkState(CNetState* state);	// assign a state to a thread
    CNetState* findFreeSlot(int start = -1);	// find an unused slot for new client

    friend class ClientIterator;
    friend class NetworkThreadStateIterator;
    friend class CNetworkThread;
    friend class CNetworkInput;
    friend class CNetworkOutput;
};

extern CNetworkManager g_NetworkManager;

#endif // _INC_CNETWORKMANAGER_H
