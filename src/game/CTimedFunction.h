/**
* @file CTimedFunction.h
*
*/

#ifndef _INC_CTIMEDFUNCTION_H
#define _INC_CTIMEDFUNCTION_H

#include "../common/CUID.h"
#include "CTimedObject.h"


class CTimedFunction : public CTimedObject
{
public:
    static constexpr uint kuiCommandSize = 1024;

private:
    friend class CTimedFunctionHandler;

    CTimedFunctionHandler* _pHandler;
    CUID  _uidAttached;
    tchar _ptcCommand[kuiCommandSize];

public:
    CTimedFunction(CTimedFunctionHandler* pHandler, const CUID& uidAttached, const char * pcCommand);
    ~CTimedFunction() = default; // Removal from ticking list is already managed by CTimedObject destructor

    const CUID& GetUID() const {
        return _uidAttached;
    }
    lpctstr GetCommand() const noexcept {
        return _ptcCommand;
    }

    virtual bool OnTick() override;
    virtual bool IsDeleted() const override; // abstract
};

#endif // _INC_CTIMEDFUNCTION_H
