/**
* @file CSectorList.h
*
*/

#ifndef _INC_CSECTORLIST_H
#define _INC_CSECTORLIST_H

#include "uo_files/CUOMapList.h"
#include "CSector.h"
#include <array>


class CSectorList
{
public:
	static const char* m_sClassName;

	// Sector data
	struct MapSectorsData
	{
		std::unique_ptr<CSector[]> _pSectors;

		// Pre-calculated values, for faster retrieval
		int _iSectorSize;
		int _iSectorColumns;  // how much sectors are in a column (x) in a given map
		int _iSectorRows;     // how much sectors are in a row (y) in a given map
		int _iSectorQty;      // how much sectors are in a map
	};

	//std::array<MapSectorsData, MAP_SUPPORTED_QTY> _SectorData; // Use plain C-style vectors, to remove even the minimum overhead of std::array methods
	MapSectorsData _SectorData[MAP_SUPPORTED_QTY];
	bool _fInitialized;

public:
	CSectorList();
	~CSectorList();

	CSectorList(const CSectorList& copy) = delete;
	CSectorList& operator=(const CSectorList& other) = delete;

public:
	static const CSectorList* Get() noexcept;

	void Init();
	void Close(bool fClosingWorld);

	int GetSectorSize(int map) const noexcept;
	int GetSectorQty(int map) const noexcept;
	int GetSectorCols(int map) const noexcept;
	int GetSectorRows(int map) const noexcept;

	CSector* GetSector(int map, int index) const noexcept;	// gets sector # from one map
	CSector* GetSector(int map, short x, short y) const noexcept;

	int GetSectorAbsoluteQty() const noexcept;
	CSector* GetSectorAbsolute(int index) noexcept;
};


#endif // _INC_CSECTORLIST_H
