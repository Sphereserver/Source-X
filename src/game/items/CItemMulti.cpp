
#include "../../common/CException.h"
#include "../../common/CUIDExtra.h"
#include "../../common/sphereproto.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"
#include "../CWorld.h"
#include "CItemMulti.h"

/////////////////////////////////////////////////////////////////////////////

CItemMulti::CItemMulti( ITEMID_TYPE id, CItemBase * pItemDef ) :	// CItemBaseMulti
	CItem( id, pItemDef )
{
	CItemBaseMulti * pItemBase = static_cast<CItemBaseMulti*>(Base_GetDef());
	m_shipSpeed.period = pItemBase->m_shipSpeed.period;
	m_shipSpeed.tiles = pItemBase->m_shipSpeed.tiles;
	m_SpeedMode = pItemBase->m_SpeedMode;

	m_pRegion = nullptr;
    _uidOwner.InitUID();
    _uidMovingCrate.InitUID();

    _fIsAddon = false;
    _iHouseType = HOUSE_PRIVATE;

    _iBaseStorage = 0;
    _iIncreasedStorage = 0;
    _iLockdownsPercent = 50;
}

CItemMulti::~CItemMulti()
{
	MultiUnRealizeRegion();	// unrealize before removed from ground.
	DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
	// NOTE: ??? This is dangerous to iterators. The "next" item may no longer be valid !

	// Attempt to remove all the accessory junk.
	// NOTE: assume we have already been removed from Top Level

	if ( ! m_pRegion )
		return;
    CItem *pCrate = GetMovingCrate(false).ItemFind();
    if (pCrate && _uidOwner.IsValidUID())
    {
        CItemContainer *pCont = static_cast<CItemContainer*>(pCrate);
        if (pCont->GetCount() > 0)
        {
            CChar *pOwner = _uidOwner.CharFind();
            if (pOwner)
            {
                pOwner->GetBank()->ContentAdd(_uidMovingCrate.ItemFind());
            }
        }
        else
        {
            pCrate->Delete();
        }
    }
	delete m_pRegion;
}

const CItemBaseMulti * CItemMulti::Multi_GetDef() const
{
	return( static_cast <const CItemBaseMulti *>( Base_GetDef()));
}

int CItemMulti::Multi_GetMaxDist() const
{
	ADDTOCALLSTACK("CItemMulti::Multi_GetMaxDist");
	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return 0;
	return( pMultiDef->GetMaxDist());
}

const CItemBaseMulti * CItemMulti::Multi_GetDef( ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemMulti::Multi_GetDef");
	return( dynamic_cast <const CItemBaseMulti *> ( CItemBase::FindItemBase(id)));
}

bool CItemMulti::MultiRealizeRegion()
{
	ADDTOCALLSTACK("CItemMulti::MultiRealizeRegion");
	// Add/move a region for the multi so we know when we are in it.
	// RETURN: ignored.

	if (!IsTopLevel() || _fIsAddon)
		return false;

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
	{
		DEBUG_ERR(( "Bad Multi type 0%x, uid=0%x\n", GetID(), (dword) GetUID()));
		return false;
	}

	if ( m_pRegion == NULL )
	{
		CResourceID rid;
		rid.SetPrivateUID( GetUID());
		m_pRegion = new CRegionWorld( rid );
	}

	// Get Background region.
	CPointMap pt = GetTopPoint();
	const CRegionWorld * pRegionBack = dynamic_cast <CRegionWorld*> (pt.GetRegion( REGION_TYPE_AREA ));
	ASSERT( pRegionBack );
	ASSERT( pRegionBack != m_pRegion );

	// Create the new region rectangle.
	CRectMap rect;
	reinterpret_cast<CRect&>(rect) = pMultiDef->m_rect;
	rect.m_map = pt.m_map;
	rect.OffsetRect( pt.m_x, pt.m_y );
	m_pRegion->SetRegionRect( rect );
	m_pRegion->m_pt = pt;

	dword dwFlags = pMultiDef->m_dwRegionFlags;
	if ( IsType(IT_SHIP))
        dwFlags |= REGION_FLAG_SHIP;
	else
        dwFlags |= pRegionBack->GetRegionFlags(); // Houses get some of the attribs of the land around it.
	m_pRegion->SetRegionFlags(dwFlags);

	tchar *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s (%s)", pRegionBack->GetName(), GetName());
	m_pRegion->SetName(pszTemp);

	return m_pRegion->RealizeRegion();
}

void CItemMulti::MultiUnRealizeRegion()
{
	ADDTOCALLSTACK("CItemMulti::MultiUnRealizeRegion");
	if ( m_pRegion == NULL )
		return;

	m_pRegion->UnRealizeRegion();

	// find all creatures in the region and remove this from them.
	CWorldSearch Area( m_pRegion->m_pt, Multi_GetMaxDist() );
	Area.SetSearchSquare(true);
	for (;;)
	{
		CChar * pChar = Area.GetChar();
		if ( pChar == NULL )
			break;
		if ( pChar->m_pArea != m_pRegion )
			continue;
		pChar->MoveToRegionReTest(REGION_TYPE_AREA);
	}
}

bool CItemMulti::Multi_CreateComponent(ITEMID_TYPE id, short dx, short dy, char dz, dword dwKeyCode, bool fIsAddon)
{
	ADDTOCALLSTACK("CItemMulti::Multi_CreateComponent");
	CItem * pItem = CreateTemplate( id );
	ASSERT(pItem);

	CPointMap pt = GetTopPoint();
	pt.m_x += dx;
	pt.m_y += dy;
	pt.m_z += dz;

	bool fNeedKey = false;

	switch ( pItem->GetType() )
	{
		case IT_KEY:	// it will get locked down with the house ?
		case IT_SIGN_GUMP:
		case IT_SHIP_TILLER:
			pItem->m_itKey.m_UIDLock.SetPrivateUID( dwKeyCode );	// Set the key id for the key/sign.
			m_uidLink.SetPrivateUID(pItem->GetUID());
			fNeedKey = true;
			break;
		case IT_DOOR:
			pItem->SetType(IT_DOOR_LOCKED);
			fNeedKey = true;
			break;
		case IT_CONTAINER:
			pItem->SetType(IT_CONTAINER_LOCKED);
			fNeedKey = true;
			break;
		case IT_SHIP_SIDE:
			pItem->SetType(IT_SHIP_SIDE_LOCKED);
			break;
		case IT_SHIP_HOLD:
			pItem->SetType(IT_SHIP_HOLD_LOCK);
			break;
		default:
			break;
	}

    if (pItem->GetHue() == HUE_DEFAULT)
    {
        pItem->SetHue(GetHue());
    }

	pItem->SetAttr( ATTR_MOVE_NEVER | (m_Attr&(ATTR_MAGIC|ATTR_INVIS)));
	pItem->m_uidLink = GetUID();	// lock it down with the structure.

	if ( pItem->IsTypeLockable() || pItem->IsTypeLocked())
	{
		pItem->m_itContainer.m_UIDLock.SetPrivateUID( dwKeyCode );	// Set the key id for the door/key/sign.
		pItem->m_itContainer.m_lock_complexity = 10000;	// never pickable.
	}

	pItem->MoveToUpdate( pt );
	OnComponentCreate( pItem );
    if (fIsAddon)
    {
        fNeedKey = false;
    }
	return fNeedKey;
}

void CItemMulti::Multi_Create( CChar * pChar, dword dwKeyCode)
{
	ADDTOCALLSTACK("CItemMulti::Multi_Create");
	// Create house or Ship extra stuff.
	// ARGS:
	//  dwKeyCode = set the key code for the doors/sides to this in case it's a drydocked ship.
	// NOTE:
	//  This can only be done after the house is given location.

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	// We are top level.
	if ( pMultiDef == NULL ||
		! IsTopLevel())
		return;

	if ( dwKeyCode == UID_CLEAR )
		dwKeyCode = GetUID();

	// ??? SetTimeout( GetDecayTime()); house decay ?

	bool fNeedKey = false;
    bool fIsAddon = IsAddon();
	size_t iQty = pMultiDef->m_Components.GetCount();
	for ( size_t i = 0; i < iQty; i++ )
	{
		const CItemBaseMulti::CMultiComponentItem &component = pMultiDef->m_Components.ElementAt(i);
		fNeedKey |= Multi_CreateComponent(component.m_id, component.m_dx, component.m_dy, component.m_dz, dwKeyCode, fIsAddon);
	}

    Multi_GetSign();	// set the m_uidLink
    if (pChar)
    {
        SetOwner(pChar->GetUID());
        pChar->AddHouse(GetUID());
    }

    if (fIsAddon)   // Addons doesn't require keys.
    {
        _fIsAddon = true;
        return;
    }

	if (pChar)
	{
		m_itShip.m_UIDCreator = pChar->GetUID();
        if (g_Cfg._fAutoHouseKeys)
        {
            GenerateKey(pChar, true);
        }
	}
}

bool CItemMulti::Multi_IsPartOf( const CItem * pItem ) const
{
	ADDTOCALLSTACK("CItemMulti::Multi_IsPartOf");
	// Assume it is in my area test already.
	// IT_MULTI
	// IT_SHIP
	if ( !pItem )
		return false;
	if ( pItem == this )
		return true;
	return (pItem->m_uidLink == GetUID());
}

CItem * CItemMulti::Multi_FindItemComponent( int iComp ) const
{
	ADDTOCALLSTACK("CItemMulti::Multi_FindItemComponent");
	UNREFERENCED_PARAMETER(iComp);
	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return NULL;

	return NULL;
}

CItem * CItemMulti::Multi_FindItemType( IT_TYPE type ) const
{
	ADDTOCALLSTACK("CItemMulti::Multi_FindItemType");
	// Find a part of this multi nearby.
	if ( ! IsTopLevel())
		return NULL;

	CWorldSearch Area( GetTopPoint(), Multi_GetMaxDist() );
	Area.SetSearchSquare(true);
	for (;;)
	{
		CItem * pItem = Area.GetItem();
		if ( pItem == NULL )
			return NULL;
		if ( ! Multi_IsPartOf( pItem ))
			continue;
		if ( pItem->IsType( type ))
			return( pItem );
	}
}

bool CItemMulti::OnTick()
{
	ADDTOCALLSTACK("CItemMulti::OnTick");
	return true;
}

void CItemMulti::OnMoveFrom()
{
	ADDTOCALLSTACK("CItemMulti::OnMoveFrom");
	// Being removed from the top level.
	// Might just be moving.

    if (!IsAddon()) // Addons doesn't have region, don't try to unrealize it.
    {
        ASSERT(m_pRegion);
        m_pRegion->UnRealizeRegion();
    }
}

bool CItemMulti::MoveTo(CPointMap pt, bool bForceFix) // Put item on the ground here.
{
	ADDTOCALLSTACK("CItemMulti::MoveTo");
	// Move this item to it's point in the world. (ground/top level)
	if ( ! CItem::MoveTo(pt, bForceFix))
		return false;

	// Multis need special region info to track when u are inside them.
	// Add new region info.
    if (!IsAddon()) // Addons doesn't have region, don't try to unrealize it.
    {
        MultiRealizeRegion();
    }
	return true;
}

CItem * CItemMulti::Multi_GetSign()
{
	ADDTOCALLSTACK("CItemMulti::Multi_GetSign");
	// Get my sign or tiller link.
	CItem * pTiller = m_uidLink.ItemFind();
	if ( pTiller == NULL )
	{
		pTiller = Multi_FindItemType( IsType(IT_SHIP) ? IT_SHIP_TILLER : IT_SIGN_GUMP );
		if ( pTiller == NULL )
			return( this );
		m_uidLink = pTiller->GetUID();
	}
	return( pTiller );
}

void CItemMulti::OnHearRegion( lpctstr pszCmd, CChar * pSrc )
{
	ADDTOCALLSTACK("CItemMulti::OnHearRegion");
	// IT_SHIP or IT_MULTI

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return;
	TALKMODE_TYPE		mode	= TALKMODE_SAY;

	for ( size_t i = 0; i < pMultiDef->m_Speech.GetCount(); i++ )
	{
		CResourceLink * pLink = pMultiDef->m_Speech[i];
		ASSERT(pLink);
		CResourceLock s;
		if ( ! pLink->ResourceLock( s ))
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger( s, pszCmd, pSrc, mode );
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		break;
	}
}

void CItemMulti::SetOwner(CUID uidOwner)
{
    ADDTOCALLSTACK("CItemMulti::SetOwner");
    CChar *pOldOwner = _uidOwner.CharFind();
    CChar *pNewOwner = uidOwner.CharFind();
    CUID uidHouse = GetUID();
    if (!g_Serv.IsLoading())
    {
        if (!pNewOwner) // New owner may not exist or not be a CChar
        {
            return; // In this case, stop the proccess.
        }
        if (pOldOwner)  // Old Owner may not exist, ie being removed?
        {
            pOldOwner->DelHouse(uidHouse);
            RemoveKeys(pOldOwner);
        }
    }
    _uidOwner = uidOwner;
}

void CItemMulti::AddCoowner(CUID uidCoowner)
{
    ADDTOCALLSTACK("CItemMulti::AddCoowner");
    if (!uidCoowner.IsValidUID())
    {
        return;
    }
    CChar *pCoowner = uidCoowner.CharFind();
    if (!pCoowner)
    {
        return;
    }
    CUID uidHouse = GetUID();
    for (std::vector<CUID>::iterator it = _lCoowners.begin(); it != _lCoowners.end(); ++it)
    {
        if (*it == uidCoowner)
        {
            return; // Already added
        }
    }
}

void CItemMulti::DelCoowner(CUID uidCoowner)
{
    ADDTOCALLSTACK("CItemMulti::DelCoowner");
    if (!uidCoowner.IsValidUID())
    {
        return;
    }
    for (std::vector<CUID>::iterator it = _lCoowners.begin(); it != _lCoowners.end(); ++it)
    {
        if (*it == uidCoowner)
        {
            CChar* pCoowner = it->CharFind();
            if (pCoowner)
            {
                RemoveKeys(pCoowner);
            }
            _lCoowners.erase(it);
            return;
        }
    }
}

void CItemMulti::AddFriend(CUID uidFriend)
{
    ADDTOCALLSTACK("CItemMulti::AddFriend");
    if (!uidFriend.IsValidUID())
    {
        return;
    }
    CChar *pFriend = uidFriend.CharFind();
    if (!pFriend)
    {
        return;
    }
    CUID uidHouse = GetUID();
    for (std::vector<CUID>::iterator it = _lFriends.begin(); it != _lFriends.end(); ++it)
    {
        if (*it == uidFriend)
        {
            return; // Already added
        }
    }
}

void CItemMulti::DelFriend(CUID uidFriend)
{
    ADDTOCALLSTACK("CItemMulti::DelFriend");
    if (!uidFriend.IsValidUID())
    {
        return;
    }
    for (std::vector<CUID>::iterator it = _lFriends.begin(); it != _lFriends.end(); ++it)
    {
        if (*it == uidFriend)
        {
            CChar* pFriend = it->CharFind();
            if (pFriend)
            {
                RemoveKeys(pFriend);
            }
            _lFriends.erase(it);
            return;
        }
    }
}

uint16 CItemMulti::GetCoownerCount()
{
    return (uint16)_lCoowners.size();
}

uint16 CItemMulti::GetFriendCount()
{
    return (uint16)_lFriends.size();
}

bool CItemMulti::IsOwner(CUID uidTarget)
{
    return (_uidOwner == uidTarget);
}

bool CItemMulti::IsCoowner(CUID uidTarget)
{
    if (_lCoowners.empty())
    {
        return false;
    }
    for (std::vector<CUID>::iterator it = _lCoowners.begin(); it != _lCoowners.end(); ++it)
    {
        if (*it == uidTarget)
        {
            return true;
        }
    }
    return false;
}

bool CItemMulti::IsFriend(CUID uidTarget)
{
    if (_lFriends.empty())
    {
        return false;
    }
    for (std::vector<CUID>::iterator it = _lFriends.begin(); it != _lFriends.end(); ++it)
    {
        if (*it == uidTarget)
        {
            return true;
        }
    }
    return false;
}

CItem *CItemMulti::GenerateKey(CChar *pTarget, bool fPlaceOnBank)
{
    ADDTOCALLSTACK("CItemMulti::GenerateKey");
    if (!pTarget)
    {
        return nullptr;
    }
    CItem * pKey = nullptr;
    // Create the key to the door.
    ITEMID_TYPE id = IsAttr(ATTR_MAGIC) ? ITEMID_KEY_MAGIC : ITEMID_KEY_COPPER;
    pKey = CreateScript(id, pTarget);
    ASSERT(pKey);
    pKey->SetType(IT_KEY);
    if (g_Cfg.m_fAutoNewbieKeys)
    {
        pKey->SetAttr(ATTR_NEWBIE);
    }
    pKey->SetAttr(m_Attr&ATTR_MAGIC);
    pKey->m_itKey.m_UIDLock.SetPrivateUID(GetUID());
    pKey->m_uidLink = GetUID();


    if (fPlaceOnBank)
    {
        pTarget->GetBank()->ContentAdd(pKey);
        pTarget->SysMessageDefault(DEFMSG_MSG_KEY_DUPEBANK);
    }
    else
    {
        // Put in your pack
        pTarget->GetPackSafe()->ContentAdd(pKey);
    }
    return pKey;
}

void CItemMulti::RemoveKeys(CChar *pTarget)
{
    ADDTOCALLSTACK("CItemMulti::RemoveKeys");
    if (!pTarget)
    {
        return;
    }
    CUID uidHouse = GetUID();
    CItem *pItemNext = nullptr;
    for (CItem *pItemKey = pTarget->GetPack()->GetContentHead(); pItemKey != nullptr; pItemKey = pItemNext)
    {
        pItemNext = pItemKey->GetNext();
        if (pItemKey->m_uidLink == uidHouse)
        {
            pItemKey->Delete();
        }
    }

    for (CItem *pItemKey = pTarget->GetPack()->GetContentHead(); pItemKey != nullptr; pItemKey = pItemNext)
    {
        pItemNext = pItemKey->GetNext();
        if (pItemKey->m_uidLink == uidHouse)
        {
            pItemKey->Delete();
        }
    }
}

void CItemMulti::SetMovingCrate(CUID uidCrate)
{
    ADDTOCALLSTACK("CItemMulti::SetMovingCrate");
    if (!uidCrate.IsValidUID())
    {
        return;
    }
    CItemContainer *pNewCrate = static_cast<CItemContainer*>(uidCrate.ItemFind());
    if (!pNewCrate)
    {
        return;
    }
    CItemContainer *pCurrentCrate = static_cast<CItemContainer*>(_uidMovingCrate.ItemFind());
    if (pCurrentCrate && pCurrentCrate->GetCount() > 0)
    {
        pNewCrate->ContentsTransfer(pCurrentCrate, false);
        pCurrentCrate->Delete();
    }
    _uidMovingCrate = uidCrate;
    pNewCrate->m_uidLink = GetUID();
}

CUID CItemMulti::GetMovingCrate(bool fCreate)
{
    if (_uidMovingCrate.IsValidUID())
    {
        return _uidMovingCrate;
    }
    if (fCreate)
    {
        CItem *pCrate = CItem::CreateBase(ITEMID_CRATE1);
        ASSERT(pCrate);
        pCrate->MoveTo(GetTopPoint());
        _uidMovingCrate = pCrate->GetUID();
        return pCrate->GetUID();
    }
    return UID_UNUSED;
}

void CItemMulti::Redeed(bool fDisplayMsg, bool fMoveToBank)
{
    ADDTOCALLSTACK("CItemMulti::Redeed");
    CChar *pOwner = _uidOwner.CharFind();
    CUID uidTransfer;
    uidTransfer.InitUID();
    TransferAllItemsToMovingCrate(uidTransfer, true);
    uidTransfer = GetMovingCrate(true);
    CItem *pCrate = nullptr;
    if (uidTransfer)
    {
        pCrate = uidTransfer.ItemFind();
    }
    if (!pOwner)
    {
        return;
    }
    if (fMoveToBank && pCrate)
    {
        pOwner->GetBank()->ContentAdd(pCrate);
    }

    ITEMID_TYPE itDeed = (ITEMID_TYPE)(GetKeyNum("DEED_ID", true));
    CItem *pDeed = CItem::CreateBase(itDeed != ITEMID_NOTHING ? itDeed : ITEMID_DEED1);
    if (pDeed)
    {
        pDeed->SetHue(GetHue());
        pDeed->m_itDeed.m_Type = GetID();
        if (m_Attr & ATTR_MAGIC)
        {
            pDeed->SetAttr(ATTR_MAGIC);
        }
        pOwner->ItemBounce(pDeed, fDisplayMsg);
    }
    if (!IsAddon())
    {
        pOwner->DelHouse(GetUID());
    }
    Delete();
}

void CItemMulti::TransferAllItemsToMovingCrate(CUID uidTargetContainer, bool fRemoveComponents, TRANSFER_TYPE iType)
{
    ADDTOCALLSTACK("CItemMulti::TransferAllItemsToMovingCrate");
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());

    //Transfer Types
    bool fTransferAddons = (((iType & TRANSFER_ADDONS) || (iType & TRANSFER_ALL)));
    bool fTransferAll = (iType & TRANSFER_ALL);
    bool fTransferLockDowns = (((iType & TRANSFER_LOCKDOWNS) || (iType & TRANSFER_ALL)));
    if (fTransferLockDowns) // Try to move locked down items, also to reduce the CWorldSearch impact.
    {
        TransferLockdownsToMovingCrate();
    }
    CPointMap ptArea;
    if (IsAddon())
    {
        ptArea = GetTopPoint().GetRegion(REGION_TYPE_HOUSE)->m_pt;  // Addons doesnt have Area, search in the above region.
    }
    else
    {
        ptArea = m_pRegion->m_pt;
    }
    CWorldSearch Area(ptArea, Multi_GetMaxDist());	// largest area.
    Area.SetSearchSquare(true);
    for (;;)
    {
        CItem * pItem = Area.GetItem();
        if (pItem == NULL)
            break;
        if (pItem->GetUID() == GetUID() || pItem->GetUID() == _uidMovingCrate)	// this gets deleted seperately.
            continue;
        if (fTransferAddons && (pItem->IsType(IT_MULTI_ADDON) || pItem->IsType(IT_MULTI)))  // If the item is a house Addon, redeed it.
        {
            static_cast<CItemMulti*>(pItem)->Redeed(false, false);
            Area.RestartSearch();	// we removed an item and this will mess the search loop, so restart to fix it
            continue;
        }
        if (!Multi_IsPartOf(pItem) && !fTransferAll)  // Items not linked to this multi.
        {
            continue;
        }
        if (pItem->GetKeyNum("COMPONENT", true))    // Components should never be transfered.
        {
            if (fRemoveComponents)  // But they can or cannot be removed.
            {
                pItem->Delete();		// delete the key id for the door/key/sign.
                Area.RestartSearch();	// we removed an item and this will mess the search loop, so restart to fix it
            }
            continue;
        }
        if (pCrate) // Transfer items if there is a target and they are not Components (doors, signs, etc).
        {
            if (!pItem->IsAttr(ATTR_LOCKEDDOWN) || fTransferLockDowns) // Only transfer Locked down items if fTransferLockDowns is true.
            {
                pCrate->ContentAdd(pItem);
                pItem->RemoveFromView();
            }
        }
    }
    CItemContainer *pTargCont = static_cast<CItemContainer*>(uidTargetContainer.ItemFind());
    if (pCrate && pTargCont)
    {
        pCrate->ClrAttr(ATTR_MOVE_NEVER | ATTR_STATIC | ATTR_LOCKEDDOWN);   //Clear all attrs
        pCrate->SetKeyNum("MULTI_UID", GetUID());
        pTargCont->ContentAdd(pCrate);
    }
}

void CItemMulti::TransferLockdownsToMovingCrate()
{
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());
    if (_lLockDowns.empty())
    {
        return;
    }
    for (std::vector<CUID>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
    {
        CItem *pItem = it->ItemFind();
        if (pItem)  // Move all valid items.
        {
            pCrate->ContentAdd(pItem);
            pItem->ClrAttr(ATTR_LOCKEDDOWN);
            pItem->m_uidLink.InitUID();
        }
    }
    _lLockDowns.clear();    // Clear the list, asume invalid items should be cleared too.
}

void CItemMulti::SetAddon(bool fIsAddon)
{
    _fIsAddon = fIsAddon;
}

bool CItemMulti::IsAddon()
{
    return _fIsAddon;
}

void CItemMulti::SetBaseStorage(uint16 iLimit)
{
    _iBaseStorage = iLimit;
}

uint16 CItemMulti::GetBaseStorage()
{
    return _iBaseStorage;
}

void CItemMulti::SetBaseVendors(uint8 iLimit)
{
    _iBaseVendors = iLimit;
}

uint8 CItemMulti::GetBaseVendors()
{
    return _iBaseVendors;
}

uint8 CItemMulti::GetMaxVendors()
{
    return (uint8)(_iBaseVendors + (_iBaseVendors * GetIncreasedStorage()) / 100);
}

uint8 CItemMulti::GetCurrentVendors()
{
    return (uint8)_lVendors.size();
}

void CItemMulti::SetIncreasedStorage(uint16 iIncrease)
{
    _iIncreasedStorage = iIncrease;
}

uint16 CItemMulti::GetIncreasedStorage()
{
    return _iIncreasedStorage;
}

uint16 CItemMulti::GetMaxStorage()
{
    return _iBaseStorage + ((_iBaseStorage * _iIncreasedStorage) / 100);
}

uint16 CItemMulti::GetCurrentStorage()
{
    if (_lLockDowns.empty())
    {
        return 0;
    }
    uint16 iCount = 0;
    for (std::vector<CUID>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
    {
        CItem *pItem = it->ItemFind();
        if (pItem)
        {
            CItemContainer *pCont = static_cast<CItemContainer*>(pItem);
            if (pCont)
            {
                iCount += (uint16)pCont->GetCount();
                continue;
            }
            ++iCount;
        }
    }
    return iCount;
}

uint16 CItemMulti::GetMaxLockdowns()
{
    return GetBaseStorage() / GetLockdownsPercent();
}

uint8 CItemMulti::GetLockdownsPercent()
{
    return _iLockdownsPercent;
}

void CItemMulti::SetLockdownsPercent(uint8 iPercent)
{
    _iLockdownsPercent = iPercent;
}

ushort CItemMulti::GetCurrentLockdowns()
{
    return (uint16)_lLockDowns.size();
}

void CItemMulti::LockItem(CUID uidItem, bool fUpdateFlags)
{
    if (IsLockedItem(uidItem))
    {
        return;
    }
    _lLockDowns.emplace_back(uidItem);
    if (fUpdateFlags)
    {
        CItem *pItem = uidItem.ItemFind();
        if (pItem)
        {
            pItem->SetAttr(ATTR_LOCKEDDOWN);
            pItem->m_uidLink = GetUID();
        }
    }
}

void CItemMulti::UnlockItem(CUID uidItem, bool fUpdateFlags)
{
    for (std::vector<CUID>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
    {
        if (*it == uidItem)
        {
            _lLockDowns.erase(it);
            break;
        }
    }
    if (fUpdateFlags)
    {
        CItem *pItem = uidItem.ItemFind();
        if (pItem)
        {
            pItem->ClrAttr(ATTR_SECURE);
            pItem->m_uidLink.InitUID();
        }
    }
}

bool CItemMulti::IsLockedItem(CUID uidItem)
{
    if (_lLockDowns.empty())
    {
        return false;
    }
    for (std::vector<CUID>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
    {
        if (*it == uidItem)
        {
            return true;
        }
    }
    return false;
}

void CItemMulti::AddVendor(CUID uidVendor)
{
    if (IsLockedItem(uidVendor))
    {
        return;
    }
    _lVendors.emplace_back(uidVendor);
}

void CItemMulti::DelVendor(CUID uidVendor)
{
    for (std::vector<CUID>::iterator it = _lVendors.begin(); it != _lVendors.end(); ++it)
    {
        if (*it == uidVendor)
        {
            _lVendors.erase(it);
            break;
        }
    }
}

bool CItemMulti::IsHouseVendor(CUID uidVendor)
{
    if (_lVendors.empty())
    {
        return false;
    }
    for (std::vector<CUID>::iterator it = _lVendors.begin(); it != _lVendors.end(); ++it)
    {
        if (*it == uidVendor)
        {
            return true;
        }
    }
    return false;
}

bool CItemMulti::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK("CItemMulti::r_GetRef");
    // COMP(x).

    if (!strnicmp(pszKey, "COMP(", 4))
    {
        pszKey += 5;
        int i = Exp_GetVal(pszKey);
        SKIP_SEPARATORS(pszKey);
        pRef = Multi_FindItemComponent(i);
        return true;
    }
    if (!strnicmp(pszKey, "REGION", 6))
    {
        pszKey += 6;
        SKIP_SEPARATORS(pszKey);
        pRef = m_pRegion;
        return true;
    }

    return(CItem::r_GetRef(pszKey, pRef));
}

enum
{
    SHV_ADDVENDOR,
    SHV_DELVENDOR,
    SHV_LOCKITEM,
    SHV_MOVETOCRATE,
    SHV_MULTICREATE,
    SHV_REDEED,
    SHV_REMOVEKEYS,
    SHV_UNLOCKITEM,
    SHV_QTY
};

lpctstr const CItemMulti::sm_szVerbKeys[SHV_QTY + 1] =
{
    "ADDVENDOR",
    "DELVENDOR",
    "LOCKITEM",
    "MOVETOCRATE",
    "MULTICREATE",
    "REDEED",
    "REMOVEKEYS",
    "UNLOCKITEM",
    NULL
};

bool CItemMulti::r_Verb(CScript & s, CTextConsole * pSrc) // Execute command from script
{
    ADDTOCALLSTACK("CItemMulti::r_Verb");
    EXC_TRY("Verb");
    // Speaking in this multis region.
    // return: true = command for the multi.

    int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys) - 1);
    switch (iCmd)
    {
        case SHV_ADDVENDOR:
            AddVendor((CUID)s.GetArgDWVal());
            break;
        case SHV_DELVENDOR:
        {
            CUID uidVendor = s.GetArgDWVal();
            if (!uidVendor.IsValidUID())
            {
                _lVendors.clear();
            }
            else
            {
                DelVendor((CUID)s.GetArgDWVal());
            }
            break;
        }
        case SHV_LOCKITEM:
        {
            int64 piCmd[2];
            Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
            CUID uidItem =(dword)piCmd[0];
            bool fUpdateFlags = piCmd[1] ? true : false;
            LockItem(uidItem, fUpdateFlags);
            break;
        }
        case SHV_UNLOCKITEM:
        {
            int64 piCmd[2];
            Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
            CUID uidItem = (dword)piCmd[0];
            bool fUpdateFlags = piCmd[1] ? true : false;
            if (!uidItem.IsValidUID())
            {
                _lLockDowns.clear();
            }
            else
            {
                UnlockItem(uidItem, fUpdateFlags);
            }
            break;
        }
        case SHV_MOVETOCRATE:
        {
            int64 piCmd[3];
            Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
            CUID uidItem = (dword)piCmd[0];
            bool fRemoveComponents = piCmd[1] ? true : false;
            TRANSFER_TYPE iTransferType = (TRANSFER_TYPE)piCmd[2];
            TransferAllItemsToMovingCrate(uidItem, fRemoveComponents, iTransferType);
            break;
        }
        case SHV_MULTICREATE:
        {
            CUID uidChar = s.GetArgDWVal();
            CChar *	pCharSrc = uidChar.CharFind();
            Multi_Create(pCharSrc, 0);
            break;
        }
        case SHV_REDEED:
        {
            int64 piCmd[2];
            Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
            bool fShowMsg = piCmd[0] ? true : false;
            bool fMoveToBank = piCmd[1] ? true : false;
            Redeed(fShowMsg, fMoveToBank);
            break;
        }
        case SHV_REMOVEKEYS:
        {
            CUID uidChar = s.GetArgDWVal();
            CChar *	pCharSrc = uidChar.CharFind();
            RemoveKeys(pCharSrc);
            break;
        }

        default:
        {
            return CItem::r_Verb(s, pSrc);
        }
    }

    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPTSRC;
    EXC_DEBUG_END;
    return true;
}

enum SHL_TYPE
{
    SHL_ADDCOOWNER,
    SHL_ADDFRIEND,
    SHL_ADDKEY,
    SHL_BASESTORAGE,
    SHL_BASEVENDORS,
    SHL_COMP,
    SHL_CURRENTLOCKDOWNS,
    SHL_CURRENTSTORAGE,
    SHL_CURRENTVENDORS,
    SHL_DELCOOWNER,
    SHL_DELFRIEND,
    SHL_INCREASEDSTORAGE,
    SHL_ISCOOWNER,
    SHL_ISFRIEND,
    SHL_ISHOUSEVENDOR,
    SHL_ISLOCKEDITEM,
    SHL_ISOWNER,
    SHL_LOCKDOWNSPERCENT,
    SHL_MAXLOCKDOWNS,
    SHL_MAXSTORAGE,
    SHL_MAXVENDORS,
    SHL_MOVINGCRATE,
    SHL_OWNER,
    SHL_REGION,
    SHL_REMOVEKEYS,
    SHL_QTY
};

const lpctstr CItemMulti::sm_szLoadKeys[SHL_QTY + 1] =
{
    "ADDCOOWNER",
    "ADDFRIEND",
    "ADDKEY",
    "BASESTORAGE",
    "BASEVENDORS",
    "COMP",
    "CURRENTLOCKDOWNS",
    "CURRENTSTORAGE",
    "CURRENTVENDORS",
    "DELCOORWNER",
    "DELFRIEND",
    "INCREASEDSTORAGE",
    "ISCOOWNER",
    "ISFRIEND",
    "ISHOUSEVENDOR",
    "ISLOCKEDITEM",
    "ISOWNER",
    "LOCKDOWNSPERCENT",
    "MAXLOCKDOWNS",
    "MAXSTORAGE",
    "MAXVENDORS",
    "MOVINGCRATE",
    "OWNER",
    "REGION",
    "REMOVEKEYS",
    NULL
};

void CItemMulti::r_Write(CScript & s)
{
    ADDTOCALLSTACK_INTENSIVE("CItemMulti::r_Write");
    CItem::r_Write(s);
    if (m_pRegion)
    {
        m_pRegion->r_WriteBody(s, "REGION.");
    }
    s.WriteKeyHex("OWNER", _uidOwner);
    if (!_lCoowners.empty())
    {
        for (std::vector<CUID>::iterator it = _lCoowners.begin(); it != _lCoowners.end(); ++it)
        {
            s.WriteKeyHex("ADDCOOWNER", *it);
        }
    }
    if (!_lFriends.empty())
    {
        for (std::vector<CUID>::iterator it = _lFriends.begin(); it != _lFriends.end(); ++it)
        {
            s.WriteKeyHex("ADDFRIEND", *it);
        }
    }
}

bool CItemMulti::r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemMulti::r_WriteVal");
    int iCmd = FindTableHeadSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iCmd < 0)
    {
        return CItem::r_WriteVal(pszKey, sVal, pSrc);
    }

    switch (iCmd)
    {
        case SHL_COMP:
        {
            const CItemBaseMulti *pMultiDef = Multi_GetDef();
            pszKey += 4;

            // no component uid
            if (*pszKey == '\0')
            {
                sVal.FormatSTVal(pMultiDef->m_Components.GetCount());
            }
            else if (*pszKey == '.')
            {
                CItemBaseMulti::CMultiComponentItem	item;

                SKIP_SEPARATORS(pszKey);
                size_t iQty = Exp_GetVal(pszKey);
                if (pMultiDef->m_Components.IsValidIndex(iQty) == false)
                    return false;

                SKIP_SEPARATORS(pszKey);
                item = pMultiDef->m_Components.GetAt(iQty);

                if (!strnicmp(pszKey, "ID", 2)) sVal.FormatVal(item.m_id);
                else if (!strnicmp(pszKey, "DX", 2)) sVal.FormatVal(item.m_dx);
                else if (!strnicmp(pszKey, "DY", 2)) sVal.FormatVal(item.m_dy);
                else if (!strnicmp(pszKey, "DZ", 2)) sVal.FormatVal(item.m_dz);
                else if (!strnicmp(pszKey, "D", 1)) sVal.Format("%i,%i,%i", item.m_dx, item.m_dy, item.m_dz);
                else sVal.Format("%u,%i,%i,%i", item.m_id, item.m_dx, item.m_dy, item.m_dz);
            }
            else
            {
                return false;
            }
        }
        // House Permissions
        case SHL_ISCOOWNER:
            pszKey += 9;
            sVal.FormatBVal(IsCoowner(static_cast<CUID>(ATOI(pszKey))));
            return true;
        case SHL_ISFRIEND:
            pszKey += 8;
            sVal.FormatBVal(IsFriend(static_cast<CUID>(ATOI(pszKey))));
            return true;
        case SHL_ISOWNER:
        {
            pszKey += 7;
            CUID uidOwner = static_cast<CUID>(Exp_GetVal(pszKey));
            sVal.FormatBVal(IsOwner(uidOwner));
            return true;
        }
        case SHL_OWNER:
            sVal.FormatDWVal(_uidOwner);
            return true;

        // House General
        case SHL_MOVINGCRATE:
            sVal.FormatDWVal(GetMovingCrate(true));
            return true;
        case SHL_ADDKEY:
        {
            pszKey += 6;
            CUID uidOwner = static_cast<CUID>(Exp_GetVal(pszKey));
            bool fDupeOnBank = Exp_GetVal(pszKey) ? 1 : 0;
            CChar *pCharOwner = uidOwner.CharFind();
            CItem *pKey = nullptr;
            if (pCharOwner)
            {
                pKey = GenerateKey(pCharOwner, fDupeOnBank);
                if (pKey)
                {
                    sVal.FormatDWVal(pKey->GetUID());
                    return true;
                }
            }
            sVal.FormatDWVal(0);
            return true;
        }

        // House Storage
        case SHL_BASESTORAGE:
            sVal.FormatU16Val(GetBaseStorage());
            break;
        case SHL_BASEVENDORS:
            sVal.FormatU8Val(GetBaseVendors());
            break;
        case SHL_CURRENTLOCKDOWNS:
            sVal.FormatU16Val(GetCurrentLockdowns());
            break;
        case SHL_CURRENTSTORAGE:
            sVal.FormatU16Val(GetCurrentStorage());
            break;
        case SHL_CURRENTVENDORS:
            sVal.FormatU8Val(GetCurrentVendors());
            break;
        case SHL_INCREASEDSTORAGE:
            sVal.FormatU16Val(GetIncreasedStorage());
            break;
        case SHL_ISHOUSEVENDOR:
            sVal.FormatBVal(IsHouseVendor((CUID)Exp_GetVal(pszKey)));
            break;
        case SHL_ISLOCKEDITEM:
            sVal.FormatBVal(IsLockedItem((CUID)Exp_GetVal(pszKey)));
            break;
        case SHL_LOCKDOWNSPERCENT:
            sVal.FormatU8Val(GetLockdownsPercent());
            break;
        case SHL_MAXLOCKDOWNS:
            sVal.FormatU16Val(GetMaxLockdowns());
            break;
        case SHL_MAXSTORAGE:
            sVal.FormatU16Val(GetMaxStorage());
            break;
        case SHL_MAXVENDORS:
            sVal.FormatU16Val(GetMaxVendors());
            break;
        default:
            break;
    }
    return true;
}

bool CItemMulti::r_LoadVal( CScript & s  )
{
	ADDTOCALLSTACK("CItemMulti::r_LoadVal");
	EXC_TRY("LoadVal");

    int iCmd = FindTableHeadSorted(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iCmd < 0)
    {
        return CItem::r_LoadVal(s);
    }

    switch (iCmd)
    {
        case SHL_REGION:
        {
            if (!IsTopLevel())
            {
                MoveTo(GetTopPoint()); // Put item on the ground here.
                Update();
            }
            ASSERT(m_pRegion);
            CScript script(s.GetKey() + 7, s.GetArgStr());
            script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// Index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
            script.m_iLineNum = s.m_iLineNum;						// Line in the script file where Key/Arg were read
            return m_pRegion->r_LoadVal(script);
        }

        // House Permissions.
        case SHL_ADDCOOWNER:
            AddCoowner(static_cast<CUID>(s.GetArgDWVal()));
            break;
        case SHL_ADDFRIEND:
            AddFriend(static_cast<CUID>(s.GetArgDWVal()));
            break;
        case SHL_OWNER:
            SetOwner(static_cast<CUID>(s.GetArgDWVal()));
            break;
        case SHL_MOVINGCRATE:
            SetMovingCrate(static_cast<CUID>(s.GetArgDWVal()));
            break;
        case SHL_DELCOOWNER:
            DelCoowner(static_cast<CUID>(s.GetArgDWVal()));
            break;
        case SHL_DELFRIEND:
            DelFriend(static_cast<CUID>(s.GetArgDWVal()));
            break;
        case SHL_ADDKEY:
        {
            int64 piCmd[2];
            int iLen = Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
            CUID uidOwner = static_cast<CUID>((dword)piCmd[0]);
            bool fDupeOnBank = false;
            if (iLen > 1)
            { 
                fDupeOnBank = piCmd[1];
            }
            CChar *pCharOwner = uidOwner.CharFind();
            if (pCharOwner)
            {
                GenerateKey(pCharOwner, fDupeOnBank);
            }
            break;
        }

        // House Storage
        case SHL_BASESTORAGE:
            SetBaseStorage(s.GetArgU16Val());
            break;
        case SHL_BASEVENDORS:
            SetBaseVendors(s.GetArgU8Val());
            break;
        case SHL_INCREASEDSTORAGE:
            SetIncreasedStorage(s.GetArgU16Val());
            break;
        case SHL_LOCKDOWNSPERCENT:
            SetLockdownsPercent(s.GetArgU8Val());
            break;
        default:
            break;
    }
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
void CItemMulti::DupeCopy( const CItem * pItem )
{
	ADDTOCALLSTACK("CItemMulti::DupeCopy");
	CItem::DupeCopy(pItem);
}

bool CItemMulti::CanPlace(CChar *pChar)
{
    if (!pChar)
    {
        return false;
    }
    uint8 iMaxHouses = pChar->_iMaxHouses;
    if (iMaxHouses == 0)
    {
        if (pChar->GetClient())
        {
            iMaxHouses = pChar->GetClient()->GetAccount()->_iMaxHouses;
        }
    }
    if (iMaxHouses > 0 && iMaxHouses >= pChar->_lHouses.size())
    {
        return false;
    }
    return true;

    // Location Checks
}

void CItemMulti::OnComponentCreate(const CItem * pComponent, bool fIsAddon)
{
	UNREFERENCED_PARAMETER(pComponent);
    if (fIsAddon)
    {
        const_cast<CItem*>(pComponent)->SetKeyNum("IsAddon", 1);
    }
    else{
        const_cast<CItem*>(pComponent)->SetKeyNum("COMPONENT", 1);
    }
}
