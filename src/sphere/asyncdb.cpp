
#include "../common/CScriptObj.h"
//#include "../common/CScriptParserBufs.h"
#include "../common/CScriptTriggerArgs.h"
#include "../game/CServer.h"
#include "asyncdb.h"


CDataBaseAsyncHelper::CDataBaseAsyncHelper(void) : AbstractSphereThread("T_AsyncDBHelper", ThreadPriority::Low)
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
    SimpleThreadLock lock(m_queryMutex);
    if ( m_queriesTodo.empty() )
        return;

    QueryBlob_t currentPair;
    currentPair = m_queriesTodo.front();
    m_queriesTodo.pop_front();
    lock.unlock();

    FunctionQueryPair_t currentFunctionPair = currentPair.second;

    // Don't take it from CScriptParserBufs, since we are using a thread-unsafe object pool (script parsing is done in one thread only)
    //CScriptTriggerArgsPtr theArgs = CScriptParserBufs::GetCScriptTriggerArgsPtr();
    auto theArgs = std::make_shared<CScriptTriggerArgs>();
    theArgs->m_iN1 = currentPair.first;
    theArgs->m_s1 = currentFunctionPair.second;

    if ( currentPair.first )
        theArgs->m_iN2 = g_Serv._hDb.query(currentFunctionPair.second, theArgs->m_VarsLocal);
    else
        theArgs->m_iN2 = g_Serv._hDb.exec(currentFunctionPair.second);

    g_Serv._hDb.addQueryResult(currentFunctionPair.first, std::move(theArgs));
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
