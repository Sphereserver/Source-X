/**
* @file CUOMapMeter.h
*
*/

#ifndef _INC_CUOMAPMETER_H
#define _INC_CUOMAPMETER_H

#include "../../common/common.h"

// All these structures must be byte packed.
#ifdef MSVC_COMPILER
	// Microsoft dependant pragma
	#pragma pack(1)
	#define PACK_NEEDED
#else
	// GCC based compiler you can add:
	#define PACK_NEEDED __attribute__ ((packed))
#endif


/**
* 3 bytes (map0.mul)
*/
struct CUOMapMeter
{
    word m_wTerrainIndex;  // TERRAIN_TYPE index to Radarcol and CUOTerrainTypeRec/CUOTerrainTypeRec_HS
    int8 m_z;

    static bool IsTerrainNull( word wTerrainIndex );
} PACK_NEEDED;


// Turn off structure packing.
#ifdef MSVC_COMPILER
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOMAPMETER_H
