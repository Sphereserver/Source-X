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
	void DupeCopy( const CObjBaseTemplate * pObj );

	void SetUID( DWORD dwIndex );
	void SetUnkZ( signed char z );

public:
	static const char *m_sClassName;
	CObjBaseTemplate();
	virtual ~CObjBaseTemplate();

private:
	CObjBaseTemplate(const CObjBaseTemplate& copy);
	CObjBaseTemplate& operator=(const CObjBaseTemplate& other);

public:
	CObjBaseTemplate * GetNext() const;
	CObjBaseTemplate * GetPrev() const;

	CGrayUID GetUID() const			{	return( m_UID ); }
	bool IsItem() const				{	return( m_UID.IsItem()); }
	bool IsChar() const				{	return( m_UID.IsChar()); }
	bool IsItemInContainer() const	{	return( m_UID.IsItemInContainer() ); }
	bool IsItemEquipped() const		{	return( m_UID.IsItemEquipped() ); }
	bool IsDisconnected() const		{	return( m_UID.IsObjDisconnected() ); }
	bool IsTopLevel() const			{	return( m_UID.IsObjTopLevel() ); }
	bool IsValidUID() const			{	return( m_UID.IsValidUID() ); }
	bool IsDeleted() const;

	void SetContainerFlags( DWORD dwFlags = 0 );

	virtual int IsWeird() const;
	virtual CObjBaseTemplate * GetTopLevelObj() const = 0;

	CSector * GetTopSector() const;
	// Location

	LAYER_TYPE GetEquipLayer() const;
	void SetEquipLayer( LAYER_TYPE layer );

	BYTE GetContainedLayer() const;
	void SetContainedLayer( BYTE layer );
	const CPointMap & GetContainedPoint() const;
	void SetContainedPoint( const CPointMap & pt );

	void SetTopPoint( const CPointMap & pt );
	const CPointMap & GetTopPoint() const;
	virtual void SetTopZ( signed char z );
	signed char GetTopZ() const;
	unsigned char GetTopMap() const;

	void SetUnkPoint( const CPointMap & pt );
	const CPointMap & GetUnkPoint() const;
	signed char GetUnkZ() const;

	// Distance and direction
	int GetTopDist( const CPointMap & pt ) const;

	int GetTopDist( const CObjBaseTemplate * pObj ) const;

	int GetTopDistSight( const CPointMap & pt ) const;

	int GetTopDistSight( const CObjBaseTemplate * pObj ) const;

	int GetDist( const CObjBaseTemplate * pObj ) const;

	int GetTopDist3D( const CObjBaseTemplate * pObj ) const;

	DIR_TYPE GetTopDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const;

	DIR_TYPE GetDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const;

	virtual int GetVisualRange() const;

	// Names
	LPCTSTR GetIndividualName() const;
	bool IsIndividualName() const;
	virtual LPCTSTR GetName() const;
	virtual bool SetName( LPCTSTR pszName );
};

#endif // _INC_COBJBASETEMPLATE_H
