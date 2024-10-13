/**
* @file CUOItemTypeRec.h
* @brief Tiledata entry for an item.
*/

#ifndef _INC_CUOITEMTYPEREC_H
#define _INC_CUOITEMTYPEREC_H

#include "../../common/common.h"

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
    word m_wAnim;		// equipable items animation index. Gump base as well (50000 = male offset, 60000=female)
    word m_wHue;        // (perhaps colored light?)
    word m_wLightIndex;
    byte m_height;		// z height but may not be blocking. ex.UFLAG2_WINDOW. (If Conatainer, this is how much the container can hold?)
    char m_name[20];	// sometimes legit not to have a name
} PACK_NEEDED;


/**
* size = 41 (tiledata.mul, High Seas+)
*/
struct CUOItemTypeRec_HS
{
	uint64 m_flags;
	byte m_weight;		// 255 = unmovable.
	byte m_layer;		// LAYER_TYPE for UFLAG1_EQUIP, UFLAG3_EQUIP2 or light index for UFLAG3_LIGHT
	dword m_dwUnk11;	// ? qty in the case of UFLAG2_STACKABLE, Spell icons use this as well.
	word m_wAnim;		// equipable items animation index. Gump base as well (50000 = male offset, 60000=female)
    word m_wHue;        // (perhaps colored light?)
    word m_wLightIndex;
	byte m_height;		// z height but may not be blocking. ex.UFLAG2_WINDOW. (If Conatainer, this is how much the container can hold?)
	char m_name[20];	// sometimes legit not to have a name
} PACK_NEEDED;


// Turn off structure packing.
#ifdef MSVC_COMPILER
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOITEMTYPEREC_H
