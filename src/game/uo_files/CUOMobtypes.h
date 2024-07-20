/**
* @file CUOMobtypes.h
*
*/

#ifndef _INC_CUOMOBTYPES_H
#define _INC_CUOMOBTYPES_H

#include <vector>
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
* mobtypes.txt
*/
struct CUOMobTypesType
{
    ushort m_type; // 0 = MONSTER, 1 = SEA_MONSTER, 2 = ANIMAL, 3 = HUMAN, 4 = EQUIPMENT
    dword m_flags;

} PACK_NEEDED;

// Turn off structure packing.
#if defined(_WIN32) && defined(_MSC_VER)
#pragma pack()
#else
#undef PACK_NEEDED
#endif

class CUOMobTypes
{
    std::vector<CUOMobTypesType> _mobTypesEntries;

public:
    void Load();

    inline const bool IsLoaded() {
        if (_mobTypesEntries.size() > 0)
            return true;
        else
            return false;
    }

    inline const CUOMobTypesType* GetEntry(ushort id) {
        return &(_mobTypesEntries[id]);
    }
};





#endif // _INC_CUOMOBTYPES_H
