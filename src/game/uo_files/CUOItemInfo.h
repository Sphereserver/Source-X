/**
* @file CUOItemInfo.h
*
*/

#pragma once
#ifndef _INC_CUOITEMINFO_H
#define _INC_CUOITEMINFO_H

#include "../../common/common.h"
#include "CUOItemTypeRec.h"
#include "uofiles_enums_itemid.h"

// All these structures must be byte packed.
#if defined _WIN32 && (!__MINGW32__)
	// Microsoft dependant pragma
	#pragma pack(1)
	#define PACK_NEEDED
#else
	// GCC based compiler you can add:
	#define PACK_NEEDED __attribute__ ((packed))
#endif


struct CUOItemInfo : public CUOItemTypeRec_HS
{
    explicit CUOItemInfo( ITEMID_TYPE id );
    static ITEMID_TYPE GetMaxTileDataItem();
};


// Turn off structure packing.
#if defined _WIN32 && (!__MINGW32__)
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOITEMINFO_H
