
#pragma once
#ifndef _INC_COBJBASETEMPLATE_H
#define _INC_COBJBASETEMPLATE_H

#include "CArray.h"
//#include "graycom.h"
#include "CString.h"
#include "CRect.h"
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

	void SetUID( dword dwIndex );
	void SetUnkZ( char z );

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

	CGrayUID GetUID() const;
	bool IsItem() const;
	bool IsChar() const;
	bool IsItemInContainer() const;
	bool IsItemEquipped() const;
	bool IsDisconnected() const;
	bool IsTopLevel() const;
	bool IsValidUID() const;
	bool IsDeleted() const;

	void SetContainerFlags( dword dwFlags = 0 );

	virtual int IsWeird() const;
	virtual CObjBaseTemplate * GetTopLevelObj() const = 0;

	CSector * GetTopSector() const;
	// Location

	LAYER_TYPE GetEquipLayer() const;
	void SetEquipLayer( LAYER_TYPE layer );

	byte GetContainedLayer() const;
	void SetContainedLayer( byte layer );
	const CPointMap & GetContainedPoint() const;
	void SetContainedPoint( const CPointMap & pt );

	void SetTopPoint( const CPointMap & pt );
	const CPointMap & GetTopPoint() const;
	virtual void SetTopZ( char z );
	char GetTopZ() const;
	uchar GetTopMap() const;

	void SetUnkPoint( const CPointMap & pt );
	const CPointMap & GetUnkPoint() const;
	char GetUnkZ() const;

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
	lpctstr GetIndividualName() const;
	bool IsIndividualName() const;
	virtual lpctstr GetName() const;
	virtual bool SetName( lpctstr pszName );
};

#endif // _INC_COBJBASETEMPLATE_H
