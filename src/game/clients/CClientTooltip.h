/**
* @file CClientTooltip.h
*
*/

#pragma once
#ifndef _INC_CCLIENTTOOLTIP_H
#define _INC_CCLIENTTOOLTIP_H

#include "../common/common.h"
#include "../common/spherecom.h"


// Storage for Tooltip data while in trigger on an item
class CClientTooltip
{
public:
    static const char *m_sClassName;
    dword m_clilocid;
    tchar m_args[SCRIPT_MAX_LINE_LEN];

public:
    explicit CClientTooltip(dword clilocid, lpctstr args = NULL);

private:
    CClientTooltip(const CClientTooltip& copy);
    CClientTooltip& operator=(const CClientTooltip& other);

public:
    void __cdecl FormatArgs(lpctstr format, ...) __printfargs(2,3);
};


#endif // _INC_CCLIENTTOOLTIP_H
