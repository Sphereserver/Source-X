/**
* @file CItemVendable.h
*
*/

#ifndef _INC_CITEMVENDABLE_H
#define _INC_CITEMVENDABLE_H

#include "CItem.h"


class CItemVendable : public CItem
{
	// Any item that can be sold and has value.
private:
	static lpctstr const sm_szLoadKeys[];
	word m_quality;		// 0-100 quality.
	dword m_price;		// The price of this item if on a vendor. (allow random (but remembered) pluctuations)

public:
	static const char *m_sClassName;
	CItemVendable( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemVendable();

	CItemVendable(const CItemVendable& copy) = delete;
	CItemVendable& operator=(const CItemVendable& other) = delete;

public:
	word GetQuality() const;
	void SetQuality( word quality = 0 );

	void SetPlayerVendorPrice( dword dwVal );
	dword GetBasePrice() const;
	dword GetVendorPrice( int iConvertFactor , bool forselling); 

	bool IsValidSaleItem( bool fBuyFromVendor ) const;
	bool IsValidNPCSaleItem() const;

	virtual void DupeCopy( const CObjBase * pItem ) override;  // overriding CItem::DupeCopy

	void Restock( bool fSellToPlayers );
	virtual void r_Write( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_LoadVal( CScript & s ) override;
};


#endif // _INC_CITEMVENDABLE_H
