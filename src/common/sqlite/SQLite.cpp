#include "../../common/CLog.h"
#include "../../sphere/threads.h"
#include "../CExpression.h"
#include "../CException.h"
#include "../CScript.h"
#include "SQLite.h"
#include "../../lib/sqlite/sqlite3.h"


CSQLite::CSQLite()
{
	m_sqlite3=nullptr;
	m_iLastError=SQLITE_OK;
    _fInMemory = false;
}

CSQLite::~CSQLite()
{
	Close();
}

int CSQLite::Open( lpctstr strFileName )
{
	Close();

	int iErr=sqlite3_open(UTF8MBSTR(strFileName), &m_sqlite3);

	if (iErr!=SQLITE_OK)
    {
        m_iLastError=iErr;
    }
    else
    {
        if (!strcmp(":memory:", strFileName))
            _fInMemory = true;
        else
            _sFileName = strFileName;
    }

	return iErr;
}

void CSQLite::Close()
{
    _sFileName.Clear();
    _fInMemory = false;
	if (m_sqlite3)
	{
		sqlite3_close(m_sqlite3);
		m_sqlite3=nullptr;
	}
}

bool CSQLite::IsOpen()
{
	return (m_sqlite3 != nullptr);
}

int CSQLite::QuerySQL( lpctstr strSQL,  CVarDefMap & mapQueryResult )
{
	mapQueryResult.Clear();
	mapQueryResult.SetNumNew("NUMROWS", 0);

	TablePtr retTable = QuerySQLPtr(strSQL);
    if (retTable.m_pTable == nullptr)
        goto err_and_ret;

	if (retTable.m_pTable->GetRowCount())
	{
		int iRows = retTable.m_pTable->GetRowCount();
		int iCols = retTable.m_pTable->GetColCount();
		mapQueryResult.SetNum("NUMROWS", iRows);
		mapQueryResult.SetNum("NUMCOLS", iCols);

		char *pcStore = Str_GetTemp();
		for (int iRow=0; iRow<iRows; ++iRow)
		{
			retTable.m_pTable->GoRow(iRow);
			for (int iCol=0; iCol<iCols; ++iCol)
			{
                snprintf(pcStore, STR_TEMPLENGTH, "%d.%d", iRow, iCol);
				mapQueryResult.SetStr(pcStore, true, retTable.m_pTable->GetValue(iCol));
                snprintf(pcStore, STR_TEMPLENGTH, "%d.%s", iRow, retTable.m_pTable->GetColName(iCol));
				mapQueryResult.SetStr(pcStore, true, retTable.m_pTable->GetValue(iCol));
			}
		}
	}

err_and_ret:
    if (m_iLastError!=SQLITE_OK)
        g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite query \"%s\" failed. Error: %d\n", strSQL, m_iLastError);
	return m_iLastError;
}

Table CSQLite::QuerySQL( lpctstr strSQL )
{
	if (!IsOpen()) {
		m_iLastError=SQLITE_ERROR;
		return Table();
	}

	char ** retStrings = nullptr;
	char * errmsg = nullptr;
	int iRows=0, iCols=0;

	int iErr=sqlite3_get_table(m_sqlite3, UTF8MBSTR(strSQL), &retStrings,
		&iRows, &iCols, &errmsg);

	if (iErr!=SQLITE_OK)
	{
		m_iLastError=iErr;
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite query \"%s\" failed. Error: %d\n", strSQL, iErr);
	}

	sqlite3_free(errmsg);

	Table retTable;

	if (iRows>0)
        retTable.m_iPos=0;

	retTable.m_iCols=iCols;
	retTable.m_iRows=iRows;
	retTable.m_strlstCols.reserve(iCols);

	int iPos=0;

	for (; iPos<iCols; ++iPos)
	{
		retTable.m_strlstCols.emplace_back(stdvstring());

		if (retStrings[iPos])
			ConvertUTF8ToVString( retStrings[iPos], retTable.m_strlstCols.back() );
		else
            retTable.m_strlstCols.back().emplace_back('\0');
	}

	retTable.m_lstRows.resize(iRows);
	for (int iRow=0; iRow<iRows; ++iRow)
	{
		retTable.m_lstRows[iRow].reserve(iCols);
		for (int iCol=0; iCol<iCols; ++iCol)
		{
			retTable.m_lstRows[iRow].emplace_back(stdvstring());

			if (retStrings[iPos])
				ConvertUTF8ToVString( retStrings[iPos], retTable.m_lstRows[iRow].back() );
			else
                retTable.m_lstRows[iRow].back().emplace_back('\0');

			++iPos;
		}
	}

	sqlite3_free_table(retStrings);
    m_iLastError=SQLITE_OK;

	return retTable;
}

TablePtr CSQLite::QuerySQLPtr( lpctstr strSQL )
{
	if (!IsOpen())
    {
		m_iLastError=SQLITE_ERROR;
		return nullptr;
	}

	char ** retStrings = nullptr;
	char * errmsg = nullptr;
	int iRows=0, iCols=0;

	int iErr=sqlite3_get_table(m_sqlite3, UTF8MBSTR(strSQL), &retStrings,
		&iRows, &iCols, &errmsg);

	if (iErr!=SQLITE_OK)
	{
		m_iLastError=iErr;
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite query \"%s\" failed. Error: %d\n", strSQL, iErr);
	}

	sqlite3_free(errmsg);

	Table * retTable = new Table();

	if (iRows>0)
        retTable->m_iPos=0;

	retTable->m_iCols=iCols;
	retTable->m_iRows=iRows;
	retTable->m_strlstCols.reserve(iCols);

	int iPos=0;

	for (; iPos<iCols; ++iPos)
	{
		retTable->m_strlstCols.emplace_back(stdvstring());

		if (retStrings[iPos])
			ConvertUTF8ToVString( retStrings[iPos], retTable->m_strlstCols.back() );
		else
            retTable->m_strlstCols.back().emplace_back('\0');
	}

	retTable->m_lstRows.resize(iRows);
	for (int iRow=0; iRow<iRows; ++iRow)
	{
		retTable->m_lstRows[iRow].reserve(iCols);
		for (int iCol=0; iCol<iCols; ++iCol)
		{
			retTable->m_lstRows[iRow].emplace_back(stdvstring());

			if (retStrings[iPos])
				ConvertUTF8ToVString( retStrings[iPos], retTable->m_lstRows[iRow].back() );
			else
                retTable->m_lstRows[iRow].back().emplace_back('\0');

			++iPos;
		}
	}
	sqlite3_free_table(retStrings);
    m_iLastError=SQLITE_OK;

	return TablePtr(retTable);
}

void CSQLite::ConvertUTF8ToVString( const char * strInUTF8MB, stdvstring & strOut )
{
    const size_t len = Str_LengthUTF8(strInUTF8MB);
    strOut.resize(len + 1, 0);
    lptstr ptcStrOut = strOut.data();
	ASSERT(ptcStrOut);
    UTF8MBSTR::ConvertUTF8ToString(strInUTF8MB, ptcStrOut);
}

int CSQLite::ExecuteSQL( lpctstr strSQL )
{
	if (!IsOpen())
    {
		m_iLastError=SQLITE_ERROR;
		return SQLITE_ERROR;
	}

	char * errmsg = nullptr;

	int iErr = sqlite3_exec( m_sqlite3, UTF8MBSTR(strSQL), nullptr, nullptr, &errmsg );

	if (iErr!=SQLITE_OK)
	{
		m_iLastError=iErr;
		g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite query \"%s\" failed. Error: %d\n", strSQL, iErr);
	}

	sqlite3_free(errmsg);

	return iErr;
}

int CSQLite::IsSQLComplete( lpctstr strSQL )
{
	return sqlite3_complete( UTF8MBSTR(strSQL) );
}

int CSQLite::ImportDB(lpctstr strInFileName)
{
    if (!CSFile::FileExists(strInFileName))
        return SQLITE_CANTOPEN;
    //if (IsStrEmpty(strTable))
    //    return SQLITE_ERROR;

    int iErr;

    sqlite3 *in_db;
    iErr = sqlite3_open(strInFileName, &in_db);
    if (iErr != SQLITE_OK)
        goto clean_and_ret;

    /*
    char pcQuery[100];
    char *pcErrMsg = nullptr;

    // Does the table exist in the loaded db?
    sqlite3_snprintf(int(sizeof(pcQuery)), pcQuery, "SELECT sql FROM sqlite_master WHERE type='table' AND name='%s'", strTable);
    iErr = sqlite3_exec( in_db, UTF8MBSTR(pcQuery), nullptr, nullptr, &pcErrMsg );
    if (iErr != SQLITE_OK)
        goto clean_and_ret;
    
    // Copy the table schema: how?

    // End
    clean_and_ret:
        sqlite3_free(pcErrMsg);
        sqlite3_close(in_db);
        if (iErr != SQLITE_OK)
        {
            m_iLastError=iErr;
            g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite ImportDB failed. Error: %d\n", iErr);
        }
        return iErr;
    */

    sqlite3_backup *in_backup;
    in_backup = sqlite3_backup_init(m_sqlite3, "main", in_db, "main");
    if (in_backup == nullptr)
    {
        iErr = sqlite3_errcode(m_sqlite3);
        goto clean_and_ret;
    }

    iErr = sqlite3_backup_step(in_backup, -1);
    if (iErr != SQLITE_DONE)
        goto clean_and_ret;

    iErr = sqlite3_backup_finish(in_backup);
    //if (iErr != SQLITE_OK)
    //    goto clean_and_ret;

clean_and_ret:
    sqlite3_close(in_db);
    if (iErr != SQLITE_OK)
    {
        m_iLastError=iErr;
        g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite ImportDB failed. Error: %d\n", iErr);
    }
    return iErr;
}

int CSQLite::ExportDB(lpctstr strOutFileName)
{
    int iErr;

    sqlite3 *out_db;
    iErr = sqlite3_open(strOutFileName, &out_db);
    if (iErr != SQLITE_OK)
        goto clean_and_ret;

    sqlite3_backup *out_backup;
    out_backup = sqlite3_backup_init(out_db, "main", m_sqlite3, "main");
    if (out_backup == nullptr)
    {
        iErr = sqlite3_errcode(out_db);
        goto clean_and_ret;
    }

    iErr = sqlite3_backup_step(out_backup, -1);
    if (iErr != SQLITE_DONE)
        goto clean_and_ret;

    iErr = sqlite3_backup_finish(out_backup);
    //if (iErr != SQLITE_OK)
    //    goto clean_and_ret;

clean_and_ret:
    sqlite3_close(out_db);
    if (iErr != SQLITE_OK)
    {
        m_iLastError=iErr;
        g_Log.Event(LOGM_NOCONTEXT|LOGL_ERROR, "SQLite ExportDB failed. Error: %d\n", iErr);
    }
    return iErr;
}

int CSQLite::GetLastChangesCount()
{
	return sqlite3_changes(m_sqlite3);
}

llong CSQLite::GetLastInsertRowID()
{
	if (m_sqlite3==nullptr)
        return 0; // RowID's starts with 1...

	return llong(sqlite3_last_insert_rowid(m_sqlite3));
}

bool CSQLite::BeginTransaction()
{
	if (!IsOpen())
	{
		m_iLastError=SQLITE_ERROR;
		return false;
	}
	m_iLastError = ExecuteSQL("BEGIN TRANSACTION");
	return m_iLastError==SQLITE_OK;
}

bool CSQLite::CommitTransaction()
{
	if (!IsOpen())
	{
		m_iLastError=SQLITE_ERROR;
		return false;
	}
	m_iLastError = ExecuteSQL("COMMIT TRANSACTION");
	return m_iLastError==SQLITE_OK;
}

bool CSQLite::RollbackTransaction()
{
	if (!IsOpen())
	{
		m_iLastError=SQLITE_ERROR;
		return false;
	}
	m_iLastError = ExecuteSQL("ROLLBACK TRANSACTION");
	return m_iLastError==SQLITE_OK;
}

// CScriptObj functions

enum LDBO_TYPE
{
	LDBO_CONNECTED,
    LDBO_FILENAME,
	LDBO_ROW,
	LDBO_QTY
};

lpctstr const CSQLite::sm_szLoadKeys[LDBO_QTY+1] =
{
	"CONNECTED",
    "FILENAME",
	"ROW",
	nullptr
};

enum LDBOV_TYPE
{
	LDBOV_CLOSE,
	LDBOV_CONNECT,
	LDBOV_EXECUTE,
    LDBOV_EXPORTDB,
    LDBOV_IMPORTDB,
	LDBOV_QUERY,
	LDBOV_QTY
};

lpctstr const CSQLite::sm_szVerbKeys[LDBOV_QTY+1] =
{
	"CLOSE",
	"CONNECT",
	"EXECUTE",
    "EXPORTDB",
    "IMPORTDB",
	"QUERY",
	nullptr
};

bool CSQLite::r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef)
{
	ADDTOCALLSTACK("CSQLite::r_GetRef");
	UnreferencedParameter(ptcKey);
	UnreferencedParameter(pRef);
	return false;
}

bool CSQLite::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CSQLite::r_LoadVal");
	UnreferencedParameter(s);
	return false;
}

bool CSQLite::r_WriteVal(lpctstr ptcKey, CSString &sVal, CTextConsole *pSrc, bool fNoCallParent, bool fNoCallChildren)
{
    UnreferencedParameter(fNoCallParent);
    UnreferencedParameter(fNoCallChildren);
	ADDTOCALLSTACK("CSQLite::r_WriteVal");
	EXC_TRY("WriteVal");

	int index = FindTableHeadSorted(ptcKey, sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys)-1);
	switch ( index )
	{
		case LDBO_CONNECTED:
			sVal.FormatVal(IsOpen());
			break;

        case LDBO_FILENAME:
            if (!_fInMemory)
                sVal = _sFileName;
            break;

		case LDBO_ROW:
		{
			ptcKey += strlen(sm_szLoadKeys[index]);
			SKIP_SEPARATORS(ptcKey);
			sVal = m_QueryResult.GetKeyStr(ptcKey);
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

bool CSQLite::r_Verb(CScript & s, CTextConsole * pSrc)
{
	ADDTOCALLSTACK("CSQLite::r_Verb");
	EXC_TRY("Verb");

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, ARRAY_COUNT(sm_szVerbKeys)-1);
	switch ( index )
	{
		case LDBOV_CLOSE:
            if ( _fInMemory )
                return false; 
			Close();
			break;

		case LDBOV_CONNECT:
            if ( _fInMemory )
                return false; 
            Open(s.GetArgStr());
            break;

		case LDBOV_EXECUTE:
			ExecuteSQL(s.GetArgRaw());
			break;

        case LDBOV_EXPORTDB:
            ExportDB(s.GetArgStr());
            break;

        case LDBOV_IMPORTDB:
            ImportDB(s.GetArgStr());
            break;

		case LDBOV_QUERY:
			QuerySQL(s.GetArgRaw(), m_QueryResult);
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


// Table Class...


lpctstr Table::GetColName( int iCol )
{
	if (iCol>=0 && iCol<m_iCols)
	{
		return &m_strlstCols[iCol][0];
	}
	return 0;
}

bool Table::GoFirst()
{
	if (m_lstRows.size())
	{
		m_iPos=0;
		return true;
	}
	return false;
}

bool Table::GoLast()
{
	if (m_lstRows.size())
	{
		m_iPos=(int)m_lstRows.size()-1;
		return true;
	}
	return false;
}

bool Table::GoNext()
{
	if (m_iPos+1<(int)m_lstRows.size())
	{
		++m_iPos;
		return true;
	}
	return false;
}

bool Table::GoPrev()
{
	if (m_iPos>0)
	{
		--m_iPos;
		return true;
	}
	return false;
}

bool Table::GoRow(uint iRow)
{
	if (iRow<m_lstRows.size())
	{
		m_iPos=iRow;
		return true;
	}
	return false;
}

lpctstr Table::GetValue(lpctstr lpColName)
{
	if (!lpColName)
        return 0;
	if (m_iPos<0)
        return 0;
	for (int i=0; i<m_iCols; ++i)
	{
		if (!strcmpi(&m_strlstCols[i][0],lpColName))
		{
			return &m_lstRows[m_iPos][i][0];
		}
	}
	return 0;
}

lpctstr Table::GetValue(int iColIndex)
{
	if (iColIndex<0 || iColIndex>=m_iCols) return 0;
	if (m_iPos<0) return 0;
	return &m_lstRows[m_iPos][iColIndex][0];
}

lpctstr Table::operator [] (lpctstr lpColName)
{
	if (!lpColName)
        return 0;
	if (m_iPos<0)
        return 0;
	for (int i=0; i<m_iCols; ++i)
	{
		if (!strcmpi(&m_strlstCols[i][0],lpColName))
		{
			return &m_lstRows[m_iPos][i][0];
		}
	}
	return 0;
}

lpctstr Table::operator [] (int iColIndex)
{
	if (iColIndex<0 || iColIndex>=m_iCols)
        return 0;
	if (m_iPos<0)
        return 0;
	return &m_lstRows[m_iPos][iColIndex][0];
}

void Table::JoinTable(Table & tblJoin)
{
	if (m_iCols==0) {
		*this=tblJoin;
		return;
	}
	if (m_iCols!=tblJoin.m_iCols) return;

	if (tblJoin.m_iRows>0)
	{
		m_iRows+=tblJoin.m_iRows;

		for (std::vector<row>::const_iterator it = tblJoin.m_lstRows.begin(), end = tblJoin.m_lstRows.end(); it != end; ++it)
		{
			m_lstRows.emplace_back(*it);
		}
	}
}

TablePtr::TablePtr( )
{
	m_pTable=nullptr;
}

TablePtr::TablePtr( Table * pTable )
{
	m_pTable = pTable;
}

TablePtr::TablePtr( const TablePtr& cTablePtr )
{
	m_pTable=cTablePtr.m_pTable;
	((TablePtr *)&cTablePtr)->m_pTable=nullptr;
}

TablePtr::~TablePtr()
{
	if (m_pTable)
        delete m_pTable;
}

void TablePtr::operator =( const TablePtr& cTablePtr )
{
	if (m_pTable)
        delete m_pTable;
	m_pTable=cTablePtr.m_pTable;
	((TablePtr *)&cTablePtr)->m_pTable=nullptr;
}

Table * TablePtr::Detach()
{
	Table * pTable=m_pTable;
	m_pTable=nullptr;
	return pTable;
}

void TablePtr::Attach( Table * pTable )
{
	if (m_pTable)
        delete m_pTable;
	m_pTable=pTable;
}

void TablePtr::Destroy()
{
	if (m_pTable)
        delete m_pTable;
	m_pTable=nullptr;
}

