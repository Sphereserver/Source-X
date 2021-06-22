#include "../../common/crypto/CMD5.h"
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../../network/CClientIterator.h"
#include "../chars/CChar.h"
#include "../CServer.h"
#include "../CWorld.h"
#include "../CWorldGameTime.h"
#include "CAccount.h"
#include "CClient.h"
#include <algorithm>


//**********************************************************************
// -CAccounts


size_t CAccounts::Account_GetCount() const
{
	return m_Accounts.size();
}

bool CAccounts::Account_Load( lpctstr pszNameRaw, CScript & s, bool fChanges )
{
	ADDTOCALLSTACK("CAccounts::Account_Load");

	// Only read as "[ACCOUNT name]" format if arguments exist.
	if ( s.HasArgs() && !strnicmp(pszNameRaw, "ACCOUNT", 7) )
	{
		pszNameRaw = s.GetArgStr();
	}

	tchar szName[MAX_ACCOUNT_NAME_SIZE];
	if ( !CAccount::NameStrip(szName, pszNameRaw) )
	{
		if ( !fChanges )
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszNameRaw);
			return false;
		}
	}
	m_fLoading = true;

	CAccount * pAccount = Account_Find(szName);
	if ( pAccount )
	{
		// Look for existing duplicate ?
		if ( !fChanges )
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': duplicate name\n", pszNameRaw);
			return false;
		}
	}
	else
	{
		pAccount = new CAccount(szName);
		ASSERT(pAccount != nullptr);
	}
	pAccount->r_Load(s);

	m_fLoading = false;

	return true;
}

bool CAccounts::Account_LoadAll( bool fChanges, bool fClearChanges )
{
	ADDTOCALLSTACK("CAccounts::Account_LoadAll");
	lpctstr pszBaseDir;
	lpctstr pszBaseName;
	char *z = Str_GetTemp();

	pszBaseDir = g_Cfg.m_sAcctBaseDir.IsEmpty() ? g_Cfg.m_sWorldBaseDir : g_Cfg.m_sAcctBaseDir;
	pszBaseName = ( fChanges ) ? (SPHERE_FILE "acct" SPHERE_SCRIPT) : (SPHERE_FILE "accu" SPHERE_SCRIPT);

	Str_CopyLimitNull(z, pszBaseDir, STR_TEMPLENGTH);
	Str_ConcatLimitNull(z, pszBaseName, STR_TEMPLENGTH);

	CScript s;
	if ( ! s.Open(z, OF_READ|OF_TEXT|OF_DEFAULTMODE| ( fChanges ? OF_NONCRIT : 0) ))
	{
		if ( !fChanges )
		{
			if ( Account_LoadAll(true) )	// if we have changes then we are ok.
				return true;

											//	auto-creating account files
			if ( !Account_SaveAll() )
				g_Log.Event(LOGL_FATAL|LOGM_INIT, "Can't open account file '%s'\n", s.GetFilePath());
			else
				return true;
		}
		return false;
	}

	if ( fClearChanges )
	{
		ASSERT( fChanges );
		// empty the changes file.

		s.Close();
		if (!s.Open(nullptr, OF_WRITE | OF_TEXT | OF_DEFAULTMODE))
			return false;

		s.WriteString( "// Accounts are periodically moved to the " SPHERE_FILE "accu" SPHERE_SCRIPT " file.\n"
			"// All account changes should be made here.\n"
			"// Use the /ACCOUNT UPDATE command to force accounts to update.\n"
			);
		return true;
	}

	CScriptFileContext ScriptContext(&s);
	while (s.FindNextSection())
	{
		Account_Load(s.GetKey(), s, fChanges);
	}

	if ( !fChanges )
        Account_LoadAll(true);

	return true;
}


bool CAccounts::Account_SaveAll()
{
	ADDTOCALLSTACK("CAccounts::Account_SaveAll");
	EXC_TRY("SaveAll");
	// Look for changes FIRST.
	Account_LoadAll(true);

	lpctstr pszBaseDir;

	if ( g_Cfg.m_sAcctBaseDir.IsEmpty() ) pszBaseDir = g_Cfg.m_sWorldBaseDir;
	else pszBaseDir = g_Cfg.m_sAcctBaseDir;

	CScript s;
	if ( !CWorld::OpenScriptBackup(s, pszBaseDir, "accu", g_World.m_iSaveCountID) )
		return false;

	s.Printf("// " SPHERE_TITLE " %s accounts file\n"
		"// NOTE: This file cannot be edited while the server is running.\n"
		"// Any file changes must be made to " SPHERE_FILE "accu" SPHERE_SCRIPT ". This is read in at save time.\n",
		g_Serv.GetName());

	for ( size_t i = 0; i < m_Accounts.size(); ++i )
	{
		CAccount * pAccount = Account_Get(i);
		if ( pAccount )
			pAccount->r_Write(s);
	}

	Account_LoadAll(true, true);	// clear the change file now.
	return true;
	EXC_CATCH;
	return false;
}

CAccount * CAccounts::Account_FindChat( lpctstr pszChatName )
{
	ADDTOCALLSTACK("CAccounts::Account_FindChat");
	for ( size_t i = 0; i < m_Accounts.size(); i++ )
	{
		CAccount * pAccount = Account_Get(i);
		if ( pAccount != nullptr && pAccount->m_sChatName.CompareNoCase(pszChatName) == 0 )
			return pAccount;
	}
	return nullptr;
}

CAccount * CAccounts::Account_Find( lpctstr pszName )
{
	ADDTOCALLSTACK("CAccounts::Account_Find");
	tchar szName[ MAX_ACCOUNT_NAME_SIZE ];

	if ( !CAccount::NameStrip(szName, pszName) )
		return nullptr;

	size_t i = m_Accounts.FindKey(szName);
	if ( i != SCONT_BADINDEX )
		return Account_Get(i);

	return nullptr;
}

CAccount * CAccounts::Account_FindCreate( lpctstr pszName, bool fAutoCreate )
{
	ADDTOCALLSTACK("CAccounts::Account_FindCreate");

	CAccount * pAccount = Account_Find(pszName);
	if ( pAccount )
		return pAccount;

	if ( fAutoCreate )	// Create if not found.
	{
		bool fGuest = ( g_Serv.m_eAccApp == ACCAPP_GuestAuto || g_Serv.m_eAccApp == ACCAPP_GuestTrial || ! strnicmp( pszName, "GUEST", 5 ));
		tchar szName[ MAX_ACCOUNT_NAME_SIZE ];

		if (( pszName[0] >= '0' ) && ( pszName[0] <= '9' ))
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name. Due to account enumerations incompatibilities, account names could not start with digits.\n", pszName);
		else if ( CAccount::NameStrip(szName, pszName) )
			return new CAccount(szName, fGuest);
		else
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);
	}
	return nullptr;
}

bool CAccounts::Account_Delete( CAccount * pAccount )
{
	ADDTOCALLSTACK("CAccounts::Account_Delete");
	ASSERT(pAccount != nullptr);

	CScriptTriggerArgs Args;
	Args.Init(pAccount->GetName());
	enum TRIGRET_TYPE tr = TRIGRET_RET_FALSE;
	g_Serv.r_Call("f_onaccount_delete", &g_Serv, &Args, nullptr, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		return false;
	}

	m_Accounts.RemovePtr( pAccount );
	return true;
}

void CAccounts::Account_Add( CAccount * pAccount )
{
	ADDTOCALLSTACK("CAccounts::Account_Add");
	ASSERT(pAccount != nullptr);
	if ( !g_Serv.IsLoading() )
	{
		CScriptTriggerArgs Args;
		Args.Init(pAccount->GetName());
		//Accounts are 'created' in server startup so we don't fire the function.
		TRIGRET_TYPE tRet = TRIGRET_RET_FALSE;
		g_Serv.r_Call("f_onaccount_create", &g_Serv, &Args, nullptr, &tRet);
		if ( tRet == TRIGRET_RET_TRUE )
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': Creation blocked via script\n", pAccount->GetName());
			Account_Delete(pAccount);
			return;
		}
	}
	m_Accounts.AddSortKey(pAccount,pAccount->GetName());
}

CAccount * CAccounts::Account_Get( size_t index )
{
	ADDTOCALLSTACK("CAccounts::Account_Get");
	if ( ! m_Accounts.IsValidIndex(index))
		return nullptr;
	return static_cast <CAccount *>( m_Accounts[index] );
}

bool CAccounts::Cmd_AddNew( CTextConsole * pSrc, lpctstr pszName, lpctstr ptcArg, bool md5 )
{
	ADDTOCALLSTACK("CAccounts::Cmd_AddNew");
	if (pszName == nullptr || pszName[0] == '\0')
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Username is required to add an account.\n" );
		return false;
	}

	CAccount * pAccount = Account_Find( pszName );
	if ( pAccount != nullptr )
	{
		pSrc->SysMessagef( "Account '%s' already exists\n", pszName );
		return false;
	}

	tchar szName[ MAX_ACCOUNT_NAME_SIZE ];

	if ( !CAccount::NameStrip(szName, pszName) )
	{
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);
		return false;
	}

	pAccount = new CAccount(szName);
	ASSERT(pAccount);
	pAccount->_dateConnectedFirst = pAccount->_dateConnectedLast = CSTime::GetCurrentTime();

	pAccount->SetPassword(ptcArg, md5);
	return true;
}

/**
* CAccount actions.
* This enum describes the CACcount acctions performed by command shell.
*/
enum VACS_TYPE
{
	VACS_ADD, // Add a new CAccount.
	VACS_ADDMD5, // Add a new CAccount, storing the password with md5.
	VACS_BLOCKED, // Bloc a CAccount.
	VACS_HELP, // Show CAccount commands.
	VACS_JAILED, // "Jail" the CAccount.
	VACS_UNUSED, // Use a command on unused CAccount.
	VACS_UPDATE, // Process the acct file to add accounts.
	VACS_QTY // TODOC.
};

lpctstr const CAccounts::sm_szVerbKeys[] =	// CAccounts:: // account group verbs.
{
	"ADD",
	"ADDMD5",
	"BLOCKED",
	"HELP",
	"JAILED",
	"UNUSED",
	"UPDATE",
	nullptr
};

bool CAccounts::Cmd_ListUnused(CTextConsole * pSrc, lpctstr pszDays, lpctstr pszVerb, lpctstr pszArgs, dword dwMask)
{
	ADDTOCALLSTACK("CAccounts::Cmd_ListUnused");
	int iDaysTest = Exp_GetVal(pszDays);

	CSTime datetime = CSTime::GetCurrentTime();
	int iDaysCur = datetime.GetDaysTotal();

	size_t iCountOrig = Account_GetCount();
	size_t iCountCheck = iCountOrig;
	size_t iCount = 0;
	for ( size_t i = 0; ; i++ )
	{
		if ( Account_GetCount() < iCountCheck )
		{
			iCountCheck--;
			i--;
		}
		CAccount * pAccount = Account_Get(i);
		if ( pAccount == nullptr )
			break;

		int iDaysAcc = pAccount->_dateConnectedLast.GetDaysTotal();
		if ( ! iDaysAcc )
		{
			// account has never been used ? (get create date instead)
			iDaysAcc = pAccount->_dateConnectedFirst.GetDaysTotal();
		}

		if ( (iDaysCur - iDaysAcc) < iDaysTest ) continue;
		if ( dwMask && !pAccount->IsPriv((word)(dwMask)) ) continue;

		iCount ++;
		if ( pszVerb == nullptr || pszVerb[0] == '\0' )
		{
			// just list stuff about the account.
			CScript script("SHOW LASTCONNECTDATE");
			pAccount->r_Verb( script, pSrc );
			continue;
		}

		// can't delete unused priv accounts this way.
		if ( ! strcmpi( pszVerb, "DELETE" ) && pAccount->GetPrivLevel() > PLEVEL_Player )
		{
			iCount--;
			pSrc->SysMessagef( "Can't Delete PrivLevel %d Account '%s' this way.\n",
				pAccount->GetPrivLevel(), static_cast<lpctstr>(pAccount->GetName()) );
		}
		else
		{
			CScript script( pszVerb, pszArgs );
			pAccount->r_Verb( script, pSrc );
		}
	}

	pSrc->SysMessagef( "Matched %" PRIuSIZE_T " of %" PRIuSIZE_T " accounts unused for %d days\n",
		iCount, iCountOrig, iDaysTest );

	if ( pszVerb && ! strcmpi(pszVerb, "DELETE") )
	{
		size_t iDeleted = iCountOrig - Account_GetCount();

		if ( iDeleted < iCount )
			pSrc->SysMessagef("%" PRIuSIZE_T " deleted, %" PRIuSIZE_T " cleared of characters (must try to delete again)\n",
				iDeleted, iCount - iDeleted );
		else if ( iDeleted > 0 )
			pSrc->SysMessagef("All %" PRIuSIZE_T " unused accounts deleted.\n", iDeleted);
		else
			pSrc->SysMessagef("No accounts deleted.\n");
	}

	return true;
}

bool CAccounts::Account_OnCmd( tchar * pszArgs, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CAccounts::Account_OnCmd");
	ASSERT( pSrc );

	if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
		return false;

	tchar * ppCmd[5];
	size_t iQty = Str_ParseCmds( pszArgs, ppCmd, CountOf( ppCmd ));

	VACS_TYPE index;
	if ( iQty <= 0 ||
		ppCmd[0] == nullptr ||
		ppCmd[0][0] == '\0' )
	{
		index = VACS_HELP;
	}
	else
	{
		index = (VACS_TYPE) FindTableSorted( ppCmd[0], sm_szVerbKeys, CountOf( sm_szVerbKeys )-1 );
	}

	static lpctstr const sm_pszCmds[] =
	{
		"/ACCOUNT UPDATE\n",
		"/ACCOUNT UNUSED days [command]\n",
		"/ACCOUNT ADD name password",
		"/ACCOUNT ADDMD5 name hash",
		"/ACCOUNT name BLOCK 0/1\n",
		"/ACCOUNT name JAIL 0/1\n",
		"/ACCOUNT name DELETE = delete all chars and the account\n",
		"/ACCOUNT name PLEVEL x = set account priv level\n",
		"/ACCOUNT name EXPORT filename\n"
	};

	switch (index)
	{
		case VACS_ADD:
			return Cmd_AddNew(pSrc, ppCmd[1], ppCmd[2]);

		case VACS_ADDMD5:
			return Cmd_AddNew(pSrc, ppCmd[1], ppCmd[2], true);

		case VACS_BLOCKED:
			return Cmd_ListUnused(pSrc, ppCmd[1], ppCmd[2], ppCmd[3], PRIV_BLOCKED);

		case VACS_HELP:
			{
				for ( size_t i = 0; i < CountOf(sm_pszCmds); ++i )
					pSrc->SysMessage( sm_pszCmds[i] );
			}
			return true;

		case VACS_JAILED:
			return Cmd_ListUnused(pSrc, ppCmd[1], ppCmd[2], ppCmd[3], PRIV_JAILED);

		case VACS_UPDATE:
			Account_SaveAll();
			return true;

		case VACS_UNUSED:
			return Cmd_ListUnused( pSrc, ppCmd[1], ppCmd[2], ppCmd[3] );

		default:
			break;
	}

	// Must be a valid account ?

	CAccount * pAccount = Account_Find( ppCmd[0] );
	if ( pAccount == nullptr )
	{
		pSrc->SysMessagef( "Account '%s' does not exist\n", ppCmd[0] );
		return false;
	}

	if ( !ppCmd[1] || !ppCmd[1][0] )
	{
		CClient	*pClient = pAccount->FindClient();

		char	*z = Str_GetTemp();
		snprintf(z, STR_TEMPLENGTH, 
			"Account '%s': PLEVEL:%d, BLOCK:%d, IP:%s, CONNECTED:%s, ONLINE:%s\n",
			pAccount->GetName(), pAccount->GetPrivLevel(), pAccount->IsPriv(PRIV_BLOCKED),
			pAccount->m_Last_IP.GetAddrStr(), pAccount->_dateConnectedLast.Format(nullptr),
			( pClient ? ( pClient->GetChar() ? pClient->GetChar()->GetName() : "<not logged>" ) : "no" )
		);
		pSrc->SysMessage(z);
		return true;
	}
	else
	{
		CSString cmdArgs;
		if (ppCmd[4] && ppCmd[4][0])
			cmdArgs.Format("%s %s %s", ppCmd[2], ppCmd[3], ppCmd[4]);
		else if (ppCmd[3] && ppCmd[3][0])
			cmdArgs.Format("%s %s", ppCmd[2], ppCmd[3]);
		else if (ppCmd[2] && ppCmd[2][0])
			cmdArgs.Format("%s", ppCmd[2]);

		CScript script( ppCmd[1], cmdArgs.GetBuffer() );

		return pAccount->r_Verb( script, pSrc );
	}
}

//**********************************************************************
// -CAccount


bool CAccount::NameStrip( tchar * pszNameOut, lpctstr pszNameInp )
{
	ADDTOCALLSTACK("CAccount::NameStrip");

	size_t iLen = Str_GetBare(pszNameOut, pszNameInp, MAX_ACCOUNT_NAME_SIZE, ACCOUNT_NAME_VALID_CHAR);

	if ( iLen <= 0 )
		return false;
	// Check for newline characters.
	if ( strchr(pszNameOut, 0x0A) || strchr(pszNameOut, 0x0C) || strchr(pszNameOut, 0x0D) )
		return false;
	if ( !strnicmp(pszNameOut, "EOF", 3) || !strnicmp(pszNameOut, "ACCOUNT", 7) )
		return false;
	// Check for name already used.
	if ( FindTableSorted(pszNameOut, CAccounts::sm_szVerbKeys, CountOf(CAccounts::sm_szVerbKeys)-1) >= 0 )
		return false;
	if ( g_Cfg.IsObscene(pszNameOut) )
		return false;

	return true;
}

static lpctstr constexpr sm_szPrivLevels[ PLEVEL_QTY+1 ] =
{
	"Guest",		// 0 = This is just a guest account. (cannot PK)
	"Player",		// 1 = Player or NPC.
	"Counsel",		// 2 = Can travel and give advice.
	"Seer",			// 3 = Can make things and NPC's but not directly bother players.
	"GM",			// 4 = GM command clearance
	"Dev",			// 5 = Not bothererd by GM's
	"Admin",		// 6 = Can switch in and out of gm mode. assign GM's
	"Owner",		// 7 = Highest allowed level.
	nullptr
};

PLEVEL_TYPE CAccount::GetPrivLevelText( lpctstr pszFlags ) // static
{
	ADDTOCALLSTACK("CAccount::GetPrivLevelText");
	int level = FindTable( pszFlags, sm_szPrivLevels, CountOf(sm_szPrivLevels)-1 );
	if ( level >= 0 )
		return (PLEVEL_TYPE)level;

	level = Exp_GetVal( pszFlags );
	if ( level < PLEVEL_Guest )
		return PLEVEL_Guest;
	if ( level > PLEVEL_Owner )
		return PLEVEL_Owner;

	return (PLEVEL_TYPE)level;
}

CAccount::CAccount( lpctstr pszName, bool fGuest )
{
	g_Serv.StatInc( SERV_STAT_ACCOUNTS );

	tchar szName[ MAX_ACCOUNT_NAME_SIZE ];
	if ( !CAccount::NameStrip( szName, pszName ) )
		g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name\n", pszName);
	m_sName = szName;

	if ( !strnicmp(m_sName, "GUEST", 5) || fGuest )
		SetPrivLevel(PLEVEL_Guest);
	else
		SetPrivLevel(PLEVEL_Player);

	m_PrivFlags = (word)(g_Cfg.m_iAutoPrivFlags);
    SetResDisp(RDS_T2A);
	m_MaxChars = 0;
    _iMaxHouses = g_Cfg._iMaxHousesAccount;
    _iMaxShips = g_Cfg._iMaxShipsAccount;

	_iTimeConnectedTotal = 0;
	_iTimeConnectedLast = 0;
	// Add myself to the list.
	g_Accounts.Account_Add( this );
}

void CAccount::DeleteChars()
{
	ADDTOCALLSTACK("CAccount::DeleteChars");
	CClient * pClient = FindClient();
	if ( pClient != nullptr )
	{	// we have no choice but to kick them.
		pClient->GetNetState()->markReadClosed();
	}

	// Now track down all my disconnected chars !
	if ( ! g_Serv.IsLoading())
	{
		size_t i = m_Chars.GetCharCount();
		while (i > 0)
		{
			CChar * pChar = m_Chars.GetChar(--i).CharFind();
			if (pChar != nullptr)
			{
				pChar->Delete();
				pChar->ClearPlayer();
			}

			m_Chars.DetachChar(i);
		}
	}
}


CAccount::~CAccount()
{
	g_Serv.StatDec( SERV_STAT_ACCOUNTS );

	DeleteChars();
	ClearPasswordTries(true);
}

void CAccount::SetPrivLevel( PLEVEL_TYPE plevel )
{
	ADDTOCALLSTACK("CAccount::SetPrivLevel");
	m_PrivLevel = plevel;	// PLEVEL_Counsel
}

CClient * CAccount::FindClient( const CClient * pExclude ) const
{
	ADDTOCALLSTACK("CAccount::FindClient");

	CClient* pClient = nullptr;
	ClientIterator it;
	for (pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( pClient == pExclude )
			continue;
		if ( this == pClient->GetAccount())
			break;
	}
	return( pClient );
}

byte CAccount::GetMaxChars() const
{
	return std::min((m_MaxChars > 0 ? m_MaxChars : g_Cfg.m_iMaxCharsPerAccount), MAX_CHARS_PER_ACCT);
}

void CAccount::SetMaxChars(byte chars)
{
	m_MaxChars = minimum(chars, MAX_CHARS_PER_ACCT);
}

bool CAccount::IsMyAccountChar( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CAccount::IsMyAccountChar");
	if ( pChar == nullptr )
		return false;
	if ( pChar->m_pPlayer == nullptr )
		return false;
	return(	pChar->m_pPlayer->GetAccount() == this );
}

size_t CAccount::DetachChar( CChar * pChar )
{
	ADDTOCALLSTACK("CAccount::DetachChar");
	ASSERT( pChar );
	ASSERT( IsMyAccountChar( pChar ));

	if ( m_uidLastChar == pChar->GetUID())
	{
		m_uidLastChar.InitUID();
	}

	return( m_Chars.DetachChar( pChar ));
}

size_t CAccount::AttachChar( CChar * pChar )
{
	ADDTOCALLSTACK("CAccount::AttachChar");
	ASSERT(pChar);
	ASSERT( IsMyAccountChar( pChar ));

	// is it already linked ?
	size_t i = m_Chars.AttachChar( pChar );
	if ( i != SCONT_BADINDEX )
	{
		size_t iQty = m_Chars.GetCharCount();
		if ( iQty > MAX_CHARS_PER_ACCT )
		{
			g_Log.Event( LOGM_ACCOUNTS|LOGL_ERROR, "Account '%s' has %" PRIuSIZE_T " characters\n", GetName(), iQty );
		}
	}

	return( i );
}

void CAccount::TogPrivFlags( word wPrivFlags, lpctstr pszArgs )
{
	ADDTOCALLSTACK("CAccount::TogPrivFlags");

	if ( pszArgs == nullptr || pszArgs[0] == '\0' )	// toggle.
	{
		m_PrivFlags ^= wPrivFlags;
	}
	else if ( Exp_GetVal( pszArgs ))
	{
		m_PrivFlags |= wPrivFlags;
	}
	else
	{
		m_PrivFlags &= ~wPrivFlags;
	}
}

void CAccount::OnLogin( CClient * pClient )
{
	ADDTOCALLSTACK("CAccount::OnLogin");

	ASSERT(pClient);
	pClient->m_timeLogin = CWorldGameTime::GetCurrentTime().GetTimeRaw();	// g_World clock of login time. "LASTCONNECTTIME"

	if ( GetPrivLevel() >= PLEVEL_Counsel )	// ON by default.
	{
		m_PrivFlags |= PRIV_GM_PAGE;
	}

	// Get the real world time/date.
	CSTime datetime = CSTime::GetCurrentTime();

	if ( !_iTimeConnectedTotal )	// first time - save first ip and timestamp
	{
		m_First_IP.SetAddrIP(pClient->GetPeer().GetAddrIP());
		_dateConnectedFirst = datetime;
	}

	if ( pClient->GetConnectType() == CONNECT_TELNET )
	{
		// link the admin client.
		++g_Serv.m_iAdminClients;
	}

	g_Log.Event( LOGM_CLIENTS_LOG, "%x:Login for account '%s'. IP='%s'. ConnectionType: %s.\n",
		pClient->GetSocketID(), GetName(), pClient->GetPeerStr(), pClient->GetConnectTypeStr(pClient->GetConnectType()) );

	m_Last_IP.SetAddrIP(pClient->GetPeer().GetAddrIP());
	//m_TagDefs.SetStr("LastLogged", false, _dateConnectedLast.Format(nullptr));
	//_dateConnectedLast = datetime;
}

void CAccount::OnLogout(CClient *pClient, bool fWasChar)
{
	ADDTOCALLSTACK("CAccount::OnLogout");
	ASSERT(pClient);

	if ( pClient->GetConnectType() == CONNECT_TELNET ) // unlink the admin client.
		-- g_Serv.m_iAdminClients;

	// calculate total game time. skip this calculation in
	// case if it was login type packet. it has the same type,
	// so we should check whatever player is attached to a char
	if ( pClient->IsConnectTypePacket() && fWasChar )
	{
		_iTimeConnectedLast = (CWorldGameTime::GetCurrentTime().GetTimeDiff(pClient->m_timeLogin) ) / (MSECS_PER_SEC * 60 );
		if ( _iTimeConnectedLast < 0 )
			_iTimeConnectedLast = 0;

		_iTimeConnectedTotal += _iTimeConnectedLast;
	}
}

bool CAccount::Kick( CTextConsole * pSrc, bool fBlock )
{
	ADDTOCALLSTACK("CAccount::Kick");
	if (( GetPrivLevel() >= pSrc->GetPrivLevel()) &&  ( pSrc->GetChar() ) )
	{
		pSrc->SysMessageDefault(DEFMSG_MSG_ACC_PRIV);
		return false;
	}

	if ( fBlock )
	{
		SetPrivFlags( PRIV_BLOCKED );
		pSrc->SysMessagef( g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_BLOCK), GetName() );
	}

	lpctstr pszAction = fBlock ? "KICK" : "DISCONNECT";

	tchar * z = Str_GetTemp();
	snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_KICK), GetName(), pszAction, pSrc->GetName());
	g_Log.Event(LOGL_EVENT|LOGM_GM_CMDS, "%s.\n", z);

	return true;
}

bool CAccount::CheckPasswordTries(CSocketAddress csaPeerName)
{
	if ( csaPeerName.IsLocalAddr() || (csaPeerName.GetAddrIP() == 0x7F000001) )
	{
		return true;
	}

	int iAccountMaxTries = g_Cfg.m_iClientLoginMaxTries;
	bool bReturn = true;
	dword dwCurrentIP = csaPeerName.GetAddrIP();
	int64 timeCurrent = CWorldGameTime::GetCurrentTime().GetTimeRaw();

	BlockLocalTime_t::iterator itData = m_BlockIP.find(dwCurrentIP);
	if ( itData != m_BlockIP.end() )
	{
		BlockLocalTimePair_t itResult = (*itData).second;
		TimeTriesStruct_t & ttsData = itResult.first;
		ttsData.m_Last = timeCurrent;

		if ( ttsData.m_vcDelay > timeCurrent )
		{
			bReturn = false;
		}
		else
		{
			if ((( ttsData.m_Last - ttsData.m_First ) > 15* MSECS_PER_SEC) && (itResult.second < iAccountMaxTries))
			{
				ttsData.m_First = timeCurrent;
				ttsData.m_vcDelay = 0;
				itResult.second = 0;
			}
			else
			{
				itResult.second++;

				if ( itResult.second > iAccountMaxTries )
				{
					ttsData.m_First = ttsData.m_vcDelay;
					ttsData.m_vcDelay = 0;
					itResult.second = 0;
				}
				else if ( itResult.second == iAccountMaxTries )
				{
					ttsData.m_vcDelay = ttsData.m_Last + (llong)(g_Cfg.m_iClientLoginTempBan);
					bReturn = false;
				}
			}
		}

		m_BlockIP[dwCurrentIP] = itResult;
	}
	else
	{
		TimeTriesStruct_t ttsData;
		ttsData.m_First = timeCurrent;
		ttsData.m_Last = timeCurrent;
		ttsData.m_vcDelay = 0;

		m_BlockIP[dwCurrentIP] = std::make_pair(ttsData, 0);
	}

	if ( m_BlockIP.size() > 100 )
	{
		ClearPasswordTries();
	}

	return bReturn;
}


void CAccount::ClearPasswordTries(bool bAll)
{
	if ( bAll )
	{
		m_BlockIP.clear();
		return;
	}

	llong timeCurrent = CWorldGameTime::GetCurrentTime().GetTimeRaw();
	for ( BlockLocalTime_t::iterator itData = m_BlockIP.begin(), end = m_BlockIP.end(); itData != end; )
	{
		BlockLocalTimePair_t itResult = (*itData).second;
		if ( (timeCurrent - itResult.first.m_Last) > g_Cfg.m_iClientLoginTempBan )
		{
			itData = m_BlockIP.erase(itData);
			end = m_BlockIP.end();
		}
		else
			++itData;
	}
}

bool CAccount::CheckPassword( lpctstr pszPassword )
{
	ADDTOCALLSTACK("CAccount::CheckPassword");
	ASSERT(pszPassword);

	if ( m_sCurPassword.IsEmpty() )
	{
		// If account password is empty, set the password given by the client trying to connect
		if ( !SetPassword(pszPassword) )
			return false;
	}

    CScriptTriggerArgs Args;
	Args.m_VarsLocal.SetStrNew("Account",GetName());
	Args.m_VarsLocal.SetStrNew("Password",pszPassword);
	TRIGRET_TYPE tr = TRIGRET_RET_FALSE;
	g_Serv.r_Call("f_onaccount_connect", &g_Serv, &Args, nullptr, &tr);
	if ( tr == TRIGRET_RET_TRUE )
		return false;
	if ( tr == TRIGRET_RET_HALFBAKED)
		return true;

	// Get the password.
	if( g_Cfg.m_fMd5Passwords )
	{
		// Hash the Password provided by the user and compare the hashes
		char digest[33];
		CMD5::fastDigest( digest, pszPassword );

		if( !strcmpi( digest, GetPassword() ) )
			return true;
	}
	else
	{
		if ( ! strcmpi( GetPassword(), pszPassword ) )
			return true;
	}

	if ( ! m_sNewPassword.IsEmpty() && ! strcmpi( GetNewPassword(), pszPassword ))
	{
		// using the new password.
		// kill the old password.
		SetPassword( m_sNewPassword );
		m_sNewPassword.Clear();
		return true;
	}

	return false;	// failure.
}

bool CAccount::SetPassword( lpctstr pszPassword, bool isMD5Hash )
{
	ADDTOCALLSTACK("CAccount::SetPassword");

	if ( Str_Check( pszPassword ) )	// Prevents exploits
		return false;

	bool useMD5 = g_Cfg.m_fMd5Passwords;

	//Accounts are 'created' in server startup so we don't fire the function.
	if ( !g_Serv.IsLoading() )
	{
		CScriptTriggerArgs Args;
		Args.Init(GetName());
		Args.m_VarsLocal.SetStrNew("password",pszPassword);
        Args.m_VarsLocal.SetStrNew("oldPassword",m_sCurPassword.GetBuffer());
		TRIGRET_TYPE tRet = TRIGRET_RET_FALSE;
		g_Serv.r_Call("f_onaccount_pwchange", &g_Serv, &Args, nullptr, &tRet);
		if ( tRet == TRIGRET_RET_TRUE )
		{
			return false;
		}
	}
	size_t enteredPasswordLength = strlen(pszPassword);
	if ( isMD5Hash && useMD5 ) // If it is a hash, check length and set it directly
	{
		if ( enteredPasswordLength == 32 )
			m_sCurPassword = pszPassword;

		return true;
	}

	size_t actualPasswordBufferSize = minimum(MAX_ACCOUNT_PASSWORD_ENTER, enteredPasswordLength) + 1;
	char * actualPassword = new char[actualPasswordBufferSize];
	Str_CopyLimitNull(actualPassword, pszPassword, actualPasswordBufferSize);

	if ( useMD5 )
	{
		char digest[33];

		// Auto-Hash if not loading or reloading
		if( !g_Accounts.m_fLoading )
		{
			CMD5::fastDigest( digest, actualPassword );
			m_sCurPassword = digest;
		}

		// Automatically convert the password into a hash if it's coming from the files
		// and shorter or longer than 32 characters
		else
		{
			if( enteredPasswordLength == 32 )
			{
				// password is already in hashed form
				m_sCurPassword = pszPassword;
			}
			else
			{
				// Autoconverted a Password on Load. Print
				g_Log.Event( LOGM_INIT|LOGL_EVENT, "MD5: Converted password for '%s'.\n", GetName() );
				CMD5::fastDigest( digest, actualPassword );
				m_sCurPassword = digest;
			}
		}
	}
	else
	{
		m_sCurPassword = actualPassword;
	}

	delete[] actualPassword;
	return true;
}

// Generate a new password
void CAccount::SetNewPassword( lpctstr pszPassword )
{
	ADDTOCALLSTACK("CAccount::SetNewPassword");
	if ( !pszPassword || !pszPassword[0] )		// no password given, auto-generate password
	{
		static tchar const passwdChars[] = "ABCDEFGHJKLMNPQRTUVWXYZ2346789";
		int len = (int)strlen(passwdChars);
		int charsCnt = Calc_GetRandVal(4) + 6;	// 6 - 10 chars
		if ( charsCnt > (MAX_ACCOUNT_PASSWORD_ENTER - 1) )
			charsCnt = MAX_ACCOUNT_PASSWORD_ENTER - 1;

		tchar szTmp[MAX_ACCOUNT_PASSWORD_ENTER + 1];
		for ( int i = 0; i < charsCnt; ++i )
			szTmp[i] = passwdChars[Calc_GetRandVal(len)];

		szTmp[charsCnt] = '\0';
		m_sNewPassword = szTmp;
		return;
	}

	m_sNewPassword = pszPassword;
	if ( m_sNewPassword.GetLength() > MAX_ACCOUNT_PASSWORD_ENTER )
		m_sNewPassword.Resize(MAX_ACCOUNT_PASSWORD_ENTER);
}

bool CAccount::SetResDisp(byte what)
{
	if (what >= RDS_T2A && what < RDS_QTY)
	{
		m_ResDisp = what;
		return true;
	}
	return false;
}

bool CAccount::SetGreaterResDisp(byte what)
{
	if (what > m_ResDisp)
		return SetResDisp(what);
	return false;
}

// Set account RESDISP automatically based on player client version
bool CAccount::SetAutoResDisp(CClient *pClient)
{
	ADDTOCALLSTACK("CAccount::SetAutoResDisp");
	if ( !pClient )
		return false;

	const CNetState* pNS = pClient->GetNetState();
	if (pNS->isClientVersion(MINCLIVER_TOL))
		return SetResDisp(RDS_TOL);
	else if (pNS->isClientVersion(MINCLIVER_HS))
		return SetResDisp(RDS_HS);
	else if (pNS->isClientVersion(MINCLIVER_SA))
		return SetResDisp(RDS_SA);
	else if (pNS->isClientVersion(MINCLIVER_ML))
		return SetResDisp(RDS_ML);
	else if (pNS->isClientVersion(MINCLIVER_SE))
		return SetResDisp(RDS_SE);
	else if (pNS->isClientVersion(MINCLIVER_AOS))
		return SetResDisp(RDS_AOS);
	else if (pNS->isClientVersion(MINCLIVER_LBR))
		return SetResDisp(RDS_LBR);
	else
		return SetResDisp(RDS_T2A);
}

enum AC_TYPE
{
	AC_ACCOUNT,
	AC_BLOCK,
	AC_CHARS,
	AC_CHARUID,
	AC_CHATNAME,
	AC_FIRSTCONNECTDATE,
	AC_FIRSTIP,
	AC_GUEST,
	AC_JAIL,
	AC_LANG,
	AC_LASTCHARUID,
	AC_LASTCONNECTDATE,
	AC_LASTCONNECTTIME,
	AC_LASTIP,
	AC_MAXCHARS,
    AC_MAXHOUSES,
	AC_MAXSHIPS,
	AC_MD5PASSWORD,
	AC_NAME,
	AC_NEWPASSWORD,
	AC_PASSWORD,
	AC_PLEVEL,
	AC_PRIV,
	AC_RESDISP,
	AC_TAG,
	AC_TAG0,
	AC_TAGAT,
	AC_TAGCOUNT,
	AC_TOTALCONNECTTIME,
	AC_QTY
};

lpctstr const CAccount::sm_szLoadKeys[AC_QTY+1] = // static
{
	"ACCOUNT",
	"BLOCK",
	"CHARS",
	"CHARUID",
	"CHATNAME",
	"FIRSTCONNECTDATE",
	"FIRSTIP",
	"GUEST",
	"JAIL",
	"LANG",
	"LASTCHARUID",
	"LASTCONNECTDATE",
	"LASTCONNECTTIME",
	"LASTIP",
	"MAXCHARS",
    "MAXHOUSES",
	"MAXSHIPS",
	"MD5PASSWORD",
	"NAME",
	"NEWPASSWORD",
	"PASSWORD",
	"PLEVEL",
	"PRIV",
	"RESDISP",
	"TAG",
	"TAG0",
	"TAGAT",
	"TAGCOUNT",
	"TOTALCONNECTTIME",
	nullptr
};

bool CAccount::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CAccount::r_GetRef");
	if ( ! strnicmp( ptcKey, "CHAR.", 5 ))
	{
		// How many chars.
		ptcKey += 5;
		size_t i = Exp_GetSTVal(ptcKey);
		if ( m_Chars.IsValidIndex(i) )
		{
			pRef = m_Chars.GetChar(i).CharFind();
		}
		SKIP_SEPARATORS(ptcKey);
		return true;
	}
	return CScriptObj::r_GetRef( ptcKey, pRef );
}

bool CAccount::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
	ADDTOCALLSTACK("CAccount::r_WriteVal");
	EXC_TRY("WriteVal");
	if ( !pSrc )
		return false;

	bool fZero = false;

	switch ( FindTableHeadSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		case AC_NAME:
		case AC_ACCOUNT:
			sVal = m_sName;
			break;
		case AC_CHARS:
			sVal.FormatSTVal( m_Chars.GetCharCount() );
			break;
		case AC_BLOCK:
			sVal.FormatVal( IsPriv(PRIV_BLOCKED) );
			break;
		case AC_CHATNAME:
			sVal = m_sChatName;
			break;
		case AC_TAGAT:
			{
				ptcKey += 5;	// eat the 'TAGAT'
 				if ( *ptcKey == '.' )	// do we have an argument?
 				{
 					SKIP_SEPARATORS( ptcKey );
 					size_t iQty = Exp_GetSTVal( ptcKey );
					if ( iQty >= m_TagDefs.GetCount() )
 						return false; // trying to get non-existant tag

 					const CVarDefCont * pTagAt = m_TagDefs.GetAt( iQty );
 					if ( !pTagAt )
 						return false; // trying to get non-existant tag

 					SKIP_SEPARATORS( ptcKey );
 					if ( ! *ptcKey )
 					{
 						sVal.Format("%s=%s", pTagAt->GetKey(), pTagAt->GetValStr());
 					}
 					else if ( !strnicmp( ptcKey, "KEY", 3 )) // key?
 					{
 						sVal = pTagAt->GetKey();
 					}
 					else if ( !strnicmp( ptcKey, "VAL", 3 )) // val?
 					{
 						sVal = pTagAt->GetValStr();
 					}
                    else
                    {
                        return false;
                    }
                    break;
 				}
			return false;
			}
			break;
		case AC_TAGCOUNT:
			sVal.FormatSTVal( m_TagDefs.GetCount() );
			break;
		case AC_FIRSTCONNECTDATE:
			sVal = _dateConnectedFirst.Format(nullptr);
			break;
		case AC_FIRSTIP:
			sVal = m_First_IP.GetAddrStr();
			break;
		case AC_GUEST:
			sVal.FormatVal( GetPrivLevel() == PLEVEL_Guest );
			break;
		case AC_JAIL:
			sVal.FormatVal( IsPriv(PRIV_JAILED) );
			break;
		case AC_LANG:
			sVal = m_lang.GetStr();
			break;
		case AC_LASTCHARUID:
			sVal.FormatHex( m_uidLastChar );
			break;
		case AC_LASTCONNECTDATE:
			sVal = _dateConnectedLast.Format(nullptr);
			break;
		case AC_LASTCONNECTTIME:
			sVal.FormatLLVal( _iTimeConnectedLast );
			break;
		case AC_LASTIP:
			sVal = m_Last_IP.GetAddrStr();
			break;
		case AC_MAXCHARS:
			sVal.FormatVal( GetMaxChars() );
			break;
        case AC_MAXHOUSES:
            sVal.FormatU8Val(_iMaxHouses);
            break;
		case AC_MAXSHIPS:
			sVal.FormatU8Val(_iMaxShips);
			break;
		case AC_PLEVEL:
			sVal.FormatVal( m_PrivLevel );
			break;
		case AC_NEWPASSWORD:
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin ||
				pSrc->GetPrivLevel() < GetPrivLevel())	// can't see accounts higher than you
			{
				return false;
			}
			sVal = GetNewPassword();
			break;
		case AC_MD5PASSWORD:
		case AC_PASSWORD:
			if ( pSrc->GetPrivLevel() < PLEVEL_Admin ||
				pSrc->GetPrivLevel() < GetPrivLevel())	// can't see accounts higher than you
			{
				return false;
			}
			sVal = GetPassword();
			break;
		case AC_PRIV:
			sVal.FormatHex( m_PrivFlags );
			break;
		case AC_RESDISP:
			sVal.FormatVal( m_ResDisp );
			break;
		case AC_TAG0:
			fZero = true;
			++ptcKey;
			FALLTHROUGH;
		case AC_TAG:			// "TAG" = get/set a local tag.
			{
				if ( ptcKey[3] != '.' )
					return false;
				ptcKey += 4;
				sVal = m_TagDefs.GetKeyStr(ptcKey, fZero );
				break;
			}
		case AC_TOTALCONNECTTIME:
			sVal.FormatLLVal( _iTimeConnectedTotal );
			break;

		default:
			return ( fNoCallParent ? false : CScriptObj::r_WriteVal( ptcKey, sVal, pSrc, false ) );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CAccount::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CAccount::r_LoadVal");
	EXC_TRY("LoadVal");

	int i = FindTableHeadSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );
	if ( i < 0 )
	{
		return false;
	}

	switch ( i )
	{
		case AC_BLOCK:
			if ( ! s.HasArgs() || s.GetArgVal())
			{
				SetPrivFlags( PRIV_BLOCKED );
			}
			else
			{
				ClearPrivFlags( PRIV_BLOCKED );
			}
			break;
		case AC_CHARUID:
			// just ignore this ? chars are loaded later !
			if ( ! g_Serv.IsLoading())
			{
				const CUID uid( s.GetArgVal());
				CChar * pChar = uid.CharFind();
				if (pChar == nullptr)
				{
					DEBUG_ERR(( "Invalid CHARUID 0%x for account '%s'\n", (dword)(uid), GetName()));
					return false;
				}
				if ( ! IsMyAccountChar( pChar ))
				{
					DEBUG_ERR(( "CHARUID 0%x (%s) not attached to account '%s'\n", (dword)(uid), pChar->GetName(), GetName()));
					return false;
				}
				AttachChar(pChar);
			}
			break;
		case AC_CHATNAME:
			m_sChatName = s.GetArgStr();
			break;
		case AC_FIRSTCONNECTDATE:
			_dateConnectedFirst.Read( s.GetArgStr());
			break;
		case AC_FIRSTIP:
			m_First_IP.SetAddrStr( s.GetArgStr());
			break;
		case AC_GUEST:
			if ( ! s.HasArgs() || s.GetArgVal())
			{
				SetPrivLevel( PLEVEL_Guest );
			}
			else
			{
				if ( GetPrivLevel() == PLEVEL_Guest )
					SetPrivLevel( PLEVEL_Player );
			}
			break;
		case AC_JAIL:
			if ( ! s.HasArgs() || s.GetArgVal())
			{
				SetPrivFlags( PRIV_JAILED );
			}
			else
			{
				ClearPrivFlags( PRIV_JAILED );
			}
			break;
		case AC_LANG:
			m_lang.Set( s.GetArgStr());
			break;
		case AC_LASTCHARUID:
			m_uidLastChar.SetObjUID(s.GetArgDWVal());
			break;
		case AC_LASTCONNECTDATE:
			_dateConnectedLast.Read( s.GetArgStr());
			break;
		case AC_LASTCONNECTTIME:
			// Previous total amount of time in game
			_iTimeConnectedLast = s.GetArgLLVal();
			break;
		case AC_LASTIP:
			m_Last_IP.SetAddrStr( s.GetArgStr());
			break;
		case AC_MAXCHARS:
			SetMaxChars( s.GetArgUCVal() );
			break;
        case AC_MAXHOUSES:
            _iMaxHouses = s.GetArgUCVal();
            break;
		case AC_MAXSHIPS:
			_iMaxHouses = s.GetArgUCVal();
			break;
		case AC_MD5PASSWORD:
			SetPassword( s.GetArgStr(), true);
			break;
		case AC_PLEVEL:
			SetPrivLevel( GetPrivLevelText( s.GetArgRaw()));
			break;
		case AC_NEWPASSWORD:
			SetNewPassword( s.GetArgStr());
			break;
		case AC_PASSWORD:
			SetPassword( s.GetArgStr() );
			break;
		case AC_PRIV:
			m_PrivFlags = s.GetArgWVal();
			if ( m_PrivFlags & PRIV_UNUSED )
			{
				g_Log.EventError("Fixing PRIV field (0%x) for account %s have not supported flags set (caught by mask 0%x).\n", m_PrivFlags, GetName(), (word)PRIV_UNUSED);
				m_PrivFlags &= ~PRIV_UNUSED;
			}
			break;
		case AC_RESDISP:
			SetResDisp(s.GetArgBVal());
			break;

		case AC_TAG0:
        case AC_TAG:
        {
            const bool fZero = (i == AC_TAG0);
            lpctstr ptcKey = s.GetKey();
            ptcKey += (fZero ? 5 : 4);
            bool fQuoted = false;
            lpctstr ptcArg = s.GetArgStr(&fQuoted);
            m_TagDefs.SetStr(ptcKey, fQuoted, ptcArg, fZero);
        }
        break;

		case AC_TOTALCONNECTTIME:
			// Previous total amount of time in game
			_iTimeConnectedTotal = s.GetArgLLVal();
			break;

		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CAccount::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CAccount::r_Write");
	if ( GetPrivLevel() >= PLEVEL_QTY )
		return;

	s.WriteSection("%s", m_sName.GetBuffer());

	if ( GetPrivLevel() != PLEVEL_Player )
	{
		s.WriteKeyStr( "PLEVEL", sm_szPrivLevels[ GetPrivLevel() ] );
	}
	if ( m_PrivFlags != PRIV_DETAIL )
	{
		s.WriteKeyHex( "PRIV", m_PrivFlags &~( PRIV_BLOCKED | PRIV_JAILED ));
	}
	if ( GetResDisp() )
	{
		s.WriteKeyVal( "RESDISP", GetResDisp() );
	}
	if ( m_MaxChars > 0 )
	{
		s.WriteKeyVal( "MAXCHARS", m_MaxChars );
	}
	if ( IsPriv( PRIV_JAILED ))
	{
		s.WriteKeyVal( "JAIL", 1 );
	}
	if ( IsPriv( PRIV_BLOCKED ))
	{
		s.WriteKeyVal( "BLOCK", 1 );
	}
	if ( ! m_sCurPassword.IsEmpty())
	{
		s.WriteKeyStr( "PASSWORD", GetPassword() );
	}
	if ( ! m_sNewPassword.IsEmpty())
	{
		s.WriteKeyStr( "NEWPASSWORD", GetNewPassword() );
	}
	if ( _iTimeConnectedTotal )
	{
		s.WriteKeyVal( "TOTALCONNECTTIME", _iTimeConnectedTotal );
	}
	if ( _iTimeConnectedLast )
	{
		s.WriteKeyVal( "LASTCONNECTTIME", _iTimeConnectedLast );
	}
	if ( m_uidLastChar.IsValidUID())
	{
		s.WriteKeyHex( "LASTCHARUID", m_uidLastChar.GetObjUID() );
	}
    if (_iMaxHouses != g_Cfg._iMaxHousesAccount)
    {
        s.WriteKeyVal("MaxHouses", _iMaxHouses);
    }
    if (_iMaxShips != g_Cfg._iMaxShipsAccount)
    {
        s.WriteKeyVal("MaxShips", _iMaxShips);
    }

	m_Chars.WritePartyChars(s);

	if ( _dateConnectedFirst.IsTimeValid())
	{
		s.WriteKeyStr( "FIRSTCONNECTDATE", _dateConnectedFirst.Format(nullptr));
	}
	if ( m_First_IP.IsValidAddr() )
	{
		s.WriteKeyStr( "FIRSTIP", m_First_IP.GetAddrStr());
	}

	if ( _dateConnectedLast.IsTimeValid())
	{
		s.WriteKeyStr( "LASTCONNECTDATE", _dateConnectedLast.Format(nullptr));
	}
	if ( m_Last_IP.IsValidAddr() )
	{
		s.WriteKeyStr( "LASTIP", m_Last_IP.GetAddrStr());
	}
	if ( ! m_sChatName.IsEmpty())
	{
		s.WriteKeyStr( "CHATNAME", m_sChatName.GetBuffer());
	}
	if ( m_lang.IsDef())
	{
		s.WriteKeyStr( "LANG", m_lang.GetStr());
	}

	// Write New variables
	m_BaseDefs.r_WritePrefix(s);

	m_TagDefs.r_WritePrefix(s, "TAG");
}

enum AV_TYPE
{
	AV_BLOCK,
	AV_DELETE,
	AV_KICK,
	AV_TAGLIST
};

lpctstr const CAccount::sm_szVerbKeys[] =
{
	"BLOCK",
	"DELETE",
	"KICK",
	"TAGLIST",
	nullptr,
};

bool CAccount::r_Verb( CScript &s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CAccount::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

	lpctstr ptcKey = s.GetKey();

	// can't change accounts higher than you in any way
	if (( pSrc->GetPrivLevel() < GetPrivLevel() ) &&  ( pSrc->GetPrivLevel() < PLEVEL_Admin ))
		return false;

	if ( !strnicmp(ptcKey, "CLEARTAGS", 9) )
	{
		ptcKey = s.GetArgStr();
		SKIP_SEPARATORS(ptcKey);
		m_TagDefs.ClearKeys(ptcKey);
		return true;
	}

	int i = FindTableSorted( s.GetKey(), sm_szVerbKeys, CountOf( sm_szVerbKeys )-1 );
	if ( i < 0 )
	{
		bool fLoad = CScriptObj::r_Verb( s, pSrc );
		if ( !fLoad ) //try calling custom functions
		{
            // RES_FUNCTION call
			CSString sVal;
			CScriptTriggerArgs Args( s.GetArgRaw() );
			fLoad = r_Call( ptcKey, pSrc, &Args, &sVal );
		}
		return fLoad;
	}

	switch ( i )
	{
		case AV_DELETE: // "DELETE"
			{
				CClient * pClient = FindClient();
				lpctstr sCurrentName = GetName();

				if ( pClient )
				{
					pClient->CharDisconnect();
					pClient->GetNetState()->markReadClosed();
				}

				char *z = Str_GetTemp();
				if (g_Accounts.Account_Delete(this))
				{
					snprintf(z, STR_TEMPLENGTH, "Account %s deleted.\n", sCurrentName);
				}
				else
				{
					snprintf(z, STR_TEMPLENGTH, "Cannot delete account %s.\n", sCurrentName);
				}

				g_Log.EventStr(0, z);
				if (pSrc && (pSrc != &g_Serv))
					pSrc->SysMessage(z);

				return true;
			}
		case AV_BLOCK:
			if ( ! s.HasArgs() || s.GetArgVal())
				SetPrivFlags(PRIV_BLOCKED);
			else
				ClearPrivFlags(PRIV_BLOCKED);
			return true;

		case AV_KICK:
			// if they are online right now then kick them!
			{
				CClient * pClient = FindClient();
				if ( pClient )
				{
					pClient->addKick( pSrc, i == AV_BLOCK );
				}
				else
				{
					Kick( pSrc, i == AV_BLOCK );
				}
			}
			return true;

		case AV_TAGLIST:
			m_TagDefs.DumpKeys( pSrc, "TAG." );
			return true;

		default:
			break;
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}
