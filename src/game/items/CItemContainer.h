/**
* @file CItemContainer.h
*
*/

#ifndef _INC_CITEMCONTAINER_H
#define _INC_CITEMCONTAINER_H

#include "../CContainer.h"
#include "CItemVendable.h"


class CItemContainer : public CItemVendable, public CContainer
{
	// This item has other items inside it.
	static lpctstr const sm_szVerbKeys[];
    CUID _uidMultiSecured;
    CUID _uidMultiCrate;
public:
	static const char *m_sClassName;

	// bool m_fTinkerTrapped;	// magic trap is diff.

	virtual bool NotifyDelete() override;
	virtual void DeletePrepare() override;

    void SetSecuredOfMulti(CUID uidMulti);
    void SetCrateOfMulti(CUID uidMulti);

public:
	CItemContainer( ITEMID_TYPE id, CItemBase * pItemDef );
    virtual ~CItemContainer();

private:
	CItemContainer(const CItemContainer& copy);
	CItemContainer& operator=(const CItemContainer& other);

public:
	bool IsWeighed() const;
	bool IsSearchable() const;

	bool IsItemInside(const CItem * pItem) const;
	bool CanContainerHold(const CItem * pItem, const CChar * pCharMsg );

	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );
	virtual void r_Write( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false );
	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );

	virtual int GetWeight(word amount = 0) const;
	void OnWeightChange( int iChange );

	void ContentAdd( CItem * pItem, bool bForceNoStack = false );
	void ContentAdd( CItem * pItem, CPointMap pt, bool bForceNoStack = false, uchar gridIndex = 0 );
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

	virtual void DupeCopy( const CItem * pItem ) override;  // overriding CItem::DupeCopy

	CPointMap GetRandContainerLoc() const;

	void OnOpenEvent( CChar * pCharOpener, const CObjBaseTemplate * pObjTop );
};


#endif // _INC_CITEMCONTAINER_H
