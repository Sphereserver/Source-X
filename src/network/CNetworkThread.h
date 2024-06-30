/**
* @file CNetworkThread.h
* @brief Handles i/o for a set of clients (at a lower level, using their underlying CNetState), owned by a single CNetworkManager instance.
*/

#ifndef _INC_CNETWORKTHREAD_H
#define _INC_CNETWORKTHREAD_H

#include "../sphere/threads.h"
#include "../sphere/containers.h"
#include "CNetworkInput.h"
#include "CNetworkOutput.h"

class CNetState;
class CNetworkManager;
class PacketSend;
class PacketTransaction;

class CNetworkThread : public AbstractSphereThread
{
private:
    CNetworkManager* m_manager;					// parent network manager
    size_t m_id;								// network thread #
    int64 _iTimeLastStateDataCheck;

    typedef std::deque<CNetState*> NetworkStateList;
    NetworkStateList m_states;					// states controlled by this thread

    ThreadSafeQueue<CNetState*> m_assignQueue;	// queue of states waiting to be taken by this thread

    CNetworkInput m_input;		// handles data input
    CNetworkOutput m_output;		// handles data output

public:
    size_t id(void) const { return m_id; }							// network thread #
    size_t getClientCount(void) const { return m_states.size(); }	// current number of clients controlled by thread

public:
    static const char* m_sClassName;
    CNetworkThread(CNetworkManager* manager, size_t id);
    virtual ~CNetworkThread(void);

    CNetworkThread(const CNetworkThread& copy) = delete;
    CNetworkThread& operator=(const CNetworkThread& other) = delete;

public:
    void assignNetworkState(CNetState* state);	// assign a network state to this thread
    void queuePacket(PacketSend* packet, bool appendTransaction);
    void queuePacketTransaction(PacketTransaction* transaction);

public:
    void onAsyncSendComplete(CNetState* state, bool success)
    {
        // notify that async operation completed
        m_output.onAsyncSendComplete(state, success);
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

    size_t flush(CNetState* state)
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
    friend class CNetworkManager;
    friend class CNetworkInput;
    friend class CNetworkOutput;
    friend class NetworkThreadStateIterator;
};


/***************************************************************************
 *
 *	class NetworkThreadStateIterator		Works as network state iterator getting the states
 *											for a thread, safely.
 *
 ***************************************************************************/
class NetworkThreadStateIterator
{
protected:
    const CNetworkThread* m_thread;	// network thread to iterate
    int  m_nextIndex;				// next index to check
    bool m_safeAccess;				// whether to use safe access

public:
    explicit NetworkThreadStateIterator(const CNetworkThread* thread);
    ~NetworkThreadStateIterator(void);

private:
    NetworkThreadStateIterator(const NetworkThreadStateIterator& copy);
    NetworkThreadStateIterator& operator=(const NetworkThreadStateIterator& other);

public:
    CNetState* next(void); // find next state
};


#endif // _INC_CNETWORKTHREAD_H
