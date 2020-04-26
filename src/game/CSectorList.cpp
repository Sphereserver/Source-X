#include "../common/sphere_library/sstringobjs.h"
#include "../common/assertion.h"
#include "../common/CLog.h"
#include "CWorld.h"
#include "CSectorList.h"

CSectorList::CSectorList() : _SectorData{}
{
	_fInitialized = false;
}

CSectorList::~CSectorList()
{
	Close();
}

const CSectorList* CSectorList::Get() noexcept // static
{
	return &g_World._Sectors;
}

void CSectorList::Init()
{
	ADDTOCALLSTACK("CSectorList::Init");

	if ( _fInitialized )	//	disable changes on-a-fly
		return;

    // Initialize map planes
	g_MapList.Init();

	// Initialize sector data for every map plane
	TemporaryString ts;
	TemporaryString tsConcat;
	int iSectorIndex = 0;
	for (int iMap = 0; iMap < MAP_SUPPORTED_QTY; ++iMap)
	{
		MapSectorsData& sd = _SectorData[iMap];
		sd._iSectorSize = sd._iSectorColumns = sd._iSectorRows = sd._iSectorQty = 0;
		sd._pSectors.release();

		if (!g_MapList.IsMapSupported(iMap))
			continue;

		const int iSectorQty = g_MapList.CalcSectorQty(iMap);
		const int iMaxX = g_MapList.CalcSectorCols(iMap);
		const int iMaxY = g_MapList.CalcSectorRows(iMap);

		snprintf(ts, ts.realLength(), " map%d=%d", iMap, iSectorQty);
		Str_ConcatLimitNull(tsConcat, ts, tsConcat.realLength());

		sd._pSectors		= std::make_unique<CSector[]>(iSectorQty);
		sd._iSectorSize		= g_MapList.GetSectorSize(iMap);
		sd._iSectorQty		= iSectorQty;
		sd._iSectorColumns	= iMaxX;
		sd._iSectorRows		= iMaxY;

		short iSectorX = 0, iSectorY = 0;
		for (; iSectorIndex < iSectorQty; ++iSectorIndex)
		{
			if (iSectorX >= iMaxX)
			{
				iSectorX = 0;
				++iSectorY;
			}
			CSector* pSector = &(sd._pSectors[iSectorIndex]);
			ASSERT(pSector);
			pSector->Init(iSectorIndex, (uchar)iMap, iSectorX, iSectorY);
		}
	}

	for (MapSectorsData& sd : _SectorData)
	{
		if (!sd._pSectors)
			continue;

		for (int iSector = 0; iSector < sd._iSectorQty; ++iSector)
		{
			sd._pSectors[iSector].SetAdjacentSectors();
		}
	}
	_fInitialized = true;

    g_Log.Event(LOGM_INIT, "Allocated map sectors:%s\n", static_cast<lpctstr>(tsConcat));
}

void CSectorList::Close()
{
	ADDTOCALLSTACK("CSectorList::Close");

	for (MapSectorsData& sd : _SectorData)
	{
		if (!sd._pSectors)
			continue;

		// Delete everything in sector
		for (int iSector = 0; iSector < sd._iSectorQty; ++iSector)
		{
			sd._pSectors[iSector].Close();
		}

		// Free memory allocated by sectors
		//  Do this in two separate moments because destructors of items may access server sectors
		sd._pSectors.release();
	}
	_fInitialized = false;
}


#define IsMapInvalid(map)	((map < 0) || (map > MAP_SUPPORTED_QTY))

int CSectorList::GetSectorSize(int map) const noexcept
{
	return (IsMapInvalid(map) ? -1 : _SectorData[map]._iSectorSize);
}

int CSectorList::GetSectorQty(int map) const noexcept
{
	return (IsMapInvalid(map) ? -1 : _SectorData[map]._iSectorQty);
}

int CSectorList::GetSectorCols(int map) const noexcept
{
	return (IsMapInvalid(map) ? -1 : _SectorData[map]._iSectorColumns);
}

int CSectorList::GetSectorRows(int map) const noexcept
{
	return (IsMapInvalid(map) ? -1 : _SectorData[map]._iSectorRows);
}

CSector* CSectorList::GetSector(int map, int index) const noexcept
{
	if (IsMapInvalid(map) || !_SectorData[map]._pSectors)
		return nullptr;
	const MapSectorsData& sd = _SectorData[map];
	return (index < sd._iSectorQty) ? &(sd._pSectors[index]) : nullptr;
}

CSector* CSectorList::GetSector(int map, short x, short y) const noexcept
{
	if (IsMapInvalid(map) || !_SectorData[map]._pSectors)
		return nullptr;

	const MapSectorsData& sd = _SectorData[map];
	const int xSect = (x / sd._iSectorSize), ySect = (y / sd._iSectorSize);
	if ((xSect > sd._iSectorColumns) || (ySect > sd._iSectorRows))
		return nullptr;

	const int index = ((ySect * sd._iSectorColumns) + xSect);
	return (index < sd._iSectorQty) ? &(sd._pSectors[index]) : nullptr;
}

#undef IsMapInvalid


int CSectorList::GetSectorAbsoluteQty() const noexcept
{
	int iCount = 0;
	for (const MapSectorsData& sd : _SectorData)
	{
		if (!sd._pSectors)
			continue;

		iCount += sd._iSectorQty;
	}
	return iCount;
}

CSector* CSectorList::GetSectorAbsolute(int index) noexcept
{
	for (int base = 0, m = 0; m < MAP_SUPPORTED_QTY; ++m)
	{
		const MapSectorsData& sd = _SectorData[m];
		if (!sd._pSectors)
			continue;

		if ((base + sd._iSectorQty) > index)
		{
			const int iSummation = (index - base);
			return &(sd._pSectors[iSummation]);
		}

		base += sd._iSectorQty;
	}
	return nullptr;
}
