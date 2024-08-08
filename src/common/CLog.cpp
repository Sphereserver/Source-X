
#include "../sphere/ProfileTask.h"
#include "../game/CServer.h"
#include "CException.h"
#include "CScript.h"
#include "CLog.h"


int CEventLog::VEvent(dword dwMask, lpctstr pszFormat, ConsoleTextColor iColor, va_list args) noexcept
{
    if (pszFormat == nullptr || pszFormat[0] == '\0')
        return 0;

	tchar* pszTemp = Str_GetTemp();
    size_t len = vsnprintf(pszTemp, (Str_TempLength() - 1), pszFormat, args);
    if (! len)
        Str_CopyLimitNull(pszTemp, pszFormat, (Str_TempLength() - 1));

    // This get rids of exploits done sending 0x0C to the log subsytem.
    // tchar *	 pFix;
    // if ( ( pFix = strchr( pszText, 0x0C ) ) )
    //	*pFix = ' ';

    return EventStr(dwMask, pszTemp, iColor);
}

int CEventLog::Event(dword dwMask, lpctstr pszFormat, ...) noexcept
{
    va_list vargs;
    va_start(vargs, pszFormat);
    int iret = VEvent(dwMask, pszFormat, CTCOL_DEFAULT, vargs);
    va_end(vargs);
    return iret;
}

int CEventLog::EventDebug(lpctstr pszFormat, ...) noexcept
{
    va_list vargs;
    va_start(vargs, pszFormat);
    int iret = VEvent(LOGM_DEBUG|LOGM_NOCONTEXT, pszFormat, CTCOL_DEFAULT, vargs);
    va_end(vargs);
    return iret;
}

int CEventLog::EventError(lpctstr pszFormat, ...) noexcept
{
    va_list vargs;
    va_start(vargs, pszFormat);
    int iret = VEvent(LOGL_ERROR, pszFormat, CTCOL_DEFAULT, vargs);
    va_end(vargs);
    return iret;
}

int CEventLog::EventWarn(lpctstr pszFormat, ...) noexcept
{
    va_list vargs;
    va_start(vargs, pszFormat);
    int iret = VEvent(LOGL_WARN, pszFormat, CTCOL_DEFAULT, vargs);
    va_end(vargs);
    return iret;
}

int CEventLog::EventCustom(ConsoleTextColor iColor, dword dwMask, lpctstr pszFormat, ...) noexcept
{
    va_list vargs;
    va_start(vargs, pszFormat);
    int iret = VEvent(dwMask, pszFormat, iColor, vargs);
    va_end(vargs);
    return iret;
}

#ifdef _DEBUG
int CEventLog::EventEvent(lpctstr pszFormat, ...) noexcept
{
    va_list vargs;
    va_start(vargs, pszFormat);
    int iret = VEvent(LOGL_EVENT, pszFormat, CTCOL_DEFAULT, vargs);
    va_end(vargs);
    return iret;
}
#endif //_DEBUG


//-------

CLog::CLog()
{
	m_fLockOpen = false;
	m_pScriptContext = nullptr;
	m_pObjectContext = nullptr;
	m_dwMsgMask = LOGL_ERROR | LOGM_INIT | LOGM_CLIENTS_LOG | LOGM_GM_PAGE;
	SetFilePath( SPHERE_FILE "log.log" );	// default name to go to.
}

const CScript * CLog::_SetScriptContext( const CScript * pScriptContext )
{
	const CScript * pOldScript = m_pScriptContext;
	m_pScriptContext = pScriptContext;
	return pOldScript;
}

const CScript * CLog::SetScriptContext(const CScript * pScriptContext)
{
    MT_UNIQUE_LOCK_RETURN(CLog::_SetScriptContext(pScriptContext));
}

const CScriptObj * CLog::_SetObjectContext( const CScriptObj * pObjectContext )
{
	const CScriptObj * pOldObject = m_pObjectContext;
	m_pObjectContext = pObjectContext;
	return pOldObject;
}

const CScriptObj * CLog::SetObjectContext(const CScriptObj * pObjectContext)
{
    MT_UNIQUE_LOCK_RETURN(CLog::_SetObjectContext(pObjectContext));
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
	return ( m_dwMsgMask & ~(LOGL_QTY|LOGF_QTY) );
}

void CLog::SetLogMask( dword dwMask )
{
	m_dwMsgMask = GetLogLevel() | ( dwMask & ~(LOGL_QTY|LOGF_QTY) );
}

bool CLog::IsLoggedMask( dword dwMask ) const
{
	return ( ((dwMask & ~(LOGL_QTY | LOGF_QTY | LOGM_NOCONTEXT | LOGM_DEBUG)) == 0) ||	// debug and msgs with no context are not masks to be logged?
			 (( GetLogMask() & ( dwMask & ~(LOGL_QTY|LOGF_QTY) )) != 0) );
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
	const bool mask = IsLoggedMask(dwMask);
	const bool lvl = IsLoggedLevel((LOG_TYPE)dwMask);
	return mask || lvl;
}

bool CLog::_OpenLog( lpctstr pszBaseDirName )	// name set previously.
{
	ADDTOCALLSTACK("CLog::_OpenLog");

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
				_Close();
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
	snprintf(pszTemp, Str_TempLength(), SPHERE_FILE "%d-%02d-%02d.log",
		m_dateStamp.GetYear(), m_dateStamp.GetMonth(), m_dateStamp.GetDay());
	CSString sFileName = GetMergedFileName(m_sBaseDir, pszTemp);

	// Use the OF_READWRITE to append to an existing file.
	if ( CSFileText::_Open( sFileName.GetBuffer(), OF_SHARE_DENY_NONE|OF_READWRITE|OF_TEXT ) )
	{
		setvbuf(_pStream, nullptr, _IONBF, 0);
		return true;
	}
	return false;
}

bool CLog::OpenLog(lpctstr pszBaseDirName)	// name set previously.
{
	ADDTOCALLSTACK("CLog::OpenLog");
	MT_UNIQUE_LOCK_RETURN(CLog::_OpenLog(pszBaseDirName));
}

int CLog::EventStr( dword dwMask, lpctstr pszMsg, ConsoleTextColor iLogColor) noexcept
{
    ADDTOCALLSTACK("CLog::EventStr");
	// NOTE: This could be called in odd interrupt context so don't use dynamic stuff
	if ( !IsLogged(dwMask) )	// I don't care about these.
		return 0;
	if ( !pszMsg || !*pszMsg )
		return 0;

	int iRet = 0;

	try
	{
        ConsoleTextColor iLogTextColor = iLogColor;
        ConsoleTextColor iLogTypeColor = CTCOL_DEFAULT;

		// Put up the date/time.
		CSTime datetime = CSTime::GetCurrentTime();	// last real time stamp.

		tchar szTime[32];
		snprintf(szTime, sizeof(szTime), "%02d:%02d:", datetime.GetHour(), datetime.GetMinute());
		lpctstr pszLabel = nullptr;
		switch (dwMask & LOGL_QTY)
		{
			case LOGL_FATAL:	// fatal error !
				pszLabel = "FATAL:";
                iLogTypeColor = CTCOL_RED;
				break;
			case LOGL_CRIT:		// critical.
				pszLabel = "CRITICAL:";
                iLogTypeColor = CTCOL_RED;
				break;
			case LOGL_ERROR:	// non-fatal errors.
				pszLabel = "ERROR:";
                iLogTypeColor = CTCOL_RED;
				break;
			case LOGL_WARN:
                pszLabel = "WARNING:";
                iLogTypeColor = CTCOL_RED;
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
				g_Serv.PrintStr(CTCOL_YELLOW, szTime );
			}

			if ( pszLabel )	// some sort of error
			{
				g_Serv.PrintStr( iLogTypeColor, pszLabel );
                if ((dwMask & 0x07) == LOGL_WARN)
                {
                    iLogTextColor = CTCOL_DEFAULT;
                }
                else
                {
                    iLogTextColor = CTCOL_WHITE;
                }
			}
			else if ((dwMask & LOGM_DEBUG) && !(dwMask & LOGM_INIT))	// debug log
			{
				pszLabel = "DEBUG:";
				g_Serv.PrintStr(CTCOL_MAGENTA, pszLabel);
			}

			if ( szScriptContext[0] )
			{
				g_Serv.PrintStr( CTCOL_CYAN, szScriptContext );
			}
			g_Serv.PrintStr( iLogTextColor, pszMsg );
		}

		// Print to log file.
		if ( !(dwMask & LOGF_CONSOLE_ONLY) )
		{
			MT_UNIQUE_LOCK_SET;

			if ( datetime.GetDay() != m_dateStamp.GetDay())
			{
				// it's a new day, open a log file with new day name.
				_Close();	// LINUX should alrady be closed.
				_OpenLog();
                _Printf("Log date: %s\n", m_dateStamp.Format(nullptr));
			}
#ifndef _WIN32
			else
			{
                _Close(); // The log file is opened for the first time by the OpenLog call done when reading the sphere.ini.
				const uint mode = OF_READWRITE|OF_TEXT|OF_SHARE_DENY_NONE;
				_Open(nullptr, mode);	// LINUX needs to close and re-open for each log line !
			}
#endif

			_WriteString( szTime );
			if ( pszLabel )
				_WriteString( pszLabel );
			if ( szScriptContext[0] )
				_WriteString( szScriptContext );

			_WriteString( pszMsg );

#ifndef _WIN32
			_Close();
#endif
		}

		iRet = 1;

	}
	catch (...)
	{
		// Not much we can do about this
		iRet = 0;
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
	}

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
		size_t uiLen = 0;
		if ( pErr != nullptr )
		{
			eSeverity = pErr->m_eSeverity;
			const CAssert * pAssertErr = dynamic_cast<const CAssert*>(pErr);
			if (pAssertErr)
				pAssertErr->GetErrorMessage(szMsg, sizeof(szMsg));
			else
				pErr->GetErrorMessage(szMsg, sizeof(szMsg));
			uiLen = strlen(szMsg);
		}
		else
		{
			eSeverity = LOGL_CRIT;
			Str_CopyLimitNull(szMsg, "Generic exception", sizeof(szMsg));
			uiLen = strlen(szMsg);
		}

		uiLen += snprintf( szMsg + uiLen, sizeof(szMsg) - uiLen, ", in " );

		va_list vargs;
		va_start(vargs, pszCatchContext);

		uiLen += vsnprintf(szMsg + uiLen, sizeof(szMsg) - uiLen, pszCatchContext, vargs);
		uiLen += snprintf (szMsg + uiLen, sizeof(szMsg) - uiLen, "\n");

		EventStr(eSeverity, szMsg);
		va_end(vargs);
	}
	catch (...)
	{
		// Not much we can do about this.
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
	}
	sm_prevCatchTick = timeCurrent;
}

void _cdecl CLog::CatchStdException(const std::exception * pExc, lpctstr pszCatchContext, ...)
{
    tchar szMsg[512];

    va_list vargs;
    va_start(vargs, pszCatchContext);

    const size_t uiLen = vsnprintf(szMsg, sizeof(szMsg), pszCatchContext, vargs);
    Str_ConcatLimitNull(szMsg, "\n", sizeof(szMsg) - uiLen);

    va_end(vargs);

    EventStr(LOGL_CRIT, szMsg);

    snprintf(szMsg, sizeof(szMsg), "exception.what(): %s\n", pExc->what());
    EventStr(LOGL_CRIT, szMsg);
}
