/**
* @file CUOMapList.h
*
*/

#ifndef _INC_CUOMAPLIST_H
#define _INC_CUOMAPLIST_H

#include "../../common/common.h"

class CServerMapDiffCollection;

extern class CUOMapList
{
#define MAP_SUPPORTED_QTY 256

    friend struct CUOInstall;
    friend class  CSectorList;
    friend class  CWorld;
    friend class  CWorldMap;
    friend class  CServerMapBlock;

protected:
    struct MapGeoData
    {
        int16  iNum;        // real map number (0 for 0 and 1, 2 for 2, and so on) - file name
        int16  iId;         // map id used by the client
        uint16 uiSizeX;
        uint16 uiSizeY;
        int16  iSectorSize;
        //uint16 uiSectorDivShift;
        bool   fEnabled;       // supported map?
        bool   fInitialized;

        [[nodiscard]]
        static constexpr MapGeoData invalid() noexcept
        {
            return {
                .iNum = -1,
                .iId = -1,
                .uiSizeX = (uint16)-1,
                .uiSizeY = (uint16)-1,
                .iSectorSize = -1,
                //.uiSectorDivShift = 0,
                .fEnabled = true,
                .fInitialized = false
            };
        }
    };

    struct MapGeoDataHolder
    {
        MapGeoData maps[MAP_SUPPORTED_QTY];

        void clear() noexcept;
    }
    m_mapGeoData;

    CServerMapDiffCollection * m_pMapDiffCollection;

public:
    /** @name Constructors, Destructor, Asign operator:
     */
    ///@{
    CUOMapList() noexcept;

    CUOMapList(const CUOMapList& copy) = delete;
    CUOMapList& operator=(const CUOMapList& other) = delete;
    ///@}

public:
    /** @name Modifiers:
     */
    ///@{
    void Clear() noexcept;
    //void ResetMap(uint map, ushort maxx, ushort maxy, ushort sectorsize, ushort realmapnum, ushort mapid);
    void Init();
    bool Load(int map, char *args);
    ///@}

    /** @name Operations:
     */
    ///@{
protected:
    bool DetectMapSize(int map);
    int GetSectorSize(int map) const;

public:
    bool IsMapSupported(int map) const noexcept;
    bool IsInitialized(int map) const;
    int CalcSectorQty(int map) const;  // Use it only when initializing the map sectors! (because it's slower than the Get* method)
    int CalcSectorCols(int map) const; // Use it only when initializing the map sectors! (because it's slower than the Get* method)
    int CalcSectorRows(int map) const; // Use it only when initializing the map sectors! (because it's slower than the Get* method)
    inline uint16 GetMapSizeX(int map) const noexcept;
    inline uint16 GetMapSizeY(int map) const noexcept;
    int GetMapCenterX(int map) const;
    int GetMapCenterY(int map) const;

    int GetMapFileNum(int map) const;
    int GetMapID(int map) const;

    ///@}
} g_MapList;


// Inline methods definition

inline uint16 CUOMapList::GetMapSizeX(int map) const noexcept
{
    // Used by CPointBase::IsValidXY() and IsValidPoint(), which is called a LOT
    //ASSERT(IsMapSupported(map));
    //ASSERT(m_sizex[map] != -1);
    return m_mapGeoData.maps[map].uiSizeX;
}

inline uint16 CUOMapList::GetMapSizeY(int map) const noexcept
{
    // Used by CPointBase::IsValidXY() and IsValidPoint(), which is called a LOT
    //ASSERT(IsMapSupported(map));
    //ASSERT(m_sizey[map] != -1);
    return m_mapGeoData.maps[map].uiSizeY;
}

#endif //_INC_CUOMAPLIST_H
