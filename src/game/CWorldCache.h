/**
* @file CWorldCache.h
*/

#ifndef _INC_CWORLDCACHE_H
#define _INC_CWORLDCACHE_H

#include "../common/CServerMap.h"

class CWorldCache
{
	friend class CWorld;

	int64	_iTimeLastMapBlockCacheCheck;
	
	using MapBlockCacheCont = std::unique_ptr<CServerMapBlock>;
	using MapBlockCache = std::unique_ptr<MapBlockCacheCont[]>;
	MapBlockCache _mapBlocks[MAP_SUPPORTED_QTY];

public:
	static const char* m_sClassName;
	CWorldCache();
	~CWorldCache() = default;

	void Init();

	void CheckMapBlockCache(int64 iCurTime, int64 iCacheTime);
};

#endif // _INC_CWORLDCACHE_H