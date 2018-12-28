
#include "../game/uo_files/uofiles_macros.h"
#include "CObjBaseTemplate.h"

void CObjBaseTemplate::DupeCopy( const CObjBaseTemplate * pObj )
{
	// NOTE: Never copy m_UID
	ASSERT(pObj);
	m_sName = pObj->m_sName;
	m_pt = pObj->m_pt;
}

void CObjBaseTemplate::SetUID( dword dwIndex )
{
	// don't set container flags through here.
	m_UID.SetObjUID( dwIndex );	// Will have UID_F_ITEM as well.
}

void CObjBaseTemplate::SetUnkZ( char z )
{
	m_pt.m_z = z;
}

CObjBaseTemplate::CObjBaseTemplate()
{
}

CObjBaseTemplate::~CObjBaseTemplate()
{
}

CObjBaseTemplate * CObjBaseTemplate::GetNext() const
{
	return ( static_cast <CObjBaseTemplate*> ( CSObjListRec::GetNext()) );
}
CObjBaseTemplate * CObjBaseTemplate::GetPrev() const
{
	return ( static_cast <CObjBaseTemplate*> ( CSObjListRec::GetPrev()) );
}

CSector * CObjBaseTemplate::GetTopSector() const
{
	return GetTopLevelObj()->GetTopPoint().GetSector();
}
// Location

void CObjBaseTemplate::SetEquipLayer( LAYER_TYPE layer )
{
	SetUIDContainerFlags( UID_O_EQUIPPED );
	m_pt.m_x = 0;	// these don't apply.
	m_pt.m_y = 0;
	// future: strongly typed enums will remove the need for this cast
	m_pt.m_z = (char)(layer); // layer equipped.
	m_pt.m_map = 0;
}

void CObjBaseTemplate::SetContainedLayer( byte layer )
{
	// used for corpse or Restock count as well in Vendor container.
	m_pt.m_z = layer;
}

void CObjBaseTemplate::SetContainedPoint( const CPointMap & pt )
{
    SetUIDContainerFlags( UID_O_CONTAINED );
	m_pt.m_x = pt.m_x;
	m_pt.m_y = pt.m_y;
	m_pt.m_z = LAYER_NONE;
	m_pt.m_map = 0;
}

void CObjBaseTemplate::SetTopPoint( const CPointMap & pt )
{
    SetUIDContainerFlags(0);
	ASSERT( pt.IsValidPoint() );	// already checked b4.
	m_pt = pt;
}

void CObjBaseTemplate::SetTopZ( char z )
{
	m_pt.m_z = z;
}

char CObjBaseTemplate::GetTopZ() const
{
	return m_pt.m_z;
}

uchar CObjBaseTemplate::GetTopMap() const
{
	return m_pt.m_map;
}

void CObjBaseTemplate::SetUnkPoint( const CPointMap & pt )
{
	m_pt = pt;
}

char CObjBaseTemplate::GetUnkZ() const	// Equal to GetTopZ ?
{
	return m_pt.m_z;
}

// Distance and direction
int CObjBaseTemplate::GetTopDist( const CPointMap & pt ) const
{
	return GetTopPoint().GetDist( pt );
}

int CObjBaseTemplate::GetTopDist( const CObjBaseTemplate * pObj ) const
{
	// don't check for logged out.
	// Assume both already at top level.
	ASSERT( pObj );
	if ( pObj->IsDisconnected())
		return( INT16_MAX );
	return GetTopPoint().GetDist( pObj->GetTopPoint());
}

int CObjBaseTemplate::GetTopDistSight( const CPointMap & pt ) const
{
	return GetTopPoint().GetDistSight( pt );
}

int CObjBaseTemplate::GetTopDistSight( const CObjBaseTemplate * pObj ) const
{
	// don't check for logged out.
	// Assume both already at top level.
	ASSERT( pObj );
	if ( pObj->IsDisconnected())
		return INT16_MAX ;
	return GetTopPoint().GetDistSight( pObj->GetTopPoint());
}

int CObjBaseTemplate::GetDist( const CObjBaseTemplate * pObj ) const
{
	// logged out chars have infinite distance
	if ( pObj == nullptr )
		return INT16_MAX ;
	pObj = pObj->GetTopLevelObj();
	if ( pObj->IsDisconnected())
		return INT16_MAX ;
	return GetTopDist( pObj );
}

int CObjBaseTemplate::GetTopDist3D( const CObjBaseTemplate * pObj ) const // 3D Distance between points
{
	// logged out chars have infinite distance
	// Assume both already at top level.
	ASSERT( pObj );
	if ( pObj->IsDisconnected())
		return INT16_MAX ;
	return GetTopPoint().GetDist3D( pObj->GetTopPoint());
}

DIR_TYPE CObjBaseTemplate::GetTopDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault ) const
{
	ASSERT( pObj );
	return GetTopPoint().GetDir( pObj->GetTopPoint(), DirDefault );
}

DIR_TYPE CObjBaseTemplate::GetDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault ) const
{
	ASSERT( pObj );
	pObj = pObj->GetTopLevelObj();
	return GetTopDir( pObj, DirDefault );
}

int CObjBaseTemplate::GetVisualRange() const
{
	return UO_MAP_VIEW_SIZE_DEFAULT;
}

// Names
lpctstr CObjBaseTemplate::GetIndividualName() const
{
	return m_sName;
}

bool CObjBaseTemplate::IsIndividualName() const
{
	return ! m_sName.IsEmpty();
}

lpctstr CObjBaseTemplate::GetName() const
{
	return m_sName;
}

bool CObjBaseTemplate::SetName( lpctstr pszName )
{
	// NOTE: Name length <= MAX_NAME_SIZE
	if ( !pszName )
		return false;
	m_sName = pszName;
	return true;
}