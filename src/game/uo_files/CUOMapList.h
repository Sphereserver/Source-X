/**
* @file CUOMapList.h
*
*/

#ifndef _INC_CUOMAPLIST_H
#define _INC_CUOMAPLIST_H


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
    bool m_maps[MAP_SUPPORTED_QTY];	    // list of supported maps
    int m_mapnum[MAP_SUPPORTED_QTY];    // real map number (0 for 0 and 1, 2 for 2, and so on) - file name
    int m_mapid[MAP_SUPPORTED_QTY];	    // map id used by the client
    int m_sizex[MAP_SUPPORTED_QTY];
    int m_sizey[MAP_SUPPORTED_QTY];

    int m_sectorsize[MAP_SUPPORTED_QTY];

    bool m_mapsinitalized[MAP_SUPPORTED_QTY];

    CServerMapDiffCollection * m_pMapDiffCollection;

public:
    /** @name Constructors, Destructor, Asign operator:
     */
    ///@{
    CUOMapList();

private:
    CUOMapList(const CUOMapList& copy);
    CUOMapList& operator=(const CUOMapList& other);
    ///@}

public:
    /** @name Modifiers:
     */
    ///@{
    void Clear();
    void ResetMap(int map, int maxx, int maxy, int sectorsize, int realmapnum, int mapid);
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
    inline int GetMapSizeX(int map) const noexcept;
    inline int GetMapSizeY(int map) const noexcept;
    int GetMapCenterX(int map) const;
    int GetMapCenterY(int map) const;

    int GetMapFileNum(int map) const;
    int GetMapID(int map) const;
    
    ///@}
} g_MapList;


// Inline methods definition

int CUOMapList::GetMapSizeX(int map) const noexcept
{
    // Used by CPointBase::IsValidXY(), which is called a LOT
    //ASSERT(IsMapSupported(map));
    //ASSERT(m_sizex[map] != -1);
    return m_sizex[map];
}

int CUOMapList::GetMapSizeY(int map) const noexcept
{
    // Used by CPointBase::IsValidXY(), which is called a LOT
    //ASSERT(IsMapSupported(map));
    //ASSERT(m_sizey[map] != -1);
    return m_sizey[map];
}

#endif //_INC_CUOMAPLIST_H
