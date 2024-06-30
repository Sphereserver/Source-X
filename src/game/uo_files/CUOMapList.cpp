#include "../../common/CLog.h"
#include "../../common/CServerMap.h"
#include "../../common/CUOInstall.h"
#include "../CServerConfig.h"
#include "CUOMapList.h"


// MapGeoDataHolder

void CUOMapList::MapGeoDataHolder::clear() noexcept
{
    const CUOMapList::MapGeoData invalid_data = MapGeoData::invalid();
    for (auto& map : maps)
    {
        memcpy(&map, reinterpret_cast<const void*>(&invalid_data), sizeof(CUOMapList::MapGeoData));
    }
}


// CUOMapList:: Constructors, Destructor, Asign operator.

CUOMapList::CUOMapList() noexcept
{
    Clear();
}

// CUOMapList:: Modifiers.

void CUOMapList::Clear() noexcept
{
    m_mapGeoData.clear();
    m_pMapDiffCollection = nullptr;

    constexpr int iMaxMapFileIdx = 6;
    for (int i = 0; i < iMaxMapFileIdx; ++i)
        m_mapGeoData.maps[i] = MapGeoData::invalid();
        //ResetMap(i, -1, -1, -1, i, i);
}

void CUOMapList::ResetMap(int map, int maxx, int maxy, int sectorsize, int realmapnum, int mapid)
{
    MapGeoData& map_data = m_mapGeoData.maps[map];
    map_data.sizex = maxx;
    map_data.sizey = maxy;
    map_data.sectorsize = sectorsize;
    map_data.num = realmapnum;
    map_data.id = mapid;
}

void CUOMapList::Init()
{
    if (g_Cfg.m_fUseMapDiffs && !m_pMapDiffCollection)
    {
        m_pMapDiffCollection = new CServerMapDiffCollection();
        m_pMapDiffCollection->Init();
    }

    for ( int i = 0; i < MAP_SUPPORTED_QTY; ++i )
    {
        MapGeoData& map_data = m_mapGeoData.maps[i];
        if ( map_data.enabled )	// map marked as available. check whatever it's possible
        {
            //	check coordinates first
            if ( map_data.num == -1 )
                map_data.enabled = false;
            else if ( map_data.sizex <= 0 || map_data.sizey <= 0 || map_data.sectorsize <= 0 )
                map_data.enabled = DetectMapSize(i);
        }
    }
}

bool CUOMapList::Load(int map, char *args)
{
    if (( map < 0 ) || ( map >= MAP_SUPPORTED_QTY))
    {
        g_Log.EventError("Invalid map #%d couldn't be initialized.\n", map);
        return false;
    }

    MapGeoData& map_data = m_mapGeoData.maps[map];
    if ( false == map_data.initialized )	// disable double intialization
    {
        tchar * ppCmd[5];	// maxx,maxy,sectorsize,mapnum[like 0 for map0/statics0/staidx0],mapid
        size_t iCount = Str_ParseCmds(args, ppCmd, ARRAY_COUNT(ppCmd), ",");

        if ( iCount <= 0 )	// simple MAPX= same as disabling the map
        {
            map_data.enabled = false;
        }
        else
        {
            int	maxx = 0, maxy = 0, sectorsize = 0, realmapnum = 0, mapid = -1;
            if ( ppCmd[0] ) maxx = atoi(ppCmd[0]);
            if ( ppCmd[1] ) maxy = atoi(ppCmd[1]);
            if ( ppCmd[2] ) sectorsize = atoi(ppCmd[2]);
            if ( ppCmd[3] ) realmapnum = atoi(ppCmd[3]);
            if ( ppCmd[4] ) mapid = atoi(ppCmd[4]);

            // zero settings of anything except the real map num means
            if ( maxx )					// skipping the argument
            {
                if (( maxx < 8 ) || ( maxx % 8 ))
                {
                    g_Log.EventError("MAP%d: X coord must be multiple of 8 (%d is invalid, %d is still effective).\n",
                        map, maxx, map_data.sizex);
                }
                else
                    map_data.sizex = maxx;
            }
            if ( maxy )
            {
                if (( maxy < 8 ) || ( maxy % 8 ))
                {
                    g_Log.EventError("MAP%d: Y coord must be multiple of 8 (%d is invalid, %d is still effective).\n",
                        map, maxy, map_data.sizey);
                }
                else
                    map_data.sizey = maxy;
            }
            if ( sectorsize > 0 )
                map_data.sectorsize = sectorsize;
            if ( realmapnum >= 0 )
                map_data.num = realmapnum;
            if ( mapid >= 0 )
                map_data.id = mapid;
            else
                map_data.id = map;
        }

        map_data.initialized = true;
    }
    return true;
}


// CUOMapList:: Operations.

bool CUOMapList::DetectMapSize(int map) // it sets also the default sector size, if not specified in the ini (<= 0)
{
    if (m_mapGeoData.maps[map].initialized == false )
        return false;

    const int index = m_mapGeoData.maps[map].num;
    if ( index < 0 )
        return false;

    if (g_Install.m_Maps[index].IsFileOpen() == false)
        return false;

    //
    //	#0 - map0.mul			(felucca, 6144x4096 or 7168x4096, 77070336 or 89915392 bytes)
    //	#1 - map0 or map1.mul	(trammel, 6144x4096 or 7168x4096, 77070336 or 89915392 bytes)
    //	#2 - map2.mul			(ilshenar, 2304x1600, 11289600 bytes)
    //	#3 - map3.mul			(malas, 2560x2048, 16056320 bytes)
    //	#4 - map4.mul			(tokuno islands, 1448x1448, 6421156 bytes)
    //	#5 - map5.mul			(ter mur, 1280x4096, 16056320 bytes)
    //

#define SECTORSIZE_DEFAULT  64  /* 8 x 8 */
    MapGeoData& map_data = m_mapGeoData.maps[map];
    switch (index)
    {
        case 0: // map0.mul
        case 1: // map1.mul
            if (g_Install.m_Maps[index].GetLength() == 89915392 ||			// ML-sized
                !strcmpi(g_Install.m_Maps[index].GetFileExt(), ".uop"))		// UOP are all ML-sized
                //g_Install.m_Maps[index].GetLength() == 89923808)			// (UOP packed)
            {
                // ML+ map
                if (map_data.sizex <= 0)
                    map_data.sizex = 7168;
                if (map_data.sizey <= 0)
                    map_data.sizey = 4096;
            }
            else
            {
                // Pre ML map
                if (map_data.sizex <= 0)
                    map_data.sizex = 6144;
                if (map_data.sizey <= 0)
                    map_data.sizey = 4096;
            }

            if (map_data.sectorsize <= 0)
                map_data.sectorsize = SECTORSIZE_DEFAULT;
            break;

        case 2: // map2.mul
            if (map_data.sizex <= 0)
                map_data.sizex = 2304;
            if (map_data.sizey <= 0)
                map_data.sizey = 1600;
            if (map_data.sectorsize <= 0)
                map_data.sectorsize = SECTORSIZE_DEFAULT;
            break;

        case 3: // map3.mul
            if (map_data.sizex <= 0)
                map_data.sizex = 2560;
            if (map_data.sizey <= 0)
                map_data.sizey = 2048;
            if (map_data.sectorsize <= 0)
                map_data.sectorsize = SECTORSIZE_DEFAULT;
            break;

        case 4: // map4.mul
            if (map_data.sizex <= 0)
                map_data.sizex = 1448;
            if (map_data.sizey <= 0)
                map_data.sizey = 1448;
            if (map_data.sectorsize <= 0)
                map_data.sectorsize = SECTORSIZE_DEFAULT;
            break;

        case 5: // map5.mul
            if (map_data.sizex <= 0)
                map_data.sizex = 1280;
            if (map_data.sizey <= 0)
                map_data.sizey = 4096;
            if (map_data.sectorsize <= 0)
                map_data.sectorsize = SECTORSIZE_DEFAULT;
            break;

        default:
            DEBUG_ERR(("Unknown map index %d with file size of %d bytes. Please specify the correct size manually.\n",
                index, g_Install.m_Maps[index].GetLength()));
            break;
    }

    return (map_data.sizex > 0 && map_data.sizey > 0 && map_data.sectorsize > 0);
}

bool CUOMapList::IsMapSupported(int map) const noexcept
{
    if (( map < 0 ) || ( map >= MAP_SUPPORTED_QTY))
        return false;
    return m_mapGeoData.maps[map].enabled;
}

bool CUOMapList::IsInitialized(int map) const
{
    ASSERT(IsMapSupported(map));
    return m_mapGeoData.maps[map].initialized;
}

int CUOMapList::GetSectorSize(int map) const
{
    ASSERT(IsMapSupported(map));
    ASSERT(m_mapGeoData.maps[map].sectorsize > 0);
    return m_mapGeoData.maps[map].sectorsize;
}

int CUOMapList::CalcSectorQty(int map) const
{
    return (CalcSectorCols(map) * CalcSectorRows(map));
}

int CUOMapList::CalcSectorCols(int map) const
{
    ASSERT(IsMapSupported(map));
    const int a = m_mapGeoData.maps[map].sizex;
    const int b = GetSectorSize(map);
    // ceil division: some maps may not have x or y size perfectly dividable by 64 (default sector size),
    //  still we need to make room even for sectors with a smaller number of usable tiles
    return ((a / b) + ((a % b) != 0));
}

int CUOMapList::CalcSectorRows(int map) const
{
    ASSERT(IsMapSupported(map));
    const int a = m_mapGeoData.maps[map].sizey;
    const int b = GetSectorSize(map);
    // ceil division: some maps may not have x or y size perfectly dividable by 64 (default sector size),
    //  still we need to make room even for sectors with a smaller number of usable tiles
    return ((a / b) + ((a % b) != 0));
}

int CUOMapList::GetMapCenterX(int map) const
{
    ASSERT(IsMapSupported(map));
    ASSERT(m_mapGeoData.maps[map].sizex != -1);
    return (m_mapGeoData.maps[map].sizex / 2);
}

int CUOMapList::GetMapCenterY(int map) const
{
    ASSERT(IsMapSupported(map));
    ASSERT(m_mapGeoData.maps[map].sizey != -1);
    return (m_mapGeoData.maps[map].sizey / 2);
}

int CUOMapList::GetMapFileNum(int map) const
{
    ASSERT(IsMapSupported(map));
    ASSERT(m_mapGeoData.maps[map].num != -1);
    return m_mapGeoData.maps[map].num;
}

int CUOMapList::GetMapID(int map) const
{
    ASSERT(IsMapSupported(map));
    ASSERT(m_mapGeoData.maps[map].id != -1);
    return m_mapGeoData.maps[map].id;
}
