#ifndef _INC_CSCRIPTPARSERBUFS_H
#define _INC_CSCRIPTPARSERBUFS_H

#include "CScriptTriggerArgs.h"


using CScriptTriggerArgsPtr = std::shared_ptr<CScriptTriggerArgs>;
/*
class CScriptTriggerArgsPtr : public std::shared_ptr<CScriptTriggerArgs>
{
    CScriptTriggerArgsPtr(std::nullptr_t) = delete;
}
*/


class CScriptParserBufs
{
    CScriptParserBufs() noexcept = default;
    ~CScriptParserBufs() noexcept = default;

public:
    //static CScriptSubExprStatePtr GetCScriptSubExprStatePtr();
    static CScriptTriggerArgsPtr  GetCScriptTriggerArgsPtr();
};

#endif // _INC_CSCRIPTPARSERBUFS_H
