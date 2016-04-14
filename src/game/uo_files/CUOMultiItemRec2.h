#pragma once
#ifndef _INC_UOFILES_CUOMULTIITEMREC2_H
#define _INC_UOFILES_CUOMULTIITEMREC2_H

#include "../../common/common.h"
#include "enums.h"

// All these structures must be byte packed.
#if defined _WINDOWS && (!__MINGW32__)
// Microsoft dependant pragma
#pragma pack(1)
#define PACK_NEEDED
#else
// GCC based compiler you can add:
#define PACK_NEEDED __attribute__ ((packed))
#endif

struct CUOMultiItemRec2 // (Multi.mul, High Seas+)
{
    // Describe multi's like houses and boats. One single tile.
    // From Multi.Idx and Multi.mul files
    word  m_wTileID;	// ITEMID_TYPE = Index to tile CUOItemTypeRec/CUOItemTypeRec2
    short m_dx;	// signed delta.
    short m_dy;
    short m_dz;
    dword m_visible;	// 0 or 1 (non-visible items are things like doors and signs)
    dword m_unknown;	// unknown data

    ITEMID_TYPE GetDispID() const;
} PACK_NEEDED;

// Turn off structure packing.
#if defined _WINDOWS && (!__MINGW32__)
#pragma pack()
#else
#undef PACK_NEEDED
#endif

#endif //_INC_UOFILES_CUOMULTIITEMREC2_H
