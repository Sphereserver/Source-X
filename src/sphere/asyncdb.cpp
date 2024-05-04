
#include "../common/CScriptObj.h"
#include "../common/CScriptTriggerArgs.h"
#include "../game/triggers.h"
#include "../game/CServer.h"
#include "asyncdb.h"

CDataBaseAsyncHelper g_asyncHdb;

CDataBaseAsyncHelper::CDataBaseAsyncHelper(void) : AbstractSphereThread("AsyncDatabaseHelper", IThread::Low)
{
}

CDataBaseAsyncHelper::~CDataBaseAsyncHelper(void)
{
}

void CDataBaseAsyncHelper::onStart()
{
	AbstractSphereThread::onStart();
}

void CDataBaseAsyncHelper::tick()
{
	if ( !m_queriesTodo.empty() )
	{
		QueryBlob_t currentPair;
		{
			SimpleThreadLock lock(m_queryMutex);
			currentPair = m_queriesTodo.front();
			m_queriesTodo.pop_front();
		}

		FunctionQueryPair_t currentFunctionPair = currentPair.second;

		CScriptTriggerArgs * theArgs = new CScriptTriggerArgs();
		theArgs->m_iN1 = currentPair.first;
		theArgs->m_s1 = currentFunctionPair.second;

		if ( currentPair.first )
			theArgs->m_iN2 = g_Serv._hDb.query(currentFunctionPair.second, theArgs->m_VarsLocal);
		else
			theArgs->m_iN2 = g_Serv._hDb.exec(currentFunctionPair.second);

		g_Serv._hDb.addQueryResult(currentFunctionPair.first, theArgs);
	}
}

void CDataBaseAsyncHelper::waitForClose()
{
	{
		SimpleThreadLock stlThelock(m_queryMutex);

		m_queriesTodo.clear();
	}

	AbstractSphereThread::waitForClose();
}

void CDataBaseAsyncHelper::addQuery(bool isQuery, lpctstr sFunction, lpctstr sQuery)
{
	SimpleThreadLock stlThelock(m_queryMutex);

	m_queriesTodo.emplace_back( QueryBlob_t(isQuery, FunctionQueryPair_t(CSString(sFunction), CSString(sQuery))) );
}
