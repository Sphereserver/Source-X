
#include <algorithm>
#include "../game/chars/CChar.h"
#include "../game/items/CItemShip.h"
#include "../common/CLog.h"
#include "../sphere/ProfileTask.h"
#include "CException.h"
#include "CRect.h"
#include "CSectorTemplate.h"
#include "CUIDExtra.h"


CCharsDisconnectList::CCharsDisconnectList()
{

}

////////////////////////////////////////////////////////////////////////
// -CCharsActiveList

CCharsActiveList::CCharsActiveList()
{
	m_timeLastClient = 0;
	m_iClients = 0;
}

void CCharsActiveList::OnRemoveObj( CSObjListRec * pObRec )
{
	ADDTOCALLSTACK("CCharsActiveList::OnRemoveObj");
	// Override this = called when removed from group.
	CChar * pChar = dynamic_cast <CChar*>(pObRec);
	ASSERT( pChar );
	if ( pChar->IsClient())
	{
		ClientDetach();
        m_timeLastClient = g_World.GetCurrentTime().GetTimeRaw();	// mark time in case it's the last client
	}
	CSObjList::OnRemoveObj(pObRec);
	pChar->SetContainerFlags(UID_O_DISCONNECT);
}

size_t CCharsActiveList::HasClients() const
{
	return m_iClients;
}

void CCharsActiveList::AddCharToSector( CChar * pChar )
{
	ADDTOCALLSTACK("CCharsActiveList::AddCharToSector");
	ASSERT( pChar );
	// ASSERT( pChar->m_pt.IsValid());
	if ( pChar->IsClient())
	{
		ClientAttach();
	}
	CSObjList::InsertHead(pChar);
}

void CCharsActiveList::ClientAttach()
{
	ADDTOCALLSTACK("CCharsActiveList::ClientAttach");
	++m_iClients;
}

void CCharsActiveList::ClientDetach()
{
	ADDTOCALLSTACK("CCharsActiveList::ClientDetach");
	--m_iClients;
}

//////////////////////////////////////////////////////////////
// -CItemList

bool CItemsList::sm_fNotAMove = false;

void CItemsList::OnRemoveObj( CSObjListRec * pObRec )
{
	ADDTOCALLSTACK("CItemsList::OnRemoveObj");
	// Item is picked up off the ground. (may be put right back down though)
	CItem * pItem = dynamic_cast <CItem*>(pObRec);
	ASSERT( pItem );

	if ( ! sm_fNotAMove )
	{
		pItem->OnMoveFrom();	// IT_MULTI, IT_SHIP and IT_COMM_CRYSTAL
	}

	CSObjList::OnRemoveObj(pObRec);
	pItem->SetContainerFlags(UID_O_DISCONNECT);	// It is no place for the moment.
}

void CItemsList::AddItemToSector( CItem * pItem )
{
	ADDTOCALLSTACK("CItemsList::AddItemToSector");
	// Add to top level.
	// Either MoveTo() or SetTimeout is being called.
	ASSERT( pItem );
	CSObjList::InsertHead( pItem );
}

CItemsList::CItemsList()
{

}

int CObjPointSortArray::CompareKey( int id, CPointSort* pBase, bool fNoSpaces ) const
{
	UNREFERENCED_PARAMETER(fNoSpaces);
	ASSERT( pBase );
	return( id - pBase->GetPointSortIndex());
}


CObjPointSortArray::CObjPointSortArray()
{

}

//////////////////////////////////////////////////////////////////
// -CSectorBase

void CSectorBase::SetAdjacentSectors()
{
    int iMaxX = g_MapList.GetSectorCols(m_map);
    int iMaxY = g_MapList.GetSectorRows(m_map);
    int iMaxSectors = g_MapList.GetSectorQty(m_map);
    int index = 0;
    int iSectorCount = 0;
    for (int i = 0; i < m_map; ++i) // all sectors are stored in the same array, get a tmp count of all of the lower maps
    {
        if (!g_MapList.IsMapSupported(i))   // Skip disabled / unsupported maps
        {
            continue;
        }
        iSectorCount += g_MapList.GetSectorQty(i);
    }
    int tmpIndex[DIR_QTY] =
    {
        tmpIndex[DIR_N] = -iMaxX,       // North -iMaxX (an entire row)
        tmpIndex[DIR_NE] = -(iMaxX - 1),// North East ( -iMaxX  + 1 = -(iMaxX - 1))
        tmpIndex[DIR_E] = 1,            // East = +1
        tmpIndex[DIR_SE] = iMaxX + 1,   // SouthEast = iMaxX + 1
        tmpIndex[DIR_S] = iMaxX,        // South = iMaxX
        tmpIndex[DIR_SW] = iMaxX - 1,   // SouthWest = iMaxX - 1
        tmpIndex[DIR_W] = -1,           // West = -1
        tmpIndex[DIR_NW] = -(iMaxX + 1) // NorthWest = ( -iMaxX - 1 = -(iMaxX + 1))
    };
    for (int i = 0; i < (int)DIR_QTY; ++i)
    {
        index = iSectorCount + m_index + tmpIndex[i];
        if ((index <= 0) || (index >= iMaxSectors) || (index % iMaxX == 0) || index % iMaxY == 0) //out out bounds X || Y, first Col or first Row.
        {
            _mAdjacentSectors[(DIR_TYPE)i] = nullptr;
            continue;
        }
        _mAdjacentSectors[(DIR_TYPE)i] = g_World.m_Sectors[index];
    }
}

CSector *CSectorBase::GetAdjacentSector(DIR_TYPE dir) const
{
    ASSERT(dir >= DIR_N && dir < DIR_QTY);
    return _mAdjacentSectors.at(dir);
}

CSectorBase::CSectorBase()
{
	m_map = 0;
	m_index = 0;
	m_dwFlags = 0;
}

CSectorBase::~CSectorBase()
{
	ClearMapBlockCache();
}

void CSectorBase::Init(int index, int newmap)
{
	ADDTOCALLSTACK("CSectorBase::Init");
	if (( newmap < 0 ) || ( newmap >= 256 ) || !g_MapList.m_maps[newmap] )
	{
		g_Log.EventError("Trying to initalize a sector %d in unsupported map #%d. Defaulting to 0,0.\n", index, newmap);
	}
	else if (( index < 0 ) || ( index >= g_MapList.GetSectorQty(newmap) ))
	{
		m_map = newmap;
		g_Log.EventError("Trying to initalize a sector by sector number %d out-of-range for map #%d. Defaulting to 0,%d.\n", index, newmap, newmap);
	}
	else
	{
		m_index = index;
		m_map = newmap;
	}
}

bool CSectorBase::CheckMapBlockTime( const MapBlockCache::value_type& Elem ) //static
{
	ADDTOCALLSTACK("CSectorBase::CheckMapBlockTime");
	return (Elem.second->m_CacheTime.GetCacheAge() > m_iMapBlockCacheTime);
}

void CSectorBase::ClearMapBlockCache()
{
	ADDTOCALLSTACK("CSectorBase::ClearMapBlockCache");

	for (MapBlockCache::iterator it = m_MapBlockCache.begin(); it != m_MapBlockCache.end(); ++it)
		delete it->second;

	m_MapBlockCache.clear();
}

void CSectorBase::CheckMapBlockCache()
{
	ADDTOCALLSTACK("CSectorBase::CheckMapBlockCache");
	// Clean out the sectors map cache if it has not been used recently.
	// iTime == 0 = delete all.
	if ( m_MapBlockCache.empty() )
		return;
	//DEBUG_ERR(("CacheHit\n"));
    int iBlock = -1;
	for (MapBlockCache::iterator it;;)
	{
		EXC_TRY("CheckMapBlockCache_new");
		it = find_if( m_MapBlockCache.begin(), m_MapBlockCache.end(), CheckMapBlockTime );
		if ( it == m_MapBlockCache.end() )
			break;
		else
		{
			if ( m_iMapBlockCacheTime <= 0 || it->second->m_CacheTime.GetCacheAge() >= m_iMapBlockCacheTime )
			{
				//DEBUG_ERR(("removing...\n"));
				EXC_SET_BLOCK("CacheTime up - Deleting");
                iBlock = it->first;
				delete it->second;
				m_MapBlockCache.erase(it);
			}
		}
		EXC_CATCH;
		EXC_DEBUG_START;
		CPointMap pt = GetBasePoint();
		g_Log.EventDebug("m_MapBlockCache.erase(%d)\n", iBlock); 
		g_Log.EventDebug("check time %d, index %d/%" PRIuSIZE_T "\n", m_iMapBlockCacheTime, iBlock, m_MapBlockCache.size());
		g_Log.EventDebug("sector #%d [%d,%d,%d,%d]\n", GetIndex(), pt.m_x, pt.m_y, pt.m_z, pt.m_map);
		EXC_DEBUG_END;
	}
}


const CServerMapBlock * CSectorBase::GetMapBlock( const CPointMap & pt )
{
	ADDTOCALLSTACK("CSectorBase::GetMapBlock");
	// Get a map block from the cache. load it if not.
	ASSERT( pt.IsValidXY());
	CPointMap pntBlock( UO_BLOCK_ALIGN(pt.m_x), UO_BLOCK_ALIGN(pt.m_y), 0, pt.m_map);
	ASSERT( m_MapBlockCache.size() <= (UO_BLOCK_SIZE * UO_BLOCK_SIZE));

	ProfileTask mapTask(PROFILE_MAP);

	if ( !pt.IsValidXY() )
	{
		g_Log.EventWarn("Attempting to access invalid memory block at %s.\n", pt.WriteUsed());
		return nullptr;
	}

	CServerMapBlock * pMapBlock;

	// Find it in cache.
	int iBlock = pntBlock.GetPointSortIndex();
	MapBlockCache::iterator it = m_MapBlockCache.find(iBlock);
	if ( it != m_MapBlockCache.end() )
	{
		it->second->m_CacheTime.HitCacheTime();
		return it->second;
	}

	// else load it.
	try
	{
		pMapBlock = new CServerMapBlock(pntBlock);
		ASSERT(pMapBlock != nullptr);
	}
	catch ( const CSError& e )
	{
		g_Log.EventError("Exception creating new memory block at %s. (%s)\n", pntBlock.WriteUsed(), e.m_pszDescription);
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return nullptr;
	}
	catch (...)
	{
		g_Log.EventError("Exception creating new memory block at %s.\n", pntBlock.WriteUsed());
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return nullptr;
	}

	// Add it to the cache.
	m_MapBlockCache[iBlock] = pMapBlock;
	return( pMapBlock );
}

bool CSectorBase::IsInDungeon() const
{
	ADDTOCALLSTACK("CSectorBase::IsInDungeon");
	// What part of the maps are filled with dungeons.
	// Used for light / weather calcs.
	CPointMap pt = GetBasePoint();
	CRegion *pRegion = GetRegion(pt, REGION_TYPE_AREA);

	return ( pRegion && pRegion->IsFlag(REGION_FLAG_UNDERGROUND) );
}

CRegion * CSectorBase::GetRegion( const CPointBase & pt, dword dwType ) const
{
	ADDTOCALLSTACK("CSectorBase::GetRegion");
	// Does it match the mask of types we care about ?
	// Assume sorted so that the smallest are first.
	//
	// REGION_TYPE_AREA => RES_AREA = World region area only = CRegionWorld
	// REGION_TYPE_ROOM => RES_ROOM = NPC House areas only = CRegion.
	// REGION_TYPE_MULTI => RES_WORLDITEM = UID linked types in general = CRegionWorld

	size_t iQty = m_RegionLinks.size();
	for ( size_t i = 0; i < iQty; ++i )
	{
		CRegion * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

		ASSERT( pRegion->GetResourceID().IsValidUID());
		if ( pRegion->GetResourceID().IsItem())
		{
			CItemShip * pShipItem = dynamic_cast <CItemShip *>(pRegion->GetResourceID().ItemFind());
			if (pShipItem)
			{
				if (!(dwType & REGION_TYPE_SHIP))
					continue;
			}
			else if (!(dwType & REGION_TYPE_HOUSE))
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( ! ( dwType & REGION_TYPE_AREA ))
				continue;
		}
		else
		{
			if ( ! ( dwType & REGION_TYPE_ROOM ))
				continue;
		}

		if ( pRegion->m_pt.m_map != pt.m_map )
			continue;
		if ( ! pRegion->IsInside2d( pt ))
			continue;
		return pRegion;
	}
	return nullptr;
}

// Balkon: get regions list (to cicle through intercepted house regions)
size_t CSectorBase::GetRegions( const CPointBase & pt, dword dwType, CRegionLinks & rlist ) const
{
	ADDTOCALLSTACK("CSectorBase::GetRegions");
	size_t iQty = m_RegionLinks.size();
	for ( size_t i = 0; i < iQty; ++i )
	{
		CRegion * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

		ASSERT( pRegion->GetResourceID().IsValidUID());
		if ( pRegion->GetResourceID().IsItem())
		{
			CItemShip * pShipItem = dynamic_cast <CItemShip *>(pRegion->GetResourceID().ItemFind());
			if (pShipItem)
			{
				if (!(dwType & REGION_TYPE_SHIP))
					continue;
			}
			else if (!(dwType & REGION_TYPE_HOUSE))
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( ! ( dwType & REGION_TYPE_AREA ))
				continue;
		}
		else
		{
			if ( ! ( dwType & REGION_TYPE_ROOM ))
				continue;
		}

		if ( pRegion->m_pt.m_map != pt.m_map )
			continue;
		if ( ! pRegion->IsInside2d( pt ))
			continue;
		rlist.push_back(pRegion);
	}
	return rlist.size();
}

bool CSectorBase::UnLinkRegion( CRegion * pRegionOld )
{
	ADDTOCALLSTACK("CSectorBase::UnLinkRegion");
	if ( !pRegionOld )
		return false;
	return m_RegionLinks.RemovePtr(pRegionOld);
}

bool CSectorBase::LinkRegion( CRegion * pRegionNew )
{
	ADDTOCALLSTACK("CSectorBase::LinkRegion");
	// link in a region. may have just moved !
	// Make sure the smaller regions are first in the array !
	// Later added regions from the MAP file should be the smaller ones, 
	//  according to the old rules.
	ASSERT(pRegionNew);
	ASSERT( pRegionNew->IsOverlapped(GetRect()) );
	size_t iQty = m_RegionLinks.size();

	for ( size_t i = 0; i < iQty; ++i )
	{
		CRegion * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);
		if ( pRegionNew == pRegion )
		{
			DEBUG_ERR(( "region already linked!\n" ));
			return false;
		}

		if ( pRegion->IsOverlapped(pRegionNew))
		{
			// NOTE : We should use IsInside() but my version isn't completely accurate for it's FALSE return
			if ( pRegion->IsEqualRegion( pRegionNew ))
			{
				DEBUG_ERR(( "Conflicting region!\n" ));
				return false;
			}

			// it is accurate in the TRUE case.
			if ( pRegionNew->IsInside(pRegion))
				continue;

			// keep item (multi) regions on top
			if ( pRegion->GetResourceID().IsItem() && !pRegionNew->GetResourceID().IsItem() )
				continue;

			// must insert before this.
			m_RegionLinks.insert(i, pRegionNew);
			return true;
		}
	}

	m_RegionLinks.push_back(pRegionNew);
	return true;
}

CTeleport * CSectorBase::GetTeleport( const CPointMap & pt ) const
{
	ADDTOCALLSTACK("CSectorBase::GetTeleport");
	// Any teleports here at this point ?

	size_t i = m_Teleports.FindKey(pt.GetPointSortIndex());
	if ( i == m_Teleports.BadIndex() )
		return nullptr;

	CTeleport *pTeleport = static_cast<CTeleport *>(m_Teleports[i]);
	if ( pTeleport->m_map != pt.m_map )
		return nullptr;
	if ( abs(pTeleport->m_z - pt.m_z) > 5 )
		return nullptr;

	return pTeleport;
}

bool CSectorBase::AddTeleport( CTeleport * pTeleport )
{
	ADDTOCALLSTACK("CSectorBase::AddTeleport");
	// NOTE: can't be 2 teleports from the same place !
	// ASSERT( Teleport is actually in this sector !

	size_t i = m_Teleports.FindKey( pTeleport->GetPointSortIndex());
	if ( i != m_Teleports.BadIndex() )
	{
		DEBUG_ERR(( "Conflicting teleport %s!\n", pTeleport->WriteUsed() ));
		return false;
	}
	m_Teleports.AddSortKey( pTeleport, pTeleport->GetPointSortIndex());
	return true;
}

bool CSectorBase::IsFlagSet( dword dwFlag ) const
{
	return (( m_dwFlags & dwFlag) ? true : false );
}

CPointMap CSectorBase::GetBasePoint() const
{
	ADDTOCALLSTACK("CSectorBase::GetBasePoint");
	// What is the coord base of this sector. upper left point.
	ASSERT( m_index >= 0 && m_index < g_MapList.GetSectorQty(m_map) );
	CPointMap pt(( (word)((m_index % g_MapList.GetSectorCols(m_map)) * g_MapList.GetSectorSize(m_map))),
		(word)((m_index / g_MapList.GetSectorCols(m_map)) * g_MapList.GetSectorSize(m_map)),
		0,
		(uchar)(m_map));
	return pt;
}

CRectMap CSectorBase::GetRect() const
{
	ADDTOCALLSTACK("CSectorBase::GetRect");
	// Get a rectangle for the sector.
	CPointMap pt = GetBasePoint();
	CRectMap rect;
	rect.m_left = pt.m_x;
	rect.m_top = pt.m_y;
	rect.m_right = pt.m_x + g_MapList.GetSectorSize(pt.m_map);	// East
	rect.m_bottom = pt.m_y + g_MapList.GetSectorSize(pt.m_map);	// South
	rect.m_map = pt.m_map;
	return rect;
}
