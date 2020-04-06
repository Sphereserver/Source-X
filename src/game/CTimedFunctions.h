/**
* @file CTimedFunctions.h
*
*/

#ifndef _INC_CTIMEDFUNCTIONS_H
#define _INC_CTIMEDFUNCTIONS_H

#include "../common/CScriptContexts.h"
#include "../common/CScriptObj.h"
#include "../common/CUID.h"

struct TimedFunction;
class CScript;
class CString;

class CTimedFunctions
{
public:
    static int Load(const char *pszName, bool fQuoted, const char *pszVal);
    static void Add(CUID uid, int numSeconds, lpctstr funcname);
    static void Erase(CUID uid);
    static void Stop(CUID uid, lpctstr funcname);
    static void Clear();
    static TRIGRET_TYPE Loop(lpctstr funcname, int LoopsMade, CScriptLineContext StartContext,
        CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult);
    static int IsTimer(CUID uid, lpctstr funcname);
};
#endif // _INC_CTIMEDFUNCTIONS_H
