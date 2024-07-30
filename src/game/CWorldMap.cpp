#include "../common/resource/sections/CItemTypeDef.h"
#include "../common/resource/sections/CRandGroupDef.h"
#include "../common/resource/sections/CRegionResourceDef.h"
#include "../common/CException.h"
#include "../common/CScriptTriggerArgs.h"
#include "../common/CRect.h"
#include "../common/CLog.h"
#include "../sphere/threads.h"
#include "../sphere/ProfileTask.h"
#include "chars/CChar.h"
#include "items/CItem.h"
#include "items/CItemMultiCustom.h"
#include "uo_files/CUOTerrainInfo.h"
#include "uo_files/CUOMapList.h"
#include "triggers.h"
#include "CWorld.h"
#include "CWorldCache.h"
#include "CWorldSearch.h"
#include "CWorldMap.h"

//************************
// Natural resources.

CItem * CWorldMap::CheckNaturalResource(const CPointMap & pt, IT_TYPE iType, bool fTest, CChar * pCharSrc ) // static
{
	ADDTOCALLSTACK("CWorldMap::CheckNaturalResource");
	// RETURN:
	//  The resource tracking item.
	//  nullptr = nothing here.

	if ( !pt.IsValidPoint() )
		return nullptr;

	EXC_TRY("CheckNaturalResource");

	// Check/Decrement natural resources at this location.
	// We create an invis object to time out and produce more.
	// RETURN: Quantity they wanted. 0 = none here.

	if ( fTest )	// Is the resource avail at all here ?
	{
		EXC_SET_BLOCK("is item near type");
		if ((iType != IT_TREE) && (iType != IT_ROCK) )
		{
			if ( !IsTypeNear_Top(pt, iType, 0) )
				return nullptr;
		}
		else
		{
			if ( !IsItemTypeNear(pt, iType, 0, false) ) //cannot be used, because it does no Z check... what if there is a static tile 70 tiles under me?
				return nullptr;
		}
	}

	// Find the resource object.
	EXC_SET_BLOCK("find existant bit");
	CItem * pResBit;
	auto Area = CWorldSearchHolder::GetInstance(pt);
	for (;;)
	{
		pResBit = Area->GetItem();
		if ( !pResBit )
			break;
		// NOTE: ??? Not all resource objects are world gems. should they be ?
		// I wanted to make tree stumps etc be the resource section some day.

		if ( pResBit->IsType(iType) && (pResBit->GetID() == ITEMID_WorldGem) )
			break;
	}

	// If none then create one.
	if ( pResBit )
		return pResBit;

	// What type of ore is here ?
	// NOTE: This is unrelated to the fact that we might not be skilled enough to find it.
	// Odds of a vein of ore being present are unrelated to my skill level.
	// Odds of my finding it are.
	// RES_REGIONRESOURCE from RES_REGIONTYPE linked to RES_AREA

	EXC_SET_BLOCK("get region");
	const CRegionWorld* pRegion = dynamic_cast<const CRegionWorld*>( pt.GetRegion( REGION_TYPE_AREA ));
	if ( !pRegion )
		return nullptr;

	Area->RestartSearch();
	Area->SetAllShow(true);
	for (;;)
	{
		CItem *pItem = Area->GetItem();
		if ( !pItem )
			break;
		if ( pItem->GetType() != iType )
			return nullptr;
	}

	// just use the background (default) region for this
	if (pRegion->m_Events.empty())
	{
		CPointMap ptZero(0,0,0,pt.m_map);
		pRegion = dynamic_cast<const CRegionWorld*>(ptZero.GetRegion(REGION_TYPE_AREA));
		ASSERT(pRegion);
	}

	// Find RES_REGIONTYPE
	EXC_SET_BLOCK("resource group");
	const CRandGroupDef * pResGroup = pRegion->FindNaturalResource(iType);
	if ( !pResGroup )
		return nullptr;

	// Find RES_REGIONRESOURCE
	EXC_SET_BLOCK("get random group element");
	size_t id = pResGroup->GetRandMemberIndex(pCharSrc);
	CRegionResourceDef * pOreDef;
	if ( id == sl::scont_bad_index() )
	{
		pOreDef	= dynamic_cast <CRegionResourceDef *>(g_Cfg.ResourceGetDefByName(RES_REGIONRESOURCE, "mr_nothing"));
	}
	else
	{
		CResourceID rid	= pResGroup->GetMemberID( id );
		pOreDef = dynamic_cast <CRegionResourceDef *>( g_Cfg.ResourceGetDef( rid ));
	}

	if ( !pOreDef )
		return nullptr;

	EXC_SET_BLOCK("create bit");
	pResBit = CItem::CreateScript(ITEMID_WorldGem, pCharSrc, iType); // override the type, otherwise the worldgem will be treated as a spawn item
	if ( !pResBit )
		return nullptr;

	pResBit->SetAttr(ATTR_INVIS|ATTR_MOVE_NEVER);
	pResBit->m_itResource.m_ridRes = pOreDef->GetResourceID();

	// Total amount of ore here.
	word amount = (word)pOreDef->m_vcAmount.GetRandom();
	if ( (g_Cfg.m_iRacialFlags & RACIALF_HUMAN_WORKHORSE) && pCharSrc->IsHuman() )
	{
		if ( (iType == IT_ROCK) && (pCharSrc->GetTopMap() == 0) )
			amount += 1;	// Workhorse racial bonus, giving +1 ore to humans in Felucca.
		else if ( (iType == IT_TREE) && (pCharSrc->GetTopMap() == 1) )
			amount += 2;	// Workhorse racial bonus, giving +2 logs to humans in Trammel.
	}
	pResBit->SetAmount( amount );
	pResBit->MoveToDecay(pt, pOreDef->m_vcRegenerateTime.GetRandom() * MSECS_PER_TENTH);	// Delete myself in this amount of time.

	EXC_SET_BLOCK("resourcefound");
	if ( pCharSrc != nullptr )
	{
		CScriptTriggerArgs Args(0, 0, pResBit);
		TRIGRET_TYPE tRet = TRIGRET_RET_DEFAULT;
		if ( IsTrigUsed(TRIGGER_REGIONRESOURCEFOUND) )
			tRet = pCharSrc->OnTrigger(CTRIG_RegionResourceFound, pCharSrc, &Args);
		if ( IsTrigUsed(TRIGGER_RESOURCEFOUND) )
			tRet = pOreDef->OnTrigger("@ResourceFound", pCharSrc, &Args);

		if (tRet == TRIGRET_RET_TRUE)
		{
			if ( pResBit->IsDisconnected() )
				return nullptr;
			pResBit->SetAmount(0);
		}
	}
	return pResBit;

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("point '%d,%d,%d,%d' type '%d' [0%x]\n", pt.m_x, pt.m_y, pt.m_z, pt.m_map, (int)(iType),
		pCharSrc ? (dword)(pCharSrc->GetUID()) : 0);
	EXC_DEBUG_END;
	return nullptr;
}


CItemTypeDef* CWorldMap::GetTerrainItemTypeDef(dword dwTerrainIndex) // static
{
	ADDTOCALLSTACK("CWorldMap::GetTerrainItemTypeDef");
	CResourceDef* pRes = nullptr;

	if (g_World.m_TileTypes.valid_index(dwTerrainIndex))
	{
		pRes = static_cast<CItemTypeDef*>(g_World.m_TileTypes[dwTerrainIndex].get());
	}

	if (!pRes)
	{
		pRes = g_Cfg.ResourceGetDef(CResourceID(RES_TYPEDEF, 0));
	}
	ASSERT(pRes);

	CItemTypeDef* pItemTypeDef = dynamic_cast <CItemTypeDef*> (pRes);
	ASSERT(pItemTypeDef);

	return pItemTypeDef;
}


IT_TYPE CWorldMap::GetTerrainItemType(dword dwTerrainIndex) // static
{
	ADDTOCALLSTACK("CWorldMap::GetTerrainItemType");
	CResourceDef* pRes = nullptr;

	if (g_World.m_TileTypes.valid_index(dwTerrainIndex))
	{
		pRes = static_cast<CItemTypeDef*>(g_World.m_TileTypes[dwTerrainIndex].get());
	}

	if (!pRes)
		return IT_NORMAL;

	const CItemTypeDef* pItemTypeDef = dynamic_cast<CItemTypeDef*>(pRes);
	if (!pItemTypeDef)
		return IT_NORMAL;

	return (IT_TYPE)pItemTypeDef->GetItemType();
}



//////////////////////////////////////////////////////////////////
// Map reading and blocking.

// gets sector # from one map
CSector* CWorldMap::GetSector(int map, int index) noexcept // static
{
	//ADDTOCALLSTACK_DEBUG("CWorldMap::GetSector(index)");

	const int iMapSectorQty = g_World._Sectors.GetSectorQty(map);
	if (index >= iMapSectorQty)
	{
		g_Log.EventError("Unsupported sector #%d for map #%d specified.\n", index, map);
		return nullptr;
	}

	return g_World._Sectors.GetSector(map, index);
}

CSector* CWorldMap::GetSector(int map, short x, short y) noexcept // static
{
	//ADDTOCALLSTACK_DEBUG("CWorldMap::GetSector(x,y)");
	return g_World._Sectors.GetSector(map, x, y);
}


const CServerMapBlock* CWorldMap::GetMapBlock(const CPointMap& pt) // static
{
	//ADDTOCALLSTACK_DEBUG("CWorldMap::GetMapBlock");
	// Get a map block from the cache. load it if not.

    EXC_TRY("GetMapBlock");

	if (!pt.IsValidXY() || !g_MapList.IsInitialized(pt.m_map))
	{
		g_Log.EventWarn("Attempting to access invalid map block at %s.\n", pt.WriteUsed());
		return nullptr;
	}

    EXC_SET_BLOCK("Try to get from cache");
	const ProfileTask mapTask(PROFILE_MAP);

	const int iBx = pt.m_x / UO_BLOCK_SIZE;
	const int iBy = pt.m_y / UO_BLOCK_SIZE;
	const int iBXMax = g_MapList.GetMapSizeX(pt.m_map) / UO_BLOCK_SIZE;
	const int iBlockIdx = (iBy * iBXMax) + iBx;
	CWorldCache::MapBlockCacheCont& block = g_World._Cache._mapBlocks[pt.m_map][iBlockIdx];
	if (block)
	{
		// Found it in cache.
		block->m_CacheTime.HitCacheTime();
		return block.get();
	}

    EXC_SET_BLOCK("Create new");
	// else load and add it to the cache.
	block = std::make_unique<CServerMapBlock>(iBx, iBy, pt.m_map);
	ASSERT(block);

	return block.get();

    EXC_CATCH;
    return nullptr;
}

// Tile info fromMAP*.MUL at given coordinates
const CUOMapMeter* CWorldMap::GetMapMeter(const CPointMap& pt) // static
{
	const CServerMapBlock* pMapBlock = GetMapBlock(pt);
	if (!pMapBlock)
		return nullptr;

	return pMapBlock->GetTerrain(UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
}

std::optional<CUOMapMeter> CWorldMap::GetMapMeterAdjusted(const CPointMap& pt)
{
	const CUOMapMeter* pMeter = GetMapMeter(pt);
	if (!pMeter)
		return std::nullopt;

	CUOMapMeter pMapTop(*pMeter);
	const CUOMapMeter pMapLeft(CheckMapTerrain(pMapTop, pt.m_x, pt.m_y + 1, pt.m_map));
	const CUOMapMeter pMapBottom(CheckMapTerrain(pMapTop, pt.m_x + 1, pt.m_y + 1, pt.m_map));
	const CUOMapMeter pMapRight(CheckMapTerrain(pMapTop, pt.m_x + 1, pt.m_y, pt.m_map));

	const short iAverage = GetAreaAverage(pMapTop.m_z, pMapLeft.m_z, pMapBottom.m_z, pMapRight.m_z);
	if ((char)abs((short)pMapTop.m_z - (short)pMapBottom.m_z) > (char)abs((short)pMapLeft.m_z - (short)pMapRight.m_z))
		pMapTop.m_z = GetFloorAvarage(pMapLeft.m_z, pMapRight.m_z, iAverage);
	else
		pMapTop.m_z = GetFloorAvarage(pMapTop.m_z, pMapBottom.m_z, iAverage);
	return std::make_optional<CUOMapMeter>(pMapTop);
}

bool CWorldMap::IsTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance ) // static
{
	ADDTOCALLSTACK("CWorldMap::IsTypeNear_Top");
	if ( !pt.IsValidPoint() )
		return false;
	const CPointMap ptn = FindTypeNear_Top( pt, iType, iDistance );
	return ptn.IsValidPoint();
}

CPointMap CWorldMap::FindTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance ) // static
{
	ADDTOCALLSTACK("CWorldMap::FindTypeNear_Top");

#define RESOURCE_Z_CHECK 8

	CPointMap ptFound;
	const CItemBase * pItemDef = nullptr;
	const CItemBaseDupe * pDupeDef = nullptr;
    CItem * pItem = nullptr;
	height_t Height = 0;
	byte z = 0;
	CPointMap ptTest;

	uint iRetElem = 4;

    CPointMap ptElem[5] = {{}};
	//for ( iQty = 0; iQty < 4; ++iQty )
	//	ptElem[iQty].m_z = UO_SIZE_MIN_Z;
	ptElem[0].m_z = ptElem[1].m_z  = ptElem[2].m_z  = ptElem[3].m_z = UO_SIZE_MIN_Z;
	ptElem[4] = CPointMap(INT16_MIN, INT16_MIN, UO_SIZE_MIN_Z);

	bool fElem[4] = { false, false, false, false };

	// Check dynamics
	auto Area = CWorldSearchHolder::GetInstance( pt, iDistance );
	Area->SetAllShow( true );
	for (;;)
	{
		z = 0;
		Height = 0;
		pItem = Area->GetItem();
		if ( pItem == nullptr )
			break;

        const CPointMap& ptItemTop = pItem->GetTopPoint();
		if ( pt.GetDist( ptItemTop ) > iDistance )
			continue;

		pItemDef = CItemBase::FindItemBase( pItem->GetDispID() );
		if ( pItemDef == nullptr )
			continue;

		Height = pItemDef->GetHeight();
		if ( pItemDef->GetID() != pItem->GetDispID() ) //not a parent item
		{
			pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE)(pItem->GetDispID()));
			if ( ! pDupeDef )
			{
				g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pItem->GetDispID(),ptTest.m_x,ptTest.m_y,ptTest.m_z);
				Height = pItemDef->GetHeight();
			}
			else
				Height = pDupeDef->GetHeight();
		}
		z = Height + ptItemTop.m_z;
		z = minimum(z , UO_SIZE_Z ); //height + current position = the top point

		if ( ptElem[0].m_z > z ) //if ( ptElem[0].m_z > pItem->GetTopPoint().m_z )
			continue;

		//if ( ((( pItem->GetTopPoint().m_z - pt.m_z ) > 0) && ( pItem->GetTopPoint().m_z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - pItem->GetTopPoint().m_z ) < 0) && (( pt.m_z - pItem->GetTopPoint().m_z ) < - RESOURCE_Z_CHECK )))
		if ( ((( z - pt.m_z ) > 0) && ( z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - z ) < 0) && (( pt.m_z - z ) < - RESOURCE_Z_CHECK )))
			continue;

		if (( z < ptElem[0].m_z ) || (( z == ptElem[0].m_z ) && ( fElem[0] )))
			continue;

		ptElem[0] = ptItemTop;
		ptElem[0].m_z = z;
		fElem[0] = false;

		//DEBUG_ERR(("dynamic pItem->IsType( iType %d) %d\n",iType,pItem->IsType( iType )));
		if ( pItem->IsType( iType ) )  //( pItem->Item_GetDef()->IsType(iType) ) )
		{
			fElem[0] = true;
			iRetElem = 0;
		}
	}

	// Parts of multis ?
	CRegionLinks rlinks;
	size_t iRegionQty = pt.GetRegions(REGION_TYPE_MULTI, &rlinks);
	if ( iRegionQty > 0 )
	{
        const CRegion *pRegion = nullptr;
		const CUOMulti *pMulti = nullptr;				// Multi Def (multi check)
		const CUOMultiItemRec_HS *pMultiItem = nullptr;	    // Multi item iterator
		for ( size_t iRegion = 0; iRegion < iRegionQty; pMulti = nullptr, ++iRegion )
		{
			pRegion = rlinks.at(iRegion);
			pItem = pRegion->GetResourceID().ItemFindFromResource();
			if ( !pItem )
				continue;
			pMulti = g_Cfg.GetMultiItemDefs(pItem);
			if ( !pMulti )
				continue;
			size_t iMultiQty = pMulti->GetItemCount();
			for ( size_t iMulti = 0; iMulti < iMultiQty; pItemDef = nullptr, pMultiItem = nullptr, Height = 0, ++iMulti )
			{
				pMultiItem = pMulti->GetItem(iMulti);

				if ( !pMultiItem )
					break;

				//DEBUG_ERR(("abs( pMultiItem->m_dx ) %x, abs( pMultiItem->m_dy ) %x, abs( pMultiItem->m_dz ) %x,\n
				//	iDistance %x IF STATEMENT %x %x\n",
				//	abs( pMultiItem->m_dx ), abs( pMultiItem->m_dy ), abs( pMultiItem->m_dz ), iDistance, ( abs( pMultiItem->m_dx ) <= iDistance ), ( abs( pMultiItem->m_dy ) <= iDistance ) ));

				if ( !pMultiItem->m_visible )
					continue;

				ptTest = CPointMap( pMultiItem->m_dx + pt.m_x, pMultiItem->m_dy + pt.m_y, (char)( pMultiItem->m_dz + pt.m_z ), pt.m_map );

				pItemDef = CItemBase::FindItemBase( pMultiItem->GetDispID() );
				if ( pItemDef == nullptr )
					continue;

				Height = pItemDef->GetHeight();
				if ( pItemDef->GetID() != pMultiItem->GetDispID() ) //not a parent item
				{
					pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE)(pMultiItem->GetDispID()));
					if ( ! pDupeDef )
					{
						g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pMultiItem->GetDispID(),ptTest.m_x,ptTest.m_y,ptTest.m_z);
						Height = pItemDef->GetHeight();
					}
					else
						Height = pDupeDef->GetHeight();
				}
				ptTest.m_z = minimum(ptTest.m_z + Height, UO_SIZE_Z); //height + current position = the top point

				if ( pt.GetDist( ptTest ) > iDistance )
					continue;

				if ( ptElem[1].m_z > ptTest.m_z )
					continue;

				if ( ((( ptTest.m_z - pt.m_z ) > 0) && ( ptTest.m_z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - ptTest.m_z ) < 0) && (( pt.m_z - ptTest.m_z ) < - RESOURCE_Z_CHECK )))
					continue;

				if (( ptTest.m_z < ptElem[1].m_z ) || (( ptTest.m_z == ptElem[1].m_z ) && ( fElem[1] )))
					continue;
		        //DEBUG_ERR(("pMultiItem->GetDispID()%x\n",pMultiItem->GetDispID()));
				ptElem[1] = ptTest;
				fElem[1] = false;

				//DEBUG_ERR(("multi pItemDef->IsType( iType %d) %d\n",iType,pItemDef->IsType( iType )));
				if ( pItemDef->IsType( iType ) )
				{
					fElem[1] = true;
					//if ( ptElem[iRetElem].m_z > ptElem[1].m_z )
					if ( ptElem[1].m_z > ptElem[iRetElem].m_z )
						iRetElem = 1;
				}

				//DEBUG_ERR(( "DISPID: %x X %d Y %d Z %d\n", pMultiItem->GetDispID(), (pMultiItem->m_dx), (pMultiItem->m_dy), (pMultiItem->m_dz) ));
			}
		}
	}

	// STATIC - checks one 8x8 block
	const CServerMapBlock * pMapBlock = GetMapBlock( pt );
	ASSERT( pMapBlock );

	uint iStaticQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iStaticQty > 0 )  // no static items here.
	{
		const CUOStaticItemRec * pStatic = nullptr;

		for ( uint i = 0; i < iStaticQty; ++i, pStatic = nullptr, Height = 0, pItemDef = nullptr )
		{
			pStatic = pMapBlock->m_Statics.GetStatic( i );

			ptTest = CPointMap( pStatic->m_x + pMapBlock->m_x, pStatic->m_y + pMapBlock->m_y, pStatic->m_z, pt.m_map );

			pItemDef = CItemBase::FindItemBase( pStatic->GetDispID() );
			if ( pItemDef == nullptr )
				continue;

			//DEBUG_ERR(("pStatic->GetDispID() %d; name %s; pStatic->m_z %d\n",pStatic->GetDispID(),pItemDef->GetName(),pStatic->m_z));
			Height = pItemDef->GetHeight();
			if ( pItemDef->GetID() != pStatic->GetDispID() ) //not a parent item
			{
				pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE)(pStatic->GetDispID()));
				if ( ! pDupeDef )
				{
					g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pStatic->GetDispID(),ptTest.m_x,ptTest.m_y,ptTest.m_z);
					Height = pItemDef->GetHeight();
				}
				else
					Height = pDupeDef->GetHeight();
			}
			ptTest.m_z = minimum(ptTest.m_z + Height, UO_SIZE_Z); //height + current position = the top point

			if ( pt.GetDist( ptTest ) > iDistance )
				continue;

			//if ( ptElem[2].m_z > pStatic->m_z )
			if ( ptElem[2].m_z > ptTest.m_z )
				continue;

			if ( ((( pStatic->m_z - pt.m_z ) > 0) && ( pStatic->m_z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - pStatic->m_z ) < 0) && (( pt.m_z - pStatic->m_z ) < - RESOURCE_Z_CHECK )))
				continue;

			if (( ptTest.m_z < ptElem[2].m_z ) || (( ptTest.m_z == ptElem[2].m_z ) && ( fElem[2] )))
				continue;

			ptElem[2] = ptTest;
			fElem[2] = false;

			//DEBUG_ERR(("static pItemDef->IsType( iType %d) %d;pItemDef->GetType() %d;pItemDef->GetID() %d;pItemDef->GetDispID() %d\n",iType,pItemDef->IsType( iType ),pItemDef->GetType(),pItemDef->GetID(),pItemDef->GetDispID()));
			if ( pItemDef->IsType( iType ) )
			{
				//DEBUG_ERR(("found %d; ptTest: %d,%d,%d\n",__LINE__,ptTest.m_x,ptTest.m_y,ptTest.m_z));
				fElem[2] = true;
				//DEBUG_ERR(("ptElem[iRetElem].m_z %d, ptElem[2].m_z %d\n",ptElem[iRetElem].m_z,ptElem[2].m_z));
				//if ( ptElem[iRetElem].m_z > ptElem[2].m_z )
				if ( ptElem[2].m_z > ptElem[iRetElem].m_z )
					iRetElem = 2;
			}
		}
	}

	// Check for appropriate terrain type
	CRectMap rect;
	rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
		pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
		pt.m_map);

	const CUOMapMeter * pMeter = nullptr;
	for ( int x = rect.m_left; x < rect.m_right; ++x, pMeter = nullptr )
	{
		for ( int y = rect.m_top; y < rect.m_bottom; ++y, pMeter = nullptr )
		{
			ptTest = CPointMap((word)(x), (word)(y), pt.m_z, pt.m_map);
			pMeter = GetMapMeter(ptTest);
			if ( !pMeter )
				continue;
			if ( pt.GetDist( ptTest ) > iDistance )
				continue;
			if ( ptElem[3].m_z > pMeter->m_z )
				continue;

			//DEBUG_ERR(("(( pMeter->m_z (%d) - pt.m_z (%d) ) > 0) && ( pMeter->m_z (%d) - pt.m_z (%d) ) > RESOURCE_Z_CHECK (%d) >> %d\n",pMeter->m_z,pt.m_z,pMeter->m_z,pt.m_z,RESOURCE_Z_CHECK,(( pMeter->m_z - pt.m_z ) > 0) && ( pMeter->m_z - pt.m_z ) > RESOURCE_Z_CHECK));
			//DEBUG_ERR(("(( pt.m_z (%d) - pMeter->m_z (%d) ) < 0) && (( pt.m_z (%d) - pMeter->m_z (%d) ) < - RESOURCE_Z_CHECK (%d) )) >> %d\n",pt.m_z,pMeter->m_z,pt.m_z,pMeter->m_z,- RESOURCE_Z_CHECK,((( pt.m_z - pMeter->m_z ) < 0) && (( pt.m_z - pMeter->m_z ) < - RESOURCE_Z_CHECK ))));
			if ( ((( pMeter->m_z - pt.m_z ) > 0) && ( pMeter->m_z - pt.m_z ) > RESOURCE_Z_CHECK ) || ((( pt.m_z - pMeter->m_z ) < 0) && (( pt.m_z - pMeter->m_z ) < - RESOURCE_Z_CHECK )))
				continue;

			//DEBUG_ERR(("pMeter->m_z (%d) < ptElem[3].m_z (%d) >> %d\n",pMeter->m_z,ptElem[3].m_z,pMeter->m_z < ptElem[3].m_z));
			if (( pMeter->m_z < ptElem[3].m_z ) || (( pMeter->m_z == ptElem[3].m_z ) && ( fElem[3] )))
				continue;

			ptElem[3] = ptTest;
			fElem[3] = false;

			//DEBUG_ERR(("iType %x, TerrainType %x\n",iType,g_World.GetTerrainItemType( pMeter->m_wTerrainIndex )));
			if ( iType == GetTerrainItemType( pMeter->m_wTerrainIndex ) )
			{
				fElem[3] = true;
				//if ( ptElem[iRetElem].m_z > ptElem[3].m_z )
				//DEBUG_ERR(("ptElem[3].m_z %d; ptElem[iRetElem].m_z %d\n",ptElem[3].m_z, ptElem[iRetElem].m_z));
				if ( ptElem[3].m_z > ptElem[iRetElem].m_z )
						iRetElem = 3;
				//DEBUG_ERR(("fElem3 %d %d\n",ptElem[3].m_z, fElem[3]));
				continue;
			}

			rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance, pt.m_x + iDistance + 1, pt.m_y + iDistance + 1, pt.m_map);
		}
	}

	/*CPointMap a;
	a.m_z = maximum(
	// priority dynamic->multi->static->terrain
	int iRetElem;
	if (( ptElem[0].m_z >= ptElem[1].m_z ) && ( fElem[0] ))
		iRetElem = 0;
	else if (( ptElem[1].m_z >= ptElem[2].m_z ) && ( fElem[1] ))
		iRetElem = 1;
	else if (( ptElem[2].m_z >= ptElem[3].m_z ) && ( fElem[2] ))
		iRetElem = 2;
	else if ( fElem[3] )
		iRetElem = 3;
	else
		iRetElem = 4;*/

	ASSERT(iRetElem < ARRAY_COUNT(ptElem));
    if (0 != iRetElem && ptElem[0].m_z > ptElem[iRetElem].m_z)
        iRetElem = 4;
    else if (1 != iRetElem && ptElem[1].m_z > ptElem[iRetElem].m_z)
        iRetElem = 4;
    else if (2 != iRetElem && ptElem[2].m_z > ptElem[iRetElem].m_z)
        iRetElem = 4;
    else if (3 != iRetElem && ptElem[3].m_z > ptElem[iRetElem].m_z)
        iRetElem = 4;

	//DEBUG_ERR(("iRetElem %d; %d %d %d %d; %d %d %d ISVALID: %d\n",iRetElem,ptElem[1].m_z,ptElem[2].m_z,ptElem[3].m_z,ptElem[4].m_z,pt.m_x,pt.m_y,pt.m_z,ptElem[iRetElem].IsValidPoint()));
	//DEBUG_ERR(("X: %d  Y: %d  Z: %d\n",ptElem[iRetElem].m_x,ptElem[iRetElem].m_y,ptElem[iRetElem].m_z));
	return ( ptElem[iRetElem] );
#undef RESOURCE_Z_CHECK
}

bool CWorldMap::IsItemTypeNear( const CPointMap & pt, IT_TYPE iType, int iDistance, bool fCheckMulti ) // static
{
	ADDTOCALLSTACK("CWorldMap::IsItemTypeNear");
	if ( !pt.IsValidPoint() )
		return false;
	const CPointMap ptn = FindItemTypeNearby( pt, iType, iDistance, fCheckMulti );
	return ptn.IsValidPoint();
}

CPointMap CWorldMap::FindItemTypeNearby(const CPointMap & pt, IT_TYPE iType, int iDistance, bool fCheckMulti, bool fLimitZ) // static
{
	ADDTOCALLSTACK("CWorldMap::FindItemTypeNearby");
	// Find the closest item of this type.
	// This does not mean that i can touch it.
	// ARGS:
	//   iDistance = 2d distance to search.
	//   fCheckMulti = check also the basecomponents of a MULTI
	//   fLimitZ = the search is limited to the Z of the selected point.

	CPointMap ptFound;
	int iTestDistance;

	// Check dynamics first since they are the easiest.
	auto Area = CWorldSearchHolder::GetInstance( pt, iDistance );
	for (;;)
	{
        const CItem * pItem = Area->GetItem();
		if ( pItem == nullptr )
			break;

		if ( ! pItem->IsType( iType ) && ! pItem->Item_GetDef()->IsType(iType) )
			continue;
        const CPointMap& ptItemTop = pItem->GetTopPoint();
		if ( fLimitZ && ( ptItemTop.m_z != pt.m_z ))
			continue;

		iTestDistance = pt.GetDist(ptItemTop);
		if ( iTestDistance > iDistance )
			continue;

		ptFound = ptItemTop;
		iDistance = iTestDistance;	// tighten up the search.
		if ( ! iDistance )
			return ptFound;
	}

	// Check for appropriate terrain type
	CRectMap rect;
	rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
		pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
		pt.m_map);

	const CUOMapMeter * pMeter = nullptr;
	for (int x = rect.m_left; x < rect.m_right; ++x, pMeter = nullptr )
	{
		for ( int y = rect.m_top; y < rect.m_bottom; ++y, pMeter = nullptr )
		{
            CPointMap ptTest((short)x, (short)y, pt.m_z, pt.m_map);
			pMeter = GetMapMeter(ptTest);

			if ( !pMeter )
				continue;
			if ( fLimitZ && ( pMeter->m_z != pt.m_z ) )
				continue;

			ptTest.m_z = pMeter->m_z;

			if ( iType != GetTerrainItemType( pMeter->m_wTerrainIndex ) )
				continue;

			iTestDistance = pt.GetDist(ptTest);
			if ( iTestDistance > iDistance )
				break;

			ptFound = ptTest;
			iDistance = iTestDistance;	// tighten up the search.
			if ( ! iDistance )
				return ptFound;

			rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
				pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
				pt.m_map);
		}
	}


	// Check for statics
	rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
		pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
		pt.m_map);

	const CServerMapBlock * pMapBlock = nullptr;
	const CUOStaticItemRec * pStatic = nullptr;
	const CItemBase * pItemDef = nullptr;

	for (int x = rect.m_left; x < rect.m_right; x += UO_BLOCK_SIZE, pMapBlock = nullptr )
	{
		for ( int y = rect.m_top; y < rect.m_bottom; y += UO_BLOCK_SIZE, pMapBlock = nullptr )
		{
            const CPointMap ptTest((short)x, (short)y, pt.m_z, pt.m_map);
			pMapBlock = GetMapBlock( ptTest );

			if ( !pMapBlock )
				continue;

			size_t iQty = pMapBlock->m_Statics.GetStaticQty();
			if ( iQty <= 0 )
				continue;

			pStatic = nullptr;
			pItemDef = nullptr;

			for ( uint i = 0; i < iQty; ++i, pStatic = nullptr, pItemDef = nullptr )
			{
				pStatic = pMapBlock->m_Statics.GetStatic( i );
				if ( fLimitZ && ( pStatic->m_z != ptTest.m_z ) )
					continue;

				// inside the range we want ?
                const CPointMap ptStatic( pStatic->m_x+pMapBlock->m_x, pStatic->m_y+pMapBlock->m_y, pStatic->m_z, ptTest.m_map);
				iTestDistance = pt.GetDist(ptStatic);
				if ( iTestDistance > iDistance )
					continue;

                const ITEMID_TYPE idTile = pStatic->GetDispID();

				// Check the script def for the item.
				pItemDef = CItemBase::FindItemBase( idTile );
				if ( pItemDef == nullptr )
					continue;
				if ( ! pItemDef->IsType( iType ))
					continue;

				ptFound = ptStatic;
				iDistance = iTestDistance;
				if ( ! iDistance )
					return ptFound;

				rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
					pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
					pt.m_map);
			}
		}
	}

	// Check for multi components
	if (fCheckMulti == true)
	{
		rect.SetRect( pt.m_x - iDistance, pt.m_y - iDistance,
			pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
			pt.m_map);

		for (int x = rect.m_left; x < rect.m_right; ++x)
		{
			for (int y = rect.m_top; y < rect.m_bottom; ++y)
			{
                const CPointMap ptTest((short)x, (short)y, pt.m_z, pt.m_map);

				CRegionLinks rlinks;
				size_t iRegionQty = ptTest.GetRegions(REGION_TYPE_MULTI, &rlinks);
				if ( iRegionQty > 0 )
				{
					for (size_t iRegion = 0; iRegion < iRegionQty; ++iRegion)
					{
						const CRegion* pRegion = rlinks[iRegion];
						CItem* pRegionItem = pRegion->GetResourceID().ItemFindFromResource();
						if (pRegionItem == nullptr)
							continue;

                        const CPointMap& ptTop = pRegionItem->GetTopPoint();
						//Differences between the coordinates we are checking and the MULTI gembit coordinates.
                        const short x2 = ptTest.m_x - ptTop.m_x;
                        const short y2 = ptTest.m_y - ptTop.m_y;
                        const short z2 = ptTest.m_z - ptTop.m_z;

                        if (CItemMultiCustom* pItemMultiCustom = dynamic_cast<CItemMultiCustom*>(pRegionItem))
                        {
                            CItemMultiCustom::CMultiComponent* pComponents[INT8_MAX];
                            size_t iItemQty = pItemMultiCustom->GetComponentsAt(x2, y2, (char)z2, pComponents, pItemMultiCustom->GetDesignMain());
                            if (iItemQty <= 0)
                                continue;

                            for (size_t iItem = 0; iItem < iItemQty; ++iItem)
                            {
                                const CUOMultiItemRec_HS* pMultiItem = &pComponents[iItem]->m_item;
                                ASSERT(pMultiItem);

                                iTestDistance = pt.GetDist(ptTest);
                                if (iTestDistance > iDistance)
                                    continue;

                                const ITEMID_TYPE idTile = pMultiItem->GetDispID();

                                // Check the script def for the item.
                                pItemDef = CItemBase::FindItemBase(idTile);
                                if (pItemDef == nullptr || !pItemDef->IsType(iType))
                                    continue;

                                if (fLimitZ) //This checks if the item is on the floor
                                {
									if ((pMultiItem->m_dz != z2))
										continue;
									/*
									* The following code would only work if the item of the type we want to find has an height 0.
									* Because most of MULTI components has an height value it was impossible to find a t_wall/t_window/t_chair and so on
                                    const height_t uiItemHeight = pItemDef->GetHeight();
								   if ((pMultiItem->m_dz + uiItemHeight) != z2)
                                        continue;
									*/
                                }

                                ptFound = ptTest;
                                iDistance = iTestDistance;
                                if (!iDistance)
                                    return ptFound;
                            }
                        }
                        if (const CUOMulti* pSphereMulti = g_Cfg.GetMultiItemDefs(pRegionItem))
                        {
                            size_t iItemQty = pSphereMulti->GetItemCount();
                            for (size_t iItem = 0; iItem < iItemQty; ++iItem)
                            {
                                const CUOMultiItemRec_HS* pMultiItem = pSphereMulti->GetItem(iItem);
                                ASSERT(pMultiItem);

                                if (!pMultiItem->m_visible)
                                    continue;
                                if (pMultiItem->m_dx != x2 || pMultiItem->m_dy != y2)
                                    continue;
                                //if (fLimitZ && (pMultiItem->m_dz != z2))
                                //    continue;

                                iTestDistance = pt.GetDist(ptTest);
                                if (iTestDistance > iDistance)
                                    continue;

                                const ITEMID_TYPE idTile = pMultiItem->GetDispID();

                                // Check the script def for the item.
                                pItemDef = CItemBase::FindItemBase(idTile);
                                if (pItemDef == nullptr || !pItemDef->IsType(iType))
                                    continue;

                                if (fLimitZ) //Checking if we are in the same floor.
                                {

									if (pMultiItem->m_dz != z2)
										continue;
									/*
									* The following code would only work if the item of the type we want to find has an height 0.
									* Because most of MULTI components has an height value it was impossible to find a t_wall/t_window/t_chair and so on
									const height_t uiItemHeight = pItemDef->GetHeight();
                                    if ((pMultiItem->m_dz + uiItemHeight) != z2)
                                        continue;
                                    else if (pItemDef->IsType(IT_WALL))
                                        continue;
									*/
                                    // A valid building will have a floor on the top of a wall, so i can technically put an item
                                    //  at the height == top of the wall, which is the height of the floor of the upper level, since
                                    //  the floor has 0 height.
                                }

                                ptFound = ptTest;
                                iDistance = iTestDistance;
                                if (!iDistance)
                                    return ptFound;

                                rect.SetRect(pt.m_x - iDistance, pt.m_y - iDistance,
                                    pt.m_x + iDistance + 1, pt.m_y + iDistance + 1,
                                    pt.m_map);
                            }
                        }
					}
				}
			}
		}
	}

	return ptFound;
}


//****************************************************

void CWorldMap::GetFixPoint( const CPointMap & pt, CServerMapBlockingState & block) // static
{
	//Will get the highest CAN_I_PLATFORM|CAN_I_CLIMB and places it into block.Bottom
	ADDTOCALLSTACK("CWorldMap::GetFixPoint");

	CItemBase * pItemDef = nullptr;
	CItemBaseDupe * pDupeDef = nullptr;
	CItem * pItem = nullptr;
	uint64 uiBlockThis = 0;
	char z = 0;
	int x2 = 0, y2 = 0;

	// Height of statics at/above given coordinates
	// do gravity here for the z.
	const CServerMapBlock * pMapBlock = GetMapBlock( pt );
	if (pMapBlock == nullptr)
		return;

	size_t iQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iQty > 0 )  // any static items here?
	{
		x2 = pMapBlock->GetOffsetX(pt.m_x);
		y2 = pMapBlock->GetOffsetY(pt.m_y);
		const CUOStaticItemRec * pStatic = nullptr;
		for ( uint i = 0; i < iQty; ++i, z = 0, pStatic = nullptr, pDupeDef = nullptr )
		{
			if ( ! pMapBlock->m_Statics.IsStaticPoint( i, x2, y2 ))
				continue;

			pStatic = pMapBlock->m_Statics.GetStatic( i );
			if ( pStatic == nullptr )
				continue;

			z = pStatic->m_z;

            const ITEMID_TYPE iDispID = pStatic->GetDispID();
			pItemDef = CItemBase::FindItemBase( iDispID );
			if ( pItemDef )
			{
				if (pItemDef->GetID() == iDispID) //parent item
				{
					uiBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
					z += ((uiBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight()/2 : pItemDef->GetHeight());
				}
				else //non-parent item
				{
					pDupeDef = CItemBaseDupe::GetDupeRef(iDispID);
					if ( ! pDupeDef )
					{
						g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X: %d Y: %d Z: %d)\n",iDispID,pStatic->m_x+pMapBlock->m_x,pStatic->m_y+pMapBlock->m_y,pStatic->m_z);
						uiBlockThis = ( pItemDef->m_Can & CAN_I_MOVEMASK );
						z += ((uiBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight()/2 : pItemDef->GetHeight());
					}
					else
					{
						uiBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK);
						z += ((uiBlockThis & CAN_I_CLIMB) ? pDupeDef->GetHeight()/2 : pDupeDef->GetHeight());
					}
				}
			}
			else if ( iDispID )
            {
				CItemBase::GetItemTiledataFlags(&uiBlockThis, iDispID);
            }

			if (block.m_Bottom.m_z < z)
			{
				if ((z < pt.m_z + PLAYER_HEIGHT) && (uiBlockThis & (CAN_I_PLATFORM|CAN_I_CLIMB|CAN_I_WATER)))
				{
					block.m_Bottom.m_uiBlockFlags = uiBlockThis;
					block.m_Bottom.m_dwTile = iDispID + (ITEMID_TYPE)TERRAIN_QTY;
					block.m_Bottom.m_z = z;
                    // Leave block->...->m_height unchanged, since it already has the height of the char/item
				}
				else if (block.m_Top.m_z > z)
				{
					block.m_Top.m_uiBlockFlags = uiBlockThis;
					block.m_Top.m_dwTile = iDispID + (ITEMID_TYPE)TERRAIN_QTY;
					block.m_Top.m_z = z;
                    // Leave block->...->m_height unchanged, since it already has the height of the char/item
				}
			}
		}
	}

	pItemDef = nullptr;
	pDupeDef = nullptr;
	pItem = nullptr;
	uiBlockThis = 0;
	z = 0;
	x2 = y2 = 0;
	iQty = 0;

	// Any multi items here ?
	// Check all of them
	CRegionLinks rlinks;
	size_t iRegionQty = pt.GetRegions( REGION_TYPE_MULTI, &rlinks );
	if ( iRegionQty > 0 )
	{
		//  ------------ For variables --------------------
		const CRegion * pRegion = nullptr;
		const CUOMulti * pMulti = nullptr;
		const CUOMultiItemRec_HS * pMultiItem = nullptr;
		x2 = 0;
		y2 = 0;
		//  ------------ For variables --------------------

		for ( size_t iRegion = 0; iRegion < iRegionQty; ++iRegion, pRegion = nullptr, pItem = nullptr, pMulti = nullptr, x2 = 0, y2 = 0 )
		{
			pRegion = rlinks[iRegion];
			if ( pRegion != nullptr )
				pItem = pRegion->GetResourceID().ItemFindFromResource();

			if ( pItem != nullptr )
			{
				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if ( pMulti )
				{
                    const CPointMap& ptItemTop = pItem->GetTopPoint();
					x2 = pt.m_x - ptItemTop.m_x;
					y2 = pt.m_y - ptItemTop.m_y;
					iQty = pMulti->GetItemCount();
					for ( size_t ii = 0; ii < iQty; ++ii, pMultiItem = nullptr, z = 0 )
					{
						pMultiItem = pMulti->GetItem(ii);

						if ( !pMultiItem )
							break;

						if ( ! pMultiItem->m_visible )
							continue;

						if ( pMultiItem->m_dx != x2 || pMultiItem->m_dy != y2 )
							continue;

						z = (char)(pItem->GetTopZ() + pMultiItem->m_dz);

                        const ITEMID_TYPE iDispID = pMultiItem->GetDispID();
						pItemDef = CItemBase::FindItemBase( iDispID );
						if ( pItemDef != nullptr )
						{
							if ( pItemDef->GetID() == iDispID ) //parent item
							{
								uiBlockThis = ( pItemDef->m_Can & CAN_I_MOVEMASK );
								z += ((uiBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight()/2 : pItemDef->GetHeight());
							}
							else //non-parent item
							{
								pDupeDef = CItemBaseDupe::GetDupeRef(iDispID);
								if ( pDupeDef == nullptr )
								{
									g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X: %d Y: %d Z: %d)\n",iDispID,pMultiItem->m_dx+pItem->GetTopPoint().m_x,pMultiItem->m_dy+pItem->GetTopPoint().m_y,pMultiItem->m_dz+pItem->GetTopPoint().m_z);
									uiBlockThis = ( pItemDef->m_Can & CAN_I_MOVEMASK );
									z += ((uiBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight()/2 : pItemDef->GetHeight());
								}
								else
								{
									uiBlockThis = ( pDupeDef->m_Can & CAN_I_MOVEMASK );
									z += ((uiBlockThis & CAN_I_CLIMB) ? pDupeDef->GetHeight()/2 : pDupeDef->GetHeight());
								}
							}
						}
						else if ( iDispID )
                        {
							CItemBase::GetItemTiledataFlags(&uiBlockThis, iDispID);
                        }

						if (block.m_Bottom.m_z < z)
						{
							if ((z < pt.m_z + PLAYER_HEIGHT) && (uiBlockThis & (CAN_I_PLATFORM|CAN_I_CLIMB|CAN_I_WATER)))
							{
								block.m_Bottom.m_uiBlockFlags = uiBlockThis;
								block.m_Bottom.m_dwTile = iDispID + (ITEMID_TYPE)TERRAIN_QTY;
								block.m_Bottom.m_z = z;
                                // Leave block->...->m_height unchanged, since it already has the height of the char/item
							}
							else if (block.m_Top.m_z > z)
							{
								block.m_Top.m_uiBlockFlags = uiBlockThis;
								block.m_Top.m_dwTile = iDispID + (ITEMID_TYPE)TERRAIN_QTY;
								block.m_Top.m_z = z;
                                // Leave block->...->m_height unchanged, since it already has the height of the char/item
							}
						}
					}
				}
			}
		}
	}

	pItemDef = nullptr;
	pDupeDef = nullptr;
	pItem = nullptr;
	uiBlockThis = 0;
	x2 = y2 = 0;
	iQty = 0;
	z = 0;

	// Any dynamic items here ?
	// NOTE: This could just be an item that an NPC could just move ?
	auto Area = CWorldSearchHolder::GetInstance( pt );

	for (;;)
	{
		pItem = Area->GetItem();
		if ( !pItem )
			break;

		z = pItem->GetTopZ();

		// Invis items should not block ???
        const ITEMID_TYPE iDispID = pItem->GetDispID();
		pItemDef = CItemBase::FindItemBase( iDispID );

		if ( pItemDef )
		{
			if ( pItemDef->GetDispID() == iDispID )//parent item
			{
				uiBlockThis = ( pItemDef->m_Can & CAN_I_MOVEMASK );
				z += ((uiBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight()/2 : pItemDef->GetHeight());
			}
			else //non-parent item
			{
				pDupeDef = CItemBaseDupe::GetDupeRef(iDispID);
				if ( ! pDupeDef )
				{
					g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X: %d Y: %d Z: %d)\n",iDispID,pItem->GetTopPoint().m_x,pItem->GetTopPoint().m_y,pItem->GetTopPoint().m_z);
					uiBlockThis = ( pItemDef->m_Can & CAN_I_MOVEMASK );
					z += ((uiBlockThis & CAN_I_CLIMB) ? pItemDef->GetHeight()/2 : pItemDef->GetHeight());
				}
				else
				{
					uiBlockThis = ( pDupeDef->m_Can & CAN_I_MOVEMASK );
					z += ((uiBlockThis & CAN_I_CLIMB) ? pDupeDef->GetHeight()/2 : pDupeDef->GetHeight());
				}
			}

			if ( block.m_Bottom.m_z < z )
			{
				if ( (z < pt.m_z + PLAYER_HEIGHT) && (uiBlockThis & (CAN_I_PLATFORM|CAN_I_CLIMB|CAN_I_WATER)) )
				{
					block.m_Bottom.m_uiBlockFlags = uiBlockThis;
					block.m_Bottom.m_dwTile = pItemDef->GetDispID() + (ITEMID_TYPE)TERRAIN_QTY;
					block.m_Bottom.m_z = z;
                    // Leave block->...->m_height unchanged, since it already has the height of the char/item
				}
				else if ( block.m_Top.m_z > z )
				{
					block.m_Top.m_uiBlockFlags = uiBlockThis;
					block.m_Top.m_dwTile = pItemDef->GetDispID() + (ITEMID_TYPE)TERRAIN_QTY;
					block.m_Top.m_z = z;
                    // Leave block->...->m_height unchanged, since it already has the height of the char/item
				}
			}
		}
		else if (iDispID)
        {
			CItemBase::GetItemTiledataFlags(&uiBlockThis, pItem->GetDispID());
        }
	}

	uiBlockThis = 0;
	// Terrain height is screwed. Since it is related to all the terrain around it.
	const CUOMapMeter * pMeter = pMapBlock->GetTerrain( UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y) );
	if ( ! pMeter )
		return;

	if ( pMeter->m_wTerrainIndex == TERRAIN_HOLE )
	{
		uiBlockThis = 0;
	}
	else if ( CUOMapMeter::IsTerrainNull( pMeter->m_wTerrainIndex ) )	// inter dungeon type.
	{
		uiBlockThis = CAN_I_BLOCK;
	}
	else
	{
		CUOTerrainInfo land( pMeter->m_wTerrainIndex );
		//DEBUG_ERR(("Terrain flags - land.m_flags 0%x dwBlockThis (0%x)\n",land.m_flags,dwBlockThis));
		if ( land.m_flags & UFLAG1_WATER )
			uiBlockThis |= CAN_I_WATER;
		if ( land.m_flags & UFLAG1_DAMAGE )
			uiBlockThis |= CAN_I_FIRE;
		if ( land.m_flags & UFLAG1_BLOCK )
			uiBlockThis |= CAN_I_BLOCK;
		if ((!uiBlockThis) || ( land.m_flags & UFLAG2_PLATFORM )) // Platform items should take precendence over non-platforms.
			uiBlockThis = CAN_I_PLATFORM;
	}

	if (block.m_Bottom.m_z < pMeter->m_z)
	{
		if (((pMeter->m_z < pt.m_z + PLAYER_HEIGHT) && (uiBlockThis & (CAN_I_PLATFORM|CAN_I_CLIMB|CAN_I_WATER))) || (block.m_Bottom.m_z == UO_SIZE_MIN_Z))
		{
			block.m_Bottom.m_uiBlockFlags = uiBlockThis;
			block.m_Bottom.m_dwTile = pMeter->m_wTerrainIndex;
			block.m_Bottom.m_z = pMeter->m_z;
            // Leave block->...->m_height unchanged, since it already has the height of the char/item
		}
		else if (block.m_Top.m_z > pMeter->m_z)
		{
			block.m_Top.m_uiBlockFlags = uiBlockThis;
			block.m_Top.m_dwTile = pMeter->m_wTerrainIndex;
			block.m_Top.m_z = pMeter->m_z;
            // Leave block->...->m_height unchanged, since it already has the height of the char/item
		}
	}

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
	{
		//Fail safe...  Reset to 0z with no top block;
		block.m_Bottom.m_uiBlockFlags = 0;
		block.m_Bottom.m_dwTile = 0;
		block.m_Bottom.m_z = 0;
        // Leave block->...->m_height unchanged, since it already has the height of the char/item
		block.m_Top.m_uiBlockFlags = 0;
		block.m_Top.m_dwTile = 0;
		block.m_Top.m_z = UO_SIZE_Z;
	}
}

void CWorldMap::GetHeightPoint(const CPointMap & pt, CServerMapBlockingState & block, bool fHouseCheck) // static
{
	ADDTOCALLSTACK_DEBUG("CWorldMap::GetHeightPoint");
    const CItemBase * pItemDef = nullptr;
    const CItemBaseDupe * pDupeDef = nullptr;
	CItem * pItem = nullptr;
	uint64 uiBlockThis = 0;
    int x2 = 0, y2 = 0;
	char z = 0;
	height_t zHeight = 0;

	// Height of statics at/above given coordinates
	// do gravity here for the z.
	const CServerMapBlock * pMapBlock = GetMapBlock( pt );
	if (pMapBlock == nullptr)
		return;

	uint iQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iQty > 0 )  // no static items here.
	{
		x2 = pMapBlock->GetOffsetX(pt.m_x);
		y2 = pMapBlock->GetOffsetY(pt.m_y);
		const CUOStaticItemRec * pStatic = nullptr;
		for ( uint i = 0; i < iQty; ++i, z = 0, zHeight = 0, pStatic = nullptr, pDupeDef = nullptr )
		{
			if ( ! pMapBlock->m_Statics.IsStaticPoint( i, x2, y2 ))
				continue;

			pStatic = pMapBlock->m_Statics.GetStatic( i );
			if ( pStatic == nullptr )
				continue;

			z = pStatic->m_z;

            const ITEMID_TYPE iDispID = pStatic->GetDispID();
			pItemDef = CItemBase::FindItemBase( iDispID );
			if ( pItemDef )
			{
				//DEBUG_ERR(("pItemDef->GetID(0%x) pItemDef->GetDispID(0%x) pStatic->GetDispID(0%x)\n",pItemDef->GetID(),pItemDef->GetDispID(),pStatic->GetDispID()));
				if ( pItemDef->GetID() == iDispID ) //parent item
				{
					zHeight = pItemDef->GetHeight();
					uiBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK); //Use only Block flags, other remove
				}
				else //non-parent item
				{
					pDupeDef = CItemBaseDupe::GetDupeRef(iDispID);
					if ( ! pDupeDef )
					{
						g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X: %d Y: %d Z: %d)\n",iDispID,pStatic->m_x+pMapBlock->m_x,pStatic->m_y+pMapBlock->m_y,pStatic->m_z);
						zHeight = pItemDef->GetHeight();
						uiBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
					}
					else
					{
						zHeight = pDupeDef->GetHeight();
						uiBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK); //Use only Block flags, other remove - CAN flags cannot be inherited from the parent item due to bad script pack...
					}
				}
			}
			else if ( iDispID )
            {
				CItemBase::GetItemTiledataFlags(&uiBlockThis, iDispID);
            }

            //DEBUG_ERR(("z (%d)  block.m_iHeight (%d) block.m_Bottom.m_z (%d)\n",z,block.m_iHeight,block.m_Bottom.m_z));

            // This static is at the coordinates in question.
            // enough room for me to stand here ?
			block.CheckTile_Item(uiBlockThis, z, zHeight, iDispID + (ITEMID_TYPE)TERRAIN_QTY);
		}
	}

	pItemDef = nullptr;
	pDupeDef = nullptr;
	pItem = nullptr;
	uiBlockThis = 0;
	z = 0;
	zHeight = 0;
	x2 = y2 = 0;
	iQty = 0;

	// Any multi items here ?
	// Check all of them
	if ( fHouseCheck )
	{
		CRegionLinks rlinks;
		size_t iRegionQty = pt.GetRegions( REGION_TYPE_MULTI, &rlinks );
		if ( iRegionQty > 0 )
		{
			//  ------------ For variables --------------------
            const CRegion * pRegion = nullptr;
			const CUOMulti * pMulti = nullptr;
			const CUOMultiItemRec_HS * pMultiItem = nullptr;
			x2 = 0;
			y2 = 0;
			//  ------------ For variables --------------------

			for ( uint iRegion = 0; iRegion < iRegionQty; ++iRegion, pRegion = nullptr, pItem = nullptr, pMulti = nullptr, x2 = 0, y2 = 0 )
			{
				pRegion = rlinks[iRegion];
				if ( pRegion != nullptr )
					pItem = pRegion->GetResourceID().ItemFindFromResource();

				if ( pItem != nullptr )
				{
					pMulti = g_Cfg.GetMultiItemDefs(pItem);
					if ( pMulti )
					{
                        const CPointMap& ptItemTop = pItem->GetTopPoint();
						x2 = pt.m_x - ptItemTop.m_x;
						y2 = pt.m_y - ptItemTop.m_y;

						iQty = pMulti->GetItemCount();
						for ( uint ii = 0; ii < iQty; ++ii, pMultiItem = nullptr, z = 0, zHeight = 0 )
						{
							pMultiItem = pMulti->GetItem(ii);

							if ( !pMultiItem )
								break;

							if ( ! pMultiItem->m_visible )
								continue;

							if ( pMultiItem->m_dx != x2 || pMultiItem->m_dy != y2 )
								continue;

							z = (char)( pItem->GetTopZ() + pMultiItem->m_dz );

                            const ITEMID_TYPE iDispID = pMultiItem->GetDispID();
							pItemDef = CItemBase::FindItemBase( iDispID );
							if ( pItemDef != nullptr )
							{
								if ( pItemDef->GetID() == iDispID ) //parent item
								{
									zHeight = pItemDef->GetHeight();
									uiBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK); //Use only Block flags, other remove
								}
								else //non-parent item
								{
									pDupeDef = CItemBaseDupe::GetDupeRef(iDispID);
									if ( pDupeDef == nullptr )
									{
										g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X: %d Y: %d Z: %d)\n",
                                            iDispID, pMultiItem->m_dx + ptItemTop.m_x, pMultiItem->m_dy + ptItemTop.m_y, pMultiItem->m_dz + ptItemTop.m_z);
										zHeight = pItemDef->GetHeight();
										uiBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
									}
									else
									{
										zHeight = pDupeDef->GetHeight();
										uiBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK); //Use only Block flags, other remove - CAN flags cannot be inherited from the parent item due to bad script pack...
									}
								}
							}
							else if ( iDispID )
                            {
								CItemBase::GetItemTiledataFlags(&uiBlockThis, iDispID);
                            }

							block.CheckTile_Item(uiBlockThis, z, zHeight, iDispID + (ITEMID_TYPE)TERRAIN_QTY);
						}
					}
				}
			}
		}
	}

	pItemDef = nullptr;
	pDupeDef = nullptr;
	pItem = nullptr;
	uiBlockThis = 0;
	x2 = y2 = 0;
	iQty = 0;
	zHeight = 0;
	z = 0;

	// Any dynamic items here ?
	// NOTE: This could just be an item that an NPC could just move ?
	auto Area = CWorldSearchHolder::GetInstance( pt );

	for (;;)
	{
		pItem = Area->GetItem();
		if ( !pItem )
			break;

		z = pItem->GetTopZ();

		// Invis items should not block ???
        const ITEMID_TYPE iDispID = pItem->GetDispID();
		pItemDef = CItemBase::FindItemBase( iDispID );

		if ( pItemDef )
		{
			if ( pItemDef->GetDispID() == iDispID )//parent item
			{
				zHeight = pItemDef->GetHeight();
				uiBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK); //Use only Block flags, other remove
			}
			else //non-parent item
			{
				pDupeDef = CItemBaseDupe::GetDupeRef(iDispID);
				if ( ! pDupeDef )
				{
					g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X: %d Y: %d Z: %d)\n",iDispID,pItem->GetTopPoint().m_x,pItem->GetTopPoint().m_y,pItem->GetTopPoint().m_z);
					zHeight = pItemDef->GetHeight();
					uiBlockThis = (pItemDef->m_Can & CAN_I_MOVEMASK);
				}
				else
				{
					zHeight = pDupeDef->GetHeight();
					uiBlockThis = (pDupeDef->m_Can & CAN_I_MOVEMASK); //Use only Block flags, other remove - CAN flags cannot be inherited from the parent item due to bad script pack...
				}
			}
		}
		else if (iDispID)
        {
			CItemBase::GetItemTiledataFlags(&uiBlockThis, iDispID);
        }

        block.CheckTile_Item(uiBlockThis, z, zHeight, iDispID + (ITEMID_TYPE)TERRAIN_QTY);
	}

	uiBlockThis = 0;
	// Terrain height is screwed. Since it is related to all the terrain around it.
	std::optional<CUOMapMeter> pMapTop = GetMapMeterAdjusted(pt); //Get pMapTop Z Adjusted.
	//const CUOMapMeter* pMapTop = pMapBlock->GetTerrain(UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
	if (!pMapTop)
		return;
	//DEBUG_ERR(("pMeter->m_wTerrainIndex 0%x dwBlockThis (0%x)\n",pMeter->m_wTerrainIndex,dwBlockThis));
    if (pMapTop->m_wTerrainIndex == TERRAIN_HOLE)
    {
        uiBlockThis = 0;
    }
    else if (CUOMapMeter::IsTerrainNull(pMapTop->m_wTerrainIndex))	// inter dungeon type.
    {
        uiBlockThis = CAN_I_BLOCK;
    }
    else
    {
        const CUOTerrainInfo land(pMapTop->m_wTerrainIndex);
        //DEBUG_ERR(("Terrain flags - land.m_flags 0%x dwBlockThis (0%x)\n",land.m_flags,dwBlockThis));
        if (land.m_flags & UFLAG1_WATER)
            uiBlockThis |= CAN_I_WATER;
        if (land.m_flags & UFLAG1_DAMAGE)
            uiBlockThis |= CAN_I_FIRE;
        if (land.m_flags & UFLAG1_BLOCK)
            uiBlockThis |= CAN_I_BLOCK;
        if ((!uiBlockThis) || (land.m_flags & UFLAG2_PLATFORM)) // Platform items should take precendence over non-platforms.
            uiBlockThis = CAN_I_PLATFORM;
    }
    //DEBUG_ERR(("TERRAIN dwBlockThis (0%x)\n",dwBlockThis));

    block.CheckTile_Terrain(uiBlockThis, pMapTop->m_z, pMapTop->m_wTerrainIndex);

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
	{
		block.m_Bottom = block.m_Lowest;
		if ( block.m_Top.m_z == block.m_Bottom.m_z )
		{
			block.m_Top.m_uiBlockFlags = 0;
			block.m_Top.m_dwTile = 0;
			block.m_Top.m_z = UO_SIZE_Z;
		}
	}
}

char CWorldMap::GetFloorAvarage(char pPoint1, char pPoint2, short iAverage)
{
	//We can't use char here, because higher points like hills has 64+ heights and adding 64+65 each other exceed char limit and causes returns minus values.
	const short pTotal = pPoint1 + pPoint2, pHalf = pTotal / 2, pEven = pTotal % 2, pAverage = iAverage - pHalf;
	return static_cast<char>(pHalf + (pEven != 0 && pAverage > 5));
}

short CWorldMap::GetAreaAverage(char pTop, char pLeft, char pBottom, char pRight)
{
	const short iHighest1 = maximum(pTop, pBottom);
	const short iLowest1 = minimum(pTop, pBottom);

	const short iHighest2 = maximum(pLeft, pRight);
	const short iLowest2 = minimum(pLeft, pRight);
	return maximum(iHighest1, iHighest2) - minimum(iLowest1, iLowest2);
}

CUOMapMeter CWorldMap::CheckMapTerrain(CUOMapMeter pDefault, short x, short y, uchar map)
{
	CPointMap pt = { x, y, 0, map };
	const CServerMapBlock* pMapBlock = GetMapBlock(pt);
	if (!pMapBlock)
		return pDefault;

	const CUOMapMeter* pMeter = pMapBlock->GetTerrain(UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
	if (!pMeter)
		return pDefault;

	if (CUOMapMeter::IsTerrainNull(pMeter->m_wTerrainIndex))
		return pDefault;
	else
	{
		const CUOTerrainInfo land(pMeter->m_wTerrainIndex, false);
		if ((land.m_flags & UFLAG1_WATER))
			return pDefault;
	}
	return *pMeter;
}

char CWorldMap::GetHeightPoint(const CPointMap & pt, uint64 & uiBlockFlags, bool fHouseCheck) // static
{
	ADDTOCALLSTACK_DEBUG("CWorldMap::GetHeightPoint");
	const uint64 uiCan = uiBlockFlags;
	CServerMapBlockingState block( uiBlockFlags, pt.m_z + (PLAYER_HEIGHT / 2), pt.m_z + PLAYER_HEIGHT );
	GetHeightPoint(pt, block, fHouseCheck);

	// Pass along my results.
	uiBlockFlags = block.m_Bottom.m_uiBlockFlags;
	if ( block.m_Top.m_uiBlockFlags )
		uiBlockFlags |= CAN_I_ROOF;	// we are covered by something.

	if ((block.m_Lowest.m_uiBlockFlags & CAN_I_HOVER) || (block.m_Bottom.m_uiBlockFlags & CAN_I_HOVER) || (block.m_Top.m_uiBlockFlags & CAN_I_HOVER))
	{
		if (uiCan & CAN_C_HOVER)
			uiBlockFlags = 0; // we can hover over this
		else
			uiBlockFlags &= ~CAN_I_HOVER; // we don't have the ability to fly
	}

	if ((uiBlockFlags & ( CAN_I_CLIMB|CAN_I_PLATFORM)) && (uiCan & CAN_C_WALK))
	{
		uiBlockFlags &= ~CAN_I_CLIMB;
		uiBlockFlags |= CAN_I_PLATFORM;	// not really true but hack it anyhow.
		//DEBUG_MSG(("block.m_Bottom.m_z (%d)\n",block.m_Bottom.m_z));
		return block.m_Bottom.m_z;
	}
	if (uiCan & CAN_C_FLY)
		return pt.m_z;

	return block.m_Bottom.m_z;
}

void CWorldMap::GetHeightPoint2( const CPointMap & pt, CServerMapBlockingState & block, bool fHouseCheck ) // static
{
    EXC_TRYSUB("GHP2 with blockFlags");

	//ADDTOCALLSTACK_DEBUG("CWorldMap::GetHeightPoint2(blockingState)");
	// Height of statics at/above given coordinates
	// do gravity here for the z.

	const CServerMapBlock * pMapBlock = GetMapBlock( pt );
	if ( !pMapBlock )
	{
		g_Log.EventWarn("GetMapBlock failed at %s.\n", pt.WriteUsed());
		return;
	}

    uint64 uiBlockThis = 0;
	const uint iStaticQty = pMapBlock->m_Statics.GetStaticQty();
	if ( iStaticQty > 0 )  // no static items here.
	{
		int x2 = pMapBlock->GetOffsetX(pt.m_x);
		int y2 = pMapBlock->GetOffsetY(pt.m_y);
		for ( uint i = 0; i < iStaticQty; ++i )
		{
			if ( ! pMapBlock->m_Statics.IsStaticPoint( i, x2, y2 ))
				continue;
			const CUOStaticItemRec * pStatic = pMapBlock->m_Statics.GetStatic( i );

			char z = pStatic->m_z;
            uiBlockThis = 0;
            const ITEMID_TYPE iDispID = pStatic->GetDispID();
            height_t zHeight = CItemBase::GetItemHeight(iDispID, &uiBlockThis);

			// This static is at the coordinates in question.
			// enough room for me to stand here ?
			block.CheckTile(uiBlockThis, z, zHeight, iDispID + (ITEMID_TYPE)TERRAIN_QTY);
	    }
    }

	// Any multi items here ?
	if ( fHouseCheck )
	{
		static thread_local CRegionLinks rlinks;
		size_t iRegionQty = pt.GetRegions( REGION_TYPE_MULTI, &rlinks );
		if ( iRegionQty > 0 )
		{
			for ( size_t i = 0; i < iRegionQty; ++i)
			{
                const CRegion * pRegion = rlinks[i];
				CItem * pItem = pRegion->GetResourceID().ItemFindFromResource();
				if ( pItem != nullptr )
				{
					const CUOMulti * pMulti = g_Cfg.GetMultiItemDefs(pItem);
					if ( pMulti )
					{
                        const CPointMap& ptItemTop = pItem->GetTopPoint();
						int x2 = pt.m_x - ptItemTop.m_x;
						int y2 = pt.m_y - ptItemTop.m_y;

						uint iMultiQty = pMulti->GetItemCount();
						for ( size_t j = 0; j < iMultiQty; ++j )
						{
							const CUOMultiItemRec_HS * pMultiItem = pMulti->GetItem(j);
							ASSERT(pMultiItem);

							if ( ! pMultiItem->m_visible )
								continue;
							if ( pMultiItem->m_dx != x2 || pMultiItem->m_dy != y2 )
								continue;

							char zitem = (char)( pItem->GetTopZ() + pMultiItem->m_dz );
                            uiBlockThis = 0;
                            const ITEMID_TYPE iDispID = pMultiItem->GetDispID();
                            height_t zHeight = CItemBase::GetItemHeight( iDispID, &uiBlockThis );

							block.CheckTile(uiBlockThis, zitem, zHeight, iDispID + (ITEMID_TYPE)TERRAIN_QTY);
						}
					}
				}
			}
		}
        rlinks.clear();
    }

	{
	    // Any dynamic items here ?
	    // NOTE: This could just be an item that an NPC could just move ?
	    auto Area = CWorldSearchHolder::GetInstance( pt );
	    for (;;)
	    {
		    const CItem * pItem = Area->GetItem();
		    if ( pItem == nullptr )
			    break;

		    char zitem = pItem->GetTopZ();

            // Invis items should not block ???
		    const CItemBase * pItemDef = pItem->Item_GetDef();
		    ASSERT(pItemDef);

		    // Get Attributes from ItemDef. If they are not set, get them from the static object (DISPID)
		    uiBlockThis = pItemDef->Can(CAN_I_DOOR | CAN_I_WATER | CAN_I_CLIMB | CAN_I_BLOCK | CAN_I_PLATFORM);
		    height_t zHeight = pItemDef->GetHeight();

		    uint64 uiStaticBlockThis = 0;
		    height_t zStaticHeight = CItemBase::GetItemHeight(pItem->GetDispID(), &uiStaticBlockThis);

		    if (uiBlockThis == 0)
			    uiBlockThis = uiStaticBlockThis;
		    if (zHeight == 0)
			    zHeight = zStaticHeight;

		    if ( !block.CheckTile(uiBlockThis, zitem, zHeight, pItemDef->GetDispID() + (ITEMID_TYPE)TERRAIN_QTY ) )
		    {
		    }
	    }
	}

	// Check Terrain here.
	// Terrain height is screwed. Since it is related to all the terrain around it.

	{
        const CUOMapMeter * pMeter = pMapBlock->GetTerrain(UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y));
        ASSERT(pMeter);

        if (pMeter->m_wTerrainIndex == TERRAIN_HOLE)
            uiBlockThis = 0;
        else if (pMeter->m_wTerrainIndex == TERRAIN_NULL)	// inter dungeon type.
            uiBlockThis = CAN_I_BLOCK;
        else
        {
            const CUOTerrainInfo land(pMeter->m_wTerrainIndex);
            if (land.m_flags & UFLAG2_PLATFORM) // Platform items should take precendence over non-platforms.
                uiBlockThis = CAN_I_PLATFORM;
            else if (land.m_flags & UFLAG1_WATER)
                uiBlockThis = CAN_I_WATER;
            else if (land.m_flags & UFLAG1_DAMAGE)
                uiBlockThis = CAN_I_FIRE;
            else if (land.m_flags & UFLAG1_BLOCK)
                uiBlockThis = CAN_I_BLOCK;
            else
                uiBlockThis = CAN_I_PLATFORM;
        }
        block.CheckTile(uiBlockThis, pMeter->m_z, 0, pMeter->m_wTerrainIndex);
	}

	if ( block.m_Bottom.m_z == UO_SIZE_MIN_Z )
	{
		block.m_Bottom = block.m_Lowest;
	}

    EXC_CATCHSUB("GHP2 with blockFlags");
}

// Height of player who walked to X/Y/OLDZ
char CWorldMap::GetHeightPoint2( const CPointMap & pt, uint64 & uiBlockFlags, bool fHouseCheck ) // static
{
    EXC_TRYSUB("GHP2 with blockFlags");
    //ADDTOCALLSTACK_DEBUG("CWorldMap::GetHeightPoint2(blockFlags)");
	// Given our coords at pt including pt.m_z
	// What is the height that gravity would put me at should i step here ?
	// Assume my head height is PLAYER_HEIGHT/2
	// ARGS:
	//  pt = the point of interest.
	//  pt.m_z = my starting altitude.
	//  dwBlockFlags = what we can pass thru. doors, water, walls ?
	//		CAN_C_GHOST	= Moves thru doors etc.	- CAN_I_DOOR
	//		CAN_C_SWIM = walk thru water - CAN_I_WATER
	//		CAN_C_WALK = it is possible that i can step up. - CAN_I_PLATFORM
	//		CAN_C_PASSWALLS	= walk through all blocking items - CAN_I_BLOCK
	//		CAN_C_FLY  = gravity does not effect me. - CAN_I_CLIMB
	//		CAN_C_FIRE_IMMUNE = i can walk into lava etc. - CAN_I_FIRE
	//  pRegion = possible regional effects. (multi's)
	// RETURN:
	//  pt.m_z = Our new height at pt.m_x,pt.m_y
	//  dwBlockFlags = our blocking flags at the given location. CAN_I_WATER,CAN_C_WALK,CAN_FLY,CAN_SPIRIT,
	//    CAN_C_INDOORS = i am covered from the sky
	//

	// ??? NOTE: some creatures should be taller than others !!!

	const uint64 uiCan = uiBlockFlags;
	CServerMapBlockingState block(uiBlockFlags, pt.m_z, PLAYER_HEIGHT);
	GetHeightPoint2( pt, block, fHouseCheck );

	// Pass along my results.
	uiBlockFlags = block.m_Bottom.m_uiBlockFlags;
	if (block.m_Top.m_uiBlockFlags)
	{
		uiBlockFlags |= CAN_I_ROOF;	// we are covered by something.

		// Do not check for landtiles to block me. We pass through if statics are under them
		if (block.m_Top.m_dwTile > TERRAIN_QTY)
		{
			// If this tile possibly blocks me, roof cannot block me
			if (block.m_Top.m_uiBlockFlags & (~CAN_I_ROOF))
			{
				if (block.m_Top.m_z < block.m_Bottom.m_z + PLAYER_HEIGHT)
					uiBlockFlags |= CAN_I_BLOCK; // we can't fit under this!
			}
		}
	}

	if ((block.m_Lowest.m_uiBlockFlags & CAN_I_HOVER) || (block.m_Bottom.m_uiBlockFlags & CAN_I_HOVER) || (block.m_Top.m_uiBlockFlags & CAN_I_HOVER))
	{
		if (uiCan & CAN_C_HOVER)
			uiBlockFlags = 0; // we can hover over this
		else
			uiBlockFlags &= ~CAN_I_HOVER; // we don't have the ability to fly
	}

	if ((uiBlockFlags & (CAN_I_CLIMB|CAN_I_PLATFORM)) && (uiCan & CAN_C_WALK))
	{
		uiBlockFlags &= ~CAN_I_CLIMB;
		uiBlockFlags |= CAN_I_PLATFORM;	// not really true but hack it anyhow.
		return( block.m_Bottom.m_z );
	}

	if (uiCan & CAN_C_FLY)
		return( pt.m_z );

	return( block.m_Bottom.m_z );

    EXC_CATCHSUB("GHP2 with blockFlags");
    return UO_SIZE_MIN_Z;
}

