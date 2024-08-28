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
    DEBUGNETWORK(("Receiving new connection.\n"));

    // 1. Use "select" to check if the main socket of the server has data to read and it's ready to do so.
    // 2. Use "accept" to extract the first connection request from the queue and creates a new connected socket for the client.
    //  The listening socket remains open to accept further connections. Each call to accept only handles one connection request.
    //  Therefore, to handle multiple incoming connections at the same time, we must call accept in a loop/repeatedly. We wait for the new acceptNewConnection call.
    // 3. Use "recv" on the new socket to receive the data.

    // accept socket connection
    EXC_SET_BLOCK("accept");
    CSocketAddress client_addr;
    SOCKET h = g_Serv.m_SocketMain.Accept(client_addr);
    if (h == INVALID_SOCKET)
        return;

    // check ip history
    EXC_SET_BLOCK("ip history");

    DEBUGNETWORK(("Retrieving IP history for '%s'.\n", client_addr.GetAddrStr()));
    const int maxIp = g_Cfg.m_iConnectingMaxIP;
    const int climaxIp = g_Cfg.m_iClientsMaxIP;
    HistoryIP& ip = m_ips.getHistoryForIP(client_addr);
    if (ip.m_ttl < g_Cfg.m_iNetHistoryTTL)
    {
        // This is an IP of interest, i want to remember it for longer.
        ip.m_ttl = g_Cfg.m_iNetHistoryTTL;
    }

    const int64 iIpPrevConnectionTime = ip.m_timeLastConnectedMs;
    ip.m_timeLastConnectedMs = CSTime::GetMonotonicSysTimeMilli();
    ip.m_connectionAttempts += 1;

    DEBUGNETWORK(("Incoming connection from '%s' [IP history: blocked=%d, ttl=%d, pings=%d, connecting=%d, connected=%d].\n",
        ip.m_ip.GetAddrStr(), ip.m_blocked, ip.m_ttl, ip.m_pings, ip.m_connecting, ip.m_connected));

    const auto _printIPBlocked = [&ip](void) -> void {
        g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Blocked connection from '%s' [IP history: blocked=%d, ttl=%d, pings=%d, connecting=%d, connected=%d].\n",
            ip.m_ip.GetAddrStr(), ip.m_blocked, ip.m_ttl, ip.m_pings, ip.m_connecting, ip.m_connected);
    };

    // check if ip is allowed to connect
    if (ip.checkPing() ||								// check for ip ban
        (maxIp > 0 && ip.m_connecting > maxIp) ||		// check for too many connecting
        (climaxIp > 0 && ip.m_connected > climaxIp))	// check for too many connected
    {
        EXC_SET_BLOCK("rejected");
        DEBUGNETWORK(("Closing incoming connection [max ip=%d, clients max ip=%d].\n", maxIp, climaxIp));

        CLOSESOCKET(h);

        _printIPBlocked();
        if (ip.m_blocked)
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: Blocked IP.\n");
        else if (maxIp && ip.m_connecting > maxIp)
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: CONNECTINGMAXIP reached %d/%d.\n", ip.m_connecting, maxIp);
        else if (climaxIp && ip.m_connected > climaxIp)
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: CLIENTMAXIP reached %d/%d.\n", ip.m_connected, climaxIp);
        else if (ip.m_pings >= g_Cfg.m_iNetMaxPings)
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: MAXPINGS reached %d/%d.\n", ip.m_pings, g_Cfg.m_iNetMaxPings);
        else
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: unclassified.\n");

        return;
    }

    if ((g_Cfg._iMaxConnectRequestsPerIP > 0) && (ip.m_connectionAttempts >= (int64)g_Cfg._iMaxConnectRequestsPerIP))
    {
        // Call this special scripted function.
        CScriptTriggerArgs fargs_ex(client_addr.GetAddrStr());
        fargs_ex.m_iN1 = iIpPrevConnectionTime;
        fargs_ex.m_iN2 = ip.m_timeLastConnectedMs;  // Current connection time.
        fargs_ex.m_iN3 = ip.m_connectionAttempts;
        fargs_ex.m_VarsLocal.SetNumNew("BAN_TIMEOUT", 5ll * 60);
        TRIGRET_TYPE fret = TRIGRET_RET_FALSE;
        g_Serv.r_Call("f_onserver_connectreq_ex", &g_Serv, &fargs_ex, nullptr, &fret);
        if (fret == TRIGRET_RET_TRUE)
        {
            // reject
            CLOSESOCKET(h);

            _printIPBlocked();
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: requested kick via script 'f_onserver_connectreq_ex'.\n");
        }
        else if (fret == TRIGRET_RET_DEFAULT) // 2
        {
            // block IP
            CLOSESOCKET(h);
            ip.setBlocked(true, fargs_ex.m_VarsLocal.GetKeyNum("BAN_TIMEOUT"));

            _printIPBlocked();
            g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: requested kick + IP block allowed by script 'f_onserver_connectreq_ex'.\n");
        }
    }

    /*
    // Check if there's already more data waiting to be read (suspicious?).
    // Classic client (but not some 3rd party clients) have pending data after the second connection request
    //  (first: serverlist, second: redirected to the actual game server), so right now this kind of check isn't useful (unless coupled with some
    //  other kind of checks to filter out legitimate clients sending legit data).

    uint64 iIncomingDataStreamLength;
#ifdef _WIN32
    u_long pending_data_size;
    if (ioctlsocket(h, FIONREAD, &pending_data_size) != NO_ERROR)
    {
        const int iWSAErrCode = WSAGetLastError();
        lptstr ptcErrorBuf = Str_GetTemp();
        CSError::GetSystemErrorMessage(dword(iWSAErrCode), // iWSAErrCode shouldn't be negative
            ptcErrorBuf, Str_TempLength());

        _printIPBlocked();
        g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: ioctlsocket (FIONREAD) failed. Error Code: %d ('%s').\n",
            iWSAErrCode, ptcErrorBuf);

        CLOSESOCKET(h);
        ASSERT(iWSAErrCode > 0);
        return;
    }
    iIncomingDataStreamLength = pending_data_size;
#else
    ssize_t pending_data_size = recv(h, nullptr, 0, MSG_DONTWAIT | MSG_PEEK | MSG_TRUNC);
    if (pending_data_size < 0)
    {
        _printIPBlocked();
        g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: recv (peek,trunc) failed. Error Code: %d ('%s'),\n",
            errno, strerror(errno));

        CLOSESOCKET(h);
        return;
    }
    iIncomingDataStreamLength = uint64(pending_data_size);
#endif
    DEBUGNETWORK(("Size of pending data after accepting connection: %" PRIu64 " bytes.\n", iIncomingDataStreamLength));

    if (iIncomingDataStreamLength > 0)
    {
        _printIPBlocked();
        g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: size of pending data after accepting connection > 0 (%" PRIu64 " bytes).\n",
            iIncomingDataStreamLength);

        CLOSESOCKET(h);
        return;
    }
    */

    // Call this special scripted function.
    CScriptTriggerArgs fargs_acquired(client_addr.GetAddrStr());
    fargs_acquired.m_iN1 = iIpPrevConnectionTime;
    fargs_acquired.m_iN2 = ip.m_timeLastConnectedMs; // Current connection time.
    TRIGRET_TYPE fret = TRIGRET_RET_FALSE;
    g_Serv.r_Call("f_onserver_connection_acquired", &g_Serv, &fargs_acquired, nullptr, &fret);

    // select an empty slot
    EXC_SET_BLOCK("detecting slot");
    CNetState* state = findFreeSlot();
    if (state == nullptr)
    {
        // not enough empty slots
        EXC_SET_BLOCK("no slot available");
        _printIPBlocked();

        g_Log.Event(LOGM_CLIENTS_LOG | LOGL_ERROR, "Reject reason: CLIENTMAX reached.\n");
        CLOSESOCKET(h);
        return;
    }

    DEBUGNETWORK(("%x:Allocated slot for client (socket %u).\n", state->id(), (uint)h));

    // assign slot and a CClient
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
    constexpr int kMaxFd = minimum(1024, FD_SETSIZE);
    if (g_Cfg.m_iClientsMax > kMaxFd)
    {
        g_Log.EventWarn("ClientsMax setting > %d (system limit), defaulting to max.\n", kMaxFd);
        g_Cfg.m_iClientsMax = kMaxFd;
    }
    m_states = new CNetState * [g_Cfg.m_iClientsMax];
    for (int l = 0; l < g_Cfg.m_iClientsMax; ++l)
        m_states[l] = new CNetState(l);
    m_stateCount = g_Cfg.m_iClientsMax;

    DEBUGNETWORK(("Created %d network slots (system limit of %d clients)\n", m_stateCount, kMaxFd));

    // create network threads
    createNetworkThreads(g_Cfg._uiNetworkThreads);

    m_isThreaded = g_Cfg._uiNetworkThreads > 0;
    if (isThreaded())
    {
        // start network threads
        for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
        {
            // The thread structure (class) was created via createNetworkThreads, now spawn a new thread and do the work inside there.
            // The start method creates a thread with "runner" as main function thread. Runner calls Start, which calls onStart.
            // onStart, among the other things, actually sets the thread name.
            (*it)->start();
        }

        g_Log.Event(LOGM_INIT, "Started %" PRIuSIZE_T " network threads.\n", m_threads.size());
    }
    else
    {
        // initialise network threads (if g_Cfg._uiNetworkThreads is == 0 then we'll have only 1 CNetworkThread)
        size_t ntCount = m_threads.size();
        UnreferencedParameter(ntCount);
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

    const int64 iCurSysTimeMs = CSTime::GetMonotonicSysTimeMilli();
    EXC_TRY("Tick");
    for (int i = 0; i < m_stateCount; ++i)
    {
        CNetState* state = m_states[i];
        if (state->isInUse() == false)
            continue;

        // If the client hasn't completed login for a definite amount of time, disconnect it...
        const CClient* pClient = state->getClient();
        if (!pClient || (pClient->GetConnectType() == CONNECT_UNK))
        {
            const int64 iTimeSinceConnectionMs = iCurSysTimeMs - state->m_iConnectionTimeMs;
            if (iTimeSinceConnectionMs > g_Cfg._iTimeoutIncompleteConnectionMs)
            {
                EXC_SET_BLOCK("mark closed for timeout");
                g_Log.Event(LOGM_CLIENTS_LOG | LOGL_EVENT,
                    "%x:Force closing connection from IP %s. Reason: timed out before completing login.\n",
                    state->id(), state->m_peerAddress.GetAddrStr());
                //state->markReadClosed();
                //state->markWriteClosed();
                state->clear();
                continue;
            }
        }

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
    {
        acceptNewConnection();
    }

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
