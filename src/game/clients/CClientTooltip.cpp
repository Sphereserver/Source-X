#include "../../common/sphere_library/sstring.h"
#include "CClientTooltip.h"
#include <cstdarg>

CClientTooltip::CClientTooltip(dword dwClilocID)
{
    m_clilocid = dwClilocID;
    m_args[0] = '\0';
}

CClientTooltip::CClientTooltip(dword dwClilocID, lpctstr ptcArgs)
{
    ASSERT(ptcArgs);
    m_clilocid = dwClilocID;
    Str_CopyLimitNull(m_args, ptcArgs, MAX_TOOLTIP_LEN);
}

CClientTooltip::CClientTooltip(dword dwClilocID, int64 iArgs)
{
    m_clilocid = dwClilocID;
    snprintf(m_args, MAX_TOOLTIP_LEN - 1, "%" PRId64, iArgs);
}

void CClientTooltip::FormatArgs(lpctstr format, ...)
{
    va_list vargs;
    va_start( vargs, format );

    if ( ! vsnprintf( m_args, MAX_TOOLTIP_LEN - 1, format, vargs ) )
        Str_CopyLimitNull( m_args, format, MAX_TOOLTIP_LEN );

    va_end( vargs );
}
