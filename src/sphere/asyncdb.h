/**
* @file asyncdb.h
* @brief Database async queries.
*/

#pragma once
#ifndef ASYNCDB_H
#define ASYNCDB_H

#include "threads.h"
#include "mutex.h"
//#include "../game/graysvr.h" Removed to test.


class CDataBaseAsyncHelper : public AbstractSphereThread
{
private:
	typedef std::pair<CGString, CGString> FunctionQueryPair_t;
	typedef std::pair<bool, FunctionQueryPair_t> QueryBlob_t;
	typedef std::deque<QueryBlob_t> QueueQuery_t;

private:
	SimpleMutex m_queryMutex;
	QueueQuery_t m_queriesTodo;

public:
	CDataBaseAsyncHelper(void);
	~CDataBaseAsyncHelper(void);
private:
	CDataBaseAsyncHelper(const CDataBaseAsyncHelper& copy);
	CDataBaseAsyncHelper& operator=(const CDataBaseAsyncHelper& other);

public:
	virtual void onStart();
	virtual void tick();
	virtual void waitForClose();

public:
	void addQuery(bool isQuery, LPCTSTR sFunction, LPCTSTR sQuery);
};

#endif //ASYNCDB_H
