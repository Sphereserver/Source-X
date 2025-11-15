#include "../common/sphere_library/sstringobjs.h"
#include "../common/CLog.h"
#include "CWorld.h"
#include "CSectorList.h"

CSectorList::CSectorList() :
    _SectorData{}, _fInitialized(false)
{
}

CSectorList::~CSectorList()
{
	Close(true);
}

const CSectorList& CSectorList::Get() noexcept // static
{
    return g_World._Sectors;
}

void CSectorList::Init()
{
	ADDTOCALLSTACK("CSectorList::Init");

	if ( _fInitialized )	//	disable changes on-a-fly
		return;

	// Initialize sector data for every map plane    
    TemporaryString ts;
	TemporaryString tsConcat;

	for (int iMap = 0; iMap < MAP_SUPPORTED_QTY; ++iMap)
	{
		MapSectorsData& sd = _SectorData[iMap];
		sd.iSectorSize = sd.iSectorColumns = sd.iSectorRows = sd.iSectorQty = 0;
		sd._pSectors.reset();

		if (!g_MapList.IsMapSupported(iMap))
			continue;

        const int iSectorQty= g_MapList.CalcSectorQty(iMap);
        const int iMaxX     = g_MapList.CalcSectorCols(iMap);
        const int iMaxY     = g_MapList.CalcSectorRows(iMap);

		snprintf(ts.buffer(), ts.capacity(), " map%d=%d", iMap, iSectorQty);
		Str_ConcatLimitNull(tsConcat.buffer(), ts.buffer(), tsConcat.capacity());

		sd._pSectors		= std::make_unique<CSector[]>(iSectorQty);
        ASSERT(sd._pSectors);

		sd.iSectorSize		= g_MapList.GetSectorSize(iMap);
		sd.iSectorQty		= iSectorQty;
		sd.iSectorColumns	= iMaxX;
		sd.iSectorRows		= iMaxY;

        // Calc sector_shift (log2(iSectorSize)). We use this to do a bit shift instead of a division
        //  when we do sector calculations (that is, very often).
        uint32 sector_shift = 0;
        for (uint sz = sd.iSectorSize; sz > 1; sz >>= 1)
            ++sector_shift;
        sd.uiSectorSizeDivShift = (uint16)sector_shift;


		short iSectorX = 0, iSectorY = 0;
        for (int iSectorIndex = 0; iSectorIndex < iSectorQty; ++iSectorIndex)
		{
            // Map sectors are ordered in row-major order:
            //  consecutive elements from the same row are contiguous and the inner loop varies the column index.
            if (iSectorX >= iMaxX)
            {
                iSectorX = 0;
                ++iSectorY;
            }

			CSector* pSector = &(sd._pSectors[iSectorIndex]);
			pSector->Init(iSectorIndex, (uchar)iMap, iSectorX, iSectorY);

            ++iSectorX;
		}

	}

	for (MapSectorsData& sd : _SectorData)
	{
		if (!sd._pSectors)
			continue;

		for (int iSector = 0; iSector < sd.iSectorQty; ++iSector)
		{
			sd._pSectors[iSector].SetAdjacentSectors();
		}
	}
	_fInitialized = true;

    g_Log.Event(LOGM_INIT, "Allocated map sectors:%s\n", tsConcat.buffer());
}

void CSectorList::Close(bool fClosingWorld)
{
	ADDTOCALLSTACK("CSectorList::Close");

	for (MapSectorsData& sd : _SectorData)
	{
		if (!sd._pSectors)
			continue;

		// Delete everything in sector
		for (int iSector = 0; iSector < sd.iSectorQty; ++iSector)
		{
			sd._pSectors[iSector].Close(fClosingWorld);
		}
	}
	_fInitialized = false;
}


[[nodiscard]]
const MapSectorsData* CSectorList::GetMapSectorData(int map) const noexcept
{
    if (!g_MapList.IsMapSupported(map))
        return nullptr;

    return &_SectorData[map];
}

[[nodiscard]]
const MapSectorsData& CSectorList::GetMapSectorDataUnchecked(int map) const noexcept
{
    // We assume we HAVE checked that's a valid map and that we are not indexing the _SectorData out of bounds.
    //if (!g_MapList.IsMapSupported(map))
    //    return nullptr;

    return _SectorData[map];
}

CSector* CSectorList::GetSectorByIndexUnchecked(int map, int index) const noexcept
{
    // We call this method very often and from places we ASSUME have already checked that the map number is legit.
    //  (it's done in CWorldMap::GetSectorByIndex).

    // Micro-optimization: Skip the function call (with the call cost) and the if statement to avoid the possible cost of a branch misprediction.
    //if (!g_MapList.IsMapSupported(map) || !_SectorData[map]._pSectors)
    //	return nullptr;
    //if ((map < 0) || ((size_t)map < sizeof(_SectorData)) )
    //    return nullptr;

	const MapSectorsData& sd = _SectorData[map];
    return (index < sd.iSectorQty) ? &(sd._pSectors.get()[index]) : nullptr;
}

CSector* CSectorList::GetSectorByCoordsUnchecked(int map, short x, short y) const NOEXCEPT_NODEBUG
{
    // We call this method very often and from places we ASSUME have already checked that the map number is legit.
    //  (it's done in CPointBase::GetSector())

    // Micro-optimization: Skip the IsMapSupported function call and the if statement to avoid the possible cost of a branch misprediction.
    //if (!g_MapList.IsMapSupported(map) || !_SectorData[map]._pSectors)
    //	return nullptr;

	const MapSectorsData& sd = _SectorData[map];

    // We know that sector sizes MUST be multiple of 2, so just use bit shifts.
    //const int xSectTest = (x / sd.iSectorSize), ySectTest = (y / sd.iSectorSize);
    const int xSect = (x >> sd.uiSectorSizeDivShift), ySect = (y >> sd.uiSectorSizeDivShift);
    //ASSERT(xSectTest == xSect);
    //ASSERT(ySectTest == ySect);

/*
#ifdef _DEBUG
    // We also assume that X and Y are inside map boundaries.
	if ((xSect >= sd.iSectorColumns) || (ySect >= sd.iSectorRows))
		return nullptr;

    const int index = ((ySect * sd.iSectorColumns) + xSect);
    return (index < sd.iSectorQty) ? &(sd._pSectors[index]) : nullptr;
#else
*/
    const int index = ((ySect * sd.iSectorColumns) + xSect);
    DEBUG_ASSERT(index < sd.iSectorQty);
    return &(sd._pSectors[index]);
//#endif

}

int CSectorList::GetSectorAbsoluteQty() const noexcept
{
	int iCount = 0;
	for (const MapSectorsData& sd : _SectorData)
	{
		if (!sd._pSectors)
            continue;

		iCount += sd.iSectorQty;
	}
	return iCount;
}

CSector* CSectorList::GetSectorAbsolute(int index) noexcept
{
	for (int base = 0, m = 0; m < MAP_SUPPORTED_QTY; ++m)
	{
		const MapSectorsData& sd = _SectorData[m];
		if (!sd._pSectors)
            continue;   // WTF?

		if ((base + sd.iSectorQty) > index)
		{
			const int iSummation = (index - base);
			return &(sd._pSectors[iSummation]);
		}

		base += sd.iSectorQty;
	}
	return nullptr;
}
