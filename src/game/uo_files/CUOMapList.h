/**
* @file CUOMapList.h
*
*/

#ifndef _INC_CUOMAPLIST_H
#define _INC_CUOMAPLIST_H

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


class CServerMapDiffCollection;

extern class CUOMapList
{
public:
    int m_sizex[256];
    int m_sizey[256];
    int m_sectorsize[256];
    bool m_maps[256];		// list of supported maps
    int m_mapnum[256];		// real map number (0 for 0 and 1, 2 for 2, and so on) - file name
    int m_mapid[256];		// map id used by the client
    CServerMapDiffCollection * m_pMapDiffCollection;

protected:
    bool m_mapsinitalized[256];


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
    bool IsMapSupported(int map) const;
    int GetCenterX(int map) const;
    int GetCenterY(int map) const;
    int GetSectorCols(int map) const;
    int GetSectorQty(int map) const;
    int GetX(int map) const;
    int GetSectorRows(int map) const;
    int GetSectorSize(int map) const;
    int GetY(int map) const;
    bool IsInitialized(int map) const;
    ///@}
} g_MapList;


// Turn off structure packing.
#if defined(_WIN32) && defined(_MSC_VER)
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOMAPLIST_H
