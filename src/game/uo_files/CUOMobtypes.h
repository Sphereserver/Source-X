/**
* @file CUOMobtypes.h
*
*/

#ifndef _INC_CUOMOBTYPES_H
#define _INC_CUOMOBTYPES_H

#include <vector>
#include "../../common/common.h"

/**
* mobtypes.txt
*/
struct CUOMobTypesType
{
    ushort m_type; // 0 = MONSTER, 1 = SEA_MONSTER, 2 = ANIMAL, 3 = HUMAN, 4 = EQUIPMENT
    dword m_flags;

};

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
