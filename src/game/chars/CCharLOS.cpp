
#include <cmath>
#include "../uo_files/CUOTerrainInfo.h"
#include "CChar.h"


bool CChar::CanSeeLOS( const CPointMap &ptDst, CPointMap *pptBlock, int iMaxDist, word wFlags, bool bCombatCheck ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS");
	// WARNING: CanSeeLOS is an expensive function (lot of calculations but	most importantly it has to read the UO files, and file I/O is slow).

	if ( (m_pPlayer && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER)) || (m_pNPC && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_NPC)) )
		return CanSeeLOS_New(ptDst, pptBlock, iMaxDist, wFlags);

	// Max distance of iMaxDist
	// Line of sight check
	// NOTE: if not blocked. pptBlock is undefined.
	// 3D LOS later - real LOS, i.e. we can't shoot through the floor, but can shoot through the hole in it

	if ( !bCombatCheck && IsPriv(PRIV_GM) )	// If i'm checking the LOS during a combat, i don't want to shoot through the walls even if i'm a GM
		return true;

	CPointMap ptSrc = GetTopPoint();
	int iDist = ptSrc.GetDist(ptDst);
	if ( iDist > iMaxDist )
	{
	blocked:
		if ( pptBlock )
			*pptBlock = ptSrc;
		return false;	// blocked
	}

	// Walk towards the object. If any spot is too far above our heads then we can not see what's behind it.
	int iDistTry = 0;
	while ( --iDist >= 0 )
	{
		DIR_TYPE dir = ptSrc.GetDir(ptDst);
		dword dwBlockFlags;
		if ( dir % 2 && !IsSetEF(EF_NoDiagonalCheckLOS) )	// test only diagonal dirs
		{
			CPointMap ptTest = ptSrc;
			DIR_TYPE dirTest1 = static_cast<DIR_TYPE>(dir - 1);	// get 1st ortogonal
			DIR_TYPE dirTest2 = static_cast<DIR_TYPE>(dir + 1);	// get 2nd ortogonal
			if ( dirTest2 == DIR_QTY )		// roll over
				dirTest2 = DIR_N;

			ptTest.Move(dirTest1);
			dwBlockFlags = CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY;
			char z = g_World.GetHeightPoint2(ptTest, dwBlockFlags, true);
			char zDiff = (char)(abs(z - ptTest.m_z));

			if ( (zDiff > PLAYER_HEIGHT) || (dwBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR)) )		// blocked
			{
				ptTest = ptSrc;
				ptTest.Move(dirTest2);
				{
					dwBlockFlags = CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY;
					z = g_World.GetHeightPoint2(ptTest, dwBlockFlags, true);
					zDiff = (char)(abs(z - ptTest.m_z));
					if ( zDiff > PLAYER_HEIGHT )
						goto blocked;

					if ( dwBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR) )
					{
						ptSrc = ptTest;
						goto blocked;
					}
				}
			}
			ptTest.m_z = z;
		}

		if ( iDist )
		{
			ptSrc.Move(dir);	// NOTE: The dir is very coarse and can change slightly.
			dwBlockFlags = CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY;
			char z = g_World.GetHeightPoint2(ptSrc, dwBlockFlags, true);
			char zDiff = (char)(abs(z - ptSrc.m_z));

			if ( (zDiff > PLAYER_HEIGHT) || (dwBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR)) || (iDistTry > iMaxDist) )
				goto blocked;

			ptSrc.m_z = z;
			++iDistTry;
		}
	}

	if ( abs(ptSrc.m_z - ptDst.m_z) >= 20 )
		return false;
	return true;	// made it all the way to the object with no obstructions.
}

// a - gradient < x < b + gradient
#define BETWEENPOINT(coord, coordt, coords) ( (coord > ((double)minimum(coordt, coords) - 0.5)) && (coord < ((double)maximum(coordt, coords) + 0.5)) )
#define APPROX(num) ((double)((num - floor(num)) > 0.5)? ceil(num) : floor(num))
//#define CALCITEMHEIGHT(num) num + ((pItemDef->GetTFlags() & 0x400)? pItemDef->GetHeight() / 2 : pItemDef->GetHeight())
#define WARNLOS(_x_)		if ( g_Cfg.m_iDebugFlags & DEBUGF_LOS ) { g_pLog->EventWarn _x_; }

bool inline CChar::CanSeeLOS_New_Failed( CPointMap *pptBlock, CPointMap &ptNow ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS_New_Failed");
	if ( pptBlock )
		*pptBlock = ptNow;
	return false;
}

bool CChar::CanSeeLOS_New( const CPointMap &ptDst, CPointMap *pptBlock, int iMaxDist, word flags, bool bCombatCheck ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS_New");
	// WARNING: CanSeeLOS is an expensive function (lot of calculations but	most importantly it has to read the UO files, and file I/O is slow).

	if ( !bCombatCheck && IsPriv(PRIV_GM) )	// If i'm checking the LOS during a combat, i don't want to shoot through the walls even if i'm a GM
	{
		WARNLOS(("GM Pass\n"));
		return true;
	}

	CPointMap ptSrc = GetTopPoint();
	CPointMap ptNow(ptSrc);

	if ( ptSrc.m_map != ptDst.m_map )	// Different map
		return this->CanSeeLOS_New_Failed(pptBlock, ptNow);

	if ( ptSrc == ptDst )	// Same point ^^
		return true;

	char iTotalZ = ptSrc.m_z + GetHeightMount(true);
	ptSrc.m_z = minimum(iTotalZ, UO_SIZE_Z);	//true - substract one from the height because of eyes height
	WARNLOS(("Total Z: %d\n", ptSrc.m_z));

	int dx, dy, dz;
	dx = ptDst.m_x - ptSrc.m_x;
	dy = ptDst.m_y - ptSrc.m_y;
	dz = ptDst.m_z - ptSrc.m_z;

	double dist2d, dist3d;
	dist2d = sqrt(static_cast<double>(dx*dx + dy*dy));
	if ( dz )
		dist3d = sqrt(static_cast<double>(dist2d*dist2d + dz*dz));
	else
		dist3d = dist2d;

	if ( APPROX(dist2d) > static_cast<double>(iMaxDist) )
	{
		WARNLOS(("( APPROX(dist2d)(%f) > ((double)iMaxDist)(%f) ) --> NOLOS\n", APPROX(dist2d), (double)iMaxDist));
		return CanSeeLOS_New_Failed(pptBlock, ptNow);
	}

	double dFactorX, dFactorY, dFactorZ;
	dFactorX = dx / dist3d;
	dFactorY = dy / dist3d;
	dFactorZ = dz / dist3d;

	double nPx, nPy, nPz;
	nPx = ptSrc.m_x;
	nPy = ptSrc.m_y;
	nPz = ptSrc.m_z;

	std::vector<CPointMap> path;
	for (;;)
	{
		if ( BETWEENPOINT(nPx, ptDst.m_x, ptSrc.m_x) && BETWEENPOINT(nPy, ptDst.m_y, ptSrc.m_y) && BETWEENPOINT(nPz, ptDst.m_z, ptSrc.m_z) )
		{
			dx = (int)APPROX(nPx);
			dy = (int)APPROX(nPy);
			dz = (int)APPROX(nPz);

			// Add point to vector
			if ( !path.empty() )
			{
				CPointMap ptEnd = path[path.size() - 1];
				if ( ptEnd.m_x != dx || ptEnd.m_y != dy || ptEnd.m_z != dz )
					path.push_back(CPointMap((word)dx, (word)dy, (char)dz, ptSrc.m_map));
			}
			else
			{
				path.push_back(CPointMap((word)dx, (word)dy, (char)dz, ptSrc.m_map));
			}
			WARNLOS(("PATH X:%d Y:%d Z:%d\n", dx, dy, dz));

			nPx += dFactorX;
			nPy += dFactorY;
			nPz += dFactorZ;
		}
		else
			break;
	}

	if ( !path.empty() )
	{
		if ( path[path.size() - 1] != ptDst )
			path.push_back(CPointMap(ptDst.m_x, ptDst.m_y, ptDst.m_z, ptDst.m_map));
	}
	else
	{
		path.clear();
		return CanSeeLOS_New_Failed(pptBlock, ptNow);
	}

	WARNLOS(("Path calculated %" PRIuSIZE_T "\n", path.size()));
	// Ok now we should loop through all the points and checking for maptile, staticx, items, multis.
	// If something is in the way and it has the wrong flags LOS return false

	const CServerMapBlock *pBlock			= NULL;		// Block of the map (for statics)
	const CUOStaticItemRec *pStatic			= NULL;		// Statics iterator (based on SphereMapBlock)
	const CSphereMulti *pMulti 				= NULL;		// Multi Def (multi check)
	const CUOMultiItemRec_HS *pMultiItem	= NULL;		// Multi item iterator
	CRegion *pRegion					= NULL;		// Nulti regions
	CRegionLinks rlinks;								// Links to multi regions
	CItem *pItem						= NULL;
	CItemBase *pItemDef 				= NULL;
	CItemBaseDupe *pDupeDef				= NULL;

	dword wTFlags = 0;
	height_t Height = 0;
	word terrainid = 0;
	bool bPath = true;
	bool bNullTerrain = false;

	CRegion *pSrcRegion = ptSrc.GetRegion(REGION_TYPE_AREA|REGION_TYPE_ROOM|REGION_TYPE_MULTI);
	CRegion *pNowRegion = NULL;

	int lp_x = 0, lp_y = 0;
	char min_z = 0, max_z = 0;

	for (size_t i = 0; i < path.size(); lp_x = ptNow.m_x, lp_y = ptNow.m_y, pItemDef = NULL, pStatic = NULL, pMulti = NULL, pMultiItem = NULL, min_z = 0, max_z = 0, ++i )
	{
		ptNow = path[i];
		WARNLOS(("---------------------------------------------\n"));
		WARNLOS(("Point %d,%d,%d \n", ptNow.m_x, ptNow.m_y, ptNow.m_z));

		pNowRegion = ptNow.GetRegion(REGION_TYPE_AREA|REGION_TYPE_ROOM|REGION_TYPE_MULTI);

		if ( (flags & LOS_NO_OTHER_REGION) && (pSrcRegion != pNowRegion) )
		{
			WARNLOS(("flags & 0200 and path is leaving my region - BLOCK\n"));
			bPath = false;
			break;
		}

		if ( (flags & LOS_NC_MULTI) && ptNow.GetRegion(REGION_TYPE_MULTI) && (ptNow.GetRegion(REGION_TYPE_MULTI) != ptSrc.GetRegion(REGION_TYPE_MULTI)) )
		{
			WARNLOS(("flags & 0400 and path is crossing another multi - BLOCK\n"));
			bPath = false;
			break;
		}

		if ( (lp_x != ptNow.m_x) || (lp_y != ptNow.m_y) )
		{
			WARNLOS(("\tLoading new map block.\n"));
			pBlock = g_World.GetMapBlock(ptNow);
		}

		if ( !pBlock ) // something is wrong
		{
			WARNLOS(("GetMapBlock Failed\n"));
			bPath = false;
			break;
		}

		if ( !(flags & LOS_NB_TERRAIN) )
		{
			if ( !((flags & LOS_NB_LOCAL_TERRAIN) && (pSrcRegion == pNowRegion)) )
			{
				// ------ MapX.mul Check ----------
				terrainid = pBlock->GetTerrain(UO_BLOCK_OFFSET(ptNow.m_x), UO_BLOCK_OFFSET(ptNow.m_y))->m_wTerrainIndex;
				WARNLOS(("Terrain %d\n", terrainid));

				if ( (flags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (g_World.GetTerrainItemType(terrainid) != IT_WATER) && (g_World.GetTerrainItemType(terrainid) != IT_NORMAL) )
				{
					WARNLOS(("Terrain %d blocked - flags & LOS_FISHING, distance >= 2 and type of pItemDef is not IT_WATER\n", terrainid));
					WARNLOS(("ptSrc: %d,%d,%d; ptNow: %d,%d,%d; terrainid: %d; terrainid type: %d\n", ptSrc.m_x, ptSrc.m_y, ptSrc.m_z, ptNow.m_x, ptNow.m_y, ptNow.m_z, terrainid, g_World.GetTerrainItemType(terrainid)));
					bPath = false;
					break;
				}

				//#define MAPTILEMIN minimum(minimum(minimum(pBlock->GetTerrain(0,0)->m_z, pBlock->GetTerrain(0,1)->m_z), pBlock->GetTerrain(1,0)->m_z), pBlock->GetTerrain(1,1)->m_z)
				//#define MAPTILEMAX maximum(maximum(maximum(pBlock->GetTerrain(0,0)->m_z, pBlock->GetTerrain(0,1)->m_z), pBlock->GetTerrain(1,0)->m_z), pBlock->GetTerrain(1,1)->m_z);
				//#define MAPTILEZ pBlock->GetTerrain(UO_BLOCK_OFFSET(ptNow.m_x), UO_BLOCK_OFFSET(ptNow.m_y))->m_z;

				if ( (terrainid != TERRAIN_HOLE) && (terrainid != 475) )
				{
					if ( terrainid < 430 || terrainid > 437 )
					{
						/*this stuff should do some checking for surrounding items:
						aaa
						aXa
						aaa
						min_z is determined as a minimum of all a/X terrain, where X is ptNow
						*/
						byte pos_x = UO_BLOCK_OFFSET(ptNow.m_x) > 1 ? UO_BLOCK_OFFSET(ptNow.m_x - 1) : 0;
						byte pos_y = UO_BLOCK_OFFSET(ptNow.m_y) > 1 ? UO_BLOCK_OFFSET(ptNow.m_y - 1) : 0;
						const byte defx = UO_BLOCK_OFFSET(ptNow.m_x);
						const byte defy = UO_BLOCK_OFFSET(ptNow.m_y);
						min_z = pBlock->GetTerrain(pos_x, pos_y)->m_z;
						max_z = pBlock->GetTerrain(defx, defy)->m_z;
						for ( byte posy = pos_y; (abs(defx - UO_BLOCK_OFFSET(pos_x)) <= 1 && pos_x <= 7); ++pos_x )
						{
							for ( pos_y = posy; (abs(defy - UO_BLOCK_OFFSET(pos_y)) <= 1 && pos_y <= 7); ++pos_y )
							{
								char terrain_z = pBlock->GetTerrain(pos_x, pos_y)->m_z;
								min_z = minimum(min_z, terrain_z);
							}
						}
						//min_z = MAPTILEZ;
						//max_z = MAPTILEZ;
						WARNLOS(("Terrain %d - m:%d M:%d\n", terrainid, min_z, max_z));
						if ( CUOMapMeter::IsTerrainNull(terrainid) )
							bNullTerrain = true; //what if there are some items on that hole?
						if ( (min_z <= ptNow.m_z && max_z >= ptNow.m_z) && (ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z) )
						{
							WARNLOS(("Terrain %d - m:%d M:%d - block\n", terrainid, min_z, max_z));
							bPath = false;
							break;
						}
						CUOTerrainInfo land(terrainid);
						if ( (land.m_flags & UFLAG1_WATER) && (flags & LOS_NC_WATER) )
							bNullTerrain = true;
					}
				}
				//#undef MAPTILEMIN
				//#undef MAPTILEMAX
				//#undef MAPTILEZ
			}
		}

		// ------ StaticsX.mul Check --------
		if ( !(flags & LOS_NB_STATIC) )
		{
			if ( !((flags & LOS_NB_LOCAL_STATIC) && (pSrcRegion == pNowRegion)) )
			{
				for ( size_t s = 0; s < pBlock->m_Statics.GetStaticQty(); pStatic = NULL, pItemDef = NULL, ++s )
				{
					pStatic = pBlock->m_Statics.GetStatic(s);
					if ( (pStatic->m_x + pBlock->m_x != ptNow.m_x) || (pStatic->m_y + pBlock->m_y != ptNow.m_y) )
						continue;

					//Fix for Stacked items blocking view
					if ( (pStatic->m_x == ptDst.m_x) && (pStatic->m_y == ptDst.m_y) && (pStatic->m_z >= GetTopZ()) && (pStatic->m_z <= ptSrc.m_z) )
						continue;

					pItemDef = CItemBase::FindItemBase(pStatic->GetDispID());
					wTFlags = 0;
					Height = 0;
					bNullTerrain = false;

					if ( !pItemDef )
					{
						WARNLOS(("STATIC - Cannot get pItemDef for item (0%x)\n", pStatic->GetDispID()));
					}
					else
					{
						if ( (flags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (pItemDef->GetType() != IT_WATER) &&
							( pItemDef->Can(CAN_I_DOOR | CAN_I_PLATFORM | CAN_I_BLOCK | CAN_I_CLIMB | CAN_I_FIRE | CAN_I_ROOF) || (pItemDef->m_Can & CAN_I_BLOCKLOS) ) )
						{
							WARNLOS(("pStatic blocked - flags & 0800, distance >= 2 and type of pItemDef is not IT_WATER\n"));
							bPath = false;
							break;
						}

						wTFlags = pItemDef->GetTFlags();
						Height = pItemDef->GetHeight();

						if ( pItemDef->GetID() != pStatic->GetDispID() ) //not a parent item
						{
							WARNLOS(("Not a parent item (STATIC)\n"));
							pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE)(pStatic->GetDispID()));
							if ( !pDupeDef )
							{
								g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X: %d Y: %d Z: %d)\n", pStatic->GetDispID(), ptNow.m_x, ptNow.m_y, pStatic->m_z);
								wTFlags = pItemDef->GetTFlags();
								Height = pItemDef->GetHeight();
							}
							else
							{
								wTFlags = pDupeDef->GetTFlags();
								Height = pDupeDef->GetHeight();
							}
						}
						else
						{
							WARNLOS(("Parent item (STATIC)\n"));
						}

						Height = (wTFlags & UFLAG2_CLIMBABLE) ? Height / 2 : Height;

						if ( ((wTFlags & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) && !((wTFlags & UFLAG2_WINDOW) && (flags & LOS_NB_WINDOWS)) )
						{
							WARNLOS(("pStatic %0x %d,%d,%d - %d\n", pStatic->GetDispID(), pStatic->m_x, pStatic->m_y, pStatic->m_z, Height));
							min_z = pStatic->m_z;
							max_z = minimum(Height + min_z, UO_SIZE_Z);
							WARNLOS(("wTFlags(0%x)\n", wTFlags));

							WARNLOS(("pStatic %0x Z check: %d,%d (Now: %d) (Dest: %d).\n", pStatic->GetDispID(), min_z, max_z, ptNow.m_z, ptDst.m_z));
							if ( (min_z <= ptNow.m_z) && (max_z >= ptNow.m_z) )
							{
								if ( ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z )
								{
									WARNLOS(("pStatic blocked - m:%d M:%d\n", min_z, max_z));
									bPath = false;
									break;
								}
							}
						}
					}
				}
			}
		}

		if ( !bPath )
			break;

		// --------- In game items ----------
		if ( !(flags & LOS_NB_DYNAMIC) )
		{
			if ( !((flags & LOS_NB_LOCAL_DYNAMIC) && (pSrcRegion == pNowRegion)) )
			{
				CWorldSearch AreaItems(ptNow, 0);
				for (;;)
				{
					pItem = AreaItems.GetItem();
					if ( !pItem )
						break;
					if ( pItem->GetUnkPoint().m_x != ptNow.m_x || pItem->GetUnkPoint().m_y != ptNow.m_y )
						continue;
					if ( !CanSeeItem(pItem) )
						continue;

					//Fix for Stacked items blocking view
					if ( (pItem->GetUnkPoint().m_x == ptDst.m_x) && (pItem->GetUnkPoint().m_y == ptDst.m_y) && (pItem->GetUnkPoint().m_z >= GetTopZ()) && (pItem->GetUnkPoint().m_z <= ptSrc.m_z) )
						continue;

					pItemDef = CItemBase::FindItemBase(pItem->GetDispID());
					wTFlags = 0;
					Height = 0;
					bNullTerrain = false;

					if ( !pItemDef )
					{
						WARNLOS(("DYNAMIC - Cannot get pItemDef for item (0%x)\n", pItem->GetDispID()));
					}
					else
					{
						if ( (flags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (pItemDef->GetType() != IT_WATER) &&
							( pItemDef->Can(CAN_I_DOOR | CAN_I_PLATFORM | CAN_I_BLOCK | CAN_I_CLIMB | CAN_I_FIRE | CAN_I_ROOF) || (pItemDef->m_Can & CAN_I_BLOCKLOS) ) )
						{
							WARNLOS(("pItem blocked - flags & 0800, distance >= 2 and type of pItemDef is not IT_WATER\n"));
							bPath = false;
							break;
							//return CanSeeLOS_New_Failed(pptBlock, ptNow);
						}

						wTFlags = pItemDef->GetTFlags();
						Height = pItemDef->GetHeight();

						if ( pItemDef->GetID() != pItem->GetDispID() )	//not a parent item
						{
							WARNLOS(("Not a parent item (DYNAMIC)\n"));
							pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE)(pItem->GetDispID()));
							if ( !pDupeDef )
							{
								g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X: %d Y: %d Z: %d)\n", pItem->GetDispID(), ptNow.m_x, ptNow.m_y, pItem->GetUnkZ());
								wTFlags = pItemDef->GetTFlags();
								Height = pItemDef->GetHeight();
							}
							else
							{
								wTFlags = pDupeDef->GetTFlags();
								Height = pDupeDef->GetHeight();
							}
						}
						else
						{
							WARNLOS(("Parent item (DYNAMIC)\n"));
						}

						Height = (wTFlags & UFLAG2_CLIMBABLE) ? Height / 2 : Height;

						if ( ((wTFlags & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) || pItemDef->m_Can & CAN_I_BLOCKLOS) && !((wTFlags & UFLAG2_WINDOW) && (flags & LOS_NB_WINDOWS)) )
						{
							WARNLOS(("pItem %0x(%0x) %d,%d,%d - %d\n", (dword)pItem->GetUID(), pItem->GetDispID(), pItem->GetUnkPoint().m_x, pItem->GetUnkPoint().m_y, pItem->GetUnkPoint().m_z, Height));
							min_z = pItem->GetUnkPoint().m_z;
							max_z = minimum(Height + min_z, UO_SIZE_Z);
							WARNLOS(("wTFlags(0%x)\n", wTFlags));

							WARNLOS(("pItem %0x(%0x) Z check: %d,%d (Now: %d) (Dest: %d).\n", (dword)pItem->GetUID(), pItem->GetDispID(), min_z, max_z, ptNow.m_z, ptDst.m_z));
							if ( min_z <= ptNow.m_z && max_z >= ptNow.m_z )
							{
								if ( ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z )
								{
									WARNLOS(("pItem blocked - m:%d M:%d\n", min_z, max_z));
									bPath = false;
									break;
								}
							}
						}
					}
				}
			}
		}

		if ( !bPath )
			break;

		// ----------- Multis ---------------

		if ( !(flags & LOS_NB_MULTI) )
		{
			if ( !((flags & LOS_NB_LOCAL_MULTI) && (pSrcRegion == pNowRegion)) )
			{
				size_t iQtyr = ptNow.GetRegions(REGION_TYPE_MULTI, rlinks);
				if ( iQtyr > 0 )
				{
					for ( size_t ii = 0; ii < iQtyr; pMulti = NULL, ++ii, pItem = NULL, pRegion = NULL )
					{
						pRegion = rlinks.GetAt(ii);
						if ( pRegion )
							pItem = pRegion->GetResourceID().ItemFind();

						if ( !pItem )
							continue;

						pMulti = g_Cfg.GetMultiItemDefs(pItem);
						if ( !pMulti )
							continue;

						size_t iQty = pMulti->GetItemCount();
						for ( size_t iii = 0; iii < iQty; pItemDef = NULL, pMultiItem = NULL, ++iii )
						{
							pMultiItem = pMulti->GetItem(iii);
							if ( !pMultiItem )
								break;
							if ( !pMultiItem->m_visible )
								continue;
							if ( (pMultiItem->m_dx + pItem->GetTopPoint().m_x != ptNow.m_x) || (pMultiItem->m_dy + pItem->GetTopPoint().m_y != ptNow.m_y) )
								continue;

							pItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
							wTFlags = 0;
							Height = 0;
							bNullTerrain = false;

							if ( !pItemDef )
							{
								WARNLOS(("MULTI - Cannot get pItemDef for item (0%x)\n", pMultiItem->GetDispID()));
							}
							else
							{
								if ( (flags & LOS_FISHING) && (ptSrc.GetDist(ptNow) >= 2) && (pItemDef->GetType() != IT_WATER) &&
									( pItemDef->Can(CAN_I_DOOR | CAN_I_PLATFORM | CAN_I_BLOCK | CAN_I_CLIMB | CAN_I_FIRE | CAN_I_ROOF) || (pItemDef->m_Can & CAN_I_BLOCKLOS) ) )
								{
									WARNLOS(("pMultiItem blocked - flags & 0800, distance >= 2 and type of pItemDef is not IT_WATER\n"));
									bPath = false;
									break;
									//return CanSeeLOS_New_Failed(pptBlock, ptNow);
								}

								wTFlags = pItemDef->GetTFlags();
								Height = pItemDef->GetHeight();

								if ( pItemDef->GetID() != pMultiItem->GetDispID() ) //not a parent item
								{
									WARNLOS(("Not a parent item (MULTI)\n"));
									pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE)(pMultiItem->GetDispID()));
									if ( !pDupeDef )
									{
										g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X: %d Y: %d Z: %d)\n", pMultiItem->GetDispID(), ptNow.m_x, ptNow.m_y, pMultiItem->m_dz + pItem->GetTopPoint().m_z);
										wTFlags = pItemDef->GetTFlags();
										Height = pItemDef->GetHeight();
									}
									else
									{
										wTFlags = pDupeDef->GetTFlags();
										Height = pDupeDef->GetHeight();
									}
								}
								else
								{
									WARNLOS(("Parent item (MULTI)\n"));
								}

								Height = (wTFlags & UFLAG2_CLIMBABLE) ? Height / 2 : Height;

								if ( ((wTFlags & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) || (pItemDef->m_Can & CAN_I_BLOCKLOS)) && !((wTFlags & UFLAG2_WINDOW) && (flags & LOS_NB_WINDOWS)) )
								{
									WARNLOS(("pMultiItem %0x %d,%d,%d - %d\n", pMultiItem->GetDispID(), pMultiItem->m_dx, pMultiItem->m_dy, pMultiItem->m_dz, Height));
									min_z = (char)(pMultiItem->m_dz) + pItem->GetTopPoint().m_z;
									max_z = minimum(Height + min_z, UO_SIZE_Z);
									WARNLOS(("wTFlags(0%x)\n", wTFlags));

									if ( min_z <= ptNow.m_z && max_z >= ptNow.m_z )
									{
										if ( ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z )
										{
											WARNLOS(("pMultiItem blocked - m:%d M:%d\n", min_z, max_z));
											bPath = false;
											break;
										}
									}
								}
							}
						}

						if ( !bPath )
							break;
					}
				}
			}
		}

		if ( bNullTerrain )
			bPath = false;

		if ( !bPath )
			break;
	}

	path.clear();

	if ( !bPath )
		return CanSeeLOS_New_Failed(pptBlock, ptNow);

	return true;
}

#undef WARNLOS
#undef APPROX
#undef BETWEENPOINT
//#undef CALCITEMHEIGHT

bool CChar::CanSeeLOS( const CObjBaseTemplate *pObj, word wFlags, bool bCombatCheck ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS");
	// WARNING: CanSeeLOS is an expensive function (lot of calculations but	most importantly it has to read the UO files, and file I/O is slow).

	if ( !CanSee(pObj) )
		return false;

	pObj = pObj->GetTopLevelObj();
	if ( !pObj )
		return false;

	if ( (m_pPlayer && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER)) || (m_pNPC && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_NPC)) )
	{
		CPointMap pt = pObj->GetTopPoint();
		const CChar *pChar = dynamic_cast<const CChar*>(pObj);
		if ( pChar )
		{
			char iTotalZ = pt.m_z + pChar->GetHeightMount(true);
			pt.m_z = minimum(iTotalZ, UO_SIZE_Z);
		}
		return CanSeeLOS_New(pt, NULL, pObj->GetVisualRange(), wFlags, bCombatCheck);
	}
	else
		return CanSeeLOS(pObj->GetTopPoint(), NULL, pObj->GetVisualRange(), wFlags, bCombatCheck);
}