// Stuff for making this application run as an NT service

#ifdef _WIN32

#include <direct.h>
#include "../common/CException.h"
#include "../common/CExpression.h"
#include "../common/sphereversion.h"
#include "../common/CLog.h"
#include "../game/CObjBase.h"
#include "../game/CServer.h"
#include "../game/spheresvr.h"
#include "ntwindow.h"
#include "ntservice.h"

CNTService g_NTService;

CNTService::CNTService()
{
    m_hStatusHandle = nullptr;
    m_hServerStopEvent = nullptr;
	m_fIsNTService = false;
    m_sStatus = {};
}

// Try to create the registry key containing the working directory for the application
static void ExtractPath(LPTSTR szPath)
{
	TCHAR *pszPath = strrchr(szPath, '\\');
	if ( pszPath )
		*pszPath = 0;
}

static LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize)
{
	// Check CSError.
	//	PURPOSE:  copies error message text to a string
	//
	//	PARAMETERS:
	//		lpszBuf - destination buffer
	//		dwSize - size of buffer
	//
	//	RETURN VALUE:
	//		destination buffer

	int nChars = CSError::GetSystemErrorMessage( GetLastError(), lpszBuf, dwSize );
	sprintf( lpszBuf+nChars, " (0x%lX)", GetLastError());
	return lpszBuf;
}

/////////////////////////////////////////////////////////////////////////////////////

//	PURPOSE:  Allows any thread to log a message to the NT Event Log
void CNTService::ReportEvent( WORD wType, DWORD dwEventID, LPCTSTR lpszMsg, LPCTSTR lpszArgs )
{
	UnreferencedParameter(dwEventID);
	g_Log.Event(LOGM_INIT|(( wType == EVENTLOG_INFORMATION_TYPE ) ? LOGL_EVENT : LOGL_ERROR), "%s %s\n", lpszMsg, lpszArgs);
}

// RETURN: false = exit app.
bool CNTService::_OnTick()
{
	if (( !m_fIsNTService ) || ( m_sStatus.dwCurrentState != SERVICE_STOP_PENDING ) )
		return true;

	// Let the SCM know we aren't ignoring it
	SetServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 1000);
	g_Serv.SetExitFlag(4);
	return false;
}

//	PURPOSE:  Sets the current status of the service and reports it to the Service Control Manager
BOOL CNTService::SetServiceStatus( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint )
{
	if ( dwCurrentState == SERVICE_START_PENDING )
		m_sStatus.dwControlsAccepted = 0;
	else
		m_sStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	m_sStatus.dwCurrentState = dwCurrentState;
	m_sStatus.dwWin32ExitCode = dwWin32ExitCode;
	m_sStatus.dwWaitHint = dwWaitHint;

	static DWORD dwCheckPoint = 1;
	if (( dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
		m_sStatus.dwCheckPoint = 0;
	else
		m_sStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the Service Control Manager
	BOOL fResult = ::SetServiceStatus(m_hStatusHandle, &m_sStatus);
	if ( !fResult && m_fIsNTService )
	{
		// if fResult is not 0, then an error occurred.  Throw this in the event log.
		char szErr[256];
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "SetServiceStatus", GetLastErrorText(szErr, sizeof(szErr)));
	}
	return fResult;
}

//	PURPOSE:  This function is called by the SCM whenever ControlService() is called on this service.  The
//		SCM does not start the service through this function.
void WINAPI CNTService::service_ctrl(DWORD dwCtrlCode) // static
{
	if ( dwCtrlCode == SERVICE_CONTROL_STOP )
        g_NTService.ServiceStop();
    g_NTService.SetServiceStatus(g_NTService.m_sStatus.dwCurrentState, NO_ERROR, 0);
}

//	PURPOSE: is called by the SCM, and takes care of some initialization and calls ServiceStart().
void WINAPI CNTService::service_main(DWORD dwArgc, LPTSTR *lpszArgv) // static
{
    g_NTService.ServiceStartMain(dwArgc, lpszArgv);
}

//	PURPOSE:  starts the service. (synchronous)
void CNTService::ServiceStartMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	TCHAR *pszMsg = Str_GetTemp();

	sprintf(pszMsg, SPHERE_TITLE " " SPHERE_BUILD_NAME_VER_PREFIX SPHERE_BUILD_INFO_GIT_STR " - %s", g_Serv.GetName());

	m_hStatusHandle = RegisterServiceCtrlHandler(pszMsg, service_ctrl);
	if ( !m_hStatusHandle )	// Not much we can do about this.
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "RegisterServiceCtrlHandler failed\n");
		return;
	}

	// SERVICE_STATUS members that don't change
	m_sStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	m_sStatus.dwServiceSpecificExitCode = 0;

	// report the status to the service control manager.
	if ( SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
		ServiceStart( dwArgc, lpszArgv);

	SetServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

//	PURPOSE:  starts the service. (synchronous)
int CNTService::ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv)
{
	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Service start pending.");

	// Service initialization report status to the service control manager.
	int rc = -1;
	if ( !SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
		return rc;

	// Create the event object.  The control handler function signals this event when it receives a "stop" control code
	m_hServerStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if ( !m_hServerStopEvent )
		return rc;

	if ( !SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
	{
bailout1:
		CloseHandle(m_hServerStopEvent);
		return rc;
	}

	// Create the event object used in overlapped i/o
	HANDLE hEvents[2] = {nullptr, nullptr};
	hEvents[0] = m_hServerStopEvent;
	hEvents[1] = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if ( !hEvents[1] )
		goto bailout1;

	if ( !SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
	{
bailout2:
		CloseHandle(hEvents[1]);
		goto bailout1;
	}

	// Create a security descriptor that allows anyone to write to the pipe
	PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
	if ( !pSD )
		goto bailout2;

	if ( !InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION) )
	{
bailout3:
		free(pSD);
		goto bailout2;
	}

	// Add a nullptr descriptor ACL to the security descriptor
	if ( !SetSecurityDescriptorDacl(pSD, TRUE, nullptr, FALSE) )
		goto bailout3;

	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = TRUE;

	if ( !SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
		goto bailout3;

	// Set the name of the named pipe this application uses.  We create a named pipe to ensure that
	// only one instance of this application runs on the machine at a time.  If there is an instance
	// running, it will own this pipe, and any further attempts to create the same named pipe will fail.
	char lpszPipeName[80];
	sprintf(lpszPipeName, "\\\\.\\pipe\\" SPHERE_FILE "svr\\%s", g_Serv.GetName());

	char szErr[256];

	// Open the named pipe
	HANDLE hPipe = CreateNamedPipe(lpszPipeName,
		FILE_FLAG_OVERLAPPED|PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
		1, 0, 0, 1000, &sa);
	if ( hPipe == INVALID_HANDLE_VALUE )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Can't create named pipe. Check multi-instance?", GetLastErrorText(szErr, sizeof(szErr)));
		goto bailout3;
	}

	if ( SetServiceStatus(SERVICE_RUNNING, NO_ERROR, 0) )
	{
		rc = Sphere_MainEntryPoint(dwArgc, lpszArgv);

		if ( !rc )
			ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "service stopped", GetLastErrorText(szErr, sizeof(szErr)));
		else
		{
			char szMessage[80];
			sprintf(szMessage, "%d.", rc);
			ReportEvent(EVENTLOG_ERROR_TYPE, 0, "service stopped abnormally", szMessage);
		}
	}
	else
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "ServiceStart failed.");

	CloseHandle(hPipe);
	goto bailout3;
}

//	PURPOSE:  reports that the service is attempting to stop
void CNTService::ServiceStop()
{
	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Attempting to stop service");

	m_sStatus.dwCurrentState = SERVICE_STOP_PENDING;
	SetServiceStatus(m_sStatus.dwCurrentState, NO_ERROR, 3000);

	if (m_hServerStopEvent)
		SetEvent(m_hServerStopEvent);
}

//	PURPOSE:  Installs the service on the local machine
void CNTService::CmdInstallService()
{
	char	szPath[SPHERE_MAX_PATH * 2];
	char	szErr[256];

	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Installing Service.");

	// Try to determine the name and path of this application.
	if ( !GetModuleFileName(nullptr, szPath, sizeof(szPath)) )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install GetModuleFileName", GetLastErrorText(szErr, sizeof(szErr)));
		return;
	}

	// Try to open the Service Control Manager
	SC_HANDLE schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if ( !schSCManager )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install OpenSCManager", GetLastErrorText(szErr, sizeof(szErr)));
		return;
	}

	// Try to create the service
	char szInternalName[MAX_PATH];
	sprintf(szInternalName, SPHERE_TITLE " - %s", g_Serv.GetName());

	SC_HANDLE schService = CreateService(
		schSCManager,					// handle of the Service Control Manager
		szInternalName,				// Internal name of the service (used when controlling the service using "net start" or "netsvc")
		szInternalName,			// Display name of the service (displayed in the Control Panel | Services page)
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START,				// Start automatically when the OS starts
		SERVICE_ERROR_NORMAL,
		szPath,							// Path and filename of this executable
		nullptr, nullptr, nullptr, nullptr, nullptr
	);
	if ( !schService )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install CreateService", GetLastErrorText(szErr, sizeof(szErr)));
bailout1:
		CloseServiceHandle(schSCManager);
		return;
	}

	// Configure service - Service description
	char szDescription[MAX_PATH];
	sprintf(szDescription, "SphereServer Service for %s", g_Serv.GetName());

	SERVICE_DESCRIPTION sdDescription;
	sdDescription.lpDescription = szDescription;
	if ( !ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sdDescription) )
	{
		// not critical, so no need to abort the service creation
		ReportEvent(EVENTLOG_WARNING_TYPE, 0, "Install SetDescription", GetLastErrorText(szErr, sizeof(szErr)));
	}

	// Configure service - Restart options
	SC_ACTION scAction[3];
	scAction[0].Type = SC_ACTION_RESTART;	// restart process on failure
	scAction[0].Delay = 10000;				// wait 10 seconds before restarting
	scAction[1].Type = SC_ACTION_RESTART;
	scAction[1].Delay = 10000;
	scAction[2].Type = SC_ACTION_RESTART;	// wait 2 minutes before restarting the third time
	scAction[2].Delay = 120000;

	SERVICE_FAILURE_ACTIONS sfaFailure;
	sfaFailure.dwResetPeriod = (1 * 60 * 60); // reset failure count after an hour passes with no fails
	sfaFailure.lpRebootMsg = nullptr;	// no reboot message
	sfaFailure.lpCommand = nullptr;	// no command executed
	sfaFailure.cActions = ARRAY_COUNT(scAction);		// number of actions
	sfaFailure.lpsaActions = scAction;	//
	if ( !ChangeServiceConfig2(schService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfaFailure) )
	{
		// not critical, so no need to abort the service creation
		ReportEvent(EVENTLOG_WARNING_TYPE, 0, "Install SetAutoRestart", GetLastErrorText(szErr, sizeof(szErr)));
	}

	HKEY	hKey;
	char	szKey[MAX_PATH];

	// Register the application for event logging
	DWORD dwData;
	// Try to create the registry key containing information about this application
	strcpy(szKey, "System\\CurrentControlSet\\Services\\EventLog\\Application\\" SPHERE_FILE "svr");

	if (RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey))
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegCreateKey", GetLastErrorText(szErr, sizeof(szErr)));
	else
	{
		// Try to create the registry key containing the name of the EventMessageFile
		//  Replace the name of the exe with the name of the dll in the szPath variable
		if (RegSetValueEx(hKey, "EventMessageFile", 0, REG_EXPAND_SZ, (LPBYTE) szPath, (DWORD)strlen(szPath) + 1))
			ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegSetValueEx", GetLastErrorText(szErr, sizeof(szErr)));
		else
		{
			// Try to create the registry key containing the types of errors this application will generate
			dwData = EVENTLOG_ERROR_TYPE|EVENTLOG_INFORMATION_TYPE|EVENTLOG_WARNING_TYPE;
			if ( RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD, (LPBYTE) &dwData, sizeof(DWORD)) )
				ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegSetValueEx", GetLastErrorText(szErr, sizeof(szErr)));
		}
		RegCloseKey(hKey);
	}

	// Set the working path for the application
	sprintf(szKey, "System\\CurrentControlSet\\Services\\" SPHERE_TITLE " - %s\\Parameters", g_Serv.GetName());
	if ( RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey) )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegCreateKey", GetLastErrorText(szErr, sizeof(szErr)));
bailout2:
		CloseServiceHandle(schService);
		goto bailout1;
	}
	ExtractPath(szPath);

	if ( RegSetValueEx(hKey, "WorkingPath", 0, REG_SZ, (const unsigned char *) &szPath[0], (DWORD)strlen(szPath)) )
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegSetValueEx", GetLastErrorText(szErr, sizeof(szErr)));

	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Install OK", g_Serv.GetName());
	goto bailout2;
}

//	PURPOSE:  Stops and removes the service
void CNTService::CmdRemoveService()
{
	char szErr[256];

	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Removing Service.");

	SC_HANDLE schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if ( !schSCManager )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Remove OpenSCManager failed", GetLastErrorText(szErr, sizeof(szErr)));
		return;
	}

	// Try to obtain the handle of this service
	char szInternalName[MAX_PATH];
	sprintf(szInternalName, SPHERE_TITLE " - %s", g_Serv.GetName());

	SC_HANDLE schService = OpenService(schSCManager, szInternalName, SERVICE_ALL_ACCESS);
	if ( !schService )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Remove OpenService failed", GetLastErrorText(szErr, sizeof(szErr)));

		CloseServiceHandle(schSCManager);
		return;
	}

	// Check to see if the service is started, if so try to stop it.
	if ( ControlService(schService, SERVICE_CONTROL_STOP, &m_sStatus) )
	{
		ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Stopping");

		Sleep(1000);
		while ( QueryServiceStatus(schService, &m_sStatus) )	// wait the service to stop
		{
			if ( m_sStatus.dwCurrentState == SERVICE_STOP_PENDING )
                SLEEP(1000);
			else
				break;
		}

		if (m_sStatus.dwCurrentState == SERVICE_STOPPED)
			ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Stopped");
		else
			ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Failed to Stop");
	}

	// Remove the service
	if ( DeleteService(schService) )
		ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Removed OK");
	else
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Remove Fail", GetLastErrorText(szErr, sizeof(szErr)));

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void CNTService::CmdMainStart()
{
	m_fIsNTService = true;

	char szTmp[256];
	sprintf(szTmp, SPHERE_TITLE " - %s", g_Serv.GetName());
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ szTmp, (LPSERVICE_MAIN_FUNCTION)service_main },
		{ nullptr, nullptr },
	};

	if ( !StartServiceCtrlDispatcher(dispatchTable) )
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "StartServiceCtrlDispatcher", GetLastErrorText(szTmp, sizeof(szTmp)));
}

/////////////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION: main()
//
// @todo Refactor installing and starting service after https://github.com/Sphereserver/Source-X/issues/1400 is resolved.
//
/////////////////////////////////////////////////////////////////////////////////////
#ifdef MSVC_COMPILER
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	UnreferencedParameter(hPrevInstance);
    AbstractThread::setThreadName("T_SphereStartup");

	TCHAR	*argv[32];
	argv[0] = nullptr;
	int argc = Str_ParseCmds(lpCmdLine, &argv[1], ARRAY_COUNT(argv)-1, " =\t") + 1;

    // Process the command line arguments.
    if (argc > 1 && _IS_SWITCH(*argv[1]))
    {
        // Check if we are changing the ini path.
        if (toupper(argv[1][1]) == 'I')
        {
            if (argc == 3)
            {
                // Clean quotes around path, if it contains spaces.
                const char* raw = argv[2];
                char path[SPHERE_MAX_PATH];
                size_t j = 0;
                for (size_t i = 0; raw[i] != '\0' && j < sizeof(path) - 1; ++i) {
                    if (raw[i] != '"') {
                        path[j++] = raw[i];
                    }
                }
                path[j] = '\0';

                // Define path to the ini files.
                g_Cfg.SetIniDirectory(path);
            }
        }
    }

	// We need to find out what the server name is and the log files folder... look it up in the .ini file
    if (!g_Cfg.LoadIni(false))
    {
        // Sphere.ini couldn't be loaded. We might be running Sphere as service and have different working directory.
        TCHAR szPath[MAX_PATH];
        GetModuleFileName(nullptr, szPath, sizeof(szPath));
        // Extract and change directory from this application.
        ExtractPath(szPath);
        if (0 == _chdir(szPath))
        {
            printf("Can't change current directory.\n");
            TerminateProcess(GetCurrentProcess(), 1);
            return 1;
        }
        // Try opening it again and continue as before.
        g_Cfg.LoadIni(false);
    }

    if (!g_Cfg.m_fUseNTService ||	// since there is no way to detect how did we start, use config for that
        Sphere_GetOSInfo()->dwPlatformId != VER_PLATFORM_WIN32_NT) // We are running Win9x - So we are not an NT service.
    {
        g_NTWindow._NTWInitParams = {hInstance, lpCmdLine, nCmdShow};
        g_NTWindow.start();

        int iRet = Sphere_MainEntryPoint(argc, argv);

        TerminateProcess(GetCurrentProcess(), iRet);
        return iRet;
    }

    // We are running as a NT Service
    g_NTService.SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 5000);

	// process the command line arguments...
	if (( argc > 1 ) && _IS_SWITCH(*argv[1]) )
	{
		if ( toupper(argv[1][1]) == 'K' )		// service control
		{
			if ( argc < 3 )
			{
				printf("Use \"-K command\" with operation to proceed (install/remove)\n");
			}
			else if ( !strcmp(argv[2], "install") )
			{
                g_NTService.CmdInstallService();
			}
			else if ( !strcmp(argv[2], "remove") )
			{
                g_NTService.CmdRemoveService();
			}
			return 0;
		}
	}

	// If the argument does not match any of the above parameters, the Service Control Manager (SCM) may
	// be attempting to start the service, so we must call StartServiceCtrlDispatcher.
    g_NTService.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Starting Service.");

    g_NTService.CmdMainStart();
    g_NTService.SetServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
	return -1;
}

#endif //_WIN32

