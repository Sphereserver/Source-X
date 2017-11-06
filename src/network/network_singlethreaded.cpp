#ifndef _MTNETWORK

#include "../common/CException.h"
#include "../game/clients/CClient.h"
#include "../game/chars/CChar.h"
#include "../common/CLog.h"
#include "../game/CServer.h"
#include "../game/CServerTime.h"
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
*	class SafeClientIterator		Works as client iterator getting the clients in a thread-safe way
*
*
***************************************************************************/

SafeClientIterator::SafeClientIterator(const NetworkIn* network)
{
	m_network = (network == NULL? &g_NetworkIn : network);
	m_id = -1;
	m_max = m_network->m_stateCount;
}

SafeClientIterator::~SafeClientIterator(void)
{
	m_network = NULL;
}

CClient* SafeClientIterator::next(bool includeClosing)
{
	// this method should be thread-safe, but does not loop through clients in the order that they have
	// connected -- ideally CSObjList (or a similar container for clients) should be traversed from
	// newest client to oldest and be thread-safe)
	while (++m_id < m_max)
	{
		const NetState* state = m_network->m_states[m_id];

		// skip states which do not have a valid client, or are closed
		if (state->isInUse(state->getClient()) == false || state->isClosed())
			continue;

		// skip states which are being closed
		if (includeClosing == false && state->isClosing())
			continue;

		return state->getClient();
	}

	return NULL;
}


/***************************************************************************
*
*
*	class NetworkIn::HistoryIP	Simple interface for IP history maintainese
*
*
***************************************************************************/

NetworkIn::NetworkIn(void) : AbstractSphereThread("NetworkIn", IThread::Highest)
{
	m_lastGivenSlot = 0;
	m_buffer = NULL;
	m_decryptBuffer = NULL;
	m_states = NULL;
	m_stateCount = 0;
}

NetworkIn::~NetworkIn(void)
{
	if (m_buffer != NULL)
	{
		delete[] m_buffer;
		m_buffer = NULL;
	}

	if (m_decryptBuffer != NULL)
	{
		delete[] m_decryptBuffer;
		m_decryptBuffer = NULL;
	}

	if (m_states != NULL)
	{
		for (int i = 0; i < m_stateCount; i++)
		{
			delete m_states[i];
			m_states[i] = NULL;
		}

		delete[] m_states;
		m_states = NULL;
	}

	m_clients.DeleteAll();
}

void NetworkIn::onStart(void)
{
	AbstractSphereThread::onStart();
	m_lastGivenSlot = -1;
	m_buffer = new byte[NETWORK_BUFFERSIZE];
	m_decryptBuffer = new byte[NETWORK_BUFFERSIZE];

	DEBUGNETWORK(("Registering packets...\n"));
	m_packets.registerStandardPackets();

	// create states
	m_states = new NetState*[g_Cfg.m_iClientsMax];
	for (size_t l = 0; l < g_Cfg.m_iClientsMax; l++)
		m_states[l] = new NetState(l);
	m_stateCount = g_Cfg.m_iClientsMax;

	DEBUGNETWORK(("Created %d network slots (system limit of %d clients)\n", m_stateCount, FD_SETSIZE));
}

void NetworkIn::tick(void)
{
	ADDTOCALLSTACK("NetworkIn::tick");

	EXC_TRY("NetworkIn");
	if (g_Serv.m_iExitFlag || g_Serv.m_iModeCode != SERVMODE_Run)
		return;

	// periodically check ip history
	static char iPeriodic = 0;
	if (iPeriodic == 0)
	{
		EXC_SET("periodic");
		periodic();
	}

	if (++iPeriodic > 20)
		iPeriodic = 0;

	// check for incoming data
	EXC_SET("select");
	fd_set readfds;
	int ret = checkForData(&readfds);
	if (ret <= 0)
		return;

	EXC_SET("new connection");
	if (FD_ISSET(g_Serv.m_SocketMain.GetSocket(), &readfds))
		acceptConnection();

	EXC_SET("messages");
	byte* buffer = m_buffer;
	for (int i = 0; i < m_stateCount; i++)
	{
		EXC_SET("start network profile");
		ProfileTask networkTask(PROFILE_NETWORK_RX);

		EXC_SET("messages - next client");
		NetState* client = m_states[i];
		ASSERT(client != NULL);

		EXC_SET("messages - check client");
		if (client->isInUse() == false || client->isClosing() ||
			client->getClient() == NULL || client->m_socket.IsOpen() == false)
			continue;

		EXC_SET("messages - check frozen");
		if (!FD_ISSET(client->m_socket.GetSocket(), &readfds))
		{
			if ((client->m_client->GetConnectType() != CONNECT_TELNET) && (client->m_client->GetConnectType() != CONNECT_AXIS))
			{
				// check for timeout
				int64 iLastEventDiff = -g_World.GetTimeDiff( client->m_client->m_timeLastEvent );
				if ( g_Cfg.m_iDeadSocketTime && iLastEventDiff > g_Cfg.m_iDeadSocketTime )
				{
					g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Frozen client disconnected  (DeadSocketTime reached).\n", client->m_id);
					client->markReadClosed();
				}
			}
			continue;
		}

		// receive data
		EXC_SET("messages - receive");
		size_t received = client->m_socket.Receive(buffer, NETWORK_BUFFERSIZE, 0);
		if (received <= 0 || received > NETWORK_BUFFERSIZE)
		{
			client->markReadClosed();
			continue;
		}

		EXC_SET("start client profile");
		CurrentProfileData.Count(PROFILE_DATA_RX, received);
		ProfileTask clientTask(PROFILE_CLIENTS);

		EXC_SET("messages - process");
		if (client->m_client->GetConnectType() == CONNECT_UNK)
		{
			if (client->m_seeded == false)
			{
				if (received >= 4) // login connection
				{
					EXC_SET("login message");
					if ( memcmp(buffer, "GET /", 5) == 0 || memcmp(buffer, "POST /", 6) == 0 ) // HTTP
					{
						EXC_SET("http request");
						if ( g_Cfg.m_fUseHTTP != 2 )
						{
							client->markReadClosed();
							continue;
						}

						client->m_client->SetConnectType(CONNECT_HTTP);
						if ( !client->m_client->OnRxWebPageRequest(buffer, received) )
						{
							client->markReadClosed();
							continue;
						}

						continue;
					}

					EXC_SET("game client seed");
					dword seed(0);
					size_t iSeedLen(0);
					if (client->m_newseed || (buffer[0] == XCMD_NewSeed && received >= NETWORK_SEEDLEN_NEW))
					{
						DEBUGNETWORK(("%x:Receiving new client login handshake.\n", client->id()));

						CEvent *pEvent = static_cast<CEvent*>((void *)buffer);

						if (client->m_newseed)
						{
							// we already received the 0xEF on its own, so move the pointer
							// back 1 byte to align it
							iSeedLen = NETWORK_SEEDLEN_NEW - 1;
							pEvent = (CEvent *)(((byte*)pEvent) - 1);
						}
						else
						{
							iSeedLen = NETWORK_SEEDLEN_NEW;
							client->m_newseed = true;
						}

						if (received >= iSeedLen)
						{
							DEBUG_MSG(("%x:New Login Handshake Detected. Client Version: %u.%u.%u.%u\n", client->id(), pEvent->NewSeed.m_Version_Maj, pEvent->NewSeed.m_Version_Min, pEvent->NewSeed.m_Version_Rev, pEvent->NewSeed.m_Version_Pat));

							client->m_reportedVersion = CCrypto::GetVerFromNumber(pEvent->NewSeed.m_Version_Maj, pEvent->NewSeed.m_Version_Min, pEvent->NewSeed.m_Version_Rev, pEvent->NewSeed.m_Version_Pat);
							seed = (dword) pEvent->NewSeed.m_Seed;
						}
						else
						{
							DEBUGNETWORK(("%x:Not enough data received to be a valid handshake (%" PRIuSIZE_T ").\n", client->id(), received));
						}
					}
					else
					{
						// assume it's a normal client log in
						DEBUGNETWORK(("%x:Receiving old client login handshake.\n", client->id()));

						seed = ( buffer[0] << 24 ) | ( buffer[1] << 16 ) | ( buffer[2] << 8 ) | buffer[3];
						iSeedLen = NETWORK_SEEDLEN_OLD;
					}

					DEBUGNETWORK(("%x:Client connected with a seed of 0x%x (new handshake=%d, seed length=%" PRIuSIZE_T ", received=%" PRIuSIZE_T ", version=%u).\n", client->id(), seed, client->m_newseed? 1 : 0, iSeedLen, received, client->m_reportedVersion));

					if ( !seed || iSeedLen > received )
					{
						g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Invalid client detected, disconnecting.\n", client->id());
						client->markReadClosed();
						continue;
					}

					client->m_seeded = true;
					client->m_seed = seed;
					buffer += iSeedLen;
					received -= iSeedLen;
				}
				else
				{
					if (*buffer == XCMD_NewSeed)
					{
						// Game client
						client->m_newseed = true;
						continue;
					}

					EXC_SET("ping #1");
					if (client->m_client->OnRxPing(buffer, received) == false)
						client->markReadClosed();

					continue;
				}
			}

			if (received == 0)
			{
				if (client->m_seed == 0xFFFFFFFF)
				{
					// UOKR Client opens connection with 255.255.255.255
					EXC_SET("KR client seed");

					DEBUG_WARN(("UOKR Client Detected.\n"));
					client->m_client->SetConnectType(CONNECT_CRYPT);
					client->m_clientType = CLIENTTYPE_KR;
					new PacketKREncryption(client->getClient());
				}
				continue;
			}

			if (received < 5)
			{
				EXC_SET("ping #2");
				if (client->m_client->OnRxPing(buffer, received) == false)
					client->markReadClosed();

				continue;
			}

			// log in the client
			EXC_SET("messages - setup");
			client->m_client->SetConnectType(CONNECT_CRYPT);
			client->m_client->xProcessClientSetup(static_cast<CEvent*>((void *)buffer), received);
			continue;
		}

		client->m_client->m_timeLastEvent = CServerTime::GetCurrentTime();

		// first data on a new connection - find out what should come next
		if ( client->m_client->m_Crypt.IsInit() == false )
		{
			EXC_SET("encryption setup");
			ASSERT(received <= sizeof(CEvent));

			CEvent evt;
			memcpy(&evt, buffer, received);

			switch ( client->m_client->GetConnectType() )
			{
			case CONNECT_CRYPT:
				if (received >= 5)
				{
					if (*buffer == XCMD_EncryptionReply && client->isClientKR())
					{
						EXC_SET("encryption reply");

						// receiving response to 0xe3 packet
						size_t iEncKrLen = evt.EncryptionReply.m_len;
						if (received < iEncKrLen)
							break;
						else if (received == iEncKrLen)
							continue;

						received -= iEncKrLen;
						client->m_client->xProcessClientSetup(static_cast<CEvent*>((void *)(evt.m_Raw + iEncKrLen)), received);
						break;
					}
					else
					{
						EXC_SET("encryption detection");

						client->m_client->xProcessClientSetup(&evt, received);
					}
				}
				else
				{
					EXC_SET("ping #3");

					// not enough data to be a real client
					client->m_client->SetConnectType(CONNECT_UNK);
					if (client->m_client->OnRxPing(buffer, received) == false)
					{
						client->markReadClosed();
						continue;
					}
				}
				break;

			case CONNECT_HTTP:
				EXC_SET("http message");
				if ( !client->m_client->OnRxWebPageRequest(evt.m_Raw, received) )
				{
					client->markReadClosed();
					continue;
				}
				break;

			case CONNECT_TELNET:
				EXC_SET("telnet message");
				if ( !client->m_client->OnRxConsole(evt.m_Raw, received) )
				{
					client->markReadClosed();
					continue;
				}
				break;
			case CONNECT_AXIS:
				EXC_SET("Axis message");
				if ( !client->m_client->OnRxAxis(evt.m_Raw, received) )
				{
					client->markReadClosed();
					continue;
				}
				break;

			default:
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Junk messages with no crypt\n", client->m_id);
				client->markReadClosed();
				continue;
			}

			continue;
		}

		// decrypt the client data and add it to queue
		EXC_SET("decrypt messages");
		client->m_client->m_Crypt.Decrypt(m_decryptBuffer, buffer, received);

		if (client->m_incoming.buffer == NULL)
		{
			// create new buffer
			client->m_incoming.buffer = new Packet(m_decryptBuffer, received);
		}
		else
		{
			// append to buffer
			size_t pos = client->m_incoming.buffer->getPosition();
			client->m_incoming.buffer->seek(client->m_incoming.buffer->getLength());
			client->m_incoming.buffer->writeData(m_decryptBuffer, received);
			client->m_incoming.buffer->seek(pos);
		}

		Packet* packet = client->m_incoming.buffer;
		size_t len = packet->getRemainingLength();

		EXC_SET("record message");
		xRecordPacket(client->m_client, packet, "client->server");

		// process the message
		EXC_TRYSUB("ProcessMessage");

		while (len > 0 && !client->isClosing())
		{
			byte packetID = packet->getRemainingData()[0];
			Packet* handler = m_packets.getHandler(packetID);

			if (handler != NULL)
			{
				size_t packetLength = handler->checkLength(client, packet);
				//				DEBUGNETWORK(("Checking length: counted %u.\n", packetLength));

				//	fall back and delete the packet
				if (packetLength <= 0)
				{
					DEBUGNETWORK(("%x:Game packet (0x%x) does not match the expected length, waiting for more data...\n", client->id(), packetID));
					break;
				}

				len -= packetLength;

				//	Packet filtering - check if any function triggering is installed
				//		allow skipping the packet which we do not wish to get
				if (client->m_client->xPacketFilter(packet->getRemainingData(), packetLength))
				{
					packet->skip(len);
					len = 0;
					break;
				}

				// copy data to handler
				handler->seek();
				for (size_t j = 0; j < packetLength; j++)
				{
					byte next = packet->readByte();
					handler->writeByte(next);
				}

				handler->resize(packetLength);
				handler->seek(1);
				handler->onReceive(client);
			}
			else
			{
				//	Packet filtering - check if any function triggering is installed
				//		allow skipping the packet which we do not wish to get
				if (client->m_client->xPacketFilter(packet->getRemainingData(), packet->getRemainingLength()))
				{
					// packet has been handled by filter but we don't know how big the packet
					// actually is.. we can only assume the entire buffer is used.
					len = 0;
					break;
				}

				// unknown packet.. we could skip 1 byte at a time but this can produce
				// strange behaviours (it's unlikely that only 1 byte is incorrect), so
				// it's best to discard everything we have
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%x:Unknown game packet (0x%x) received.\n", client->id(), packetID);
				packet->skip(len);
				len = 0;
			}
		}

		EXC_CATCHSUB("Network");
		EXC_DEBUGSUB_START;
		TemporaryString dump;
		packet->dump(dump);

		g_Log.EventDebug("%x:Parsing %s", client->id(), (lpctstr)dump);

		client->m_packetExceptions++;
		if (client->m_packetExceptions > 10 && client->m_client != NULL)
		{
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%x:Disconnecting client from account '%s' since it is causing exceptions problems\n", client->id(), client->m_client->GetAccount() ? client->m_client->GetAccount()->GetName() : "");
			client->m_client->addKick(&g_Serv, false);
		}

		EXC_DEBUGSUB_END;

		// delete the buffer once it has been exhausted
		if (len <= 0)
		{
			client->m_incoming.buffer = NULL;
			delete packet;
		}
	}

	EXC_CATCH;
	EXC_DEBUG_START;

	EXC_DEBUG_END;
}

int NetworkIn::checkForData(fd_set* storage)
{
	//	Berkeley sockets needs nfds to be updated that while in Windows that's ignored
#ifdef _WIN32
#define ADDTOSELECT(_x_)	FD_SET(_x_, storage)
#else
#define ADDTOSELECT(_x_)	{ FD_SET(_x_, storage); if ( _x_ > nfds ) nfds = _x_; }
#endif

	ADDTOCALLSTACK("NetworkIn::checkForData");


	EXC_TRY("CheckForData");
	int nfds = 0;

	EXC_SET("zero");
	FD_ZERO(storage);

#ifndef _WIN32
#ifdef LIBEV_REGISTERMAIN
	if (g_Cfg.m_fUseAsyncNetwork == 0)
#endif
#endif
	{
		EXC_SET("main socket");
		ADDTOSELECT(g_Serv.m_SocketMain.GetSocket());
	}

	EXC_SET("check states");
	for (int i = 0; i < m_stateCount; i++ )
	{
		EXC_SET("check socket");
		NetState* state = m_states[i];
		if ( state->isInUse() == false )
			continue;

		EXC_SET("cleaning queues");
		for (int ii = 0; ii < PacketSend::PRI_QTY; ii++)
			state->m_outgoing.queue[ii].clean();

		EXC_SET("check closing");
		if (state->isClosing())
		{
			if (state->isClosed() == false)
			{
				EXC_SET("check pending data");
				if (state->hasPendingData())
				{
					if (state->needsFlush() == false)
					{
						DEBUGNETWORK(("%x:Flushing data for client.\n", state->id()));

						EXC_SET("flush data");
						g_NetworkOut.flush(state->getClient());
					}
					else
					{
						DEBUGNETWORK(("%x:Waiting for data to be flushed.\n", state->id()));
					}
					continue;
				}

				EXC_SET("mark closed");
				state->markReadClosed();
				if (g_NetworkOut.isActive() == false)
					state->markWriteClosed();
			}

			EXC_SET("check closed");
			if (state->isClosed() && state->isSendingAsync() == false)
			{
				EXC_SET("clear socket");
				DEBUGNETWORK(("%x:Client is being cleared since marked to close.\n", state->id()));
				state->clear();
			}

			continue;
		}

		if (state->m_socket.IsOpen())
		{
			EXC_SET("add to select");
			ADDTOSELECT(state->m_socket.GetSocket());
		}
	}

	EXC_SET("prepare timeout");
	timeval Timeout;	// time to wait for data.
	Timeout.tv_sec=0;
	Timeout.tv_usec=100;	// micro seconds = 1/1000000

	EXC_SET("perform select");
	return select(nfds+1, storage, NULL, NULL, &Timeout);

	EXC_CATCH;
	EXC_DEBUG_START;

	EXC_DEBUG_END;
	return 0;

#undef ADDTOSELECT
}

void NetworkIn::acceptConnection(void)
{
	ADDTOCALLSTACK("NetworkIn::acceptConnection");

	EXC_TRY("acceptConnection");
	CSocketAddress client_addr;

	DEBUGNETWORK(("Receiving new connection\n"));

	EXC_SET("accept");
	SOCKET h = g_Serv.m_SocketMain.Accept(client_addr);
	if (( h >= 0 ) && ( h != INVALID_SOCKET ))
	{
		EXC_SET("ip history");

		DEBUGNETWORK(("Retrieving IP history for '%s'.\n", client_addr.GetAddrStr()));
		HistoryIP& ip = m_ips.getHistoryForIP(client_addr);
		int maxIp = g_Cfg.m_iConnectingMaxIP;
		int climaxIp = g_Cfg.m_iClientsMaxIP;

		DEBUGNETWORK(("Incoming connection from '%s' [blocked=%d, ttl=%d, pings=%d, connecting=%d, connected=%d]\n",
			ip.m_ip.GetAddrStr(), ip.m_blocked, ip.m_ttl, ip.m_pings, ip.m_connecting, ip.m_connected));

		//	ip is blocked
		if ( ip.checkPing() ||
			// or too much connect tries from this ip
			( maxIp && ( ip.m_connecting > maxIp )) ||
			// or too much clients from this ip
			( climaxIp && ( ip.m_connected > climaxIp ))
			)
		{
			EXC_SET("rejecting");
			DEBUGNETWORK(("Closing incoming connection [max ip=%d, clients max ip=%d).\n", maxIp, climaxIp));

			CLOSESOCKET(h);

			if (ip.m_blocked)
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (Blocked IP)\n", (lpctstr)client_addr.GetAddrStr());
			else if ( maxIp && ip.m_connecting > maxIp )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CONNECTINGMAXIP reached %d/%d)\n", (lpctstr)client_addr.GetAddrStr(), ip.m_connecting, maxIp);
			else if ( climaxIp && ip.m_connected > climaxIp )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CLIENTMAXIP reached %d/%d)\n", (lpctstr)client_addr.GetAddrStr(), ip.m_connected, climaxIp);
			else if ( ip.m_pings >= NETHISTORY_MAXPINGS )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (MAXPINGS reached %d/%d)\n", (lpctstr)client_addr.GetAddrStr(), ip.m_pings, (int)(NETHISTORY_MAXPINGS));
			else
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected.\n", (lpctstr)client_addr.GetAddrStr());
		}
		else
		{
			EXC_SET("detecting slot");
			int slot = getStateSlot();
			if ( slot == -1 )			// we do not have enough empty slots for clients
			{
				EXC_SET("no slot ready");
				DEBUGNETWORK(("Unable to allocate new slot for client, too many clients already.\n"));

				CLOSESOCKET(h);

				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CLIENTMAX reached)\n", (lpctstr)client_addr.GetAddrStr());
			}
			else
			{
				DEBUGNETWORK(("%x:Allocated slot for client (%d).\n", slot, (int)h));

				EXC_SET("assigning slot");
				m_states[slot]->init(h, client_addr);

				DEBUGNETWORK(("%x:State initialised, registering client instance.\n", slot));

				EXC_SET("recording client");
				if (m_states[slot]->m_client != NULL)
					m_clients.InsertHead(m_states[slot]->getClient());

				DEBUGNETWORK(("%x:Client successfully initialised.\n", slot));
			}
		}
	}
	EXC_CATCH;
}

int NetworkIn::getStateSlot(int startFrom)
{
	ADDTOCALLSTACK("NetworkIn::getStateSlot");

	if ( startFrom == -1 )
		startFrom = m_lastGivenSlot + 1;

	//	give ordered slot number, each time incrementing by 1 for easier log view
	for (int i = startFrom; i < m_stateCount; i++ )
	{
		if (m_states[i]->isInUse())
			continue;

		return ( m_lastGivenSlot = i );
	}

	//	we did not find empty slots till the end, try rescan from beginning
	if ( startFrom > 0 )
		return getStateSlot(0);

	//	no empty slots exists, arbitrary too many clients
	return -1;
}

void NetworkIn::periodic(void)
{
	ADDTOCALLSTACK("NetworkIn::periodic");

	EXC_TRY("periodic");
	// check if max connecting limit is obeyed
	int connectingMax = g_Cfg.m_iConnectingMax;
	if (connectingMax > 0)
	{
		EXC_SET("limiting connecting clients");
		int connecting = 0;

		ClientIterator clients(this);
		for (const CClient* client = clients.next(); client != NULL; client = clients.next())
		{
			if (client->IsConnecting())
			{
				if (++connecting > connectingMax)
				{
					DEBUGNETWORK(("%x:Closing client since '%d' connecting overlaps '%d'\n", client->m_net->id(), connecting, connectingMax));

					client->m_net->markReadClosed();
				}
			}
			if (connecting > connectingMax)
			{
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%d clients in connect mode (max %d), closing %d\n", connecting, connectingMax, connecting - connectingMax);
			}
		}
	}

	// tick the ip history, remove some from the list
	EXC_SET("ticking history");
	m_ips.tick();

	// resize m_states to account for m_iClientsMax changes
	int max = g_Cfg.m_iClientsMax;
	if (max > m_stateCount)
	{
		EXC_SET("increasing network state size");
		DEBUGNETWORK(("Increasing number of client slots from %d to %d\n", m_stateCount, max));

		// reallocate state buffer to accomodate additional clients
		int prevCount = m_stateCount;
		NetState** prevStates = m_states;

		NetState** newStates = new NetState*[max];
		memcpy(newStates, prevStates, m_stateCount * sizeof(NetState*));
		for (int i = prevCount; i < max; i++)
			newStates[i] = new NetState(i);

		m_states = newStates;
		m_stateCount = max;

		// destroy previous states
		delete[] prevStates;
	}
	else if (max < m_stateCount)
	{
		EXC_SET("decreasing network state size");
		DEBUGNETWORK(("Decreasing number of client slots from %d to %d\n", m_stateCount, max));

		// move used slots to free spaces if possible
		defragSlots(max);

		// delete excess states but leave array intact
		for (int i = max; i < m_stateCount; i++)
		{
			delete m_states[i];
			m_states[i] = NULL;
		}

		m_stateCount = max;
	}

	EXC_CATCH;
}

void NetworkIn::defragSlots(int fromSlot)
{
	ADDTOCALLSTACK("NetworkIn::defragSlots");

	int nextUsedSlot = fromSlot - 1;

	for (int i = 0; i < m_stateCount; i++)
	{
		// don't interfere with in-use states
		if (m_states[i] != NULL && m_states[i]->isInUse())
			continue;

		// find next used slot
		bool slotFound = false;
		while (slotFound == false)
		{
			if (++nextUsedSlot >= m_stateCount)
				break;

			NetState* state = m_states[nextUsedSlot];
			if (state != NULL && state->isInUse())
				slotFound = true;
		}

		// no more slots to be moved
		if (slotFound == false)
			break;

		if (nextUsedSlot != i)
		{
			DEBUGNETWORK(("Moving client '%x' to slot '%x'.\n", nextUsedSlot, i));

			// swap states
			NetState* usedSlot = m_states[nextUsedSlot];
			usedSlot->setId(i);

			m_states[nextUsedSlot] = m_states[i];
			m_states[i] = usedSlot;
		}
	}
}


/***************************************************************************
*
*
*	class NetworkOut			Networking thread used for queued sending outgoing packets
*
*
***************************************************************************/

NetworkOut::NetworkOut(void) : AbstractSphereThread("NetworkOut", IThread::RealTime)
{
	m_profile.EnableProfile(PROFILE_NETWORK_TX);
	m_profile.EnableProfile(PROFILE_DATA_TX);

	m_encryptBuffer = new byte[MAX_BUFFER];
}

NetworkOut::~NetworkOut(void)
{
	if (m_encryptBuffer != NULL)
	{
		delete[] m_encryptBuffer;
		m_encryptBuffer = NULL;
	}
}

void NetworkOut::tick(void)
{
	ADDTOCALLSTACK("NetworkOut::tick");
	ProfileTask networkTask(PROFILE_NETWORK_TX);

	if (g_Serv.m_iExitFlag || g_Serv.m_iModeCode == SERVMODE_Exiting)
	{
		setPriority(IThread::Highest);
		return;
	}

	static uchar iCount = 0;
	EXC_TRY("NetworkOut");

	iCount++;

	bool toProcess[PacketSend::PRI_QTY];
	if (isActive() == false)
	{
		// process queues faster in single-threaded mode
		toProcess[PacketSend::PRI_HIGHEST]	= true;
		toProcess[PacketSend::PRI_HIGH]		= true;
		toProcess[PacketSend::PRI_NORMAL]	= true;
		toProcess[PacketSend::PRI_LOW]		= ((iCount % 2) == 1);
		toProcess[PacketSend::PRI_IDLE]		= ((iCount % 4) == 3);
	}
	else
	{
		// throttle rate of sending in multi-threaded mode
		toProcess[PacketSend::PRI_HIGHEST]	= true;
		toProcess[PacketSend::PRI_HIGH]		= ((iCount %  2) == 1);
		toProcess[PacketSend::PRI_NORMAL]	= ((iCount %  4) == 3);
		toProcess[PacketSend::PRI_LOW]		= ((iCount %  8) == 7);
		toProcess[PacketSend::PRI_IDLE]		= ((iCount % 16) == 15);

		EXC_SET("flush");
		proceedFlush();
	}

	int packetsSent = 0;

	SafeClientIterator clients;
	while (CClient* client = clients.next())
	{
		const NetState* state = client->GetNetState();
		ASSERT(state != NULL);

		EXC_SET("highest");
		if (toProcess[PacketSend::PRI_HIGHEST] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_HIGHEST);

		EXC_SET("high");
		if (toProcess[PacketSend::PRI_HIGH] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_HIGH);

		EXC_SET("normal");
		if (toProcess[PacketSend::PRI_NORMAL] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_NORMAL);

		EXC_SET("low");
		if (toProcess[PacketSend::PRI_LOW] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_LOW);

		EXC_SET("idle");
		if (toProcess[PacketSend::PRI_IDLE] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_IDLE);

		EXC_SET("async");
		if (state->isWriteClosed() == false)
			packetsSent += proceedQueueAsync(client);

		if (state->isWriteClosed() == false)
			proceedQueueBytes(client);
	}

	// increase priority during 'active' periods
	if (packetsSent > 0)
		setPriority(IThread::RealTime);
	else
		setPriority(IThread::Highest);

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("ActiveThread=%d, TickCount=%d\n", isActive()? 1 : 0, iCount);
	EXC_DEBUG_END;
}

void NetworkOut::schedule(const PacketSend* packet, bool appendTransaction)
{
	ADDTOCALLSTACK("NetworkOut::schedule");

	ASSERT(packet != NULL);
	scheduleOnce(packet->clone(), appendTransaction);
}

void NetworkOut::scheduleOnce(PacketSend* packet, bool appendTransaction)
{
	ADDTOCALLSTACK("NetworkOut::scheduleOnce");

	ASSERT(packet != NULL);
	NetState* state = packet->m_target;
	ASSERT(state != NULL);

	// don't bother queuing packets for invalid sockets
	if (state == NULL || state->isInUse() == false || state->isWriteClosed())
	{
		delete packet;
		return;
	}

	if (state->m_outgoing.pendingTransaction != NULL && appendTransaction)
		state->m_outgoing.pendingTransaction->push_back(packet);
	else
		scheduleOnce(new SimplePacketTransaction(packet));
}

void NetworkOut::scheduleOnce(PacketTransaction* transaction)
{
	ADDTOCALLSTACK("NetworkOut::scheduleOnce[2]");

	ASSERT(transaction != NULL);
	NetState* state = transaction->getTarget();
	ASSERT(state != NULL);

	// don't bother queuing packets for invalid sockets
	if (state == NULL || state->isInUse() == false || state->isWriteClosed())
	{
		delete transaction;
		return;
	}

	long priority = transaction->getPriority();
	ASSERT(priority >= PacketSend::PRI_IDLE && priority < PacketSend::PRI_QTY);

#ifdef NETWORK_MAXQUEUESIZE
	// limit by number of packets to be in queue
	if (priority > PacketSend::PRI_IDLE)
	{
		size_t maxClientPackets = NETWORK_MAXQUEUESIZE;
		if (maxClientPackets > 0)
		{
			if (state->m_outgoing.queue[priority].size() >= maxClientPackets)
			{
				//				DEBUGNETWORK(("%x:Packet decreased priority due to overal amount %d overlapping %d.\n", state->id(), state->m_outgoing.queue[priority].size(), maxClientPackets));

				transaction->setPriority(priority-1);
				scheduleOnce(transaction);
				return;
			}
		}
	}
#endif

	state->m_outgoing.queue[priority].push(transaction);
}

void NetworkOut::flushAll(void)
{
	ADDTOCALLSTACK("NetworkOut::flushAll");

	SafeClientIterator clients;
	while (CClient* client = clients.next(true))
		flush(client);
}

void NetworkOut::flush(CClient* client)
{
	ADDTOCALLSTACK("NetworkOut::flush");

	ASSERT(client != NULL);

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);
	if (state->isInUse(client) == false)
		return;

	// flushing is not thread-safe, and can only be performed by the NetworkOut thread
	if (isActive() && isCurrentThread() == false)
	{
		// mark state to be flushed
		state->markFlush(true);
	}
	else
	{
		for (int priority = 0; priority < PacketSend::PRI_QTY && state->isWriteClosed() == false; priority++)
			proceedQueue(client, priority);

		if (state->isWriteClosed() == false)
			proceedQueueAsync(client);

		if (state->isWriteClosed() == false)
			proceedQueueBytes(client);

		state->markFlush(false);
	}
}

void NetworkOut::proceedFlush(void)
{
	ADDTOCALLSTACK("NetworkOut::proceedFlush");

	SafeClientIterator clients;
	while (CClient* client = clients.next(true))
	{
		NetState* state = client->GetNetState();
		ASSERT(state != NULL);

		if (state->isWriteClosed())
			continue;

		if (state->needsFlush())
			flush(client);

		if (state->isClosing() && state->hasPendingData() == false)
			state->markWriteClosed();
	}
}

int NetworkOut::proceedQueue(CClient* client, int priority)
{
	ADDTOCALLSTACK("NetworkOut::proceedQueue");

	int maxClientPackets = NETWORK_MAXPACKETS;
	int maxClientLength = NETWORK_MAXPACKETLEN;
	CServerTime time = CServerTime::GetCurrentTime();

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	if (state->isWriteClosed() || (state->m_outgoing.queue[priority].empty() && state->m_outgoing.currentTransaction == NULL))
		return 0;

	int length = 0;

	// send N transactions from the queue
	int i;
	for (i = 0; i < maxClientPackets; i++)
	{
		// select next transaction
		while (state->m_outgoing.currentTransaction == NULL)
		{
			if (state->m_outgoing.queue[priority].empty())
				break;

			state->m_outgoing.currentTransaction = state->m_outgoing.queue[priority].front();
			state->m_outgoing.queue[priority].pop();
		}

		PacketTransaction* transaction = state->m_outgoing.currentTransaction;
		if (transaction == NULL)
			break;

		// acquire next packet from the transaction
		PacketSend* packet = transaction->empty()? NULL : transaction->front();
		transaction->pop();

		if (transaction->empty())
		{
			// no more packets left in the transacton, clear it so we can move on to the next one
			state->m_outgoing.currentTransaction = NULL;
			delete transaction;
		}

		if (state->canReceive(packet) == false || packet->onSend(client) == false)
		{
			// don't count this towards the limit, allow an extra packet to be processed
			delete packet;
			maxClientPackets++;
			continue;
		}

		EXC_TRY("proceedQueue");
		length += packet->getLength();

		EXC_SET("sending");
		if (sendPacket(client, packet) == false)
		{
			state->clearQueues();
			state->markWriteClosed();
			break;
		}

		EXC_SET("check length");
		if (length > maxClientLength)
		{
			//			DEBUGNETWORK(("%x:Packets sending stopped at %d packet due to overall length %d overlapping %d.\n", state->id(), i, length, maxClientLength));

			break;
		}

		EXC_CATCH;
		EXC_DEBUG_START;
		g_Log.EventDebug("id='%x', pri='%d', packet '%d' of '%d' to send, length '%d' of '%d'\n",
			state->id(), priority, i, maxClientPackets, length, maxClientLength);
		EXC_DEBUG_END;
	}
	return i;
}

int NetworkOut::proceedQueueAsync(CClient* client)
{
	ADDTOCALLSTACK("NetworkOut::proceedQueueAsync");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	if (state->isWriteClosed() || state->isAsyncMode() == false)
		return 0;

	state->m_outgoing.asyncQueue.clean();
	if (state->m_outgoing.asyncQueue.empty() || state->isSendingAsync())
		return 0;

	// get next packet
	PacketSend* packet = NULL;

	while (state->m_outgoing.asyncQueue.empty() == false)
	{
		packet = state->m_outgoing.asyncQueue.front();
		state->m_outgoing.asyncQueue.pop();

		if (state->canReceive(packet) == false || packet->onSend(client) == false)
		{
			if (packet != NULL)
			{
				delete packet;
				packet = NULL;
			}

			continue;
		}

		break;
	}

	// start sending
	if (packet != NULL)
	{
		if (sendPacketNow(client, packet) == false)
		{
			state->clearQueues();
			state->markWriteClosed();
		}

		return 1;
	}

	return 0;
}

void NetworkOut::proceedQueueBytes(CClient* client)
{
	ADDTOCALLSTACK("NetworkOut::proceedQueueBytes");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	if (state->isWriteClosed() || state->m_outgoing.bytes.GetDataQty() <= 0)
		return;

	int ret = sendBytesNow(client, state->m_outgoing.bytes.RemoveDataLock(), state->m_outgoing.bytes.GetDataQty());
	if (ret > 0)
	{
		state->m_outgoing.bytes.RemoveDataAmount(ret);
	}
	else if (ret < 0)
	{
		state->clearQueues();
		state->markWriteClosed();
	}
}

void NetworkOut::onAsyncSendComplete(NetState* state, bool success)
{
	ADDTOCALLSTACK("NetworkOut::onAsyncSendComplete");

	//DEBUGNETWORK(("AsyncSendComplete\n"));
	ASSERT(state != NULL);

	state->setSendingAsync(false);
	if (success == false)
		return;

	if (proceedQueueAsync(state->getClient()) != 0)
		proceedQueueBytes(state->getClient());
}

bool NetworkOut::sendPacket(CClient* client, PacketSend* packet)
{
	ADDTOCALLSTACK("NetworkOut::sendPacket");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	// only allow high-priority packets to be sent to queued for closed sockets
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

	return sendPacketNow(client, packet);
}

#ifdef _WIN32

void CALLBACK SendCompleted(dword dwError, dword cbTransferred, LPWSAOVERLAPPED lpOverlapped, uint64 iFlags)
{
	UNREFERENCED_PARAMETER(iFlags);
	ADDTOCALLSTACK("SendCompleted");

	NetState* state = reinterpret_cast<NetState *>(lpOverlapped->hEvent);
	if (state == NULL)
	{
		DEBUGNETWORK(("Async i/o operation completed without client context.\n"));
		return;
	}

	if (dwError != 0)
	{
		DEBUGNETWORK(("%x:Async i/o operation completed with error code 0x%x, %d bytes sent.\n", state->id(), dwError, cbTransferred));
	}
	//else
	//{
	//	DEBUGNETWORK(("%x:Async i/o operation completed successfully, %d bytes sent.\n", state->id(), cbTransferred));
	//}

	g_NetworkOut.onAsyncSendComplete(state, dwError == 0 && cbTransferred > 0);
}

#endif

bool NetworkOut::sendPacketNow(CClient* client, PacketSend* packet)
{
	ADDTOCALLSTACK("NetworkOut::sendPacketNow");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	EXC_TRY("proceedQueue");

	xRecordPacket(client, packet, "server->client");

	EXC_SET("send trigger");
	if (packet->onSend(client))
	{
		byte* sendBuffer = NULL;
		dword sendBufferLength = 0;

		if (state->m_client == NULL)
		{
			DEBUGNETWORK(("%x:Sending packet to closed client?\n", state->id()));

			sendBuffer = packet->getData();
			sendBufferLength = packet->getLength();
		}
		else
		{

			if (client->xOutPacketFilter(packet->getData(), packet->getLength()) == true)
			{
				delete packet;
				return true;
			}

			if (client->GetConnectType() == CONNECT_GAME)
			{
				// game clients require encryption
				EXC_SET("compress and encrypt");

				// compress
				size_t compressLength = client->xCompress(m_encryptBuffer, packet->getData(), packet->getLength());

				// encrypt
				if (client->m_Crypt.GetEncryptionType() == ENC_TFISH)
					client->m_Crypt.Encrypt(m_encryptBuffer, m_encryptBuffer, compressLength);

				sendBuffer = m_encryptBuffer;
				sendBufferLength = compressLength;
			}
			else
			{
				// other clients expect plain data
				sendBuffer = packet->getData();
				sendBufferLength = packet->getLength();
			}
		}

		if ( g_Cfg.m_fUseExtraBuffer )
		{
			// queue packet data
			state->m_outgoing.bytes.AddNewData(sendBuffer, sendBufferLength);
		}
		else
		{
			// send packet data now
			size_t totalSent = 0;

			do
			{
				// a single send attempt may not send the entire buffer, so we need to
				// loop through this process until we have sent the expected number of bytes
				int sent = sendBytesNow(client, sendBuffer + totalSent, sendBufferLength - totalSent);
				if (sent > 0)
				{
					totalSent += sent;
				}
				else if (sent == 0 && totalSent == 0)
				{
					// if no bytes were sent then we can try to ignore the error
					// by re-queueing the packet, but this is only viable if no
					// data has been sent (if part of the packet has gone, we have
					// no choice but to disconnect the client since we'll be oos)

					// WARNING: scheduleOnce is intended to be used by the main
					// thread, and is likely to cause stability issues when called
					// from here!
					DEBUGNETWORK(("%x:Send failure occurred with a non-critical error. Requeuing packet may affect stability.\n", state->id()));
					scheduleOnce(packet, true);
					return true;
				}
				else
				{
					// critical error occurred
					delete packet;
					return false;
				}
			}
			while (totalSent < sendBufferLength);
		}

		EXC_SET("sent trigger");
		packet->onSent(client);
	}

	delete packet;
	return true;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("id='%x', packet '0x%x', length '%" PRIuSIZE_T "'\n",
		state->id(), *packet->getData(), packet->getLength());
	EXC_DEBUG_END;
	return false;
}

int NetworkOut::sendBytesNow(CClient* client, const byte* data, dword length)
{
	ADDTOCALLSTACK("NetworkOut::sendBytesNow");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	EXC_TRY("sendBytesNow");
	int ret = 0;

	// send data
	EXC_SET("sending");

#if defined(_WIN32) && !defined(_LIBEV)
	if (state->isAsyncMode())
	{
		// send via async winsock
		ZeroMemory(&state->m_overlapped, sizeof(WSAOVERLAPPED));
		state->m_overlapped.hEvent = state;
		state->m_bufferWSA.len = length;
		state->m_bufferWSA.buf = (CHAR*)data;

		DWORD bytesSent;
		if (state->m_socket.SendAsync(&state->m_bufferWSA, 1, &bytesSent, 0, &state->m_overlapped, (LPWSAOVERLAPPED_COMPLETION_ROUTINE)SendCompleted) == 0)
		{
			ret = bytesSent;
			state->setSendingAsync(true);
		}
		else
			ret = 0;
	}
	else
#endif
	{
		// send via standard api
		ret = state->m_socket.Send(data, length);
	}

	// check for error
	if (ret <= 0)
	{
		EXC_SET("error parse");
		int errCode = CSocket::GetLastError(true);

#ifdef _WIN32
		if (state->isAsyncMode() && errCode == WSA_IO_PENDING)
		{
			// safe to ignore this, data has actually been sent
			ret = length;
		}
		else if (state->isAsyncMode() == false && errCode == WSAEWOULDBLOCK)
#else
		if (errCode == EAGAIN || errCode == EWOULDBLOCK)
#endif
		{
			// send failed but it is safe to ignore and try again later
			ret = 0;
		}
#ifdef _WIN32
		else if (errCode == WSAECONNRESET || errCode == WSAECONNABORTED)
#else
		else if (errCode == ECONNRESET || errCode == ECONNABORTED)
#endif
		{
			// connection has been lost, client should be cleared
			ret = INT32_MIN;
		}
		else
		{
			EXC_SET("unexpected connection error - delete packet");

			if (state->isClosing() == false)
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%x:TX Error %d\n", state->id(), errCode);

#ifndef _WIN32
			return INT32_MIN;
#else
			ret = 0;
#endif
		}
	}

	if (ret > 0)
		CurrentProfileData.Count(PROFILE_DATA_TX, ret);

	return ret;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("id='%x', packet '0x%x', length '%u'\n", state->id(), *data, length);
	EXC_DEBUG_END;
	return INT32_MIN;
}

#endif
