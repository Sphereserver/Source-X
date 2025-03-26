/**
* @file ntservice.h
*
*/

#ifndef _INC_NTSERVICE_H
#define _INC_NTSERVICE_H

#ifdef _WIN32

#include "../common/common.h"

#ifdef NON_MSVC_COMPILER
	#include <excpt.h>
#else
	#include <eh.h>			// exception handling info.
#endif // NON_MSVC_COMPILER



extern class CNTService
{
private:
	bool m_fIsNTService;	// Are we running an NT service ?

	HANDLE m_hServerStopEvent;
	SERVICE_STATUS_HANDLE m_hStatusHandle;
	SERVICE_STATUS m_sStatus;

private:
	int  ServiceStart(DWORD, LPTSTR *);
	void ServiceStop();
	void ServiceStartMain(DWORD dwArgc, LPTSTR *lpszArgv);

	// Our exported API.
	static void WINAPI service_ctrl(DWORD dwCtrlCode);
	static void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);

public:
	static const char *m_sClassName;
	CNTService();

private:
	CNTService(const CNTService& copy);
	CNTService& operator=(const CNTService& other);

public:
	void ReportEvent( WORD wType, DWORD dwMsgID, LPCTSTR lpszMsg, LPCTSTR lpszArgs = nullptr );
	BOOL SetServiceStatus(DWORD, DWORD, DWORD);

	// command line entries.
	void CmdInstallService();
	void CmdRemoveService();
	void CmdMainStart();

	bool _OnTick();
	bool IsRunningAsService() const
	{
		return( m_fIsNTService );
	}
} g_NTService;  // extern class


#endif // _WIN32

#endif // _INC_NTSERVICE_H
