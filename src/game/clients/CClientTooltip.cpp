
#include <cstdio>

#include "../../common/sphere_library/CSString.h"
#include "CClientTooltip.h"


CClientTooltip::CClientTooltip(dword clilocid, lpctstr args)
{
    m_clilocid = clilocid;
    if ( args )
        strncpy(m_args, args, MAX_TOOLTIP_LEN - 1);
    else
        m_args[0] = '\0';
}

void __cdecl CClientTooltip::FormatArgs(lpctstr format, ...)
{
    va_list vargs;
    va_start( vargs, format );

    if ( ! vsnprintf( m_args, MAX_TOOLTIP_LEN - 1, format, vargs ) )
        strncpy( m_args, format, MAX_TOOLTIP_LEN - 1 );

    va_end( vargs );
}
