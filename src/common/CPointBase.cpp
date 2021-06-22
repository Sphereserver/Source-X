#include "../game/items/CItem.h"
#include "../game/CSector.h"
#include "../game/CServer.h"
#include "../game/CWorldMap.h"
#include "resource/sections/CItemTypeDef.h"
#include "CLog.h"
#include "CRect.h"
#include "CPointBase.h"


DIR_TYPE GetDirTurn( DIR_TYPE dir, int offset )
{
	// Turn in a direction.
	// +1 = to the right.
	// -1 = to the left.
	ASSERT((offset + DIR_QTY + dir) >= 0);
	offset += DIR_QTY + dir;
	offset %= DIR_QTY;
	return static_cast<DIR_TYPE>(offset);
}

//*************************************************************************
// -CPointBase

lpctstr CPointBase::sm_szDirs[DIR_QTY+1] =
{
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_0),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_1),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_2),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_3),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_4),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_5),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_6),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_7),
	g_Cfg.GetDefaultMsg(DEFMSG_MAP_DIR_8),
};

const short CPointBase::sm_Moves[DIR_QTY+1][2] =
{
	{  0, -1 }, // DIR_N
    {  1, -1 }, // DIR_NE
    {  1,  0 }, // DIR_E
    {  1,  1 }, // DIR_SE
    {  0,  1 }, // DIR_S
    { -1,  1 }, // DIR_SW
    { -1,  0 }, // DIR_W
    { -1, -1 }, // DIR_NW
    {  0,  0 },	// DIR_QTY = here.
};


enum PT_TYPE
{
	PT_ISNEARTYPE,
	PT_M,
	PT_MAP,
	PT_REGION,
	PT_ROOM,
	PT_SECTOR,
	PT_TERRAIN,
	PT_TYPE,
	PT_X,
	PT_Y,
	PT_Z,
	PT_QTY
};


lpctstr const CPointBase::sm_szLoadKeys[PT_QTY+1] =
{
	"ISNEARTYPE",
	"M",
	"MAP",
	"REGION",
	"ROOM",
	"SECTOR",
	"TERRAIN",
	"TYPE",
	"X",
	"Y",
	"Z",
	nullptr
};

CPointBase::CPointBase() noexcept :
	m_x(-1), m_y(-1), m_z(0), m_map(0)	// Same thing as calling InitPoint(), but without this extra cuntion call
{
	//InitPoint();
}

CPointBase::CPointBase(short x, short y, char z, uchar map) noexcept :
	m_x(x), m_y(y), m_z(z), m_map(map)
{
}

bool CPointBase::operator== ( const CPointBase & pt ) const noexcept
{
	return( m_x == pt.m_x && m_y == pt.m_y && m_z == pt.m_z && m_map == pt.m_map );
}

bool CPointBase::operator!= ( const CPointBase & pt ) const noexcept
{
	return ( ! ( *this == pt ));
}

const CPointBase& CPointBase::operator+= ( const CPointBase & pt ) noexcept
{
	m_x += pt.m_x;
	m_y += pt.m_y;
	m_z += pt.m_z;
	return( * this );
}

const CPointBase& CPointBase::operator-= ( const CPointBase & pt ) noexcept
{
	m_x -= pt.m_x;
	m_y -= pt.m_y;
	m_z -= pt.m_z;
	return( * this );
}

void CPointBase::InitPoint() noexcept
{
	m_x = m_y = -1;	// invalid location.
	m_z = 0;
	m_map = 0;
}
void CPointBase::ZeroPoint() noexcept
{
	m_x = m_y = 0;	// invalid location.
	m_z = 0;
	m_map = 0;
}

int CPointBase::GetDistZ( const CPointBase & pt ) const noexcept
{
	return SphereAbs(m_z - pt.m_z);
}

int CPointBase::GetDistBase( const CPointBase & pt ) const noexcept // Distance between points
{
    // This method is called very frequently, ADDTOCALLSTACK unneededly sucks cpu
    //ADDTOCALLSTACK_INTENSIVE("CPointBase::GetDistBase");

	// Do not consider z or m_map.
    const int dx = SphereAbs(m_x - pt.m_x);
    const int dy = SphereAbs(m_y - pt.m_y);

	return maximum(dx, dy);

	// What the heck?
	/*double dist = sqrt(static_cast<double>((dx * dx) + (dy * dy)));

	return (int)(( (dist - floor(dist)) > 0.5 ) ? (ceil(dist)) : (floor(dist)));*/
	// Return the real distance return((int) sqrt(dx*dx+dy*dy+dz*dz));
}

int CPointBase::GetDist( const CPointBase & pt ) const noexcept // Distance between points
{
    // This method is called very frequently, ADDTOCALLSTACK unneededly sucks cpu
	//ADDTOCALLSTACK_INTENSIVE("CPointBase::GetDist");

	// Get the basic 2d distance.
	if ( !pt.IsValidPoint() )
		return INT16_MAX;
	if ( pt.m_map != m_map )
		return INT16_MAX;

	return GetDistBase(pt);
}

int CPointBase::GetDistSightBase( const CPointBase & pt ) const noexcept // Distance between points based on UO sight
{
	const int dx = SphereAbs(m_x - pt.m_x);
	const int dy = SphereAbs(m_y - pt.m_y);
	return maximum(dx, dy);
}

int CPointBase::GetDistSight( const CPointBase & pt ) const noexcept // Distance between points based on UO sight
{
	if ( !pt.IsValidPoint() )
		return INT16_MAX;
	if ( pt.m_map != m_map )
		return INT16_MAX;

	const int dx = SphereAbs(m_x - pt.m_x);
	const int dy = SphereAbs(m_y - pt.m_y);
	return maximum(dx, dy);
}

int CPointBase::GetDist3D( const CPointBase & pt ) const noexcept // Distance between points
{
	// OK, 1 unit of Z is not the same (real life) distance as 1 unit of X (or Y)
	const int dist = GetDist(pt);

	// Get the deltas and correct the Z for height first
	const int dz = (GetDistZ(pt) / (PLAYER_HEIGHT / 2)); // Take player height into consideration

	return maximum(dz, dist);
	// What the heck?
	/*double realdist = sqrt(static_cast<double>((dist * dist) + (dz * dz)));

	return (int)(( (realdist - floor(realdist)) > 0.5 ) ? (ceil(realdist)) : (floor(realdist)));*/
}

bool CPointBase::IsValidZ() const noexcept
{
	return ( (m_z > -UO_SIZE_Z) && (m_z < UO_SIZE_Z) );
}

bool CPointBase::IsValidXY() const noexcept
{
	if ( (m_x < 0) || (m_y < 0) )
		return false;
	if ( (m_x >= g_MapList.GetMapSizeX(m_map)) || (m_y >= g_MapList.GetMapSizeY(m_map)) )
		return false;
	return true;
}

bool CPointBase::IsCharValid() const noexcept
{
	if ( (m_z <= -UO_SIZE_Z) || (m_z >= UO_SIZE_Z) )
		return false;
	if ((m_x <= 0) || (m_y <= 0))
		return false;
	if ((m_x >= g_MapList.GetMapSizeX(m_map)) || (m_y >= g_MapList.GetMapSizeY(m_map)))
		return false;
	return true;
}

void CPointBase::ValidatePoint() noexcept
{
	if ( m_x < 0 )
		m_x = 0;
    const short iMaxX = (short)g_MapList.GetMapSizeX(m_map);
	if (m_x >= iMaxX)
		m_x = iMaxX - 1;

	if ( m_y < 0 )
		m_y = 0;
    const short iMaxY = (short)g_MapList.GetMapSizeY(m_map);
	if (m_y >= iMaxY)
		m_y = iMaxY - 1;
}

bool CPointBase::IsSame2D( const CPointBase & pt ) const noexcept
{
	return ( m_x == pt.m_x && m_y == pt.m_y );
}

void CPointBase::Set( const CPointBase & pt ) noexcept
{
	m_x = pt.m_x;
	m_y = pt.m_y;
	m_z = pt.m_z;
	m_map = pt.m_map;
}

void CPointBase::Set( short x, short y, char z, uchar map ) noexcept
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_map = map;
}

void CPointBase::Move( DIR_TYPE dir )
{
	// Move a point in a direction.
	ASSERT( (dir > DIR_INVALID) && (dir <= DIR_QTY) );
	m_x += (short)(sm_Moves[dir][0]);
	m_y += (short)(sm_Moves[dir][1]);
}

void CPointBase::MoveN( DIR_TYPE dir, int amount )
{
	// Move a point in a direction.
	ASSERT( dir <= DIR_QTY );
	m_x += (short)(sm_Moves[dir][0] * amount);
	m_y += (short)(sm_Moves[dir][1] * amount);
}

bool CPointBase::r_WriteVal( lpctstr ptcKey, CSString & sVal ) const
{
	ADDTOCALLSTACK("CPointBase::r_WriteVal");
	if ( !strnicmp( ptcKey, "STATICS", 7 ) )
	{
		ptcKey	+= 7;
		const CServerMapBlock * pBlock = CWorldMap::GetMapBlock( static_cast<CPointMap>(*this) );
		if ( !pBlock )
			return false;

		if ( *ptcKey == '\0' )
		{
			uint uiStaticQty = 0, uiStaticMaxQty = pBlock->m_Statics.GetStaticQty();
			for ( uint i = 0; i < uiStaticMaxQty; ++i )
			{
				const CUOStaticItemRec * pStatic = pBlock->m_Statics.GetStatic( i );
				const CPointMap ptTest( pStatic->m_x + pBlock->m_x, pStatic->m_y + pBlock->m_y, pStatic->m_z, this->m_map );
				if ( this->GetDist( ptTest ) > 0 )
					continue;
				++uiStaticQty;
			}

			sVal.FormatUVal( uiStaticQty );
			return true;
		}

		SKIP_SEPARATORS( ptcKey );

		const CUOStaticItemRec * pStatic = nullptr;
		int iStatic = 0;
		int type = 0;

		if ( !strnicmp( ptcKey, "FINDID", 6 ) )
		{
			ptcKey += 6;
			SKIP_SEPARATORS( ptcKey );
			iStatic = Exp_GetVal( ptcKey );
			type = RES_GET_TYPE( iStatic );
			if ( type == 0 )
				type = RES_ITEMDEF;
			SKIP_SEPARATORS( ptcKey );
		}
		else
		{
			iStatic = Exp_GetVal( ptcKey );
			type = RES_GET_TYPE( iStatic );
		}

		if ( type == RES_ITEMDEF )
		{
			const CItemBase * pItemDef = CItemBase::FindItemBase((ITEMID_TYPE)(RES_GET_INDEX(iStatic)));
			if ( !pItemDef )
			{
				sVal.FormatVal( 0 );
				return false;
			}
            uint uiStaticMaxQty = pBlock->m_Statics.GetStaticQty();
			for ( uint i = 0; i < uiStaticMaxQty; pStatic = nullptr, ++i )
			{
				pStatic = pBlock->m_Statics.GetStatic( i );
				CPointMap ptTest( pStatic->m_x+pBlock->m_x, pStatic->m_y+pBlock->m_y, pStatic->m_z, this->m_map);
				if ( this->GetDist( ptTest ) > 0 )
					continue;
				if ( pStatic->GetDispID() == pItemDef->GetDispID() )
					break;
			}
		}
		else
		{
            uint uiStaticMaxQty = pBlock->m_Statics.GetStaticQty();
			for ( uint i = 0; i < uiStaticMaxQty; pStatic = nullptr, ++i )
			{
				pStatic = pBlock->m_Statics.GetStatic( i );
				CPointMap ptTest( pStatic->m_x+pBlock->m_x, pStatic->m_y+pBlock->m_y, pStatic->m_z, this->m_map);
				if ( this->GetDist( ptTest ) > 0 )
					continue;
				if ( iStatic == 0 )
					break;
				--iStatic;
			}
		}

		if ( !pStatic )
		{
			sVal.FormatHex(0);
			return true;
		}

		SKIP_SEPARATORS( ptcKey );
		if ( !*ptcKey )
			ptcKey	= "ID";

		ITEMID_TYPE idTile = pStatic->GetDispID();

		if ( !strnicmp( ptcKey, "COLOR", 5 ) )
		{
			sVal.FormatHex( pStatic->m_wHue );
			return true;
		}
		else if ( !strnicmp( ptcKey, "ID", 2 ) )
		{
			sVal.FormatHex( idTile );
			return true;
		}
		else if ( !strnicmp( ptcKey, "Z", 1 ) )
		{
			sVal.FormatVal( pStatic->m_z );
			return true;
		}

		// Check the script def for the item.
		CItemBase * pItemDef = CItemBase::FindItemBase( idTile );
		if ( pItemDef == nullptr )
		{
			DEBUG_ERR(("Must have ITEMDEF section for item ID 0%x\n", idTile ));
			return false;
		}

		return pItemDef->r_WriteVal( ptcKey, sVal, &g_Serv );
	}
	else if ( !strnicmp( ptcKey, "COMPONENTS", 10) )
	{
		ptcKey += 10;

		CRegionLinks rlinks;
		const CRegion* pRegion = nullptr;
		CItem* pItem = nullptr;
		const CUOMulti* pMulti = nullptr;
		const CUOMultiItemRec_HS* pMultiItem = nullptr;
		size_t iMultiQty = GetRegions(REGION_TYPE_MULTI, &rlinks);

		if ( *ptcKey == '\0' )
		{
			int iComponentQty = 0;
			for (size_t i = 0; i < iMultiQty; ++i)
			{
				pRegion = rlinks[i];
				if (pRegion == nullptr)
					continue;

				pItem = pRegion->GetResourceID().ItemFindFromResource();
				if (pItem == nullptr)
					continue;

				const CPointMap ptMulti(pItem->GetTopPoint());
				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if (pMulti == nullptr)
					continue;

				size_t iQty = pMulti->GetItemCount();
				for (size_t ii = 0; ii < iQty; ++ii)
				{
					pMultiItem = pMulti->GetItem(ii);
					if (pMultiItem == nullptr)
						break;
					if (pMultiItem->m_visible == 0)
						continue;

					const CPointMap ptTest((word)(ptMulti.m_x + pMultiItem->m_dx), (word)(ptMulti.m_y + pMultiItem->m_dy), (char)(ptMulti.m_z + pMultiItem->m_dz), this->m_map);
					if (GetDist(ptTest) > 0)
						continue;

					++iComponentQty;
				}
			}

			sVal.FormatVal( iComponentQty );
			return true;
		}

		SKIP_SEPARATORS( ptcKey );

		int iComponent = 0;
		int type = 0;

		if ( strnicmp( ptcKey, "FINDID", 6 ) == 0 )
		{
			ptcKey += 6;
			SKIP_SEPARATORS( ptcKey );
			iComponent = Exp_GetVal( ptcKey );
			type = RES_GET_TYPE( iComponent );
			if ( type == 0 )
				type = RES_ITEMDEF;
			SKIP_SEPARATORS( ptcKey );
		}
		else
		{
			iComponent = Exp_GetVal( ptcKey );
			type = RES_GET_TYPE( iComponent );
		}

		if ( type == RES_ITEMDEF )
		{
			const CItemBase * pItemDef = CItemBase::FindItemBase((ITEMID_TYPE)(RES_GET_INDEX(iComponent)));
			if ( pItemDef == nullptr )
			{
				sVal.FormatVal( 0 );
				return false;
			}

			for (size_t i = 0; i < iMultiQty; ++i)
			{
				pRegion = rlinks[i];
				if (pRegion == nullptr)
					continue;

				pItem = pRegion->GetResourceID().ItemFindFromResource();
				if (pItem == nullptr)
					continue;

				const CPointMap ptMulti = pItem->GetTopPoint();
				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if (pMulti == nullptr)
					continue;

				size_t iQty = pMulti->GetItemCount();
				for (size_t ii = 0; ii < iQty; pMultiItem = nullptr, ii++)
				{
					pMultiItem = pMulti->GetItem(ii);
					if (pMultiItem == nullptr)
						break;
					if (pMultiItem->m_visible == 0)
						continue;
					CPointMap ptTest((word)(ptMulti.m_x + pMultiItem->m_dx), (word)(ptMulti.m_y + pMultiItem->m_dy), (char)(ptMulti.m_z + pMultiItem->m_dz), this->m_map);
					if (GetDist(ptTest) > 0)
						continue;

					const CItemBase* pMultiItemDef = CItemBase::FindItemBase(pMultiItem->GetDispID());
					if (pMultiItemDef != nullptr && pMultiItemDef->GetDispID() == pItemDef->GetDispID())
						break;
				}

				if (pMultiItem != nullptr)
					break;
			}
		}
		else
		{
			for (size_t i = 0; i < iMultiQty; ++i)
			{
				pRegion = rlinks[i];
				if (pRegion == nullptr)
					continue;

				pItem = pRegion->GetResourceID().ItemFindFromResource();
				if (pItem == nullptr)
					continue;

				const CPointMap ptMulti = pItem->GetTopPoint();
				pMulti = g_Cfg.GetMultiItemDefs(pItem);
				if (pMulti == nullptr)
					continue;

				size_t iQty = pMulti->GetItemCount();
				for (size_t ii = 0; ii < iQty; pMultiItem = nullptr, ++ii)
				{
					pMultiItem = pMulti->GetItem(ii);
					if (pMultiItem == nullptr)
						break;
					if (pMultiItem->m_visible == 0)
						continue;
					CPointMap ptTest((word)(ptMulti.m_x + pMultiItem->m_dx), (word)(ptMulti.m_y + pMultiItem->m_dy), (char)(ptMulti.m_z + pMultiItem->m_dz), this->m_map);
					if (GetDist(ptTest) > 0)
						continue;

					if (iComponent == 0)
						break;

					--iComponent;
				}

				if (pMultiItem != nullptr)
					break;
			}
		}

		if ( pMultiItem == nullptr )
		{
			sVal.FormatHex(0);
			return true;
		}

		SKIP_SEPARATORS( ptcKey );
		if ( !*ptcKey )
			ptcKey	= "ID";

		ITEMID_TYPE idTile = pMultiItem->GetDispID();

		if ( strnicmp( ptcKey, "ID", 2 ) == 0 )
		{
			sVal.FormatHex( idTile );
			return true;
		}
		else if ( strnicmp( ptcKey, "MULTI", 5 ) == 0 )
		{
			ptcKey += 5;
			if (*ptcKey != '\0')
			{
				SKIP_SEPARATORS(ptcKey);
				return pItem->r_WriteVal( ptcKey, sVal, &g_Serv );
			}

			sVal.FormatHex( pItem->GetUID() );
			return true;
		}
		else if ( strnicmp( ptcKey, "Z", 1 ) == 0 )
		{
			sVal.FormatVal( pItem->GetTopZ() + pMultiItem->m_dz );
			return true;
		}

		// Check the script def for the item.
		CItemBase * pItemDef = CItemBase::FindItemBase( idTile );
		if ( pItemDef == nullptr )
		{
			DEBUG_ERR(("Must have ITEMDEF section for item ID 0%x\n", idTile ));
			return false;
		}

		return pItemDef->r_WriteVal( ptcKey, sVal, &g_Serv );
	}

	int index = FindTableHeadSorted( ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 );
	if ( index < 0 )
		return false;

	switch ( index )
	{
		case PT_M:
		case PT_MAP:
			sVal.FormatVal(m_map);
			break;
		case PT_X:
			sVal.FormatVal(m_x);
			break;
		case PT_Y:
			sVal.FormatVal(m_y);
			break;
		case PT_Z:
			sVal.FormatVal(m_z);
			break;
		case PT_ISNEARTYPE:
		{
			ptcKey += 10;
			SKIP_SEPARATORS( ptcKey );
			SKIP_ARGSEP( ptcKey );

			int iType = g_Cfg.ResourceGetIndexType( RES_TYPEDEF, ptcKey );
			int iDistance = 0;
			bool bCheckMulti = false;

			SKIP_IDENTIFIERSTRING( ptcKey );
			SKIP_SEPARATORS( ptcKey );
			SKIP_ARGSEP( ptcKey );

			if ( *ptcKey ) iDistance = Exp_GetVal(ptcKey);
			if ( *ptcKey ) bCheckMulti = Exp_GetVal(ptcKey) != 0;
			sVal.FormatVal( CWorldMap::IsItemTypeNear(*this, IT_TYPE(iType), iDistance, bCheckMulti));
			break;
		}
		case PT_REGION:
		{
			// Check that the syntax is correct.
			if ( ptcKey[6] && ptcKey[6] != '.' )
				return false;

			CRegionWorld * pRegionTemp = dynamic_cast <CRegionWorld*>(this->GetRegion(REGION_TYPE_AREA | REGION_TYPE_MULTI));

			if ( !ptcKey[6] )
			{
				// We're just checking if the reference is valid.
				sVal.FormatVal( pRegionTemp? 1:0 );
				return true;
			}

			// We're trying to retrieve a property from the region.
			ptcKey += 7;
			if ( pRegionTemp )
				return pRegionTemp->r_WriteVal( ptcKey, sVal, &g_Serv );

			return false;
		}
		case PT_ROOM:
		{
			if ( ptcKey[4] && ptcKey[4] != '.' )
				return false;

			CRegion * pRegionTemp = this->GetRegion( REGION_TYPE_ROOM );

			if ( !ptcKey[4] )
			{
				sVal.FormatVal( pRegionTemp? 1:0 );
				return true;
			}

			ptcKey += 5;
			if ( pRegionTemp )
				return pRegionTemp->r_WriteVal( ptcKey, sVal, &g_Serv );

			return false;
		}
		case PT_SECTOR:
		{
			if ( ptcKey[6] == '.' )
			{
				ptcKey += 7;
				CSector * pSectorTemp = this->GetSector();
				if (pSectorTemp)
					return pSectorTemp->r_WriteVal(ptcKey, sVal, &g_Serv);
			}
			return false;
		}
		default:
		{
			const CUOMapMeter * pMeter = CWorldMap::GetMapMeter(*this);
			if ( pMeter )
			{
				switch( index )
				{
					case PT_TYPE:
					{
						CItemTypeDef * pTypeDef = CWorldMap::GetTerrainItemTypeDef( pMeter->m_wTerrainIndex );
						if ( pTypeDef != nullptr )
							sVal = pTypeDef->GetResourceName();
						else
							sVal = "";
					} return true;	
					case PT_TERRAIN:
					{
						ptcKey += 7;
						if ( *ptcKey == '.' )	// do we have an argument?
						{
							SKIP_SEPARATORS( ptcKey );
							if ( !strnicmp( ptcKey, "Z", 1 ))
							{
								sVal.FormatVal( pMeter->m_z );
								return true;
							}

							return false;
						}
						else
						{
							sVal.FormatHex( pMeter->m_wTerrainIndex );
						}
					} return true;
				}
			}
			return false;
		}
	}

	return true;
}

bool CPointBase::r_LoadVal( lpctstr ptcKey, lpctstr pszArgs )
{
	ADDTOCALLSTACK("CPointBase::r_LoadVal");
	int index = FindTableSorted( ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 );
	if ( index < 0 )
		return false;

	int iVal = Exp_GetVal(pszArgs);
	switch (index)
	{
		case 0: m_map = (uchar)iVal; break;
		case 1: m_x = (short)iVal; break;
		case 2: m_y = (short)iVal; break;
		case 3: m_z = (char)iVal; break;
	}
	return true;
}


DIR_TYPE CPointBase::GetDir( const CPointBase & pt, DIR_TYPE DirDefault ) const // Direction to point pt
{
	ADDTOCALLSTACK_INTENSIVE("CPointBase::GetDir");
	// Get the 2D direction between points.
	const int dx = (m_x-pt.m_x);
    const int dy = (m_y-pt.m_y);

    const int ax = abs(dx);
    const int ay = abs(dy);

	if ( ay > ax )
	{
		if ( ! ax )
		{
			return(( dy > 0 ) ? DIR_N : DIR_S );
		}
		int slope = ay / ax;
		if ( slope > 2 )
			return(( dy > 0 ) ? DIR_N : DIR_S );
		if ( dx > 0 )	// westish
		{
			return(( dy > 0 ) ? DIR_NW : DIR_SW );
		}
		return(( dy > 0 ) ? DIR_NE : DIR_SE );
	}
	else
	{
		if ( ! ay )
		{
			if ( ! dx )
				return( DirDefault );	// here ?
			return(( dx > 0 ) ? DIR_W : DIR_E );
		}
		int slope = ax / ay;
		if ( slope > 2 )
			return(( dx > 0 ) ? DIR_W : DIR_E );
		if ( dy > 0 )
		{
			return(( dx > 0 ) ? DIR_NW : DIR_NE );
		}
		return(( dx > 0 ) ? DIR_SW : DIR_SE );
	}
}

int CPointBase::StepLinePath( const CPointBase & ptSrc, int iSteps )
{
	ADDTOCALLSTACK("CPointBase::StepLinePath");
	// Take x steps toward this point.
	int dx = m_x - ptSrc.m_x;
	int dy = m_y - ptSrc.m_y;
	int iDist2D = GetDist( ptSrc );
	if ( ! iDist2D )
		return 0;

	m_x = (short)(ptSrc.m_x + IMulDivLL( iSteps, dx, iDist2D ));
	m_y = (short)(ptSrc.m_y + IMulDivLL( iSteps, dy, iDist2D ));
	return iDist2D;
}

tchar * CPointBase::WriteUsed( tchar * ptcBuffer, uint uiBufferLen) const
{
	ADDTOCALLSTACK_INTENSIVE("CPointBase::WriteUsed");
	if ( m_map )
		snprintf(ptcBuffer, uiBufferLen, "%" PRId16 ",%" PRId16 ",%" PRId8 ",%" PRIu8, m_x, m_y, m_z, m_map);
	else if ( m_z )
		snprintf(ptcBuffer, uiBufferLen, "%" PRId16 ",%" PRId16 ",%" PRId8, m_x, m_y, m_z);
	else
		snprintf(ptcBuffer, uiBufferLen, "%" PRId16 ",%" PRId16, m_x, m_y);
	return ptcBuffer;
}

lpctstr CPointBase::WriteUsed() const
{
	return WriteUsed( Str_GetTemp(), STR_TEMPLENGTH );
}

int CPointBase::Read( tchar * pszVal )
{
	ADDTOCALLSTACK("CPointBase::Read");
	// parse reading the point
	// NOTE: do not use = as a separator here !
    CPointBase ptTest;
    ptTest.m_z = 0;
    ptTest.m_map = 0;

	tchar * ppVal[4];
	int iArgs = Str_ParseCmds( pszVal, ppVal, CountOf( ppVal ), " ,\t" );
	switch ( iArgs )
	{
		default:
		case 4:	// m_map
			if ( IsDigit(ppVal[3][0]))
			{
                ptTest.m_map = (uchar)(Str_ToUI(ppVal[3]));
				if ( !g_MapList.IsMapSupported(ptTest.m_map) )
				{
					g_Log.EventError("Unsupported map #%d specified. Auto-fixing that to 0.\n", ptTest.m_map);
                    ptTest.m_map = 0;
				}
			}
			FALLTHROUGH;
		case 3: // m_z
			if (IsDigit(ppVal[2][0]) || ppVal[2][0] == '-')
			{
				ptTest.m_z = (char)(Str_ToI(ppVal[2]));
			}
			FALLTHROUGH;
		case 2:
			if (IsDigit(ppVal[1][0]))
			{
				ptTest.m_y = (short)(Str_ToI(ppVal[1]));
			}
			FALLTHROUGH;
		case 1:
			if (IsDigit(ppVal[0][0]))
			{
				ptTest.m_x = (short)(Str_ToI(ppVal[0]));
			}
			FALLTHROUGH;
		case 0:
			break;
	}
    
    if (!ptTest.IsValidPoint())
    {
        InitPoint();
        return 0;
    }

    Set(ptTest);
	return iArgs;
}

CSector * CPointBase::GetSector() const
{
	ADDTOCALLSTACK_INTENSIVE("CPointBase::GetSector");
	if ( !IsValidXY() )
	{
		g_Log.Event(LOGL_ERROR, "Point(%d,%d): trying to get a sector for point on map #%d out of bounds for this map(%d,%d). Defaulting to sector 0 of the map.\n",
			m_x, m_y, m_map, g_MapList.GetMapSizeX(m_map), g_MapList.GetMapSizeY(m_map));
		return CWorldMap::GetSector(m_map, 0);
	}

	// Get the world Sector we are in.
	return CWorldMap::GetSector(m_map, m_x, m_y);
}

CRegion * CPointBase::GetRegion( dword dwType ) const
{
	ADDTOCALLSTACK_INTENSIVE("CPointBase::GetRegion");
	// What region in the current CSector am i in ?
	// We only need to update this every 8 or so steps ?
	// REGION_TYPE_AREA
	if ( ! IsValidPoint() )
		return nullptr;

    const CSector *pSector = GetSector();
	if ( pSector )
		return pSector->GetRegion(*this, dwType);

	return nullptr;
}

size_t CPointBase::GetRegions( dword dwType, CRegionLinks *pRLinks ) const
{
	ADDTOCALLSTACK_INTENSIVE("CPointBase::GetRegions");
	if ( !IsValidPoint() )
		return 0;

	const CSector *pSector = GetSector();
	if ( pSector )
		return pSector->GetRegions(*this, dwType, pRLinks);

	return 0;
}

int CPointBase::GetPointSortIndex() const noexcept
{
	return (int)MAKEDWORD( m_x, m_y );
}


//*************************************************************************
// -CPointSort

CPointSort::CPointSort(short x, short y, char z, uchar map ) noexcept :
    CPointMap(x, y, z, map)
{
}

CPointSort::CPointSort(const CPointBase& pt) noexcept :
	CPointMap(pt.m_x, pt.m_y, pt.m_z, pt.m_map)
{
}