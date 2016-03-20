
#pragma once
#ifndef CITEMVENDABLE_H
#define CITEMVENDABLE_H

#include "CItem.h"
#include "CItemBase.h"


class CItemVendable : public CItem
{
	// Any item that can be sold and has value.
private:
	static LPCTSTR const sm_szLoadKeys[];
	WORD m_quality;		// 0-100 quality.
	DWORD m_price;		// The price of this item if on a vendor. (allow random (but remembered) pluctuations)

public:
	static const char *m_sClassName;
	CItemVendable( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemVendable();

private:
	CItemVendable(const CItemVendable& copy);
	CItemVendable& operator=(const CItemVendable& other);

public:

	WORD GetQuality() const;
	void SetQuality(WORD quality = 0);

	void SetPlayerVendorPrice( DWORD dwVal );
	DWORD GetBasePrice() const;
	DWORD GetVendorPrice( int iConvertFactor );

	bool IsValidSaleItem( bool fBuyFromVendor ) const;
	bool IsValidNPCSaleItem() const;

	virtual void DupeCopy( const CItem * pItem );

	void Restock( bool fSellToPlayers );
	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
};


#endif // CITEMVENDABLE_H
