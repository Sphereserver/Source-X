#include "../../../game/chars/CChar.h"
#include "../../../game/clients/CClient.h"
#include "../../../game/items/CItemStone.h"
#include "../../../game/CServer.h"
#include "../../../game/CWorld.h"
#include "../../../game/CWorldGameTime.h"
#include "../../../network/CClientIterator.h"
#include "../../sphere_library/CSFileList.h"
#include "../../CException.h"
#include "../../CExpression.h"
#include "../../sphereversion.h"
#include "../CResourceLock.h"
#include "CWebPageDef.h"


enum WV_TYPE
{
	WV_CLIENTLIST,
	WV_GMPAGELIST,
	WV_GUILDLIST,
	WV_TOWNLIST,
	WV_WEBPAGE,
	WV_QTY
};

lpctstr const CWebPageDef::sm_szVerbKeys[WV_QTY+1] =
{
	"CLIENTLIST",	// make a table of all the clients.
	"GMPAGELIST",	// make a table of the gm pages.
	"GUILDLIST",	// make a table of the guilds.
	"TOWNLIST",		// make a table of the towns.
	"WEBPAGE",		// feed a web page to the source caller
	nullptr
};


//********************************************************
// -CSFileConsole

class CSFileConsole : public CTextConsole
{
public:
	static const char *m_sClassName;
	CSFileText m_FileOut;

public:
	CSFileConsole() = default;
    virtual ~CSFileConsole();
    CSFileConsole(const CSFileConsole& copy) = delete;
	CSFileConsole& operator=(const CSFileConsole& other) = delete;


	virtual PLEVEL_TYPE GetPrivLevel() const
	{
		return PLEVEL_Admin;
	}
	virtual lpctstr GetName() const
	{
		return "WebFile";
	}
	virtual void SysMessage( lpctstr pszMessage ) const
	{
		if ( pszMessage == nullptr )
			return;
		(const_cast <CSFileConsole*>(this))->m_FileOut.WriteString(pszMessage);
	}
};

CSFileConsole::~CSFileConsole() = default;

//********************************************************
// -CWebPageDef

int CWebPageDef::sm_iListIndex;

CWebPageDef::CWebPageDef( CResourceID rid ) : CResourceLink( rid )
{
	// Web page m_sWebPageFilePath
	m_type = WEBPAGE_TEMPLATE;
	m_privlevel=PLEVEL_Guest;

	m_timeNextUpdate = 0;
	m_iUpdatePeriod = 2*60; // in seconds
	m_iUpdateLog = 0;

	// default source name
	if ( rid.GetResIndex() )
		m_sSrcFilePath.Format( SPHERE_FILE "statusbase%d.htm", rid.GetResIndex());
	else
		m_sSrcFilePath = SPHERE_FILE "statusbase.htm";
}

enum WC_TYPE
{
	WC_PLEVEL,
	WC_WEBPAGEFILE,
	WC_WEBPAGELOG,
	WC_WEBPAGESRC,
	WC_WEBPAGEUPDATE,
	WC_QTY
};

lpctstr const CWebPageDef::sm_szLoadKeys[WC_QTY+1] =
{
	"PLEVEL",				// What priv level does one need to be to view this page.
	"WEBPAGEFILE",			// For periodic generated pages.
	"WEBPAGELOG",			// daily log a copy of this page.
	"WEBPAGESRC",			// the source name of the web page.
	"WEBPAGEUPDATE",		// how often to generate a page ? (seconds)
	nullptr
};

bool CWebPageDef::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallChildren);
	ADDTOCALLSTACK("CWebPageDef::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
	{
		case WC_PLEVEL:
			sVal.FormatVal( m_privlevel );
			break;
		case WC_WEBPAGEFILE:
			sVal = m_sDstFilePath;
			break;
		case WC_WEBPAGELOG:
			sVal.FormatVal( m_iUpdateLog );
			break;
		case WC_WEBPAGESRC:
			sVal = m_sSrcFilePath;
			break;
		case WC_WEBPAGEUPDATE:	// (seconds)
			sVal.FormatVal( m_iUpdatePeriod );
			break;
		default:
			return ( fNoCallParent ? false : g_Serv.r_WriteVal( ptcKey, sVal, pSrc ) );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CWebPageDef::r_LoadVal( CScript & s ) // Load an item Script
{
	ADDTOCALLSTACK("CWebPageDef::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
	{
		case WC_PLEVEL:
			m_privlevel = static_cast<PLEVEL_TYPE>(s.GetArgVal());
			break;
		case WC_WEBPAGEFILE:
			m_sDstFilePath = s.GetArgStr();
			break;
		case WC_WEBPAGELOG:
			m_iUpdateLog = s.GetArgVal();
			break;
		case WC_WEBPAGESRC:
			return SetSourceFile( s.GetArgStr(), nullptr );
		case WC_WEBPAGEUPDATE:	// (seconds)
			m_iUpdatePeriod = s.GetArgVal();
			if ( m_iUpdatePeriod && (m_type == WEBPAGE_TEXT) )
			{
				m_type = WEBPAGE_TEMPLATE;
			}
			break;
		default:
			return CScriptObj::r_LoadVal( s );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CWebPageDef::r_Verb( CScript & s, CTextConsole * pSrc )	// some command on this object as a target
{
	ADDTOCALLSTACK("CWebPageDef::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	sm_iListIndex = 0;
	tchar *pszTmp2 = Str_GetTemp();

	WV_TYPE iHeadKey = (WV_TYPE) FindTableSorted( s.GetKey(), sm_szVerbKeys, ARRAY_COUNT(sm_szVerbKeys)-1 );
	switch ( iHeadKey )
	{
		case WV_WEBPAGE:
			{
			// serv a web page to the pSrc
				CClient * pClient = dynamic_cast <CClient *>(pSrc);
				if ( pClient == nullptr )
					return false;
				//return ServPage( pClient, s.GetArgStr(), nullptr );
                ServPage(pClient, s.GetArgStr(), nullptr);
                break;
			}

		case WV_CLIENTLIST:
			{
				ClientIterator it;
				for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
				{
					CChar * pChar = pClient->GetChar();
					if ( pChar == nullptr )
						continue;
					if (( pChar->IsStatFlag(STATF_INSUBSTANTIAL) ) && (!pChar->IsStatFlag(STATF_DEAD)))
						continue;

					++sm_iListIndex;

					lpctstr pszArgs = s.GetArgStr();
					if ( pszArgs[0] == '\0' )
						pszArgs = "<tr><td>%NAME%</td><td>%REGION.NAME%</td></tr>\n";
					Str_CopyLimitNull( pszTmp2, pszArgs, Str_TempLength() );
					pChar->ParseScriptText( Str_MakeFiltered( pszTmp2 ), &g_Serv, 1 );
					pSrc->SysMessage( pszTmp2 );
				}
			}
			break;

		case WV_GUILDLIST:
		case WV_TOWNLIST:
			{
				if ( !s.HasArgs() )
					return false;

				IT_TYPE	needtype = ( iHeadKey == WV_GUILDLIST ) ? IT_STONE_GUILD : IT_STONE_TOWN;

				for ( size_t i = 0; i < g_World.m_Stones.size(); ++i )
				{
					CItemStone * pStone = g_World.m_Stones[i].get();
					if ( !pStone || !pStone->IsType(needtype) )
						continue;

					++sm_iListIndex;

					Str_CopyLimitNull(pszTmp2, s.GetArgStr(), Str_TempLength());
					pStone->ParseScriptText(Str_MakeFiltered(pszTmp2), &g_Serv, 1);
					pSrc->SysMessage(pszTmp2);
				}
			}
			break;

		case WV_GMPAGELIST:
			{
				if ( ! s.HasArgs())
					return false;
				for (auto &sptrPage : g_World.m_GMPages)
				{
					CGMPage* pPage = sptrPage.get();
					++sm_iListIndex;
					Str_CopyLimitNull( pszTmp2, s.GetArgStr(), Str_TempLength());
					pPage->ParseScriptText( Str_MakeFiltered( pszTmp2 ), &g_Serv, 1 );
					pSrc->SysMessage( pszTmp2 );
				}
			}
			break;

		default:
			return CResourceLink::r_Verb(s,pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CWebPageDef::WebPageUpdate( bool fNow, lpctstr pszDstName, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CWebPageDef::WebPageUpdate");
	// Generate the status web pages.
	// Read in the Base status page "*STATUSBASE.HTM"
	// Filter the XML type tags.
	// Output the status page "*.HTM"
	// Server name
	// Server email
	// Number of clients, items, NPC's

	if ( ! fNow )
	{
		if ( m_iUpdatePeriod <= 0 )
			return false;
		if (CWorldGameTime::GetCurrentTime().GetTimeRaw() < m_timeNextUpdate )
			return true;	// should still be valid
	}

	ASSERT(pSrc);
	m_timeNextUpdate = CWorldGameTime::GetCurrentTime().GetTimeRaw() + (m_iUpdatePeriod * MSECS_PER_SEC);
	if ( pszDstName == nullptr )
		pszDstName = m_sDstFilePath;

	if ( m_type != WEBPAGE_TEMPLATE ||
		*pszDstName == '\0' ||
		m_sSrcFilePath.IsEmpty())
		return false;

	CScript FileRead;
	if ( ! FileRead.Open( m_sSrcFilePath, OF_READ|OF_TEXT|OF_DEFAULTMODE ))
		return false;

	CScriptFileContext context( &FileRead );	// set this as the context.

	CSFileConsole FileOut;
	if ( ! FileOut.m_FileOut.Open( pszDstName, OF_WRITE|OF_TEXT ))
	{
		DEBUG_ERR(( "Can't open web page output '%s'\n", pszDstName ));
		return false;
	}

	bool fScriptMode = false;

	while ( FileRead.ReadTextLine( false ))
	{
		tchar *pszTmp = Str_GetTemp();
		Str_CopyLimitNull( pszTmp, FileRead.GetKey(), Str_TempLength());

		tchar * pszHead = strstr( pszTmp, "<script language=\"Sphere\">" );
		if ( pszHead != nullptr )
		{
			// Deal with the stuff preceding the scripts.
			*pszHead = '\0';
			pszHead += 26;
			ParseScriptText( pszTmp, pSrc, 1 );
			FileOut.SysMessage( pszTmp );
			fScriptMode = true;
		}
		else
		{
			pszHead = pszTmp;
		}

		// Look for the end of </script>
		if ( fScriptMode )
		{
			GETNONWHITESPACE(pszHead);
			tchar * pszFormat = pszHead;

			pszHead = strstr( pszFormat, "</script>" );
			if ( pszHead != nullptr )
			{
				*pszHead = '\0';
				pszHead += 9;
				fScriptMode = false;
			}

			if ( pszFormat[0] != '\0' )
			{
				// Allow if/then logic ??? OnTriggerRun( CScript &s, TRIGRUN_SINGLE_EXEC, &FileOut )
				CScript script( pszFormat );
				if ( ! r_Verb( script, &FileOut ))
				{
					DEBUG_ERR(( "Web page source format error '%s'\n", static_cast<lpctstr>(pszTmp) ));
					continue;
				}
			}

			if ( fScriptMode )
				continue;
		}

		// Look for stuff we can displace here. %STUFF%
		ParseScriptText( pszHead, pSrc, 1 );
		FileOut.SysMessage( pszHead );
	}

	return true;
}

void CWebPageDef::WebPageLog()
{
	ADDTOCALLSTACK("CWebPageDef::WebPageLog");
	if ( ! m_iUpdateLog || ! m_iUpdatePeriod )
		return;
	if ( m_type != WEBPAGE_TEMPLATE )
		return;

	CSFileText FileRead;
	if ( ! FileRead.Open( m_sDstFilePath, OF_READ|OF_TEXT ))
		return;

	lpctstr pszExt = FileRead.GetFileExt();

	tchar szName[ _MAX_PATH ];
	Str_CopyLimitNull( szName, m_sDstFilePath, _MAX_PATH);
	szName[ m_sDstFilePath.GetLength() - strlen(pszExt) ] = '\0';

	CSTime datetime = CSTime::GetCurrentTime();

	tchar *pszTemp = Str_GetTemp();
	snprintf(pszTemp, Str_TempLength(), "%s%d%02d%02d%s", szName, datetime.GetYear()%100, datetime.GetMonth(), datetime.GetDay(), pszExt);

	CSFileText FileTest;
	if ( FileTest.Open(pszTemp, OF_READ|OF_TEXT) )
		return;

	// Copy it.
	WebPageUpdate(true, pszTemp, &g_Serv);
}

lpctstr const CWebPageDef::sm_szPageExt[] =
{
	".BMP",
	".GIF",
	".HTM",
	".HTML",
	".JPEG",
	".JPG",
	".JS",
	".TXT",
};

bool CWebPageDef::SetSourceFile( lpctstr pszName, CClient * pClient )
{
	ADDTOCALLSTACK("CWebPageDef::SetSourceFile");
	static WEBPAGE_TYPE const sm_szPageExtType[] =
	{
		WEBPAGE_BMP,
		WEBPAGE_GIF,
		WEBPAGE_TEMPLATE,
        WEBPAGE_TEMPLATE,
		WEBPAGE_JPG,
		WEBPAGE_JPG,
		WEBPAGE_TEXT,
		WEBPAGE_TEXT
	};

	// attempt to set this to a source file.
	// test if it exists.
	size_t iLen = strlen( pszName );
	if ( iLen <= 3 )
		return false;

	lpctstr pszExt = CSFile::GetFilesExt( pszName );
	if ( pszExt == nullptr || pszExt[0] == '\0' )
		return false;

	int iType = FindTableSorted( pszExt, sm_szPageExt, ARRAY_COUNT( sm_szPageExt ));
	if ( iType < 0 )
		return false;
	m_type = sm_szPageExtType[iType];

	if ( pClient == nullptr )
	{
		// this is being set via the Script files.
		// make sure the file is valid
		CScript FileRead;
		if ( ! g_Cfg.OpenResourceFind( FileRead, pszName ))
		{
			DEBUG_ERR(( "Can't open web page input '%s'\n", static_cast<lpctstr>(m_sSrcFilePath) ));
			return false;
		}
		m_sSrcFilePath = FileRead.GetFilePath();
	}
	else
	{
		if ( *pszName == '/' )
			pszName ++;
		if ( strstr( pszName, ".." ))	// this sort of access is not allowed.
			return false;
		if ( strstr( pszName, "\\\\" ))	// this sort of access is not allowed.
			return false;
		if ( strstr( pszName, "//" ))	// this sort of access is not allowed.
			return false;
		m_sSrcFilePath = CSFile::GetMergedFileName( g_Cfg.m_sSCPBaseDir, pszName );
	}

	return true;
}

bool CWebPageDef::IsMatch( lpctstr pszMatch ) const
{
	ADDTOCALLSTACK("CWebPageDef::IsMatch");
	if ( pszMatch == nullptr )	// match all.
		return true;

	lpctstr pszDstName = GetDstName();
	lpctstr pszTry;

	if ( pszDstName[0] )
	{
		// Page is generated periodically. match the output name
		pszTry = CScript::GetFilesTitle( pszDstName );
	}
	else
	{
		// match the source name
		pszTry = CScript::GetFilesTitle( GetName());
	}

	return( ! strcmpi( pszTry, pszMatch ));
}

lpctstr const CWebPageDef::sm_szPageType[WEBPAGE_QTY+1] =
{
	"text/html",		// WEBPAGE_TEMPLATE (.htm)
	"text/html",		// WEBPAGE_TEMPLATE (.html)
	"image/x-xbitmap",	// WEBPAGE_BMP,
	"image/gif",		// WEBPAGE_GIF,
	"image/jpeg",		// WEBPAGE_JPG,
	nullptr				// WEBPAGE_QTY
};

lpctstr const CWebPageDef::sm_szTrigName[WTRIG_QTY+1] =	// static
{
	"@AAAUNUSED",
	"@LOAD",
	nullptr
};

int CWebPageDef::ServPageRequest( CClient * pClient, lpctstr pszURLArgs, CSTime * pdateIfModifiedSince )
{
	ADDTOCALLSTACK("CWebPageDef::ServPageRequest");
	UnreferencedParameter(pszURLArgs);
	// Got a web page request from the client.
	// ARGS:
	//  pszURLArgs = args on the URL line ex. http://www.hostname.com/dir?args
	// RETURN:
	//  HTTP error code = 0-200 page was served.

	ASSERT(pClient);

	if ( HasTrigger(WTRIG_Load))
	{
		CResourceLock s;
		if ( ResourceLock(s))
		{
			if (CScriptObj::OnTriggerScript( s, sm_szTrigName[WTRIG_Load], pClient, nullptr ) == TRIGRET_RET_TRUE)
				return 0;	// Block further action.
		}
	}

	if ( m_privlevel )
	{
		if ( pClient->GetAccount() == nullptr )
			return 401;	// Authorization required
		if ( pClient->GetPrivLevel() < m_privlevel )
			return 403;	// Forbidden
	}

	CSTime datetime = CSTime::GetCurrentTime();

	lpctstr pszName;
	bool fGenerate = false;

	if ( m_type == WEBPAGE_TEMPLATE ) // my version of cgi
	{
		pszName = GetDstName();
		if ( pszName[0] == '\0' )
		{
			pszName = "temppage.htm";
			fGenerate = true;
		}
		else
		{
			fGenerate = ! m_iUpdatePeriod;
		}

		// The page must be generated on demand.
		if ( ! WebPageUpdate( fGenerate, pszName, pClient ))
			return 500;
	}
	else
	{
		pszName = GetName();
	}

	// Get proper Last-Modified: time.
	time_t dateChange;
	dword dwSize;
	if ( ! CSFileList::ReadFileInfo( pszName, dateChange, dwSize ))
	{
		return 500;
	}

	const char *pcDate = datetime.FormatGmt(nullptr);	// current date.
    // If pdateIfModifiedSince is not valid, it means that "If-Modified-Since:" wasn't sent by the web browser, so we need to send the whole page in any case here.
	if ( pdateIfModifiedSince->IsTimeValid() && !fGenerate && (pdateIfModifiedSince->GetTime() <= dateChange) )
	{
		tchar *pszTemp = Str_GetTemp();
		snprintf(pszTemp, Str_TempLength(),
			"HTTP/1.1 304 Not Modified\r\nDate: %s\r\nServer: " SPHERE_TITLE " " SPHERE_BUILD_NAME_VER_PREFIX SPHERE_BUILD_INFO_STR "\r\nContent-Length: 0\r\n\r\n", pcDate);
		new PacketWeb(pClient, (byte*)pszTemp, (uint)strlen(pszTemp));
		return 0;
	}

	// Now serve up the page.
	CSFile FileRead;
	if ( ! FileRead.Open( pszName, OF_READ|OF_BINARY ))
		return 500;

	// Send the header first.
    static constexpr size_t uiWebDataBufSize = 8 * 1024;
    std::unique_ptr<tchar[]> ptcWebDataBuf = std::make_unique<tchar[]>(uiWebDataBufSize);
	tchar* szTmp = ptcWebDataBuf.get();

	int iLen = snprintf(szTmp, uiWebDataBufSize,
		"HTTP/1.1 200 OK\r\n" // 100 Continue
		"Date: %s\r\n"
		"Server: " SPHERE_TITLE " " SPHERE_BUILD_NAME_VER_PREFIX SPHERE_BUILD_INFO_STR "\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Type: %s\r\n",
		pcDate,
		sm_szPageType[m_type] // type of the file. image/gif, image/x-xbitmap, image/jpeg
		);

	if ( m_type == WEBPAGE_TEMPLATE )
		iLen += snprintf(szTmp + iLen, uiWebDataBufSize - iLen, "Expires: 0\r\n");
	else
		iLen += snprintf(szTmp + iLen, uiWebDataBufSize - iLen, "Last-Modified: %s\r\n",  CSTime(dateChange).FormatGmt(nullptr));

	iLen += snprintf( szTmp + iLen, uiWebDataBufSize - iLen,
		"Content-Length: %u\r\n"
		"\r\n",
		dwSize
		);

	PacketWeb packet;
	packet.setData((byte*)szTmp, (uint)iLen);
	packet.send(pClient);

	for (;;)
	{
		iLen = FileRead.Read( szTmp, uiWebDataBufSize);
		if ( iLen <= 0 )
			break;
		packet.setData((byte*)szTmp, (uint)iLen);
		packet.send(pClient);
		//dwSize -= iLen;
		if ( iLen < (int)uiWebDataBufSize)
		{
			// memset( szTmp+iLen, 0, uiWebDataBufSize-iLen );
			break;
		}
	}
	return 0;
}

static int GetHexDigit( tchar ch )
{
	if ( IsDigit( ch ))
		return( ch - '0' );

	ch = static_cast<tchar>(toupper(ch));
	if ( ch > 'F' || ch <'A' )
		return -1;

	return( ch - ( 'A' - 10 ));
}

static int HtmlDeCode( tchar * pszDst, lpctstr pszSrc )
{
	int i = 0;
	for (;;)
	{
		tchar ch = *pszSrc++;
		if ( ch == '+' )
		{
			ch = ' ';
		}
		else if ( ch == '%' )
		{
			ch = *pszSrc++;
			int iVal = GetHexDigit(ch);
			if ( ch )
			{
				ch = *pszSrc++;
				if ( ch )
				{
					ch = static_cast<tchar>(iVal*0x10 + GetHexDigit(ch));
					if ((uchar)(ch) == 0xa0)
						ch = '\0';
				}
			}
		}
		*pszDst++ = ch;
		if ( ch == 0 )
			break;
		i++;
	}
	return( i );
}

bool CWebPageDef::ServPagePost( CClient * pClient, lpctstr pszURLArgs, tchar * pContentData, size_t uiContentLength )
{
	ADDTOCALLSTACK("CWebPageDef::ServPagePost");
	UnreferencedParameter(pszURLArgs);
	// RETURN: true = this was the page of interest.

	ASSERT(pClient);

	if ( pContentData == nullptr || uiContentLength <= 0 )
		return false;
	if ( ! HasTrigger(XTRIG_UNKNOWN))	// this form has no triggers.
		return false;

	// Parse the data.
	pContentData[uiContentLength] = 0;
	tchar * ppArgs[64];
	int iArgs = Str_ParseCmds(pContentData, ppArgs, ARRAY_COUNT(ppArgs), "&");
	if (( iArgs <= 0 ) || ( iArgs >= 63 ))
		return false;

	// T or TXT or TEXT = the text fields.
	// B or BTN or BUTTON = the buttons
	// C or CHK or CHECK = the check boxes

	CDialogResponseArgs resp;
	dword dwButtonID = UINT32_MAX;
	for ( int i = 0; i < iArgs; ++i )
	{
		tchar * pszNum = ppArgs[i];
		while ( IsAlpha(*pszNum) )
			++pszNum;

		int iNum = atoi(pszNum);
		while ( *pszNum )
		{
			if ( *pszNum == '=' )
			{
				++pszNum;
				break;
			}
			++pszNum;
		}
		switch ( toupper(ppArgs[i][0]) )
		{
			case 'B':
				dwButtonID = iNum;
				break;
			case 'C':
				if ( !iNum )
					continue;
				if ( atoi(pszNum) )
				{
                    resp.m_CheckArray.push_back(iNum);
				}
				break;
			case 'T':
				if ( iNum > 0 )
				{
					tchar *pszData = Str_GetTemp();
					HtmlDeCode( pszData, pszNum );
					resp.AddText((word)(iNum), pszData);
				}
				break;
		}
	}

	// Use the data in RES_WEBPAGE block.
	CResourceLock s;
	if ( !ResourceLock(s) )
		return false;

	// Find the correct entry point.
	while ( s.ReadKeyParse())
	{
		if ( !s.IsKeyHead("ON", 2) || ( (dword)s.GetArgVal() != dwButtonID ))
			continue;
		OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, pClient, &resp);
		return true;
	}

	// Put up some sort of failure page ?

	return false;
}

void CWebPageDef::ServPage( CClient * pClient, tchar * pszPage, CSTime * pdateIfModifiedSince )	// static
{
	ADDTOCALLSTACK("CWebPageDef::ServPage");
	// make sure this is a valid format for the request.

	tchar szPageName[_MAX_PATH];
	Str_GetBare( szPageName, pszPage, sizeof(szPageName), "!\"#$%&()*,:;<=>?[]^{|}-+'`" );

	int iError = 404;
	CWebPageDef * pWebPage = g_Cfg.FindWebPage(szPageName);
	if ( pWebPage )
	{
		iError = pWebPage->ServPageRequest(pClient, szPageName, pdateIfModifiedSince);
		if ( ! iError )
			return;
	}

	// Is it a file in the Script directory ?
	if ( iError == 404 )
	{
		const CResourceID ridjunk( RES_UNKNOWN, 1 );
		CWebPageDef tmppage( ridjunk );
		if ( tmppage.SetSourceFile( szPageName, pClient ))
		{
			if ( !tmppage.ServPageRequest(pClient, szPageName, pdateIfModifiedSince) )
				return;
		}
	}

	// Can't find it !?
	// just take the default page. or have a custom 404 page ?

	pClient->m_Targ_Text = pszPage;

	tchar *pszTemp = Str_GetTemp();
	sprintf(pszTemp, SPHERE_FILE "%d.htm", iError);
	pWebPage = g_Cfg.FindWebPage(pszTemp);
	if ( pWebPage )
	{
		if ( ! pWebPage->ServPageRequest( pClient, pszPage, nullptr ))
			return;
	}

	// Hmm we should do something !!!?
	// Try to give a reasonable default error msg.

	lpctstr pszErrText;
	switch (iError)
	{
		case 401: pszErrText = "Authorization Required"; break;
		case 403: pszErrText = "Forbidden"; break;
		case 404: pszErrText = "Object Not Found"; break;
		case 500: pszErrText = "Internal Server Error"; break;
		default: pszErrText = "Unknown Error"; break;
	}

	CSTime datetime = CSTime::GetCurrentTime();
	const char *sDate = datetime.FormatGmt(nullptr);
	CSString sMsgHead;
	CSString sText;

	sText.Format(
        "<meta charset = \"UTF-8\">"
		"<html><head><title>Error %d</title>"
		"<meta name=robots content=noindex>"
		"</head><body>"
		"<h2>HTTP Error %d</h2><p><strong>%d %s</strong></p>"
		"<p>The " SPHERE_TITLE " server cannot deliver the file or script you asked for.</p>"
		"<p>Please contact the server's administrator if this problem persists.</p>"
		"</body></html>",
		iError,
		iError,
		iError,
		pszErrText);

	sMsgHead.Format(
		"HTTP/1.1 %d %s\r\n"
		"Date: %s\r\n"
		"Server: " SPHERE_TITLE " " SPHERE_BUILD_NAME_VER_PREFIX SPHERE_BUILD_INFO_STR "\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"\r\n%s",
		iError, pszErrText,
		static_cast<lpctstr>(sDate),
		sText.GetLength(),
		static_cast<lpctstr>(sText));

	new PacketWeb(pClient, reinterpret_cast<const byte *>(sMsgHead.GetBuffer()), sMsgHead.GetLength());
}

