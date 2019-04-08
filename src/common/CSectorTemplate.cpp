
#include <algorithm>
#include "../common/CLog.h"
#include "../game/chars/CChar.h"
#include "../game/items/CItemShip.h"
#include "../game/CWorld.h"
#include "../sphere/ProfileTask.h"
#include "CException.h"
#include "CRect.h"
#include "CSectorTemplate.h"


////////////////////////////////////////////////////////////////////////
// -CCharsDisconnectList

void CCharsDisconnectList::AddCharDisconnected( CChar * pChar )
{
    ADDTOCALLSTACK("CCharsDisconnectList::AddCharDisconnected");
    pChar->SetUIDContainerFlags(UID_O_DISCONNECT);
    InsertHead(pChar);
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
        ClientDecrease();
        m_timeLastClient = g_World.GetCurrentTime().GetTimeRaw();	// mark time in case it's the last client
	}
	CSObjList::OnRemoveObj(pObRec);
	pChar->SetUIDContainerFlags(UID_O_DISCONNECT);
}

int CCharsActiveList::GetClientsNumber() const
{
	return m_iClients;
}

void CCharsActiveList::AddCharActive( CChar * pChar )
{
	ADDTOCALLSTACK("CCharsActiveList::AddCharActive");
	ASSERT( pChar );
	// ASSERT( pChar->m_pt.IsValid());
	CSObjList::InsertHead(pChar); // this also removes the Char from the old sector
    // UID_O_DISCONNECT is removed also by SetTopPoint. But the calls are in this order: SetTopPoint, then AddCharActive -> CSObjList::InsertHead(pChar) ->-> OnRemoveObj
    //  (which sets UID_O_DISCONNECT), then we return in AddCharActive, where we need to manually remove this flag, otherwise we need to call SetTopPoint() here.
    pChar->RemoveUIDFlags(UID_O_DISCONNECT);
    if ( pChar->IsClient())
    {
        ClientIncrease();
    }
}

void CCharsActiveList::ClientIncrease()
{
	ADDTOCALLSTACK("CCharsActiveList::ClientIncrease");
	++m_iClients;
}

void CCharsActiveList::ClientDecrease()
{
	ADDTOCALLSTACK("CCharsActiveList::ClientDecrease");
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
	pItem->SetUIDContainerFlags(UID_O_DISCONNECT);	// It is no place for the moment.
}

void CItemsList::AddItemToSector( CItem * pItem )
{
	ADDTOCALLSTACK("CItemsList::AddItemToSector");
	// Add to top level.
	// Either MoveTo() or SetTimeout is being called.
	ASSERT( pItem );
	CSObjList::InsertHead( pItem ); // this also removes the Char from the old sector
    pItem->RemoveUIDFlags(UID_O_DISCONNECT);
}


//////////////////////////////////////////////////////////////////
// -CSectorBase::CObjPointSortArray


int CSectorBase::CObjPointSortArray::CompareKey( int id, CPointSort* pBase, bool fNoSpaces ) const
{
    UNREFERENCED_PARAMETER(fNoSpaces);
    ASSERT( pBase );
    return (id - pBase->GetPointSortIndex());
}


//////////////////////////////////////////////////////////////////
// -CSectorBase

void CSectorBase::SetAdjacentSectors()
{
    const int iMaxX = g_MapList.GetSectorCols(m_map);
    const int iMaxY = g_MapList.GetSectorRows(m_map);
    const int iMaxSectors = g_MapList.GetSectorQty(m_map);
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
	return pMapBlock;
}

bool CSectorBase::IsInDungeon() const
{
	ADDTOCALLSTACK("CSectorBase::IsInDungeon");
	// What part of the maps are filled with dungeons.
	// Used for light / weather calcs.
	CRegion *pRegion = GetRegion(GetBasePoint(), REGION_TYPE_AREA);

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

        const CResourceID& ridRegion = pRegion->GetResourceID();
		ASSERT( ridRegion.IsValidUID());
		if ( ridRegion.IsUIDItem())
		{
			const CItemShip * pShipItem = dynamic_cast <CItemShip *>(ridRegion.ItemFindFromResource());
			if (pShipItem)
			{
				if (!(dwType & REGION_TYPE_SHIP))
					continue;
			}
			else if (!(dwType & REGION_TYPE_HOUSE))
				continue;
		}
		else if ( ridRegion.GetResType() == RES_AREA )
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

// Balkon: get regions list (to cycle through intercepted house regions)
size_t CSectorBase::GetRegions( const CPointBase & pt, dword dwType, CRegionLinks *pRLinks ) const
{
	ADDTOCALLSTACK("CSectorBase::GetRegions");
	size_t iQty = m_RegionLinks.size();
	for ( size_t i = 0; i < iQty; ++i )
	{
		CRegion * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

		ASSERT( pRegion->GetResourceID().IsValidUID());
		if ( pRegion->GetResourceID().IsUIDItem())
		{
			CItemShip * pShipItem = dynamic_cast <CItemShip *>(pRegion->GetResourceID().ItemFindFromResource());
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
        pRLinks->push_back(pRegion);
	}
	return pRLinks->size();
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
			if ( pRegion->GetResourceID().IsUIDItem() && !pRegionNew->GetResourceID().IsUIDItem() )
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
	ADDTOCALLSTACK_INTENSIVE("CSectorBase::GetBasePoint");
	// What is the coord base of this sector. upper left point.
	ASSERT( m_index >= 0 && m_index < g_MapList.GetSectorQty(m_map) );
    const int iCols = g_MapList.GetSectorCols(m_map);
    const int iSize = g_MapList.GetSectorSize(m_map);
	CPointMap pt(
        (word)((m_index % iCols) * iSize),
		(word)((m_index / iCols) * iSize),
		0,
		(uchar)(m_map));
	return pt;
}

CRectMap CSectorBase::GetRect() const
{
    ADDTOCALLSTACK_INTENSIVE("CSectorBase::GetRect");
	// Get a rectangle for the sector.
	const CPointMap pt = GetBasePoint();
    const int iSectorSize = g_MapList.GetSectorSize(pt.m_map);
	CRectMap rect;
	rect.m_left = pt.m_x;
	rect.m_top = pt.m_y;
	rect.m_right = pt.m_x + iSectorSize;	// East
	rect.m_bottom = pt.m_y + iSectorSize;	// South
	rect.m_map = pt.m_map;
	return rect;
}
