
#ifdef _WIN32
	#include "../sphere/ntservice.h"    // g_Service
    #include "../sphere/ntwindow.h"     // g_NTWindow
#else
	#include "../sphere/UnixTerminal.h"
    #include <sys/resource.h>   // for setpriority
#endif

#if defined(_WIN32) && !defined(_DEBUG)
	#include "../common/crashdump/crashdump.h"
#endif

#include "../common/crypto/CCryptoKeyCalc.h"
#include "../common/CException.h"
#include "../common/CExpression.h"
#include "../common/CLog.h"
#include "../common/CTextConsole.h"
#include "../common/CUOClientVersion.h"
#include "../common/sphereversion.h"
#include "../network/CClientIterator.h"
#include "../network/CIPHistoryManager.h"
#include "../network/CNetworkManager.h"
#include "../sphere/ProfileTask.h"
#include "chars/CChar.h"
#include "clients/CAccount.h"
#include "clients/CClient.h"
#include "CScriptProfiler.h"
#include "CServer.h"
#include "CWorld.h"
#include "CWorldComm.h"
#include "CWorldGameTime.h"
#include "spheresvr.h"
#include "triggers.h"
#include <cstdio>


//******************************************************

static void dword_q_sort(dword *numbers, dword left, dword right)
{
    dword pivot, l_hold, r_hold;

    l_hold = left;
    r_hold = right;
    pivot = numbers[left];
    while (left < right)
    {
        while ((numbers[right] >= pivot) && (left < right)) right--;
        if (left != right)
        {
            numbers[left] = numbers[right];
            left++;
        }
        while ((numbers[left] <= pivot) && (left < right)) left++;
        if (left != right)
        {
            numbers[right] = numbers[left];
            right--;
        }
    }
    numbers[left] = pivot;
    pivot = left;
    left = l_hold;
    right = r_hold;
    if (left < pivot)
        dword_q_sort(numbers, left, pivot-1);
    if (right > pivot)
        dword_q_sort(numbers, pivot+1, right);
}

static void defragSphere(char *path)
{
    ASSERT(path != nullptr);

    constexpr size_t mb10 = 10*1024*1024;
    constexpr size_t mb5  = 5*1024*1024;

    CSFileText inf;
    CSFile ouf;
    char file_buf[1024];
    char path_buf[SPHERE_MAX_PATH], path_buf_2[SPHERE_MAX_PATH];
    char *str_ptr = nullptr, *str_ptr_2 = nullptr;

    g_Log.Event(LOGM_INIT,	"Defragmentation (UID alteration) of " SPHERE_TITLE " saves.\n"
                           "Use it on your risk and if you know what you are doing since it can possibly harm your server.\n"
                           "The process can take up to several hours depending on the CPU you have.\n"
                           "After finished, you will have your '" SPHERE_FILE "*.scp' files converted and saved as '" SPHERE_FILE "*.scp.new'.\n");

    // UID_O_INDEX_MASK ?
    constexpr dword MAX_UID = 40'000'000U; // limit to 40mln of objects, takes 40mln*4bytes ~= 160mb

    dword dwIdxUID = 0;
    dword* puids = (dword*)calloc(MAX_UID, sizeof(dword));
    for ( uint i = 0; i < 3; ++i )
    {
        Str_CopyLimitNull(path_buf, path, sizeof(path_buf));
        if ( i == 0 )		strcat(path_buf, SPHERE_FILE "statics" SPHERE_SCRIPT_EXT);
        else if ( i == 1 )	strcat(path_buf, SPHERE_FILE "world" SPHERE_SCRIPT_EXT);
        else				strcat(path_buf, SPHERE_FILE "chars" SPHERE_SCRIPT_EXT);

        g_Log.Event(LOGM_INIT, "Reading current UIDs: %s\n", path_buf);
        if ( !inf.Open(path_buf, OF_READ|OF_TEXT|OF_DEFAULTMODE) )
        {
            g_Log.Event(LOGM_INIT, "Cannot open file for reading. Skipped!\n");
            continue;
        }
        size_t uiBytesRead = 0, uiTotalMb = 0;
        while ((dwIdxUID < MAX_UID) && !feof(inf._pStream))
        {
            fgets(file_buf, sizeof(file_buf), inf._pStream);
            uiBytesRead += strlen(file_buf);
            if ( uiBytesRead > mb10 )
            {
                uiBytesRead -= mb10;
                uiTotalMb += 10;
                g_Log.Event(LOGM_INIT, "Total read %" PRIuSIZE_T " Mb\n", uiTotalMb);
            }
            if (( file_buf[0] == 'S' ) && ( strstr(file_buf, "SERIAL=") == file_buf ))
            {
                str_ptr = file_buf + 7;
                str_ptr_2 = str_ptr;
                while (*str_ptr_2 && (*str_ptr_2 != '\r') && (*str_ptr_2 != '\n'))
                {
                    ++str_ptr_2;
                }
                *str_ptr_2 = 0;

                //	prepare new uid
                *(str_ptr-1) = '0';
                *str_ptr = 'x';
                --str_ptr;
                puids[dwIdxUID++] = strtoul(str_ptr, &str_ptr_2, 16);
            }
        }
        inf.Close();
    }
    const dword dwTotalUIDs = dwIdxUID;
    g_Log.Event(LOGM_INIT, "Totally having %" PRIu32 " unique objects (UIDs), latest: 0%x\n", dwTotalUIDs, puids[dwTotalUIDs-1]);

    g_Log.Event(LOGM_INIT, "Quick-Sorting the UIDs array...\n");
    dword_q_sort(puids, 0, dwTotalUIDs -1);

    for ( uint i = 0; i < 5; ++i )
    {
        Str_CopyLimitNull(path_buf, path, sizeof(path_buf));
        if ( !i )			strcat(path_buf, SPHERE_FILE "accu.scp");
        else if ( i == 1 )	strcat(path_buf, SPHERE_FILE "chars" SPHERE_SCRIPT_EXT);
        else if ( i == 2 )	strcat(path_buf, SPHERE_FILE "data" SPHERE_SCRIPT_EXT);
        else if ( i == 3 )	strcat(path_buf, SPHERE_FILE "world" SPHERE_SCRIPT_EXT);
        else if ( i == 4 )	strcat(path_buf, SPHERE_FILE "statics" SPHERE_SCRIPT_EXT);
        g_Log.Event(LOGM_INIT, "Updating UID-s in %s to %s.new\n", path_buf, path_buf);
        if ( !inf.Open(path_buf, OF_READ|OF_TEXT|OF_DEFAULTMODE) )
        {
            g_Log.Event(LOGM_INIT, "Cannot open file for reading. Skipped!\n");
            continue;
        }
        Str_ConcatLimitNull(path_buf, ".new", sizeof(path_buf));
        if ( !ouf.Open(path_buf, OF_WRITE|OF_CREATE|OF_DEFAULTMODE) )
        {
            g_Log.Event(LOGM_INIT, "Cannot open file for writing. Skipped!\n");
            continue;
        }

        size_t uiBytesRead = 0, uiTotalMb = 0;
        while ( inf.ReadString(file_buf, sizeof(file_buf)) )
        {
            dwIdxUID = (dword)strlen(file_buf);
            if (dwIdxUID > (ARRAY_COUNT(file_buf) - 3))
                dwIdxUID = ARRAY_COUNT(file_buf) - 3;

            file_buf[dwIdxUID] = file_buf[dwIdxUID +1] = file_buf[dwIdxUID +2] = 0;	// just to be sure to be in line always
                // NOTE: it is much faster than to use memcpy to clear before reading
            bool fSpecial = false;
            uiBytesRead += dwIdxUID;
            if ( uiBytesRead > mb5 )
            {
                uiBytesRead -= mb5;
                uiTotalMb += 5;
                g_Log.Event(LOGM_INIT, "Total processed %" PRIuSIZE_T " Mb\n", uiTotalMb);
            }
            str_ptr = file_buf;

            //	Note 28-Jun-2004
            //	mounts seems having ACTARG1 > 0x30000000. The actual UID is ACTARG1-0x30000000. The
            //	new also should be new+0x30000000. need investigation if this can help making mounts
            //	not to disappear after the defrag
            if (( file_buf[0] == 'A' ) && ( strstr(file_buf, "ACTARG1=0") == file_buf ))		// ACTARG1=
                str_ptr += 8;
            else if (( file_buf[0] == 'C' ) && ( strstr(file_buf, "CONT=0") == file_buf ))			// CONT=
                str_ptr += 5;
            else if (( file_buf[0] == 'C' ) && ( strstr(file_buf, "CHARUID=0") == file_buf ))		// CHARUID=
                str_ptr += 8;
            else if (( file_buf[0] == 'L' ) && ( strstr(file_buf, "LASTCHARUID=0") == file_buf ))	// LASTCHARUID=
                str_ptr += 12;
            else if (( file_buf[0] == 'L' ) && ( strstr(file_buf, "LINK=0") == file_buf ))			// LINK=
                str_ptr += 5;
            else if (( file_buf[0] == 'M' ) && ( strstr(file_buf, "MEMBER=0") == file_buf ))		// MEMBER=
            {
                str_ptr += 7;
                fSpecial = true;
            }
            else if (( file_buf[0] == 'M' ) && ( strstr(file_buf, "MORE1=0") == file_buf ))		// MORE1=
                str_ptr += 6;
            else if (( file_buf[0] == 'M' ) && ( strstr(file_buf, "MORE2=0") == file_buf ))		// MORE2=
                str_ptr += 6;
            else if (( file_buf[0] == 'S' ) && ( strstr(file_buf, "SERIAL=0") == file_buf ))		// SERIAL=
                str_ptr += 7;
            else if ((( file_buf[0] == 'T' ) && ( strstr(file_buf, "TAG.") == file_buf )) ||		// TAG.=
                     (( file_buf[0] == 'R' ) && ( strstr(file_buf, "REGION.TAG") == file_buf )))
            {
                while ( *str_ptr && ( *str_ptr != '=' ))
                    ++str_ptr;
                ++str_ptr;
            }
            else if (( i == 2 ) && strchr(file_buf, '='))	// spheredata.scp - plain VARs
            {
                while ( *str_ptr && ( *str_ptr != '=' ))
                    ++str_ptr;
                ++str_ptr;
            }
            else
                str_ptr = nullptr;

            //	UIDs are always hex, so prefixed by 0
            if ( str_ptr && ( *str_ptr != '0' ))
                str_ptr = nullptr;

            //	here we got potentialy UID-contained variable
            //	check if it really is only UID-like var containing
            if ( str_ptr )
            {
                str_ptr_2 = str_ptr;
                while ( *str_ptr_2 &&
                       ((( *str_ptr_2 >= '0' ) && ( *str_ptr_2 <= '9' )) ||
                        (( *str_ptr_2 >= 'a' ) && ( *str_ptr_2 <= 'f' ))) )
                    ++str_ptr_2;
                if ( !fSpecial )
                {
                    if ( *str_ptr_2 && ( *str_ptr_2 != '\r' ) && ( *str_ptr_2 != '\n' )) // some more text in line
                        str_ptr = nullptr;
                }
            }

            //	here we definitely know that this is very uid-like
            if ( str_ptr )
            {
                char c, c1, c2;
                c = *str_ptr_2;

                *str_ptr_2 = 0;
                //	here in p we have the current value of the line.
                //	check if it is a valid UID

                //	prepare converting 0.. to 0x..
                c1 = *(str_ptr-1);
                c2 = *str_ptr;
                *(str_ptr-1) = '0';
                *str_ptr = 'x';
                --str_ptr;
                dwIdxUID = strtoul(str_ptr, &str_ptr_2, 16);
                ++str_ptr;
                *(str_ptr-1) = c1;
                *str_ptr = c2;
                //	Note 28-Jun-2004
                //	The search algourytm is very simple and fast. But maybe integrate some other, at least /2 algorythm
                //	since has amount/2 tryes at worst chance to get the item and never scans the whole array
                //	It should improve speed since defragmenting 150Mb saves takes ~2:30 on 2.0Mhz CPU
                {
                    dword dStep = dwTotalUIDs /2;
                    dword d = dStep;
                    for (;;)
                    {
                        dStep /= 2;

                        if ( puids[d] == dwIdxUID)
                        {
                            dwIdxUID = d | (puids[d]&0xF0000000);	// do not forget attach item and special flags like 04..
                            break;
                        }
                        else
                        {
                            if (puids[d] < dwIdxUID)
                                d += dStep;
                            else
                                d -= dStep;
                        }

                        if ( dStep == 1 )
                        {
                            dwIdxUID = 0xFFFFFFFFL;
                            break; // did not find the UID
                        }
                    }
                }

                //	Search for this uid in the table
                /*				for ( d = 0; d < dTotalUIDs; d++ )
                {
                    if ( !uids[d] )	// end of array
                    {
                        uid = 0xFFFFFFFFL;
                        break;
                    }
                    else if ( uids[d] == uid )
                    {
                        uid = d | (uids[d]&0xF0000000);	// do not forget attach item and special flags like 04..
                        break;
                    }
                }*/

                //	replace UID by the new one since it has been found
                *str_ptr_2 = c;
                if (dwIdxUID != 0xFFFFFFFFL )
                {
                    *str_ptr = 0;
                    ASSERT(strlen(str_ptr_2) < sizeof(path_buf));
                    Str_CopyLimitNull(path_buf, str_ptr_2, sizeof(path_buf));  // here we don't need anymore the old values of path_buf, so i can reuse it here
                    snprintf(path_buf_2, sizeof(path_buf_2), "0%" PRIx32, dwIdxUID);
                    strcat(file_buf, path_buf_2);
                    strcat(file_buf, path_buf);
                }
            }
            //	output the resulting line
            ouf.Write(file_buf, (int)strlen(file_buf));
        }
        inf.Close();
        ouf.Close();
    }

    free(puids);
    g_Log.Event(LOGM_INIT,	"Defragmentation complete.\n");
}


////////////////////////////////////////////////////////////////////////////////////////
// -CServer

CServer::CServer() : CServerDef( SPHERE_TITLE, CSocketAddressIP( SOCKET_LOCAL_ADDRESS ))
{
	SetExitFlag(0);
	m_fResyncPause = false;
	m_fResyncRequested = nullptr;

#ifdef _WIN32
    _fCloseNTWindowOnTerminate = false;
#endif

	m_iAdminClients = 0;

	m_timeShutdown = 0;

	m_fConsoleTextReadyFlag = false;

	// we are in start up mode. // IsLoading()
	SetServerMode( SERVMODE_PreLoadingINI );

	memset(m_PacketFilter, 0, sizeof(m_PacketFilter));
	memset(m_OutPacketFilter, 0, sizeof(m_OutPacketFilter));
}

CServer::~CServer()
{
}

void CServer::SetSignals( bool fMsg )
{
	// We have just started or we changed Secure mode.

	// set_terminate(  &Exception_Terminate );
	// set_unexpected( &Exception_Unexpected );

#ifndef _WIN32
	SetUnixSignals(g_Cfg.m_fSecure);
    LOG_TYPE lt = LOGM_INIT;
    if (!IsLoading()) {
        lt = (LOG_TYPE)((uint)lt | (uint)LOGL_EVENT);
    }
	if ( g_Cfg.m_fSecure )
	{
		g_Log.Event( lt, "Signal handlers installed.\n" );
	}
	else
	{
		g_Log.Event( lt, "Signal handlers UNinstalled.\n" );
	}
#endif

	if ( fMsg && !IsLoading() )
	{
		CWorldComm::Broadcast( g_Cfg.m_fSecure ?
			"The world is now running in SECURE MODE." :
			"WARNING: The world is NOT running in SECURE MODE." );
	}
}

bool CServer::SetProcessPriority(int iPriorityLevel)
{
    bool fSuccess;
#ifdef _WIN32
    HANDLE hCurrentProcess = GetCurrentProcess();
    DWORD dwPri;
    switch (iPriorityLevel)
    {
        case -1:    dwPri = BELOW_NORMAL_PRIORITY_CLASS;    break;
        default:
        case 0:     dwPri = NORMAL_PRIORITY_CLASS;          break;
        case 1:     dwPri = ABOVE_NORMAL_PRIORITY_CLASS;    break;
        case 2:     dwPri = HIGH_PRIORITY_CLASS;            break;
    }
    fSuccess = (bool)SetPriorityClass(hCurrentProcess, dwPri);
#else
    int iPri;
    switch (iPriorityLevel)
    {
        case -1:    iPri = 7;   break;
        default:
        case 0:     iPri = 0;   break;
        case 1:     iPri = -8;  break;
        case 2:     iPri = -15; break;
    }
    // This will FAIL if you aren't running Sphere as root, or if root didn't set for this process the s bit (chmod +x).
    if ( setpriority(PRIO_PROCESS, 0, iPri) == -1 )
        fSuccess = false;
    else
        fSuccess = true;
    //needed option for mac?
#endif
    return fSuccess;
}

void CServer::SetServerMode( SERVMODE_TYPE mode )
{
	ADDTOCALLSTACK("CServer::SetServerMode");
	m_iModeCode.store(mode, std::memory_order_release);
#ifdef _WIN32
    g_NTWindow.SetWindowTitle();
#endif
}

bool CServer::IsValidBusy() const
{
    // We might appear to be stopped but it's really ok ?
    // ?
    switch ( GetServerMode() )
    {
        case SERVMODE_Saving:
            if ( g_World.IsSaving() )
                return true;
            break;
        case SERVMODE_Loading:
        case SERVMODE_GarbageCollection:
        case SERVMODE_RestockAll:	// these may look stuck but are not.
            return true;
        default:
            return false;
    }
    return false;
}

int CServer::GetExitFlag() const noexcept
{
    return m_iExitFlag.load(std::memory_order_acquire);
}

void CServer::SetExitFlag(int iFlag) noexcept
{
    ADDTOCALLSTACK("CServer::SetExitFlag");
    if ( GetExitFlag() )
        return;
    m_iExitFlag.store(iFlag, std::memory_order_release);
}

bool CServer::IsLoading() const noexcept
{
    return ( m_fResyncPause || (GetServerMode() > SERVMODE_Run) );
}

bool CServer::IsResyncing() const noexcept
{
    return m_fResyncPause || (GetServerMode() == SERVMODE_ResyncLoad);
}

void CServer::Shutdown( int64 iMinutes ) // If shutdown is initialized
{
	ADDTOCALLSTACK("CServer::Shutdown");
	if ( iMinutes <= 0 )
	{
		if ( m_timeShutdown == 0 )
			return;

		m_timeShutdown = 0;
		CWorldComm::Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_MSG_SERV_SHUTDOWN_CANCEL ) );
		return;
	}

	m_timeShutdown = CWorldGameTime::GetCurrentTime().GetTimeRaw() + ( iMinutes * 60 * MSECS_PER_SEC);
	CWorldComm::Broadcastf(g_Cfg.GetDefaultMsg( DEFMSG_MSG_SERV_SHUTDOWN ), iMinutes);
}

void CServer::SysMessage( lpctstr pszMsg ) const
{
	// Print just to the main console.
	if ( !pszMsg )
		return;

#ifdef _WIN32
    g_NTWindow.AddConsoleOutput(std::make_unique<ConsoleOutput>(pszMsg));
#else
    g_UnixTerminal.AddConsoleOutput(std::make_unique<ConsoleOutput>(pszMsg));
#endif
}

void CServer::SysMessage(std::unique_ptr<ConsoleOutput>&& pMsg) const
{
    // Print just to the main console.
#ifdef _WIN32
    g_NTWindow.AddConsoleOutput(std::move(pMsg));
#else
    g_UnixTerminal.AddConsoleOutput(std::move(pMsg));
#endif
}

void CServer::PrintTelnet( lpctstr pszMsg ) const
{
	if ( ! m_iAdminClients )
		return;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if (( pClient->GetConnectType() == CONNECT_TELNET ) && pClient->GetAccount() )
		{
			if ( !pClient->GetAccount()->IsPriv(PRIV_TELNET_SHORT) )	// this client accepts broadcasts
				pClient->SysMessage(pszMsg);
		}
	}
}

void CServer::PrintStr(lpctstr ptcMsg) const
{
	// print to all consoles.
	if (!ptcMsg)
		return;

	SysMessage(ptcMsg);
	PrintTelnet(ptcMsg);
}

void CServer::PrintStr(ConsoleTextColor iColor, lpctstr ptcMsg) const
{
    // print to all consoles.
	if (!ptcMsg)
		return;

    SysMessage(std::make_unique<ConsoleOutput>(iColor, ptcMsg));
    PrintTelnet(ptcMsg);
}

ssize_t CServer::PrintPercent( ssize_t iCount, ssize_t iTotal ) const
{
	ADDTOCALLSTACK("CServer::PrintPercent");
	if ( iTotal <= 0 )
		return 100;

	int iPercent = (int)IMulDivLL( iCount, 100, iTotal );
	tchar *pszTemp = Str_GetTemp();
	snprintf(pszTemp, Str_TempLength(), "%d%%", iPercent);
	size_t len = strlen(pszTemp);

	PrintTelnet(pszTemp);

#ifndef _WIN32
	if ( g_UnixTerminal.isColorEnabled() )
	{
		SysMessage(pszTemp);
#endif
		while ( len > 0 )	// backspace it
		{
			PrintTelnet("\x08");
#ifndef _WIN32
 			SysMessage("\x08");
#endif
			--len;
		}
#ifndef _WIN32
	}
#endif

#ifdef _WIN32
    g_NTWindow.SetWindowTitle(pszTemp);
	g_NTService._OnTick();
#endif
	return iPercent;
}

int64 CServer::GetAgeHours() const
{
	ADDTOCALLSTACK("CServer::GetAgeHours");
	return (CWorldGameTime::GetCurrentTime().GetTimeRaw() / (60 * 60 * MSECS_PER_SEC));
}

lpctstr CServer::GetStatusString( byte iIndex ) const
{
	ADDTOCALLSTACK("CServer::GetStatusString");
	// NOTE: The key names should match those in CServerDef::r_LoadVal
	// A ping will return this as well.
	// 0 or 0x21 = main status.

	tchar * pTemp = Str_GetTemp();
	size_t iClients = StatGet(SERV_STAT_CLIENTS);
	int64 iHours = GetAgeHours() / 24;

	switch ( iIndex )
	{
		case 0x21:	// '!'
			// typical (first time) poll response.
			{
				std::string cliver = m_ClientVersion.GetClientVer();
				snprintf(pTemp, Str_TempLength(), SPHERE_TITLE ", Name=%s, Port=%d, Ver=" SPHERE_BUILD_INFO_STR ", TZ=%d, EMail=%s, URL=%s, Lang=%s, CliVer=%s\n",
					GetName(), m_ip.GetPort(), m_TimeZone, m_sEMail.GetBuffer(), m_sURL.GetBuffer(), m_sLang.GetBuffer(), cliver.c_str());
			}
			break;
		case 0x22: // '"'
			{
			// shown in the INFO page in game.
			snprintf(pTemp, Str_TempLength(), SPHERE_TITLE ", Name=%s, Age=%" PRId64 ", Clients=%" PRIuSIZE_T ", Items=%" PRIuSIZE_T ", Chars=%" PRIuSIZE_T ", Mem=%" PRIuSIZE_T "K\n",
				GetName(), iHours, iClients, StatGet(SERV_STAT_ITEMS), StatGet(SERV_STAT_CHARS), StatGet(SERV_STAT_MEM));
			}
			break;
		case 0x24: // '$'
			// show at startup.
			snprintf(pTemp, Str_TempLength(), "Admin=%s, URL=%s, Lang=%s, TZ=%d\n",
				m_sEMail.GetBuffer(), m_sURL.GetBuffer(), m_sLang.GetBuffer(), m_TimeZone);
			break;
		case 0x25: // '%'
			// ConnectUO Status string
			snprintf(pTemp, Str_TempLength(), SPHERE_TITLE " Items=%" PRIuSIZE_T ", Mobiles=%" PRIuSIZE_T ", Clients=%" PRIuSIZE_T ", Mem=%" PRIuSIZE_T,
				StatGet(SERV_STAT_ITEMS), StatGet(SERV_STAT_CHARS), iClients, StatGet(SERV_STAT_MEM));
			break;
	}

	return pTemp;
}

//*********************************************************

void CServer::ListClients( CTextConsole *pConsole ) const
{
	ADDTOCALLSTACK("CServer::ListClients");
	// Mask which clients we want ?
	// Give a format of what info we want to SHOW ?
	if ( pConsole == nullptr )
		return;

	const CChar *pCharCmd = pConsole->GetChar();
	tchar *ptcMsg = Str_GetTemp();

    // Count clients
    size_t numClients = 0;
    {
        ClientIterator it;
        for (const CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
            ++numClients;
    }

    if (numClients <= 0)
        snprintf(ptcMsg, Str_TempLength(), "%s\n", g_Cfg.GetDefaultMsg(DEFMSG_HL_NO_CLIENT));
    else if (numClients == 1)
        snprintf(ptcMsg, Str_TempLength(), "%s\n", g_Cfg.GetDefaultMsg(DEFMSG_HL_ONE_CLIENT));
    else
        snprintf(ptcMsg, Str_TempLength(), "%s %" PRIuSIZE_T "\n", g_Cfg.GetDefaultMsg(DEFMSG_HL_MANY_CLIENTS), numClients);

    pConsole->SysMessage(ptcMsg);


	ptcMsg = Str_GetTemp();

	ClientIterator it;
	for ( const CClient *pClient = it.next(); pClient != nullptr; pClient = it.next() )
	{
        const CChar* pChar = pClient->GetChar();
        const CAccount* pAcc = pClient->GetAccount();
        const tchar chRank = (pAcc && pAcc->GetPrivLevel() > PLEVEL_Player) ? '+' : '=';

		if ( pChar )
		{
			if ( pCharCmd && !pCharCmd->CanDisturb(pChar) )
				continue;

			snprintf(ptcMsg, Str_TempLength(),
                     "%" PRIx32 ":Acc%c'%s', Char='%s' (IP: %s)\n",
                     pClient->GetSocketID(), chRank, (!pAcc ? "null" : pAcc->GetName()), pChar->GetName(), pClient->GetPeerStr());
		}
		else
		{
			if ( pConsole->GetPrivLevel() < pClient->GetPrivLevel() )
				continue;

            lpcstr pszState;
			switch ( pClient->GetConnectType() )
			{
				case CONNECT_HTTP:
					pszState = "WEB";
					break;
				case CONNECT_TELNET:
					pszState = "TELNET";
					break;
				default:
					pszState = "NOT LOGGED IN";
					break;
			}

			snprintf(ptcMsg, Str_TempLength(),
                     "%" PRIx32 ":Acc%c'%s' (IP: %s) %s\n",
                     pClient->GetSocketID(), chRank, (pAcc ? pAcc->GetName() : "<NA>"), pClient->GetPeerStr(), pszState);
		}

        pConsole->SysMessage(ptcMsg);
	}
}

bool CServer::OnConsoleCmd( CSString & sText, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CServer::OnConsoleCmd");
	// RETURN: false = unsuccessful command.
	size_t len = sText.GetLength();

	// We can't have a command with no length
	if ( len <= 0 )
		return true;

	// Convert first character to lowercase
	tchar low = static_cast<tchar>(tolower(sText[0]));
	bool fRet = true;

	if ((( len > 2 ) || (( len == 2 ) && ( sText[1] != '#' ))) && ( sText[0] != 'd' ))
		goto longcommand;

	switch ( low )
	{
		case '?':
			g_Log.Event(LOGL_EVENT,
				"Available commands:\n"
				"#         Immediate save world (## to save both world and statics)\n"
				"A         Update pending changes on Accounts file\n"
				"B [msg]   Broadcast message to all clients\n"
				"C         List of online Clients (%" PRIuSIZE_T ")\n"
				"DA        Dump Areas to external file\n"
				"DUI       Dump Unscripted Items to external file\n"
				"E         Clear internal variables (like script profile)\n"
				"G         Garbage collection\n"
				"H         Hear all that is said (%s)\n"
				"I         View server Information\n"
				"L         Toggle log file (%s)\n"
				"P         Profile Info (%s) (P# to dump to profiler_dump.txt)\n"
				"R         Resync Pause\n"
				"S         Secure mode toggle (%s)\n"
				"STRIP     Dump all script templates to external file, formatted for Axis\n"
				"STRIPTNG  Dump all script templates to external file, formatted for TNG\n"
				"T         List of active Threads\n"
				"U         List used triggers\n"
				"X         Immediate exit the server (X# to save world and statics before exit)\n"
				,
				StatGet(SERV_STAT_CLIENTS),
				g_Log.IsLoggedMask( LOGM_PLAYER_SPEAK ) ? "ON" : "OFF",
				g_Log.IsFileOpen() ? "OPEN" : "CLOSED",
				GetCurrentProfileData().IsActive() ? "ON" : "OFF",
				g_Cfg.m_fSecure ? "ON" : "OFF"
				);
			break;
		case '#':	//	# - save world, ## - save both world and statics
			{		// Start a syncronous save or finish a background save synchronously
				if ( g_Serv.m_fResyncPause )
					goto do_resync;

				g_World.Save(true);
				if (( len > 1 ) && ( sText[1] == '#' ))
					g_World.SaveStatics();	// ## means
			} break;
		case 'a':
			{
				// Force periodic stuff
				g_Accounts.Account_SaveAll();
				g_Cfg._OnTick(true);
			} break;
		case 'c':	// List all clients on line.
			{
				ListClients( pSrc );
			} break;
		case 'd': // dump
			{
				lpctstr ptcKey = sText + 1;
				GETNONWHITESPACE( ptcKey );
				switch ( tolower(*ptcKey) )
				{
					case 'a': // areas
						ptcKey++;
                        GETNONWHITESPACE( ptcKey );
						if ( !g_World.DumpAreas( pSrc, ptcKey ) )
						{
                            if (pSrc != this)
                            {
                                pSrc->SysMessage("Area dump failed.\n");
                            }
                            else
                            {
                                g_Log.Event(LOGL_EVENT, "Area dump failed.\n");
                            }
							fRet = false;
						}
                        else
                        {
                            if (pSrc != this)
                            {
                                pSrc->SysMessage("Area dump successful.\n");
                            }
                            else
                            {
                                g_Log.Event(LOGL_EVENT, "Area dump successful.\n");
                            }
                        }
						break;

					case 'u': // unscripted
						ptcKey++;

						switch ( tolower(*ptcKey) )
						{
							case 'i': // items
							{
								ptcKey++;	GETNONWHITESPACE( ptcKey );
								if ( !g_Cfg.DumpUnscriptedItems( pSrc, ptcKey ) )
								{
                                    if (pSrc != this)
                                    {
                                        pSrc->SysMessage("Unscripted item dump failed.\n");
                                    }
                                    else
                                    {
                                        g_Log.Event(LOGL_EVENT, "Unscripted item dump failed.\n");
                                    }
									fRet = false;
								}
                                else
                                {
                                    if (pSrc != this)
                                    {
                                        pSrc->SysMessage("Unscripted item dump successful.\n");
                                    }
                                    else
                                    {
                                        g_Log.Event(LOGL_EVENT, "Unscripted item dump successful.\n");
                                    }
                                }
								break;
							}

							default:
								goto longcommand;
						}
						break;

					default:
						goto longcommand;
				}
			} break;
		case 'e':
			{
				if ( IsSetEF(EF_Script_Profiler) )
				{
					if ( g_profiler.initstate == 0xf1 )
					{
						CScriptProfiler::CScriptProfilerFunction	*pFun;
						CScriptProfiler::CScriptProfilerTrigger		*pTrig;

						for ( pFun = g_profiler.FunctionsHead; pFun != nullptr; pFun = pFun->next )
						{
							pFun->average = pFun->max = pFun->min = pFun->total = pFun->called = 0;
						}
						for ( pTrig = g_profiler.TriggersHead; pTrig != nullptr; pTrig = pTrig->next )
						{
							pTrig->average = pTrig->max = pTrig->min = pTrig->total = pTrig->called = 0;
						}

						g_profiler.total = g_profiler.called = 0;
                        g_Log.Event(LOGL_EVENT, "Scripts profiler info cleared\n");
					}
				}
                else
                {
                    g_Log.Event(LOGL_EVENT, "Script profiler feature is not enabled on Sphere.ini.\n");
                }
                g_Log.Event(LOGL_EVENT, "Complete!\n");
			} break;
		case 'g':
			{
				if ( g_Serv.m_fResyncPause )
				{
                do_resync:
                    if (pSrc != this)
                    {
                        pSrc->SysMessage("Not allowed during resync pause. Use 'R' to restart.\n");
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "Not allowed during resync pause. Use 'R' to restart.\n");
                    }
					fRet = false;
					break;
				}
				if ( g_World.IsSaving() )
				{
	            do_saving:
                    if (pSrc != this)
                    {
                        pSrc->SysMessage("Not allowed during background worldsave. Use '#' to finish.\n");
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "Not allowed during background worldsave. Use '#' to finish.\n");
                    }
					fRet = false;
					break;
				}
				g_World.GarbageCollection();
			} break;
		case 'h':	// Hear all said.
			{
				CScript script( "HEARALL" );
				fRet = r_Verb( script, pSrc );
			} break;
		case 'i':
			{
				CScript script( "INFORMATION" );
				fRet = r_Verb( script, pSrc );
			} break;
		case 'l': // Turn the log file on or off.
			{
				CScript script( "LOG" );
				fRet = r_Verb( script, pSrc );
			} break;
		case 'p':	// Display profile information.
			{
				ProfileDump(pSrc, ( ( len > 1 ) && ( sText[1] == '#' ) ));
			} break;
		case 'r':	// resync Pause mode. Allows resync of things in files.
			{
				if ( pSrc != this ) // not from console
				{
					pSrc->SysMessage("'r' command only allowed in the console. Use 'resync' instead.\n");
				}
				else
				{
					if ( !m_fResyncPause && g_World.IsSaving() )
						goto do_saving;
					SetResyncPause(!m_fResyncPause, pSrc, true);
				}
			} break;
		case 's':
			{
				CScript script( "SECURE" );
				fRet = r_Verb( script, pSrc );
			} break;
		case 't':
			{
                if (pSrc != this)
                {
                    pSrc->SysMessagef("Current active threads: %" PRIuSIZE_T ".\n", ThreadHolder::get().getActiveThreads());
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "Current active threads: %" PRIuSIZE_T ".\n", ThreadHolder::get().getActiveThreads());
                }
				size_t iThreadCount = ThreadHolder::get().getActiveThreads();
				for ( size_t iThreads = 0; iThreads < iThreadCount; ++iThreads )
				{
					AbstractThread * thrCurrent = ThreadHolder::get().getThreadAt(iThreads);
					if (thrCurrent != nullptr)
					{
                        pSrc->SysMessagef(
                            "%" PRIuSIZE_T " - Id: %" PRIu64 ", Priority: %d, Name: %s.\n",
                            (iThreads + 1),
                            (uint64)thrCurrent->getId(),
                            enum_alias_cast<int>(thrCurrent->getPriority()),
                            thrCurrent->getName()
                            );
					}
				}
			} break;
		case 'u':
			TriglistPrint();
			break;
		case 'x':
			{
				if (( len > 1 ) && ( sText[1] == '#' ))	//	X# - exit with save. Such exit is not protected by secure mode
				{
					if ( g_Serv.m_fResyncPause )
						goto do_resync;

					g_World.Save(true);
					g_World.SaveStatics();
					g_Log.Event( LOGL_FATAL, "Immediate Shutdown initialized!\n");
					SetExitFlag(1);
				}
				else if ( g_Cfg.m_fSecure )
				{
                    if (pSrc != this)
                    {
                        pSrc->SysMessage("NOTE: Secure mode prevents keyboard exit!\n");
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "NOTE: Secure mode prevents keyboard exit!\n");
                    }
					fRet = false;
				}
				else
				{
					g_Log.Event( LOGL_FATAL, "Immediate Shutdown initialized!\n");
					SetExitFlag( 1 );
				}
			} break;
#ifdef _TESTEXCEPTION
		case '$':	// call stack integrity
			{
	#ifdef _EXCEPTIONS_DEBUG
				{ // test without freezing the call stack
					ADDTOCALLSTACK("CServer::TestException1");
					EXC_TRY("Test1");
					throw CSError( LOGM_DEBUG, 0, "Test Exception #1");
					}
					catch (const CSError& e)
					{
						// the following call will destroy the stack trace on linux due to
						// a call to CSFile::Close fromn CLog::EventStr.
						g_Log.Event( LOGM_DEBUG, "Caught exception\n" );
                        EXC_CATCH_EXCEPTION_SPHERE(&e);
					}
				}

				{ // test pausing the call stack
					ADDTOCALLSTACK("CServer::TestException2");
					EXC_TRY("Test2");
					throw CSError( LOGM_DEBUG, 0, "Test Exception #2");
					}
					catch (const CSError& e)
					{
                        StackDebugInformation::freezeCallStack(true);
						// with freezeCallStack, the following call won't be recorded
						g_Log.Event( LOGM_DEBUG, "Caught exception\n" );
                        StackDebugInformation::freezeCallStack(false);
                        EXC_CATCH_EXCEPTION_SPHERE(&e);
					}
				}
	#else
				throw CSError(LOGL_CRIT, E_FAIL, "This test requires exception debugging enabled");
	#endif
			} break;
		case '%':	// throw simple exception
			{
				throw 0;
			}
		case '^':	// throw language exception (div 0)
			{
				int i = 5;
				int j = 0;
				i /= j;
			}
		case '&':	// throw nullptr pointer exception
			{
				CChar *character = nullptr;
				character->SetName("yo!");
			}
		case '*':	// throw custom exception
			{
				throw CSError(LOGL_CRIT, (dword)E_FAIL, "Test Exception");
			}
#endif
		default:
			goto longcommand;
	}
	goto endconsole;

longcommand:
	if ((( len > 1 ) && ( sText[1] != ' ' )) || ( low == 'b' ))
	{
		lpctstr	pszText = sText;

		if ( !strnicmp(pszText, "strip", 5) || !strnicmp(pszText, "tngstrip", 8))
		{
			size_t			i = 0;
			CResourceScript	*script;
			FILE			*f, *f1;
			char			*z = Str_GetTemp();
			char			*y = Str_GetTemp();
			char			*x;
			lpctstr			dirname;

			if ( g_Cfg.m_sStripPath.IsEmpty() )
			{
                if (pSrc != this)
                {
                    pSrc->SysMessage("StripPath not defined, function aborted.\n");
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "StripPath not defined, function aborted.\n");
                }
				return false;
			}

			dirname = g_Cfg.m_sStripPath;


			if ( !strnicmp(pszText, "strip tng", 9) || !strnicmp(pszText, "tngstrip", 8))
			{
				Str_CopyLimitNull(z, dirname, Str_TempLength());
				Str_ConcatLimitNull(z, "sphere_strip_tng" SPHERE_SCRIPT_EXT, Str_TempLength());
                if (pSrc != this)
                {
                    pSrc->SysMessagef("StripFile is %s.\n", z);
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "StripFile is %s.\n", z);
                }

				f1 = fopen(z, "w");

				if ( !f1 )
				{
                    if (pSrc != this)
                    {
                        pSrc->SysMessagef("Cannot open file %s for writing.\n", z);
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "Cannot open file %s for writing.\n", z);
                    }
					return false;
				}

				while ( (script = g_Cfg.GetResourceFile(i++)) != nullptr )
				{
					Str_CopyLimitNull(z, script->GetFilePath(), Str_TempLength());
					f = fopen(z, "r");
					if ( !f )
					{
                        if (pSrc != this)
                        {
                            pSrc->SysMessagef("Cannot open file %s for reading.\n", z);
                        }
                        else
                        {
                            g_Log.Event(LOGL_EVENT, "Cannot open file %s for reading.\n", z);
                        }
						continue;
					}

					while ( !feof(f) )
					{
						z[0] = 0;
						y[0] = 0;
						fgets(y, SCRIPT_MAX_LINE_LEN, f);

						x = y;
						GETNONWHITESPACE(x);
						Str_CopyLimitNull(z, x, Str_TempLength());

						_strlwr(z);

						if ( (( z[0] == '[' ) && strnicmp(z, "[eof]", 5) != 0) || !strnicmp(z, "defname", 7) ||
							!strnicmp(z, "name", 4) || !strnicmp(z, "type", 4) || !strnicmp(z, "id", 2) ||
							!strnicmp(z, "weight", 6) || !strnicmp(z, "value", 5) || !strnicmp(z, "dam", 3) ||
							!strnicmp(z, "armor", 5) || !strnicmp(z, "skillmake", 9) || !strnicmp(z, "on=@", 4) ||
							!strnicmp(z, "dupeitem", 8) || !strnicmp(z, "dupelist", 8) || !strnicmp(z, "p=", 2) ||
							!strnicmp(z, "can", 3) || !strnicmp(z, "tevents", 7) || !strnicmp(z, "subsection", 10) ||
							!strnicmp(z, "description", 11) || !strnicmp(z, "category", 8) || !strnicmp(z, "color", 5) ||
							!strnicmp(z, "resources", 9) )
						{
							fputs(y, f1);
						}
					}
					fclose(f);
				}
				fclose(f1);
                if (pSrc != this)
                {
                    pSrc->SysMessage("Scripts have just been stripped.\n");
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "Scripts have just been stripped.\n");
                }
				return true;
			}
			else if ( !strnicmp(pszText, "strip axis", 10) || !strnicmp(pszText, "strip", 5) )
			{
				Str_CopyLimitNull(z, dirname, Str_TempLength());
				Str_ConcatLimitNull(z, "sphere_strip_axis" SPHERE_SCRIPT_EXT, Str_TempLength());
                if (pSrc != this)
                {
                    pSrc->SysMessagef("StripFile is %s.\n", z);
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "StripFile is %s.\n", z);
                }

				f1 = fopen(z, "w");

				if ( !f1 )
				{
                    if (pSrc != this)
                    {
                        pSrc->SysMessagef("Cannot open file %s for writing.\n", z);
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "Cannot open file %s for writing.\n", z);
                    }
					return false;
				}

				while ( (script = g_Cfg.GetResourceFile(i++)) != nullptr )
				{
					Str_CopyLimitNull(z, script->GetFilePath(), Str_TempLength());
					f = fopen(z, "r");
					if ( !f )
					{
                        if (pSrc != this)
                        {
                            pSrc->SysMessagef("Cannot open file %s for reading.\n", z);
                        }
                        else
                        {
                            g_Log.Event(LOGL_EVENT, "Cannot open file %s for reading.\n", z);
                        }
						continue;
					}

					while ( !feof(f) )
					{
						z[0] = 0;
						y[0] = 0;
						fgets(y, SCRIPT_MAX_LINE_LEN, f);

						x = y;
						GETNONWHITESPACE(x);
						Str_CopyLimitNull(z, x, Str_TempLength());

						_strlwr(z);

						if ( (( z[0] == '[' ) && strnicmp(z, "[eof]", 5) != 0) || !strnicmp(z, "defname", 7) ||
							!strnicmp(z, "name", 4) || !strnicmp(z, "type", 4) || !strnicmp(z, "id", 2) ||
							!strnicmp(z, "weight", 6) || !strnicmp(z, "value", 5) || !strnicmp(z, "dam", 3) ||
							!strnicmp(z, "armor", 5) || !strnicmp(z, "skillmake", 9) || !strnicmp(z, "on=@", 4) ||
							!strnicmp(z, "dupeitem", 8) || !strnicmp(z, "dupelist", 8) || !strnicmp(z, "can", 3) ||
							!strnicmp(z, "tevents", 7) || !strnicmp(z, "subsection", 10) || !strnicmp(z, "description", 11) ||
							!strnicmp(z, "category", 8) || !strnicmp(z, "p=", 2) || !strnicmp(z, "resources", 9) ||
							!strnicmp(z, "group", 5) || !strnicmp(z, "rect=", 5) )
						{
							fputs(y, f1);
						}
					}
					fclose(f);
				}
				fclose(f1);
                if (pSrc != this)
                {
                    pSrc->SysMessagef("Scripts have just been stripped.\n");
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "Scripts have just been stripped.\n");
                }
				return true;
			}
		}

		if ( g_Cfg.IsConsoleCmd(low) )
			++pszText;

		CScript	script(pszText);
		if ( !g_Cfg.CanUsePrivVerb(this, pszText, pSrc) )
		{
            if (pSrc != this)
            {
                pSrc->SysMessagef("not privileged for command '%s'\n", pszText);
            }
            else
            {
                g_Log.Event(LOGL_EVENT, "not privileged for command '%s'\n", pszText);
            }
			fRet = false;
		}
		else if ( !r_Verb(script, pSrc) )
		{
            if (pSrc != this)
            {
                pSrc->SysMessagef("unknown command '%s'\n", pszText);
            }
            else
            {
                g_Log.Event(LOGL_EVENT, "unknown command '%s'\n", pszText);
            }
			fRet = false;
		}
	}
	else
	{
        if (pSrc != this)
        {
            pSrc->SysMessagef("unknown command '%s'\n", static_cast<lpctstr>(sText));
        }
        else
        {
            g_Log.Event(LOGL_EVENT, "unknown command '%s'\n", static_cast<lpctstr>(sText));
        }
		fRet = false;
	}

endconsole:
	sText.Clear();
	return fRet;
}

//************************************************

PLEVEL_TYPE CServer::GetPrivLevel() const
{
	return PLEVEL_Owner;
}

void CServer::ProfileDump( CTextConsole * pSrc, bool bDump )
{
	ADDTOCALLSTACK("CServer::ProfileDump");
	if ( !pSrc )
		return;

	CSFileText * ftDump = nullptr;

	if ( bDump )
	{
		ftDump = new CSFileText();
		if ( ! ftDump->Open("profiler_dump.txt", OF_CREATE|OF_TEXT) )
		{
			delete ftDump;
			ftDump = nullptr;
		}
	}

    if (pSrc != this)
    {
        pSrc->SysMessagef("Profiles %s: (%d sec total)\n", GetCurrentProfileData().IsActive() ? "ON" : "OFF", GetCurrentProfileData().GetActiveWindow());
    }
    else
    {
        g_Log.Event(LOGL_EVENT, "Profiles %s: (%d sec total)\n", GetCurrentProfileData().IsActive() ? "ON" : "OFF", GetCurrentProfileData().GetActiveWindow());
    }
    if (ftDump != nullptr)
    {
        ftDump->Printf("Profiles %s: (%d sec total)\n", GetCurrentProfileData().IsActive() ? "ON" : "OFF", GetCurrentProfileData().GetActiveWindow());
    }

	size_t uiThreadCount = ThreadHolder::get().getActiveThreads();
	for ( size_t iThreads = 0; iThreads < uiThreadCount; ++iThreads)
	{
		AbstractThread* thrCurrent = ThreadHolder::get().getThreadAt(iThreads);
		if (thrCurrent == nullptr)
			continue;

		const ProfileData& profile = static_cast<AbstractSphereThread*>(thrCurrent)->m_profile;
		if (profile.IsEnabled() == false)
			continue;

        if (pSrc != this)
        {
            pSrc->SysMessagef("Thread %llu, Name=%s\n", (ullong)thrCurrent->getId(), thrCurrent->getName());
        }
        else
        {
            g_Log.Event(LOGL_EVENT, "Thread %llu, Name=%s\n", (ullong)thrCurrent->getId(), thrCurrent->getName());
        }
		if (ftDump != nullptr)
		{
			ftDump->Printf("Thread %llu, Name=%s\n", (ullong)thrCurrent->getId(), thrCurrent->getName());
		}

		for (int i = 0; i < PROFILE_QTY; ++i)
		{
            const PROFILE_TYPE iProfile = (PROFILE_TYPE)i;
			if (profile.IsEnabled(iProfile) == false)
				continue;

            if (pSrc != this)
            {
                pSrc->SysMessagef("%-14s = %s\n", profile.GetName(iProfile), profile.GetDescription(iProfile));
            }
            else
            {
                g_Log.Event(LOGL_EVENT, "%-14s = %s\n", profile.GetName(iProfile), profile.GetDescription(iProfile));
            }
            if (ftDump != nullptr)
            {
                ftDump->Printf("%-14s = %s\n", profile.GetName(iProfile), profile.GetDescription(iProfile));
            }
		}
	}

	if ( IsSetEF(EF_Script_Profiler) )
	{
        if (g_profiler.initstate != 0xf1)
        {
            if (pSrc != this)
            {
                pSrc->SysMessage("Scripts profiler is not initialized\n");
            }
            else
            {
                g_Log.Event(LOGL_EVENT, "Scripts profiler is not initialized\n");
            }
        }
        else if (!g_profiler.called)
        {
            if (pSrc != this)
            {
                pSrc->SysMessage("Script profiler is not yet informational\n");
            }
            else
            {
                g_Log.Event(LOGL_EVENT, "Script profiler is not yet informational\n");
            }
        }
		else
		{
			CScriptProfiler::CScriptProfilerFunction * pFun;
			CScriptProfiler::CScriptProfilerTrigger * pTrig;
            const long double average = (long double)g_profiler.total / g_profiler.called;

            char tmpstring[255];
            snprintf(tmpstring, sizeof(tmpstring),
				"Scripts: called %u times and took a total of %.4f seconds (%.4Lfs average). Reporting with highest average.\n",
                g_profiler.called,
                (g_profiler.total   / 1000.0),
                (average            / 1000.0));
            if (pSrc != this)
            {
                pSrc->SysMessage(tmpstring);
            }
            else
            {
                g_Log.Event(LOGL_EVENT, tmpstring);
            }
            if (ftDump != nullptr)
            {
                ftDump->Printf(tmpstring);
            }

			for ( pFun = g_profiler.FunctionsHead; pFun != nullptr; pFun = pFun->next )
			{
				if ( pFun->average > average )
				{
                    snprintf(tmpstring, sizeof(tmpstring),
						"FUNCTION '%s' called %u times, took %.4f seconds average (%.4f min, %.4f max), total: %.4f s.\n",
                        pFun->name,
                        pFun->called,
                        (pFun->average / 1000.0),
                        (pFun->min     / 1000.0),
                        (pFun->max     / 1000.0),
                        (pFun->total   / 1000.0));

                    if (pSrc != this)
                    {
                        pSrc->SysMessage(tmpstring);
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, tmpstring);
                    }
                    if (ftDump != nullptr)
                    {
                        ftDump->Printf(tmpstring);
                    }
				}
			}
			for ( pTrig = g_profiler.TriggersHead; pTrig != nullptr; pTrig = pTrig->next )
			{
				if ( pTrig->average > average )
				{
					snprintf(tmpstring, sizeof(tmpstring),
						"TRIGGER '%s' called %u times, took %.4f seconds average (%.4f min, %.4f max), total: %.4f s.\n",
						pTrig->name,
						pTrig->called,
						(pTrig->average / 1000.0),
						(pTrig->min     / 1000.0),
						(pTrig->max     / 1000.0),
						(pTrig->total   / 1000.0));
                    if (pSrc != this)
                    {
                        pSrc->SysMessage(tmpstring);
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, tmpstring);
                    }
                    if (ftDump != nullptr)
                    {
                        ftDump->Printf(tmpstring);
                    }
				}
			}

            if (pSrc != this)
            {
                pSrc->SysMessage("Report complete!\n");
            }
            else
            {
                g_Log.Event(LOGL_EVENT, "Report complete!\n");
            }
		}
	}
    else
    {
        if (pSrc != this)
        {
            pSrc->SysMessage("Script profiler feature is not enabled on Sphere.ini.\n");
        }
        else
        {
            g_Log.Event(LOGL_EVENT, "Script profiler feature is not enabled on Sphere.ini.\n");
        }
    }

	if ( ftDump != nullptr )
	{
		ftDump->Close();
		delete ftDump;
	}
}

// ---------------------------------------------------------------------

bool CServer::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CServer::r_GetRef");
	if ( IsDigit( ptcKey[0] ))
	{
		size_t i = 1;
		while ( IsDigit( ptcKey[i] ))
			i++;

		if ( ptcKey[i] == '.' )
		{
			size_t index = atoi( ptcKey );	// must use this to stop at .
			pRef = g_Cfg.Server_GetDef(index);
			ptcKey += i + 1;
			return true;
		}
	}
	if ( g_Cfg.r_GetRef( ptcKey, pRef ))
	{
		return true;
	}
	if ( g_World.r_GetRef( ptcKey, pRef ))
	{
		return true;
	}
	return( CScriptObj::r_GetRef( ptcKey, pRef ));
}

bool CServer::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CServer::r_LoadVal");

	if ( g_Cfg.r_LoadVal(s) )
		return true;
	if ( g_World.r_LoadVal(s) )
		return true;
	return CServerDef::r_LoadVal(s);
}

bool CServer::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
	ADDTOCALLSTACK("CServer::r_WriteVal");
	if ( !strnicmp(ptcKey, "ACCOUNT.", 8) )
	{
		ptcKey += 8;
		CAccount * pAccount = nullptr;

		// extract account name/index to a temporary buffer
		tchar * pszTemp = Str_GetTemp();
		tchar * pszTempStart = pszTemp;

		Str_CopyLimitNull(pszTemp, ptcKey, Str_TempLength());
		tchar * split = strchr(pszTemp, '.');
		if ( split != nullptr )
			*split = '\0';

		// adjust ptcKey to point to end of account name/index
		ptcKey += strlen(pszTemp);

		//	try to fetch using indexes
		if (( *pszTemp >= '0' ) && ( *pszTemp <= '9' ))
		{
			uint num = Exp_GetUVal(pszTemp);
			if (*pszTemp == '\0' && num < g_Accounts.Account_GetCount())
				pAccount = g_Accounts.Account_Get(num);
		}

		//	indexes failed, try to fetch using names
		if ( pAccount == nullptr )
		{
			pszTemp = pszTempStart;
			pAccount = g_Accounts.Account_Find(pszTemp);
		}

		if ( !*ptcKey) // we're just checking if the account exists
		{
			sVal.FormatVal( (pAccount ? 1 : 0) );
			return true;
		}
		else if ( pAccount ) // we're retrieving a property from the account
		{
			SKIP_SEPARATORS(ptcKey);
			return pAccount->r_WriteVal(ptcKey, sVal, pSrc);
		}
		// we're trying to retrieve a property from an invalid account
		return false;
	}
	else if (!strnicmp(ptcKey, "GMPAGE.", 7))
	{
		ptcKey += 7;
		size_t uiNum = Exp_GetSTVal(ptcKey);
		if (uiNum >= g_World.m_GMPages.size())
			return false;

		CGMPage* pGMPage = g_World.m_GMPages[uiNum].get();
		if (!pGMPage)
			return false;

		SKIP_SEPARATORS(ptcKey);
		return pGMPage->r_WriteVal(ptcKey, sVal, pSrc);
	}

	// Just do stats values for now.
    if (!fNoCallChildren)
    {
	    if ( g_Cfg.r_WriteVal(ptcKey, sVal, pSrc) )
		    return true;
	    if ( g_World.r_WriteVal(ptcKey, sVal, pSrc) )
		    return true;
    }
	return (fNoCallParent ? false : CServerDef::r_WriteVal(ptcKey, sVal, pSrc));
}

enum SV_TYPE
{
	SV_ACCOUNT,
	SV_ACCOUNTS, //read only
	SV_ALLCLIENTS,
	SV_B,
	SV_BLOCKIP,
    SV_CALCCRYPT,
	SV_CHARS, //read only
	SV_CLEARLISTS,
	SV_CONSOLE,
#ifdef _TEST_EXCEPTION
	SV_CRASH,
#endif
	SV_EXPORT,
	SV_GARBAGE,
	SV_GMPAGES,
	SV_HEARALL,
	SV_IMPORT,
	SV_INFORMATION,
	SV_ITEMS, //read only
	SV_LOAD,
	SV_LOG,
	SV_PRINTLISTS,
	SV_RESPAWN,
	SV_RESTOCK,
	SV_RESTORE,
	SV_RESYNC,
	SV_SAVE,
	SV_SAVECOUNT, //read only
	SV_SAVESTATICS,
	SV_SECURE,
	SV_SHRINKMEM,
	SV_SHUTDOWN,
	SV_TIME, // read only
	SV_UNBLOCKIP,
	SV_VARLIST,
	SV_QTY
};

lpctstr const CServer::sm_szVerbKeys[SV_QTY+1] =
{
	"ACCOUNT",
	"ACCOUNTS", // read only
	"ALLCLIENTS",
	"B",
	"BLOCKIP",
    "CALCCRYPT",
	"CHARS", // read only
	"CLEARLISTS",
	"CONSOLE",
#ifdef _TEST_EXCEPTION
	"CRASH",
#endif
	"EXPORT",
	"GARBAGE",
	"GMPAGES",
	"HEARALL",
	"IMPORT",
	"INFORMATION",
	"ITEMS", // read only
	"LOAD",
	"LOG",
	"PRINTLISTS",
	"RESPAWN",
	"RESTOCK",
	"RESTORE",
	"RESYNC",
	"SAVE",
	"SAVECOUNT", // read only
	"SAVESTATICS",
	"SECURE",
	"SHRINKMEM",
	"SHUTDOWN",
	"TIME", // read only
	"UNBLOCKIP",
	"VARLIST",
	nullptr
};

bool CServer::r_Verb( CScript &s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CServer::r_Verb");
	if ( !pSrc )
		return false;

	EXC_TRY("Verb");
	lpctstr ptcKey = s.GetKey();
	tchar *pszMsg = nullptr;

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, ARRAY_COUNT( sm_szVerbKeys )-1 );

	if ( index < 0 )
	{
        const size_t uiFunctionIndex = r_GetFunctionIndex(ptcKey);
        if (r_CanCall(uiFunctionIndex))
        {
            // RES_FUNCTION call
            CSString sVal;
            CScriptTriggerArgs Args( s.GetArgRaw() );
            if ( r_Call( uiFunctionIndex, pSrc, &Args, &sVal ) )
                return true;
        }

		if ( !strnicmp(ptcKey, "ACCOUNT.", 8) )
		{
			index = SV_ACCOUNT;
			ptcKey += 8;

			CAccount * pAccount = nullptr;
			char *pszTemp = Str_GetTemp();

			Str_CopyLimitNull(pszTemp, ptcKey, Str_TempLength());
			char *split = strchr(pszTemp, '.');
			if ( split )
			{
				*split = 0;
				pAccount = g_Accounts.Account_Find(pszTemp);
				ptcKey += strlen(pszTemp);
			}

			SKIP_SEPARATORS(ptcKey);
			if ( pAccount && *ptcKey )
			{
				CScript script(ptcKey, s.GetArgStr());
				script.CopyParseState(s);
				return pAccount->r_LoadVal(script);
			}
			return false;
		}
		else if (!strnicmp(ptcKey, "GMPAGE.", 7))
		{
			ptcKey += 7;
			size_t iNum = Exp_GetVal(ptcKey);
			if (iNum >= g_World.m_GMPages.size())
				return false;

			CGMPage* pGMPage = g_World.m_GMPages[iNum].get();
			if (!pGMPage)
				return false;

			SKIP_SEPARATORS(ptcKey);
			CScript script(ptcKey, s.GetArgStr());
			return pGMPage->r_LoadVal(script);
		}
		else if (!strnicmp(ptcKey, "CLEARVARS", 9))
		{
			ptcKey = s.GetArgStr();
			SKIP_SEPARATORS(ptcKey);
			g_Exp.m_VarGlobals.ClearKeys(ptcKey);
			return true;
		}
	}

	switch (index)
	{
		case SV_ACCOUNTS:
		case SV_CHARS:
		case SV_GMPAGES:
		case SV_ITEMS:
		case SV_SAVECOUNT:
		case SV_TIME:
			return false;
		case SV_ACCOUNT: // "ACCOUNT"
			return g_Accounts.Account_OnCmd(s.GetArgRaw(), pSrc);

		case SV_ALLCLIENTS:	// "ALLCLIENTS"
			{
				// Send a verb to all clients
				ClientIterator it;
				for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
				{
					if ( pClient->GetChar() == nullptr )
						continue;
					CScript script(s.GetArgStr());
					script.CopyParseState(s);
					pClient->GetChar()->r_Verb( script, pSrc );
				}
			}
			break;

		case SV_B: // "B"
			CWorldComm::Broadcast( s.GetArgStr());
			break;

		case SV_BLOCKIP:
			if ( pSrc->GetPrivLevel() >= PLEVEL_Admin )
			{
				int iTimeDecay(-1);

				tchar* ppArgs[2];
				if (Str_ParseCmds(s.GetArgRaw(), ppArgs, ARRAY_COUNT(ppArgs), ", ") == false)
					return false;

				if (ppArgs[1])
					iTimeDecay = Exp_GetVal(ppArgs[1]);

				HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(ppArgs[0]);

                if (iTimeDecay >= 0)
                {
                    if (pSrc != this)
                    {
                        pSrc->SysMessagef("IP blocked for %d seconds\n", iTimeDecay);
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "IP blocked for %d seconds\n", iTimeDecay);
                    }
                }
                else
                {
                    if (pSrc != this)
                    {
                        pSrc->SysMessagef("IP%s blocked\n", (history.m_fBlocked ? " already" : ""));
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "IP%s blocked\n", (history.m_fBlocked ? " already" : ""));
                    }
                }

				history.setBlocked(true, iTimeDecay);
			}
			break;

        case SV_CALCCRYPT:
            {
                tchar* ppArgs[3];
                const int iArgs = Str_ParseCmds(s.GetArgRaw(), ppArgs, ARRAY_COUNT(ppArgs), ", ");
                if (iArgs < 1)
                    return false;
                // 1st arg: client version string
                // 2nd arg (optional): client type (GAMECLIENT_TYPE enum)

                GAMECLIENT_TYPE cliType = (iArgs >= 2) ? (GAMECLIENT_TYPE)atoi(ppArgs[1]) : CLIENTTYPE_2D;
                if ((cliType < CLIENTTYPE_2D) || (cliType > CLIENTTYPE_EC))
                {
                    g_Log.EventError("Invalid client type, defaulting to 2D (Classic Client).\n");
                    cliType = CLIENTTYPE_2D;
                }

                ENCRYPTION_TYPE encTypeForce = (iArgs >= 3) ? (ENCRYPTION_TYPE)atoi(ppArgs[2]) : ENC_NONE;
                if ((encTypeForce < ENC_NONE) || (encTypeForce >= ENC_QTY))
                {
                    g_Log.EventError("Invalid encryption type, defaulting to 'autodetect'.\n");
                    encTypeForce = ENC_NONE;
                }

                const CUOClientVersion cliver_standard(ppArgs[0]);
                const CCryptoClientKey cckey = CCryptoKeyCalc::CalculateLoginKeys(cliver_standard, cliType, encTypeForce);

                const std::string cryptIniStr = CCryptoKeyCalc::FormattedLoginKey(cliver_standard, cliType, cckey);
                if (pSrc != this)
                    pSrc->SysMessagef(cryptIniStr.c_str());
                else
                    g_Log.Event(LOGL_EVENT, "%s\n", cryptIniStr.c_str());
            }
            break;
		case SV_CONSOLE:
			{
				CSString z = s.GetArgRaw();
				OnConsoleCmd(z, pSrc);
			}
			break;

#ifdef _TEST_EXCEPTION
		case SV_CRASH:
			{
				g_Log.Event(0, "Testing exceptions.\n");
				pSrc = nullptr;
				pSrc->GetChar();
			} break;
#endif

		case SV_EXPORT: // "EXPORT" name [chars] [area distance]
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
				return false;
			if ( s.HasArgs())
			{
				tchar * Arg_ppCmd[5];
				int Arg_Qty = Str_ParseCmds( s.GetArgRaw(), Arg_ppCmd, ARRAY_COUNT( Arg_ppCmd ) );
				if ( Arg_Qty <= 0 )
					break;
				// IMPFLAGS_ITEMS
				if ( ! g_World.Export( Arg_ppCmd[0], pSrc->GetChar(),
					(Arg_Qty >= 2) ? (word)atoi(Arg_ppCmd[1]) : (word)IMPFLAGS_ITEMS,
					(Arg_Qty >= 3) ? atoi(Arg_ppCmd[2]) : INT16_MAX ))
				{
                    if (pSrc != this)
                    {
                        pSrc->SysMessage("Export failed\n");
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "Export failed\n");
                    }
				}
			}
			else
			{
                if (pSrc != this)
                {
                    pSrc->SysMessage("EXPORT name [flags] [area distance]");
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "EXPORT name [flags] [area distance]");
                }
			}
			break;
		case SV_GARBAGE:
			if ( g_Serv.m_fResyncPause || g_World.IsSaving() )
			{
                if (pSrc != this)
                {
                    pSrc->SysMessage("Not allowed during world save and/or resync pause");
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "Not allowed during world save and/or resync pause");
                }
				break;
			}
			else
			{
				g_World.GarbageCollection();
			}
			break;

		case SV_HEARALL:	// "HEARALL" = Hear all said.
			{
				pszMsg = Str_GetTemp();
				g_Log.SetLogMask( s.GetArgFlag( g_Log.GetLogMask(), LOGM_PLAYER_SPEAK ));
				snprintf(pszMsg, Str_TempLength(), "Hear All %s.\n", g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) ? "Enabled" : "Disabled" );
			}
			break;

		case SV_INFORMATION:
            {
                if (pSrc != this)
                {
                    pSrc->SysMessage(GetStatusString(0x22));
                    pSrc->SysMessage(GetStatusString(0x24));
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "%s", GetStatusString(0x22));
                    g_Log.Event(LOGL_EVENT, "%s", GetStatusString(0x24));
                }
            }
			break;

		case SV_IMPORT: // "IMPORT" name [flags] [area distance]
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
				return false;
			if (s.HasArgs())
			{
				tchar * Arg_ppCmd[5];
				int Arg_Qty = Str_ParseCmds( s.GetArgRaw(), Arg_ppCmd, ARRAY_COUNT( Arg_ppCmd ));
				if ( Arg_Qty <= 0 )
				{
					break;
				}
				// IMPFLAGS_ITEMS
                if (!g_World.Import(Arg_ppCmd[0], pSrc->GetChar(),
                    (Arg_Qty >= 2) ? (word)(atoi(Arg_ppCmd[1])) : (word)IMPFLAGS_BOTH,
                    (Arg_Qty >= 3) ? atoi(Arg_ppCmd[2]) : INT16_MAX))
                {
                    if (pSrc != this)
                    {
                        pSrc->SysMessage("Import failed\n");
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "Import failed\n");
                    }
                }
			}
			else
			{
                if (pSrc != this)
                {
                    pSrc->SysMessage("IMPORT name [flags] [area distance]");
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "IMPORT name [flags] [area distance]");
                }
			}
			break;
		case SV_LOAD:
			// Load a resource file.
			if ( g_Cfg.LoadResourcesAdd( s.GetArgStr()) == nullptr )
				return false;
			return true;

		case SV_LOG:
			{
				lpctstr	pszArgs = s.GetArgStr();
                dword Args[] = { (dword)CTCOL_DEFAULT, (dword)LOGL_EVENT, (dword)0 };
                dword mask = Args[1];
                if (*pszArgs == '@')
                {
                    ++pszArgs;
                    if (*pszArgs == '@') // @@ = just a @ symbol
                        goto log_cont;

                    const char* c = pszArgs;
                    pszArgs = strchr(c, ' ');

                    if (!pszArgs)
                        return false;

                    for (int i = 0; (c < pszArgs) && (i < 3); )
                    {
                        if (*c == ',') // default value, skip it
                        {
                            ++i;
                            ++c;
                            continue;
                        }
                        Args[i] = (Exp_GetDWVal(c));
                        ++i;

                        if (*c == ',')
                            ++c;
                        else
                            break;	// no more args here!
                    }
                    ++pszArgs;
                    if (Args[0] > (dword)CTCOL_QTY /*|| Args[0] < (dword)0*/)
                        Args[0] = (dword)CTCOL_DEFAULT;
                    if (Args[1] > (dword)LOGL_QTY || Args[1] < (dword)1)
                        Args[1] = (dword)LOGL_EVENT;
                    if (Args[2] > (dword)LOGM_QTY)
                        Args[2] = (dword)0;
                    mask = Args[1] | Args[2];
                }
log_cont:
                g_Log.EventCustom((ConsoleTextColor)Args[0], mask, "%s\n", pszArgs);
			}
			break;

		case SV_RESPAWN:
			g_World.RespawnDeadNPCs();
			break;

		case SV_RESTOCK:
			// set restock time of all vendors in World.
			// set the respawn time of all spawns in World.
			g_World.Restock();
			break;

		case SV_RESTORE:	// "RESTORE" backupfile.SCP Account CharName
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
				return false;
			if (s.HasArgs())
			{
				tchar * Arg_ppCmd[4];
				size_t Arg_Qty = Str_ParseCmds( s.GetArgRaw(), Arg_ppCmd, ARRAY_COUNT( Arg_ppCmd ));
				if ( Arg_Qty <= 0 )
				{
					break;
				}
				if ( ! g_World.Import( Arg_ppCmd[0], pSrc->GetChar(),
                    IMPFLAGS_BOTH|IMPFLAGS_ACCOUNT, INT16_MAX,
                    0, 0,
                    Arg_ppCmd[1], Arg_ppCmd[2] )
                )
				{
                    if (pSrc != this)
                    {
                        pSrc->SysMessage("Restore failed\n");
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "Restore failed\n");
                    }
				}
				else
				{
                    if (pSrc != this)
                    {
                        pSrc->SysMessage("Restore success\n");
                    }
                    else
                    {
                        g_Log.Event(LOGL_EVENT, "Restore success\n");
                    }
				}
			}
			break;
		case SV_RESYNC:
			{
				// if a resync occurs whilst a script is executing then bad thing
				// will happen. Flag that a resync has been requested and it will
				// take place during a server tick
				m_fResyncRequested = pSrc;
				break;
			}
		case SV_SAVE: // "SAVE" x
			g_World.Save(s.GetArgVal() != 0);
			break;
		case SV_SAVESTATICS:
			g_World.SaveStatics();
			break;
		case SV_SECURE: // "SECURE"
			pszMsg = Str_GetTemp();
			g_Cfg.m_fSecure = s.GetArgFlag( g_Cfg.m_fSecure, true ) != 0;
			SetSignals();
			snprintf(pszMsg, Str_TempLength(), "Secure mode %s.\n", g_Cfg.m_fSecure ? "re-enabled" : "disabled" );
			break;
		case SV_SHRINKMEM:
			{
#ifdef _WIN32
				if ( Sphere_GetOSInfo()->dwPlatformId != 2 )
				{
					g_Log.EventError("Command not avaible on Windows 95/98/ME.\n");
					return false;
				}

				if ( !SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1)) )
				{
					g_Log.EventError("Error during process shrink.\n");
					return false;
				}

                if (pSrc != this)
                {
                    pSrc->SysMessage("Memory shrinked succesfully.\n");
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "Memory shrinked succesfully.\n");
                }
#else
				g_Log.EventError( "Command not avaible on *NIX os.\n" );
				return false;
#endif
			} break;

		case SV_SHUTDOWN: // "SHUTDOWN"
			Shutdown( s.HasArgs() ? s.GetArgVal() : 15 );
			break;

		case SV_UNBLOCKIP:	// "UNBLOCKIP"
			if (pSrc->GetPrivLevel() >= PLEVEL_Admin)
			{
				HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(s.GetArgRaw());
                if (pSrc != this)
                {
                    pSrc->SysMessagef("IP%s unblocked\n", (history.m_fBlocked ? "" : " already"));
                }
                else
                {
                    g_Log.Event(LOGL_EVENT, "IP%s unblocked\n", (history.m_fBlocked ? "" : " already"));
                }
				history.setBlocked(false);
			}
			break;

		case SV_VARLIST:
			if ( ! strcmpi( s.GetArgStr(), "log" ) )
				pSrc = &g_Serv;
			g_Exp.m_VarGlobals.DumpKeys(pSrc, "VAR.");
			break;
		case SV_PRINTLISTS:
			if ( ! strcmpi( s.GetArgStr(), "log" ) )
				pSrc = &g_Serv;
			g_Exp.m_ListGlobals.DumpKeys(pSrc, "LIST.");
			break;
		case SV_CLEARLISTS:
			g_Exp.m_ListGlobals.ClearKeys( s.GetArgStr()) ;
			break;
		default:
			return CScriptObj::r_Verb(s, pSrc);
	}

    if (pszMsg && *pszMsg)
    {
        if (pSrc != this)
        {
            pSrc->SysMessage(pszMsg);
        }
        else
        {
            g_Log.Event(LOGL_EVENT, pszMsg);
        }
    }

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	g_Log.EventDebug("source '%s' char '%s' uid '0%x'\n", (pSrc && pSrc->GetName()) ? pSrc->GetName() : "",
		(pSrc && pSrc->GetChar()) ? pSrc->GetChar()->GetName() : "",
		(pSrc && pSrc->GetChar()) ? (dword)pSrc->GetChar()->GetUID() : 0 );
	EXC_DEBUG_END;
	return false;
}

//*********************************************************

extern void defragSphere(char *);

bool CServer::CommandLine( int argc, tchar * argv[] )
{
    ADDTOCALLSTACK("CServer::CommandLine");
	// Console Command line.
	// This runs after script file enum but before loading the world file.
	// RETURN:
	//  true = keep running after this.

	for ( int argn = 1; argn < argc; ++argn )
	{
		tchar * pArg = argv[argn];
		if ( ! _IS_SWITCH(pArg[0]))
			continue;

		++pArg;

		switch ( toupper(pArg[0]) )
		{
            case 'H':
			case '?':
				PrintStr( SPHERE_TITLE " \n"
					"Command Line Switches:\n"
#ifdef _WIN32
					"-Cclassname Setup custom window class name for sphere (default: " SPHERE_TITLE ").\n"
#else
					"-C do not use colored console output (default: on).\n"
#endif
					"-D Dump global variable DEFNAMEs to defs.txt.\n"
#if defined(_WIN32) && !defined(_DEBUG)
					"-E Enable the CrashDumper.\n"
#endif
					"-Gpath/to/saves/ Defrags sphere saves.\n"
#ifdef _WIN32
					"-K install/remove Installs or removes NT Service.\n"
                    "-J automatically close the console window at server termination.\n"
#endif
					"-Nstring Set the sphere name.\n"
					"-P# Set the port number.\n"
					"-Ofilename Output console to this file name\n"
					"-Q Quit when finished.\n"
					);
				return false;
#ifdef _WIN32
			case 'C':
			case 'K':
				//	these are parsed in other places - nt service, nt window part, etc
				continue;
            case 'J':
                g_Serv._fCloseNTWindowOnTerminate = true;
                continue;
#else
			case 'C':
				g_UnixTerminal.setColorEnabled(false);
				continue;
#endif
			case 'P':
				m_ip.SetPort((word)(atoi(pArg + 1)));
				continue;
			case 'N':
				// Set the system name.
				SetName(pArg+1);
				continue;
			case 'D':
				// dump all the defines to a file.
				{
					CSFileText FileDefs;
					if ( ! FileDefs.Open( "defs.txt", OF_WRITE|OF_TEXT ))
						return false;

                    ssize_t count = ssize_t(g_Exp.m_VarDefs.GetCount());
                    ssize_t i = 0;
					for ( const CVarDefCont * pCont : g_Exp.m_VarDefs )
					{
						if ( ( i % 0x1ff ) == 0 )
							PrintPercent( i, count );

						if ( pCont != nullptr )
                            FileDefs.Printf( "%s=%s\n", pCont->GetKey(), pCont->GetValStr());
                        ++i;
					}

                    CSFileText FileResDefs;
                    if ( ! FileResDefs.Open( "resdefs.txt", OF_WRITE|OF_TEXT ))
                        return false;

                    count = ssize_t(g_Exp.m_VarResDefs.GetCount());
                    i = 0;
                    for ( const CVarDefCont * pCont : g_Exp.m_VarResDefs )
                    {
                        if ( ( i % 0x1ff ) == 0 )
                            PrintPercent( i, count );

                        if ( pCont != nullptr )
                            FileResDefs.Printf( "%s=%s\n", pCont->GetKey(), pCont->GetValStr());
                        ++i;
            }
				}
				continue;
#if defined(_WIN32) && !defined(_DEBUG) && defined(WINDOWS_GENERATE_CRASHDUMP)
			case 'E':
				CrashDump::Enable();
				if (CrashDump::IsEnabled())
					PrintStr("Crash dump enabled\n");
				else
					PrintStr("Crash dump NOT enabled.\n");
				continue;
#endif
			case 'G':
				defragSphere(pArg + 1);
				continue;
			case 'O':
				if ( g_Log.Open(pArg+1, OF_SHARE_DENY_WRITE|OF_READWRITE|OF_TEXT) )
					g_Log.m_fLockOpen = true;
				continue;
			case 'Q':
				return false;
			default:
				g_Log.Event(LOGM_INIT|LOGL_CRIT, "Can't recognize command line data '%s'\n", static_cast<lpctstr>(argv[argn]));
				break;
		}
	}

	return true;
}

void CServer::SetResyncPause(bool fPause, CTextConsole * pSrc, bool fMessage)
{
	ADDTOCALLSTACK("CServer::SetResyncPause");
	if ( fPause )
	{
		m_fResyncPause = true;
        g_Log.Event(LOGL_EVENT, "%s\n", g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_START));

		if ( fMessage )
			CWorldComm::Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_START));
		else if ( pSrc && pSrc->GetChar() )
			pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_START));

		g_Cfg.Unload(true);
		SetServerMode(SERVMODE_ResyncPause);
	}
	else
	{
        g_Log.Event(LOGL_EVENT, "%s\n", g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_RESTART));
		SetServerMode(SERVMODE_ResyncLoad);

		if ( !g_Cfg.Load(true) )
		{
            g_Log.EventError("%s\n", g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_FAILED));
			if ( fMessage )
				CWorldComm::Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_FAILED));
			else if ( pSrc && pSrc->GetChar() )
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_FAILED));
		}
		else
		{
            g_Log.Event(LOGL_EVENT, "%s\n", g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_SUCCESS));
			if ( fMessage )
				CWorldComm::Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_SUCCESS));
			else if ( pSrc && pSrc->GetChar() )
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_RESYNC_SUCCESS));
		}

		m_fResyncPause = false;

		g_World.SyncGameTime();

		SetServerMode(SERVMODE_Run);
	}
}

//*********************************************************

bool CServer::SocketsInit( CSocket & socket )
{
	ADDTOCALLSTACK("CServer::SocketsInit");
	// Initialize socket
	if ( !socket.Create() )
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to create socket!\n");
		return false;
	}

	linger lval;
	lval.l_onoff = 0;
	lval.l_linger = 10;
	if ((0 != socket.SetSockOpt(SO_LINGER, reinterpret_cast<const char*>(&lval), sizeof(lval))) ||
		(0 != socket.SetNonBlocking()))
	{
		g_Log.Event(LOGL_FATAL | LOGM_INIT, "Unable to initialize socket!\n");
		return false;
	}

#ifndef _WIN32
	int onNotOff = 1;
	if(socket.SetSockOpt(SO_REUSEADDR, (char *)&onNotOff , sizeof(onNotOff )) == -1) {
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to set SO_REUSEADDR!\n");
	}
#endif
	// Bind to just one specific port if they say so.
	CSocketAddress SockAddr = m_ip;
	// if ( fGod )
	//	SockAddr.SetPort(( g_Cfg.m_iUseGodPort > 1 ) ? g_Cfg.m_iUseGodPort : m_ip.GetPort()+1000);

	int iRet = socket.Bind(SockAddr);
	if ( iRet < 0 )			// Probably already a server running.
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Unable to bind listen socket %s port %d (error code: %i)\n", SockAddr.GetAddrStr(), SockAddr.GetPort(), iRet);
		return false;
	}
	socket.Listen();

#ifdef _LIBEV
#ifdef LIBEV_REGISTERMAIN
	if ( g_Cfg.m_fUseAsyncNetwork != 0 )
		g_NetworkEvent.registerMainsocket();
#endif
#endif

	return true;
}

bool CServer::SocketsInit() // Initialize sockets
{
	ADDTOCALLSTACK("CServer::SocketsInit");
	if ( !SocketsInit(m_SocketMain) )
		return false;

	// What are we listing our port as to the world.
	// Tell the admin what we know.

	tchar szName[ SPHERE_MAX_PATH ];
	struct hostent * pHost = nullptr;

	int iRet = gethostname(szName, sizeof(szName));
	if ( iRet )
		Str_CopyLimitNull(szName, m_ip.GetAddrStr(), sizeof(szName));
	else
	{
		pHost = gethostbyname(szName);
		if ( pHost && pHost->h_addr && pHost->h_name && pHost->h_name[0] )
			Str_CopyLimitNull(szName, pHost->h_name, SPHERE_MAX_PATH);
	}

	g_Log.Event( LOGM_INIT, "Server started on hostname '%s'\n", szName);
	if ( !iRet && pHost && pHost->h_addr )
	{
		for ( size_t i = 0; pHost->h_addr_list[i] != nullptr; i++ )
		{
			CSocketAddressIP ip;
			ip.SetAddrIP(*((dword*)(pHost->h_addr_list[i]))); // 0.1.2.3
			if ( !m_ip.IsLocalAddr() && !m_ip.IsSameIP(ip) )
				continue;
			g_Log.Event(LOGM_INIT, "Monitoring IP %s:%d\n", ip.GetAddrStr(), m_ip.GetPort());
		}
	}
	return true;
}

void CServer::SocketsClose()
{
	ADDTOCALLSTACK("CServer::SocketsClose");
#ifdef _LIBEV
#ifdef LIBEV_REGISTERMAIN
	if ( g_Cfg.m_fUseAsyncNetwork != 0 )
		g_NetworkEvent.unregisterMainsocket();
#endif
#endif
	m_SocketMain.Close();
}

void CServer::_OnTick()
{
	ADDTOCALLSTACK("CServer::_OnTick");
	EXC_TRY("Tick");

#ifndef _WIN32
    // This happens on T_Main, and not on T_UnixTerm.
    if (g_UnixTerminal.isReady())
	{
		tchar c = g_UnixTerminal.read();
		if ( OnConsoleKey(m_sConsoleText, c, false) == 2 )
			m_fConsoleTextReadyFlag = true;
	}
#endif

	EXC_SET_BLOCK("ConsoleInput");
	if ( m_fConsoleTextReadyFlag )
	{
		EXC_SET_BLOCK("console input");
		CSString sText = m_sConsoleText;	// make a copy.
		m_sConsoleText.Clear();				// done using this.
		m_fConsoleTextReadyFlag = false;	// ready to use again
		OnConsoleCmd( sText, this );
	}

	EXC_SET_BLOCK("ResyncCommand");
	if ( m_fResyncRequested != nullptr )
	{
		if ( !m_fResyncPause )
		{
			SetResyncPause(true, m_fResyncRequested);
			SetResyncPause(false, m_fResyncRequested);
		}
		else
            SetResyncPause(false, m_fResyncRequested, true);
		m_fResyncRequested = nullptr;
	}

	EXC_SET_BLOCK("SetTime");
	SetValidTime();	// we are a valid game server.

	const ProfileTask overheadTask(PROFILE_OVERHEAD);

	if ( m_timeShutdown > 0 )
	{
		EXC_SET_BLOCK("shutdown");
		if ( CWorldGameTime::GetCurrentTime().GetTimeDiff(m_timeShutdown) > 0 )
			SetExitFlag(2);
	}

	EXC_SET_BLOCK("generic");
	g_Cfg._OnTick(false);
	_hDb._OnTick();
	EXC_CATCH;
}

bool CServer::Load()
{
	EXC_TRY("Load");
    ADDTOCALLSTACK("CServer::Load");

	EXC_SET_BLOCK("print sphere infos");
	g_Log.Event(LOGM_INIT, "%s.\n", g_sServerDescription.c_str());
#ifdef __GITREVISION__
	g_Log.Event(LOGM_INIT, "Compiled at %s (%s) [branch %s / build %d / GIT hash %s]\n\n", __DATE__, __TIME__, __GITBRANCH__, __GITREVISION__, __GITHASH__);
#else
	g_Log.Event(LOGM_INIT, "Compiled at %s (%s)\n\n", __DATE__, __TIME__);
#endif
#ifdef _NIGHTLYBUILD
	static lpctstr pszNightlyMsg = "\r\n"
		"-----------------------------------------------------------------\r\n"
		"This is a nightly build of SphereServer. This build is to be used for testing and/or bug reporting ONLY.\r\n"
		"DO NOT run this build on a live shard unless you know what you are doing!\r\n"
		"Nightly builds are automatically made at every commit to the source code repository and might\r\n"
		"contain errors, might be unstable or even destroy your shard as they are mostly untested!\r\n"
		"-----------------------------------------------------------------\r\n\r\n";

	g_Log.Event(LOGL_WARN|LOGF_CONSOLE_ONLY, pszNightlyMsg);
#endif

#ifdef _WIN32
	EXC_SET_BLOCK("init winsock");
	tchar * wSockInfo = Str_GetTemp();
	if ( !m_SocketMain.IsOpen() )
	{
		WSADATA wsaData{};
		int err = WSAStartup(MAKEWORD(2,2), &wsaData);
		if ( err )
		{
			if ( err == WSAVERNOTSUPPORTED )
			{
				err = WSAStartup(MAKEWORD(1,1), &wsaData);
				if ( err )
					goto nowinsock;
			}
			else
			{
nowinsock:		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Winsock 1.1 not found!\n");
				return false;
			}
			snprintf(wSockInfo, Str_TempLength(), "Using WinSock ver %d.%d (%s)\n", HIBYTE(wsaData.wVersion), LOBYTE(wsaData.wVersion), wsaData.szDescription);
		}
	}
#endif

	EXC_SET_BLOCK("loading ini");
	if (!g_Cfg.LoadIni(false))
		return false;

#ifdef _NIGHTLYBUILD
	g_Log.Event(LOGL_WARN|LOGF_LOGFILE_ONLY, pszNightlyMsg);
	if (!g_Cfg.m_fAgree)
	{
		g_Log.Event(LOGL_ERROR,"Please write AGREE=1 in Sphere.ini file to acknowledge that\nyou understand the risks of using nightly builds.\n");
		return false;
	}
#endif

#ifdef _WIN32
	if (wSockInfo[0])
		g_Log.Event(LOGM_INIT, wSockInfo);
#endif

	if (g_Cfg.m_fMySql && g_Cfg.m_fMySqlTicks)
	{
		EXC_SET_BLOCK( "Connecting to MySQL server" );
		if (_hDb.Connect())
			g_Log.Event( LOGM_NOCONTEXT, "MySQL connected to server: '%s'.\n", g_Cfg.m_sMySqlHost.GetBuffer() );
		else
		{
			g_Log.EventError( "Can't connect to MySQL DataBase. Check your ini settings or if the server is working.\n" );
			return false;
		}
	}

    EXC_SET_BLOCK( "Initializing in-memory SQLite database" );
    _hMdb.Open(":memory:");

	EXC_SET_BLOCK("setting signals");
	SetSignals();

    if (g_Cfg.m_iAutoProcessPriority != 0)
    {
        EXC_SET_BLOCK("setting process priority");
        bool fPrioritySuccess = SetProcessPriority(g_Cfg.m_iAutoProcessPriority);
        g_Log.Event(LOGM_INIT, "Setting process priority... %s.\n", fPrioritySuccess ? "Success" : "Failed");
    }

	EXC_SET_BLOCK("init world cache");
	g_World._Cache.Init();

	EXC_SET_BLOCK("loading scripts");
	TriglistInit();
	if ( !g_Cfg.Load(false) )
		return false;

	EXC_SET_BLOCK("init encryption");
	if ( m_ClientVersion.GetClientVerNumber() )
	{
        std::string cliverstr = m_ClientVersion.GetClientVer();
		g_Log.Event(LOGM_INIT, "ClientVersion=%s\n", cliverstr.c_str());
		if ( !m_ClientVersion.IsValid() )
		{
			g_Log.Event(LOGL_FATAL|LOGM_INIT, "Bad Client Version '%s'\n", cliverstr.c_str());
			return false;
		}
	}

#ifdef _WIN32
    EXC_SET_BLOCK("finalizing");
    g_NTWindow.SetWindowTitle();
#endif

	return true;
	EXC_CATCH;
	return false;
}
