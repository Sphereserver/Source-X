#ifdef _MTNETWORK

#include "../common/CException.h"
#include "../game/clients/CClient.h"
#include "../game/chars/CChar.h"
#include "../common/CLog.h"
#include "../game/CServer.h"
#include "../game/CWorld.h"
#include "../sphere/containers.h"
#include "../sphere/ProfileTask.h"
#include "network.h"
#include "receive.h"
#include "send.h"


#ifdef _WIN32
#	define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); closesocket(_x_); }
#else
#	define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); close(_x_); }
#endif


/***************************************************************************
*
*
*	void AddSocketToSet				Add a socket to an fd_set
*
*
***************************************************************************/
inline void AddSocketToSet(fd_set& fds, SOCKET socket, int& count)
{
#ifdef _WIN32
	UNREFERENCED_PARAMETER(count);
	FD_SET(socket, &fds);
#else
	FD_SET(socket, &fds);
	if (socket > count)
		count = socket;
#endif
}

/***************************************************************************
*
*
*	void GenerateNetworkThreadName				Generate network thread name
*
*
***************************************************************************/
const char * GenerateNetworkThreadName(size_t id)
{
	char * name = new char[IThread::m_nameMaxLength];
	snprintf(name, IThread::m_nameMaxLength, "T_Network #%" PRIuSIZE_T, id);
	return name;
}

#ifdef _WIN32

/***************************************************************************
*
*
*	void SendCompleted_Winsock			Winsock event handler for when async operation completes
*
*
***************************************************************************/
void CALLBACK SendCompleted_Winsock(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	UNREFERENCED_PARAMETER(dwFlags);
	ADDTOCALLSTACK("SendCompleted_Winsock");

	NetState* state = reinterpret_cast<NetState*>(lpOverlapped->hEvent);
	if (state == nullptr)
	{
		DEBUGNETWORK(("Async i/o operation (Winsock) completed without client context.\n"));
		return;
	}

	NetworkThread* thread = state->getParentThread();
	if (thread == nullptr)
	{
		DEBUGNETWORK(("%x:Async i/o operation (Winsock) completed (single-threaded, or no thread context).\n", state->id()));
		// This wasn't called by a NetworkThread, so we can't do onAsyncSendComplete.
		state->setSendingAsync(false);	// new addition. is it breaking something or instead fixing things?
		return;
	}

	if (dwError != 0)
		DEBUGNETWORK(("%x:Async i/o operation (Winsock) completed with error code 0x%x, %d bytes sent.\n", state->id(), dwError, cbTransferred));
	//else
	//	DEBUGNETWORK(("%x:Async i/o operation (Winsock) completed successfully, %d bytes sent.\n", state->id(), cbTransferred));

	thread->onAsyncSendComplete(state, dwError == 0 && cbTransferred > 0);
}

#endif

/***************************************************************************
*
*
*	class NetworkManager            Network manager, handles incoming connections and
*                                  spawns child threads to process i/o
*
*
***************************************************************************/
NetworkManager::NetworkManager(void)
{
	m_states = nullptr;
	m_stateCount = 0;
	m_lastGivenSlot = -1;
	m_isThreaded = false;
}

NetworkManager::~NetworkManager(void)
{
	stop();
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); )
	{
		delete *it;
		it = m_threads.erase(it);
	}
}

void NetworkManager::createNetworkThreads(size_t count)
{
	// create n threads to handle client i/o
	ADDTOCALLSTACK("NetworkManager::createNetworkThreads");

	// always need at least 1
	if (count < 1)
		count = 1;

	// limit the number of threads to avoid stupid values, the maximum is calculated
	// to allow a maximum of 32 clients per thread at full load
	size_t maxThreads = maximum((FD_SETSIZE / 32), 1);
	if (count > maxThreads)
	{
		count = maxThreads;
		g_Log.Event(LOGL_WARN|LOGM_INIT, "Too many network threads requested. Reducing number to %" PRIuSIZE_T ".\n", count);
	}

	ASSERT(m_threads.empty());
	for (size_t i = 0; i < count; ++i)
		m_threads.push_back(new NetworkThread(*this, i));
}

NetworkThread* NetworkManager::selectBestThread(void)
{
	// select the most suitable thread for handling a
	// new client
	ADDTOCALLSTACK("NetworkManager::selectBestThread");

	NetworkThread* bestThread = nullptr;
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

void NetworkManager::assignNetworkState(NetState* state)
{
	// assign a state to a thread
	ADDTOCALLSTACK("NetworkManager::assignNetworkState");

	NetworkThread* thread = selectBestThread();
	ASSERT(thread != nullptr);
	thread->assignNetworkState(state);
}

bool NetworkManager::checkNewConnection(void)
{
	// check for any new connections
	ADDTOCALLSTACK("NetworkManager::checkNewConnection");

	SOCKET mainSocket = g_Serv.m_SocketMain.GetSocket();

	fd_set fds;
	int count = 0;

	FD_ZERO(&fds);
	AddSocketToSet(fds, mainSocket, count);

	timeval Timeout;		// time to wait for data.
	Timeout.tv_sec=0;
	Timeout.tv_usec=100;	// micro seconds = 1/1000000

	count = select(count+1, &fds, nullptr, nullptr, &Timeout);
	if (count <= 0)
		return false;

	return FD_ISSET(mainSocket, &fds) != 0;
}

void NetworkManager::acceptNewConnection(void)
{
	// accept new connection
	ADDTOCALLSTACK("NetworkManager::acceptNewConnection");

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
	if ( ip.checkPing() ||								// check for ip ban
		( maxIp > 0 && ip.m_connecting > maxIp ) ||		// check for too many connecting
		( climaxIp > 0 && ip.m_connected > climaxIp ) )	// check for too many connected
	{
		EXC_SET_BLOCK("rejected");
		DEBUGNETWORK(("Closing incoming connection [max ip=%d, clients max ip=%d].\n", maxIp, climaxIp));

		CLOSESOCKET(h);

		if (ip.m_blocked)
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (Blocked IP)\n", static_cast<lpctstr>(client_addr.GetAddrStr()));
		else if ( maxIp && ip.m_connecting > maxIp )
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CONNECTINGMAXIP reached %d/%d)\n", static_cast<lpctstr>(client_addr.GetAddrStr()), ip.m_connecting, maxIp);
		else if ( climaxIp && ip.m_connected > climaxIp )
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CLIENTMAXIP reached %d/%d)\n", static_cast<lpctstr>(client_addr.GetAddrStr()), ip.m_connected, climaxIp);
		else if ( ip.m_pings >= NETHISTORY_MAXPINGS )
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (MAXPINGS reached %d/%d)\n", static_cast<lpctstr>(client_addr.GetAddrStr()), ip.m_pings, (int)(NETHISTORY_MAXPINGS) );
		else
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected.\n", static_cast<lpctstr>(client_addr.GetAddrStr()));

		return;
	}

	// select an empty slot
	EXC_SET_BLOCK("detecting slot");
	NetState* state = findFreeSlot();
	if (state == nullptr)
	{
		// not enough empty slots
		EXC_SET_BLOCK("no slot available");
		DEBUGNETWORK(("Unable to allocate new slot for client, too many clients already connected.\n"));
		CLOSESOCKET(h);

		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CLIENTMAX reached)\n", static_cast<lpctstr>(client_addr.GetAddrStr()));
		return;
	}

	DEBUGNETWORK(("%x:Allocated slot for client (%u).\n", state->id(), (uint)(h)));

	// assign slot
	EXC_SET_BLOCK("assigning slot");
	state->init(h, client_addr);

	DEBUGNETWORK(("%x:State initialised, registering client instance.\n", state->id()));

	EXC_SET_BLOCK("recording client");
	if (state->getClient() != nullptr)
		m_clients.InsertHead(state->getClient());

	EXC_SET_BLOCK("assigning thread");
	DEBUGNETWORK(("%x:Selecting a thread to assign to.\n", state->id()));
	assignNetworkState(state);

	DEBUGNETWORK(("%x:Client successfully initialised.\n", state->id()));

	EXC_CATCH;
}

NetState* NetworkManager::findFreeSlot(int start)
{
	// find slot for new client
	ADDTOCALLSTACK("NetworkManager::findFreeSlot");

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

void NetworkManager::start(void)
{
	DEBUGNETWORK(("Registering packets...\n"));
	m_packets.registerStandardPackets();

	// create network states
	ASSERT(m_states == nullptr);
	ASSERT(m_stateCount == 0);
	m_states = new NetState*[g_Cfg.m_iClientsMax];
	for (int l = 0; l < g_Cfg.m_iClientsMax; ++l)
		m_states[l] = new NetState(l);
	m_stateCount = g_Cfg.m_iClientsMax;

	DEBUGNETWORK(("Created %" PRIuSIZE_T " network slots (system limit of %d clients)\n", m_stateCount, FD_SETSIZE));

	// create network threads
	createNetworkThreads(g_Cfg.m_iNetworkThreads);

	m_isThreaded = g_Cfg.m_iNetworkThreads > 0;
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
		// initialise network threads (if g_Cfg.m_iNetworkThreads is == 0 then we'll have only 1 NetworkThread)
		size_t ntCount = m_threads.size();
		for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
		{
			NetworkThread* pThread = *it;

			// Since isThreaded is false the main/first thread will only run Sphere_MainMonitorLoop, and all the sphere/scripts/networking work will be done in the MainThread class,
			//	which code will be executed in another thread named "T_Main". So, in this case even the NetworkThread(s) will be executed on the main thread.
			if (ntCount > 1)
			{
				// If we have more than one thread (this hasn't sense... at this point isThreaded should be == true)
				char name[IThread::m_nameMaxLength];
				snprintf(name, sizeof(name), "T_Worker #%u", (uint)pThread->getId());
				pThread->overwriteInternalThreadName(name);
			}
			
			(*it)->onStart();	// the thread structure (class) was created via createNetworkThreads, but we execute the worker method of that class in this thread
		}
	}
}

void NetworkManager::stop(void)
{
	// terminate child threads
	for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
		(*it)->waitForClose();
}

void NetworkManager::tick(void)
{
	ADDTOCALLSTACK("NetworkManager::tick");

	EXC_TRY("Tick");
	for (int i = 0; i < m_stateCount; ++i)
	{
		NetState* state = m_states[i];
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
			// sent from NetworkOutput::QueuePacketTransaction can be ignored
			// the safest solution to this is to send additional signals from here
			NetworkThread* thread = state->getParentThread();
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
					NetworkThread * thread = state->getParentThread();
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

void NetworkManager::processAllInput(void)
{
	// process network input
	ADDTOCALLSTACK("NetworkManager::processAllInput");

	// checkNewConnection will work on both Windows and Linux because it uses the select method, even if it's not the most efficient way to do it
	if (checkNewConnection())
		acceptNewConnection();

	if (isInputThreaded() == false)	// Don't do this if the input is multi threaded, since the NetworkThread ticks automatically by itself
	{
		// force each thread to process input (NOT THREADSAFE)
		for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
			(*it)->processInput();
	}
}

void NetworkManager::processAllOutput(void)
{
	// process network output
	ADDTOCALLSTACK("NetworkManager::processAllOutput");

	if (isOutputThreaded() == false) // Don't do this if the output is multi threaded, since the NetworkThread ticks automatically by itself
	{
		// force each thread to process output (NOT THREADSAFE)
		for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
			(*it)->processOutput();
	}
}

void NetworkManager::flushAllClients(void)
{
	// flush data for every client
	ADDTOCALLSTACK("NetworkManager::flushAllClients");

	for (NetworkThreadList::iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
		(*it)->flushAllClients();
}

size_t NetworkManager::flush(NetState * state)
{
	// flush data for a single client
	ADDTOCALLSTACK("NetworkManager::flush");
	ASSERT(state != nullptr);
	NetworkThread * thread = state->getParentThread();
	if (thread != nullptr)
		return thread->flush(state);

	return 0;
}

/***************************************************************************
*
*
*	class NetworkThread             Handles i/o for a set of clients, owned
*                                  by a single NetworkManager instance
*
*
***************************************************************************/
NetworkThread::NetworkThread(NetworkManager& manager, size_t id)
	: AbstractSphereThread(GenerateNetworkThreadName(id), IThread::Disabled),
	m_manager(manager), m_id(id)
{
}

NetworkThread::~NetworkThread(void)
{
	// thread name was allocated by GenerateNetworkThreadName, so should be delete[]'d
	delete[] getName();
}

void NetworkThread::assignNetworkState(NetState* state)
{
	ADDTOCALLSTACK("NetworkThread::assignNetworkState");
	m_assignQueue.push(state);
	if (getPriority() == IThread::Disabled)
		awaken();
}

void NetworkThread::checkNewStates(void)
{
	// check for states that have been assigned but not moved to our list
	ADDTOCALLSTACK("NetworkThread::checkNewStates");
	ASSERT(!isActive() || isCurrentThread());

	while (m_assignQueue.empty() == false)
	{
		NetState* state = m_assignQueue.front();
		m_assignQueue.pop();

		ASSERT(state != nullptr);
		state->setParentThread(this);
		m_states.emplace_back(state);
	}
}

void NetworkThread::dropInvalidStates(void)
{
	// check for states in our list that don't belong to us
	ADDTOCALLSTACK("NetworkThread::dropInvalidStates");
	ASSERT(!isActive() || isCurrentThread());

	for (NetworkStateList::iterator it = m_states.begin(); it != m_states.end(); )
	{
		NetState* state = *it;
		if (state->getParentThread() != this)
		{
			// state has been unassigned or reassigned elsewhere
			it = m_states.erase(it);
		}
		else if (state->isInUse() == false)
		{
			// state is invalid
			state->setParentThread(nullptr);
			it = m_states.erase(it);
		}
		else
		{
			// state is good
			++it;
		}
	}
}

void NetworkThread::onStart(void)
{
	AbstractSphereThread::onStart();
	m_input.setOwner(this);
	m_output.setOwner(this);
#ifdef MTNETWORK_INPUT
	m_profile.EnableProfile(PROFILE_NETWORK_RX);
	m_profile.EnableProfile(PROFILE_DATA_RX);
#endif
#ifdef MTNETWORK_OUTPUT
	m_profile.EnableProfile(PROFILE_NETWORK_TX);
	m_profile.EnableProfile(PROFILE_DATA_TX);
#endif
}

void NetworkThread::tick(void)
{
	// process periodic actions
	ADDTOCALLSTACK("NetworkThread::tick");
	checkNewStates();
	dropInvalidStates();

	if (m_states.empty())
	{
		// we haven't been assigned any clients, so go idle for now
		if (getPriority() != IThread::Disabled)
			setPriority(IThread::Low);
		return;
	}

#ifdef MTNETWORK_INPUT
	processInput();
#endif
#ifdef MTNETWORK_OUTPUT
	processOutput();
#endif

	// we're active, take priority
	setPriority(static_cast<IThread::Priority>(g_Cfg.m_iNetworkThreadPriority));
}

void NetworkThread::flushAllClients(void)
{
	ADDTOCALLSTACK("NetworkThread::flushAllClients");
	NetworkThreadStateIterator states(this);
	while (NetState* state = states.next())
		m_output.flush(state);
}

/***************************************************************************
*
*
*	class NetworkInput					Handles network input from clients
*
*
***************************************************************************/
NetworkInput::NetworkInput(void) : m_thread(nullptr)
{
	m_receiveBuffer = new byte[NETWORK_BUFFERSIZE];
	m_decryptBuffer = new byte[NETWORK_BUFFERSIZE];
}

NetworkInput::~NetworkInput()
{
	if (m_receiveBuffer != nullptr)
		delete[] m_receiveBuffer;
	if (m_decryptBuffer != nullptr)
		delete[] m_decryptBuffer;
}

bool NetworkInput::processInput()
{
	ADDTOCALLSTACK("NetworkInput::processInput");
	ASSERT(m_thread != nullptr);

#ifndef MTNETWORK_INPUT
	receiveData();
	processData();
#else
	// when called from within the thread's context, we just receive data
	if ( (m_thread->isActive() && m_thread->isCurrentThread()) ||	// check for multi-threaded network
		!m_thread->isActive() )										// check for single-threaded network
	{
		receiveData();
		processData();
	}

	// when called from outside the thread's context, we process data
	else if (!m_thread->isActive() || !m_thread->isCurrentThread())
	{
		// if the thread does not receive ticks, we must perform a quick select to see if we should
		// wake up the thread
		if (m_thread->isActive() && m_thread->getPriority() == IThread::Disabled)
		{
			fd_set fds;
			if (checkForData(fds))
				m_thread->awaken();
		}

		processData();
	}
#endif
	return true;
}

void NetworkInput::receiveData()
{
	ADDTOCALLSTACK("NetworkInput::receiveData");
	ASSERT(m_thread != nullptr);
#ifdef MTNETWORK_INPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif
	EXC_TRY("ReceiveData");

	// check for incoming data
	EXC_SET_BLOCK("select");
	fd_set fds;
	if (checkForData(fds) == false)
		return;

	EXC_SET_BLOCK("messages");
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		EXC_SET_BLOCK("check socket");
		if (state->isReadClosed())
			continue;

		EXC_SET_BLOCK("start network profile");
		const ProfileTask networkTask(PROFILE_NETWORK_RX);
		if ( ! FD_ISSET(state->m_socket.GetSocket(), &fds))
		{
			state->m_incoming.rawPackets.clean();
			continue;
		}

		// receive data
		EXC_SET_BLOCK("messages - receive");
		int received = state->m_socket.Receive(m_receiveBuffer, NETWORK_BUFFERSIZE, 0);
		if (received <= 0 || received > NETWORK_BUFFERSIZE)
		{
			state->markReadClosed();
			EXC_SET_BLOCK("next state");
			continue;
		}

		EXC_SET_BLOCK("start client profile");
		CurrentProfileData.Count(PROFILE_DATA_RX, received);

		EXC_SET_BLOCK("messages - parse");

		// our objective here is to take the received data and separate it into packets to
		// be stored in NetState::m_incoming.rawPackets
		byte* buffer = m_receiveBuffer;
		while (received > 0)
		{
			// currently we just take the data and push it into a queue for the main thread
			// to parse into actual packets
			// todo: if possible, it would be useful to be able to perform that separation here,
			// but this is made difficult due to the variety of client types and encryptions that
			// may be connecting
			uint length = (uint)received;

			Packet* packet = new Packet(buffer, length);
			state->m_incoming.rawPackets.push(packet);
			buffer += length;
			received -= (int)(length);
		}
	}

	EXC_CATCH;
}

void NetworkInput::processData()
{
	ADDTOCALLSTACK("NetworkInput::processData");
	ASSERT(m_thread != nullptr);
#ifdef MTNETWORK_INPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif
	EXC_TRY("ProcessData");

	// check which states have data
	EXC_SET_BLOCK("messages");
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		EXC_SET_BLOCK("check socket");
		if (state->isReadClosed())
			continue;

		EXC_SET_BLOCK("start network profile");
		const ProfileTask networkTask(PROFILE_NETWORK_RX);

		const CClient* client = state->getClient();
		ASSERT(client != nullptr);

		EXC_SET_BLOCK("check message");
		if (state->m_incoming.rawPackets.empty())
		{
            const CONNECT_TYPE connecttype = client->GetConnectType();
			if ((connecttype != CONNECT_TELNET) && (connecttype != CONNECT_AXIS))
			{
				// check for timeout
				EXC_SET_BLOCK("check frozen");
				const int64 iLastEventDiff = -g_World.GetTimeDiff( client->m_timeLastEvent );
				if ( g_Cfg.m_iDeadSocketTime > 0 && iLastEventDiff > g_Cfg.m_iDeadSocketTime )
				{
					g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Frozen client disconnected (DeadSocketTime reached).\n", state->id());
					state->m_client->addLoginErr( PacketLoginError::Other );		//state->markReadClosed();
				}
			}

			if (state->m_incoming.rawBuffer == nullptr)
			{
				EXC_SET_BLOCK("next state");
				continue;
			}
		}

		EXC_SET_BLOCK("messages - process");
		// we've already received some raw data, we just need to add it to any existing data we have
		while (state->m_incoming.rawPackets.empty() == false)
		{
			Packet* packet = state->m_incoming.rawPackets.front();
			state->m_incoming.rawPackets.pop();
			ASSERT(packet != nullptr);

			EXC_SET_BLOCK("packet - queue data");
			if (state->m_incoming.rawBuffer == nullptr)
			{
				// create new buffer
				state->m_incoming.rawBuffer = new Packet(packet->getData(), packet->getLength());
			}
			else
			{
				// append to buffer
				uint pos = state->m_incoming.rawBuffer->getPosition();
				state->m_incoming.rawBuffer->seek(state->m_incoming.rawBuffer->getLength());
				state->m_incoming.rawBuffer->writeData(packet->getData(), packet->getLength());
				state->m_incoming.rawBuffer->seek(pos);
			}

			delete packet;
		}

		if (g_Serv.IsLoading() == false)
		{
			EXC_SET_BLOCK("start client profile");
			const ProfileTask clientTask(PROFILE_CLIENTS);

			EXC_SET_BLOCK("packets - process");
			Packet* buffer = state->m_incoming.rawBuffer;
			if (buffer != nullptr)
			{
				// we have a buffer of raw bytes, we need to go through them all and process as much as we can
				while (state->isReadClosed() == false && buffer->getRemainingLength() > 0)
				{
					if (processData(state, buffer))
						continue;

					// processData didn't want to use any data, which means we probably
					// received some invalid data or that the packet was malformed
					// best course of action right now is to close the connection
					state->markReadClosed();
					break;
				}

				if (buffer->getRemainingLength() <= 0)
				{
					EXC_SET_BLOCK("packets - clear buffer");
					delete buffer;
					state->m_incoming.rawBuffer = nullptr;
				}
			}
		}
		EXC_SET_BLOCK("next state");
	}

	EXC_CATCH;
}

bool NetworkInput::checkForData(fd_set& fds)
{
	// select() against each socket we own
	ADDTOCALLSTACK("NetworkInput::checkForData");

	EXC_TRY("CheckForData");

	int count = 0;
	FD_ZERO(&fds);

	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		EXC_SET_BLOCK("check socket");
		if (state->isReadClosed())
			continue;

		EXC_SET_BLOCK("check closing");
		if (state->isClosing() || state->m_socket.IsOpen() == false)
			continue;

		AddSocketToSet(fds, state->m_socket.GetSocket(), count);
	}

	EXC_SET_BLOCK("prepare timeout");
	timeval timeout; // time to wait for data.
	timeout.tv_sec = 0;
	timeout.tv_usec = 100; // micro seconds = 1/1000000

	EXC_SET_BLOCK("select");
	return select(count + 1, &fds, nullptr, nullptr, &timeout) > 0;

	EXC_CATCH;
	return false;
}

bool NetworkInput::processData(NetState* state, Packet* buffer)
{
	ADDTOCALLSTACK("NetworkInput::processData");
	ASSERT(state != nullptr);
	ASSERT(buffer != nullptr);
	CClient* client = state->getClient();
	ASSERT(client != nullptr);

	if (client->GetConnectType() == CONNECT_UNK)
		return processUnknownClientData(state, buffer);

	client->m_timeLastEvent = g_World.GetCurrentTime().GetTimeRaw();

	if ( client->m_Crypt.IsInit() == false )
		return processOtherClientData(state, buffer);

	return processGameClientData(state, buffer);
}

bool NetworkInput::processGameClientData(NetState* state, Packet* buffer)
{
	ADDTOCALLSTACK("NetworkInput::processGameClientData");
	EXC_TRY("ProcessGameData");
	ASSERT(state != nullptr);
	ASSERT(buffer != nullptr);
	CClient* client = state->getClient();
	ASSERT(client != nullptr);

	EXC_SET_BLOCK("decrypt message");
	if (!client->m_Crypt.Decrypt(m_decryptBuffer, buffer->getRemainingData(), MAX_BUFFER, buffer->getRemainingLength()))
    {
        g_Log.EventError("NET-IN: processGameClientData failed (Decrypt).\n");
        return false;
    }

	if (state->m_incoming.buffer == nullptr)
	{
		// create new buffer
		state->m_incoming.buffer = new Packet(m_decryptBuffer, buffer->getRemainingLength());
	}
	else
	{
		// append to buffer
		uint pos = state->m_incoming.buffer->getPosition();
		state->m_incoming.buffer->seek(state->m_incoming.buffer->getLength());
		state->m_incoming.buffer->writeData(m_decryptBuffer, buffer->getRemainingLength());
		state->m_incoming.buffer->seek(pos);
	}

	Packet* packet = state->m_incoming.buffer;
	uint remainingLength = packet->getRemainingLength();

	EXC_SET_BLOCK("record message");
	xRecordPacket(client, packet, "client->server");

	// process the message
	EXC_TRYSUB("ProcessMessage");

	while (remainingLength > 0 && state->isClosing() == false)
	{
		ASSERT(remainingLength == packet->getRemainingLength());
		byte packetId = packet->getRemainingData()[0];
		Packet* handler = m_thread->m_manager.getPacketManager().getHandler(packetId);

		if (g_Cfg.m_iDebugFlags & DEBUGF_PACKETS)
		{
#ifdef _DEBUG
			g_Log.Event(LOGM_DEBUG, "Parsing next packet into the received packet data stream (previous dump).\n");
#endif
			Packet packetPart;
			packetPart.writeData(packet->getRemainingData(), packet->getRemainingLength());
			packetPart.resize(packet->getRemainingLength());
			xRecordPacket(client, &packetPart, "sub-packet client->server");
		}

		if (handler != nullptr)
		{
			uint packetLength = handler->checkLength(state, packet);
			if (packetLength <= 0)
			{
				DEBUGNETWORK(("%x:Game packet (0x%x) does not match the expected length, waiting for more data...\n", state->id(), packetId));
				break;
			}

			remainingLength -= packetLength;

			// Packet filtering - check if any function trigger is installed
			//  allow skipping the packet which we do not wish to get
			if (client->xPacketFilter(packet->getRemainingData(), packetLength))
			{
				packet->skip((int)(packetLength));
				continue;
			}

			// copy data to handler
			handler->seek();
			handler->writeData(packet->getRemainingData(), packetLength);
			packet->skip((int)packetLength);

			// move to position 1 (no need for id) and fire onReceive()
			handler->resize(packetLength);
			handler->seek(1);
			handler->onReceive(state);
		}
		else
		{
			// Packet filtering - check if any function trigger is installed
			//  allow skipping the packet which we do not wish to get
			if (client->xPacketFilter(packet->getRemainingData(), remainingLength))
			{
				// todo: adjust packet filter to specify size!
				// packet has been handled by filter but we don't know how big the packet
				// actually is.. we can only assume the entire buffer is used.
				packet->skip((int)(remainingLength));
				remainingLength = 0;
				break;
			}

			// unknown packet.. we could skip 1 byte at a time but this can produce
			// strange behaviours (it's unlikely that only 1 byte is incorrect), so
			// it's best to just discard everything we have
			g_Log.Event(LOGL_WARN, "%x:Unknown game packet (0x%x) received.\n", state->id(), packetId);

#ifdef _DEBUG
            TemporaryString tsDump;
            packet->dump(tsDump);
            g_Log.EventDebug("%x:%s %s\n", client->GetSocketID(), "(unknown packet data) client -> server", (lpctstr)tsDump);
#endif

			packet->skip((int)(remainingLength));
			remainingLength = 0;
		}
	}

	EXC_CATCHSUB("Message");
	EXC_DEBUGSUB_START;
	TemporaryString tsDump;
	packet->dump(tsDump);

	g_Log.EventDebug("%x:Parsing %s", state->id(), static_cast<lpctstr>(tsDump));

	state->m_packetExceptions++;
	if (state->m_packetExceptions > 10)
	{
		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%x:Disconnecting client from account '%s' since it is causing exceptions problems\n", state->id(), client != nullptr && client->GetAccount() ? client->GetAccount()->GetName() : "");
		if (client != nullptr)
			client->addKick(&g_Serv, false);
		else
			state->markReadClosed();
	}

	EXC_DEBUGSUB_END;

	// delete the buffer once it has been exhausted
	if (remainingLength <= 0)
	{
		state->m_incoming.buffer = nullptr;
		delete packet;
	}

	buffer->seek(buffer->getLength());
	return true;

	EXC_CATCH;
	return false;
}

bool NetworkInput::processOtherClientData(NetState* state, Packet* buffer)
{
	// process data from a non-game client
	ADDTOCALLSTACK("NetworkInput::processOtherClientData");
	EXC_TRY("ProcessOtherClientData");
	ASSERT(state != nullptr);
	ASSERT(buffer != nullptr);
	CClient* client = state->getClient();
	ASSERT(client != nullptr);

	switch (client->GetConnectType())
	{
	case CONNECT_CRYPT:
    {
        if (buffer->getRemainingLength() < 5)
        {
            // not enough data to be a real client
            EXC_SET_BLOCK("ping #3");
            client->SetConnectType(CONNECT_UNK);
            if (client->OnRxPing(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
                return false;
            break;
        }

        // first real data from client which we can use to log in
        EXC_SET_BLOCK("encryption setup");
        ASSERT(buffer->getRemainingLength() <= sizeof(CEvent));

        std::unique_ptr<CEvent> evt = std::make_unique<CEvent>();
        memcpy(evt.get(), buffer->getRemainingData(), buffer->getRemainingLength());

        if (evt->Default.m_Cmd == XCMD_EncryptionReply && state->isClientKR())
        {
            EXC_SET_BLOCK("encryption reply");

            // receiving response to 0xE3 packet
            uint iEncKrLen = evt->EncryptionReply.m_len;
            if (buffer->getRemainingLength() < iEncKrLen)
                return false; // need more data

            buffer->skip(iEncKrLen);
            return true;
        }

        client->xProcessClientSetup(evt.get(), buffer->getRemainingLength());
    }
		break;

	case CONNECT_HTTP:
		EXC_SET_BLOCK("http message");
		if (client->OnRxWebPageRequest(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
			return false;
		break;

	case CONNECT_TELNET:
		EXC_SET_BLOCK("telnet message");
		if (client->OnRxConsole(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
			return false;
		break;
	case CONNECT_AXIS:
		EXC_SET_BLOCK("Axis message");
		if (client->OnRxAxis(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
			return false;
		break;

	default:
		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Junk messages with no crypt\n", state->id());
		return false;
	}

	buffer->seek(buffer->getLength());
	return true;
	EXC_CATCH;
	return false;
}

bool NetworkInput::processUnknownClientData(NetState* state, Packet* buffer)
{
	// process data from an unknown client type
	ADDTOCALLSTACK("NetworkInput::processUnknownClientData");
	EXC_TRY("ProcessNewClient");
	ASSERT(state != nullptr);
	ASSERT(buffer != nullptr);
	CClient* client = state->getClient();
	ASSERT(client != nullptr);

    bool fHTTPReq = false;
    const uint uiOrigRemainingLength = buffer->getRemainingLength();
    const byte* const pOrigRemainingData = buffer->getRemainingData();
    if (state->m_seeded == false)
    {
        fHTTPReq =  (uiOrigRemainingLength >= 5 && memcmp(pOrigRemainingData, "GET /", 5) == 0) ||
                    (uiOrigRemainingLength >= 6 && memcmp(pOrigRemainingData, "POST /", 6) == 0);
    }
	if (!fHTTPReq && (uiOrigRemainingLength > INT8_MAX))
	{
		g_Log.EventWarn("%x:Client connected with a seed length of %u exceeding max length limit of %d, disconnecting.\n", state->id(), uiOrigRemainingLength, INT8_MAX);
		return false;
	}

	if (state->m_seeded == false)
	{
		// check for HTTP connection
		if (fHTTPReq)
		{
			EXC_SET_BLOCK("http request");
			if (g_Cfg.m_fUseHTTP != 2)
				return false;

			client->SetConnectType(CONNECT_HTTP);
            client->OnRxWebPageRequest(buffer->getRemainingData(), uiOrigRemainingLength);

			buffer->seek(buffer->getLength());
			return true;
		}

		// check for new seed (sometimes it's received on its own)
		else if (uiOrigRemainingLength == 1 && pOrigRemainingData[0] == XCMD_NewSeed)
		{
			state->m_newseed = true;
			buffer->skip(1);
			return true;
		}

		// check for ping data
		else if (uiOrigRemainingLength < 4)
		{
			EXC_SET_BLOCK("ping #1");
			if (client->OnRxPing(pOrigRemainingData, uiOrigRemainingLength) == false)
				state->markReadClosed();

			buffer->seek(buffer->getLength());
			return true;
		}

		// at least 4 bytes and not a web request, so must assume
		// it is a game client seed
		EXC_SET_BLOCK("game client seed");
		dword seed = 0;

		DEBUGNETWORK(("%x:Client connected with a seed length of %u ([0]=0x%x)\n", state->id(), uiOrigRemainingLength, (int)(pOrigRemainingData[0])));
		if (state->m_newseed || (pOrigRemainingData[0] == XCMD_NewSeed && uiOrigRemainingLength >= NETWORK_SEEDLEN_NEW))
		{
			DEBUGNETWORK(("%x:Receiving new client login handshake.\n", state->id()));

			if (state->m_newseed == false)
			{
				// we don't need the command byte
				state->m_newseed = true;
				buffer->skip(1);
			}

			if (buffer->getRemainingLength() >= (NETWORK_SEEDLEN_NEW - 1))
			{
				seed = buffer->readInt32();
				dword versionMajor = buffer->readInt32();
				dword versionMinor = buffer->readInt32();
				dword versionRevision = buffer->readInt32();
				dword versionPatch = buffer->readInt32();

				DEBUG_MSG(("%x:New Login Handshake Detected. Client Version: %u.%u.%u.%u\n", state->id(), versionMajor, versionMinor, versionRevision, versionPatch));
				state->m_reportedVersion = CCrypto::GetVerFromNumber(versionMajor, versionMinor, versionRevision, versionPatch);
			}
			else
			{
				DEBUGNETWORK(("%x:Not enough data received to be a valid handshake (%" PRIuSIZE_T ").\n", state->id(), buffer->getRemainingLength()));
			}
		}
		else if(pOrigRemainingData[0] == XCMD_UOGRequest && uiOrigRemainingLength == 8)
		{
			DEBUGNETWORK(("%x:Receiving new UOG status request.\n", state->id()));
			buffer->skip(7);
			buffer->getRemainingData()[0] = 0x7F;
			return true;
		}
		else
		{
			// assume it's a normal client login
			DEBUGNETWORK(("%x:Receiving old client login handshake.\n", state->id()));

			seed = buffer->readInt32();
		}

		DEBUGNETWORK(("%x:Client connected with a seed of 0x%x (new handshake=%d, version=%u).\n", state->id(), seed, state->m_newseed ? 1 : 0, state->m_reportedVersion));

		if (seed == 0)
		{
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Invalid client detected, disconnecting.\n", state->id());
			return false;
		}

		state->m_seeded = true;
		state->m_seed = seed;

		if (buffer->getRemainingLength() <= 0 && state->m_seed == 0xFFFFFFFF)
		{
			// UO:KR Client opens connection with 255.255.255.255 and waits for the
			// server to send a packet before continuing
			EXC_SET_BLOCK("KR client seed");

			DEBUG_WARN(("UO:KR Client Detected.\n"));
			client->SetConnectType(CONNECT_CRYPT);
			state->m_clientType = CLIENTTYPE_KR;
			new PacketKREncryption(client);
		}

		return true;
	}

	if (uiOrigRemainingLength < 5)
	{
		// client has a seed assigned but hasn't send enough data to be anything useful,
		// some programs (ConnectUO) send a fake seed when really they're just sending
		// ping data
		EXC_SET_BLOCK("ping #2");
		if (client->OnRxPing(pOrigRemainingData, uiOrigRemainingLength) == false)
			return false;

		buffer->seek(buffer->getLength());
		return true;
	}

	// process login packet for client
	EXC_SET_BLOCK("messages  - setup");
	client->SetConnectType(CONNECT_CRYPT);
	client->xProcessClientSetup(reinterpret_cast<CEvent*>(buffer->getRemainingData()), uiOrigRemainingLength);
	buffer->seek(buffer->getLength());
	return true;

	EXC_CATCH;
	return false;
}


/***************************************************************************
*
*
*	class NetworkOutput					Handles network output to clients
*
*
***************************************************************************/
NetworkOutput::NetworkOutput() : m_thread(nullptr)
{
	m_encryptBuffer = new byte[MAX_BUFFER];
}

NetworkOutput::~NetworkOutput()
{
	if (m_encryptBuffer != nullptr)
		delete[] m_encryptBuffer;
}

bool NetworkOutput::processOutput()
{
	ADDTOCALLSTACK("NetworkOutput::processOutput");
	ASSERT(m_thread != nullptr);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	const ProfileTask networkTask(PROFILE_NETWORK_TX);

	static uchar tick = 0;
	EXC_TRY("NetworkOutput");
	++tick;

	// decide which queues need to be processed
	bool toProcess[PacketSend::PRI_QTY];
	if (m_thread->isActive())
	{
		toProcess[PacketSend::PRI_HIGHEST]  = true;
		toProcess[PacketSend::PRI_HIGH]     = ((tick %  2) ==  1);
		toProcess[PacketSend::PRI_NORMAL]   = ((tick %  4) ==  3);
		toProcess[PacketSend::PRI_LOW]      = ((tick %  8) ==  7);
		toProcess[PacketSend::PRI_IDLE]     = ((tick % 16) == 15);
	}
	else
	{
		// we receive less ticks in single-threaded mode, so process packet
		// queues a bit faster
		toProcess[PacketSend::PRI_HIGHEST]	= true;
		toProcess[PacketSend::PRI_HIGH]		= true;
		toProcess[PacketSend::PRI_NORMAL]	= true;
		toProcess[PacketSend::PRI_LOW]		= ((tick % 2) == 1);
		toProcess[PacketSend::PRI_IDLE]		= ((tick % 4) == 3);
	}

	// process clients which have been marked for flushing
	EXC_SET_BLOCK("flush");
	checkFlushRequests();

	size_t packetsSent = 0;
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		if (state->isWriteClosed())
			continue;

		// process packet queues
		for (int priority = PacketSend::PRI_HIGHEST; priority >= 0; --priority)
		{
			if (toProcess[priority] == false)
				continue;
			else if (state->isWriteClosed())
				break;
			packetsSent += processPacketQueue(state, priority);
		}

		// process asynchronous queue
		if (state->isWriteClosed() == false)
			packetsSent += processAsyncQueue(state);

		// process byte queue
		if (state->isWriteClosed() == false && processByteQueue(state))
			packetsSent++;
	}

	if (packetsSent > 0)
	{
		// notify thread there could be more to process
		if (m_thread->getPriority() == IThread::Disabled)
			m_thread->awaken();
	}

	return packetsSent > 0;
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("ActiveThread=%d, TickCount=%d\n", m_thread->isActive()? 1 : 0, tick);
	EXC_DEBUG_END;
	return false;
}

void NetworkOutput::checkFlushRequests(void)
{
	// check for clients who need data flushing
	ADDTOCALLSTACK("NetworkOutput::checkFlushRequests");
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		if (state->isWriteClosed())
			continue;

		if (state->needsFlush())
			flush(state);

		if (state->isClosing() && state->hasPendingData() == false)
			state->markWriteClosed();
	}
}

size_t NetworkOutput::flush(NetState* state)
{
	// process all queues for a client
	ADDTOCALLSTACK("NetworkOutput::flush");
	ASSERT(state);
	ASSERT(m_thread);

	if (state->isInUse() == false || state->isClosed())
		return 0;

	if (m_thread->isActive() && m_thread->isCurrentThread() == false)
	{
		// when this isn't the active thread, all we can do is raise a request to flush this
		// client later
		state->markFlush(true);
		if (m_thread->getPriority() == IThread::Disabled)
			m_thread->awaken();

		return 0;
	}

	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
	size_t packetsSent = 0;
	for (int priority = PacketSend::PRI_HIGHEST; priority >= 0; --priority)
	{
		if (state->isWriteClosed())
			break;
		packetsSent += processPacketQueue(state, priority);
	}

	if (state->isWriteClosed() == false)
		packetsSent += processAsyncQueue(state);

	if (state->isWriteClosed() == false && processByteQueue(state))
		++packetsSent;

	state->markFlush(false);
	return packetsSent;
}

size_t NetworkOutput::processPacketQueue(NetState* state, uint priority)
{
	// process a client's packet queue
	ADDTOCALLSTACK("NetworkOutput::processPacketQueue");
	ASSERT(state != nullptr);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	if (state->isWriteClosed() ||
		(state->m_outgoing.queue[priority].empty() && state->m_outgoing.currentTransaction == nullptr))
		return 0;

	CClient* client = state->getClient();
	ASSERT(client != nullptr);

	size_t maxPacketsToProcess = NETWORK_MAXPACKETS;
	size_t maxLengthToProcess = NETWORK_MAXPACKETLEN;
	size_t packetsProcessed = 0;
	size_t lengthProcessed = 0;

	while (packetsProcessed < maxPacketsToProcess && lengthProcessed < maxLengthToProcess)
	{
		// select next transaction
		while (state->m_outgoing.currentTransaction == nullptr)
		{
			if (state->m_outgoing.queue[priority].empty())
				break;

			state->m_outgoing.currentTransaction = state->m_outgoing.queue[priority].front();
			state->m_outgoing.queue[priority].pop();
		}

		PacketTransaction* transaction = state->m_outgoing.currentTransaction;
		if (transaction == nullptr)
			break;

		// acquire next packet from transaction
		PacketSend* packet = transaction->front();
		transaction->pop();

		// if the transaction is now empty we can clear it now so we can move
		// on to the next transaction later
		if (transaction->empty())
		{
			state->m_outgoing.currentTransaction = nullptr;
			delete transaction;
		}

		// check if the packet is allowed this packet
		if (state->canReceive(packet) == false || packet->onSend(client) == false)
		{
			delete packet;
			continue;
		}

		EXC_TRY("processPacketQueue");
		lengthProcessed += packet->getLength();
		++packetsProcessed;

		EXC_SET_BLOCK("sending");
		if (sendPacket(state, packet) == false)
		{
			state->clearQueues();
			state->markWriteClosed();
			break;
		}

		EXC_CATCH;
		EXC_DEBUG_START;
		g_Log.EventDebug("id='%x', pri='%u', packet '%" PRIuSIZE_T "' of '%" PRIuSIZE_T "' to send, length '%" PRIuSIZE_T "' of '%" PRIuSIZE_T "'\n",
			state->id(), priority, packetsProcessed, maxPacketsToProcess, lengthProcessed, maxLengthToProcess);
		EXC_DEBUG_END;
	}

	if (packetsProcessed >= maxPacketsToProcess)
	{
		DEBUGNETWORK(("Reached maximum packet count limit for this tick (%" PRIuSIZE_T "/%" PRIuSIZE_T ").\n", packetsProcessed, maxPacketsToProcess));
	}
	if (lengthProcessed >= maxLengthToProcess)
	{
		DEBUGNETWORK(("Reached maximum packet length limit for this tick (%" PRIuSIZE_T "/%" PRIuSIZE_T ").\n", lengthProcessed, maxLengthToProcess));
	}

	return packetsProcessed;
}

size_t NetworkOutput::processAsyncQueue(NetState* state)
{
	// process a client's async queue
	ADDTOCALLSTACK("NetworkOutput::processAsyncQueue");
	ASSERT(state != nullptr);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	if (state->isWriteClosed() || state->isAsyncMode() == false)
		return 0;

	if (state->m_outgoing.asyncQueue.empty() || state->isSendingAsync())
		return 0;

	const CClient* client = state->getClient();
	ASSERT(client != nullptr);

	// select the next packet to send
	PacketSend* packet = nullptr;
	while (state->m_outgoing.asyncQueue.empty() == false)
	{
		packet = state->m_outgoing.asyncQueue.front();
		state->m_outgoing.asyncQueue.pop();

		if (packet != nullptr)
		{
			// check if the client is allowed this
			if (state->canReceive(packet) && packet->onSend(client))
				break;

			// destroy the packet, we aren't going to use it
			delete packet;
			packet = nullptr;
		}
	}

	if (packet == nullptr)
		return 0;

	// start sending the packet we found
	if (sendPacketData(state, packet) == false)
	{
		state->clearQueues();
		state->markWriteClosed();
	}

	return 1;
}

bool NetworkOutput::processByteQueue(NetState* state)
{
	// process a client's byte queue
	ADDTOCALLSTACK("NetworkOutput::processByteQueue");
	ASSERT(state != nullptr);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	if (state->isWriteClosed() || state->m_outgoing.bytes.GetDataQty() <= 0)
		return false;

	size_t result = sendData(state, state->m_outgoing.bytes.RemoveDataLock(), state->m_outgoing.bytes.GetDataQty());
	if (result == _failed_result())
	{
		// error occurred
		state->clearQueues();
		state->markWriteClosed();
		return false;
	}

	if (result > 0)
		state->m_outgoing.bytes.RemoveDataAmount(result);

	return true;
}

bool NetworkOutput::sendPacket(NetState* state, PacketSend* packet)
{
	// send packet to client (can be queued for async operation)
	ADDTOCALLSTACK("NetworkOutput::sendPacket");
	ASSERT(state != nullptr);
	ASSERT(packet != nullptr);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	// check the client is allowed the packet
	if (state->canReceive(packet) == false)
	{
		delete packet;
		return false;
	}

	if (state->isAsyncMode())
	{
		state->m_outgoing.asyncQueue.push(packet);
		return true;
	}

	return sendPacketData(state, packet);
}

bool NetworkOutput::sendPacketData(NetState* state, PacketSend* packet)
{
	// send packet data to client
	ADDTOCALLSTACK("NetworkOutput::sendPacketData");
	ASSERT(state != nullptr);
	ASSERT(packet != nullptr);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	CClient* client = state->getClient();
	ASSERT(client != nullptr);

	EXC_TRY("sendPacketData");
	xRecordPacket(client, packet, "server->client");

	EXC_SET_BLOCK("send trigger");
	if (packet->onSend(client) == false)
	{
		delete packet;
		return true;
	}

	if (client->xOutPacketFilter(packet->getData(), packet->getLength()) == true)
	{
		delete packet;
		return true;
	}

	EXC_SET_BLOCK("prepare data");
	byte* sendBuffer = nullptr;
	uint sendBufferLength = 0;

	if (client->GetConnectType() == CONNECT_GAME)
	{
		// game clients require encryption
		EXC_SET_BLOCK("compress and encrypt");

		// compress
		uint compressLength = client->xCompress(m_encryptBuffer, packet->getData(), MAX_BUFFER, packet->getLength());
        if (compressLength == 0)
        {
            g_Log.EventError("NET-OUT: Trying to compress (Huffman) too much data. Packet will not be sent. (Probably it's a dialog with a lot of data inside).\n");
            return false;
        }

		// encrypt
        if (client->m_Crypt.GetEncryptionType() == ENC_TFISH)
        {
            if (!client->m_Crypt.Encrypt(m_encryptBuffer, m_encryptBuffer, MAX_BUFFER, compressLength))
            {
                g_Log.EventError("NET-OUT: Trying to compress (TFISH/MD5) too much data. Packet will not be sent. (Probably it's a dialog with a lot of data inside).\n");
                return false;
            }
        }

		sendBuffer = m_encryptBuffer;
		sendBufferLength = compressLength;
	}
	else
	{
		// other clients expect plain data
		sendBuffer = packet->getData();
		sendBufferLength = packet->getLength();
	}

	// queue packet data
	EXC_SET_BLOCK("queue data");
	state->m_outgoing.bytes.AddNewData(sendBuffer, sendBufferLength);

	// if buffering is disabled then process the queue straight away
	// we need to do this rather than sending the packet data directly, otherwise if
	// the packet does not send all at once we will be stuck with an incomplete data
	// transfer (and no clean way to recover)
	if (g_Cfg.m_fUseExtraBuffer == false)
	{
		EXC_SET_BLOCK("send data");
		processByteQueue(state);
	}

	EXC_SET_BLOCK("sent trigger");
	packet->onSent(client);
	delete packet;
	return true;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("id='%x', packet '0x%x', length '%" PRIuSIZE_T "'\n",
		state->id(), *packet->getData(), packet->getLength());
	EXC_DEBUG_END;
	return false;
}

size_t NetworkOutput::sendData(NetState* state, const byte* data, size_t length)
{
	// send raw data to client
	ADDTOCALLSTACK("NetworkOutput::sendData");
	ASSERT(state != nullptr);
	ASSERT(data != nullptr);
	ASSERT(length > 0);
	ASSERT(length != _failed_result());
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	EXC_TRY("SendData");
	size_t result = 0;

	EXC_SET_BLOCK("sending");

#if defined(_WIN32) && !defined(_LIBEV)
	if (state->isAsyncMode())
	{
		// send via async winsock
		ZeroMemory(&state->m_overlapped, sizeof(WSAOVERLAPPED));
		state->m_overlapped.hEvent = state;
		state->m_bufferWSA.len = static_cast<ULONG>(length);
		state->m_bufferWSA.buf = reinterpret_cast<CHAR *>(const_cast<byte *>(data));

		DWORD bytesSent;
		if (state->m_socket.SendAsync(&state->m_bufferWSA, 1, &bytesSent, 0, &state->m_overlapped, (LPWSAOVERLAPPED_COMPLETION_ROUTINE)SendCompleted_Winsock) == 0)
		{
			result = bytesSent;
			state->setSendingAsync(true);
		}
		else
		{
			result = 0;
		}
	}
	else
#endif
	{
		// send via standard api
		int sent = state->m_socket.Send(data, (int)(length));
		if (sent > 0)
			result = (size_t)(sent);
		else
			result = 0;
	}

	// check for error
	if (result <= 0)
	{
		EXC_SET_BLOCK("error parse");
		int errorCode = CSocket::GetLastError(true);

#if defined(_WIN32) && !defined(_LIBEV)
		if (state->isAsyncMode() && errorCode == WSA_IO_PENDING)
		{
			// safe to ignore this, data has actually been sent (or will be)
			result = length;
		}
		else if (state->isAsyncMode() == false && errorCode == WSAEWOULDBLOCK)
#else
		if (errorCode == EAGAIN || errorCode == EWOULDBLOCK)
#endif
		{
			// send failed but it is safe to ignore and try again later
			result = 0;
		}
#if defined(_WIN32) && !defined(_LIBEV)
		else if (errorCode == WSAECONNRESET || errorCode == WSAECONNABORTED)
#else
		else if (errorCode == ECONNRESET || errorCode == ECONNABORTED)
#endif
		{
			// connection has been lost, client should be cleared
			result = _failed_result();
		}
		else
		{
			EXC_SET_BLOCK("unexpected connection error");
			if (state->isClosing() == false)
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%x:TX Error %d\n", state->id(), errorCode);

			// Connection error should clear the client too
			result = _failed_result();
		}
	}

	if (result > 0 && result != _failed_result())
		CurrentProfileData.Count(PROFILE_DATA_TX, (dword)(result));

	return result;
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("id='%x', packet '0x%x', length '%" PRIuSIZE_T "'\n", state->id(), *data, length);
	EXC_DEBUG_END;
	return _failed_result();
}

void NetworkOutput::onAsyncSendComplete(NetState* state, bool success)
{
	// notify that async operation completed
	ADDTOCALLSTACK("NetworkOutput::onAsyncSendComplete");
	ASSERT(state != nullptr);
#ifdef _LIBEV
	if (state->isSendingAsync() == true)
		DEBUGNETWORK(("%x: Changing SendingAsync from true to false.\n", state->id()));
#endif
	state->setSendingAsync(false);
	if (success == false)
		return;

#ifdef MTNETWORK_OUTPUT
	// we could process another batch of async data right now, but only if we
	// are in the output thread
	// - WinSock calls this from the same thread (so, enter the if)
	// - LinuxEv calls this from a completely different thread (cannot enter the if)
	if (m_thread->isActive() && m_thread->isCurrentThread())
	{
		if (processAsyncQueue(state) > 0)
			processByteQueue(state);
	}
#endif
}

void NetworkOutput::QueuePacket(PacketSend* packet, bool appendTransaction)
{
	// queue a packet for sending
	ADDTOCALLSTACK("NetworkOutput::QueuePacket");
	ASSERT(packet != nullptr);

	// don't bother queuing packets for invalid sockets
	NetState* state = packet->m_target;
	if (state == nullptr || state->canReceive(packet) == false)
	{
		delete packet;
		return;
	}

	if (state->m_outgoing.pendingTransaction != nullptr && appendTransaction)
		state->m_outgoing.pendingTransaction->emplace_back(packet);
	else
		QueuePacketTransaction(new SimplePacketTransaction(packet));
}

void NetworkOutput::QueuePacketTransaction(PacketTransaction* transaction)
{
	// queue a packet transaction for sending
	ADDTOCALLSTACK("NetworkOutput::QueuePacketTransaction");
	ASSERT(transaction != nullptr);

	// don't bother queuing packets for invalid sockets
	NetState* state = transaction->getTarget();
	if (state == nullptr || state->isInUse() == false || state->isWriteClosed())
	{
		delete transaction;
		return;
	}

	int priority = transaction->getPriority();
	ASSERT(priority >= PacketSend::PRI_IDLE && priority < PacketSend::PRI_QTY);

	// limit by max number of packets in queue
	size_t maxQueueSize = NETWORK_MAXQUEUESIZE;
	if (maxQueueSize > 0)
	{
		while (priority > PacketSend::PRI_IDLE && state->m_outgoing.queue[priority].size() >= maxQueueSize)
		{
			--priority;
			transaction->setPriority(priority);
		}
	}

	state->m_outgoing.queue[priority].push(transaction);

	// notify thread
	NetworkThread* thread = state->getParentThread();
	if (thread != nullptr && thread->getPriority() == IThread::Disabled)
		thread->awaken();
}


/***************************************************************************
*
*
*	class NetworkThreadStateIterator		Works as network state iterator getting the states
*											for a thread, safely.
*
*
***************************************************************************/
NetworkThreadStateIterator::NetworkThreadStateIterator(const NetworkThread* thread)
{
	ASSERT(thread != nullptr);
	m_thread = thread;
	m_nextIndex = 0;
	m_safeAccess = m_thread->isActive() && ! m_thread->isCurrentThread();
}

NetworkThreadStateIterator::~NetworkThreadStateIterator(void)
{
	m_thread = nullptr;
}

NetState* NetworkThreadStateIterator::next(void)
{
	if (m_safeAccess == false)
	{
		// current thread, we can use the thread's state list directly
		// find next non-null state
        const int sz = int(m_thread->m_states.size());
		while (m_nextIndex < sz)
		{
			NetState* state = m_thread->m_states[m_nextIndex];
			++m_nextIndex;

			if (state != nullptr)
				return state;
		}
	}
	else
	{
		// different thread, we have to use the manager's states
		while (m_nextIndex < m_thread->m_manager.m_stateCount)
		{
			NetState* state = m_thread->m_manager.m_states[m_nextIndex];
			++m_nextIndex;

			if (state != nullptr && state->getParentThread() == m_thread)
				return state;
		}
	}

	return nullptr;
}

#endif
