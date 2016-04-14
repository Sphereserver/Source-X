#include "CUOHuesRec.h"

byte CUOHuesRec::GetRGB( int rgb ) const
{
    short sColor = m_color[31];
    if ( rgb == 0 ) // R
        return (byte)(((sColor & 0x7C00) >> 7));
    else if ( rgb == 1 )
        return (byte)(((sColor & 0x3E0) >> 2));
    else if ( rgb == 3 )
        return (byte)(((sColor & 0x1F) << 3));

    return 0;
}
