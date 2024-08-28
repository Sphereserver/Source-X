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

enum MOBTYPES_ENTITY_TYPE : ushort
{
    MOBTE_MONSTER,
    MOBTE_SEA_MONSTER,
    MOBTE_ANIMAL,
    MOBTE_HUMAN,
    MOBTE_EQUIPMENT,
    MOBTE_QTY
};

struct CUOMobTypesEntry
{
    MOBTYPES_ENTITY_TYPE m_uiType;
    uint m_uiFlags;
};


class CUOMobTypes
{
    std::vector<CUOMobTypesEntry> _vMobTypesEntries;

public:
    void Load();

    bool IsLoaded() const noexcept {
        return !_vMobTypesEntries.empty();
    }

    const CUOMobTypesEntry* GetEntry(uint id) const;
};





#endif // _INC_CUOMOBTYPES_H
