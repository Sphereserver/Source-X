/**
* @file CUOTerrainInfo.h
*
*/

#ifndef _INC_CUOTERRAININFO_H
#define _INC_CUOTERRAININFO_H

#include "../../common/common.h"
#include "CUOTerrainTypeRec.h"
#include "uofiles_types.h"

// All these structures must be byte packed.
#if defined(_WIN32) && defined(_MSC_VER)
	// Microsoft dependant pragma
	#pragma pack(1)
	#define PACK_NEEDED
#else
	// GCC based compiler you can add:
	#define PACK_NEEDED __attribute__ ((packed))
#endif


struct CSphereTerrainInfo : public CUOTerrainTypeRec_HS
{
    CSphereTerrainInfo( TERRAIN_TYPE id );
};


// Turn off structure packing.
#if defined(_WIN32) && defined(_MSC_VER)
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOTERRAININFO_H
