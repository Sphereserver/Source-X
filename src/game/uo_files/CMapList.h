/**
* @file CMapList.h
*
*/

#pragma once
#ifndef _INC_CMAPLIST_H
#define _INC_CMAPLIST_H

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


class CMapDiffCollection;

extern class CMapList
{
public:
    int m_sizex[256];
    int m_sizey[256];
    int m_sectorsize[256];
    bool m_maps[256];  ///< list of supported maps
    int m_mapnum[256];  ///< real map number (0 for 0 and 1, 2 for 2, and so on) - file name
    int m_mapid[256];  ///< map id used by the client
    CMapDiffCollection * m_pMapDiffCollection;

protected:
    bool m_mapsinitalized[256];


public:
    /** @name Constructors, Destructor, Asign operator:
     */
    ///@{
    CMapList();
private:
    CMapList(const CMapList& copy);
    CMapList& operator=(const CMapList& other);
    ///@}
public:
    /** @name Modifiers:
     */
    ///@{
    void Init();
    bool Load(int map, char *args);
    bool Load(int map, int maxx, int maxy, int sectorsize, int realmapnum, int mapid);
    ///@}
    /** @name Operations:
     */
    ///@{
protected:
    bool DetectMapSize(int map);
public:
    bool IsMapSupported(int map);
    int GetCenterX(int map);
    int GetCenterY(int map);
    int GetSectorCols(int map);
    int GetSectorQty(int map);
    int GetX(int map);
    int GetSectorRows(int map);
    int GetSectorSize(int map);
    int GetY(int map);
    bool IsInitialized(int map);
    ///@}
} g_MapList;


// Turn off structure packing.
#if defined _WIN32 && (!__MINGW32__)
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CMAPLIST_H
