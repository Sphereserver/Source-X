#ifndef _INC_CSCRIPTPARSERBUFS_H
#define _INC_CSCRIPTPARSERBUFS_H

#include "CScriptTriggerArgs.h"


struct CScriptSubExprData
{
    lptstr ptcStart, ptcEnd;
    enum Type : ushort
    {
        Unknown = 0,
        // Powers of two
        MaybeNestedSubexpr	        = 0x1 << 0, // 001
        TopParenthesizedExpr        = 0x1 << 1, // 002
        None				        = 0x1 << 2, // 004
        BinaryNonLogical	        = 0x1 << 3, // 008
        And					        = 0x1 << 4, // 010
        Or					        = 0x1 << 5  // 020
    };
    ushort uiType;
    ushort uiNonAssociativeOffset; // How much bytes/characters before the start is (if any) the first non-associative operator preceding the subexpression.
};
using CScriptSubExprDataPtr = std::unique_ptr<CScriptSubExprData>;

struct CScriptExprContextData
{
    // Recursion counters and state variables
    short _iEvaluate_Conditional_Reentrant;
    short _iParseScriptText_Reentrant;
    bool  _fParseScriptText_Brackets;
};
using CScriptExprContextPtr = std::shared_ptr<CScriptExprContextData>;

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
    //static CScriptSubExprDataPtr GetCScriptSubExprDataPtr();
    static CScriptExprContextPtr GetCScriptExprContextDataPtr();
    static CScriptTriggerArgsPtr  GetCScriptTriggerArgsPtr();
};

#endif // _INC_CSCRIPTPARSERBUFS_H
