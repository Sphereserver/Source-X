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

void CWorldCache::Init()
{
	for (int i = 0; i < MAP_SUPPORTED_QTY; ++i)
	{
		if (!g_MapList.IsInitialized(i))
			continue;

		const int iBlocks = (g_MapList.GetX(i) * g_MapList.GetY(i)) / UO_BLOCK_SIZE;
		_mapBlocks[i] = std::make_unique<MapBlockCacheCont[]>(iBlocks);
	}
	
}

void CWorldCache::CheckMapBlockCache(int64 iCurTime, int64 iCacheTime)
{
	ADDTOCALLSTACK("CWorld::CheckMapBlockCache");
	// Clean out the sectors map cache if it has not been used recently.
	// iTime == 0 = delete all.

	const ProfileTask overheadTask(PROFILE_MAP);
	for (int i = 0; i < MAP_SUPPORTED_QTY; ++i)
	{
		MapBlockCache& cache = _mapBlocks[i];
		if (!cache)
			continue;

		const int iBlocks = (g_MapList.GetX(i) * g_MapList.GetY(i)) / UO_BLOCK_SIZE;
		for (int j = 0; j < iBlocks; ++j)
		{
			MapBlockCacheCont& block = cache[j];
			if (!block)
				continue;

			if (iCacheTime <= 0 || (block->m_CacheTime.GetCacheAge() >= iCacheTime))
				block.release();
		}
	}

	_iTimeLastMapBlockCacheCheck = iCurTime + iCacheTime;
}