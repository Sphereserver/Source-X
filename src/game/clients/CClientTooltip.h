
#pragma once
#ifndef _INC_CCLIENTTOOLTIP_H
#define _INC_CCLIENTTOOLTIP_H

#include "../common/common.h"
#include "../common/graycom.h"


// Storage for Tooltip data while in trigger on an item
class CClientTooltip
{
public:
    static const char *m_sClassName;
    DWORD m_clilocid;
    TCHAR m_args[SCRIPT_MAX_LINE_LEN];

public:
    explicit CClientTooltip(DWORD clilocid, LPCTSTR args = NULL);

private:
    CClientTooltip(const CClientTooltip& copy);
    CClientTooltip& operator=(const CClientTooltip& other);

public:
    void __cdecl FormatArgs(LPCTSTR format, ...) __printfargs(2,3);
};

#endif // _INC_CCLIENTTOOLTIP_H
