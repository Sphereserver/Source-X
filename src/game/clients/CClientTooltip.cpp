
#include <cstdio>

#include "../../common/sphere_library/CSString.h"
#include "CClientTooltip.h"


CClientTooltip::CClientTooltip(dword clilocid, lpctstr args)
{
    m_clilocid = clilocid;
    if ( args )
        strcpylen(m_args, args, SCRIPT_MAX_LINE_LEN);
    else
        m_args[0] = '\0';
}

void __cdecl CClientTooltip::FormatArgs(lpctstr format, ...)
{
    va_list vargs;
    va_start( vargs, format );

    if ( ! vsnprintf( m_args, SCRIPT_MAX_LINE_LEN, format, vargs ) )
        strcpylen( m_args, format, SCRIPT_MAX_LINE_LEN );

    va_end( vargs );
}
