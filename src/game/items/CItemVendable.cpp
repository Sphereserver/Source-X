
#include "../../common/CException.h"
#include "CItemVendable.h"

CItemVendable::CItemVendable( ITEMID_TYPE id, CItemBase * pDef ) :
    CTimedObject(PROFILE_ITEMS),
    CItem( id, pDef )
{
	// Constructor
	m_price = 0;
	m_quality = 0;
}

CItemVendable::~CItemVendable()
{
	// Nothing really to do...no dynamic memory has been allocated.
	DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
}

void CItemVendable::DupeCopy( const CObjBase * pItemObj )
{
	ADDTOCALLSTACK("CItemVendable::DupeCopy");
    auto pItem = dynamic_cast<const CItem*>(pItemObj);
    ASSERT(pItem);
	CItem::DupeCopy( pItem );

	const CItemVendable * pVendItem = dynamic_cast <const CItemVendable *>(pItem);
	if ( pVendItem == nullptr )
		return;

	m_price = pVendItem->m_price;
	m_quality = pVendItem->m_quality;
}

enum IVC_TYPE
{
	IVC_PRICE,
	IVC_QUALITY,
	IVC_QTY
};

lpctstr const CItemVendable::sm_szLoadKeys[IVC_QTY+1] =
{
	"PRICE",
	"QUALITY",
	nullptr
};

bool CItemVendable::r_WriteVal(lpctstr ptcKey, CSString &sVal, CTextConsole *pSrc, bool fNoCallParent, bool fNoCallChildren)
{
    UnreferencedParameter(fNoCallChildren);
	ADDTOCALLSTACK("CItemVendable::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
	{
	case IVC_PRICE:	// PRICE
		sVal.FormatVal( m_price );
		return true;
	case IVC_QUALITY:	// QUALITY
		sVal.FormatVal( GetQuality());
		return true;
	default:
		return (fNoCallParent ? false : CItem::r_WriteVal( ptcKey, sVal, pSrc ));
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemVendable::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemVendable::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
	{
	case IVC_PRICE:	// PRICE
		m_price = s.GetArgVal();
		return true;
	case IVC_QUALITY:	// QUALITY
		SetQuality( s.GetArgWVal());
		return true;
	}
	return CItem::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CItemVendable::r_Write(CScript &s)
{
	ADDTOCALLSTACK_DEBUG("CItemVendable::r_Write");
	CItem::r_Write(s);
	if ( GetQuality() != 0 )
		s.WriteKeyVal( "QUALITY", GetQuality());

	// am i on a vendor right now ?
	if ( m_price > 0 )
		s.WriteKeyVal( "PRICE", m_price );

	return;
}

void CItemVendable::Restock( bool fSellToPlayers )
{
	ADDTOCALLSTACK("CItemVendable::Restock");
	// This is on a non-pet vendor.
	// allow prices to fluctuate randomly (per vendor) but hold the values for a bit.

	ASSERT( IsItemInContainer());

	if ( fSellToPlayers )
	{
		// restock to my full amount.

		word iAmountRestock = GetContainedLayer();
		if ( ! iAmountRestock )
		{
			SetContainedLayer(1);
			iAmountRestock = 1;
		}
		if ( GetAmount() < iAmountRestock )
		{
			SetAmount( iAmountRestock );	// restock amount
		}
	}
	else
	{
		// Clear the amount i have bought from players.
		// GetAmount() is the max that i will buy in the next period.
		SetContainedLayer(0);
	}
}

void CItemVendable::SetPlayerVendorPrice( dword lPrice )
{
	ADDTOCALLSTACK("CItemVendable::SetPlayerVendorPrice");
	// This can only be inside a vendor container.
	m_price = maximum(lPrice, 0);
}

word CItemVendable::GetQuality() const
{
	return m_quality;
}

void CItemVendable::SetQuality(word quality)
{
	m_quality = quality;
}

dword CItemVendable::GetBasePrice() const
{
	ADDTOCALLSTACK("CItemVendable::GetBasePrice");
	// Get the price that the player set on his vendor
	return m_price;
}

dword CItemVendable::GetVendorPrice( int iConvertFactor , bool forselling )
{
	ADDTOCALLSTACK("CItemVendable::GetVendorPrice");
	// forselling = 0 Player is buying from a vendor.
	// forselling = 1 Player is selling to a vendor.
	// When selling an item, we must avoid use the Price. Price may be modify and can exploit.

	// Item is on the vendor or on the player's backpack depending  what we doing
	// Consider: (if not on a player vendor)
	//  Quality of the item.
	//  rareity of the item.
	// ARGS:
	// iConvertFactor will consider:
	//  Vendors Karma.
	//  Players Karma
	// -100 = reduce price by 100%   (player selling to vendor?)
	//    0 = base price
	// +100 = increase price by 100% (vendor selling to player?)

	CItemBase* pItemDef;
	llong llPrice = 0;

	//Check if there is an override value first
	const CVarDefCont* pVarDef = GetKey("OVERRIDE.VALUE", true);
	if (pVarDef)
		llPrice = (llong)pVarDef->GetValNum();
	else
	{
		if (!forselling) //When selling an item, you never check the price to avoid exploit
		{
			llPrice = m_price; // Price is set on player vendor
		}
	}

	if ( llPrice <= 0 )	// No price/overrride.value set, we use the value of item.
	{
		
		if ( IsType(IT_DEED) )
		{
			// Deeds just represent the item they are deeding.
			pItemDef = CItemBase::FindItemBase((ITEMID_TYPE)(ResGetIndex(m_itDeed.m_Type)));
			if ( !pItemDef )
				return 1;
		}
		else
			pItemDef = Item_GetDef();

		llPrice = pItemDef->GetMakeValue(GetQuality()); //If value is a range(ex:10,20), value change depending quality 
	}
	
	llPrice += IMulDivLL(llPrice, maximum(iConvertFactor, -100), 100);
	if ( llPrice > UINT32_MAX )
		return UINT32_MAX;
	else
		return (dword)llPrice;
}

bool CItemVendable::IsValidSaleItem( bool fBuyFromVendor ) const
{
	ADDTOCALLSTACK("CItemVendable::IsValidSaleItem");
	// Can this individual item be sold or bought ?
	if ( !IsMovableType() )
	{
		if ( fBuyFromVendor )
		{
			DEBUG_ERR(( "Vendor uid=0%x selling unmovable item %s='%s'\n", (dword)(GetTopLevelObj()->GetUID()), GetResourceName(), GetName()));
		}
		return false;
	}
	if ( !fBuyFromVendor )
	{
		// cannot sell these to a vendor.
		if ( IsAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER))
			return false;	// spellbooks !
	}
	if ( IsType(IT_COIN) )
		return false;
	return true;
}

bool CItemVendable::IsValidNPCSaleItem() const
{
	ADDTOCALLSTACK("CItemVendable::IsValidNPCSaleItem");
	// This item is in an NPC's vendor box.
	// Is it a valid item that NPC's should be selling ?

	CItemBase * pItemDef = Item_GetDef();

	if ( m_price <= 0 && pItemDef->GetMakeValue(0) <= 0 )
	{
		DEBUG_ERR(( "Vendor uid=0%x selling unpriced item %s='%s'\n", (dword)(GetTopLevelObj()->GetUID()), GetResourceName(), GetName()));
		return false;
	}

	if ( !IsValidSaleItem(true) )
	{
		DEBUG_ERR(( "Vendor uid=0%x selling bad item %s='%s'\n", (dword)(GetTopLevelObj()->GetUID()), GetResourceName(), GetName()));
		return false;
	}

	return true;
}
