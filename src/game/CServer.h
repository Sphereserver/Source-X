/**
* @file CServer.h
*
*/

#ifndef _INC_CSERVER_H
#define _INC_CSERVER_H

#include "../network/CSocket.h"
#include "../common/CSFileObj.h"
#include "../common/CTextConsole.h"
#include "../common/CDataBase.h"
#include "../common/sqlite/SQLite.h"
#include "../sphere/ConsoleInterface.h"
#include "clients/CChat.h"
#include "CServerDef.h"
#include <atomic>


class CItemShip;

enum SERVMODE_TYPE
{
	SERVMODE_RestockAll,		// Major event, potentially slow.
	SERVMODE_GarbageCollection,	// Executing the garbage collection, potentially demanding.
	SERVMODE_Saving,			// Forced save freezes the system.
	SERVMODE_Run,				// Game is up and running.
	SERVMODE_ResyncPause,		// Server paused during resync.

    SERVMODE_PreLoadingINI,     // Initial (first) parsing of the sphere.ini
	SERVMODE_Loading,			// Initial load.
	SERVMODE_ResyncLoad,		// Loading after resync.
	SERVMODE_Exiting			// Closing down.
};


extern class CServer : public CServerDef, public CTextConsole
{
	static lpctstr const sm_szVerbKeys[];

public:
    static const char* m_sClassName;
    std::atomic<SERVMODE_TYPE> m_iModeCode;	// Just some error code to return to system.
    std::atomic_int m_iExitFlag;			// identifies who caused the exit. <0 = error
    bool m_fResyncPause;		            // Server is temporarily halted so files can be updated.
    CTextConsole* m_fResyncRequested;		// A resync pause has been requested by this source.
    
#ifdef _WIN32
    bool _fCloseNTWindowOnTerminate;
#endif

    CSocket m_SocketMain;	// This is the incoming monitor socket.(might be multiple ports?)
    CSocket m_SocketGod;	// This is for god clients.

    // admin console.
    int m_iAdminClients;		    // how many of my clients are admin consoles ?
    CSString m_sConsoleText;
    bool m_fConsoleTextReadyFlag;	// interlocking flag for moving between tasks.

    int64 m_timeShutdown;	// When to perform the shutdowm (g_World.clock)
    CChat m_Chats;	        // keep all the active chats

    char m_PacketFilter[255][32];       // list of packet filtering functions
    char m_OutPacketFilter[255][32];    // list of outgoing packet filtering functions

    CSFileObj   _hFile;     // File script object
    CDataBase   _hDb;		// Online database (MySQL)
    CSQLite	    _hLdb;		// Local (file) database (SQLite)
    CSQLite     _hMdb;      // In-memory database (SQLite)

private:
	void ProfileDump( CTextConsole * pSrc, bool bDump = false );

public:
	CServer();
	virtual ~CServer();

private:
	CServer(const CServer& copy);
	CServer& operator=(const CServer& other);

public:
    SERVMODE_TYPE GetServerMode() const;
	void SetServerMode( SERVMODE_TYPE mode );
    bool IsValidBusy() const;
    int GetExitFlag() const;
    void SetExitFlag(int iFlag);
    bool IsLoading() const;
    bool IsResyncing() const;
	void Shutdown( int64 iMinutes );
	void SetSignals( bool fMsg = true );
    bool SetProcessPriority(int iPriorityLevel);

	bool SocketsInit(); // Initialize sockets
	bool SocketsInit( CSocket & socket );
	void SocketsClose();

	bool Load();

	void SysMessage( lpctstr pMsg ) const;
    void SysMessage(std::unique_ptr<ConsoleOutput>&& pMsg) const;
	void PrintTelnet( lpctstr pszMsg ) const;
    void PrintStr(lpctstr pMsg) const;
    void PrintStr(ConsoleTextColor iColor, lpctstr pMsg) const;
	ssize_t PrintPercent( ssize_t iCount, ssize_t iTotal ) const;

	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;

	lpctstr GetStatusString( byte iIndex = 0 ) const;
	virtual int64 GetAgeHours() const override;

	bool OnConsoleCmd( CSString & sText, CTextConsole * pSrc );

	void _OnTick();

public:
	void ListClients( CTextConsole * pClient ) const;
	void SetResyncPause( bool fPause, CTextConsole * pSrc, bool bMessage = false );
	bool CommandLine( int argc, tchar * argv[] );

	lpctstr GetName() const { return CServerDef::GetName(); }
	PLEVEL_TYPE GetPrivLevel() const;
} g_Serv;	// current state stuff not saved.


#endif // _INC_CSERVER_H
