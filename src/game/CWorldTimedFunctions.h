/**
* @file CWorldTimedFunctions.h
*
*/

#ifndef _INC_CWORLDTIMEDFUNCTIONS_H
#define _INC_CWORLDTIMEDFUNCTIONS_H

#include "../common/CScriptContexts.h"
#include "../common/CScriptObj.h"
#include "../common/CUID.h"

class CTimedFunction;
class CScript;
class CString;

class CWorldTimedFunctions
{
public:
    static int Load(lpctstr ptcKeyword, bool fQuoted, lpctstr ptcArg);
    static void Add(const CUID& uid, int64 iTimeout, lpctstr ptcCommand);
    static void ClearUID(const CUID& uid);
    static void Stop(const CUID& uid, lpctstr ptcCommand);
    static void Clear();
    static TRIGRET_TYPE Loop(lpctstr ptcCommand, int LoopsMade, CScriptLineContext StartContext,
        CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult);
    static int64 IsTimer(const CUID& uid, lpctstr funcname);
};
#endif // _INC_CWORLDTIMEDFUNCTIONS_H
