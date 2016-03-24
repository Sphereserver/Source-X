
#pragma once
#ifndef _INC_CITEMMULTI_H
#define _INC_CITEMMULTI_H

#include "CItem.h"

#define MAX_MULTI_LIST_OBJS 128
#define MAX_MULTI_CONTENT 1024


class CItemMulti : public CItem
{
	// IT_MULTI IT_SHIP
	// A ship or house etc.
private:
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];

protected:
	CRegionWorld * m_pRegion;		// we own this region.
	bool Multi_IsPartOf( const CItem * pItem ) const;
	void MultiUnRealizeRegion();
	bool MultiRealizeRegion();
	CItem * Multi_FindItemType( IT_TYPE type ) const;
	CItem * Multi_FindItemComponent( int iComp ) const;
	const CItemBaseMulti * Multi_GetDef() const;
	bool Multi_CreateComponent( ITEMID_TYPE id, short dx, short dy, signed char dz, dword dwKeyCode );

public:
	int Multi_GetMaxDist() const;
	size_t  Multi_ListObjs(CObjBase ** ppObjList);
	struct ShipSpeed // speed of a ship
	{
		uchar period;	// time between movement
		uchar tiles;	// distance to move
	};
	ShipSpeed m_shipSpeed; // Speed of ships (IT_SHIP)
	byte m_SpeedMode;

protected:
	virtual void OnComponentCreate( const CItem * pComponent );


public:
	static const char *m_sClassName;
	CItemMulti( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemMulti();

private:
	CItemMulti(const CItemMulti& copy);
	CItemMulti& operator=(const CItemMulti& other);

public:
	virtual bool OnTick();
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false); // Put item on the ground here.
	virtual void OnMoveFrom();	// Moving from current location.
	void OnHearRegion( lpctstr pszCmd, CChar * pSrc );
	CItem * Multi_GetSign();	// or Tiller

	void Multi_Create( CChar * pChar, dword dwKeyCode );
	static const CItemBaseMulti * Multi_GetDef( ITEMID_TYPE id );

	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
	virtual void DupeCopy( const CItem * pItem );
};
#endif // _INC_CITEMMULTI_H
