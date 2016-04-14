#pragma once
#ifndef _INC_UOFILES_CUOTERRAININFO_H
#define _INC_UOFILES_CUOTERRAININFO_H

#include "../../common/common.h"
#include "CUOTerrainTypeRec2.h"
#include "types.h"

// All these structures must be byte packed.
#if defined _WIN32 && (!__MINGW32__)
// Microsoft dependant pragma
#pragma pack(1)
#define PACK_NEEDED
#else
// GCC based compiler you can add:
#define PACK_NEEDED __attribute__ ((packed))
#endif

struct CSphereTerrainInfo : public CUOTerrainTypeRec2
{
    CSphereTerrainInfo( TERRAIN_TYPE id );
};

// Turn off structure packing.
#if defined _WIN32 && (!__MINGW32__)
#pragma pack()
#else
#undef PACK_NEEDED
#endif


#endif //_INC_UOFILES_CUOTERRAININFO_H
