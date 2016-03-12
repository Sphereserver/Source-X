
#include "../common/CString.h"
#include "CClientTooltip.h"

CClientTooltip::CClientTooltip(DWORD clilocid, LPCTSTR args)
{
    m_clilocid = clilocid;
    if ( args )
        strcpylen(m_args, args, SCRIPT_MAX_LINE_LEN);
    else
        m_args[0] = '\0';
}

void __cdecl CClientTooltip::FormatArgs(LPCTSTR format, ...)
{
    va_list vargs;
    va_start( vargs, format );

    if ( ! _vsnprintf( m_args, SCRIPT_MAX_LINE_LEN, format, vargs ) )
        strcpylen( m_args, format, SCRIPT_MAX_LINE_LEN );

    va_end( vargs );
}