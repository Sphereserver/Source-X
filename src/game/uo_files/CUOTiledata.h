/**
* @file CUOTiledata.h
*
*/

#ifndef _INC_CUOTILEDATA_H
#define _INC_CUOTILEDATA_H

#include "CUOItemTypeRec.h"
#include "CUOTerrainTypeRec.h"
#include "uofiles_enums_itemid.h"
#include "uofiles_types.h"
#include <vector>

class CUOTiledata
{
    std::vector<CUOItemTypeRec_HS> _tiledataItemEntries;
    std::vector<CUOTerrainTypeRec_HS> _tiledataTerrainEntries;

public:
    static ITEMID_TYPE GetMaxTileDataItem();
    void Load();
    inline const CUOItemTypeRec_HS* GetItemEntry(ITEMID_TYPE id) const {
        return &(_tiledataItemEntries[id]);
    }
    inline const CUOTerrainTypeRec_HS* GetTerrainEntry(TERRAIN_TYPE id) const {
        return &(_tiledataTerrainEntries[id]);
    }
};





#endif // _INC_CUOTILEDATA_H
