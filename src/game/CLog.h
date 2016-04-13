
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
