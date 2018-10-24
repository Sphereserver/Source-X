
#include "CException.h"
#include "common.h"
#include "../sphere/ProfileTask.h"
#include "CLog.h"
#include "../game/CServer.h"


CLog::CLog()
{
	m_fLockOpen = false;
	m_pScriptContext = nullptr;
	m_pObjectContext = nullptr;
	m_dwMsgMask = LOGL_ERROR |
				  LOGM_INIT | LOGM_CLIENTS_LOG | LOGM_GM_PAGE;
	SetFilePath( SPHERE_FILE "log.log" );	// default name to go to.
}

const CScript * CLog::SetScriptContext( const CScript * pScriptContext )
{
	const CScript * pOldScript = m_pScriptContext;
	m_pScriptContext = pScriptContext;
	return pOldScript;
}

const CScriptObj * CLog::SetObjectContext( const CScriptObj * pObjectContext )
{
	const CScriptObj * pOldObject = m_pObjectContext;
	m_pObjectContext = pObjectContext;
	return pOldObject;
}
bool CLog::SetFilePath( lpctstr pszName )
{
	ASSERT( ! IsFileOpen());
	return CSFileText::SetFilePath( pszName );
}

lpctstr CLog::GetLogDir() const
{
	return m_sBaseDir;
}

dword CLog::GetLogMask() const
{
	return ( m_dwMsgMask &~ (LOGL_QTY|LOGF_QTY) );
}

void CLog::SetLogMask( dword dwMask )
{
	m_dwMsgMask = GetLogLevel() | ( dwMask &~ (LOGL_QTY|LOGF_QTY) );
}

bool CLog::IsLoggedMask( dword dwMask ) const
{
	return ( ((dwMask &~ (LOGL_QTY | LOGF_QTY | LOGM_NOCONTEXT | LOGM_DEBUG)) == 0) ||	// debug and msgs with no context are not masks to be logged?
			 (( GetLogMask() & ( dwMask &~ (LOGL_QTY|LOGF_QTY) )) != 0) );
}

LOG_TYPE CLog::GetLogLevel() const
{
	return (LOG_TYPE)(m_dwMsgMask & LOGL_QTY);
}

void CLog::SetLogLevel( LOG_TYPE level )
{
	m_dwMsgMask = GetLogMask() | ( level & LOGL_QTY );
}

bool CLog::IsLoggedLevel( LOG_TYPE level ) const
{
	return ( ((level & LOGL_QTY) != 0) && (GetLogLevel() >= (level & LOGL_QTY)) );
}

bool CLog::IsLogged( dword dwMask ) const
{
	bool mask = IsLoggedMask(dwMask);
	bool lvl = IsLoggedLevel((LOG_TYPE)dwMask);
	return mask || lvl;
}

bool CLog::OpenLog( lpctstr pszBaseDirName )	// name set previously.
{
	if ( m_fLockOpen )	// the log is already locked open
		return false;

	if ( m_sBaseDir == nullptr )
		return false;

	if ( pszBaseDirName != nullptr )
	{
		if ( pszBaseDirName[0] && pszBaseDirName[1] == '\0' )
		{
			if ( *pszBaseDirName == '0' )
			{
				Close();
				return false;
			}
		}
		else
        {
			m_sBaseDir = pszBaseDirName;
        }
	}

	// Get the new name based on date.
	m_dateStamp = CSTime::GetCurrentTime();
	tchar *pszTemp = Str_GetTemp();
	snprintf(pszTemp, STR_TEMPLENGTH, SPHERE_FILE "%d-%02d-%02d.log",
		m_dateStamp.GetYear(), m_dateStamp.GetMonth(), m_dateStamp.GetDay());
	CSString sFileName = GetMergedFileName(m_sBaseDir, pszTemp);

	// Use the OF_READWRITE to append to an existing file.
	if ( CSFileText::Open( sFileName.GetPtr(), OF_SHARE_DENY_NONE|OF_READWRITE|OF_TEXT ) )
	{
		setvbuf(_pStream, nullptr, _IONBF, 0);
		return true;
	}
	return false;
}

void CLog::SetColor(ConsoleTextColor color)
{
#ifdef _WIN32
	switch (color)
	{
		case CTCOL_RED:
			NTWindow_PostMsgColor(RGB(255, 0, 0));
			break;
		case CTCOL_GREEN:
			NTWindow_PostMsgColor(RGB(0, 255, 0));
			break;
		case CTCOL_YELLOW:
			NTWindow_PostMsgColor(RGB(127, 127, 0));
			break;
		case CTCOL_BLUE:
			NTWindow_PostMsgColor(RGB(0, 0, 255));
			break;
		case CTCOL_MAGENTA:
			NTWindow_PostMsgColor(RGB(255, 0, 255));
			break;
		case CTCOL_CYAN:
			NTWindow_PostMsgColor(RGB(0, 127, 255));
			break;
		case CTCOL_WHITE:
			NTWindow_PostMsgColor(RGB(255, 255, 255));
			break;        
        default:
            NTWindow_PostMsgColor(0);
	}
#else
	g_UnixTerminal.setColor(color);
#endif
}

int CLog::EventStr( dword dwMask, lpctstr pszMsg )
{
	// NOTE: This could be called in odd interrupt context so don't use dynamic stuff
	if ( !IsLogged(dwMask) )	// I don't care about these.
		return 0;
	else if ( !pszMsg || !*pszMsg )
		return 0;

	int iRet = 0;
	m_mutex.lock();

	try
	{
		// Put up the date/time.
		CSTime datetime = CSTime::GetCurrentTime();	// last real time stamp.

		tchar szTime[32];
		snprintf(szTime, sizeof(szTime), "%02d:%02d:", datetime.GetHour(), datetime.GetMinute());

		lpctstr pszLabel = nullptr;
		switch (dwMask & LOGL_QTY)
		{
			case LOGL_FATAL:	// fatal error !
				pszLabel = "FATAL:";
				break;
			case LOGL_CRIT:		// critical.
				pszLabel = "CRITICAL:";
				break;
			case LOGL_ERROR:	// non-fatal errors.
				pszLabel = "ERROR:";
				break;
			case LOGL_WARN:
				pszLabel = "WARNING:";
				break;
		}

		// Get the script context. (if there is one)
		tchar szScriptContext[ _MAX_PATH + 16 ];
		if ( !(dwMask & LOGM_NOCONTEXT) && m_pScriptContext )
		{
			CScriptLineContext LineContext = m_pScriptContext->GetContext();
			snprintf( szScriptContext, sizeof(szScriptContext),"(%s,%d)", m_pScriptContext->GetFileTitle(), LineContext.m_iLineNum );
		}
		else
        {
			szScriptContext[0] = '\0';
        }

		// Print to screen.
		if ( !(dwMask & LOGF_LOGFILE_ONLY) )
		{
			if ( !(dwMask & LOGM_INIT) && !g_Serv.IsLoading() )
			{
				SetColor(CTCOL_YELLOW);
				g_Serv.PrintStr( szTime );
				SetColor(CTCOL_DEFAULT);
			}

			if ( pszLabel )	// some sort of error
			{
				SetColor(CTCOL_RED);
				g_Serv.PrintStr( pszLabel );
				if ( (dwMask & 0x07) == LOGL_WARN )
					SetColor(CTCOL_DEFAULT);
				else
					SetColor(CTCOL_WHITE);
			}
			else if ((dwMask & LOGM_DEBUG) && !(dwMask & LOGM_INIT))	// debug log
			{
				pszLabel = "DEBUG:";
				SetColor(CTCOL_MAGENTA);
				g_Serv.PrintStr(pszLabel);
				SetColor(CTCOL_DEFAULT);
			}

			if ( szScriptContext[0] )
			{
				SetColor(CTCOL_CYAN);
				g_Serv.PrintStr( szScriptContext );
				SetColor(CTCOL_DEFAULT);
			}
			g_Serv.PrintStr( pszMsg );

			// Back to normal color.
			SetColor(CTCOL_DEFAULT);
		}
		
		// Print to log file.
		if ( !(dwMask & LOGF_CONSOLE_ONLY) )
		{
			if ( datetime.GetDay() != m_dateStamp.GetDay())
			{
				// it's a new day, open a log file with new day name.
				Close();	// LINUX should alrady be closed.
				OpenLog();
                Printf("Log date: %s\n", m_dateStamp.Format(nullptr));
			}
#ifndef _WIN32
			else
			{
                Close(); // The log file is opened for the first time by the OpenLog call done when reading the sphere.ini.
				uint mode = OF_READWRITE|OF_TEXT|OF_SHARE_DENY_NONE;
				Open(nullptr, mode);	// LINUX needs to close and re-open for each log line !
			}
#endif

			WriteString( szTime );
			if ( pszLabel )
				WriteString( pszLabel );
			if ( szScriptContext[0] )
				WriteString( szScriptContext );

			WriteString( pszMsg );

#ifndef _WIN32
			Close();
#endif
		}

		iRet = 1;

	}
	catch (...)
	{
		// Not much we can do about this
		iRet = 0;
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	m_mutex.unlock();
	return iRet;
}

CSTime CLog::sm_prevCatchTick;

void _cdecl CLog::CatchEvent( const CSError * pErr, lpctstr pszCatchContext, ... )
{
	CSTime timeCurrent = CSTime::GetCurrentTime();
	if ( sm_prevCatchTick.GetTime() == timeCurrent.GetTime() )	// prevent message floods.
		return;
	// Keep a record of what we catch.
	try
	{
		tchar szMsg[512];
		LOG_TYPE eSeverity;
		size_t stLen = 0;
		if ( pErr != nullptr )
		{
			eSeverity = pErr->m_eSeverity;
			const CAssert * pAssertErr = dynamic_cast<const CAssert*>(pErr);
			if (pAssertErr)
				pAssertErr->GetErrorMessage(szMsg, sizeof(szMsg));				
			else
				pErr->GetErrorMessage(szMsg, sizeof(szMsg));
			stLen = strlen(szMsg);
		}
		else
		{
			eSeverity = LOGL_CRIT;
			strcpy(szMsg, "Generic exception");
			stLen = strlen(szMsg);
		}

		stLen += sprintf( szMsg+stLen, ", in " );

		va_list vargs;
		va_start(vargs, pszCatchContext);

		stLen += vsprintf(szMsg+stLen, pszCatchContext, vargs);
		stLen += sprintf(szMsg+stLen, "\n");

		EventStr(eSeverity, szMsg);
		va_end(vargs);
	}
	catch (...)
	{
		// Not much we can do about this.
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	sm_prevCatchTick = timeCurrent;
}

void _cdecl CLog::CatchStdException(const std::exception * pExc, lpctstr pszCatchContext, ...)
{
    tchar szMsg[512];

    va_list vargs;
    va_start(vargs, pszCatchContext);
    vsnprintf(szMsg, sizeof(szMsg), pszCatchContext, vargs);
    va_end(vargs);
    EventStr(LOGL_CRIT, szMsg);

    snprintf(szMsg, sizeof(szMsg), "exception.what(): %s\n", pExc->what());
    EventStr(LOGL_CRIT, szMsg);
}