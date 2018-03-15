/**
* @file CUOItemTypeRec.h
* @brief Tiledata entry for an item.
*/

#ifndef _INC_CUOITEMTYPEREC_H
#define _INC_CUOITEMTYPEREC_H

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
* size = 37 (tiledata.mul)
* Second half of tiledata.mul file is for item tiles (ITEMID_TYPE).
* if all entries are 0 then this is unused and undisplayable.
*/
struct CUOItemTypeRec
{
    dword m_flags;
    byte m_weight;		// 255 = unmovable.
    byte m_layer;		// LAYER_TYPE for UFLAG1_EQUIP, UFLAG3_EQUIP2 or light index for UFLAG3_LIGHT
    dword m_dwUnk6;		// ? qty in the case of UFLAG2_STACKABLE, Spell icons use this as well.
    dword m_dwAnim;		// equipable items animation index. (50000 = male offset, 60000=female) Gump base as well
    word m_wUnk14;		// ?
    byte m_height;		// z height but may not be blocking. ex.UFLAG2_WINDOW
    char m_name[20];	// sometimes legit not to have a name
} PACK_NEEDED;


/**
* size = 41 (tiledata.mul, High Seas+)
*/
struct CUOItemTypeRec_HS
{
	dword m_flags;
	dword m_dwUnk5;		// ? new in HS
	byte m_weight;		// 255 = unmovable.
	byte m_layer;		// LAYER_TYPE for UFLAG1_EQUIP, UFLAG3_EQUIP2 or light index for UFLAG3_LIGHT
	dword m_dwUnk11;	// ? qty in the case of UFLAG2_STACKABLE, Spell icons use this as well.
	dword m_dwAnim;		// equipable items animation index. (50000 = male offset, 60000=female) Gump base as well
	word m_wUnk19;		// ?
	byte m_height;		// z height but may not be blocking. ex.UFLAG2_WINDOW
	char m_name[20];	// sometimes legit not to have a name
} PACK_NEEDED;


// Turn off structure packing.
#if defined(_WIN32) && defined(_MSC_VER)
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOITEMTYPEREC_H
