/**
* @file CItemContainer.h
*
*/

#pragma once
#ifndef _INC_CITEMCONTAINER_H
#define _INC_CITEMCONTAINER_H

#include "../CContainer.h"
#include "CItemVendable.h"


class CItemContainer : public CItemVendable, public CContainer
{
	// This item has other items inside it.
	static lpctstr const sm_szVerbKeys[];
public:
	static const char *m_sClassName;
	// bool m_fTinkerTrapped;	// magic trap is diff.
	bool NotifyDelete();
	void DeletePrepare();

public:
	CItemContainer( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemContainer()
	{
		DeleteAll();	// get rid of my contents first to protect against weight calc errors.
		DeletePrepare();
	}

private:
	CItemContainer(const CItemContainer& copy);
	CItemContainer& operator=(const CItemContainer& other);

public:
	bool IsWeighed() const;
	bool IsSearchable() const;

	bool IsItemInside(const CItem * pItem) const;
	bool CanContainerHold(const CItem * pItem, const CChar * pCharMsg );

	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );
	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc );
	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );

	virtual int GetWeight(word amount = 0) const;
	void OnWeightChange( int iChange );

	void ContentAdd( CItem * pItem );
	void ContentAdd( CItem * pItem, CPointMap pt, uchar gridIndex = 0 );
protected:
	void OnRemoveObj( CSObjListRec* pObRec );	// Override this = called when removed from list.
public:
	bool IsItemInTrade() const;
	void Trade_Status( bool bCheck );
	void Trade_UpdateGold( dword platinum, dword gold );
	void Trade_Delete();

	void MakeKey();
	void SetKeyRing();
	void Game_Create();
	void Restock();
	bool OnTick();

	virtual void DupeCopy( const CItem * pItem );

	CPointMap GetRandContainerLoc() const;

	void OnOpenEvent( CChar * pCharOpener, const CObjBaseTemplate * pObjTop );
};


#endif // _INC_CITEMCONTAINER_H
