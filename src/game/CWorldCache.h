/**
* @file CWorldCache.h
*/

#ifndef _INC_CWORLDCACHE_H
#define _INC_CWORLDCACHE_H

#include "../common/CServerMap.h"

class CWorldCache
{
	friend class CWorld;
	friend class CWorldMap;

	int64	_iTimeLastMapBlockCacheCheck;
	
	using MapBlockCacheCont = std::unique_ptr<CServerMapBlock>;     // Info about a single map block
	using MapBlockCache = std::unique_ptr<MapBlockCacheCont[]>;     // An element per each map block
	MapBlockCache _mapBlocks[MAP_SUPPORTED_QTY];                    // An element per each map

public:
	static const char* m_sClassName;
	CWorldCache();
	~CWorldCache() = default;

	void Init();

	void CheckMapBlockCache(int64 iCurTime, int64 iCacheTime);
};

#endif // _INC_CWORLDCACHE_H
