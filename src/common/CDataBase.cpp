
#include "../game/CServerConfig.h"
#include "../game/CServer.h"
#include "../game/CServerTime.h"
#include "../sphere/asyncdb.h"
#include "CLog.h"
#include "CException.h"
#include "CScriptTriggerArgs.h"
#include "CDataBase.h"

extern CDataBaseAsyncHelper g_asyncHdb;


CDataBase::CDataBase()
{
	m_bConnected = false;
	_myData = nullptr;
}

CDataBase::~CDataBase()
{
	if ( isConnected() )
		Close();
}

bool CDataBase::Connect(const char *user, const char *password, const char *base, const char *host)
{
	ADDTOCALLSTACK("CDataBase::Connect");
	SimpleThreadLock lock(m_connectionMutex);

	m_bConnected = false;

	unsigned long ver = mysql_get_client_version();
	if ( ver < MIN_MYSQL_VERSION_ALLOW )
	{
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Your MySQL client library is too old (version %lu). Minimal allowed version is %d. MySQL support disabled.\n", ver, MIN_MYSQL_VERSION_ALLOW);
		g_Cfg.m_bMySql = false;
		return false;
	}

	_myData = mysql_init(nullptr);
	if ( !_myData )
		return false;

	int portnum = 0;
	const char *port = nullptr;
	if ( (port = strchr(host, ':')) != nullptr )
	{
		char *pcTemp = Str_GetTemp();
		Str_CopyLimitNull(pcTemp, host, STR_TEMPLENGTH);
		*(strchr(pcTemp, ':')) = 0;
		portnum = atoi(port+1);
		host = pcTemp;
	}

	if ( !mysql_real_connect(_myData, host, user, password, base, portnum, nullptr, CLIENT_MULTI_STATEMENTS ) )
	{
		const char *error = mysql_error(_myData);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "MySQL connect fail: %s\n", error);
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "Visit this link for more information: http://dev.mysql.com/doc/mysql/search.php?q=%s\n", error);
		mysql_close(_myData);
		_myData = nullptr;
		return false;
	}

	return (m_bConnected = true);
}

bool CDataBase::Connect()
{
	ADDTOCALLSTACK("CDataBase::Connect");
	return Connect(g_Cfg.m_sMySqlUser, g_Cfg.m_sMySqlPass, g_Cfg.m_sMySqlDB, g_Cfg.m_sMySqlHost);
}

bool CDataBase::isConnected()
{
	ADDTOCALLSTACK("CDataBase::isConnected");
	return m_bConnected;
}

void CDataBase::Close()
{
	ADDTOCALLSTACK("CDataBase::Close");
	SimpleThreadLock lock(m_connectionMutex);
	mysql_close(_myData);
	_myData = nullptr;
	m_bConnected = false;
}

bool CDataBase::query(const char *query, CVarDefMap & mapQueryResult)
{
    ADDTOCALLSTACK("CDataBase::query");
    mapQueryResult.Clear();
    mapQueryResult.SetNumNew("NUMROWS", 0);
    
    if ( !isConnected() )
        return false;
        
    int result;
    MYSQL_RES * m_res = nullptr;
    const char * myErr = nullptr;

    {
        /*
         * connection can only handle one query at a time, so we need to lock until we
         * finish the query -and- retrieve the results
         */
        SimpleThreadLock lock(m_connectionMutex);
        result = mysql_query(_myData, query);
        
        if ( result == 0 )
        {
            m_res = mysql_store_result(_myData);
            if ( m_res == nullptr )
                return false;
        }
        else
        {
            myErr = mysql_error(_myData);
        }
    }
    
    if ( m_res != nullptr )
    {
        MYSQL_FIELD * fields = mysql_fetch_fields(m_res);
        int num_fields = mysql_num_fields(m_res);
        
        mapQueryResult.SetNum("NUMROWS", mysql_num_rows(m_res));
        mapQueryResult.SetNum("NUMCOLS", num_fields);
        
        char key[12];
        char **trow = nullptr;
        int rownum = 0;
        char *zStore = Str_GetTemp();
        char empty = (char)0;
        while ( (trow = mysql_fetch_row(m_res)) != nullptr )
        {
            for ( int i = 0; i < num_fields; ++i )
            {
                char *z = trow[i];
                if (z == nullptr) //We need to check if the row empty, and return char 0 as return, because SetStr clean up \0 chars from vardef and causes the Sphere crash.
                    z = &empty;
                    
                if ( !rownum )
                {
                    mapQueryResult.SetStr(Str_FromI_Fast(i, key, sizeof(key), 10), true, z);
                    mapQueryResult.SetStr(fields[i].name, true, z);
                }
            
                snprintf(zStore, STR_TEMPLENGTH, "%d.%d", rownum, i);
                mapQueryResult.SetStr(zStore, true, z);
                snprintf(zStore, STR_TEMPLENGTH, "%d.%s", rownum, fields[i].name);
                mapQueryResult.SetStr(zStore, true, z);
            }
            ++rownum;
        }
        mysql_free_result(m_res);
        return true;
    }
    else
    {
        g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "MySQL query \"%s\" failed due to \"%s\"\n", query, ( *myErr ? myErr : "unknown reason"));
    }
    
    if (( result == CR_SERVER_GONE_ERROR ) || ( result == CR_SERVER_LOST ))
        Close();
        
    return false;
}

bool __cdecl CDataBase::queryf(CVarDefMap & mapQueryResult, char *fmt, ...)
{
	ADDTOCALLSTACK("CDataBase::queryf");
	TemporaryString tsBuf;
	va_list	marker;
	va_start(marker, fmt);
	vsnprintf(tsBuf.buffer(), tsBuf.capacity(), fmt, marker);
	va_end(marker);
	return this->query(tsBuf, mapQueryResult);
}

bool CDataBase::exec(const char *query)
{
	ADDTOCALLSTACK("CDataBase::exec");
	if ( !isConnected() )
		return false;

	int result = 0;

	{
		// connection can only handle one query at a time, so we need to lock until we finish
		SimpleThreadLock lock(m_connectionMutex);
		result = mysql_query(_myData, query);
		if (result == 0)
		{
			// even though we don't want (or expect) any result data, we must retrieve
			// is anyway otherwise we will lose our connection to the server
			MYSQL_RES* res = mysql_store_result(_myData);
			if (res != nullptr)
				mysql_free_result(res);

			return true;
		}
		else
		{
			const char *myErr = mysql_error(_myData);
			g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "MySQL query \"%s\" failed due to \"%s\"\n",
				query, ( *myErr ? myErr : "unknown reason"));
		}
	}

	if (( result == CR_SERVER_GONE_ERROR ) || ( result == CR_SERVER_LOST ))
		Close();

	return false;
}

bool __cdecl CDataBase::execf(char *fmt, ...)
{
	ADDTOCALLSTACK("CDataBase::execf");
	TemporaryString tsBuf;
	va_list	marker;

	va_start(marker, fmt);
	vsnprintf(tsBuf.buffer(), tsBuf.capacity(), fmt, marker);
	va_end(marker);

	return this->exec(tsBuf);
}

bool CDataBase::addQuery(bool isQuery, lpctstr theFunction, lpctstr theQuery)
{
	if ( g_Cfg.m_Functions.ContainsKey( theFunction ) == false )
	{
		DEBUG_ERR(("Invalid callback function (%s) for AEXECUTE/AQUERY.\n", theFunction));
		return false;
	}
	else
	{
		if ( !g_asyncHdb.isActive() )
			g_asyncHdb.start();

		g_asyncHdb.addQuery(isQuery,theFunction,theQuery);
		return true;
	}
}

void CDataBase::addQueryResult(CSString & theFunction, CScriptTriggerArgs * theResult)
{
	SimpleThreadLock stlThelock(m_resultMutex);

	m_QueryArgs.push(FunctionArgsPair_t(theFunction, theResult));
}

bool CDataBase::OnTick()
{
	ADDTOCALLSTACK("CDataBase::OnTick");
	static int tickcnt = 0;
	EXC_TRY("Tick");

	if ( !g_Cfg.m_bMySql )	//	mySQL is not supported
		return true;

	//	do not ping sql server too heavily
	if ( ++tickcnt >= 1000 )
	{
		tickcnt = 0;

		if ( isConnected() )	//	currently connected - just check that the link is alive
		{
			SimpleThreadLock lock(m_connectionMutex);
			if ( mysql_ping(_myData) )
			{
				g_Log.EventError("MySQL server link has been lost. Trying to reattach to it.\n");
				Close();

				if ( !Connect() )
				{
					g_Log.EventError("MySQL reattach failed/timed out. SQL operations disabled.\n");
				}
			}
		}
	}

	if ( !m_QueryArgs.empty() && !(tickcnt % TICKS_PER_SEC) )
	{
		FunctionArgsPair_t currentPair;
		{
			SimpleThreadLock lock(m_resultMutex);
			currentPair = m_QueryArgs.front();
			m_QueryArgs.pop();
		}

		if ( !g_Serv.r_Call(currentPair.first, &g_Serv, currentPair.second) )
		{
			// error
		}

		ASSERT(currentPair.second != nullptr);
		delete currentPair.second;
	}

	return true;
	EXC_CATCH;

	//EXC_DEBUG_START;
	//EXC_DEBUG_END;
	return false;
}

enum DBO_TYPE
{
	DBO_AEXECUTE,
	DBO_AQUERY,
	DBO_CONNECTED,
	DBO_ESCAPEDATA,
	DBO_ROW,
	DBO_QTY
};

lpctstr const CDataBase::sm_szLoadKeys[DBO_QTY+1] =
{
	"AEXECUTE",
	"AQUERY",
	"CONNECTED",
	"ESCAPEDATA",
	"ROW",
	nullptr
};

enum DBOV_TYPE
{
	DBOV_CLOSE,
	DBOV_CONNECT,
	DBOV_EXECUTE,
	DBOV_QUERY,
	DBOV_QTY
};

lpctstr const CDataBase::sm_szVerbKeys[DBOV_QTY+1] =
{
	"CLOSE",
	"CONNECT",
	"EXECUTE",
	"QUERY",
	nullptr
};

bool CDataBase::r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef)
{
	ADDTOCALLSTACK("CDataBase::r_GetRef");
	UNREFERENCED_PARAMETER(ptcKey);
	UNREFERENCED_PARAMETER(pRef);
	return false;
}

bool CDataBase::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CDataBase::r_LoadVal");
	UNREFERENCED_PARAMETER(s);
	return false;
/*
	lpctstr ptcKey = s.GetKey();
	EXC_TRY("LoadVal");

	int index = FindTableHeadSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1);

	switch ( index )
	{
		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
*/
}

bool CDataBase::r_WriteVal(lpctstr ptcKey, CSString &sVal, CTextConsole *pSrc, bool fNoCallParent, bool fNoCallChildren)
{
    UNREFERENCED_PARAMETER(fNoCallParent);
    UNREFERENCED_PARAMETER(fNoCallChildren);
	ADDTOCALLSTACK("CDataBase::r_WriteVal");
	EXC_TRY("WriteVal");

	// Just return 0 if MySQL is disabled
	if (!g_Cfg.m_bMySql)
	{
		sVal.FormatVal( 0 );
		return true;
	}

	int index = FindTableHeadSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1);
	switch ( index )
	{
		case DBO_AEXECUTE:
		case DBO_AQUERY:
			{
				ptcKey += strlen(sm_szLoadKeys[index]);
				GETNONWHITESPACE(ptcKey);
				sVal.FormatVal(0);

				if ( ptcKey[0] != '\0' )
				{
					tchar * ppArgs[2];
					if ( Str_ParseCmds(const_cast<tchar *>(ptcKey), ppArgs, CountOf( ppArgs )) != 2)
					{
						DEBUG_ERR(("Not enough arguments for %s\n", CDataBase::sm_szLoadKeys[index]));
					}
					else
					{
						sVal.FormatVal( addQuery((index == DBO_AQUERY), ppArgs[0], ppArgs[1]) );
					}
				}
				else
				{
					DEBUG_ERR(("Not enough arguments for %s\n", CDataBase::sm_szLoadKeys[index]));
				}
			} break;

		case DBO_CONNECTED:
			sVal.FormatVal(isConnected());
			break;

		case DBO_ESCAPEDATA:
		{
			ptcKey += strlen(sm_szLoadKeys[index]);
			GETNONWHITESPACE(ptcKey);
			sVal = "";

			if ( ptcKey[0] != '\0' )
			{
				tchar * escapedString = Str_GetTemp();

				SimpleThreadLock lock(m_connectionMutex);
				if ( isConnected() && mysql_real_escape_string(_myData, escapedString, ptcKey, (uint)(strlen(ptcKey))) )
				{
					sVal = escapedString;
				}
			}
		} break;

		case DBO_ROW:
		{
			ptcKey += strlen(sm_szLoadKeys[index]);
			SKIP_SEPARATORS(ptcKey);
			sVal = m_QueryResult.GetKeyStr(ptcKey, true);
		} break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' [%p]\n", ptcKey, static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CDataBase::r_Verb(CScript & s, CTextConsole * pSrc)
{
	ADDTOCALLSTACK("CDataBase::r_Verb");
	EXC_TRY("Verb");

	// Just return true if MySQL is disabled
	if (!g_Cfg.m_bMySql)
		return true;

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys)-1);
	switch ( index )
	{
		case DBOV_CLOSE:
			if ( isConnected() )
				Close();
			break;

		case DBOV_CONNECT:
			if ( isConnected() )
				Close();

			Connect();
			break;

		case DBOV_EXECUTE:
			exec(s.GetArgRaw());
			break;

		case DBOV_QUERY:
			query(s.GetArgRaw(), m_QueryResult);
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}
