/**
* @file CTimedFunctionHandler.h
*
*/

#ifndef _INC_CTIMEDFUNCTIONHANDLER_H
#define _INC_CTIMEDFUNCTIONHANDLER_H

#include "../common/CScriptContexts.h"
#include "../common/CScriptObj.h"
#include "../common/CUID.h"
#include "CServerTime.h"
#include <vector>


class CScript;

class CTimedFunctionHandler
{
public:
    struct TimedFunction
    {
        int		elapsed;
        char	funcname[1024];
        CUID 	uid;
    };

private:
    std::vector<TimedFunction *> m_timedFunctions[TICKS_PER_SEC];
    int m_curTick;
    int m_processedFunctionsPerTick;
    std::vector<TimedFunction *> m_tfRecycled;
    std::vector<TimedFunction *> m_tfQueuedToBeAdded;
    bool m_isBeingProcessed;

public:
    static const char *m_sClassName;
    CTimedFunctionHandler();

private:
    CTimedFunctionHandler(const CTimedFunctionHandler& copy);
    CTimedFunctionHandler& operator=(const CTimedFunctionHandler& other);

public:
    void OnTick();
    void r_Write(CScript & s);

    int Load(const char *pszName, bool fQuoted, const char *pszVal);
    void Add(CUID uid, int numSeconds, lpctstr funcname);
    void Erase(CUID uid);
    void Stop(CUID uid, lpctstr funcname);
    void Clear();
    TRIGRET_TYPE Loop(lpctstr funcname, int LoopsMade, CScriptLineContext StartContext,
        CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult);
    int IsTimer(CUID uid, lpctstr funcname);
};
#endif // _INC_CTIMEDFUNCTIONHANDLER_H
