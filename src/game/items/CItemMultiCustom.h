/**
* @file CItemMultiCustom.h
*
*/

#pragma once
#ifndef _INC_CITEMMULTICUSTOM_H
#define _INC_CITEMMULTICUSTOM_H

#include "CItemMulti.h"


class PacketHouseDesign;

class CItemMultiCustom : public CItemMulti
{
	// IT_MULTI_CUSTOM
	// A customizable multi
public:
	struct Component
	{
		CUOMultiItemRec_HS m_item;
		short m_isStair;
		bool m_isFloor;
	};

private:
	typedef std::vector<Component*> ComponentsContainer;
	struct DesignDetails
	{
		int m_iRevision;
		ComponentsContainer m_vectorComponents;
		PacketHouseDesign* m_pData;
		int m_iDataRevision;
	};

	class CSphereMultiCustom : public CSphereMulti
	{
	public:
		void LoadFrom(DesignDetails * pDesign);

	public:
		CSphereMultiCustom() { };
	private:
		CSphereMultiCustom(const CSphereMultiCustom& copy);
		CSphereMultiCustom& operator=(const CSphereMultiCustom& other);
	};

private:
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];

	DesignDetails m_designMain;
	DesignDetails m_designWorking;
	DesignDetails m_designBackup;
	DesignDetails m_designRevert;

	CClient * m_pArchitect;
	CRectMap m_rectDesignArea;
	CSphereMultiCustom * m_pSphereMulti;

	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s  );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	const CPointMap GetComponentPoint(Component * pComponent) const;
	const CPointMap GetComponentPoint(short dx, short dy, char dz) const;
	void CopyDesign(DesignDetails * designFrom, DesignDetails * designTo);

private:
	typedef std::map<ITEMID_TYPE,int> ValidItemsContainer;	// ItemID, FeatureMask
	static ValidItemsContainer sm_mapValidItems;
	static bool LoadValidItems();

public:
	static const char *m_sClassName;
	CItemMultiCustom( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemMultiCustom();

private:
	CItemMultiCustom(const CItemMultiCustom& copy);
	CItemMultiCustom& operator=(const CItemMultiCustom& other);

public:
	void BeginCustomize( CClient * pClientSrc );
	void EndCustomize( bool bForce = false );
	void SwitchToLevel( CClient * pClientSrc, uchar iLevel );
	void CommitChanges( CClient * pClientSrc = NULL );
	void AddItem( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z = INT8_MIN, short iStairID = 0 );
	void AddStairs( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z = INT8_MIN, short iStairID = -1 );
	void AddRoof( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z );
	void RemoveItem( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z );
	bool RemoveStairs( Component * pStairComponent );
	void RemoveRoof( CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z );
	void SendVersionTo( CClient * pClientSrc );
	void SendStructureTo( CClient * pClientSrc );
	void BackupStructure();
	void RestoreStructure( CClient * pClientSrc = NULL );
	void RevertChanges( CClient * pClientSrc = NULL );
	void ResetStructure( CClient * pClientSrc = NULL );

	const CSphereMultiCustom * GetMultiItemDefs();
	const CRect GetDesignArea();
	size_t GetFixtureCount(DesignDetails * pDesign = NULL);
	size_t GetComponentsAt(short dx, short dy, char dz, Component ** pComponents, DesignDetails * pDesign = NULL);
	int GetRevision(const CClient * pClientSrc = NULL) const;
	uchar GetLevelCount();
	short GetStairCount();

	static uchar GetPlane( char z );
	static uchar GetPlane( Component * pComponent );
	static char GetPlaneZ( uchar plane );
	static bool IsValidItem( ITEMID_TYPE id, CClient * pClientSrc, bool bMulti );
};


#endif // _INC_CITEMMULTICUSTOM_H
