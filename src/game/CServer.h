/**
* @file CServer.h
*
*/

#ifndef _INC_CSERVER_H
#define _INC_CSERVER_H

#include "../common/sqlite/SQLite.h"
#include "../common/CSFileObj.h"
#include "../common/CTextConsole.h"
#include "../common/CDataBase.h"
#include "../sphere/ConsoleInterface.h"
#include "clients/CChat.h"
#include "CServerDef.h"
#include <atomic>


class CItemShip;

enum class ServMode
{
    RestockAll,             // Major event, potentially slow.
    GarbageCollection,      // Executing the garbage collection, potentially demanding.
    Saving,                 // Forced save freezes the system.
    Run,                    // Game is up and running.
    ResyncPause,            // Server paused during resync.

    StartupPreLoadingIni,   // Initial (first) parsing of the sphere.ini
    StartupLoadingScripts,  // Initial load.
    StartupLoadingSaves,
    ResyncLoad,             // Loading after resync.
    Exiting                 // Closing down.
};


extern class CServer : public CServerDef, public CTextConsole
{
	static lpctstr const sm_szVerbKeys[];

public:
    static const char* m_sClassName;
    std::atomic<ServMode> m_iModeCode;      // Query the server state.
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
	virtual ~CServer() override;

	CServer(const CServer& copy) = delete;
	CServer& operator=(const CServer& other) = delete;

public:
    void SetServerMode( ServMode mode );
    ServMode GetServerMode() const noexcept;
    bool IsStartupLoadingScripts() const noexcept;
    //bool IsStartupLoadingSaves() const noexcept;
    bool IsLoadingGeneric() const noexcept;
    bool IsResyncing() const noexcept;
    bool IsDestroyingWorld() const noexcept;

    bool IsValidBusy() const;

    int GetExitFlag() const noexcept;
    void SetExitFlag(int iFlag) noexcept;
    void Shutdown( int64 iMinutes );
	void SetSignals( bool fMsg = true );
    bool SetProcessPriority(int iPriorityLevel);

	bool SocketsInit(); // Initialize sockets
	bool SocketsInit( CSocket & socket );
	void SocketsClose();

	bool Load();

	virtual void SysMessage( lpctstr pMsg ) const override;
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
    bool CommandLinePreLoad( int argc, tchar * argv[] );
    bool CommandLinePostLoad( int argc, tchar * argv[] );

	virtual lpctstr GetName() const override {
	    return CServerDef::GetName();
    }
	virtual PLEVEL_TYPE GetPrivLevel() const override;
} g_Serv;	// current state stuff not saved.


#endif // _INC_CSERVER_H
