#include <algorithm>
#include "../common/CException.h"
#include "../common/CLog.h"
#include "../common/CRect.h"
#include "../game/CWorld.h"
#include "../sphere/ProfileTask.h"
#include "chars/CChar.h"
#include "items/CItemShip.h"
#include "CSectorList.h"
#include "CWorldGameTime.h"
#include "CSectorTemplate.h"


////////////////////////////////////////////////////////////////////////
// -CCharsDisconnectList

void CCharsDisconnectList::AddCharDisconnected( CChar * pChar )
{
    ADDTOCALLSTACK("CCharsDisconnectList::AddCharDisconnected");

    pChar->SetUIDContainerFlags(UID_O_DISCONNECT);
    CSObjCont::InsertContentTail(pChar);
}

////////////////////////////////////////////////////////////////////////
// -CCharsActiveList

CCharsActiveList::CCharsActiveList()
{
	m_iTimeLastClient = 0;
	m_iClients = 0;
}

void CCharsActiveList::OnRemoveObj(CSObjContRec* pObjRec )
{
	ADDTOCALLSTACK("CCharsActiveList::OnRemoveObj");
    ASSERT(pObjRec);
    DEBUG_ASSERT(nullptr != dynamic_cast<const CChar*>(pObjRec));

	// Override this = called when removed from group.
	CSObjCont::OnRemoveObj(pObjRec);

	CChar* pChar = static_cast<CChar*>(pObjRec);
	if (pChar->IsClientType())
	{
		--m_iClients;
		m_iTimeLastClient = CWorldGameTime::GetCurrentTime().GetTimeRaw();	// mark time in case it's the last client
	}
	pChar->SetUIDContainerFlags(UID_O_DISCONNECT);
}

void CCharsActiveList::AddCharActive( CChar * pChar )
{
	ADDTOCALLSTACK("CCharsActiveList::AddCharActive");
	ASSERT( pChar );
	// ASSERT( pChar->m_pt.IsValid());

	CSObjCont* pParent = pChar->GetParent();
	if (pParent != this)
	{
		CSObjCont::InsertContentTail(pChar); // this also removes the Char from the old sector
		if (pChar->IsClientActive())
		{
			++m_iClients;
		}
	}

    // UID_O_DISCONNECT is removed also by SetTopPoint. But the calls are in this order: SetTopPoint, then AddCharActive -> CSObjList::InsertContentHead(pChar) ->-> OnRemoveObj
    //  (which sets UID_O_DISCONNECT), then we return in AddCharActive, where we need to manually remove this flag, otherwise we need to call SetTopPoint() here.
    pChar->RemoveUIDFlags(UID_O_DISCONNECT);
}

//////////////////////////////////////////////////////////////
// -CItemList

bool CItemsList::sm_fNotAMove = false;

void CItemsList::OnRemoveObj(CSObjContRec* pObjRec)
{
	ADDTOCALLSTACK("CItemsList::OnRemoveObj");
    DEBUG_ASSERT(nullptr != dynamic_cast<const CItem*>(pObjRec));

	// Item is picked up off the ground. (may be put right back down though)
	CItem * pItem = static_cast<CItem*>(pObjRec);

	if ( ! sm_fNotAMove )
	{
		pItem->OnMoveFrom();	// IT_MULTI, IT_SHIP and IT_COMM_CRYSTAL
	}

	DEBUG_ASSERT(pObjRec->GetParent() == this);
	CSObjCont::OnRemoveObj(pObjRec);
	DEBUG_ASSERT(pObjRec->GetParent() == nullptr);

	pItem->SetUIDContainerFlags(UID_O_DISCONNECT);	// It is no place for the moment.
}

void CItemsList::AddItemToSector( CItem * pItem )
{
	ADDTOCALLSTACK("CItemsList::AddItemToSector");
	// Add to top level.
	// Either MoveTo() or SetTimeout is being called.
	ASSERT( pItem );
    //DEBUG_ASSERT(nullptr != dynamic_cast<const CItem*>(pItem));

	CSObjCont* pParent = pItem->GetParent();
	if (pParent != this)
	{
		DEBUG_ASSERT(
            (pParent == nullptr) ||
            (pParent == g_World.GetObjectsNew()) ||
            (nullptr != dynamic_cast<const CSectorObjCont*>(pParent))
        );

		CSObjCont::InsertContentTail(pItem); // this also removes the Char from the old sector

		DEBUG_ASSERT(pItem->GetParent() == this);
#ifdef _DEBUG
        const CSObjCont* pObjNew = g_World.GetObjectsNew();
        auto itNew = std::find(pObjNew->cbegin(), pObjNew->cend(), pItem);
        DEBUG_ASSERT(itNew == pObjNew->cend());

        auto itOldParent = std::find(pParent->cbegin(), pParent->cend(), pItem);
        DEBUG_ASSERT(itOldParent == pParent->cend());
#endif
	}

    pItem->RemoveUIDFlags(UID_O_DISCONNECT);
}



//////////////////////////////////////////////////////////////////
// -CSectorBase

void CSectorBase::SetAdjacentSectors()
{
	const CSectorList* pSectors = CSectorList::Get();

    const int iMaxX = pSectors->GetSectorCols(m_map);
    ASSERT(iMaxX > 0);
    [[maybe_unused]] const int iMaxY = pSectors->GetSectorRows(m_map);
    ASSERT(iMaxY > 0);
    const int iMaxSectors = pSectors->GetSectorQty(m_map);

    // Sectors are layed out in the array horizontally: when the row is complete (X), the subsequent sector is placed in
    //  the column below (Y).
    // Between each X coordinate there's a single sector index difference;
    //  between each Y coordinate there's a number of sectors equal to the sectors in a row.
    /*
      NW        N        NE
         y-1 | y-1 | y-1
         x-1 |     | x+1
        -----|-----|-----
             | my  |
      W  x-1 | pos | x+1  E
        -----|-----|-----
         y+1 | y+1 | y+1
         x-1 |     | x+1
      SW        S        SE
    */

    struct _xyDir_s { short x, y; };
    static constexpr _xyDir_s _xyDir[DIR_QTY]
    {
        {0, -1},    // N
        {+1,-1},    // NE
        {+1, 0},    // E
        {+1, +1},   // SE
        {0, +1},    // S
        {-1, +1},   // SW
        {-1, 0},    // W
        {-1, -1}    // NW
    };

    for (int i = 0; i < (int)DIR_QTY; ++i)
    {
        // out of bounds checks
		const int iAdjX = _x + _xyDir[i].x;
		const int iAdjY = _y + _xyDir[i].y;

		int index = m_index;
        index  += ((iAdjY * iMaxX) + iAdjX);
        if (index < 0 || (index > iMaxSectors))
        {
            continue;
        }
        _ppAdjacentSectors[(DIR_TYPE)i] = pSectors->GetSector(m_map, index);
    }
}

CSector *CSectorBase::_GetAdjacentSector(DIR_TYPE dir) const
{
    ASSERT(dir >= DIR_N && dir < DIR_QTY);
    return _ppAdjacentSectors[dir];
}

CSectorBase::CSectorBase() :
    _ppAdjacentSectors{{}}
{
	m_map = 0;
	m_index = 0;
	m_dwFlags = 0;
	_x = _y = -1;
    //memset(_ppAdjacentSectors, 0, DIR_QTY * sizeof(_ppAdjacentSectors));
}

void CSectorBase::Init(int index, uchar map, short x, short y)
{
	ADDTOCALLSTACK("CSectorBase::Init");
	if (!g_MapList.IsMapSupported(map) || !g_MapList.IsInitialized(map))
	{
		g_Log.EventError("Trying to initalize a sector %d in unsupported map #%d. Defaulting to 0,0.\n", index, map);
	}
	else if (( index < 0 ) || ( index >= CSectorList::Get()->GetSectorQty(map) ))
	{
		m_map = map;
		g_Log.EventError("Trying to initalize a sector by sector number %d out-of-range for map #%d. Defaulting to 0,%d.\n", index, map, map);
	}
	else
	{
        ASSERT(x >= 0 && y >= 0);
		m_index = index;
		m_map = map;
        _x = x;
        _y = y;
	}
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
	ADDTOCALLSTACK_DEBUG("CSectorBase::GetRegion");
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
	//ADDTOCALLSTACK_DEBUG("CSectorBase::GetRegions");  // Called very frequently
	size_t iQty = m_RegionLinks.size();
	for ( size_t i = 0; i < iQty; ++i )
	{
		CRegion * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

        const CResourceID& ridRegion = pRegion->GetResourceID();
		ASSERT(ridRegion.IsValidUID());
		if (ridRegion.IsUIDItem())
		{
			const CItemShip * pShipItem = dynamic_cast <const CItemShip *>(ridRegion.ItemFindFromResource());
			if (pShipItem)
			{
				if (!(dwType & REGION_TYPE_SHIP))
					continue;
			}
			else if (!(dwType & REGION_TYPE_HOUSE))
				continue;
		}
		else if (ridRegion.GetResType() == RES_AREA )
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
        pRLinks->emplace_back(pRegion);
	}
	return pRLinks->size();
}

bool CSectorBase::UnLinkRegion( CRegion * pRegionOld )
{
	ADDTOCALLSTACK("CSectorBase::UnLinkRegion");
	if ( !pRegionOld )
		return false;
    auto it = std::find(m_RegionLinks.begin(), m_RegionLinks.end(), pRegionOld);
    if (it == m_RegionLinks.end())
        return false;
    m_RegionLinks.erase(it);
    return true;
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
			DEBUG_ERR(( "Region already linked!\n" ));
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
			m_RegionLinks.emplace(m_RegionLinks.begin() + i, pRegionNew);
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
	if ( i == sl::scont_bad_index() )
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
	if ( i != sl::scont_bad_index() )
	{
		DEBUG_ERR(( "Conflicting teleport %s!\n", pTeleport->WriteUsed() ));
		return false;
	}
	m_Teleports.AddSortKey( pTeleport, pTeleport->GetPointSortIndex());
	return true;
}

bool CSectorBase::IsFlagSet( dword dwFlag ) const noexcept
{
	return (( m_dwFlags & dwFlag) ? true : false );
}

CPointMap CSectorBase::GetBasePoint() const
{
	// ADDTOCALLSTACK_DEBUG("CSectorBase::GetBasePoint"); // It's commented because it's slow and this method is called VERY often!
	// What is the coord base of this sector. upper left point.
	const CSectorList* pSectors = CSectorList::Get();
#if _DEBUG
	DEBUG_ASSERT( (m_index >= 0) && (m_index < pSectors->GetSectorQty(m_map)) );
	// Again this method is called very often, so call the least functions possible and do the minimum amount of checks required
#endif
    const int iCols = pSectors->GetSectorCols(m_map);
    const int iSize = pSectors->GetSectorSize(m_map);

	const int iQuot = (m_index % iCols), iRem = (m_index / iCols); // Help the compiler to optimize the division
	return // Initializer list for CPointMap, it's the fastest way to return an object (requires less optimizations, which aren't used in debug build)
	{
		(short)(iQuot * iSize),	// x
		(short)(iRem * iSize),	// y
		0,						// z
		m_map					// m
	};
}

CRectMap CSectorBase::GetRect() const noexcept
{
    //ADDTOCALLSTACK_DEBUG("CSectorBase::GetRect"); // It's commented because it's slow and this method is called VERY often!
	// Get a rectangle for the sector.
	const CPointMap& pt = GetBasePoint();
    const int iSectorSize = CSectorList::Get()->GetSectorSize(pt.m_map);
	return // Initializer list for CRectMap, it's the fastest way to return an object (requires less optimizations, which aren't used in debug build)
	{
		pt.m_x,					// left
		pt.m_y,					// yop
		pt.m_x + iSectorSize,	// right: East
		pt.m_y + iSectorSize,	// bottom: South
		pt.m_map				// map
	};
}
