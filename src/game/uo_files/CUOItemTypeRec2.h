#pragma once
#ifndef _INC_UOFILES_CUOITEMTYPEREC2_H
#define _INC_UOFILES_CUOITEMTYPEREC2_H

#include "../../common/common.h"

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
* size = 41 (tiledata.mul, High Seas+)
*/
struct CUOItemTypeRec2
{
    dword m_flags;
    dword m_dwUnk5;		///< ? new in HS
    byte m_weight;		///< 255 = unmovable.
    byte m_layer;		///< LAYER_TYPE for UFLAG1_EQUIP, UFLAG3_EQUIP2 or light index for UFLAG3_LIGHT
    dword m_dwUnk11;	///< ? qty in the case of UFLAG2_STACKABLE, Spell icons use this as well.
    dword m_dwAnim;		///< equipable items animation index. (50000 = male offset, 60000=female) Gump base as well
    word m_wUnk19;		///< ?
    byte m_height;		///< z height but may not be blocking. ex.UFLAG2_WINDOW
    char m_name[20];	///< sometimes legit not to have a name
} PACK_NEEDED;

// Turn off structure packing.
#if defined _WIN32 && (!__MINGW32__)
#pragma pack()
#else
#undef PACK_NEEDED
#endif

#endif //_INC_UOFILES_CUOITEMTYPEREC2_H
