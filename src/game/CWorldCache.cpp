#include "../sphere/threads.h"
#include "../sphere/ProfileTask.h"
#include "uo_files/CUOMapList.h"
#include "CWorldCache.h"

CWorldCache::CWorldCache()
{
	_iTimeLastMapBlockCacheCheck = 0;

	for (int i = 0; i < MAP_SUPPORTED_QTY; ++i)
		_mapBlocks[i].reset();
}


static int _GetMapBlocksCount(int iMap)
{
	const int iXBlocks = g_MapList.GetMapSizeX(iMap) / UO_BLOCK_SIZE;
	const int iYBlocks = g_MapList.GetMapSizeY(iMap) / UO_BLOCK_SIZE;
	return (iXBlocks * iYBlocks);
}

void CWorldCache::Init()
{
	for (int i = 0; i < MAP_SUPPORTED_QTY; ++i)
	{
		if (!g_MapList.IsInitialized(i))
			continue;

		const int iBlocks = _GetMapBlocksCount(i);
		_mapBlocks[i] = std::make_unique<MapBlockCacheCont[]>(iBlocks);
	}
}

void CWorldCache::CheckMapBlockCache(int64 iCurTime, int64 iCacheTime)
{
	ADDTOCALLSTACK("CWorldCache::CheckMapBlockCache");
	// Clean out the sectors map cache if it has not been used recently.
	// iTime == 0 = delete all.

	const ProfileTask overheadTask(PROFILE_MAP);
	for (int i = 0; i < MAP_SUPPORTED_QTY; ++i)
	{
		MapBlockCache& cache = _mapBlocks[i];
		if (!cache)
			continue;

		const int iBlocks = _GetMapBlocksCount(i);
		for (int j = 0; j < iBlocks; ++j)
		{
			MapBlockCacheCont& block = cache[j];
			if (!block)
				continue;

			if (iCacheTime <= 0 || (block->m_CacheTime.GetCacheAge() >= iCacheTime))
				block.reset();
		}
	}

	_iTimeLastMapBlockCacheCheck = iCurTime + iCacheTime;
}
