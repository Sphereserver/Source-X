/**
* @file CClientTooltip.h
*
*/

#ifndef _INC_CCLIENTTOOLTIP_H
#define _INC_CCLIENTTOOLTIP_H

#include "../../common/common.h"


#define MAX_TOOLTIP_LEN 256

// Storage for Tooltip data while in trigger on an item
class CClientTooltip
{
public:
    static const char *m_sClassName;
    dword m_clilocid;
    tchar m_args[MAX_TOOLTIP_LEN];

public:
    CClientTooltip(dword dwClilocID);
    CClientTooltip(dword dwClilocID, lpctstr ptcArgs);
    CClientTooltip(dword dwClilocID, int64 iArgs);

    CClientTooltip(const CClientTooltip& copy) = delete;
    CClientTooltip& operator=(const CClientTooltip& other) = delete;

public:
    void __cdecl FormatArgs(lpctstr format, ...) __printfargs(2,3);
};


#endif // _INC_CCLIENTTOOLTIP_H
