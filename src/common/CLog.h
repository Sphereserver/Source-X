/**
* @file CLog.h
* @brief Sphere logging system.
*/

#ifndef _INC_CLOG_H
#define _INC_CLOG_H

#include "sphere_library/CSFileText.h"
#include "sphere_library/CSTime.h"
#include "../sphere/ConsoleInterface.h"
#include "../sphere/UnixTerminal.h"
#include <exception>

// -----------------------------
//	CEventLog
// -----------------------------

enum LOG_TYPE : uint
{
	// --severity Level
	//	it can be only one of them, so we don't need to use a bitmask
	LOGL_FATAL	= 1, 	// fatal error ! cannot continue.
	LOGL_CRIT	= 2, 	// critical. might not continue.
	LOGL_ERROR	= 3, 	// non-fatal errors. can continue.
	LOGL_WARN	= 4,	// strange.
	LOGL_EVENT	= 5,	// Misc major events.
	LOGL_QTY	= 0xF,	// first 4 bits = binary 1111 = dec 15 = hex 0xF

	// --log Flags
	LOGF_CONSOLE_ONLY	= 0x10,	// do not write this message to the log file
	LOGF_LOGFILE_ONLY	= 0x20,	// write this message only to the log file
	LOGF_QTY			= 0xF0,

	// --subject Matter (severity level is first 4 bits, LOGL_EVENT)
	LOGM_ACCOUNTS		= 0x000100,
	LOGM_INIT			= 0x000200,	// start up messages.
	LOGM_SAVE			= 0x000400,	// world save status.
	LOGM_CLIENTS_LOG	= 0x000800,	// all clients as they log in and out.
	LOGM_GM_PAGE		= 0x001000,	// player gm pages.
	LOGM_PLAYER_SPEAK	= 0x002000,	// All that the players say.
	LOGM_GM_CMDS		= 0x004000,	// Log all GM commands.
	LOGM_CHEAT			= 0x008000,	// Probably an exploit !
	LOGM_KILLS			= 0x010000,	// Log player combat results.
	LOGM_HTTP			= 0x020000,
	LOGM_NOCONTEXT		= 0x040000,	// do not include context information
	LOGM_DEBUG			= 0x080000,	// debug kind of message with "DEBUG:" prefix
    LOGM_QTY            = 0x0FFF00  // All masks.
};

class CSError;
class CScript;
class CScriptObj;


class CEventLog
{
	// Any text event stream. (destination is independant)
	// May include __LINE__ or __FILE__ macro as well ?

protected:
	virtual int EventStr(dword dwMask, lpctstr pszMsg, ConsoleTextColor iColor = CTCOL_DEFAULT) noexcept = 0;

    int VEvent(dword dwMask, lpctstr pszFormat, ConsoleTextColor iColor, va_list args) noexcept;

public:
	int _cdecl Event( dword dwMask, lpctstr pszFormat, ... ) noexcept __printfargs(3,4);
	int _cdecl EventDebug(lpctstr pszFormat, ...) noexcept  __printfargs(2,3);
	int _cdecl EventError(lpctstr pszFormat, ...) noexcept __printfargs(2,3);
	int _cdecl EventWarn(lpctstr pszFormat, ...) noexcept __printfargs(2,3);
    int _cdecl EventCustom(ConsoleTextColor iColor, dword dwMask, lpctstr pszFormat, ...) noexcept __printfargs(4,5);
#ifdef _DEBUG
	int _cdecl EventEvent( lpctstr pszFormat, ... ) noexcept __printfargs(2,3);
#endif //_DEBUG

public:
	CEventLog() = default;

private:
	CEventLog(const CEventLog& copy);
	CEventLog& operator=(const CEventLog& other);
};


extern struct CLog : public CSFileText, public CEventLog
{
private:
	// MT_CMUTEX_DEF; // There's already the CSFileText::MT_CMUTEX

	dword m_dwMsgMask;			// Level of log detail messages. IsLogMsg()
	CSTime m_dateStamp;			// last real time stamp.
	CSString m_sBaseDir;

	const CScript * m_pScriptContext;		// The current context.
	const CScriptObj * m_pObjectContext;	// The current context.

	static CSTime sm_prevCatchTick;			// don't flood with these.

public:
	bool m_fLockOpen;

protected:	const CScript * _SetScriptContext( const CScript * pScriptContext );
public:     const CScript * SetScriptContext( const CScript * pScriptContext );
protected:  const CScriptObj * _SetObjectContext( const CScriptObj * pObjectContext );
public:	    const CScriptObj * SetObjectContext( const CScriptObj * pObjectContext );


protected:	bool _OpenLog(lpctstr pszName = nullptr);	// name set previously.
public:		bool OpenLog(lpctstr pszName = nullptr);

	bool SetFilePath(lpctstr pszName);

	lpctstr GetLogDir() const;
	dword GetLogMask() const;
	void SetLogMask( dword dwMask );
	bool IsLoggedMask( dword dwMask ) const;
	LOG_TYPE GetLogLevel() const;
	void SetLogLevel( LOG_TYPE level );
	bool IsLoggedLevel( LOG_TYPE level ) const;
	bool IsLogged( dword dwMask ) const;

	virtual int EventStr( dword dwMask, lpctstr pszMsg, ConsoleTextColor iLogColor = CTCOL_DEFAULT ) noexcept final;	// final: for now, it doesn't have any other virtual methods
	void _cdecl CatchEvent( const CSError * pErr, lpctstr pszCatchContext, ...  ) __printfargs(3,4);
    void _cdecl CatchStdException( const std::exception * pExc, lpctstr pszCatchContext, ...  ) __printfargs(3,4);

public:
	CLog();

	CLog(const CLog& copy) = delete;
	CLog& operator=(const CLog& other) = delete;
} g_Log;		// Log file


#define DEBUG_ERR(_x_)	        g_Log.EventError _x_

#ifdef _DEBUG
    #define DEBUG_WARN(_x_)		g_Log.EventWarn _x_
    #define DEBUG_MSG(_x_)		g_Log.EventDebug _x_
    #define DEBUG_EVENT(_x_)	g_Log.EventEvent _x_
    #define DEBUG_MYFLAG(_x_)	g_Log.Event _x_
#else
    // Better use placeholders than leaving it empty, because unwanted behaviours may occur.
    //  example: an else clause without brackets will include next line instead of DEBUG_WARN, because the macro is empty.
    #define DEBUG_WARN(_x_)		(void)0
    #define DEBUG_MSG(_x_)		(void)0
    #define DEBUG_EVENT(_x_)	(void)0
    #define DEBUG_MYFLAG(_x_)	(void)0
#endif

#endif // _INC_CLOG_H
