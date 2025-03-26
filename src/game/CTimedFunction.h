/**
* @file CTimedFunction.h
*
*/

#ifndef _INC_CTIMEDFUNCTION_H
#define _INC_CTIMEDFUNCTION_H

#include "../common/CUID.h"
#include "../common/sphere_library/CSObjContRec.h"
#include "CTimedObject.h"


class CTimedFunctionHandler;

class CTimedFunction : public CTimedObject, CSObjContRec
{
public:
    static constexpr uint kuiCommandSize = 1024;

private:
    friend class CTimedFunctionHandler;

    CUID  _uidAttached;
    tchar _ptcCommand[kuiCommandSize];

public:
    CTimedFunction(const CUID& uidAttached, const char * pcCommand);
    ~CTimedFunction() = default; // Removal from ticking list is already managed by CTimedObject destructor

    const CUID& GetUID() const {
        return _uidAttached;
    }
    lpctstr GetCommand() const noexcept {
        return _ptcCommand;
    }

protected:  virtual bool _CanTick() const override;
public:     virtual bool  CanTick() const override;

protected:	virtual bool _OnTick() override;
public:		virtual bool  OnTick() override;

protected:  virtual bool _IsDeleted() const override;
public:     virtual bool IsDeleted() const override; // abstract
};

#endif // _INC_CTIMEDFUNCTION_H
