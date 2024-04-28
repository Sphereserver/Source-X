/**
* @file CDataBase.h
* @brief mySQL wrapper for easier data operations witheen in-game server.
*/

#ifndef _INC_CDATABASE_H
#define	_INC_CDATABASE_H

#include "sphere_library/CSString.h"
#include "sphere_library/smutex.h"
#include "CScriptObj.h"
#include "CVarDefMap.h"
#include <vector>
#include <queue>

#include "../../lib/mariadb/errmsg.h"	// mysql standard include
#include "../../lib/mariadb/mysql.h"	// this needs to be defined AFTER common.h


#define	MIN_MARIADB_VERSION_ALLOW	30002


class CVarDefMap;

class CDataBase : public CScriptObj
{
public:
	static const char *m_sClassName;
	//	construction
	CDataBase();
	~CDataBase();

private:
	CDataBase(const CDataBase& copy);
	CDataBase& operator=(const CDataBase& other);

public:
	bool Connect(const char *user, const char *password, const char *base = "", const char *host = "localhost");
	bool Connect();
	void Close();							//	close link with db

	//	select
	bool query(const char *query, CVarDefMap & mapQueryResult);			//	proceeds the query for SELECT
	bool __cdecl queryf(CVarDefMap & mapQueryResult, char *fmt, ...) __printfargs(3,4);
	bool exec(const char *query);			//	executes query (pretty faster) for ALTER, UPDATE, INSERT, DELETE, ...
	bool __cdecl execf(char *fmt, ...) __printfargs(2,3);
	void addQueryResult(CSString & theFunction, CScriptTriggerArgs * theResult);

	//	set / get / info methods
	bool isConnected();
	bool _OnTick();

	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;

	virtual lpctstr GetName() const override
	{
		return "SQL_OBJ";
	}

public:
	CVarDefMap	m_QueryResult;
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];

private:
	typedef std::pair<CSString, CScriptTriggerArgs *> FunctionArgsPair_t;
	typedef std::queue<FunctionArgsPair_t> QueueFunction_t;

protected:
	bool	m_fConnected;					//	are we online?
	MYSQL	*_myData;						//	mySQL link
	QueueFunction_t m_QueryArgs;

private:
	SimpleMutex m_connectionMutex;
	SimpleMutex m_resultMutex;
	bool addQuery(bool isQuery, lpctstr theFunction, lpctstr theQuery);
};

#endif // _INC_CDATABASE_H
