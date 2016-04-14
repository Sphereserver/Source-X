#include "CUOMapMeter.h"

static bool CUOMapMeter::IsTerrainNull( word wTerrainIndex )
{
    switch ( wTerrainIndex )
    {
        case 0x244: //580
            return true;
    }
    return false;
}
