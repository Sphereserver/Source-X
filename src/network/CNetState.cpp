#include "../common/sphereproto.h"
#include "../common/CUOClientVersion.h"
#include "../common/CLog.h"
#include "../game/CServer.h"
#include "../game/CServerConfig.h"
#include "../game/CWorld.h"
#include "../game/chars/CChar.h"
#include "../game/clients/CClient.h"
#include "packet.h"
#include "CNetworkThread.h"
#include "CNetState.h"

#ifdef _LIBEV
    extern LinuxEv g_NetworkEvent;
#endif

#define NETWORK_DISCONNECTPRI	PacketSend::PRI_HIGHEST			// packet priorty to continue sending before closing sockets


CNetState::CNetState(int id) :
    m_outgoing{}, m_incoming{}
{
    m_id = id;
    m_client = nullptr;
    m_needsFlush = false;
    m_useAsync = false;
    m_iConnectionTimeMs = -1;
    m_packetExceptions = 0;
    _iInByteCounter = _iOutByteCounter = 0;
    m_clientType = CLIENTTYPE_2D;
    m_clientVersionNumber = 0;
    m_reportedVersionNumber = 0;
    m_isInUse = false;
    m_parent = nullptr;

    clear();
}


CNetState::~CNetState(void)
{
}

void CNetState::clear(void)
{
    ADDTOCALLSTACK("CNetState::clear");
    DEBUGNETWORK(("%x:Clearing client state.\n", id()));

    m_isReadClosed = true;
    m_isWriteClosed = true;
    m_needsFlush = false;

    if (m_client != nullptr)
    {
        g_Serv.StatDec(SERV_STAT_CLIENTS);

        static constexpr LOG_TYPE logFlags = enum_alias_cast<LOG_TYPE>(LOGM_NOCONTEXT | LOGM_CLIENTS_LOG | LOGL_EVENT);
        const CONNECT_TYPE connectionType = m_client->GetConnectType();
        const size_t uiClients = g_Serv.StatGet(SERV_STAT_CLIENTS);
        const lpctstr ptcAddress = m_peerAddress.GetAddrStr();
        if (connectionType == CONNECT_LOGIN)
        {	// when a connection is refused (ie. wrong password), there's no account
            g_Log.Event(logFlags, "%x:Client disconnected [Total:%" PRIuSIZE_T "]. IP='%s'.\n",
                m_id, uiClients, ptcAddress);
        }
        else if (connectionType == CONNECT_GAME)
        {
            const lpctstr ptcAccountName = m_client->GetName();
            const CChar* pAttachedChar = m_client->GetChar();
            if (!pAttachedChar)
            {
                g_Log.Event(logFlags, "%x:Client disconnected [Total:%" PRIuSIZE_T "]. Account: '%s'. IP='%s'.\n",
                    m_id, uiClients, ptcAccountName, ptcAddress);
            }
            else
            {
                g_Log.Event(logFlags, "%x:Client disconnected [Total:%" PRIuSIZE_T "]. Account: '%s'. Char: '%s'. IP='%s'.\n",
                    m_id, uiClients, ptcAccountName, pAttachedChar->GetName(), ptcAddress);
            }
        }
        else
        {
            g_Log.Event(logFlags, "%x:Client disconnected [Total:%" PRIuSIZE_T "]. Connection Type: '%s'. IP='%s'.\n",
                m_id, uiClients, m_client->GetConnectTypeStr(connectionType), ptcAddress);
        }


#ifdef _LIBEV
        if (m_socket.IsOpen() && g_Cfg.m_fUseAsyncNetwork != 0)
            g_NetworkEvent.unregisterClient(this);
#endif

        //	record the client reference to the garbage collection to be deleted on its time
        g_World.ScheduleSpecialObjDeletion(m_client);
    }

#ifdef _WIN32
    if (m_socket.IsOpen() && isAsyncMode())
        m_socket.ClearAsync();
#endif

    m_socket.Close();
    m_client = nullptr;

    // empty queues
    clearQueues();

    // clean junk queue entries
    for (int i = 0; i < PacketSend::PRI_QTY; ++i)
        m_outgoing.queue[i].clean();
    m_outgoing.asyncQueue.clean();
    m_incoming.rawPackets.clean();

    if (m_outgoing.currentTransaction != nullptr)
    {
        delete m_outgoing.currentTransaction;
        m_outgoing.currentTransaction = nullptr;
    }

    if (m_outgoing.pendingTransaction != nullptr)
    {
        delete m_outgoing.pendingTransaction;
        m_outgoing.pendingTransaction = nullptr;
    }

    if (m_incoming.buffer != nullptr)
    {
        delete m_incoming.buffer;
        m_incoming.buffer = nullptr;
    }

    if (m_incoming.rawBuffer != nullptr)
    {
        delete m_incoming.rawBuffer;
        m_incoming.rawBuffer = nullptr;
    }

    m_iConnectionTimeMs = -1;
    m_sequence = 0;
    m_seeded = false;
    m_newseed = false;
    m_seed = 0;
    m_clientVersionNumber = m_reportedVersionNumber = 0;
    m_clientType = CLIENTTYPE_2D;
    m_isSendingAsync = false;
    m_packetExceptions = 0;
    setAsyncMode(false);
    m_isInUse = false;
}

void CNetState::clearQueues(void)
{
    ADDTOCALLSTACK("CNetState::clearQueues");

    // clear packet queues
    for (size_t i = 0; i < PacketSend::PRI_QTY; i++)
    {
        while (m_outgoing.queue[i].empty() == false)
        {
            delete m_outgoing.queue[i].front();
            m_outgoing.queue[i].pop();
        }
    }

    // clear async queue
    while (m_outgoing.asyncQueue.empty() == false)
    {
        delete m_outgoing.asyncQueue.front();
        m_outgoing.asyncQueue.pop();
    }

    // clear byte queue
    m_outgoing.bytes.Empty();

    // clear received queue
    while (m_incoming.rawPackets.empty() == false)
    {
        delete m_incoming.rawPackets.front();
        m_incoming.rawPackets.pop();
    }
}

void CNetState::init(SOCKET socket, CSocketAddress addr)
{
    ADDTOCALLSTACK("CNetState::init");

    clear();

    DEBUGNETWORK(("%x:Initializing client\n", id()));
    int iSockRet;

    m_peerAddress = addr;
    m_socket.SetSocket(socket);

    if (g_Cfg.m_fUseAsyncNetwork != 0)
    {
        iSockRet = m_socket.SetNonBlocking();
        ASSERT(iSockRet == 0);
    }

    // Disable NAGLE algorythm for data compression/coalescing.
    // Send as fast as we can. we handle packing ourselves.
    
    int iSockFlag = 1;
    iSockRet = m_socket.SetSockOpt(TCP_NODELAY, &iSockFlag, sizeof(iSockFlag), IPPROTO_TCP);
    CheckReportNetAPIErr(iSockRet, "NetState::init.TCP_NODELAY");

    m_iConnectionTimeMs = CSTime::GetMonotonicSysTimeMilli();

    g_Serv.StatInc(SERV_STAT_CLIENTS);
    CClient* client = new CClient(this);
    m_client = client;

#ifdef _LIBEV
    if (g_Cfg.m_fUseAsyncNetwork != 0)
    {
        DEBUGNETWORK(("%x:Registering async client\n", id()));
        g_NetworkEvent.registerClient(this, LinuxEv::Write);
    }
#endif

    DEBUGNETWORK(("%x:Opening network state\n", id()));
    m_isWriteClosed = false;
    m_isReadClosed = false;
    m_isInUse = true;

    DEBUGNETWORK(("%x:Determining async mode. Current: %d\n", id(), isAsyncMode()));
    detectAsyncMode();
}

bool CNetState::isInUse(const CClient* client) const volatile noexcept
{
    if (m_isInUse == false)
        return false;

    return client == nullptr || m_client == client;
}

void CNetState::markReadClosed(void) volatile
{
    ADDTOCALLSTACK("CNetState::markReadClosed");

    DEBUGNETWORK(("%x:Client being closed by read-thread\n", m_id));
    m_isReadClosed = true;
    if (m_parent != nullptr && m_parent->getPriority() == ThreadPriority::Disabled)
        m_parent->awaken();
}

void CNetState::markWriteClosed(void) volatile
{
    DEBUGNETWORK(("%x:Client being closed by write-thread\n", m_id));
    m_isWriteClosed = true;
}

void CNetState::markFlush(bool needsFlush) volatile noexcept
{
    m_needsFlush = needsFlush;
}

void CNetState::setAsyncMode(bool isAsync) volatile noexcept
{
    m_useAsync = isAsync;
}

bool CNetState::isAsyncMode(void) const volatile noexcept
{
    return m_useAsync;
}

bool CNetState::isSendingAsync(void) const volatile noexcept
{
    return m_isSendingAsync;
}

void CNetState::setSendingAsync(bool isSending) volatile noexcept
{
    m_isSendingAsync = isSending;
}

void CNetState::detectAsyncMode(void)
{
    ADDTOCALLSTACK("CNetState::detectAsyncMode");
    bool wasAsync = isAsyncMode();

    // is async mode enabled?
    if (!g_Cfg.m_fUseAsyncNetwork || !isInUse())
        setAsyncMode(false);

    // if the version mod flag is not set, always use async mode
    else if (g_Cfg.m_fUseAsyncNetwork == 1)
        setAsyncMode(true);

    // http clients do not want to be using async networking unless they have keep-alive set
    else if (getClient() != nullptr && getClient()->GetConnectType() == CONNECT_HTTP)
        setAsyncMode(false);

    // only use async with clients newer than 4.0.0
    // - normally the client version is unknown for the first 1 or 2 packets, so all clients will begin
    //   without async networking (but should switch over as soon as it has been determined)
    // - a minor issue with this is that for clients without encryption we cannot determine their version
    //   until after they have fully logged into the game server and sent a client version packet.
    else if (isClientVersionNumber(MINCLIVER_AUTOASYNC) || isClientKR() || isClientEnhanced())
        setAsyncMode(true);
    else
        setAsyncMode(false);

    if (wasAsync != isAsyncMode())
        DEBUGNETWORK(("%x:Switching async mode from %s to %s.\n", id(), wasAsync ? "1" : "0", isAsyncMode() ? "1" : "0"));
}

bool CNetState::hasPendingData(void) const
{
    ADDTOCALLSTACK("CNetState::hasPendingData");
    // check if state is even valid
    if (isInUse() == false || m_socket.IsOpen() == false)
        return false;

    // even if there are packets, it doesn't matter once the write thread considers the state closed
    if (isWriteClosed())
        return false;

    // check packet queues (only count high priority+ for closed states)
    for (int i = (isClosing() ? NETWORK_DISCONNECTPRI : PacketSend::PRI_IDLE); i < PacketSend::PRI_QTY; i++)
    {
        if (m_outgoing.queue[i].empty() == false)
            return true;
    }

    // check async data
    if (isAsyncMode() && m_outgoing.asyncQueue.empty() == false)
        return true;

    // check byte queue
    if (m_outgoing.bytes.GetDataQty() > 0)
        return true;

    // check current transaction
    if (m_outgoing.currentTransaction != nullptr)
        return true;

    return false;
}

bool CNetState::canReceive(PacketSend* packet) const
{
    if (isInUse() == false || m_socket.IsOpen() == false || packet == nullptr)
        return false;

    if (isClosing() && packet->getPriority() < NETWORK_DISCONNECTPRI)
        return false;

    if (packet->getTarget()->m_client == nullptr)
        return false;

    return true;
}

void CNetState::beginTransaction(int priority)
{
    ADDTOCALLSTACK("CNetState::beginTransaction");
    if (m_outgoing.pendingTransaction != nullptr)
    {
        DEBUGNETWORK(("%x:New network transaction started whilst a previous is still open.\n", id()));
    }

    // ensure previous transaction is committed
    endTransaction();

    //DEBUGNETWORK(("%x:Starting a new packet transaction.\n", id()));

    m_outgoing.pendingTransaction = new ExtendedPacketTransaction(this, g_Cfg.m_fUsePacketPriorities ? priority : (int)(PacketSend::PRI_NORMAL));
}

void CNetState::endTransaction(void)
{
    ADDTOCALLSTACK("CNetState::endTransaction");
    if (m_outgoing.pendingTransaction == nullptr)
        return;

    //DEBUGNETWORK(("%x:Scheduling packet transaction to be sent.\n", id()));

    m_parent->queuePacketTransaction(m_outgoing.pendingTransaction);
    m_outgoing.pendingTransaction = nullptr;
}


bool CNetState::isClientCryptVersionNumber(dword version) const
{
    return m_clientVersionNumber && CUOClientVersion(m_clientVersionNumber) >= CUOClientVersion(version);
};

bool CNetState::isClientReportedVersionNumber(dword version) const
{
    return m_reportedVersionNumber && CUOClientVersion(m_reportedVersionNumber) >= CUOClientVersion(version);
};

bool CNetState::isClientVersionNumber(dword version) const
{
    return isClientCryptVersionNumber(version) || isClientReportedVersionNumber(version);
}

bool CNetState::isCryptLessVersionNumber(dword version) const
{
    return m_clientVersionNumber && CUOClientVersion(m_clientVersionNumber) < CUOClientVersion(version);
};

bool CNetState::isClientReportedLessVersionNumber(dword version) const
{
    return m_reportedVersionNumber && CUOClientVersion(m_reportedVersionNumber) < CUOClientVersion(version);
};

bool CNetState::isClientLessVersionNumber(dword version) const
{
    return isCryptLessVersionNumber(version) || isClientReportedLessVersionNumber(version);
}
