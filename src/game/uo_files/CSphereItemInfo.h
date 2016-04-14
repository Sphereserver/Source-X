#pragma once
#ifndef _INC_UOFILES_CSPHEREITEMINFO_H
#define _INC_UOFILES_CSPHEREITEMINFO_H

#include "../../common/common.h"
#include "CUOItemTypeRec2.h"
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


struct CSphereItemInfo : public CUOItemTypeRec2
{
    explicit CSphereItemInfo( ITEMID_TYPE id );
    static ITEMID_TYPE GetMaxTileDataItem();
};


// Turn off structure packing.
#if defined _WINDOWS && (!__MINGW32__)
#pragma pack()
#else
#undef PACK_NEEDED
#endif

#endif //_INC_UOFILES_CSPHEREITEMINFO_H
