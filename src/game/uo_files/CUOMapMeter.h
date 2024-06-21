/**
* @file CUOMapMeter.h
*
*/

#ifndef _INC_CUOMAPMETER_H
#define _INC_CUOMAPMETER_H

#include "../../common/common.h"

// All these structures must be byte packed.
#if defined(_WIN32) && defined(_MSC_VER)
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
    char m_z;

    static bool IsTerrainNull( word wTerrainIndex );
} PACK_NEEDED;


// Turn off structure packing.
#if defined(_WIN32) && defined(_MSC_VER)
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOMAPMETER_H
