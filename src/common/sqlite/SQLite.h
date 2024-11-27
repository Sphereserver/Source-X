/**
* @file SQLite.h
* @brief Sphere wrapper for SQLite functions.
*/

#ifndef _INC_SQLITE_H
#define _INC_SQLITE_H

#include "../sphere_library/CSString.h"
#include "../CScriptObj.h"
#include "../CVarDefMap.h"
#include <vector>


//////////////////////////////////////////////////////////////////////////
// Typedefs
//////////////////////////////////////////////////////////////////////////

typedef std::vector<tchar> stdvstring;
typedef std::vector<stdvstring> vstrlist;
typedef vstrlist row;


//////////////////////////////////////////////////////////////////////////
// Classes
//////////////////////////////////////////////////////////////////////////

// Forward declarations
struct sqlite3;
class SQLiteTable;
class SQLiteTablePtr;


// Main wrapper
class CSQLite : public CScriptObj
{
public:
	static const char *m_sClassName;
	//	construction
	CSQLite();
	~CSQLite();

	int Open( lpctstr strFileName );
	void Close();
	bool IsOpen();


	sqlite3 * GetPtr() const    { return m_sqlite3; };
	int GetLastError() const    { return m_iLastError; };
	void ClearError()           { m_iLastError=0; };  // SQLITE_OK = 0

	SQLiteTablePtr QuerySQLPtr( lpctstr strSQL );
	SQLiteTable QuerySQL( lpctstr strSQL );
	int QuerySQL( lpctstr strSQL,  CVarDefMap & mapQueryResult );
	int ExecuteSQL( lpctstr strSQL );
	int IsSQLComplete( lpctstr strSQL );

    int ImportDB(lpctstr strInFileName);
    int ExportDB(lpctstr strOutFileName);

	int GetLastChangesCount();
    llong GetLastInsertRowID();


	bool BeginTransaction();
	bool CommitTransaction();
	bool RollbackTransaction();


	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;

	virtual lpctstr GetName() const override
	{
		return "SQLite_OBJ";
	}

	CVarDefMap	m_QueryResult;
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];

private:
	sqlite3 * m_sqlite3;
	int m_iLastError;

    CSString _sFileName;
    bool _fInMemory;

	static void ConvertUTF8ToVString( const char * strInUTF8MB, stdvstring & strOut );
};

// Table class...
class SQLiteTable
{
	friend class CSQLite;

public:
	SQLiteTable();
	virtual ~SQLiteTable();

	// Gets the number of columns
	int GetColCount(){ return m_iCols; };

	// Gets the number of rows
	int GetRowCount(){ return m_iRows; };

	// Gets the current selected row. -1 when no rows exists.
	int GetCurRow(){ return m_iPos; };

	// Gets the column name at m_iCol index.
	// Returns null if the index is out of bounds.
	lpctstr GetColName( int iCol );

	void ResetRow(){ m_iPos = -1; };

	// Sets the 'iterator' to the first row
	// returns false if fails (no records)
	bool GoFirst();

	// Sets the 'iterator' to the last row
	// returns false if fails (no records)
	bool GoLast();

	// Sets the 'iterator' to the next row
	// returns false if fails (reached end)
	bool GoNext();

	// Sets the 'iterator' to the previous row
	// returns false if fails (reached beginning)
	bool GoPrev();

	// Sets the 'iterator' to the [iRow] row
	// returns false if fails (out of bounds)
	bool GoRow(uint iRow);

	// Gets the value of lpColName column, in the current row
	// returns null if fails (no records)
	lpctstr GetValue(lpctstr lpColName);

	// Gets the value of iColIndex column, in the current row
	// returns null if fails (no records)
	lpctstr GetValue(int iColIndex);

	// Gets the value of lpColName column, in the current row
	// returns null if fails (no records)
	lpctstr operator [] (lpctstr lpColName);

	// Gets the value of iColIndex column, in the current row
	// returns null if fails (no records)
	lpctstr operator [] (int iColIndex);

	// Adds the rows from another table to this table
	// It is the caller's reponsibility to make sure
	// The two tables are matching
	void JoinTable(SQLiteTable & tblJoin);

private:
	int m_iRows, m_iCols;

	row m_strlstCols;
	std::vector<row> m_lstRows;

	int m_iPos;
};

// A class to contain a pointer to a Table class,
// Which will spare the user from freeing a pointer.
class SQLiteTablePtr
{
public:
	// Default constructor
	SQLiteTablePtr( );

	// Construct from a Table*
	SQLiteTablePtr( SQLiteTable * pTable );

	// Copy constructor.
	// Will prevent the original TablePtr from deleting the table.
	// If you have a previous table connected to this class,
	//   you do not have to worry,
	//   it will commit suicide before eating the new table.
    SQLiteTablePtr( SQLiteTablePtr & cTablePtr );
    SQLiteTablePtr( const SQLiteTablePtr & cTablePtr ) = delete;

	// Destructor...
	virtual ~SQLiteTablePtr();

	// Copy operator.
	// Will prevent the original TablePtr from deleting the table.
	// If you have a previous table connected to this class,
	//   you do not have to worry,
	//   it will commit suicide before eating the new table.
    void operator =(SQLiteTablePtr & cTablePtr);
    void operator =(const SQLiteTablePtr & cTablePtr) = delete;

	// Functor operator, will de-reference the m_pTable member.
	// WARNING: Use with care! Check for non-null m_pTable first!
	SQLiteTable& operator()() { return *m_pTable; };

	// bool operator, to check if m_pTable is valid.
	operator bool() { return m_pTable != nullptr; };

	// Detaches the class from the Table,
	// and returns the Table that were just detached...
	SQLiteTable * Detach();

	// Frees the current Table, and attaches the pTable.
	void Attach(SQLiteTable * pTable);

	// Frees the current Table.
	void Destroy();

	// Pointer to the table.
	// I do not see any reason for encapsulating in Get/Set functions.
	SQLiteTable * m_pTable;
};


#endif // _INC_SQLITE_H
