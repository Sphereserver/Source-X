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
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

protected:
	CRegionWorld * m_pRegion;		// we own this region.
	bool Multi_IsPartOf( const CItem * pItem ) const;
	void MultiUnRealizeRegion();
	bool MultiRealizeRegion();
	CItem * Multi_FindItemType( IT_TYPE type ) const;
	CItem * Multi_FindItemComponent( int iComp ) const;
	const CItemBaseMulti * Multi_GetDef() const;
	bool Multi_CreateComponent( ITEMID_TYPE id, signed short dx, signed short dy, signed char dz, DWORD dwKeyCode );

public:
	int Multi_GetMaxDist() const;
	size_t  Multi_ListObjs(CObjBase ** ppObjList);
	struct ShipSpeed // speed of a ship
	{
		unsigned char period;	// time between movement
		unsigned char tiles;	// distance to move
	};
	ShipSpeed m_shipSpeed; // Speed of ships (IT_SHIP)
	BYTE m_SpeedMode;

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
	void OnHearRegion( LPCTSTR pszCmd, CChar * pSrc );
	CItem * Multi_GetSign();	// or Tiller

	void Multi_Create( CChar * pChar, DWORD dwKeyCode );
	static const CItemBaseMulti * Multi_GetDef( ITEMID_TYPE id );

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
	virtual void DupeCopy( const CItem * pItem );
};
#endif // _INC_CITEMMULTI_H
