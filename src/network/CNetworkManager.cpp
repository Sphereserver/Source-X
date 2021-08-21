#include "../game/clients/CClient.h"
#include "../game/CServer.h"
#include "../game/CServerConfig.h"
#include "../sphere/threads.h"
#include "CNetState.h"
#include "CNetworkThread.h"
#include "CNetworkManager.h"


CNetworkManager::CNetworkManager(void)
{
    m_states = nullptr;
    m_stateCount = 0;
    m_lastGivenSlot = -1;
    m_isThreaded = false;
}

CNetworkManager::~CNetworkManager(void)
{
    stop();
    for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); )
    {
        delete* it;
        it = m_threads.erase(it);
    }
}

void CNetworkManager::createNetworkThreads(size_t count)
{
    // create n threads to handle client i/o
    ADDTOCALLSTACK("CNetworkManager::createNetworkThreads");

    // always need at least 1
    if (count < 1)
        count = 1;

    // limit the number of threads to avoid stupid values, the maximum is calculated
    // to allow a maximum of 32 clients per thread at full load
    size_t maxThreads = maximum((FD_SETSIZE / 32), 1);
    if (count > maxThreads)
    {
        count = maxThreads;
        g_Log.Event(LOGL_WARN | LOGM_INIT, "Too many network threads requested. Reducing number to %" PRIuSIZE_T ".\n", count);
    }

    ASSERT(m_threads.empty());
    for (size_t i = 0; i < count; ++i)
        m_threads.emplace_back(new CNetworkThread(this, i));
}

CNetworkThread* CNetworkManager::selectBestThread(void)
{
    // select the most suitable thread for handling a
    // new client
    ADDTOCALLSTACK("CNetworkManager::selectBestThread");

    CNetworkThread* bestThread = nullptr;
    size_t bestThreadSize = INTPTR_MAX;
    DEBUGNETWORK(("Searching for a suitable thread to handle a new client..\n"));

    // search for quietest thread
    for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
    {
        if ((*it)->getClientCount() < bestThreadSize)
        {
            bestThread = *it;
            bestThreadSize = bestThread->getClientCount();
        }
    }

    ASSERT(bestThread != nullptr);
    DEBUGNETWORK(("Selected thread #%" PRIuSIZE_T ".\n", bestThread->id()));
    return bestThread;
}

void CNetworkManager::assignNetworkState(CNetState* state)
{
    // assign a state to a thread
    ADDTOCALLSTACK("CNetworkManager::assignNetworkState");

    CNetworkThread* thread = selectBestThread();
    ASSERT(thread != nullptr);
    thread->assignNetworkState(state);
}

bool CNetworkManager::checkNewConnection(void)
{
    // check for any new connections
    ADDTOCALLSTACK("CNetworkManager::checkNewConnection");

    SOCKET mainSocket = g_Serv.m_SocketMain.GetSocket();

    fd_set fds;
    int count = 0;

    FD_ZERO(&fds);
    AddSocketToSet(fds, mainSocket, count);

    timeval Timeout;		// time to wait for data.
    Timeout.tv_sec = 0;
    Timeout.tv_usec = 100;	// micro seconds = 1/1000000

    count = select(count + 1, &fds, nullptr, nullptr, &Timeout);
    if (count <= 0)
        return false;

    return FD_ISSET(mainSocket, &fds) != 0;
}

void CNetworkManager::acceptNewConnection(void)
{
    // accept new connection
    ADDTOCALLSTACK("CNetworkManager::acceptNewConnection");

    EXC_TRY("acceptNewConnection");
    CSocketAddress client_addr;

    DEBUGNETWORK(("Receiving new connection.\n"));

    // accept socket connection
    EXC_SET_BLOCK("accept");
    SOCKET h = g_Serv.m_SocketMain.Accept(client_addr);
    if (h == INVALID_SOCKET)
        return;

    // check ip history
    EXC_SET_BLOCK("ip history");

    DEBUGNETWORK(("Retrieving IP history for '%s'.\n", client_addr.GetAddrStr()));
    HistoryIP& ip = m_ips.getHistoryForIP(client_addr);
    int maxIp = g_Cfg.m_iConnectingMaxIP;
    int climaxIp = g_Cfg.m_iClientsMaxIP;

    DEBUGNETWORK(("Incoming connection from '%s' [blocked=%d, ttl=%d, pings=%d, connecting=%d, connected=%d]\n",
        ip.m_ip.GetAddrStr(), ip.m_blocked, ip.m_ttl, ip.m_pings, ip.m_connecting, ip.m_connected));

    // check if ip is allowed to connect
    if (ip.checkPing() ||								// check for ip ban
        (maxIp > 0 && ip.m_connecting > maxIp) ||		// check for too many connecting
        (climaxIp > 0 && ip.m_connected > climaxIp))	// check for too many connected
    {
        EXC_SET_BLOCK("rejected");
        DEBUGNETWORK(("Closing incoming connection [max ip=%d, clients max ip=%d].\n", maxIp, climaxIp));

        CLOSESOCKET(h);

        if (ip.m_blocked)
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Connection from %s rejected. (Blocked IP)\n", static_cast<lpctstr>(client_addr.GetAddrStr()));
        else if (maxIp && ip.m_connecting > maxIp)
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Connection from %s rejected. (CONNECTINGMAXIP reached %d/%d)\n", static_cast<lpctstr>(client_addr.GetAddrStr()), ip.m_connecting, maxIp);
        else if (climaxIp && ip.m_connected > climaxIp)
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Connection from %s rejected. (CLIENTMAXIP reached %d/%d)\n", static_cast<lpctstr>(client_addr.GetAddrStr()), ip.m_connected, climaxIp);
        else if (ip.m_pings >= g_Cfg.m_iNetMaxPings)
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Connection from %s rejected. (MAXPINGS reached %d/%d)\n", static_cast<lpctstr>(client_addr.GetAddrStr()), ip.m_pings, (int)(g_Cfg.m_iNetMaxPings));
        else
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Connection from %s rejected.\n", static_cast<lpctstr>(client_addr.GetAddrStr()));

        return;
    }

    // select an empty slot
    EXC_SET_BLOCK("detecting slot");
    CNetState* state = findFreeSlot();
    if (state == nullptr)
    {
        // not enough empty slots
        EXC_SET_BLOCK("no slot available");
        DEBUGNETWORK(("Unable to allocate new slot for client, too many clients already connected.\n"));
        CLOSESOCKET(h);

        g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Connection from %s rejected. (CLIENTMAX reached)\n", static_cast<lpctstr>(client_addr.GetAddrStr()));
        return;
    }

    DEBUGNETWORK(("%x:Allocated slot for client (%u).\n", state->id(), (uint)(h)));

    // assign slot
    EXC_SET_BLOCK("assigning slot");
    state->init(h, client_addr);

    DEBUGNETWORK(("%x:State initialised, registering client instance.\n", state->id()));

    EXC_SET_BLOCK("recording client");
    if (state->getClient() != nullptr)
        m_clients.InsertContentHead(state->getClient());

    EXC_SET_BLOCK("assigning thread");
    DEBUGNETWORK(("%x:Selecting a thread to assign to.\n", state->id()));
    assignNetworkState(state);

    DEBUGNETWORK(("%x:Client successfully initialised.\n", state->id()));

    EXC_CATCH;
}

CNetState* CNetworkManager::findFreeSlot(int start)
{
    // find slot for new client
    ADDTOCALLSTACK("CNetworkManager::findFreeSlot");

    // start searching from the last given slot to try and give incremental
    // ids to clients
    if (start == -1)
        start = m_lastGivenSlot + 1;

    // find unused slot
    for (int i = start; i < m_stateCount; ++i)
    {
        if (m_states[i]->isInUse())
            continue;

        m_lastGivenSlot = i;
        return m_states[i];
    }

    if (start == 0)
        return nullptr;

    // since we started somewhere in the middle, go back to the start
    return findFreeSlot(0);
}

void CNetworkManager::start(void)
{
    DEBUGNETWORK(("Registering packets...\n"));
    m_packets.registerStandardPackets();

    // create network states
    ASSERT(m_states == nullptr);
    ASSERT(m_stateCount == 0);
    m_states = new CNetState * [g_Cfg.m_iClientsMax];
    for (int l = 0; l < g_Cfg.m_iClientsMax; ++l)
        m_states[l] = new CNetState(l);
    m_stateCount = g_Cfg.m_iClientsMax;

    DEBUGNETWORK(("Created %d network slots (system limit of %d clients)\n", m_stateCount, FD_SETSIZE));

    // create network threads
    createNetworkThreads(g_Cfg._uiNetworkThreads);

    m_isThreaded = g_Cfg._uiNetworkThreads > 0;
    if (isThreaded())
    {
        // start network threads
        for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
            (*it)->start();		// The thread structure (class) was created via createNetworkThreads, now spawn a new thread and do the work inside there.
                                // The start method creates a thread with "runner" as main function thread. Runner calls Start, which calls onStart.
                                // onStart, among the other things, actually sets the thread name.

        g_Log.Event(LOGM_INIT, "Started %" PRIuSIZE_T " network threads.\n", m_threads.size());
    }
    else
    {
        // initialise network threads (if g_Cfg._uiNetworkThreads is == 0 then we'll have only 1 CNetworkThread)
        size_t ntCount = m_threads.size();
        UNREFERENCED_PARAMETER(ntCount);
        for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
        {
            CNetworkThread* pThread = *it;

            // Since isThreaded is false the main/first thread will only run Sphere_MainMonitorLoop, and all the sphere/scripts/networking work will be done in the MainThread class,
            //	which code will be executed in another thread named "T_Main". So, in this case even the CNetworkThread(s) will be executed on the main thread.
            ASSERT(ntCount == 1);
            /*
            if (ntCount > 1)
            {
                // If we have more than one thread (this hasn't sense... at this point isThreaded should be == true)
                char name[IThread::m_nameMaxLength];
                snprintf(name, sizeof(name), "T_Net #%u", (uint)pThread->getId());
                pThread->overwriteInternalThreadName(name);
            }
            */

            pThread->onStart();	// the thread structure (class) was created via createNetworkThreads, but we execute the worker method of that class in this thread
        }
    }
}

void CNetworkManager::stop(void)
{
    // terminate child threads
    for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
        (*it)->waitForClose();
}

void CNetworkManager::tick(void)
{
    ADDTOCALLSTACK("CNetworkManager::tick");

    EXC_TRY("Tick");
    for (int i = 0; i < m_stateCount; ++i)
    {
        CNetState* state = m_states[i];
        if (state->isInUse() == false)
            continue;

        // clean packet queue entries
        EXC_SET_BLOCK("cleaning queues");
        for (int priority = 0; priority < PacketSend::PRI_QTY; ++priority)
            state->m_outgoing.queue[priority].clean();
        state->m_outgoing.asyncQueue.clean();

        EXC_SET_BLOCK("check closing");
        if (state->isClosing() == false)
        {
#ifdef _MTNETWORK
            // if data is queued whilst the thread is in the middle of processing then the signal
            // sent from CNetworkOutput::QueuePacketTransaction can be ignored
            // the safest solution to this is to send additional signals from here
            CNetworkThread* thread = state->getParentThread();
            if (thread != nullptr && state->hasPendingData() && thread->getPriority() == IThread::Disabled)
                thread->awaken();
#endif
            continue;
        }

        // the state is closing, see if it can be cleared
        if (state->isClosed() == false)
        {
            EXC_SET_BLOCK("check pending data");
            if (state->hasPendingData())
            {
                // data is waiting to be sent, so flag that a flush is needed
                EXC_SET_BLOCK("flush data");
                if (state->needsFlush() == false)
                {
                    DEBUGNETWORK(("%x:Flushing data for client.\n", state->id()));
                    CNetworkThread* thread = state->getParentThread();
                    if (thread != nullptr)
                        thread->flush(state);
                }
                continue;
            }

            // state is finished with as far as we're concerned
            EXC_SET_BLOCK("mark closed");
            state->markReadClosed();
        }

        EXC_SET_BLOCK("check closed");
        if (state->isClosed() && state->isSendingAsync() == false)
        {
            EXC_SET_BLOCK("clear socket");
            DEBUGNETWORK(("%x:Client is being cleared since marked to close.\n", state->id()));
            state->clear();
        }
    }

    // tick ip history
    m_ips.tick();

    // tick child threads, if single-threaded mode (otherwise they will tick themselves)
    if (isThreaded() == false)
    {
        for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
        {
            if ((*it)->isActive() == false)
                (*it)->tick();
        }
    }

    EXC_CATCH;
    //EXC_DEBUG_START;
    //EXC_DEBUG_END;
}

void CNetworkManager::processAllInput(void)
{
    // process network input
    ADDTOCALLSTACK("CNetworkManager::processAllInput");

    // checkNewConnection will work on both Windows and Linux because it uses the select method, even if it's not the most efficient way to do it
    if (checkNewConnection())
        acceptNewConnection();

    if (isInputThreaded() == false)	// Don't do this if the input is multi threaded, since the CNetworkThread ticks automatically by itself
    {
        // force each thread to process input (NOT THREADSAFE)
        for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
            (*it)->processInput();
    }
}

void CNetworkManager::processAllOutput(void)
{
    // process network output
    ADDTOCALLSTACK("CNetworkManager::processAllOutput");

    if (isOutputThreaded() == false) // Don't do this if the output is multi threaded, since the CNetworkThread ticks automatically by itself
    {
        // force each thread to process output (NOT THREADSAFE)
        for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
            (*it)->processOutput();
    }
}

void CNetworkManager::flushAllClients(void)
{
    // flush data for every client
    ADDTOCALLSTACK("CNetworkManager::flushAllClients");

    for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
        (*it)->flushAllClients();
}

size_t CNetworkManager::flush(CNetState* state)
{
    // flush data for a single client
    ADDTOCALLSTACK("CNetworkManager::flush");
    ASSERT(state != nullptr);
    CNetworkThread* thread = state->getParentThread();
    if (thread != nullptr)
        return thread->flush(state);

    return 0;
}
