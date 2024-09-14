#ifdef _WIN32
	#include "../sphere/ntservice.h"	// g_Service
    #include "../sphere/ntwindow.h"
    #include <process.h>				// getpid()
#else
	#include "../sphere/UnixTerminal.h"
#endif

#if defined(_WIN32) && !defined(pid_t)
    //#include <timeapi.h>    // for timeBeginPeriod
	#define pid_t int
#endif

#ifdef _LIBEV
	#include "../network/linuxev.h"
#endif

#include "../common/sphere_library/CSRand.h"
#include "../common/CLog.h"
#include "../common/CException.h"
#include "../common/CExpression.h"
#include "../common/CUOInstall.h"
#include "../common/sphereversion.h"
#include "../network/CNetworkManager.h"
#include "../network/PingServer.h"
#include "../sphere/asyncdb.h"
#include "clients/CAccount.h"
#include "CObjBase.h"
#include "CScriptProfiler.h"
#include "CServer.h"
#include "CWorld.h"
#include "spheresvr.h"
#include <sstream>
#include <cstdlib>


// Dynamic allocation of some global stuff
std::string g_sServerDescription;

// Dynamic initialization of some static members of other classes, which are used very soon after the server starts
dword CObjBase::sm_iCount = 0;				// UID table.
#ifdef _WIN32
llong CSTime::_kllTimeProfileFrequency = 1; // Default value.
#endif


// This method MUST be the first code running when the application starts!
GlobalInitializer::GlobalInitializer()
{
	// The order of the instructions is important!

	std::stringstream ssServerDescription;
	ssServerDescription << SPHERE_TITLE << " Version " << SPHERE_BUILD_INFO_STR;
	ssServerDescription << " [" << get_target_os_str() << '-' << get_target_arch_str() << "]";
	ssServerDescription << " by www.spherecommunity.net";
	g_sServerDescription = ssServerDescription.str();

//-- Time

/*
#ifdef _WIN32
    // Ensure it's ACTUALLY necessary, before resorting to this.
    timeBeginPeriod(1); // from timeapi.h, we need also to link against Winmm.lib...
#endif
*/
    PeriodicSyncTimeConstants();

//--- Sphere threading system

	DummySphereThread::createInstance();

//--- Exception handling

    // Set exception catcher?
#if defined(_WIN32) && defined(_MSC_VER) && !defined(_NIGHTLYBUILD)
    // We don't need an exception translator for the Debug build, since that build would, generally, be used with a debugger.
    // We don't want that for Release build either because, in order to call _set_se_translator, we should set the /EHa
    //	compiler flag, which slows down code a bit.
    SetExceptionTranslator();
#endif

	// Set function to handle the invalid case where a pure virtual function is called.
    SetPurecallHandler();

//--- Pre-startup sanity checks

	constexpr const char* m_sClassName = "GlobalInitializer";
	EXC_TRY("Pre-startup Init");

	static_assert(MAX_BUFFER >= sizeof(CCommand));
	static_assert(MAX_BUFFER >= sizeof(CEvent));
	static_assert(sizeof(int) == sizeof(dword));	// make this assumption often.
	static_assert(sizeof(ITEMID_TYPE) == sizeof(dword));
	static_assert(sizeof(word) == 2);
	static_assert(sizeof(dword) == 4);
	static_assert(sizeof(nword) == 2);
	static_assert(sizeof(ndword) == 4);
	static_assert(sizeof(wchar) == 2);	// 16 bits
	static_assert(sizeof(CUOItemTypeRec) == 37);	// is byte packing working ?

	EXC_CATCH;
}

void GlobalInitializer::InitRuntimeDefaultValues() // static
{
	CPointBase::InitRuntimeDefaultValues();
}

void GlobalInitializer::PeriodicSyncTimeConstants() // static
{
    // TODO: actually call it periodically!

#ifdef _WIN32
    // Needed to get precise system time.
    LARGE_INTEGER liProfFreq;
    if (QueryPerformanceFrequency(&liProfFreq))
    {
        CSTime::_kllTimeProfileFrequency = liProfFreq.QuadPart;
    }
#endif  // _WIN32
}


/* Start global declarations */

static GlobalInitializer g_GlobalInitializer;

#ifdef _WIN32
CNTWindow g_NTWindow;
#else
UnixTerminal g_UnixTerminal;
#endif

#ifdef _LIBEV
// libev is used by Linux to notify when our main socket is readable or writable, so when i can read and send data again (async I/O).
// Windows supports async network I/O via WinSock.
LinuxEv g_NetworkEvent;
#endif

CLog			g_Log;

// Config data from sphere.ini is needed from the beginning.
CServerConfig   g_Cfg;
CServer         g_Serv;   // current state, stuff not saved.
CAccounts       g_Accounts;			// All the player accounts. name sorted CAccount

// Game servers stuff.
CWorld			g_World;			// the world. (we save this stuff)

// Networking stuff. They are declared here (in the same file of the other global declarations) to control the order of construction
//  and destruction of these classes. If this order is altered, you'll get segmentation faults (access violations) when the server is closing!
#ifdef _LIBEV
	extern LinuxEv g_NetworkEvent;
#endif
	CNetworkManager g_NetworkManager;


// Again, game servers stuff.
CUOInstall		g_Install;
CVerDataMul		g_VerData;
CSRand          g_Rand;
CExpression		g_Exp;				// Global script variables.
CSStringList	g_AutoComplete;		// auto-complete list
CScriptProfiler g_profiler;			// script profiler
CUOMapList		g_MapList;			// global maps information

MainThread g_Main;
extern PingServer g_PingServer;
extern CDataBaseAsyncHelper g_asyncHdb;



//*******************************************************************
//	Main server loop

MainThread::MainThread()
	: AbstractSphereThread("T_Main", IThread::RealTime)
{
    m_profile.EnableProfile(PROFILE_NETWORK_RX);
    m_profile.EnableProfile(PROFILE_CLIENTS);
    //m_profile.EnableProfile(PROFILE_NETWORK_TX);
    m_profile.EnableProfile(PROFILE_CHARS);
    m_profile.EnableProfile(PROFILE_ITEMS);
    m_profile.EnableProfile(PROFILE_MAP);
    m_profile.EnableProfile(PROFILE_MULTIS);
    m_profile.EnableProfile(PROFILE_NPC_AI);
    m_profile.EnableProfile(PROFILE_SCRIPTS);
    m_profile.EnableProfile(PROFILE_SHIPS);
    m_profile.EnableProfile(PROFILE_TIMEDFUNCTIONS);
    m_profile.EnableProfile(PROFILE_TIMERS);
}

void MainThread::onStart()
{
	AbstractSphereThread::onStart();
	SetExceptionTranslator();
}

void MainThread::tick()
{
	Sphere_OnTick();
}

bool MainThread::shouldExit() noexcept
{
	if (g_Serv.GetExitFlag() != 0)
		return true;
	return AbstractSphereThread::shouldExit();
}


//*******************************************************************

static bool WritePidFile(int iMode = 0)
{
	lpctstr	file = SPHERE_FILE ".pid";
	FILE* pidFile;

	if (iMode == 1)		// delete
	{
		return (STDFUNC_UNLINK(file) == 0);
	}
	else if (iMode == 2)	// check for .pid file
	{
		pidFile = fopen(file, "r");
		if (pidFile)
		{
			g_Log.Event(LOGM_INIT, SPHERE_FILE ".pid already exists. Secondary launch or unclean shutdown?\n");
			fclose(pidFile);
		}
		return true;
	}
	else
	{
		pidFile = fopen(file, "w");
		if (pidFile)
		{
			pid_t spherepid = STDFUNC_GETPID();
			fprintf(pidFile, "%d\n", spherepid);
			fclose(pidFile);
			return true;
		}
		g_Log.Event(LOGM_INIT, "Cannot create pid file!\n");
		return false;
	}
}


int Sphere_InitServer( int argc, char *argv[] )
{
	constexpr const char *m_sClassName = "SphereInit";
	EXC_TRY("Init Server");
    GlobalInitializer::InitRuntimeDefaultValues();

    EXC_SET_BLOCK("loading ini and scripts");
	if ( !g_Serv.Load() )
		return -3;

	if ( argc > 1 )
	{
		EXC_SET_BLOCK("cmdline");
		if ( !g_Serv.CommandLine(argc, argv) )
			return -1;
	}

	WritePidFile(2);

	EXC_SET_BLOCK("sockets init");
	if ( !g_Serv.SocketsInit() )
		return -9;
	EXC_SET_BLOCK("load world");
	if ( !g_World.LoadAll() )
		return -8;

	//	load auto-complete dictionary
	EXC_SET_BLOCK("auto-complete");
	{
		CSFileText dict;
		if ( dict.Open(SPHERE_FILE ".dic", OF_READ|OF_TEXT|OF_DEFAULTMODE) )
		{
			tchar * pszTemp = Str_GetTemp();
			size_t count = 0;
			while ( !dict.IsEOF() )
			{
				dict.ReadString(pszTemp, SCRIPT_MAX_LINE_LEN-1);
				if ( *pszTemp )
				{
					tchar *c = strchr(pszTemp, '\r');
					if ( c != nullptr )
						*c = '\0';

					c = strchr(pszTemp, '\n');
					if ( c != nullptr )
						*c = '\0';

					if ( *pszTemp != '\0' )
					{
						++count;
						g_AutoComplete.AddTail(pszTemp);
					}
				}
			}
			g_Log.Event(LOGM_INIT, "Auto-complete dictionary loaded (contains %" PRIuSIZE_T " words).\n", count);
			dict.Close();
		}
	}

	// Display EF/OF Flags
	g_Cfg.PrintEFOFFlags();

	EXC_SET_BLOCK("finalizing");
	g_Serv.SetServerMode(SERVMODE_Run);	// ready to go.

	g_Log.Event(LOGM_INIT, "%s", g_Serv.GetStatusString(0x24));
	g_Log.Event(LOGM_INIT, "\nStartup complete (items=%" PRIuSIZE_T ", chars=%" PRIuSIZE_T ", Accounts = %" PRIuSIZE_T ")\n", g_Serv.StatGet(SERV_STAT_ITEMS), g_Serv.StatGet(SERV_STAT_CHARS), g_Serv.StatGet(SERV_STAT_ACCOUNTS));

#ifdef _WIN32
	g_Log.Event(LOGM_INIT, "Use '?' to view available console commands\n\n");
#else
	g_Log.Event(LOGL_EVENT, "Use '?' to view available console commands or Ctrl+C to exit\n\n");
#endif

	if (!g_Accounts.Account_GetCount())
		g_Log.Event(LOGL_WARN, "The server has no accounts. To create admin account use:\n  ACCOUNT ADD [login] [password]\n  ACCOUNT [login] PLEVEL 7\n\n");


	// Trigger server start
	g_Serv.r_Call("f_onserver_start", &g_Serv, nullptr);
	return g_Serv.GetExitFlag();

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("cmdline argc=%d starting with %p (argv1='%s')\n", argc, static_cast<void *>(argv), ( argc > 2 ) ? argv[1] : "");
	EXC_DEBUG_END;
	return -10;
}

void Sphere_ExitServer()
{
	// Trigger server quit
	g_Serv.r_Call("f_onserver_exit", &g_Serv, nullptr);

	g_Serv.SetServerMode(SERVMODE_Exiting);

	g_NetworkManager.stop();
	g_Main.waitForClose();
	g_PingServer.waitForClose();
	g_asyncHdb.waitForClose();
#ifdef _LIBEV
	if ( g_Cfg.m_fUseAsyncNetwork != 0 )
		g_NetworkEvent.waitForClose();
#endif

	g_Serv.SocketsClose();
	g_World.Close();

	lpctstr ptcReason;
	int iExitFlag = g_Serv.GetExitFlag();
	switch ( iExitFlag )
	{
		case -10:	ptcReason = "Unexpected error occurred";			break;
		case -9:	ptcReason = "Failed to bind server IP/port";		break;
		case -8:	ptcReason = "Failed to load worldsave files";		break;
		case -3:	ptcReason = "Failed to load server settings";		break;
		case -1:	ptcReason = "Shutdown via commandline";			    break;
#ifdef _WIN32
		case 1:		ptcReason = "X command on console";				    break;
#else
		case 1:		ptcReason = "Terminal closed by SIGHUP signal";	    break;
#endif
		case 2:		ptcReason = "SHUTDOWN command executed";			break;
		case 4:		ptcReason = "Service shutdown";					    break;
		case 5:		ptcReason = "Console window closed";				break;
		case 6:		ptcReason = "Proccess aborted by SIGABRT signal";	break;
		default:	ptcReason = "Server shutdown complete";			    break;
	}

	g_Log.Event(LOGM_INIT|LOGL_FATAL, "Server terminated: %s (code %d)\n", ptcReason, iExitFlag);
#ifdef _WIN32
    if (!g_Serv._fCloseNTWindowOnTerminate)
        g_Log.Event(LOGM_INIT | LOGF_CONSOLE_ONLY, "You can now close this window.\n");
#endif

    g_Log.Close();
    ThreadHolder::get().markThreadsClosing();

#ifdef _WIN32
    if (iExitFlag != 5)
        g_NTWindow.NTWindow_ExitServer();
#endif
}


int Sphere_OnTick()
{
	// Give the world (CMainTask) a single tick. RETURN: 0 = everything is fine.
	constexpr const char *m_sClassName = "SphereTick";
	EXC_TRY("Tick");
#ifdef _WIN32
	EXC_SET_BLOCK("service");
    g_NTService._OnTick();
#endif

	EXC_SET_BLOCK("world");
	g_World._OnTick();

	// process incoming data
	EXC_SET_BLOCK("network-in");
	g_NetworkManager.processAllInput();

	EXC_SET_BLOCK("server");
	g_Serv._OnTick();

	// push outgoing data
	EXC_SET_BLOCK("network-out");
	g_NetworkManager.processAllOutput();

	// don't put the network-tick between in.tick and out.tick, otherwise it will clean the out queue!
	EXC_SET_BLOCK("network-tick");
	g_NetworkManager.tick();	// then this thread has to call the network tick

	EXC_CATCH;
	return g_Serv.GetExitFlag();
}


//*****************************************************

static void Sphere_MainMonitorLoop()
{
	constexpr const char *m_sClassName = "SphereMonitor";
	// Just make sure the main loop is alive every so often.
	// This should be the parent thread. try to restart it if it is not.
	while ( !g_Serv.GetExitFlag() )
	{
		EXC_TRY("MainMonitorLoop");

		if ( g_Cfg.m_iFreezeRestartTime <= 0 )
		{
			DEBUG_ERR(("Freeze Restart Time cannot be cleared at run time\n"));
			g_Cfg.m_iFreezeRestartTime = 10;
		}

		EXC_SET_BLOCK("Sleep");
		// only sleep 1 second at a time, to avoid getting stuck here when closing
		// down with large m_iFreezeRestartTime values set
		for (int i = 0; i < g_Cfg.m_iFreezeRestartTime; ++i)
		{
			if ( g_Serv.GetExitFlag() )
				break;

			Sleep(1000);
		}

		EXC_SET_BLOCK("Checks");
		// Don't look for freezing when doing certain things.
		if ( g_Serv.IsLoading() || ! g_Cfg.m_fSecure || g_Serv.IsValidBusy() )
			continue;

		EXC_SET_BLOCK("Check Stuck");
#ifndef _DEBUG
		if (g_Main.checkStuck() == true)
			g_Log.Event(LOGL_CRIT, "'%s' thread hang, restarting...\n", g_Main.getName());
#endif
		EXC_CATCH;
	}

}


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

void defragSphere(char *path)
{
	ASSERT(path != nullptr);

    	constexpr size_t mb10 = 10*1024*1024;
	constexpr size_t mb5  = 5*1024*1024;

	CSFileText inf;
	CSFile ouf;
    char file_buf[1024];
	char path_buf[_MAX_PATH], path_buf_2[_MAX_PATH];
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
		if ( i == 0 )		strcat(path_buf, SPHERE_FILE "statics" SPHERE_SCRIPT);
		else if ( i == 1 )	strcat(path_buf, SPHERE_FILE "world" SPHERE_SCRIPT);
		else				strcat(path_buf, SPHERE_FILE "chars" SPHERE_SCRIPT);

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
		else if ( i == 1 )	strcat(path_buf, SPHERE_FILE "chars" SPHERE_SCRIPT);
		else if ( i == 2 )	strcat(path_buf, SPHERE_FILE "data" SPHERE_SCRIPT);
		else if ( i == 3 )	strcat(path_buf, SPHERE_FILE "world" SPHERE_SCRIPT);
		else if ( i == 4 )	strcat(path_buf, SPHERE_FILE "statics" SPHERE_SCRIPT);
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


void atexit_handler()
{
	ThreadHolder::get().markThreadsClosing();
}


#ifdef _WIN32
int Sphere_MainEntryPoint( int argc, char *argv[] )
#else
int _cdecl main( int argc, char * argv[] )
#endif
{
	static constexpr lpctstr m_sClassName = "main";
	EXC_TRY("MAIN");

    	const int atexit_handler_result = std::atexit(atexit_handler); // Handler will be called
	if (atexit_handler_result != 0)
	{
		g_Log.Event(LOGL_CRIT, "atexit handler registration failed.\n");
		goto exit_server;
	}

	{
        // Ensure i have this to have context for ADDTOCALLSTACK and other operations.
        const IThread* curthread = ThreadHolder::get().current();
        ASSERT(curthread != nullptr);
        ASSERT(dynamic_cast<DummySphereThread const *>(curthread));
        (void)curthread;
    }

#ifndef _WIN32
    IThread::setThreadName("T_SphereStartup");

    g_UnixTerminal.start();

    // We need to find out the log files folder... look it up in the .ini file (on Windows it's done in WinMain function).
    g_Serv.SetServerMode(SERVMODE_PreLoadingINI);
    g_Cfg.LoadIni(false);
#endif

    g_Serv.SetServerMode(SERVMODE_Loading);
	g_Serv.SetExitFlag( Sphere_InitServer( argc, argv ));
	if ( ! g_Serv.GetExitFlag() )
	{
		WritePidFile();

		// Start the ping server, this can only be ran in a separate thread
		if ( IsSetEF( EF_UsePingServer ) )
			g_PingServer.start();

#ifdef _LIBEV
		if ( g_Cfg.m_fUseAsyncNetwork != 0 )
			g_NetworkEvent.start();
#endif

        // CNetworkManager creates a number of CNetworkThread classes, which may run on new threads or on the calling thread. Every CNetworkThread has
        //  an instance of CNetworkInput nad CNetworkOutput, which support working in a multi threaded way (declarations and definitions in network_multithreaded.h/.cpp)
		g_NetworkManager.start();

		const bool fShouldCoreRunInSeparateThread = ( g_Cfg.m_iFreezeRestartTime > 0 );
		if (fShouldCoreRunInSeparateThread)
		{
			g_Main.start();				// Starts another thread to do all the work (it does Sphere_OnTick())
			IThread::setThreadName("T_Monitor");
			Sphere_MainMonitorLoop();	// Use this thread to monitor if the others are stuck
		}
		else
		{
			while ( !g_Serv.GetExitFlag() )
			{
				g_Main.tick();			// Use this thread to do all the work, without monitoring the other threads state
			}
		}
	}

exit_server:
	Sphere_ExitServer();
	WritePidFile(1);

#ifdef _WIN32
    if (!g_Serv._fCloseNTWindowOnTerminate)
    {
        while (g_NTWindow.isActive())
        {
            Sleep (100);
        }
    }
#endif

	return g_Serv.GetExitFlag();
	EXC_CATCH;

	return -1;
}


#include "../tables/classnames.tbl"
