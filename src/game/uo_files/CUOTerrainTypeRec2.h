#pragma once
#ifndef _INC_UOFILES_CUOTERRAINTYPEREC2_H
#define _INC_UOFILES_CUOTERRAINTYPEREC2_H

#include "../../common/common.h"

// All these structures must be byte packed.
#if defined _WINDOWS && (!__MINGW32__)
// Microsoft dependant pragma
#pragma pack(1)
#define PACK_NEEDED
#else
// GCC based compiler you can add:
#define PACK_NEEDED __attribute__ ((packed))
#endif

/**
* size = 0x1e = 30 (tiledata.mul, High Seas+)
* First half of tiledata.mul file is for terrain tiles.
*/
struct CUOTerrainTypeRec2
{
    dword m_flags;  ///< 0xc0=water, 0x40=dirt or rock, 0x60=lava, 0x50=cave, 0=floor
    dword m_unknown;
    word m_index;  ///< just counts up.  0 = unused.
    char m_name[20];
} PACK_NEEDED;

// 0x78800 = (( 0x4000 / 32 ) * 4 ) + ( 0x4000 * 30 )
#define UOTILE_TERRAIN_SIZE2 ((( TERRAIN_QTY / UOTILE_BLOCK_QTY ) * 4 ) + ( TERRAIN_QTY * sizeof( CUOTerrainTypeRec2 )))

// Turn off structure packing.
#if defined _WINDOWS && (!__MINGW32__)
#pragma pack()
#else
#undef PACK_NEEDED
#endif

#endif //_INC_UOFILES_CUOTERRAINTYPEREC2_H
