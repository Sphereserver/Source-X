/**
* @file asyncdb.h
* @brief Database async queries.
*/

#ifndef _INC_ASYNCDB_H
#define _INC_ASYNCDB_H

#include "../common/sphere_library/CSString.h"
#include "../common/sphere_library/smutex.h"
#include "threads.h"
#include <deque>


class CDataBaseAsyncHelper : public AbstractSphereThread
{
private:
	typedef std::pair<CSString, CSString> FunctionQueryPair_t;
	typedef std::pair<bool, FunctionQueryPair_t> QueryBlob_t;
	typedef std::deque<QueryBlob_t> QueueQuery_t;

private:
	SimpleMutex m_queryMutex;
	QueueQuery_t m_queriesTodo;

public:
	CDataBaseAsyncHelper(void);
    virtual ~CDataBaseAsyncHelper(void) override;

    CDataBaseAsyncHelper(const CDataBaseAsyncHelper& copy) = delete;
    CDataBaseAsyncHelper& operator=(const CDataBaseAsyncHelper& other) = delete;

public:
    virtual void onStart() override;
    virtual void tick() override;
    virtual void waitForClose() override;

public:
	void addQuery(bool isQuery, lpctstr sFunction, lpctstr sQuery);
};

extern CDataBaseAsyncHelper g_asyncHdb;


#endif // _INC_ASYNCDB_H
