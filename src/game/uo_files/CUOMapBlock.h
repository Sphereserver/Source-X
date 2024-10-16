/**
* @file CUOMapBlock.h
*
*/

#ifndef _INC_CUOMAPBLOCK_H
#define _INC_CUOMAPBLOCK_H

#include "CUOMapMeter.h"
#include "uofiles_macros.h"

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
* 196 byte block = 8x8 meters, (map0.mul)
*/
struct CUOMapBlock
{
    word m_wID1;  // ?
    word m_wID2;
    CUOMapMeter m_Meter[UO_BLOCK_SIZE * UO_BLOCK_SIZE];
} PACK_NEEDED;


// Turn off structure packing.
#ifdef MSVC_COMPILER
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOMAPBLOCK_H
