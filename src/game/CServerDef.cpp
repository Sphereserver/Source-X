
#include "../common/CException.h"
#include "../common/CExpression.h"
#include "../common/sphereproto.h"
#include "../common/sphereversion.h"
#include "../common/CLog.h"
#include "../common/CScriptTriggerArgs.h"
#include "../sphere/threads.h"
#include "CServer.h"
#include "CServerConfig.h"
#include "CServerDef.h"
#include "CWorldGameTime.h"
#include "CWorld.h"

//	Memory profiling
#ifdef _WIN32	// (Win32)
	#include <process.h>

//	grabbed from platform SDK, psapi.h
	typedef struct _PROCESS_MEMORY_COUNTERS {
		DWORD cb;
		DWORD PageFaultCount;
		SIZE_T PeakWorkingSetSize;
		SIZE_T WorkingSetSize;
		SIZE_T QuotaPeakPagedPoolUsage;
		SIZE_T QuotaPagedPoolUsage;
		SIZE_T QuotaPeakNonPagedPoolUsage;
		SIZE_T QuotaNonPagedPoolUsage;
		SIZE_T PagefileUsage;
		SIZE_T PeakPagefileUsage;
	} PROCESS_MEMORY_COUNTERS, *PPROCESS_MEMORY_COUNTERS;

	//	PSAPI external definitions
	typedef	intptr_t (WINAPI *pfnGetProcessMemoryInfo)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
	static HMODULE m_hmPsapiDll = nullptr;
	static pfnGetProcessMemoryInfo m_GetProcessMemoryInfo = nullptr;
	static PROCESS_MEMORY_COUNTERS	pcnt;
#else			// (Unix)
	#include <sys/resource.h>
#endif
    static bool sm_fHasMemoryInfo = true;		// process memory information is available?

//////////////////////////////////////////////////////////////////////
// -CServerDef

CServerDef::CServerDef( lpctstr pszName, CSocketAddressIP dwIP ) :
	m_ip( dwIP, SPHERE_DEF_PORT )	// SOCKET_LOCAL_ADDRESS
{
	// Statistics.
	memset( m_stStat, 0, sizeof( m_stStat ) );	// THIS MUST BE FIRST !

	SetName( pszName );
	m_timeLastValid = 0;
	_iTimeCreate = CWorldGameTime::GetCurrentTime().GetTimeRaw();

	// Set default time zone from UTC
    m_TimeZone = (char)( TIMEZONE / (60 * 60) );	// Greenwich mean time.
	m_eAccApp = ACCAPP_Unspecified;
}

size_t CServerDef::StatGet(SERV_STAT_TYPE i) const
{
	ADDTOCALLSTACK("CServerDef::StatGet");
	ASSERT( i >= 0 && i < SERV_STAT_QTY );
	size_t	d = m_stStat[i];

	EXC_TRY("StatGet");
	if ( i == SERV_STAT_MEM )	// memory information
	{
		d = 0;
        if ( sm_fHasMemoryInfo )
		{
#ifdef _WIN32
			if ( !m_hmPsapiDll )			// try to load psapi.dll if not loaded yet
			{
				EXC_SET_BLOCK("load process info");
				m_hmPsapiDll = LoadLibrary(TEXT("psapi.dll"));
				if (m_hmPsapiDll == nullptr)
				{
                    sm_fHasMemoryInfo = false;
					g_Log.EventError(("Unable to load process information PSAPI.DLL library. Memory information will be not available.\n"));
				}
				else
                {
#ifdef NON_MSVC_COMPILER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
                    m_GetProcessMemoryInfo = reinterpret_cast<pfnGetProcessMemoryInfo>(::GetProcAddress(m_hmPsapiDll,"GetProcessMemoryInfo"));
#if NON_MSVC_COMPILER
#pragma GCC diagnostic pop
#endif
                }
			}

			if ( m_GetProcessMemoryInfo )
			{
				EXC_SET_BLOCK("open process");
				HANDLE hProcess = GetCurrentProcess();
				if ( hProcess )
				{
					ASSERT( hProcess == (HANDLE)-1 );
					EXC_SET_BLOCK("get memory info");
					if ( m_GetProcessMemoryInfo(hProcess, &pcnt, sizeof(pcnt)) )
					{
						EXC_SET_BLOCK("read memory info");
						d = pcnt.WorkingSetSize;
					}
                    if (hProcess != nullptr)
                    {
                        CloseHandle(hProcess);
                    }
				}
			}
            if ( d != 0 )
				d /= 1024;
#else
			{
				CSFileText inf;
                char buf[50];
				char * head;
				if ( inf.Open("/proc/self/status", OF_READ|OF_TEXT) )
				{
					for (;;)
					{
						if ( !inf.ReadString(buf, sizeof(buf)) )
							break;

						if ( (head = strstr(buf, "VmRSS:")) != nullptr )
						{
							head += 6;
                            GETNONWHITESPACE(head);
                            char* head_stop = head;
                            while (head_stop ++) {
                                if (!isdigit(*head_stop))
                                    break;
                            }
                            *head_stop = '\0';
							d = atoi(head); // In kB
							break;
						}
					}
					inf.Close();
				}
			}

			if ( !d )
			{
				g_Log.EventError(("Unable to load process information from getrusage() and procfs. Memory information will be not available.\n"));
                sm_fHasMemoryInfo = false;
			}
#endif
		}
	}
	return d;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("stat '%d', val '%" PRIuSIZE_T "'\n", i, d);
	EXC_DEBUG_END;
	return 0;
}

void CServerDef::SetName( lpctstr pszName )
{
	ADDTOCALLSTACK("CServerDef::SetName");
	if ( ! pszName )
		return;

	// No HTML tags using <> either.
	tchar szName[ 2*MAX_SERVER_NAME_SIZE ];
	const int len = Str_GetBare( szName, pszName, sizeof(szName), "<>/\"\\" );
	if ( len <= 0 )
		return;

	// allow just basic chars. No spaces, only numbers, letters and underbar.
	if ( g_Cfg.IsObscene( szName ) )
	{
		DEBUG_ERR(( "Obscene server '%s' ignored.\n", szName ));
		return;
	}

	m_sName = szName;
}

void CServerDef::StatInc(SERV_STAT_TYPE i)
{
    ASSERT(i >= 0 && i < SERV_STAT_QTY);
    ++m_stStat[i];
}

void CServerDef::StatDec(SERV_STAT_TYPE i)
{
    ASSERT(i >= 0 && i < SERV_STAT_QTY);
    --m_stStat[i];
}

void CServerDef::SetStat(SERV_STAT_TYPE i, size_t uiVal)
{
    ASSERT(i >= 0 && i < SERV_STAT_QTY);
    m_stStat[i] = uiVal;
}

lpctstr CServerDef::GetName() const // virtual override
{
    return m_sName;
}

lpctstr CServerDef::GetStatus() const noexcept
{
    return m_sStatus;
}

bool CServerDef::IsConnected() const noexcept
{
    return m_timeLastValid > 0;
}

void CServerDef::SetCryptVersion()
{
    m_ClientVersion.SetClientVerFromString(m_sClientVersion.GetBuffer());
}

void CServerDef::SetValidTime()
{
	ADDTOCALLSTACK("CServerDef::SetValidTime");
	m_timeLastValid = CWorldGameTime::GetCurrentTime().GetTimeRaw();
}

int64 CServerDef::GetTimeSinceLastValid() const
{
	ADDTOCALLSTACK("CServerDef::GetTimeSinceLastValid");
	return CWorldGameTime::GetCurrentTime().GetTimeDiff( m_timeLastValid );
}

enum SC_TYPE
{
	SC_ACCAPP,
	SC_ACCAPPS,
	SC_ACCOUNTS,
	SC_ADMINEMAIL,
	SC_AGE,
	SC_CHARS,
	SC_CLIENTS,
	SC_CLIENTVERSION,
	SC_CREATE,
	SC_GMPAGES,
	SC_ITEMS,
	SC_LANG,
	SC_LASTVALIDDATE,
	SC_LASTVALIDTIME,
	SC_MEM,
	SC_NAME,
	SC_SERVIP,
	SC_SERVNAME,
	SC_SERVPORT,
	SC_TIMEZONE,
	SC_URL,			// m_sURL
	SC_URLLINK,
	SC_VERSION,
	SC_QTY
};

lpctstr const CServerDef::sm_szLoadKeys[SC_QTY+1] =	// static
{
	"ACCAPP",
	"ACCAPPS",
	"ACCOUNTS",
	"ADMINEMAIL",
	"AGE",
	"CHARS",
	"CLIENTS",
	"CLIENTVERSION",
	"CREATE",
	"GMPAGES",
	"ITEMS",
	"LANG",
	"LASTVALIDDATE",
	"LASTVALIDTIME",
	"MEM",
	"NAME",
	"SERVIP",
	"SERVNAME",
	"SERVPORT",
	"TIMEZONE",
	"URL",			// m_sURL
	"URLLINK",
	"VERSION",
	nullptr
};

static lpctstr constexpr sm_AccAppTable[ ACCAPP_QTY ] =
{
	"CLOSED",		// Closed. Not accepting more.
	"UNUSED",
	"FREE",			// Anyone can just log in and create a full account.
	"GUESTAUTO",	// You get to be a guest and are automatically sent email with u're new password.
	"GUESTTRIAL",	// You get to be a guest til u're accepted for full by an Admin.
	"UNUSED",
	"UNSPECIFIED",	// Not specified.
	"UNUSED",
	"UNUSED"
};

bool CServerDef::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CServerDef::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ) )
	{
		case SC_ACCAPP:
		case SC_ACCAPPS:
        {
            lpctstr ptcArg = s.GetArgStr();
			// Treat it as a value or a string.
			if ( IsDigit( ptcArg[0] ))
            {
				m_eAccApp = ACCAPP_TYPE(s.GetArgVal() );
            }
			else
			{
				// Treat it as a string. "Manual","Automatic","Guest"
				m_eAccApp = ACCAPP_TYPE(FindTable(ptcArg, sm_AccAppTable, ARRAY_COUNT(sm_AccAppTable)));
			}
			if ( m_eAccApp < 0 || m_eAccApp >= ACCAPP_QTY )
				m_eAccApp = ACCAPP_Unspecified;
        }
			break;
		case SC_AGE:
			break;
		case SC_CLIENTVERSION:
			m_sClientVersion = s.GetArgRaw();
			// m_ClientVersion.SetClientVerNumber( s.GetArgRaw());
			break;
		case SC_ADMINEMAIL:
        {
            lpctstr ptcArg = s.GetArgStr();
			if ( this != &g_Serv && !g_Serv.m_sEMail.IsEmpty() && strstr(ptcArg, g_Serv.m_sEMail) )
				return false;
			if ( !g_Cfg.IsValidEmailAddressFormat(ptcArg) )
				return false;
			if ( g_Cfg.IsObscene(ptcArg) )
				return false;
			m_sEMail = ptcArg;
        }
			break;
		case SC_LANG:
			{
				tchar szLang[ 32 ];
				Str_GetBare( szLang, s.GetArgStr(), sizeof(szLang), "<>/\"\\" );
				if ( g_Cfg.IsObscene(szLang) )	// Is the name unacceptable?
					return false;
				m_sLang = szLang;
			}
			break;
		case SC_SERVIP:
			m_ip.SetHostPortStr( s.GetArgStr() );
			break;

		case SC_NAME:
		case SC_SERVNAME:
			SetName( s.GetArgStr() );
			break;
		case SC_SERVPORT:
			m_ip.SetPort( s.GetArgWVal() );
			break;

		case SC_ACCOUNTS:
			SetStat( SERV_STAT_ACCOUNTS, s.GetArgVal() );
			break;

		case SC_CLIENTS:
			{
				int iClients = s.GetArgVal();
				if ( iClients < 0 )
					return false;				// invalid
				if ( iClients > FD_SETSIZE )	// Number is bugged !
					return false;
				SetStat( SERV_STAT_CLIENTS, iClients );
			}
			break;
		case SC_ITEMS:
			SetStat( SERV_STAT_ITEMS, s.GetArgDWVal() );
			break;
		case SC_CHARS:
			SetStat( SERV_STAT_CHARS, s.GetArgDWVal() );
			break;
		case SC_TIMEZONE:
			m_TimeZone = (char)s.GetArgVal();
			break;
		case SC_URL:
		case SC_URLLINK:
			// It is a basically valid URL ?
			if ( this != &g_Serv )
			{
				if ( !g_Serv.m_sURL.IsEmpty() && strstr(s.GetArgStr(), g_Serv.m_sURL) )
					return false;
			}
			if ( !strchr(s.GetArgStr(), '.' ) )
				return false;
			if ( g_Cfg.IsObscene( s.GetArgStr()) )	// Is the name unacceptable?
				return false;
			m_sURL = s.GetArgStr();
			break;
		default:
			return CScriptObj::r_LoadVal(s);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CServerDef::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
	ADDTOCALLSTACK("CServerDef::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ) )
	{
	case SC_ACCAPP:
		sVal.FormatVal( m_eAccApp );
		break;
	case SC_ACCAPPS:
		// enum string
		ASSERT( m_eAccApp >= 0 && m_eAccApp < ACCAPP_QTY );
		sVal = sm_AccAppTable[ m_eAccApp ];
		break;
	case SC_ADMINEMAIL:
		sVal = m_sEMail;
		break;
	case SC_AGE:
		// display the age in days.
		sVal.FormatLLVal( GetAgeHours()/24 );
		break;
	case SC_CLIENTVERSION:
		{
			sVal = m_ClientVersion.GetClientVer().c_str();
		}
		break;
	case SC_CREATE:
		sVal.FormatLLVal( CWorldGameTime::GetCurrentTime().GetTimeDiff(_iTimeCreate) / MSECS_PER_TENTH );
		break;
	case SC_LANG:
		sVal = m_sLang;
		break;

	case SC_LASTVALIDDATE:
		if ( m_timeLastValid > 0 )
			sVal.FormatLLVal( GetTimeSinceLastValid() / (MSECS_PER_SEC * 60 ) );
		else
			sVal = "NA";
		break;
	case SC_LASTVALIDTIME:
		// How many seconds ago.
		sVal.FormatLLVal( m_timeLastValid > 0 ? ( GetTimeSinceLastValid() / MSECS_PER_SEC) : -1 );
		break;
	case SC_SERVIP:
		sVal = m_ip.GetAddrStr();
		break;
	case SC_NAME:
	case SC_SERVNAME:
		sVal = GetName();	// What the name should be. Fill in from ping.
		break;
	case SC_SERVPORT:
		sVal.FormatWVal( m_ip.GetPort() );
		break;
	case SC_ACCOUNTS:
		sVal.FormatSTVal( StatGet( SERV_STAT_ACCOUNTS ) );
		break;
	case SC_CLIENTS:
		sVal.FormatSTVal( StatGet( SERV_STAT_CLIENTS ) );
		break;
	case SC_ITEMS:
		sVal.FormatSTVal( StatGet( SERV_STAT_ITEMS ) );
		break;
	case SC_MEM:
		sVal.FormatSTVal( StatGet( SERV_STAT_MEM ) );
		break;
	case SC_CHARS:
		sVal.FormatSTVal( StatGet( SERV_STAT_CHARS ) );
		break;
	case SC_GMPAGES:
		sVal.FormatSTVal( g_World.m_GMPages.size() );
		break;
	case SC_TIMEZONE:
		sVal.FormatVal( m_TimeZone );
		break;
	case SC_URL:
		sVal = m_sURL;
		break;
	case SC_URLLINK:
		// try to make a link of it.
		if ( m_sURL.IsEmpty() )
		{
			sVal = GetName();
			break;
		}
		sVal.Format("<a href=\"http://%s\">%s</a>", static_cast<lpctstr>(m_sURL), GetName());
		break;
	case SC_VERSION:
		sVal = SPHERE_BUILD_INFO_STR;
		break;
	default:
        if (!fNoCallChildren)
	    {
            const size_t uiFunctionIndex = r_GetFunctionIndex(ptcKey);
            if (r_CanCall(uiFunctionIndex))
            {
                // RES_FUNCTION call
			    lpctstr pszArgs = strchr(ptcKey, ' ');
			    if (pszArgs != nullptr)
				    GETNONWHITESPACE(pszArgs);

			    CScriptTriggerArgs Args( pszArgs ? pszArgs : "" );
			    if ( r_Call( uiFunctionIndex, pSrc, &Args, &sVal ) )
				    return true;
		    }
            return (fNoCallParent ? false : CScriptObj::r_WriteVal( ptcKey, sVal, pSrc, false ));
        }
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

int64 CServerDef::GetAgeHours() const
{
	ADDTOCALLSTACK("CServerDef::GetAgeHours");
	// This is just the amount of time it has been listed.
	return ( CWorldGameTime::GetCurrentTime().GetTimeDiff(_iTimeCreate) / ( MSECS_PER_SEC * 60 * 60 ));
}
