
#pragma once
#ifndef _INC_CLOG_H
#define _INC_CLOG_H

#include "../common/sphere_library/CFile.h"
#include "../common/spherecom.h"
#include "../common/common.h"
#include "../common/CTime.h"
#include "../common/CScriptObj.h"
#include "../common/CScript.h"
#include "../sphere/mutex.h"

// -----------------------------
//	CEventLog
// -----------------------------

enum LOGL_TYPE
{
	// critical level.
	LOGL_FATAL	= 1, 	// fatal error ! cannot continue.
	LOGL_CRIT	= 2, 	// critical. might not continue.
	LOGL_ERROR	= 3, 	// non-fatal errors. can continue.
	LOGL_WARN	= 4,	// strange.
	LOGL_EVENT	= 5,	// Misc major events.
	// subject matter. (severity level is first 4 bits, LOGL_EVENT)
	LOGM_INIT = 0x00100,		// start up messages.
	LOGM_NOCONTEXT = 0x20000,	// do not include context information
	LOGM_DEBUG = 0x40000		// debug kind of message with DEBUG: prefix
};

extern class CEventLog
{
	// Any text event stream. (destination is independant)
	// May include __LINE__ or __FILE__ macro as well ?

protected:
	virtual int EventStr(dword wMask, lpctstr pszMsg)
	{
		UNREFERENCED_PARAMETER(wMask);
		UNREFERENCED_PARAMETER(pszMsg);
		return 0;
	}
	virtual int VEvent(dword wMask, lpctstr pszFormat, va_list args);

public:
	int _cdecl Event( dword wMask, lpctstr pszFormat, ... ) __printfargs(3,4)
	{
		va_list vargs;
		va_start( vargs, pszFormat );
		int iret = VEvent( wMask, pszFormat, vargs );
		va_end( vargs );
		return( iret );
	}

	int _cdecl EventDebug(lpctstr pszFormat, ...) __printfargs(2,3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iret = VEvent(LOGM_NOCONTEXT|LOGM_DEBUG, pszFormat, vargs);
		va_end(vargs);
		return iret;
	}

	int _cdecl EventError(lpctstr pszFormat, ...) __printfargs(2,3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iret = VEvent(LOGL_ERROR, pszFormat, vargs);
		va_end(vargs);
		return iret;
	}

	int _cdecl EventWarn(lpctstr pszFormat, ...) __printfargs(2,3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		int iret = VEvent(LOGL_WARN, pszFormat, vargs);
		va_end(vargs);
		return iret;
	}

#ifdef _DEBUG
	int _cdecl EventEvent( lpctstr pszFormat, ... ) __printfargs(2,3)
	{
		va_list vargs;
		va_start( vargs, pszFormat );
		int iret = VEvent( LOGL_EVENT, pszFormat, vargs );
		va_end( vargs );
		return( iret );
	}
#endif //_DEBUG

#define DEBUG_ERR(_x_)	g_pLog->EventError _x_

#ifdef _DEBUG
#define DEBUG_WARN(_x_)	g_pLog->EventWarn _x_
#define DEBUG_MSG(_x_)	g_pLog->EventEvent _x_
#define DEBUG_MYFLAG(_x_) g_pLog->Event _x_
#else
#define DEBUG_WARN(_x_)
#define DEBUG_MSG(_x_)
#define DEBUG_MYFLAG(_x_)
#endif

public:
	CEventLog() { };

private:
	CEventLog(const CEventLog& copy);
	CEventLog& operator=(const CEventLog& other);
} * g_pLog;

extern struct CLog : public CFileText, public CEventLog
{
	// subject matter. (severity level is first 4 bits, LOGL_EVENT)
	#define LOGM_ACCOUNTS		0x00080
	//#define LOGM_INIT			0x00100	// start up messages.
	#define LOGM_SAVE			0x00200	// world save status.
	#define LOGM_CLIENTS_LOG	0x00400	// all clients as they log in and out.
	#define LOGM_GM_PAGE		0x00800	// player gm pages.
	#define LOGM_PLAYER_SPEAK	0x01000	// All that the players say.
	#define LOGM_GM_CMDS		0x02000	// Log all GM commands.
	#define LOGM_CHEAT			0x04000	// Probably an exploit !
	#define LOGM_KILLS			0x08000	// Log player combat results.
	#define LOGM_HTTP			0x10000
	//#define	LOGM_NOCONTEXT		0x20000	// do not include context information
	//#define LOGM_DEBUG			0x40000	// debug kind of message with DEBUG: prefix

private:
	dword m_dwMsgMask;			// Level of log detail messages. IsLogMsg()
	CGTime m_dateStamp;			// last real time stamp.
	CGString m_sBaseDir;

	const CScript * m_pScriptContext;	// The current context.
	const CScriptObj * m_pObjectContext;	// The current context.

	static CGTime sm_prevCatchTick;	// don't flood with these.
public:
	bool m_fLockOpen;
	SimpleMutex m_mutex;

public:
	const CScript * SetScriptContext( const CScript * pScriptContext );
	const CScriptObj * SetObjectContext( const CScriptObj * pObjectContext );
	bool SetFilePath( lpctstr pszName );

	lpctstr GetLogDir() const;
	bool OpenLog( lpctstr pszName = NULL );	// name set previously.
	dword GetLogMask() const;
	void SetLogMask( dword dwMask );
	bool IsLoggedMask( dword dwMask ) const;
	LOGL_TYPE GetLogLevel() const;
	void SetLogLevel( LOGL_TYPE level );
	bool IsLoggedLevel( LOGL_TYPE level ) const;
	bool IsLogged( dword wMask ) const;

	virtual int EventStr( dword wMask, lpctstr pszMsg );
	void _cdecl CatchEvent( const CSphereError * pErr, lpctstr pszCatchContext, ...  ) __printfargs(3,4);

public:
	CLog();

private:
	CLog(const CLog& copy);
	CLog& operator=(const CLog& other);

	enum Color
	{
		DEFAULT,
		YELLOW,
		RED,
		CYAN
	};

	/**
	* Changes current console color to the specified one. Note, that the color should be reset after being set
	*/
	void SetColor(Color color);
} g_Log;		// Log file


#endif // _INC_CLOG_H
