
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

#if !defined(_WIN32) || defined(_LIBEV)
	extern LinuxEv g_NetworkEvent;
#endif

#ifndef _MTNETWORK
	NetworkIn g_NetworkIn;
	NetworkOut g_NetworkOut;
#else
	NetworkManager g_NetworkManager;
#endif

//
// Packet logging
//
#if defined(_PACKETDUMP) || defined(_DUMPSUPPORT)

void xRecordPacketData(const CClient* client, const byte* data, size_t length, lpctstr heading)
{
#ifdef _DUMPSUPPORT
	if (client->GetAccount() != NULL && strnicmp(client->GetAccount()->GetName(), (lpctstr) g_Cfg.m_sDumpAccPackets, strlen( client->GetAccount()->GetName())))
		return;
#else
	if (!(g_Cfg.m_wDebugFlags & DEBUGF_PACKETS))
		return;
#endif

	Packet packet(data, length);
	xRecordPacket(client, &packet, heading);
}

void xRecordPacket(const CClient* client, Packet* packet, lpctstr heading)
{
#ifdef _DUMPSUPPORT
	if (client->GetAccount() != NULL && strnicmp(client->GetAccount()->GetName(), (lpctstr) g_Cfg.m_sDumpAccPackets, strlen( client->GetAccount()->GetName())))
		return;
#else
	if (!(g_Cfg.m_wDebugFlags & DEBUGF_PACKETS))
		return;
#endif

	TemporaryString dump;
	packet->dump(dump);

#ifdef _DEBUG
	// write to console
	g_Log.EventDebug("%x:%s %s\n", client->GetSocketID(), heading, (lpctstr)dump);
#endif

	// build file name
	tchar fname[64];
	strcpy(fname, "packets_");
	if (client->GetAccount())
		strcat(fname, client->GetAccount()->GetName());
	else
	{
		strcat(fname, "(");
		strcat(fname, client->GetPeerStr());
		strcat(fname, ")");
	}

	strcat(fname, ".log");

	CSString sFullFileName = CSFile::GetMergedFileName(g_Log.GetLogDir(), fname);

	// write to file
	CSFileText out;
	if (out.Open(sFullFileName, OF_READWRITE|OF_TEXT))
	{
		out.Printf("%s %s\n\n", heading, (lpctstr)dump);
		out.Close();
	}
}
#endif	//defined(_PACKETDUMP) || defined(_DUMPSUPPORT)


/***************************************************************************
 *
 *
 *	class NetState				Network status (client info, etc)
 *
 *
 ***************************************************************************/

NetState::NetState(int id)
{
	m_id = id;
	m_client = NULL;
	m_needsFlush = false;
	m_useAsync = false;
	m_outgoing.currentTransaction = NULL;
	m_outgoing.pendingTransaction = NULL;
	m_incoming.buffer = NULL;
#ifdef _MTNETWORK
	m_incoming.rawBuffer = NULL;
#endif
	m_packetExceptions = 0;
	m_clientType = CLIENTTYPE_2D;
	m_clientVersion = 0;
	m_reportedVersion = 0;
	m_isInUse = false;
#ifdef _MTNETWORK
	m_parent = NULL;
#endif

	clear();
}

NetState::~NetState(void)
{
}

void NetState::clear(void)
{
	ADDTOCALLSTACK("NetState::clear");
	DEBUGNETWORK(("%x:Clearing client state.\n", id()));

	m_isReadClosed = true;
	m_isWriteClosed = true;
	m_needsFlush = false;

	if (m_client != NULL)
	{
		g_Serv.StatDec(SERV_STAT_CLIENTS);
		if ( m_client->GetConnectType() == CONNECT_LOGIN )
		{	// when a connection is refused (ie. wrong password), there's no account
			g_Log.Event(LOGM_NOCONTEXT|LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Client disconnected [Total:%" PRIuSIZE_T "]. IP='%s'.\n",
				m_id, g_Serv.StatGet(SERV_STAT_CLIENTS), m_peerAddress.GetAddrStr());
		}
		else
		{
			if (!m_client->GetChar())
			{
				g_Log.Event(LOGM_NOCONTEXT|LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Client disconnected [Total:%" PRIuSIZE_T "]. Account: '%s'. IP='%s'.\n",
					m_id, g_Serv.StatGet(SERV_STAT_CLIENTS), m_client->GetName(), m_peerAddress.GetAddrStr());
			}
			else
			{
				g_Log.Event(LOGM_NOCONTEXT|LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Client disconnected [Total:%" PRIuSIZE_T "]. Account: '%s'. Char: '%s'. IP='%s'.\n",
					m_id, g_Serv.StatGet(SERV_STAT_CLIENTS), m_client->GetName(), m_client->GetChar()->GetName(), m_peerAddress.GetAddrStr());
			}
			
		}

#if !defined(_WIN32) || defined(_LIBEV)
		if (m_socket.IsOpen() && g_Cfg.m_fUseAsyncNetwork != 0)
			g_NetworkEvent.unregisterClient(this);
#endif

		//	record the client reference to the garbage collection to be deleted on it's time
		g_World.m_ObjDelete.InsertHead(m_client);
	}

#ifdef _WIN32
	if (m_socket.IsOpen() && isAsyncMode())
		m_socket.ClearAsync();
#endif

	m_socket.Close();
	m_client = NULL;

	// empty queues
	clearQueues();

	// clean junk queue entries
	for (int i = 0; i < PacketSend::PRI_QTY; i++)
		m_outgoing.queue[i].clean();
	m_outgoing.asyncQueue.clean();
#ifdef _MTNETWORK
	m_incoming.rawPackets.clean();
#endif

	if (m_outgoing.currentTransaction != NULL)
	{
		delete m_outgoing.currentTransaction;
		m_outgoing.currentTransaction = NULL;
	}

	if (m_outgoing.pendingTransaction != NULL)
	{
		delete m_outgoing.pendingTransaction;
		m_outgoing.pendingTransaction = NULL;
	}

	if (m_incoming.buffer != NULL)
	{
		delete m_incoming.buffer;
		m_incoming.buffer = NULL;
	}

#ifdef _MTNETWORK
	if (m_incoming.rawBuffer != NULL)
	{
		delete m_incoming.rawBuffer;
		m_incoming.rawBuffer = NULL;
	}
#endif

	m_sequence = 0;
	m_seeded = false;
	m_newseed = false;
	m_seed = 0;
	m_clientVersion = m_reportedVersion = 0;
	m_clientType = CLIENTTYPE_2D;
	m_isSendingAsync = false;
	m_packetExceptions = 0;
	setAsyncMode(false);
	m_isInUse = false;
}

void NetState::clearQueues(void)
{
	ADDTOCALLSTACK("NetState::clearQueues");

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

#ifdef _MTNETWORK
	// clear received queue
	while (m_incoming.rawPackets.empty() == false)
	{
		delete m_incoming.rawPackets.front();
		m_incoming.rawPackets.pop();
	}
#endif
}

void NetState::init(SOCKET socket, CSocketAddress addr)
{
	ADDTOCALLSTACK("NetState::init");

	clear();

	DEBUGNETWORK(("%x:Initialising client\n", id()));
	m_peerAddress = addr;
	m_socket.SetSocket(socket);
	m_socket.SetNonBlocking();

	// disable NAGLE algorythm for data compression/coalescing.
	// Send as fast as we can. we handle packing ourselves.
	char nbool = true;
	m_socket.SetSockOpt(TCP_NODELAY, &nbool, sizeof(char), IPPROTO_TCP);

	g_Serv.StatInc(SERV_STAT_CLIENTS);
	CClient* client = new CClient(this);
	m_client = client;

#if !defined(_WIN32) || defined(_LIBEV)
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

	DEBUGNETWORK(("%x:Determining async mode\n", id()));
	detectAsyncMode();
}

bool NetState::isInUse(const CClient* client) const volatile
{
	if (m_isInUse == false)
		return false;

	return client == NULL || m_client == client;
}

void NetState::markReadClosed(void) volatile
{
#ifdef _MTNETWORK
	ADDTOCALLSTACK("NetState::markReadClosed");
#endif

	DEBUGNETWORK(("%x:Client being closed by read-thread\n", m_id));
	m_isReadClosed = true;
#ifdef _MTNETWORK
	if (m_parent != NULL && m_parent->getPriority() == IThread::Disabled)
		m_parent->awaken();
#endif
}

void NetState::markWriteClosed(void) volatile
{
	DEBUGNETWORK(("%x:Client being closed by write-thread\n", m_id));
	m_isWriteClosed = true;
}

void NetState::markFlush(bool needsFlush) volatile
{
	m_needsFlush = needsFlush;
}

void NetState::detectAsyncMode(void)
{
	ADDTOCALLSTACK("NetState::detectAsyncMode");
	bool wasAsync = isAsyncMode();

	// is async mode enabled?
	if ( !g_Cfg.m_fUseAsyncNetwork || !isInUse() )
		setAsyncMode(false);

	// if the version mod flag is not set, always use async mode
	else if ( g_Cfg.m_fUseAsyncNetwork == 1 )
		setAsyncMode(true);

	// http clients do not want to be using async networking unless they have keep-alive set
	else if (getClient() != NULL && getClient()->GetConnectType() == CONNECT_HTTP)
		setAsyncMode(false);

	// only use async with clients newer than 4.0.0
	// - normally the client version is unknown for the first 1 or 2 packets, so all clients will begin
	//   without async networking (but should switch over as soon as it has been determined)
	// - a minor issue with this is that for clients without encryption we cannot determine their version
	//   until after they have fully logged into the game server and sent a client version packet.
	else if (isClientVersion(MINCLIVER_AUTOASYNC) || isClientKR() || isClientEnhanced())
		setAsyncMode(true);
	else
		setAsyncMode(false);

	if (wasAsync != isAsyncMode())
		DEBUGNETWORK(("%x:Switching async mode from %s to %s.\n", id(), wasAsync? "1":"0", isAsyncMode()? "1":"0"));
}

bool NetState::hasPendingData(void) const
{
	ADDTOCALLSTACK("NetState::hasPendingData");
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
	if (m_outgoing.currentTransaction != NULL)
		return true;

	return false;
}

bool NetState::canReceive(PacketSend* packet) const
{
	if (isInUse() == false || m_socket.IsOpen() == false || packet == NULL)
		return false;

	if (isClosing() && packet->getPriority() < NETWORK_DISCONNECTPRI)
		return false;

	if (packet->getTarget()->m_client == NULL)
		return false;

	return true;
}

void NetState::beginTransaction(int priority)
{
	ADDTOCALLSTACK("NetState::beginTransaction");
	if (m_outgoing.pendingTransaction != NULL)
	{
		DEBUGNETWORK(("%x:New network transaction started whilst a previous is still open.\n", id()));
	}

	// ensure previous transaction is committed
	endTransaction();

	//DEBUGNETWORK(("%x:Starting a new packet transaction.\n", id()));

	m_outgoing.pendingTransaction = new ExtendedPacketTransaction(this, g_Cfg.m_fUsePacketPriorities? priority : (int)(PacketSend::PRI_NORMAL));
}

void NetState::endTransaction(void)
{
	ADDTOCALLSTACK("NetState::endTransaction");
	if (m_outgoing.pendingTransaction == NULL)
		return;

	//DEBUGNETWORK(("%x:Scheduling packet transaction to be sent.\n", id()));

#ifndef _MTNETWORK
	g_NetworkOut.scheduleOnce(m_outgoing.pendingTransaction);
#else
	m_parent->queuePacketTransaction(m_outgoing.pendingTransaction);
#endif
	m_outgoing.pendingTransaction = NULL;
}


/***************************************************************************
 *
 *
 *	struct HistoryIP			Simple interface for IP history maintainence
 *
 *
 ***************************************************************************/
void HistoryIP::update(void)
{
	// reset ttl
	m_ttl = NETHISTORY_TTL;
}

bool HistoryIP::checkPing(void)
{
	// ip is pinging, check if blocked
	update();

	return ( m_blocked || ( m_pings++ >= NETHISTORY_MAXPINGS ));
}

void HistoryIP::setBlocked(bool isBlocked, int timeout)
{
	// block ip
	ADDTOCALLSTACK("HistoryIP:setBlocked");
	if (isBlocked == true)
	{
		CScriptTriggerArgs args(m_ip.GetAddrStr());
		args.m_iN1 = timeout;
		g_Serv.r_Call("f_onserver_blockip", &g_Serv, &args);
		timeout = (int)(args.m_iN1);
	}

	m_blocked = isBlocked;

	if (isBlocked && timeout >= 0)
		m_blockExpire = CServerTime::GetCurrentTime() + (timeout * TICK_PER_SEC);
	else
		m_blockExpire.Init();
}

/***************************************************************************
 *
 *
 *	class IPHistoryManager		Holds IP records (bans, pings, etc)
 *
 *
 ***************************************************************************/
IPHistoryManager::IPHistoryManager(void)
{
	m_lastDecayTime.Init();
}

IPHistoryManager::~IPHistoryManager(void)
{
	m_ips.clear();
}

void IPHistoryManager::tick(void)
{
	// periodic events
	ADDTOCALLSTACK("IPHistoryManager::tick");

	// check if ttl should decay (only do this once every second)
	bool decayTTL = ( !m_lastDecayTime.IsTimeValid() || (-g_World.GetTimeDiff(m_lastDecayTime)) >= TICK_PER_SEC );
	if (decayTTL)
		m_lastDecayTime = CServerTime::GetCurrentTime();

	for (IPHistoryList::iterator it = m_ips.begin(), end = m_ips.end(); it != end; ++it)
	{
		if (it->m_blocked)
		{
			// blocked ips don't decay, but check if the ban has expired
			if (it->m_blockExpire.IsTimeValid() && (CServerTime::GetCurrentTime() > it->m_blockExpire))
				it->setBlocked(false);
		}
		else if (decayTTL)
		{
			if (it->m_connected == 0 && it->m_connecting == 0)
			{
				// start to forget about clients who aren't connected
				if (it->m_ttl >= 0)
					--it->m_ttl;
			}

			 // wait a 5th of TTL between each ping decay, but do not wait less than 30 seconds
			if (it->m_pings > 0 && --it->m_pingDecay < 0)
			{
				--it->m_pings;
				it->m_pingDecay = NETHISTORY_PINGDECAY;
			}
		}
	}

	// clear old ip history
	for (IPHistoryList::iterator it = m_ips.begin(), end = m_ips.end(); it != end; ++it)
	{
		if (it->m_ttl >= 0)
			continue;

		m_ips.erase(it);
		break;
	}
}

HistoryIP& IPHistoryManager::getHistoryForIP(const CSocketAddressIP& ip)
{
	// get history for an ip
	ADDTOCALLSTACK("IPHistoryManager::getHistoryForIP");
	// commented to test
	//if (&ip)	//Temporal code until I find out the cause of an error produced here crashing the server.
	//{
		// find existing entry
		for (IPHistoryList::iterator it = m_ips.begin(), end = m_ips.end(); it != end; ++it)
		{
			if (it->m_ip == ip)
				return *it;
		}
	//}
	//else
	//{
	//	g_Log.EventDebug("No IP on getHistoryForIP, stack trace:\n" );
	//	ASSERT(&ip);
	//}

	// create a new entry
	HistoryIP hist;
	memset(&hist, 0, sizeof(hist));
	hist.m_ip = ip;
	hist.m_pingDecay = NETHISTORY_PINGDECAY;
	hist.update();

	m_ips.push_back(hist);
	return getHistoryForIP(ip);
}

HistoryIP& IPHistoryManager::getHistoryForIP(const char* ip)
{
	// get history for an ip
	ADDTOCALLSTACK("IPHistoryManager::getHistoryForIP");

	CSocketAddressIP me(ip);
	return getHistoryForIP(me);
}


/***************************************************************************
 *
 *
 *	class PacketManager             Holds lists of packet handlers
 *
 *
 ***************************************************************************/
PacketManager::PacketManager(void)
{
	memset(m_handlers, 0, sizeof(m_handlers));
	memset(m_extended, 0, sizeof(m_extended));
	memset(m_encoded, 0, sizeof(m_encoded));
}

PacketManager::~PacketManager(void)
{
	// delete standard packet handlers
	for (size_t i = 0; i < CountOf(m_handlers); ++i)
		unregisterPacket((uint)i);

	// delete extended packet handlers
	for (size_t i = 0; i < CountOf(m_extended); ++i)
		unregisterExtended((uint)i);

	// delete encoded packet handlers
	for (size_t i = 0; i < CountOf(m_encoded); ++i)
		unregisterEncoded((uint)i);
}

void PacketManager::registerStandardPackets(void)
{
	ADDTOCALLSTACK("PacketManager::registerStandardPackets");

	// standard packets
	registerPacket(XCMD_Create, new PacketCreate());							// create character
	registerPacket(XCMD_WalkRequest, new PacketMovementReq());					// movement request
	registerPacket(XCMD_Talk, new PacketSpeakReq());							// speak
	registerPacket(XCMD_Attack, new PacketAttackReq());							// begin attack
	registerPacket(XCMD_DClick, new PacketDoubleClick());						// double click object
	registerPacket(XCMD_ItemPickupReq, new PacketItemPickupReq());				// pick up item
	registerPacket(XCMD_ItemDropReq, new PacketItemDropReq());					// drop item
	registerPacket(XCMD_Click, new PacketSingleClick());						// single click object
	registerPacket(XCMD_ExtCmd, new PacketTextCommand());						// extended text command
	registerPacket(XCMD_ItemEquipReq, new PacketItemEquipReq());				// equip item
	registerPacket(XCMD_WalkAck, new PacketResynchronize());					//
	registerPacket(XCMD_DeathMenu, new PacketDeathStatus());					//
	registerPacket(XCMD_CharStatReq, new PacketCharStatusReq());				// status request
	registerPacket(XCMD_Skill, new PacketSkillLockChange());					// change skill lock
	registerPacket(XCMD_VendorBuy, new PacketVendorBuyReq());					// buy items from vendor
	registerPacket(XCMD_StaticUpdate, new PacketStaticUpdate());				// UltimaLive Packet
	registerPacket(XCMD_MapEdit, new PacketMapEdit());							// edit map pins
	registerPacket(XCMD_CharPlay, new PacketCharPlay());						// select character
	registerPacket(XCMD_BookPage, new PacketBookPageEdit());					// edit book content
	registerPacket(XCMD_Options, new PacketUnknown());							// unused options packet
	registerPacket(XCMD_Target, new PacketTarget());							// target an object
	registerPacket(XCMD_SecureTrade, new PacketSecureTradeReq());				// secure trade action
	registerPacket(XCMD_BBoard, new PacketBulletinBoardReq());					// bulletin board action
	registerPacket(XCMD_War, new PacketWarModeReq());							// toggle war mode
	registerPacket(XCMD_Ping, new PacketPingReq());								// ping request
	registerPacket(XCMD_CharName, new PacketCharRename());						// change character name
	registerPacket(XCMD_MenuChoice, new PacketMenuChoice());					// select menu item
	registerPacket(XCMD_ServersReq, new PacketServersReq());					// request server list
	registerPacket(XCMD_CharDelete, new PacketCharDelete());					// delete character
	registerPacket(XCMD_CreateNew, new PacketCreateNew());						// create character (KR/SA)
	registerPacket(XCMD_CharListReq, new PacketCharListReq());					// request character list
	registerPacket(XCMD_BookOpen, new PacketBookHeaderEdit());					// edit book
	registerPacket(XCMD_DyeVat, new PacketDyeObject());							// colour selection dialog
	registerPacket(XCMD_AllNames3D, new PacketAllNamesReq());					// all names macro (ctrl+shift)
	registerPacket(XCMD_Prompt, new PacketPromptResponse());					// prompt response
	registerPacket(XCMD_HelpPage, new PacketHelpPageReq());						// GM help page request
	registerPacket(XCMD_VendorSell, new PacketVendorSellReq());					// sell items to vendor
	registerPacket(XCMD_ServerSelect, new PacketServerSelect());				// select server
	registerPacket(XCMD_Spy, new PacketSystemInfo());							//
	registerPacket(XCMD_Scroll, new PacketUnknown(5));							// scroll closed
	registerPacket(XCMD_TipReq, new PacketTipReq());							// request tip of the day
	registerPacket(XCMD_GumpInpValRet, new PacketGumpValueInputResponse());		// text input dialog
	registerPacket(XCMD_TalkUNICODE, new PacketSpeakReqUNICODE());				// speech (unicode)
	registerPacket(XCMD_GumpDialogRet, new PacketGumpDialogRet());				// dialog response (button press)
	registerPacket(XCMD_ChatText, new PacketChatCommand());						// chat command
	registerPacket(XCMD_Chat, new PacketChatButton());							// chat button
	registerPacket(XCMD_ToolTipReq, new PacketToolTipReq());					// popup help request
	registerPacket(XCMD_CharProfile, new PacketProfileReq());					// profile read/write request
	registerPacket(XCMD_MailMsg, new PacketMailMessage());						//
	registerPacket(XCMD_ClientVersion, new PacketClientVersion());				// client version
	registerPacket(XCMD_ExtData, new PacketExtendedCommand());					//
	registerPacket(XCMD_PromptUNICODE, new PacketPromptResponseUnicode());		// prompt response (unicode)
	registerPacket(XCMD_ViewRange, new PacketViewRange());						//
	registerPacket(XCMD_ConfigFile, new PacketUnknown());						//
	registerPacket(XCMD_LogoutStatus, new PacketLogout());						//
	registerPacket(XCMD_AOSBookPage, new PacketBookHeaderEditNew());			// edit book
	registerPacket(XCMD_AOSTooltip, new PacketAOSTooltipReq());					// request tooltip data
	registerPacket(XCMD_ExtAosData, new PacketEncodedCommand());				//
	registerPacket(XCMD_Spy2, new PacketHardwareInfo());						// client hardware info
	registerPacket(XCMD_BugReport, new PacketBugReport());						// bug report
	registerPacket(XCMD_KRClientType, new PacketClientType());					// report client type (2d/kr/sa)
	registerPacket(XCMD_HighlightUIRemove, new PacketRemoveUIHighlight());		//
	registerPacket(XCMD_UseHotbar, new PacketUseHotbar());						//
	registerPacket(XCMD_MacroEquipItem, new PacketEquipItemMacro());			// equip item(s) macro (KR)
	registerPacket(XCMD_MacroUnEquipItem, new PacketUnEquipItemMacro());		// unequip item(s) macro (KR)
	registerPacket(XCMD_WalkRequestNew, new PacketMovementReqNew());			// new movement request (KR/SA)
	registerPacket(XCMD_TimeSyncRequest, new PacketTimeSyncRequest());			// time sync request (KR/SA)
	registerPacket(XCMD_CrashReport, new PacketCrashReport());					//
	registerPacket(XCMD_CreateHS, new PacketCreate70016());						// create character (HS)

	// extended packets (0xBF)
	registerExtended(EXTDATA_ScreenSize, new PacketScreenSize());				// client screen size
	registerExtended(EXTDATA_Party_Msg, new PacketPartyMessage());				// party command
	registerExtended(EXTDATA_Arrow_Click, new PacketArrowClick());				// click quest arrow
	registerExtended(EXTDATA_Wrestle_DisArm, new PacketWrestleDisarm());		// wrestling disarm macro
	registerExtended(EXTDATA_Wrestle_Stun, new PacketWrestleStun());			// wrestling stun macro
	registerExtended(EXTDATA_Lang, new PacketLanguage());						// client language
	registerExtended(EXTDATA_StatusClose, new PacketStatusClose());				// status window closed
	registerExtended(EXTDATA_Yawn, new PacketAnimationReq());					// play animation
	registerExtended(EXTDATA_Unk15, new PacketClientInfo());					// client information
	registerExtended(EXTDATA_OldAOSTooltipInfo, new PacketAosTooltipInfo());	//
	registerExtended(EXTDATA_Popup_Request, new PacketPopupReq());				// request popup menu
	registerExtended(EXTDATA_Popup_Select, new PacketPopupSelect());			// select popup option
	registerExtended(EXTDATA_Stats_Change, new PacketChangeStatLock());			// change stat lock
	registerExtended(EXTDATA_NewSpellSelect, new PacketSpellSelect());			//
	registerExtended(EXTDATA_HouseDesignDet, new PacketHouseDesignReq());		// house design request
	registerExtended(EXTDATA_AntiCheat, new PacketAntiCheat());					// anti-cheat / unknown
	registerExtended(EXTDATA_BandageMacro, new PacketBandageMacro());			//
	registerExtended(EXTDATA_GargoyleFly, new PacketGargoyleFly());				// gargoyle flying action
	registerExtended(EXTDATA_WheelBoatMove, new PacketWheelBoatMove());			// wheel boat movement

	// encoded packets (0xD7)
	registerEncoded(EXTAOS_HcBackup, new PacketHouseDesignBackup());			// house design - backup
	registerEncoded(EXTAOS_HcRestore, new PacketHouseDesignRestore());			// house design - restore
	registerEncoded(EXTAOS_HcCommit, new PacketHouseDesignCommit());			// house design - commit
	registerEncoded(EXTAOS_HcDestroyItem, new PacketHouseDesignDestroyItem());	// house design - remove item
	registerEncoded(EXTAOS_HcPlaceItem, new PacketHouseDesignPlaceItem());		// house design - place item
	registerEncoded(EXTAOS_HcExit, new PacketHouseDesignExit());				// house design - exit designer
	registerEncoded(EXTAOS_HcPlaceStair, new PacketHouseDesignPlaceStair());	// house design - place stairs
	registerEncoded(EXTAOS_HcSynch, new PacketHouseDesignSync());				// house design - synchronise
	registerEncoded(EXTAOS_HcClear, new PacketHouseDesignClear());				// house design - clear
	registerEncoded(EXTAOS_HcSwitch, new PacketHouseDesignSwitch());			// house design - change floor
	registerEncoded(EXTAOS_HcPlaceRoof, new PacketHouseDesignPlaceRoof());		// house design - place roof
	registerEncoded(EXTAOS_HcDestroyRoof, new PacketHouseDesignDestroyRoof());	// house design - remove roof
	registerEncoded(EXTAOS_SpecialMove, new PacketSpecialMove());				//
	registerEncoded(EXTAOS_HcRevert, new PacketHouseDesignRevert());			// house design - revert
	registerEncoded(EXTAOS_EquipLastWeapon, new PacketEquipLastWeapon());		//
	registerEncoded(EXTAOS_GuildButton, new PacketGuildButton());				// guild button press
	registerEncoded(EXTAOS_QuestButton, new PacketQuestButton());				// quest button press
}

void PacketManager::registerPacket(uint id, Packet* handler)
{
	// assign standard packet handler
	ADDTOCALLSTACK("PacketManager::registerPacket");
	ASSERT(id < CountOf(m_handlers));
	unregisterPacket(id);
	m_handlers[id] = handler;
}

void PacketManager::registerExtended(uint id, Packet* handler)
{
	// assign extended packet handler
	ADDTOCALLSTACK("PacketManager::registerExtended");
	ASSERT(id < CountOf(m_extended));
	unregisterExtended(id);
	m_extended[id] = handler;
}

void PacketManager::registerEncoded(uint id, Packet* handler)
{
	// assign encoded packet handler
	ADDTOCALLSTACK("PacketManager::registerEncoded");
	ASSERT(id < CountOf(m_encoded));
	unregisterEncoded(id);
	m_encoded[id] = handler;
}

void PacketManager::unregisterPacket(uint id)
{
	// delete standard packet handler
	ADDTOCALLSTACK("PacketManager::unregisterPacket");
	ASSERT(id < CountOf(m_handlers));
	if (m_handlers[id] == NULL)
		return;

	delete m_handlers[id];
	m_handlers[id] = NULL;
}

void PacketManager::unregisterExtended(uint id)
{
	// delete extended packet handler
	ADDTOCALLSTACK("PacketManager::unregisterExtended");
	ASSERT(id < CountOf(m_extended));
	if (m_extended[id] == NULL)
		return;

	delete m_extended[id];
	m_extended[id] = NULL;
}

void PacketManager::unregisterEncoded(uint id)
{
	// delete encoded packet handler
	ADDTOCALLSTACK("PacketManager::unregisterEncoded");
	ASSERT(id < CountOf(m_encoded));
	if (m_encoded[id] == NULL)
		return;

	delete m_encoded[id];
	m_encoded[id] = NULL;
}

Packet* PacketManager::getHandler(uint id) const
{
	// get standard packet handler
	if (id >= CountOf(m_handlers))
		return NULL;

	return m_handlers[id];
}

Packet* PacketManager::getExtendedHandler(uint id) const
{
	// get extended packet handler
	if (id >= CountOf(m_extended))
		return NULL;

	return m_extended[id];
}

Packet* PacketManager::getEncodedHandler(uint id) const
{
	// get encoded packet handler
	if (id >= CountOf(m_encoded))
		return NULL;

	return m_encoded[id];
}


/***************************************************************************
 *
 *
 *	class ClientIterator		Works as client iterator getting the clients
 *
 *
 ***************************************************************************/

#ifndef _MTNETWORK
ClientIterator::ClientIterator(const NetworkIn* network)
{
	m_network = (network == NULL? &g_NetworkIn : network);
	m_nextClient = static_cast <CClient*> (m_network->m_clients.GetHead());
}
#else
ClientIterator::ClientIterator(const NetworkManager* network)
{
	m_network = (network == NULL? &g_NetworkManager : network);
	m_nextClient = static_cast<CClient*> (m_network->m_clients.GetHead());
}
#endif

ClientIterator::~ClientIterator(void)
{
	m_network = NULL;
	m_nextClient = NULL;
}

CClient* ClientIterator::next(bool includeClosing)
{
	for (CClient* current = m_nextClient; current != NULL; current = current->GetNext())
	{
		// skip clients without a state, or whose state is invalid/closed
		if (current->GetNetState() == NULL || current->GetNetState()->isInUse(current) == false || current->GetNetState()->isClosed())
			continue;

		// skip clients whose connection is being closed
		if (includeClosing == false && current->GetNetState()->isClosing())
			continue;

		m_nextClient = current->GetNext();
		return current;
	}

	return NULL;
}

