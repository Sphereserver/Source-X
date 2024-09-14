/**
* @file CUOMultiItemRec.h
*
*/

#ifndef _INC_CUOMULTIITEMREC_H
#define _INC_CUOMULTIITEMREC_H

#include "../../common/common.h"
#include "uofiles_enums_itemid.h"

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
* (Multi.mul)
* Describe multi's like houses and boats. One single tile.
* From Multi.Idx and Multi.mul files.
*/
struct CUOMultiItemRec
{
    word  m_wTileID;	// ITEMID_TYPE = Index to tile CUOItemTypeRec/CUOItemTypeRec_HS
    short m_dx;			// signed delta.
    short m_dy;
    short m_dz;
    dword m_visible;	// 0 or 1 (non-visible items are things like doors and signs)
    ITEMID_TYPE GetDispID() const;
} PACK_NEEDED;


struct CUOMultiItemRec_HS // (Multi.mul, High Seas+)
{
	word  m_wTileID;	// ITEMID_TYPE = Index to tile CUOItemTypeRec/CUOItemTypeRec_HS
	short m_dx;			// signed delta.
	short m_dy;
	short m_dz;
	dword m_visible;	// 0 or 1 (non-visible items are things like doors and signs)
	dword m_shipAccess;	// 0 or 1 (rope item used to enter/exit galleons)

	ITEMID_TYPE GetDispID() const;
} PACK_NEEDED;


// Turn off structure packing.
#if defined(_WIN32) && defined(_MSC_VER)
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOMULTIITEMREC_H
