/**
* @file CUOHuesRec.h
*
*/

#ifndef _INC_CUOHUESREC_H
#define _INC_CUOHUESREC_H

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
* (Hues.mul)
*/
struct CUOHuesRec
{
    short m_color[34];
    char m_name[20];
    byte GetRGB( int rgb ) const;
} PACK_NEEDED;


// Turn off structure packing.
#if defined(_WIN32) && defined(_MSC_VER)
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOHUESREC_H
