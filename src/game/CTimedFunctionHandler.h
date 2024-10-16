/**
* @file CTimedFunctionHandler.h
*
*/

#ifndef _INC_CTIMEDFUNCTIONHANDLER_H
#define _INC_CTIMEDFUNCTIONHANDLER_H

#include "../common/sphere_library/CSObjCont.h"
#include "../common/CScriptContexts.h"
#include "../common/CScriptObj.h"
#include "CTimedFunction.h"


class CScript;
class CUID;

class CTimedFunctionHandler
{
private:
    CSObjCont _timedFunctions;

    std::string _strLoadBufferCommand;
    std::string _strLoadBufferNumbers;

public:
    static const char *m_sClassName;
    CTimedFunctionHandler();

private:
    CTimedFunctionHandler(const CTimedFunctionHandler& copy);
    CTimedFunctionHandler& operator=(const CTimedFunctionHandler& other);

public:
    void r_Write(CScript & s);

    int Load(lpctstr ptcKeyword, bool fQuoted, lpctstr ptcArg);
    void Add(const CUID& uid, int64 iTimeout, const char* pcCommand);
    void ClearUID(const CUID& uid);
    void Stop(const CUID& uid, lpctstr ptcCommand);
    void Clear();
    TRIGRET_TYPE Loop(lpctstr ptcCommand, int iLoopsMade, CScriptLineContext StartContext,
        CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult);
    int64 IsTimer(const CUID& uid, lpctstr ptcCommand) const;
};
#endif // _INC_CTIMEDFUNCTIONHANDLER_H
