#ifndef _INC_COBJBASETEMPLATE_H
#define _INC_COBJBASETEMPLATE_H

#include "CArray.h"
#include "graycom.h"
#include "CString.h"
#include "CRegion.h"
#include "CGrayUID.h"

class CObjBaseTemplate : public CGObListRec
{
	// A dynamic object of some sort.
private:
	CGrayUID	m_UID;		// How the server will refer to this. 0 = static item
	CGString	m_sName;	// unique name for the individual object.
	CPointMap	m_pt;		// List is sorted by m_z_sort.
protected:
	void DupeCopy( const CObjBaseTemplate * pObj )
	{
		// NOTE: Never copy m_UID
		ASSERT(pObj);
		m_sName = pObj->m_sName;
		m_pt = pObj->m_pt;
	}

	void SetUID( DWORD dwIndex )
	{
		// don't set container flags through here.
		m_UID.SetObjUID( dwIndex );	// Will have UID_F_ITEM as well.
	}
	void SetUnkZ( signed char z )
	{
		m_pt.m_z = z;
	}

public:
	static const char *m_sClassName;
	CObjBaseTemplate()
	{
	}
	virtual ~CObjBaseTemplate()
	{
	}

private:
	CObjBaseTemplate(const CObjBaseTemplate& copy);
	CObjBaseTemplate& operator=(const CObjBaseTemplate& other);

public:
	CObjBaseTemplate * GetNext() const
	{
		return( STATIC_CAST <CObjBaseTemplate*> ( CGObListRec::GetNext()));
	}
	CObjBaseTemplate * GetPrev() const
	{
		return( STATIC_CAST <CObjBaseTemplate*> ( CGObListRec::GetPrev()));
	}

	CGrayUID GetUID() const			{	return( m_UID ); }
	bool IsItem() const				{	return( m_UID.IsItem()); }
	bool IsChar() const				{	return( m_UID.IsChar()); }
	bool IsItemInContainer() const	{	return( m_UID.IsItemInContainer() ); }
	bool IsItemEquipped() const		{	return( m_UID.IsItemEquipped() ); }
	bool IsDisconnected() const		{	return( m_UID.IsObjDisconnected() ); }
	bool IsTopLevel() const			{	return( m_UID.IsObjTopLevel() ); }
	bool IsValidUID() const			{	return( m_UID.IsValidUID() ); }
	bool IsDeleted() const;

	void SetContainerFlags( DWORD dwFlags = 0 )
	{
		m_UID.SetObjContainerFlags( dwFlags );
	}

	virtual int IsWeird() const;
	virtual CObjBaseTemplate * GetTopLevelObj() const = 0;

	CSector * GetTopSector() const
	{
		return GetTopLevelObj()->GetTopPoint().GetSector();
	}
	// Location

	LAYER_TYPE GetEquipLayer() const
	{
		return static_cast<LAYER_TYPE>(m_pt.m_z);
	}
	void SetEquipLayer( LAYER_TYPE layer )
	{
		SetContainerFlags( UID_O_EQUIPPED );
		m_pt.m_x = 0;	// these don't apply.
		m_pt.m_y = 0;
		// future: strongly typed enums will remove the need for this cast
		m_pt.m_z = static_cast<signed char>(layer); // layer equipped.
		m_pt.m_map = 0;
	}

	BYTE GetContainedLayer() const
	{
		// used for corpse or Restock count as well in Vendor container.
		return( m_pt.m_z );
	}
	void SetContainedLayer( BYTE layer )
	{
		// used for corpse or Restock count as well in Vendor container.
		m_pt.m_z = layer;
	}
	const CPointMap & GetContainedPoint() const
	{
		return( m_pt );
	}
	void SetContainedPoint( const CPointMap & pt )
	{
		SetContainerFlags( UID_O_CONTAINED );
		m_pt.m_x = pt.m_x;
		m_pt.m_y = pt.m_y;
		m_pt.m_z = LAYER_NONE;
		m_pt.m_map = 0;
	}

	void SetTopPoint( const CPointMap & pt )
	{
		SetContainerFlags(0);
		ASSERT( pt.IsValidPoint() );	// already checked b4.
		m_pt = pt;
	}
	const CPointMap & GetTopPoint() const
	{
		return( m_pt );
	}
	virtual void SetTopZ( signed char z )
	{
		m_pt.m_z = z;
	}
	signed char GetTopZ() const
	{
		return( m_pt.m_z );
	}
	unsigned char GetTopMap() const
	{
		return( m_pt.m_map );
	}

	void SetUnkPoint( const CPointMap & pt )
	{
		m_pt = pt;
	}
	const CPointMap & GetUnkPoint() const
	{
		// don't care where this
		return( m_pt );
	}
	signed char GetUnkZ() const	// Equal to GetTopZ ?
	{
		return( m_pt.m_z );
	}

	// Distance and direction
	int GetTopDist( const CPointMap & pt ) const
	{
		return( GetTopPoint().GetDist( pt ));
	}

	int GetTopDist( const CObjBaseTemplate * pObj ) const
	{
		// don't check for logged out.
		// Assume both already at top level.
		ASSERT( pObj );
		if ( pObj->IsDisconnected())
			return( SHRT_MAX );
		return( GetTopPoint().GetDist( pObj->GetTopPoint()));
	}

	int GetTopDistSight( const CPointMap & pt ) const
	{
		return( GetTopPoint().GetDistSight( pt ));
	}

	int GetTopDistSight( const CObjBaseTemplate * pObj ) const
	{
		// don't check for logged out.
		// Assume both already at top level.
		ASSERT( pObj );
		if ( pObj->IsDisconnected())
			return( SHRT_MAX );
		return( GetTopPoint().GetDistSight( pObj->GetTopPoint()));
	}

	int GetDist( const CObjBaseTemplate * pObj ) const
	{
		// logged out chars have infinite distance
		if ( pObj == NULL )
			return( SHRT_MAX );
		pObj = pObj->GetTopLevelObj();
		if ( pObj->IsDisconnected())
			return( SHRT_MAX );
		return( GetTopDist( pObj ));
	}

	int GetTopDist3D( const CObjBaseTemplate * pObj ) const // 3D Distance between points
	{
		// logged out chars have infinite distance
		// Assume both already at top level.
		ASSERT( pObj );
		if ( pObj->IsDisconnected())
			return( SHRT_MAX );
		return( GetTopPoint().GetDist3D( pObj->GetTopPoint()));
	}

	DIR_TYPE GetTopDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const
	{
		ASSERT( pObj );
		return( GetTopPoint().GetDir( pObj->GetTopPoint(), DirDefault ));
	}

	DIR_TYPE GetDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const
	{
		ASSERT( pObj );
		pObj = pObj->GetTopLevelObj();
		return( GetTopDir( pObj, DirDefault ));
	}

	virtual int GetVisualRange() const
	{
		return( UO_MAP_VIEW_SIZE );
	}

	// Names
	LPCTSTR GetIndividualName() const
	{
		return( m_sName );
	}
	bool IsIndividualName() const
	{
		return( ! m_sName.IsEmpty());
	}
	virtual LPCTSTR GetName() const
	{
		return( m_sName );
	}
	virtual bool SetName( LPCTSTR pszName )
	{
		// NOTE: Name length <= MAX_NAME_SIZE
		if ( !pszName )
			return false;
		m_sName = pszName;
		return true;
	}
};

#endif // _INC_COBJBASETEMPLATE_H
