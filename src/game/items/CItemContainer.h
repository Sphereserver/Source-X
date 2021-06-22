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

	virtual bool NotifyDelete() override;	// overrides CItem:: method
	virtual void DeletePrepare() override;

    void SetSecuredOfMulti(const CUID& uidMulti);
    void SetCrateOfMulti(const CUID& uidMulti);

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

	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;
	virtual void r_Write( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;

	virtual int GetWeight(word amount = 0) const;
	void OnWeightChange( int iChange );

	// Contents/Carry stuff. ---------------------------------
public:
	virtual void ContentAdd( CItem * pItem, bool bForceNoStack = false ) override;
	void ContentAdd( CItem * pItem, CPointMap pt, bool fForceNoStack = false, uchar gridIndex = 0 );
protected:
	virtual void OnRemoveObj( CSObjContRec* pObRec ) override;	// Override this = called when removed from list.

protected:
	virtual void _GoAwake() override final;
	virtual void _GoSleep() override final;

public:
	bool IsItemInTrade() const;
	void Trade_Status( bool bCheck );
	void Trade_UpdateGold( dword platinum, dword gold );
	bool Trade_Delete();

	void MakeKey();
	void SetKeyRing();
	void Game_Create();
	void Restock();
	bool _OnTick();

	virtual void DupeCopy( const CItem * pItem ) override;  // overriding CItem::DupeCopy

	CPointMap GetRandContainerLoc() const;

	void OnOpenEvent( CChar * pCharOpener, const CObjBaseTemplate * pObjTop );
};


#endif // _INC_CITEMCONTAINER_H
