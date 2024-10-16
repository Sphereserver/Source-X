/**
* @file CUOStaticItemRec.h
*
*/

#ifndef _INC_CUOSTATICITEMREC_H
#define _INC_CUOSTATICITEMREC_H

#include "../../common/common.h"
#include "uofiles_enums_itemid.h"

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
* 7 byte block = static items on the map (statics0.mul)
*/
struct CUOStaticItemRec
{
    word m_wTileID;  // ITEMID_TYPE = Index to tile CUOItemTypeRec/CUOItemTypeRec_HS
    byte m_x;  // x <= 7 = offset from block.
    byte m_y;  // y <= 7
    char m_z;  //
    word m_wHue;  // HUE_TYPE modifier for the item

    // For internal caching purposes only. overload this.
    // LOBYTE(m_wColor) = Blocking flags for this item. (CAN_I_BLOCK)
    // HIBYTE(m_wColor) = Height of this object.
    ITEMID_TYPE GetDispID() const;
} PACK_NEEDED;


// Turn off structure packing.
#ifdef MSVC_COMPILER
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOSTATICITEMREC_H
