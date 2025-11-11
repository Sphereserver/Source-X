#ifdef _WIN32
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

#include "../common/CLog.h"
#include "../common/CScriptParserBufs.h"
//#include "../common/CException.h" // included in the precompiled header
//#include "../common/CExpression.h" // included in the precompiled header
#include "../common/CUOInstall.h"
#include "../network/CNetworkManager.h"
#include "../network/PingServer.h"
#include "../sphere/asyncdb.h"
#include "../sphere/GlobalInitializer.h"
#include "../sphere/StartupMonitorThread.h"
#include "../sphere/MainThread.h"
#include "clients/CAccount.h"
#include "CScriptProfiler.h"
#include "CServer.h"
#include "CWorld.h"
#include "spheresvr.h"
#include <cstdlib>

#ifdef UNIT_TESTING
#   define DOCTEST_CONFIG_IMPLEMENT
#   include <doctest/doctest.h>
#endif

/* Start global declarations */

DummySphereThread* DummySphereThread::_instance = nullptr;
std::string g_sServerDescription;

// NOLINTNEXTLINE(clazy-non-pod-global-static)
static GlobalInitializer g_GlobalInitializer;

extern CScriptParserBufs g_ScriptParserBuffers;
CScriptParserBufs g_ScriptParserBuffers;


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
//  and destruction of these classes. If this order is altered, you'll get segmentation faults (access violations) when the server is servClosing!
#ifdef _LIBEV
	extern LinuxEv g_NetworkEvent;
#endif
	CNetworkManager g_NetworkManager;

// Again, game servers stuff.
CUOInstall		g_Install;
CVerDataMul		g_VerData;
sl::GuardedAccess<CExprGlobals>     // TODO: put inside GuardedAccess also g_Cfg, and slowly also the other stuff?
    g_ExprGlobals;                  // Global script variables.
CSStringList	g_AutoComplete;		// auto-complete list
CScriptProfiler g_profiler;			// script profiler
CUOMapList		g_MapList;			// global maps information


//-- Threads.

// Startup/Monitor Sphere context bound to the bootstrap thread.
// NOLINTNEXTLINE(clazy-non-pod-global-static)
static StartupMonitorThread g_StartupMonitor;

// NOLINTNEXTLINE(clazy-non-pod-global-static)
static MainThread g_Main;

// NOLINTNEXTLINE(clazy-non-pod-global-static)
static PingServer g_PingServer;

CDataBaseAsyncHelper g_asyncHdb;


//*******************************************************************

#ifdef _WIN32
// Expose bootstrap-thread binding helpers for other TUs (e.g., WinMain/service).
#include "../sphere/StartupMonitorAPI.h"
extern "C"
{
    void Sphere_AttachBootstrapContext()
    {
        g_StartupMonitor.attachToCurrentThread("T_SphereStartup");
    }

    void Sphere_RenameBootstrapToMonitor()
    {
        g_StartupMonitor.renameAsMonitor();
    }

    void Sphere_RunMonitorLoop()
    {
        g_StartupMonitor.runMonitorLoop();
    }
}
#endif


static bool WritePidFile(int iMode = 0)
{
	lpctstr	fileName = SPHERE_FILE ".pid";
	FILE* pidFile;

	if (iMode == 1)		// delete
	{
		return (STDFUNC_UNLINK(fileName) == 0);
	}
	else if (iMode == 2)	// check for .pid file
	{
		pidFile = fopen(fileName, "r");
		if (pidFile)
		{
			g_Log.Event(LOGM_INIT, SPHERE_FILE ".pid already exists. Secondary launch or unclean shutdown?\n");
			fclose(pidFile);
		}
		return true;
	}
	else
	{
		pidFile = fopen(fileName, "w");
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
        EXC_SET_BLOCK("cmdline post-init");
        if ( !g_Serv.CommandLinePostLoad(argc, argv) )
			return -1;
	}

	WritePidFile(2);

	EXC_SET_BLOCK("sockets init");
	if ( !g_Serv.SocketsInit() )
		return -9;

	EXC_SET_BLOCK("load world");
    g_Serv.SetServerMode(ServMode::StartupLoadingSaves);
	if ( !g_World.LoadAll() )
		return -8;

    //	load auto-complete dictionary // TODO: might as well be removed...?
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
    g_Serv.SetServerMode(ServMode::Run);	// ready to go.
    // Enter Run mode: inform ThreadHolder to disable startup fallback for current()
    ThreadHolder::markServEnteredRunMode();

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
    g_Serv.r_Call("f_onserver_start", CScriptParserBufs::GetCScriptTriggerArgsPtr(), &g_Serv);
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
    g_Serv.r_Call("f_onserver_exit", CScriptParserBufs::GetCScriptTriggerArgsPtr(), &g_Serv);

    g_Serv.SetServerMode(ServMode::Exiting);

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


//*****************************************************

// Provide the monitor stuck-check wrapper referenced by StartupMonitorThread
bool Sphere_CheckMainStuckAndRestart()
{
    return g_Main.checkStuck();
}

static void atexit_handler()
{
	ThreadHolder::get().markThreadsClosing();
}

#ifdef UNIT_TESTING
static int DocTestMain()
{
    doctest::Context context;

    int res = context.run(); // run

    if(context.shouldExit()) // important - query flags (and --exit) rely on the user doing this
        return res;          // propagate the result of the tests

    return res;
}
#endif

#ifdef _WIN32
int Sphere_MainEntryPoint( int argc, char *argv[] )
#else
int main( int argc, char * argv[] )
#endif
{
#ifdef UNIT_TESTING
    return DocTestMain();
#endif

    static constexpr lpctstr m_sClassName = "main";
    EXC_TRY("MAIN");

    const int atexit_handler_result = std::atexit(atexit_handler); // Handler will be called
    if (atexit_handler_result != 0)
    {
        g_Log.Event(LOGL_CRIT, "atexit handler registration failed.\n");
        goto exit_server;
    }

#ifndef _WIN32
    // Bind the bootstrap OS thread as a proper Sphere thread during startup.
    g_StartupMonitor.attachToCurrentThread("T_SphereStartup");

    // Start UnixTerminal in its own thread.
    g_UnixTerminal.start();
#endif

    if ( argc > 1 )
    {
        EXC_SET_BLOCK("cmdline pre-init");
        if ( !g_Serv.CommandLinePreLoad(argc, argv) )
        {
#ifndef _WIN32
            g_UnixTerminal.waitForClose();
#endif
            return 0;
        }
    }

#ifndef _WIN32
    // We need to find out the log files folder... look it up in the .ini file (on Windows it's done in WinMain function).
    g_Serv.SetServerMode(ServMode::StartupPreLoadingIni);

    // Parse command line arguments.
    for (int argn = 1; argn < argc; ++argn)
    {
        const tchar * cliArg = argv[argn];
        if (! _IS_SWITCH(cliArg[0]))
        {
            continue;
        }

        ++cliArg;

        // Check if we are changing ini path.
        if (toupper(cliArg[0]) == 'I')
        {
            // Define path to ini files.
            g_Cfg.SetIniDirectory(cliArg + 2);
        }
    }

    g_Cfg.LoadIni(false);
#endif

    g_Serv.SetServerMode(ServMode::StartupLoadingScripts);
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

        const bool fShouldCoreRunInSeparateThread = (g_Cfg.m_iFreezeRestartTime != 0);
        if (fShouldCoreRunInSeparateThread)
        {
            // Core runs on a separate OS thread.
            g_Main.start();
            // Switch the bootstrap thread from startup duties to monitoring duties.
            g_StartupMonitor.renameAsMonitor();
            // Blocking monitor loop on the bootstrap thread.
            g_StartupMonitor.runMonitorLoop();
        }
        else
        {
            // Inline core on the bootstrap thread: release the startup context and bind g_Main.
            g_StartupMonitor.detachFromCurrentThread();
            AbstractThread::ThreadBindingScope bind(g_Main, "T_Main");
            while (!g_Serv.GetExitFlag())
                g_Main.tick();
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
            SLEEP(100);
        }
    }
#endif

	return g_Serv.GetExitFlag();
	EXC_CATCH;

	return -1;
}


#include "../tables/classnames.tbl"
