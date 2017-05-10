
#include <deque>
#include "../common/CException.h"
#include "../game/clients/CClient.h"
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

#ifdef _WIN32
#	define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); closesocket(_x_); }
#else
#	define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); close(_x_); }
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

	CClient* client = m_client;
	if (client != NULL)
	{
		//m_client = NULL;	// commented to test
        CAccount *pAccount = client->GetAccount();
		g_Serv.StatDec(SERV_STAT_CLIENTS);
		if ( client->GetConnectType() == CONNECT_LOGIN )
		{	// if account name is retrieved when a connection is refused (ie. wrong password), sphere will crash (at least on MinGW)
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Client disconnected [Total:%" PRIuSIZE_T "]. IP='%s'.\n",
				m_id, g_Serv.StatGet(SERV_STAT_CLIENTS), m_peerAddress.GetAddrStr());
		}
		else
		{
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Client disconnected [Total:%" PRIuSIZE_T "]. Account: '%s'. IP='%s'.\n",
				m_id, g_Serv.StatGet(SERV_STAT_CLIENTS), pAccount ? pAccount->GetName() : "No Account", m_peerAddress.GetAddrStr());
		}

#if !defined(_WIN32) || defined(_LIBEV)
		if (m_socket.IsOpen() && g_Cfg.m_fUseAsyncNetwork != 0)
			g_NetworkEvent.unregisterClient(this);
#endif

		//	record the client reference to the garbage collection to be deleted on it's time
		g_World.m_ObjDelete.InsertHead(client);
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
	//bool wasAsync = isAsyncMode();

	// Disabled because of unstability.
	setAsyncMode(false);
	return;
	// is async mode enabled?
/*	if ( !g_Cfg.m_fUseAsyncNetwork || !isInUse() )
		setAsyncMode(false);

	// if the version mod flag is not set, always use async mode
	else if ( g_Cfg.m_fUseAsyncNetwork != 2 )
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
		DEBUGNETWORK(("%x:Switching async mode from %s to %s.\n", id(), wasAsync? "1":"0", isAsyncMode()? "1":"0"));*/
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

	for (IPHistoryList::iterator it = m_ips.begin(); it != m_ips.end(); ++it)
	{
		if (it->m_blocked)
		{
			// blocked ips don't decay, but check if the ban has expired
			if (it->m_blockExpire.IsTimeValid() && CServerTime::GetCurrentTime() > it->m_blockExpire)
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
	for (IPHistoryList::iterator it = m_ips.begin(); it != m_ips.end(); ++it)
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
	if (&ip)	//Temporal code until I find out the cause of an error produced here crashing the server.
	{
		// find existing entry
		for (IPHistoryList::iterator it = m_ips.begin(); it != m_ips.end(); ++it)
		{
			if (( *it ).m_ip == ip)
				return *it;
		}
	}
	else
	{
		g_Log.EventDebug("No IP on getHistoryForIP, stack trace:\n" );
		ASSERT(&ip);
	}

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
	registerPacket(XCMD_CreateHS, new PacketCreateHS());						// create character (HS)

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


#ifndef _MTNETWORK
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

							client->m_reportedVersion = CCrypt::GetVerFromNumber(pEvent->NewSeed.m_Version_Maj, pEvent->NewSeed.m_Version_Min, pEvent->NewSeed.m_Version_Rev, pEvent->NewSeed.m_Version_Pat);
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

#else

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
	char * name = new char[25];
	sprintf(name, "NetworkThread #%" PRIuSIZE_T, id);
	return name;
}

#ifdef _WIN32

/***************************************************************************
 *
 *
 *	void SendCompleted			Winsock event handler for when async operation completes
 *
 *
 ***************************************************************************/
void CALLBACK SendCompleted(dword dwError, dword cbTransferred, LPWSAOVERLAPPED lpOverlapped, uint64 iFlags)
{
	UNREFERENCED_PARAMETER(iFlags);
	ADDTOCALLSTACK("SendCompleted");

	NetState* state = reinterpret_cast<NetState*>(lpOverlapped->hEvent);
	if (state == NULL)
	{
		DEBUGNETWORK(("Async i/o operation completed without client context.\n"));
		return;
	}

	NetworkThread* thread = state->getParentThread();
	if (thread == NULL)
	{
		DEBUGNETWORK(("%x:Async i/o operation completed.\n", state->id()));
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
	m_states = NULL;
	m_stateCount = 0;
	m_lastGivenSlot = INTPTR_MAX;
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

	NetworkThread* bestThread = NULL;
	size_t bestThreadSize = INTPTR_MAX;
	DEBUGNETWORK(("Searching for a suitable thread to handle a new client..\n"));

	// search for quietest thread
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
	{
		if ((*it)->getClientCount() < bestThreadSize)
		{
			bestThread = *it;
			bestThreadSize = bestThread->getClientCount();
		}
	}

	ASSERT(bestThread != NULL);
	DEBUGNETWORK(("Selected thread #%" PRIuSIZE_T ".\n", bestThread->id()));
	return bestThread;
}

void NetworkManager::assignNetworkState(NetState* state)
{
	// assign a state to a thread
	ADDTOCALLSTACK("NetworkManager::assignNetworkState");

	NetworkThread* thread = selectBestThread();
	ASSERT(thread != NULL);
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

	count = select(count+1, &fds, NULL, NULL, &Timeout);
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
	EXC_SET("accept");
	SOCKET h = g_Serv.m_SocketMain.Accept(client_addr);
	if (h == INVALID_SOCKET)
		return;

	// check ip history
	EXC_SET("ip history");

	DEBUGNETWORK(("Retrieving IP history for '%s'.\n", client_addr.GetAddrStr()));
	HistoryIP& ip = m_ips.getHistoryForIP(client_addr);
	int maxIp = g_Cfg.m_iConnectingMaxIP;
	int climaxIp = g_Cfg.m_iClientsMaxIP;

	DEBUGNETWORK(("Incoming connection from '%s' [blocked=%d, ttl=%d, pings=%d, connecting=%d, connected=%d]\n",
		ip.m_ip.GetAddrStr(), ip.m_blocked, ip.m_ttl, ip.m_pings, ip.m_connecting, ip.m_connected));

	// check if ip is allowed to connect
	if ( ip.checkPing() ||									// check for ip ban
			( maxIp > 0 && ip.m_connecting > maxIp ) ||		// check for too many connecting
			( climaxIp > 0 && ip.m_connected > climaxIp ) )	// check for too many connected
	{
		EXC_SET("rejected");
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
	EXC_SET("detecting slot");
	NetState* state = findFreeSlot();
	if (state == NULL)
	{
		// not enough empty slots
		EXC_SET("no slot available");
		DEBUGNETWORK(("Unable to allocate new slot for client, too many clients already connected.\n"));
		CLOSESOCKET(h);

		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CLIENTMAX reached)\n", static_cast<lpctstr>(client_addr.GetAddrStr()));
		return;
	}

	DEBUGNETWORK(("%x:Allocated slot for client (%u).\n", state->id(), (uint)(h)));

	// assign slot
	EXC_SET("assigning slot");
	state->init(h, client_addr);

	DEBUGNETWORK(("%x:State initialised, registering client instance.\n", state->id()));

	EXC_SET("recording client");
	if (state->getClient() != NULL)
		m_clients.InsertHead(state->getClient());

	EXC_SET("assigning thread");
	DEBUGNETWORK(("%x:Selecting a thread to assign to.\n", state->id()));
	assignNetworkState(state);

	DEBUGNETWORK(("%x:Client successfully initialised.\n", state->id()));

	EXC_CATCH;
}

NetState* NetworkManager::findFreeSlot(size_t start)
{
	// find slot for new client
	ADDTOCALLSTACK("NetworkManager::findFreeSlot");

	// start searching from the last given slot to try and give incremental
	// ids to clients
	if (start == INTPTR_MAX)
		start = m_lastGivenSlot + 1;

	// find unused slot
	for (size_t i = start; i < m_stateCount; ++i)
	{
		if (m_states[i]->isInUse())
			continue;

		m_lastGivenSlot = i;
		return m_states[i];
	}

	if (start == 0)
		return NULL;

	// since we started somewhere in the middle, go back to the start
	return findFreeSlot(0);
}

void NetworkManager::start(void)
{
	DEBUGNETWORK(("Registering packets...\n"));
	m_packets.registerStandardPackets();

	// create network states
	ASSERT(m_states == NULL);
	ASSERT(m_stateCount == 0);
	m_states = new NetState*[g_Cfg.m_iClientsMax];
	for (size_t l = 0; l < g_Cfg.m_iClientsMax; l++)
		m_states[l] = new NetState((int)(l));
	m_stateCount = g_Cfg.m_iClientsMax;

	DEBUGNETWORK(("Created %" PRIuSIZE_T " network slots (system limit of %d clients)\n", m_stateCount, FD_SETSIZE));

	// create network threads
	createNetworkThreads(g_Cfg.m_iNetworkThreads);

	m_isThreaded = g_Cfg.m_iNetworkThreads > 0;
	if (isThreaded())
	{
		// start network threads
		for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
			(*it)->start();

		DEBUGNETWORK(("Started %" PRIuSIZE_T " network threads.\n", m_threads.size()));
	}
	else
	{
		// initialise network threads
		for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
			(*it)->onStart();
	}
}

void NetworkManager::stop(void)
{
	// terminate child threads
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
		(*it)->waitForClose();
}

void NetworkManager::tick(void)
{
	ADDTOCALLSTACK("NetworkManager::tick");

	EXC_TRY("Tick");
	for (size_t i = 0; i < m_stateCount; ++i)
	{
		NetState* state = m_states[i];
		if (state->isInUse() == false)
			continue;

		// clean packet queue entries
		EXC_SET("cleaning queues");
		for (int priority = 0; priority < PacketSend::PRI_QTY; ++priority)
			state->m_outgoing.queue[priority].clean();
		state->m_outgoing.asyncQueue.clean();

		EXC_SET("check closing");
		if (state->isClosing() == false)
		{
#ifdef _MTNETWORK
			// if data is queued whilst the thread is in the middle of processing then the signal
			// sent from NetworkOutput::QueuePacketTransaction can be ignored
			// the safest solution to this is to send additional signals from here
			NetworkThread* thread = state->getParentThread();
			if (thread != NULL && state->hasPendingData() && thread->getPriority() == IThread::Disabled)
				thread->awaken();
#endif
			continue;
		}

		// the state is closing, see if it can be cleared
		if (state->isClosed() == false)
		{
			EXC_SET("check pending data");
			if (state->hasPendingData())
			{
				// data is waiting to be sent, so flag that a flush is needed
				EXC_SET("flush data");
				if (state->needsFlush() == false)
				{
					DEBUGNETWORK(("%x:Flushing data for client.\n", state->id()));
					NetworkThread * thread = state->getParentThread();
					if (thread != NULL)
						thread->flush(state);
				}
				continue;
			}

			// state is finished with as far as we're concerned
			EXC_SET("mark closed");
			state->markReadClosed();
		}

		EXC_SET("check closed");
		if (state->isClosed() && state->isSendingAsync() == false)
		{
			EXC_SET("clear socket");
			DEBUGNETWORK(("%x:Client is being cleared since marked to close.\n", state->id()));
			state->clear();
		}
	}

	// tick ip history
	m_ips.tick();

	// tick child threads, if single-threaded mode
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
	{
		NetworkThread* thread = *it;
		if (thread->isActive() == false)
			thread->tick();
	}

	EXC_CATCH;
	//EXC_DEBUG_START;
	//EXC_DEBUG_END;
}

void NetworkManager::processAllInput(void)
{
	// process network input
	ADDTOCALLSTACK("NetworkManager::processAllInput");
	if (checkNewConnection())
		acceptNewConnection();

	// force each thread to process input (NOT THREADSAFE)
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
		(*it)->processInput();
}

void NetworkManager::processAllOutput(void)
{
	// process network output
	ADDTOCALLSTACK("NetworkManager::processAllOutput");

	if (isOutputThreaded() == false)
	{
		// force each thread to process output (NOT THREADSAFE)
		for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
			(*it)->processOutput();
	}
}

void NetworkManager::flushAllClients(void)
{
	// flush data for every client
	ADDTOCALLSTACK("NetworkManager::flushAllClients");

	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
		(*it)->flushAllClients();
}

size_t NetworkManager::flush(NetState * state)
{
	// flush data for a single client
	ADDTOCALLSTACK("NetworkManager::flush");
	ASSERT(state != NULL);
	NetworkThread * thread = state->getParentThread();
	if (thread != NULL)
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

		ASSERT(state != NULL);
		state->setParentThread(this);
		m_states.push_back(state);
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
			state->setParentThread(NULL);
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
NetworkInput::NetworkInput(void) : m_thread(NULL)
{
	m_receiveBuffer = new byte[NETWORK_BUFFERSIZE];
	m_decryptBuffer = new byte[NETWORK_BUFFERSIZE];
}

NetworkInput::~NetworkInput()
{
	if (m_receiveBuffer != NULL)
		delete[] m_receiveBuffer;
	if (m_decryptBuffer != NULL)
		delete[] m_decryptBuffer;
}

bool NetworkInput::processInput()
{
	ADDTOCALLSTACK("NetworkInput::processInput");
	ASSERT(m_thread != NULL);

#ifndef MTNETWORK_INPUT
	receiveData();
	processData();
#else
	// when called from within the thread's context, we just receive data
	if (!m_thread->isActive() || m_thread->isCurrentThread())
		receiveData();

	// when called from outside the thread's context, we process data
	if (!m_thread->isActive() || !m_thread->isCurrentThread())
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
	ASSERT(m_thread != NULL);
#ifdef MTNETWORK_INPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif
	EXC_TRY("ReceiveData");

	// check for incoming data
	EXC_SET("select");
	fd_set fds;
	if (checkForData(fds) == false)
		return;

	EXC_SET("messages");
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		EXC_SET("check socket");
		if (state->isReadClosed())
			continue;

		EXC_SET("start network profile");
		ProfileTask networkTask(PROFILE_NETWORK_RX);
		if ( ! FD_ISSET(state->m_socket.GetSocket(), &fds))
		{
			state->m_incoming.rawPackets.clean();
			continue;
		}

		// receive data
		EXC_SET("messages - receive");
		int received = state->m_socket.Receive(m_receiveBuffer, NETWORK_BUFFERSIZE, 0);
		if (received <= 0 || received > NETWORK_BUFFERSIZE)
		{
			state->markReadClosed();
			EXC_SET("next state");
			continue;
		}

		EXC_SET("start client profile");
		CurrentProfileData.Count(PROFILE_DATA_RX, received);

		EXC_SET("messages - parse");

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
			size_t length = received;

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
	ASSERT(m_thread != NULL);
#ifdef MTNETWORK_INPUT
	ASSERT(!m_thread->isActive() || !m_thread->isCurrentThread());
#endif
	EXC_TRY("ProcessData");

	// check which states have data
	EXC_SET("messages");
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		EXC_SET("check socket");
		if (state->isReadClosed())
			continue;

		EXC_SET("start network profile");
		ProfileTask networkTask(PROFILE_NETWORK_RX);

		const CClient* client = state->getClient();
		ASSERT(client != NULL);

		EXC_SET("check message");
		if (state->m_incoming.rawPackets.empty())
		{
			if ((client->GetConnectType() != CONNECT_TELNET) && (client->GetConnectType() != CONNECT_AXIS))
			{
				// check for timeout
				EXC_SET("check frozen");
				int64 iLastEventDiff = -g_World.GetTimeDiff( client->m_timeLastEvent );
				if ( g_Cfg.m_iDeadSocketTime > 0 && iLastEventDiff > g_Cfg.m_iDeadSocketTime )
				{
					g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Frozen client disconnected (DeadSocketTime reached).\n", state->id());
					state->m_client->addLoginErr( PacketLoginError::Other );		//state->markReadClosed();
				}
			}

			if (state->m_incoming.rawBuffer == NULL)
			{
				EXC_SET("next state");
				continue;
			}
		}

		EXC_SET("messages - process");
		// we've already received some raw data, we just need to add it to any existing data we have
		while (state->m_incoming.rawPackets.empty() == false)
		{
			Packet* packet = state->m_incoming.rawPackets.front();
			state->m_incoming.rawPackets.pop();
			ASSERT(packet != NULL);

			EXC_SET("packet - queue data");
			if (state->m_incoming.rawBuffer == NULL)
			{
				// create new buffer
				state->m_incoming.rawBuffer = new Packet(packet->getData(), packet->getLength());
			}
			else
			{
				// append to buffer
				size_t pos = state->m_incoming.rawBuffer->getPosition();
				state->m_incoming.rawBuffer->seek(state->m_incoming.rawBuffer->getLength());
				state->m_incoming.rawBuffer->writeData(packet->getData(), packet->getLength());
				state->m_incoming.rawBuffer->seek(pos);
			}

			delete packet;
		}

		if (g_Serv.IsLoading() == false)
		{
			EXC_SET("start client profile");
			ProfileTask clientTask(PROFILE_CLIENTS);

			EXC_SET("packets - process");
			Packet* buffer = state->m_incoming.rawBuffer;
			if (buffer != NULL)
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
					EXC_SET("packets - clear buffer");
					delete buffer;
					state->m_incoming.rawBuffer = NULL;
				}
			}
		}
		EXC_SET("next state");
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
		EXC_SET("check socket");
		if (state->isReadClosed())
			continue;

		EXC_SET("check closing");
		if (state->isClosing() || state->m_socket.IsOpen() == false)
			continue;

		AddSocketToSet(fds, state->m_socket.GetSocket(), count);
	}

	EXC_SET("prepare timeout");
	timeval timeout; // time to wait for data.
	timeout.tv_sec = 0;
	timeout.tv_usec = 100; // micro seconds = 1/1000000

	EXC_SET("select");
	return select(count + 1, &fds, NULL, NULL, &timeout) > 0;

	EXC_CATCH;
	return false;
}

bool NetworkInput::processData(NetState* state, Packet* buffer)
{
	ADDTOCALLSTACK("NetworkInput::processData");
	ASSERT(state != NULL);
	ASSERT(buffer != NULL);
	CClient* client = state->getClient();
	ASSERT(client != NULL);

	if (client->GetConnectType() == CONNECT_UNK)
		return processUnknownClientData(state, buffer);

	client->m_timeLastEvent = CServerTime::GetCurrentTime();

	if ( client->m_Crypt.IsInit() == false )
		return processOtherClientData(state, buffer);

	return processGameClientData(state, buffer);
}

bool NetworkInput::processGameClientData(NetState* state, Packet* buffer)
{
	ADDTOCALLSTACK("NetworkInput::processGameClientData");
	EXC_TRY("ProcessGameData");
	ASSERT(state != NULL);
	ASSERT(buffer != NULL);
	CClient* client = state->getClient();
	ASSERT(client != NULL);

	EXC_SET("decrypt message");
	client->m_Crypt.Decrypt(m_decryptBuffer, buffer->getRemainingData(), buffer->getRemainingLength());

	if (state->m_incoming.buffer == NULL)
	{
		// create new buffer
		state->m_incoming.buffer = new Packet(m_decryptBuffer, buffer->getRemainingLength());
	}
	else
	{
		// append to buffer
		size_t pos = state->m_incoming.buffer->getPosition();
		state->m_incoming.buffer->seek(state->m_incoming.buffer->getLength());
		state->m_incoming.buffer->writeData(m_decryptBuffer, buffer->getRemainingLength());
		state->m_incoming.buffer->seek(pos);
	}

	Packet* packet = state->m_incoming.buffer;
	size_t remainingLength = packet->getRemainingLength();

	EXC_SET("record message");
	xRecordPacket(client, packet, "client->server");

	// process the message
	EXC_TRYSUB("ProcessMessage");

	while (remainingLength > 0 && state->isClosing() == false)
	{
		ASSERT(remainingLength == packet->getRemainingLength());
		byte packetId = packet->getRemainingData()[0];
		Packet* handler = m_thread->m_manager.getPacketManager().getHandler(packetId);

		if (handler != NULL)
		{
			size_t packetLength = handler->checkLength(state, packet);
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
			packet->skip((int)(packetLength));

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
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%x:Unknown game packet (0x%x) received.\n", state->id(), packetId);
			packet->skip((int)(remainingLength));
			remainingLength = 0;
		}
	}

	EXC_CATCHSUB("Message");
	EXC_DEBUGSUB_START;
	TemporaryString dump;
	packet->dump(dump);

	g_Log.EventDebug("%x:Parsing %s", state->id(), static_cast<lpctstr>(dump));

	state->m_packetExceptions++;
	if (state->m_packetExceptions > 10)
	{
		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%x:Disconnecting client from account '%s' since it is causing exceptions problems\n", state->id(), client != NULL && client->GetAccount() ? client->GetAccount()->GetName() : "");
		if (client != NULL)
			client->addKick(&g_Serv, false);
		else
			state->markReadClosed();
	}

	EXC_DEBUGSUB_END;

	// delete the buffer once it has been exhausted
	if (remainingLength <= 0)
	{
		state->m_incoming.buffer = NULL;
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
	ASSERT(state != NULL);
	ASSERT(buffer != NULL);
	CClient* client = state->getClient();
	ASSERT(client != NULL);

	switch (client->GetConnectType())
	{
		case CONNECT_CRYPT:
			if (buffer->getRemainingLength() < 5)
			{
				// not enough data to be a real client
				EXC_SET("ping #3");
				client->SetConnectType(CONNECT_UNK);
				if (client->OnRxPing(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
					return false;
				break;
			}

			// first real data from client which we can use to log in
			EXC_SET("encryption setup");
			ASSERT(buffer->getRemainingLength() <= sizeof(CEvent));

			CEvent evt;
			memcpy(&evt, buffer->getRemainingData(), buffer->getRemainingLength());

			if (evt.Default.m_Cmd == XCMD_EncryptionReply && state->isClientKR())
			{
				EXC_SET("encryption reply");

				// receiving response to 0xE3 packet
				size_t iEncKrLen = evt.EncryptionReply.m_len;
				if (buffer->getRemainingLength() < iEncKrLen)
					return false; // need more data

				buffer->skip((int)(iEncKrLen));
				return true;
			}

			client->xProcessClientSetup(&evt, buffer->getRemainingLength());
			break;

		case CONNECT_HTTP:
			EXC_SET("http message");
			if (client->OnRxWebPageRequest(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
				return false;
			break;

		case CONNECT_TELNET:
			EXC_SET("telnet message");
			if (client->OnRxConsole(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
				return false;
			break;
		case CONNECT_AXIS:
			EXC_SET("Axis message");
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
	ASSERT(state != NULL);
	ASSERT(buffer != NULL);
	CClient* client = state->getClient();
	ASSERT(client != NULL);

	if (state->m_seeded == false)
	{
		// check for HTTP connection
		if ((buffer->getRemainingLength() >= 5 && memcmp(buffer->getRemainingData(), "GET /", 5) == 0) ||
			(buffer->getRemainingLength() >= 6 && memcmp(buffer->getRemainingData(), "POST /", 6) == 0))
		{
			EXC_SET("http request");
			if (g_Cfg.m_fUseHTTP != 2)
				return false;

			client->SetConnectType(CONNECT_HTTP);
			if (client->OnRxWebPageRequest(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
				return false;

			buffer->seek(buffer->getLength());
			return true;
		}

		// check for new seed (sometimes it's received on its own)
		else if (buffer->getRemainingLength() == 1 && buffer->getRemainingData()[0] == XCMD_NewSeed)
		{
			state->m_newseed = true;
			buffer->skip(1);
			return true;
		}

		// check for ping data
		else if (buffer->getRemainingLength() < 4)
		{
			EXC_SET("ping #1");
			if (client->OnRxPing(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
				state->markReadClosed();

			buffer->seek(buffer->getLength());
			return true;
		}

		// at least 4 bytes and not a web request, so must assume
		// it is a game client seed
		EXC_SET("game client seed");
		dword seed = 0;

		DEBUGNETWORK(("%x:Client connected with a seed length of %" PRIuSIZE_T " ([0]=0x%x)\n", state->id(), buffer->getRemainingLength(), (int)(buffer->getRemainingData()[0])));
		if (state->m_newseed || (buffer->getRemainingData()[0] == XCMD_NewSeed && buffer->getRemainingLength() >= NETWORK_SEEDLEN_NEW))
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
				state->m_reportedVersion = CCrypt::GetVerFromNumber(versionMajor, versionMinor, versionRevision, versionPatch);
			}
			else
			{
				DEBUGNETWORK(("%x:Not enough data received to be a valid handshake (%" PRIuSIZE_T ").\n", state->id(), buffer->getRemainingLength()));
			}
		}
		else if(buffer->getRemainingData()[0] == XCMD_UOGRequest && buffer->getRemainingLength() == 8)
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
			EXC_SET("KR client seed");

			DEBUG_WARN(("UO:KR Client Detected.\n"));
			client->SetConnectType(CONNECT_CRYPT);
			state->m_clientType = CLIENTTYPE_KR;
			new PacketKREncryption(client);
		}

		return true;
	}

	if (buffer->getRemainingLength() < 5)
	{
		// client has a seed assigned but hasn't send enough data to be anything useful,
		// some programs (ConnectUO) send a fake seed when really they're just sending
		// ping data
		EXC_SET("ping #2");
		if (client->OnRxPing(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
			return false;

		buffer->seek(buffer->getLength());
		return true;
	}

	// process login packet for client
	EXC_SET("messages  - setup");
	client->SetConnectType(CONNECT_CRYPT);
	client->xProcessClientSetup(reinterpret_cast<CEvent*>(buffer->getRemainingData()), buffer->getRemainingLength());
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
NetworkOutput::NetworkOutput() : m_thread(NULL)
{
	m_encryptBuffer = new byte[MAX_BUFFER];
}

NetworkOutput::~NetworkOutput()
{
	if (m_encryptBuffer != NULL)
		delete[] m_encryptBuffer;
}

bool NetworkOutput::processOutput()
{
	ADDTOCALLSTACK("NetworkOutput::processOutput");
	ASSERT(m_thread != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	ProfileTask networkTask(PROFILE_NETWORK_TX);

	static uchar tick = 0;
	EXC_TRY("NetworkOutput");
	tick++;

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
	EXC_SET("flush");
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
		packetsSent++;

	state->markFlush(false);
	return packetsSent;
}

size_t NetworkOutput::processPacketQueue(NetState* state, uint priority)
{
	// process a client's packet queue
	ADDTOCALLSTACK("NetworkOutput::processPacketQueue");
	ASSERT(state != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	if (state->isWriteClosed() ||
		(state->m_outgoing.queue[priority].empty() && state->m_outgoing.currentTransaction == NULL))
		return 0;

	CClient* client = state->getClient();
	ASSERT(client != NULL);

	CServerTime time = CServerTime::GetCurrentTime();
	UNREFERENCED_PARAMETER(time);

	size_t maxPacketsToProcess = NETWORK_MAXPACKETS;
	size_t maxLengthToProcess = NETWORK_MAXPACKETLEN;
	size_t packetsProcessed = 0;
	size_t lengthProcessed = 0;

	while (packetsProcessed < maxPacketsToProcess && lengthProcessed < maxLengthToProcess)
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

		// acquire next packet from transaction
		PacketSend* packet = transaction->front();
		transaction->pop();

		// if the transaction is now empty we can clear it now so we can move
		// on to the next transaction later
		if (transaction->empty())
		{
			state->m_outgoing.currentTransaction = NULL;
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
		packetsProcessed++;

		EXC_SET("sending");
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
	ASSERT(state != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	if (state->isWriteClosed() || state->isAsyncMode() == false)
		return 0;

	if (state->m_outgoing.asyncQueue.empty() || state->isSendingAsync())
		return 0;

	const CClient* client = state->getClient();
	ASSERT(client != NULL);

	// select the next packet to send
	PacketSend* packet = NULL;
	while (state->m_outgoing.asyncQueue.empty() == false)
	{
		packet = state->m_outgoing.asyncQueue.front();
		state->m_outgoing.asyncQueue.pop();

		if (packet != NULL)
		{
			// check if the client is allowed this
			if (state->canReceive(packet) && packet->onSend(client))
				break;

			// destroy the packet, we aren't going to use it
			delete packet;
			packet = NULL;
		}
	}

	if (packet == NULL)
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
	ASSERT(state != NULL);
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
	ASSERT(state != NULL);
	ASSERT(packet != NULL);
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
	ASSERT(state != NULL);
	ASSERT(packet != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	CClient* client = state->getClient();
	ASSERT(client != NULL);

	EXC_TRY("sendPacketData");
	xRecordPacket(client, packet, "server->client");

	EXC_SET("send trigger");
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

	EXC_SET("prepare data");
	byte* sendBuffer = NULL;
	size_t sendBufferLength = 0;

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

	// queue packet data
	EXC_SET("queue data");
	state->m_outgoing.bytes.AddNewData(sendBuffer, sendBufferLength);

	// if buffering is disabled then process the queue straight away
	// we need to do this rather than sending the packet data directly, otherwise if
	// the packet does not send all at once we will be stuck with an incomplete data
	// transfer (and no clean way to recover)
	if (g_Cfg.m_fUseExtraBuffer == false)
	{
		EXC_SET("send data");
		processByteQueue(state);
	}

	EXC_SET("sent trigger");
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
	ASSERT(state != NULL);
	ASSERT(data != NULL);
	ASSERT(length > 0);
	ASSERT(length != _failed_result());
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	EXC_TRY("SendData");
	size_t result = 0;

	EXC_SET("sending");

#if defined(_WIN32) && !defined(_LIBEV)
	if (state->isAsyncMode())
	{
		// send via async winsock
		ZeroMemory(&state->m_overlapped, sizeof(WSAOVERLAPPED));
		state->m_overlapped.hEvent = state;
		state->m_bufferWSA.len = static_cast<ULONG>(length);
		state->m_bufferWSA.buf = reinterpret_cast<CHAR *>(const_cast<byte *>(data));

		DWORD bytesSent;
		if (state->m_socket.SendAsync(&state->m_bufferWSA, 1, &bytesSent, 0, &state->m_overlapped, (LPWSAOVERLAPPED_COMPLETION_ROUTINE)SendCompleted) == 0)
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
		EXC_SET("error parse");
		int errorCode = CSocket::GetLastError(true);

#ifdef _WIN32
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
#ifdef _WIN32
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
			EXC_SET("unexpected connection error");
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
	ASSERT(state != NULL);
	state->setSendingAsync(false);
	if (success == false)
		return;

#ifdef MTNETWORK_OUTPUT
	// we could process another batch of async data right now, but only if we
	// are in the output thread
	// - WinSock calls this from the same thread
	// - LinuxEv calls this from a completely different thread
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
	ASSERT(packet != NULL);

	// don't bother queuing packets for invalid sockets
	NetState* state = packet->m_target;
	if (state == NULL || state->canReceive(packet) == false)
	{
		delete packet;
		return;
	}

	if (state->m_outgoing.pendingTransaction != NULL && appendTransaction)
		state->m_outgoing.pendingTransaction->push_back(packet);
	else
		QueuePacketTransaction(new SimplePacketTransaction(packet));
}

void NetworkOutput::QueuePacketTransaction(PacketTransaction* transaction)
{
	// queue a packet transaction for sending
	ADDTOCALLSTACK("NetworkOutput::QueuePacketTransaction");
	ASSERT(transaction != NULL);

	// don't bother queuing packets for invalid sockets
	NetState* state = transaction->getTarget();
	if (state == NULL || state->isInUse() == false || state->isWriteClosed())
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
			priority--;
			transaction->setPriority(priority);
		}
	}

	state->m_outgoing.queue[priority].push(transaction);

	// notify thread
	NetworkThread* thread = state->getParentThread();
	if (thread != NULL && thread->getPriority() == IThread::Disabled)
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
	ASSERT(thread != NULL);
	m_thread = thread;
	m_nextIndex = 0;
	m_safeAccess = m_thread->isActive() && ! m_thread->isCurrentThread();
}

NetworkThreadStateIterator::~NetworkThreadStateIterator(void)
{
	m_thread = NULL;
}

NetState* NetworkThreadStateIterator::next(void)
{
	if (m_safeAccess == false)
	{
		// current thread, we can use the thread's state list directly
		// find next non-null state
		while (m_nextIndex < m_thread->m_states.size())
		{
			NetState* state = m_thread->m_states.at(m_nextIndex);
			++m_nextIndex;

			if (state != NULL)
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

			if (state != NULL && state->getParentThread() == m_thread)
				return state;
		}
	}

	return NULL;
}

#endif
