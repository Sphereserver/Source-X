#pragma once
#ifndef _INC_UOFILES_CUOMULTIITEMREC_H
#define _INC_UOFILES_CUOMULTIITEMREC_H

#include "../../common/common.h"
#include "enums.h"

// All these structures must be byte packed.
#if defined _WIN32 && (!__MINGW32__)
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
    word  m_wTileID;  ///< ITEMID_TYPE = Index to tile CUOItemTypeRec/CUOItemTypeRec2
    short m_dx;	 ///< signed delta.
    short m_dy;
    short m_dz;
    dword m_visible;  ///< 0 or 1 (non-visible items are things like doors and signs)
    ITEMID_TYPE GetDispID() const;
} PACK_NEEDED;

// Turn off structure packing.
#if defined _WIN32 && (!__MINGW32__)
#pragma pack()
#else
#undef PACK_NEEDED
#endif

#endif //_INC_UOFILES_CUOMULTIITEMREC_H
