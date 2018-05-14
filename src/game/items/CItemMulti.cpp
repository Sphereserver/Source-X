
#include "../../common/CException.h"
#include "../../common/CUIDExtra.h"
#include "../../common/sphereproto.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"
#include "../CWorld.h"
#include "CItemMulti.h"
#include "CItemShip.h"
#include "CItemContainer.h"

/////////////////////////////////////////////////////////////////////////////

CItemMulti::CItemMulti(ITEMID_TYPE id, CItemBase * pItemDef) :	// CItemBaseMulti
    CItem(id, pItemDef)
{
    CItemBaseMulti * pItemBase = static_cast<CItemBaseMulti*>(Base_GetDef());
    m_shipSpeed.period = pItemBase->m_shipSpeed.period;
    m_shipSpeed.tiles = pItemBase->m_shipSpeed.tiles;
    m_SpeedMode = pItemBase->m_SpeedMode;

    m_pRegion = nullptr;
    _pOwner = nullptr;
    _pMovingCrate = nullptr;
    _pGuild = nullptr;

    _fIsAddon = false;
    _iHouseType = HOUSE_PRIVATE;
    _iMultiCount = pItemBase->_iMultiCount;

    _iBaseStorage = pItemBase->_iBaseStorage;
    _iBaseVendors = pItemBase->_iBaseVendors;
    _iLockdownsPercent = pItemBase->_iLockdownsPercent;
    _iIncreasedStorage = 0;
}

CItemMulti::~CItemMulti()
{
    MultiUnRealizeRegion();	// unrealize before removed from ground.
    DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
    // NOTE: ??? This is dangerous to iterators. The "next" item may no longer be valid !

    // Attempt to remove all the accessory junk.
    // NOTE: assume we have already been removed from Top Level


    if (!m_pRegion)
    {
        return;
    }

    Redeed(false, true);
    delete m_pRegion;
}

const CItemBaseMulti * CItemMulti::Multi_GetDef() const
{
    return(static_cast <const CItemBaseMulti *>(Base_GetDef()));
}

int CItemMulti::Multi_GetMaxDist() const
{
    ADDTOCALLSTACK("CItemMulti::Multi_GetMaxDist");
    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    if (pMultiDef == nullptr)
    {
        return 0;
    }
    return pMultiDef->GetMaxDist();
}

const CItemBaseMulti * CItemMulti::Multi_GetDef(ITEMID_TYPE id) // static
{
    ADDTOCALLSTACK("CItemMulti::Multi_GetDef");
    return(dynamic_cast <const CItemBaseMulti *> (CItemBase::FindItemBase(id)));
}

bool CItemMulti::MultiRealizeRegion()
{
    ADDTOCALLSTACK("CItemMulti::MultiRealizeRegion");
    // Add/move a region for the multi so we know when we are in it.
    // RETURN: ignored.

    if (!IsTopLevel() || _fIsAddon)
    {
        return false;
    }

    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    if (pMultiDef == nullptr)
    {
        DEBUG_ERR(("Bad Multi type 0%x, uid=0%x\n", GetID(), (dword)GetUID()));
        return false;
    }

    if (m_pRegion == nullptr)
    {
        CResourceID rid;
        rid.SetPrivateUID(GetUID());
        m_pRegion = new CRegionWorld(rid);
    }

    // Get Background region.
    CPointMap pt = GetTopPoint();
    const CRegionWorld * pRegionBack = dynamic_cast <CRegionWorld*> (pt.GetRegion(REGION_TYPE_AREA));
    ASSERT(pRegionBack);
    ASSERT(pRegionBack != m_pRegion);

    // Create the new region rectangle.
    CRectMap rect;
    reinterpret_cast<CRect&>(rect) = pMultiDef->m_rect;
    rect.m_map = pt.m_map;
    rect.OffsetRect(pt.m_x, pt.m_y);
    m_pRegion->SetRegionRect(rect);
    m_pRegion->m_pt = pt;

    dword dwFlags = pMultiDef->m_dwRegionFlags;
    if (IsType(IT_SHIP))
    {
        dwFlags |= REGION_FLAG_SHIP;
    }
    else
    {
        dwFlags |= pRegionBack->GetRegionFlags(); // Houses get some of the attribs of the land around it.
    }
    m_pRegion->SetRegionFlags(dwFlags);

    tchar *pszTemp = Str_GetTemp();
    sprintf(pszTemp, "%s (%s)", pRegionBack->GetName(), GetName());
    m_pRegion->SetName(pszTemp);

    return m_pRegion->RealizeRegion();
}

void CItemMulti::MultiUnRealizeRegion()
{
    ADDTOCALLSTACK("CItemMulti::MultiUnRealizeRegion");
    if (m_pRegion == nullptr)
    {
        return;
    }

    m_pRegion->UnRealizeRegion();

    // find all creatures in the region and remove this from them.
    CWorldSearch Area(m_pRegion->m_pt, Multi_GetMaxDist());
    Area.SetSearchSquare(true);
    for (;;)
    {
        CChar * pChar = Area.GetChar();
        if (pChar == nullptr)
        {
            break;
        }
        if (pChar->m_pArea != m_pRegion)
        {
            continue;
        }
        pChar->MoveToRegionReTest(REGION_TYPE_AREA);
    }
}

bool CItemMulti::Multi_CreateComponent(ITEMID_TYPE id, short dx, short dy, char dz, dword dwKeyCode, bool fIsAddon)
{
    ADDTOCALLSTACK("CItemMulti::Multi_CreateComponent");
    CItem * pItem = CreateTemplate(id);
    ASSERT(pItem);

    CPointMap pt = GetTopPoint();
    pt.m_x += dx;
    pt.m_y += dy;
    pt.m_z += dz;

    bool fNeedKey = false;

    switch (pItem->GetType())
    {
        case IT_KEY:	// it will get locked down with the house ?
        case IT_SIGN_GUMP:
        case IT_SHIP_TILLER:
        {
            pItem->m_itKey.m_UIDLock.SetPrivateUID(dwKeyCode);	// Set the key id for the key/sign.
            m_uidLink.SetPrivateUID(pItem->GetUID());
            fNeedKey = true;
            break;
        }
        case IT_DOOR:
        {
            CScript event("events +t_house_door");
            pItem->r_LoadVal(event);
            pItem->SetType(IT_DOOR_LOCKED);
            fNeedKey = true;
            break;
        }
        case IT_CONTAINER:
        {
            CScript event("events +t_house_container");
            pItem->r_LoadVal(event);
            pItem->SetType(IT_CONTAINER_LOCKED);
            fNeedKey = true;
            break;
        }
        case IT_SHIP_SIDE:
        {
            pItem->SetType(IT_SHIP_SIDE_LOCKED);
            break;
        }
        case IT_SHIP_HOLD:
        {
            pItem->SetType(IT_SHIP_HOLD_LOCK);
            break;
        }
        default:
            break;
    }

    if (pItem->GetHue() == HUE_DEFAULT)
    {
        pItem->SetHue(GetHue());
    }

    pItem->SetAttr(ATTR_MOVE_NEVER | (m_Attr&(ATTR_MAGIC | ATTR_INVIS)));
    pItem->m_uidLink = GetUID();	// lock it down with the structure.

    if (pItem->IsTypeLockable() || pItem->IsTypeLocked())
    {
        pItem->m_itContainer.m_UIDLock.SetPrivateUID(dwKeyCode);	// Set the key id for the door/key/sign.
        pItem->m_itContainer.m_lock_complexity = 10000;	// never pickable.
    }

    CScript event("events +t_house_component");
    pItem->r_LoadVal(event);
    pItem->MoveToUpdate(pt);
    OnComponentCreate(pItem);
    if (fIsAddon)
    {
        fNeedKey = false;
    }
    return fNeedKey;
}

void CItemMulti::Multi_Setup(CChar * pChar, dword dwKeyCode)
{
    ADDTOCALLSTACK("CItemMulti::Multi_Setup");
    // Create house or Ship extra stuff.
    // ARGS:
    //  dwKeyCode = set the key code for the doors/sides to this in case it's a drydocked ship.
    // NOTE:
    //  This can only be done after the house is given location.

    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    // We are top level.
    if (pMultiDef == nullptr || !IsTopLevel())
    {
        return;
    }

    if (dwKeyCode == UID_CLEAR)
    {
        dwKeyCode = GetUID();
    }

    // ??? SetTimeout( GetDecayTime()); house decay ?

    bool fNeedKey = false;
    GenerateBaseComponents(fNeedKey, dwKeyCode);

    Multi_GetSign();	// set the m_uidLink

    if (IsAddon())   // Addons doesn't require keys and are not added to any list.
    {
        return;
    }

    if (pChar)
    {
        SetOwner(pChar);
        pChar->GetMultiStorage()->AddMulti(this);
    }

    if (pChar)
    {
        m_itShip.m_UIDCreator = pChar->GetUID();
        if (IsType(IT_SHIP))
        {
            if (g_Cfg._fAutoShipKeys)
            {
                GenerateKey(pChar, true);
            }
        }
        else
        {
            if (g_Cfg._fAutoHouseKeys)
            {
                GenerateKey(pChar, true);
            }
        }
    }
}

bool CItemMulti::Multi_IsPartOf(const CItem * pItem) const
{
    ADDTOCALLSTACK("CItemMulti::Multi_IsPartOf");
    // Assume it is in my area test already.
    // IT_MULTI
    // IT_SHIP
    if (!pItem)
    {
        return false;
    }
    if (pItem == this)
    {
        return true;
    }
    return (pItem->m_uidLink == GetUID());
}

CItem * CItemMulti::Multi_FindItemComponent(int iComp) const
{
    ADDTOCALLSTACK("CItemMulti::Multi_FindItemComponent");

    if ((int)_lComponents.size() > iComp)
    {
        return _lComponents[iComp];
    }
    return nullptr;
}

CItem * CItemMulti::Multi_FindItemType(IT_TYPE type) const
{
    ADDTOCALLSTACK("CItemMulti::Multi_FindItemType");
    // Find a part of this multi nearby.
    if (!IsTopLevel())
    {
        return nullptr;
    }

    CWorldSearch Area(GetTopPoint(), Multi_GetMaxDist());
    Area.SetSearchSquare(true);
    for (;;)
    {
        CItem * pItem = Area.GetItem();
        if (pItem == nullptr)
        {
            return nullptr;
        }
        if (!Multi_IsPartOf(pItem))
        {
            continue;
        }
        if (pItem->IsType(type))
        {
            return(pItem);
        }
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
    if (!CItem::MoveTo(pt, bForceFix))
    {
        return false;
    }

    // Multis need special region info to track when u are inside them.
    // Add new region info.
    if (!IsAddon()) // Addons doesn't have region, don't try to realize it.
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
    if (pTiller == nullptr)
    {
        pTiller = Multi_FindItemType(IsType(IT_SHIP) ? IT_SHIP_TILLER : IT_SIGN_GUMP);
        if (pTiller == nullptr)
        {
            return(this);
        }
        m_uidLink = pTiller->GetUID();  // Link the multi to it's sign/tiller
    }
    return pTiller;
}

void CItemMulti::OnHearRegion(lpctstr pszCmd, CChar * pSrc)
{
    ADDTOCALLSTACK("CItemMulti::OnHearRegion");
    // IT_SHIP or IT_MULTI

    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    if (pMultiDef == nullptr)
        return;
    TALKMODE_TYPE		mode = TALKMODE_SAY;

    for (size_t i = 0; i < pMultiDef->m_Speech.size(); i++)
    {
        CResourceLink * pLink = pMultiDef->m_Speech[i];
        ASSERT(pLink);
        CResourceLock s;
        if (!pLink->ResourceLock(s))
            continue;
        TRIGRET_TYPE iRet = OnHearTrigger(s, pszCmd, pSrc, mode);
        if (iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE)
            continue;
        break;
    }
}

void CItemMulti::SetOwner(CChar* pOwner)
{
    ADDTOCALLSTACK("CItemMulti::SetOwner");
    if (!g_Serv.IsLoading())
    {
        CChar *pOldOwner = _pOwner;
        if (pOldOwner)  // Old Owner may not exist, was removed?
        {
            pOldOwner->GetMultiStorage()->DelMulti(this);
            RemoveKeys(pOldOwner);
        }
    }
    if (!pOwner)
    {
        _pOwner = nullptr;
        return;
    }
    pOwner->GetMultiStorage()->AddMulti(this);
    _pOwner = pOwner;
}

bool CItemMulti::IsOwner(CChar* pTarget)
{
    return (_pOwner == pTarget);
}

CChar * CItemMulti::GetOwner()
{
    return _pOwner;
}

void CItemMulti::SetGuild(CItemStone* pGuild)
{
    ADDTOCALLSTACK("CItemMulti::SetGuild");
    /*if (!g_Serv.IsLoading())
    {
        CItemStone *pOldGuild = GetGuild();
        if (pOldGuild)  // Old Guild may not exist, was removed?
        {
            pOldGuild->GetMultiStorage()->DelMulti(this);
        }
    }
    if (pGuild == nullptr)
    {
        _pGuild = nullptr;
        return;
    }
    ASSERT(_pGuild);
    ASSERT(_pGuild->GetMultiStorage());
    _pGuild->GetMultiStorage()->AddMulti(this);
    _pGuild = pGuild;*/
}

bool CItemMulti::IsGuild(CItemStone* pTarget)
{
    return (_pGuild == pTarget);
}

CItemStone * CItemMulti::GetGuild()
{
    return _pGuild;
}

void CItemMulti::AddCoowner(CChar* pCoowner)
{
    ADDTOCALLSTACK("CItemMulti::AddCoowner");
    if (!g_Serv.IsLoading())
    {
        if (!pCoowner)
        {
            return;
        }
        if (GetCoownerPos(pCoowner) >= 0)
        {
            return;
        }
    }
    _lCoowners.emplace_back(pCoowner);
}

void CItemMulti::DelCoowner(CChar* pCoowner)
{
    ADDTOCALLSTACK("CItemMulti::DelCoowner");
    for (std::vector<CChar*>::iterator it = _lCoowners.begin(); it != _lCoowners.end(); ++it)
    {
        if (*it == pCoowner)
        {
            RemoveKeys(*it);
            _lCoowners.erase(it);
            return;
        }
    }
}

size_t CItemMulti::GetCoownerCount()
{
    return _lCoowners.size();
}

int CItemMulti::GetCoownerPos(CChar* pTarget)
{
    if (_lCoowners.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lCoowners.size(); ++i)
    {
        if (_lCoowners[i] == pTarget)
        {
            return (int)i;
        }
    }
    return -1;
}

void CItemMulti::AddFriend(CChar* pFriend)
{
    ADDTOCALLSTACK("CItemMulti::AddFriend");
    if (!g_Serv.IsLoading())
    {
        if (!pFriend)
        {
            return;
        }
        if (GetFriendPos(pFriend) >= 0)
        {
            return;
        }
    }
    _lFriends.emplace_back(pFriend);
}

void CItemMulti::DelFriend(CChar* pFriend)
{
    ADDTOCALLSTACK("CItemMulti::DelFriend");
    for (std::vector<CChar*>::iterator it = _lFriends.begin(); it != _lFriends.end(); ++it)
    {
        if (*it == pFriend)
        {
            RemoveKeys(*it);
            _lFriends.erase(it);
            return;
        }
    }
}

size_t CItemMulti::GetFriendCount()
{
    return _lFriends.size();
}

int CItemMulti::GetFriendPos(CChar* pTarget)
{
    if (_lFriends.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lFriends.size(); ++i)
    {
        if (_lFriends[i] == pTarget)
        {
            return (int)i;
        }
    }
    return -1;
}

void CItemMulti::AddBan(CChar* pBan)
{
    if (!g_Serv.IsLoading())
    {
        if (!pBan)
        {
            return;
        }
        if (GetBanPos(pBan) >= 0)
        {
            return;
        }
    }
    _lBans.emplace_back(pBan);
}

void CItemMulti::DelBan(CChar* pBan)
{
    ADDTOCALLSTACK("CItemMulti::DelBan");
    for (std::vector<CChar*>::iterator it = _lBans.begin(); it != _lBans.end(); ++it)
    {
        if (*it == pBan)
        {
            _lBans.erase(it);
            return;
        }
    }
}

size_t CItemMulti::GetBanCount()
{
    return _lBans.size();
}

int CItemMulti::GetBanPos(CChar* pBan)
{
    if (_lBans.empty())
    {
        return -1;
    }

    for (size_t i = 0; i < _lBans.size(); ++i)
    {
        if (_lBans[i] == pBan)
        {
            return (int)i;
        }
    }
    return -1;
}

void CItemMulti::AddAccess(CChar* pAccess)
{
	if (!g_Serv.IsLoading())
	{
		if (!pAccess)
		{
			return;
		}
		if (GetAccessPos(pAccess) >= 0)
		{
			return;
		}
	}
	_lAccesses.emplace_back(pAccess);
}

void CItemMulti::DelAccess(CChar* pAccess)
{
	ADDTOCALLSTACK("CItemMulti::DelAccess");
	for (std::vector<CChar*>::iterator it = _lAccesses.begin(); it != _lAccesses.end(); ++it)
	{
		if (*it == pAccess)
		{
			_lAccesses.erase(it);
			return;
		}
	}
}

size_t CItemMulti::GetAccessCount()
{
	return _lAccesses.size();
}

int CItemMulti::GetAccessPos(CChar* pAccess)
{
	if (_lAccesses.empty())
	{
		return -1;
	}

	for (size_t i = 0; i < _lAccesses.size(); ++i)
	{
		if (_lAccesses[i] == pAccess)
		{
			return (int)i;
		}
	}
	return -1;
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

int16 CItemMulti::GetMultiCount()
{
    return _iMultiCount;
}

void CItemMulti::Redeed(bool fDisplayMsg, bool fMoveToBank)
{
    ADDTOCALLSTACK("CItemMulti::Redeed");
    CChar *pOwner = GetOwner();

    TransferLockdownsToMovingCrate();
    RemoveAllComponents();
    TransferAllItemsToMovingCrate();
    if (!pOwner)
    {
        return;
    }
    if (fMoveToBank)
    {
        TransferMovingCrateToBank();
    }
    if (!IsAddon())
    {
        pOwner->GetMultiStorage()->DelMulti(this);
    }


    if (GetKeyNum("REMOVED", true) > 0) // Just don't pass from here again, to avoid duplicated deeds.
    {
        return;
    }
    ITEMID_TYPE itDeed = (ITEMID_TYPE)(GetKeyNum("DEED_ID", true));
    CItem *pDeed = CItem::CreateBase(itDeed <= ITEMID_NOTHING ? itDeed : ITEMID_DEED1);
    if (pDeed)
    {
        pDeed->SetHue(GetHue());
        pDeed->m_itDeed.m_Type = GetID();
        if (m_Attr & ATTR_MAGIC)
        {
            pDeed->SetAttr(ATTR_MAGIC);
        }
        if (fMoveToBank)
        {
            pOwner->GetBank(LAYER_BANKBOX)->ContentAdd(pDeed);
        }
        else
        {
            pOwner->ItemBounce(pDeed, fDisplayMsg);
        }
    }
    SetKeyNum("REMOVED", 1);
    Delete();
}

void CItemMulti::SetMovingCrate(CItemContainer* pCrate)
{
    ADDTOCALLSTACK("CItemMulti::SetMovingCrate");
    if (!pCrate)
    {
        return;
    }
    CItemContainer *pCurrentCrate = GetMovingCrate(false);
    if (pCurrentCrate && pCurrentCrate->GetCount() > 0)
    {
        pCrate->ContentsTransfer(pCurrentCrate, false);
        pCurrentCrate->Delete();
    }
    _pMovingCrate = pCrate;
    pCrate->m_uidLink = GetUID();
    CScript event("events +t_moving_crate");
    pCrate->r_LoadVal(event);
}

CItemContainer* CItemMulti::GetMovingCrate(bool fCreate)
{
    if (_pMovingCrate)
    {
        return _pMovingCrate;
    }
    if (fCreate)
    {
        CItemContainer *pCrate = static_cast<CItemContainer*>(CItem::CreateBase(ITEMID_CRATE1));
        ASSERT(pCrate);
        pCrate->MoveTo(GetTopPoint());
        pCrate->Update();
        SetMovingCrate(pCrate);
        return pCrate;
    }
    return nullptr;
}

void CItemMulti::TransferAllItemsToMovingCrate(TRANSFER_TYPE iType)
{
    ADDTOCALLSTACK("CItemMulti::TransferAllItemsToMovingCrate");
    CItemContainer *pCrate = GetMovingCrate(true);

    if (!pCrate)
    {
        g_Log.EventError("No Moving Crate could be created, aborting TransferAllItemsToMovingCrate()\n");
        return;
    }
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
        if (pItem == nullptr)
        {
            break;
        }
        if (pItem->GetUID() == GetUID() || pItem->GetUID() == pCrate->GetUID())	// this gets deleted seperately.
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
        if (GetComponentPos(pItem))   // Components should never be transfered.
        {
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
    if (pCrate->GetCount() == 0)
    {
        pCrate->Delete();
    }
}

void CItemMulti::TransferLockdownsToMovingCrate()
{
    if (_lLockDowns.empty())
    {
        return;
    }
    CItemContainer *pCrate = GetMovingCrate(true);
    if (!pCrate)
    {
        return;
    }
    for (std::vector<CItem*>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
    {
        CItem *pItem = *it;
        if (pItem)  // Move all valid items.
        {
            CScript event("events -t_house_lockdown");
            pItem->r_LoadVal(event);
            pCrate->ContentAdd(pItem);
            pItem->ClrAttr(ATTR_LOCKEDDOWN);
            pItem->m_uidLink.InitUID();
        }
    }
    _lLockDowns.clear();    // Clear the list, asume invalid items should be cleared too.
}

void CItemMulti::TransferMovingCrateToBank()
{
    ADDTOCALLSTACK("CItemMulti::TransferMovingCrateToBank");
    CItemContainer *pCrate = GetMovingCrate(false);
    CChar *pOwner = GetOwner();
    if (pCrate && pOwner)
    {
        if (pCrate->GetCount() > 0)
        {
            if (pOwner)
            {
                pCrate->RemoveFromView();
                pOwner->GetBank()->ContentAdd(pCrate);
            }
        }
        else
        {
            pCrate->Delete();
        }
    }
}

void CItemMulti::SetAddon(bool fIsAddon)
{
    _fIsAddon = fIsAddon;
}

bool CItemMulti::IsAddon()
{
    return _fIsAddon;
}

void CItemMulti::AddComponent(CItem* pComponent)
{
    ADDTOCALLSTACK("CItemMulti::AddComponent");
    if (!g_Serv.IsLoading())
    {
        if (!pComponent)
        {
            return;
        }
        if (GetComponentPos(pComponent) >= 0)
        {
            return;
        }
    }
    _lComponents.emplace_back(pComponent);
}

void CItemMulti::DelComponent(CItem* pComponent)
{
    ADDTOCALLSTACK("CItemMulti::DelComponent");
    for (std::vector<CItem*>::iterator it = _lComponents.begin(); it != _lComponents.end(); ++it)
    {
        if (*it == pComponent)
        {
            _lComponents.erase(it);
            return;
        }
    }
}

int CItemMulti::GetComponentPos(CItem* pComponent)
{
    if (_lComponents.empty())
    {
        return -1;
    }

    for (size_t i = 0; i < _lComponents.size(); ++i)
    {
        if (_lComponents[i] == pComponent)
        {
            return (int)i;
        }
    }
    return -1;
}

size_t CItemMulti::GetComponentCount()
{
    return _lComponents.size();
}

void CItemMulti::RemoveAllComponents()
{
    if (_lComponents.empty())
    {
        return;
    }
    std::vector<CItem*> _lCopy = _lComponents;
    for (std::vector<CItem*>::iterator it = _lCopy.begin(); it != _lCopy.end(); ++it)
    {
        CItem *pComp = *it;
        if (pComp)
        {
            pComp->Delete();
        }
    }
    _lComponents.clear();
}

void CItemMulti::GenerateBaseComponents(bool & fNeedKey, dword &dwKeyCode)
{
    ASSERT("CItemMulti::GenerateBaseComponents");
    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    ASSERT(pMultiDef);
    for (size_t i = 0; i < pMultiDef->m_Components.size(); i++)
    {
        const CItemBaseMulti::CMultiComponentItem &component = pMultiDef->m_Components.at(i);
        fNeedKey |= Multi_CreateComponent(component.m_id, component.m_dx, component.m_dy, component.m_dz, dwKeyCode, IsAddon());
    }
}

void CItemMulti::SetBaseStorage(uint16 iLimit)
{
    _iBaseStorage = iLimit;
}

uint16 CItemMulti::GetBaseStorage()
{
    return _iBaseStorage;
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
    for (std::vector<CItem*>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
    {
        CItem *pItem = *it;
        if (pItem)
        {
            if (pItem->IsType(IT_CONTAINER) || pItem->IsType(IT_CONTAINER_LOCKED))
            {
                CItemContainer *pCont = static_cast<CItemContainer*>(pItem);
                if (pCont)
                {
                    iCount += (uint16)pCont->GetCount();
                    continue;
                }
            }
            ++iCount;
        }
    }
    return iCount;
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


uint16 CItemMulti::GetMaxLockdowns()
{
    return (GetMaxStorage() - (GetMaxStorage() - (GetMaxStorage() * (uint16)GetLockdownsPercent())/100));
}

uint8 CItemMulti::GetLockdownsPercent()
{
    return _iLockdownsPercent;
}

void CItemMulti::SetLockdownsPercent(uint8 iPercent)
{
    _iLockdownsPercent = iPercent;
}

void CItemMulti::LockItem(CItem *pItem, bool fUpdateFlags)
{
    if (!g_Serv.IsLoading())
    {
        if (!pItem)
        {
            return;
        }
        if (GetLockedItemPos(pItem) >= 0)
        {
            return;
        }
        if (fUpdateFlags)
        {
            pItem->SetAttr(ATTR_LOCKEDDOWN);
            pItem->m_uidLink = GetUID();
            CScript event("events +t_house_lockdown");
            pItem->r_LoadVal(event);
        }

    }
    _lLockDowns.emplace_back(pItem);
}

void CItemMulti::UnlockItem(CItem *pItem, bool fUpdateFlags)
{
    for (std::vector<CItem*>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
    {
        if (*it == pItem)
        {
            _lLockDowns.erase(it);
            break;
        }
    }
    if (fUpdateFlags)
    {
        if (pItem)
        {
            pItem->ClrAttr(ATTR_SECURE);
            pItem->m_uidLink.InitUID();
            CScript event("events -t_house_lockdown");
            pItem->r_LoadVal(event);
        }
    }
}

int CItemMulti::GetLockedItemPos(CItem *pItem)
{
    if (_lLockDowns.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lLockDowns.size(); ++i)
    {
        if (_lLockDowns[i] == pItem)
        {
            return (int)i;
        }
    }
    return -1;
}

size_t CItemMulti::GetLockdownCount()
{
    return _lLockDowns.size();
}

void CItemMulti::AddVendor(CChar *pVendor)
{
    if (!g_Serv.IsLoading())
    {
        if (!pVendor)
        {
            return;
        }
    }
    if (GetHouseVendorPos(pVendor) >= 0)
    {
        return;
    }
    _lVendors.emplace_back(pVendor);
}

void CItemMulti::DelVendor(CChar *pVendor)
{
    for (std::vector<CChar*>::iterator it = _lVendors.begin(); it != _lVendors.end(); ++it)
    {
        if (*it == pVendor)
        {
            _lVendors.erase(it);
            break;
        }
    }
}

int CItemMulti::GetHouseVendorPos(CChar *pVendor)
{
    if (_lVendors.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lVendors.size(); ++i)
    {
        if (_lVendors[i] == pVendor)
        {
            return (int)i;
        }
    }
    return -1;
}

size_t CItemMulti::GetVendorCount()
{
    return _lVendors.size();
}

enum MULTIREF_REF
{
	SHR_ACCESS,
    SHR_BAN,
    SHR_COMPONENT,
    SHR_COOWNER,
    SHR_FRIEND,
    SHR_GUILD,
    SHR_LOCKDOWN,
    SHR_OWNER,
    SHR_REGION,
    SHR_VENDOR,
    SHR_QTY
};

lpctstr const CItemMulti::sm_szRefKeys[SHR_QTY + 1] =
{
	"ACCESS",
    "BAN",
    "COMPONENT",
    "COOWNER",
    "FRIEND",
    "GUILD",
    "LOCKDOWN",
    "OWNER",
    "REGION",
    "VENDOR",
    NULL
};

bool CItemMulti::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK("CItemMulti::r_GetRef");
    int iCmd = FindTableHeadSorted(pszKey, sm_szRefKeys, CountOf(sm_szRefKeys) - 1);

    if (iCmd >= 0)
    {
        pszKey += strlen(sm_szRefKeys[iCmd]);
    }

    switch (iCmd)
    {
		case SHR_ACCESS:
		{
			int i = Exp_GetVal(pszKey);
			SKIP_SEPARATORS(pszKey);
			if ((int)_lAccesses.size() > i)
			{
				CChar *pAccess = _lAccesses[i];
				if (pAccess)
				{
					pRef = pAccess;
					return true;
				}
			}
			return false;
		}
        case SHR_BAN:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lBans.size() > i)
            {
                CChar *pBan = _lBans[i];
                if (pBan)
                {
                    pRef = pBan;
                    return true;
                }
            }
            return false;
        }
        case SHR_COMPONENT:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            CItem *pComp = Multi_FindItemComponent(i);
            if (pComp)
            {
                pRef = pComp;
                return true;
            }
            return false;
            break;
        }
        case SHR_COOWNER:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lCoowners.size() > i)
            {
                CChar *pCoowner = _lCoowners[i];
                if (pCoowner)
                {
                    pRef = pCoowner;
                    return true;
                }
            }
            return false;
        }
        case SHR_FRIEND:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lFriends.size() > i)
            {
                CChar *pFriend = _lFriends[i];
                if (pFriend)
                {
                    pRef = pFriend;
                    return true;
                }
            }
            return false;
        }
        case SHR_LOCKDOWN:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lLockDowns.size() > i)
            {
                CItem *plockdown = _lLockDowns[i];
                if (plockdown)
                {
                    pRef = plockdown;
                    return true;
                }
            }
            return false;
        }
        case SHR_OWNER:
        {
            SKIP_SEPARATORS(pszKey);
            pRef = _pOwner;
            return true;
        }
        case SHR_GUILD:
        {
            SKIP_SEPARATORS(pszKey);
            pRef = _pGuild;
            return true;
        }
        case SHR_REGION:
        {
            SKIP_SEPARATORS(pszKey);
            if (m_pRegion != nullptr)
            {
                pRef = m_pRegion; // Addons do not have region so return it only when not nullptr.
                return true;
            }
            return false;
        }
        case SHR_VENDOR:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lVendors.size() > i)
            {
                CChar *pVendor = _lVendors[i];
                if (pVendor)
                {
                    pRef = pVendor;
                    return true;
                }
            }
            return false;
        }
        default:
            break;
    }

    return(CItem::r_GetRef(pszKey, pRef));
}

enum
{
	SHV_DELACCESS,
    SHV_DELBAN,
    SHV_DELCOMPONENT,
    SHV_DELCOOWNER,
    SHV_DELFRIEND,
    SHV_DELVENDOR,
    SHV_GENERATEBASECOMPONENTS,
    SHV_MOVEALLTOCRATE,
    SHV_MOVELOCKSTOCRATE,
    SHV_MULTICREATE,
    SHV_REDEED,
    SHV_REMOVEALLCOMPONENTS,
    SHV_REMOVEKEYS,
    SHV_UNLOCKITEM,
    SHV_QTY
};

lpctstr const CItemMulti::sm_szVerbKeys[SHV_QTY + 1] =
{
	"DELACCESS",
    "DELBAN",
    "DELCOMPONENT",
    "DELCOOWNER",
    "DELFRIEND",
    "DELVENDOR",
    "GENERATEBASECOMPONENTS",
    "MOVEALLTOCRATE",
    "MOVELOCKSTOCRATE",
    "MULTICREATE",
    "REDEED",
    "REMOVEALLCOMPONENTS",
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
		case SHV_DELACCESS:
		{
			CUID uidAccess = s.GetArgDWVal();
			if (!uidAccess.IsValidUID())
			{
				_lAccesses.clear();
			}
			else
			{
				CChar *pAccess = uidAccess.CharFind();
				DelAccess(pAccess);
			}
			break;
		}
        case SHV_DELBAN:
        {
            CUID uidBan = s.GetArgDWVal();
            if (!uidBan.IsValidUID())
            {
                _lBans.clear();
            }
            else
            {
                CChar *pBan = uidBan.CharFind();
                DelBan(pBan);
            }
            break;
        }
        case SHV_DELCOMPONENT:
        {
            CUID uidComponent = s.GetArgDWVal();
            if (!uidComponent.IsValidUID())
            {
                _lComponents.clear();
            }
            else
            {
                CItem *pComponent = uidComponent.ItemFind();
                DelComponent(pComponent);
            }
            break;
        }
        case SHV_DELCOOWNER:
        {
            CUID uidCoowner = s.GetArgDWVal();
            if (!uidCoowner.IsValidUID())
            {
                _lCoowners.clear();
            }
            else
            {
                CChar *pCoowner = uidCoowner.CharFind();
                DelCoowner(pCoowner);
            }
            break;
        }
        case SHV_DELFRIEND:
        {
            CUID uidFriend = s.GetArgDWVal();
            if (!uidFriend.IsValidUID())
            {
                _lFriends.clear();
            }
            else
            {
                CChar *pFriend = uidFriend.CharFind();
                DelFriend(pFriend);
            }
            break;
        }
        case SHV_DELVENDOR:
        {
            CUID uidVendor = s.GetArgDWVal();
            if (!uidVendor.IsValidUID())
            {
                _lVendors.clear();
            }
            else
            {
                CChar *pVendor = uidVendor.CharFind();
                DelVendor(pVendor);
            }
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
                CItem *pItem = uidItem.ItemFind();
                UnlockItem(pItem, fUpdateFlags);
            }
            break;
        }
        case SHV_MOVEALLTOCRATE:
        {
            TransferAllItemsToMovingCrate((TRANSFER_TYPE)s.GetArgDWVal());
            break;
        }
        case SHV_MOVELOCKSTOCRATE:
        {
            TransferLockdownsToMovingCrate();
            break;
        }
        case SHV_MULTICREATE:
        {
            CUID uidChar = s.GetArgDWVal();
            CChar *	pCharSrc = uidChar.CharFind();
            Multi_Setup(pCharSrc, 0);
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
        case SHV_REMOVEALLCOMPONENTS:
        {
            RemoveAllComponents();
            break;
        }
        case SHV_GENERATEBASECOMPONENTS:
        {
            bool fNeedKey = false;
            dword uid = UID_CLEAR;
            GenerateBaseComponents(fNeedKey, uid);
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
	SHL_ACCESSES,
	SHL_ADDACCESS,
    SHL_ADDBAN,
    SHL_ADDCOMPONENT,
    SHL_ADDCOOWNER,
    SHL_ADDFRIEND,
    SHL_ADDKEY,
    SHL_ADDVENDOR,
    SHL_BANS,
    SHL_BASESTORAGE,
    SHL_BASEVENDORS,
    SHL_COMP,
    SHL_COMPONENTS,
    SHL_COOWNERS,
    SHL_CURRENTSTORAGE,
    SHL_FRIENDS,
	SHL_GETACCESSPOS,
    SHL_GETBANPOS,
    SHL_GETCOMPONENTPOS,
    SHL_GETCOOWNERPOS,
    SHL_GETFRIENDPOS,
    SHL_GETHOUSEVENDORPOS,
    SHL_GETLOCKEDITEMPOS,
    SHL_GUILD,
    SHL_HOUSETYPE,
    SHL_INCREASEDSTORAGE,
    SHL_ISOWNER,
    SHL_LOCKDOWNS,
    SHL_LOCKDOWNSPERCENT,
    SHL_LOCKITEM,
    SHL_MAXLOCKDOWNS,
    SHL_MAXSTORAGE,
    SHL_MAXVENDORS,
    SHL_MOVINGCRATE,
    SHL_OWNER,
    SHL_REGION,
    SHL_REMOVEKEYS,
    SHL_VENDORS,
    SHL_QTY
};

const lpctstr CItemMulti::sm_szLoadKeys[SHL_QTY + 1] =
{
	"ACCESSES",
	"ADDACCESS",
    "ADDBAN",
    "ADDCOMPONENT",
    "ADDCOOWNER",
    "ADDFRIEND",
    "ADDKEY",
    "ADDVENDOR",
    "BANS",
    "BASESTORAGE",
    "BASEVENDORS",
    "COMP",
    "COMPONENTS",
    "COOWNERS",
    "CURRENTSTORAGE",
    "FRIENDS",
	"GETACCESSPOS",
    "GETBANPOS",
    "GETCOMPONENTPOS",
    "GETCOOWNERPOS",
    "GETFRIENDPOS",
    "GETHOUSEVENDORPOS",
    "GETLOCKEDITEMPOS",
    "GUILD",
    "HOUSETYPE",
    "INCREASEDSTORAGE",
    "ISOWNER",
    "LOCKDOWNS",
    "LOCKDOWNSPERCENT",
    "LOCKITEM",
    "MAXLOCKDOWNS",
    "MAXSTORAGE",
    "MAXVENDORS",
    "MOVINGCRATE",
    "OWNER",
    "REGION",
    "REMOVEKEYS",
    "VENDORS",
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
    if (_pOwner)
    {
        s.WriteKeyHex("OWNER", _pOwner->GetUID());
    }
    if (!_lCoowners.empty())
    {
        for (std::vector<CChar*>::iterator it = _lCoowners.begin(); it != _lCoowners.end(); ++it)
        {
            s.WriteKeyHex("ADDCOOWNER", (*it)->GetUID());
        }
    }
    if (!_lFriends.empty())
    {
        for (std::vector<CChar*>::iterator it = _lFriends.begin(); it != _lFriends.end(); ++it)
        {
            s.WriteKeyHex("ADDFRIEND", (*it)->GetUID());
        }
    }
    if (!_lVendors.empty())
    {
        for (std::vector<CChar*>::iterator it = _lVendors.begin(); it != _lVendors.end(); ++it)
        {
            s.WriteKeyHex("ADDVENDOR", (*it)->GetUID());
        }
    }
    if (!_lLockDowns.empty())
    {
        for (std::vector<CItem*>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
        {
            s.WriteKeyHex("LOCKITEM", (*it)->GetUID());
        }
    }
    if (!_lComponents.empty())
    {
        for (std::vector<CItem*>::iterator it = _lComponents.begin(); it != _lComponents.end(); ++it)
        {
            s.WriteKeyHex("ADDCOMPONENT", (*it)->GetUID());
        }
    }
}

bool CItemMulti::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CItemMulti::r_WriteVal");
    int iCmd = FindTableHeadSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iCmd >= 0)
    {
        pszKey += strlen(sm_szLoadKeys[iCmd]);
    }
    SKIP_SEPARATORS(pszKey);
    switch (iCmd)
    {
        case SHL_COMP:
        {
            const CItemBaseMulti *pMultiDef = Multi_GetDef();
            pszKey += 4;

            // no component uid
            if (*pszKey == '\0')
            {
                sVal.FormatSTVal(pMultiDef->m_Components.size());
            }
            else if (*pszKey == '.')
            {
                CItemBaseMulti::CMultiComponentItem	item;

                SKIP_SEPARATORS(pszKey);
                size_t iQty = Exp_GetVal(pszKey);
                if (pMultiDef->m_Components.IsValidIndex(iQty) == false)
                    return false;

                SKIP_SEPARATORS(pszKey);
                item = pMultiDef->m_Components.at(iQty);

                if (!strnicmp(pszKey, "ID", 2))
                {
                    sVal.FormatVal(item.m_id);
                }
                else if (!strnicmp(pszKey, "DX", 2))
                {
                    sVal.FormatVal(item.m_dx);
                }
                else if (!strnicmp(pszKey, "DY", 2))
                {
                    sVal.FormatVal(item.m_dy);
                }
                else if (!strnicmp(pszKey, "DZ", 2))
                {
                    sVal.FormatVal(item.m_dz);
                }
                else if (!strnicmp(pszKey, "D", 1))
                {
                    sVal.Format("%i,%i,%i", item.m_dx, item.m_dy, item.m_dz);
                }
                else
                {
                    sVal.Format("%u,%i,%i,%i", item.m_id, item.m_dx, item.m_dy, item.m_dz);
                }
            }
            else
            {
                return false;
            }
            break;
        }
        // House Permissions
        case SHL_OWNER:
        {
            CChar *pOwner = GetOwner();
            if (!IsStrEmpty(pszKey))
            {
                if (pOwner)
                {
                    return pOwner->r_WriteVal(pszKey, sVal, pSrc);
                }
            }
            if (pOwner)
            {
                sVal.FormatDWVal(pOwner->GetUID());
            }
            else
            {
                sVal.FormatDWVal(0);
            }
            break;
        }
        case SHL_GUILD:
        {
            CItemStone *pGuild = GetGuild();
            if (!IsStrEmpty(pszKey))
            {
                if (pGuild)
                {
                    return pGuild->r_WriteVal(pszKey, sVal, pSrc);
                }
            }
            if (pGuild)
            {
                sVal.FormatDWVal(pGuild->GetUID());
            }
            else
            {
                sVal.FormatDWVal(0);
            }
            break;
        }
        case SHL_ISOWNER:
        {
            CUID uidOwner = static_cast<CUID>(Exp_GetVal(pszKey));
            CChar *pOwner = uidOwner.CharFind();
            sVal.FormatBVal(IsOwner(pOwner));
            break;
        }
        case SHL_GETCOOWNERPOS:
        {
            CUID uidCoowner = static_cast<CUID>(Exp_GetVal(pszKey));
            CChar *pCoowner = uidCoowner.CharFind();
            sVal.FormatVal(GetCoownerPos(pCoowner));
            break;
        }
        case SHL_COOWNERS:
        {
            sVal.FormatSTVal(GetCoownerCount());
            break;
        }
        case SHL_GETFRIENDPOS:
        {
            CUID uidFriend = static_cast<CUID>(Exp_GetVal(pszKey));
            CChar *pCoowner = uidFriend.CharFind();
            sVal.FormatVal(GetFriendPos(pCoowner));
            break;
        }
        case SHL_FRIENDS:
        {
            sVal.FormatSTVal(GetFriendCount());
            break;
        }
		case SHL_ACCESSES:
		{
			sVal.FormatSTVal(GetAccessCount());
			break;
		}
		case SHL_GETACCESSPOS:
		{
			CUID uidAccess = static_cast<CUID>(Exp_GetVal(pszKey));
			CChar *pAccess = uidAccess.CharFind();
			sVal.FormatVal(GetAccessPos(pAccess));
			break;
		}
        case SHL_BANS:
        {
            sVal.FormatSTVal(GetBanCount());
            break;
        }
        case SHL_GETBANPOS:
        {
            CUID uidBan = static_cast<CUID>(Exp_GetVal(pszKey));
            CChar *pBan = uidBan.CharFind();
            sVal.FormatVal(GetBanPos(pBan));
            break;
        }

        // House General
        case SHL_HOUSETYPE:
        {
            sVal.FormatU8Val((uint8)_iHouseType);
            break;
        }
        case SHL_GETCOMPONENTPOS:
        {
            CUID uidComp = static_cast<CUID>(Exp_GetVal(pszKey));
            CItem *pComp = uidComp.ItemFind();
            sVal.FormatVal(GetComponentPos(pComp));
            break;
        }
        case SHL_MOVINGCRATE:
        {
            sVal.FormatDWVal(GetMovingCrate(true)->GetUID());
            break;
        }
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
                    break;
                }
            }
            sVal.FormatDWVal(0);
            break;
        }

        // House Storage
        case SHL_COMPONENTS:
        {
            sVal.FormatSTVal(GetComponentCount());
            break;
        }
        case SHL_LOCKDOWNS:
        {
            sVal.FormatSTVal(GetLockdownCount());
            break;
        }
        case SHL_VENDORS:
        {
            sVal.FormatSTVal(GetVendorCount());
            break;
        }
        case SHL_BASESTORAGE:
        {
            sVal.FormatU16Val(GetBaseStorage());
            break;
        }
        case SHL_BASEVENDORS:
        {
            sVal.FormatU8Val(GetBaseVendors());
            break;
        }
        case SHL_CURRENTSTORAGE:
        {
            sVal.FormatU16Val(GetCurrentStorage());
            break;
        }
        case SHL_INCREASEDSTORAGE:
        {
            sVal.FormatU16Val(GetIncreasedStorage());
            break;
        }
        case SHL_GETHOUSEVENDORPOS:
        {
            sVal.FormatVal(GetHouseVendorPos(((CUID)Exp_GetVal(pszKey)).CharFind()));
            break;
        }
        case SHL_GETLOCKEDITEMPOS:
        {
            sVal.FormatVal(GetLockedItemPos(((CUID)Exp_GetVal(pszKey)).ItemFind()));
            break;
        }
        case SHL_LOCKDOWNSPERCENT:
        {
            sVal.FormatU8Val(GetLockdownsPercent());
            break;
        }
        case SHL_MAXLOCKDOWNS:
        {
            sVal.FormatU16Val(GetMaxLockdowns());
            break;
        }
        case SHL_MAXSTORAGE:
        {
            sVal.FormatU16Val(GetMaxStorage());
            break;
        }
        case SHL_MAXVENDORS:
        {
            sVal.FormatU16Val(GetMaxVendors());
            break;
        }
        default:
            return CItem::r_WriteVal(pszKey, sVal, pSrc);
    }
    return true;
}

bool CItemMulti::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CItemMulti::r_LoadVal");
    EXC_TRY("LoadVal");

    int iCmd = FindTableHeadSorted(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);

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
        // misc
        case SHL_HOUSETYPE:
        {
            _iHouseType = (HOUSE_TYPE)s.GetArgU8Val();
            break;
        }
        case SHL_MOVINGCRATE:
        {
            SetMovingCrate(static_cast<CItemContainer*>(static_cast<CUID>(s.GetArgDWVal()).ItemFind()));
            break;
        }
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

        // House Permissions.
        case SHL_OWNER:
        {
            lpctstr pszKey = s.GetKey();
            pszKey += 5;
            if (*pszKey == '.')
            {
                CChar *pOwner = GetOwner();
                if (pOwner)
                {
                    return pOwner->r_LoadVal(s);
                }
                return false;
            }
            SetOwner(static_cast<CUID>(s.GetArgDWVal()).CharFind());
            break;
        }
        case SHL_GUILD:
        {
            lpctstr pszKey = s.GetKey();
            pszKey += 5;
            if (*pszKey == '.')
            {
                CItemStone *pGuild = GetGuild();
                if (pGuild)
                {
                    return pGuild->r_LoadVal(s);
                }
                return false;
            }
            CItem *pItem = static_cast<CUID>(s.GetArgDWVal()).ItemFind();
            if (pItem)
            {
                CItemStone *pStone = static_cast<CItemStone*>(pItem);
                if (pStone)
                {
                    SetGuild(pStone);
                }
                else
                {
                    return false;
                }
            }
            break;
        }
        case SHL_ADDCOOWNER:
        {
            AddCoowner(static_cast<CUID>(s.GetArgDWVal()).CharFind());
            break;
        }
        case SHL_ADDFRIEND:
        {
            AddFriend(static_cast<CUID>(s.GetArgDWVal()).CharFind());
            break;
        }
        case SHL_ADDBAN:
        {
            AddBan(static_cast<CUID>(s.GetArgDWVal()).CharFind());
            break;
        }
		case SHL_ADDACCESS:
		{
			AddAccess(static_cast<CUID>(s.GetArgDWVal()).CharFind());
			break;
		}

            // House Storage
        case SHL_ADDCOMPONENT:
        {
            AddComponent(((CUID)s.GetArgDWVal()).ItemFind());
            break;
        }
        case SHL_ADDVENDOR:
        {
            AddVendor(((CUID)s.GetArgDWVal()).CharFind());
            break;
        }
        case SHL_BASESTORAGE:
        {
            SetBaseStorage(s.GetArgU16Val());
            break;
        }
        case SHL_BASEVENDORS:
        {
            SetBaseVendors(s.GetArgU8Val());
            break;
        }
        case SHL_INCREASEDSTORAGE:
        {
            SetIncreasedStorage(s.GetArgU16Val());
            break;
        }
        case SHL_LOCKDOWNSPERCENT:
        {
            SetLockdownsPercent(s.GetArgU8Val());
            break;
        }
        case SHL_LOCKITEM:
        {
            int64 piCmd[2];
            Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
            CItem *pItem = ((CUID)(dword)piCmd[0]).ItemFind();
            bool fUpdateFlags = piCmd[1] ? true : false;
            LockItem(pItem, fUpdateFlags);
            break;
        }
        default:
            return CItem::r_LoadVal(s);
    }
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return true;
}

void CItemMulti::DupeCopy(const CItem * pItem)
{
    ADDTOCALLSTACK("CItemMulti::DupeCopy");
    CItem::DupeCopy(pItem);
}

CItemMulti *CItemMulti::Multi_Create(CChar *pChar, const CItemBase * pItemDef, CPointMap & pt, CItem *pDeed)
{
    if (!pChar)
    {
        return nullptr;
    }

    const CItemBaseMulti * pMultiDef = dynamic_cast <const CItemBaseMulti *> (pItemDef);
    bool fShip = pItemDef->IsType(IT_SHIP);	// must be in water.

    /*
    * First thing to do is to check if the character creating the multi is allowed to have it
    */
    if (fShip)
    {
        if (!pChar->GetMultiStorage()->CanAddShip(pChar, pMultiDef->_iMultiCount))
        {
            return nullptr;
        }
    }
    else
    {
        if (!pChar->GetMultiStorage()->CanAddHouse(pChar, pMultiDef->_iMultiCount))
        {
            return nullptr;
        }
    }

    /*
    * There is a small difference in the coordinates where the mouse is in the screen and the ones received,
    * let's remove that difference.
    * Note: this fix is added here, before GM check, because otherwise they will place houses on wrong position.
    */
    if (CItemBase::IsID_Multi(pItemDef->GetID()) || pMultiDef->IsType(IT_MULTI_ADDON))  
    {
        pt.m_y -= (short)(pMultiDef->m_rect.m_bottom - 1);
        pt.m_x += 1;
    }

    if (!pChar->IsPriv(PRIV_GM))
    {
        if ((pMultiDef != nullptr) && !(pDeed->IsAttr(ATTR_MAGIC)))
        {
            CRect rect = pMultiDef->m_rect; // Create a rect equal to the multi's one.
            rect.m_map = pt.m_map;          // set it's map to the current map.
            rect.OffsetRect(pt.m_x, pt.m_y);// fill the rect.
            CPointMap ptn = pt;             // A copy to work on.

            // Check for chars in the way, just search for any char in the house area, no extra tiles, it's enough for them to do not be inside the house.
            CWorldSearch Area(pt, maximum(rect.GetWidth(), rect.GetHeight()));
            Area.SetSearchSquare(true);
            for (;;)
            {
                CChar * pCharSearch = Area.GetChar();
                if (pCharSearch == nullptr)
                {
                    break;
                }
                if (!rect.IsInside2d(pCharSearch->GetTopPoint()))   // if the char is not inside the rec, ignore him.
                {
                    continue;
                }
                if (pCharSearch->IsPriv(PRIV_GM) && !pChar->CanSee(pCharSearch)) // There is a GM in my way? get through him!
                {
                    continue;
                }

                pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_MULTI_INTWAY), pCharSearch->GetName()); // Did not met any of the above exceptions so i'm blocking the placement, return.
                return nullptr;
            }

            /* Char's check passed.
            * Intensive checks are done now:
            *   - The area of search gets increased by 1 tile: There must be no blocking items from a 1x1 gap outside the house.
            *   - All coord points inside that rect must be valid and have, as much, a Z difference of +-4.
            */
            rect.m_top -= 1;    // 1 tile at each side to leave a gap between houses, checking also that this gap doesn't have any blocking object
            rect.m_right += 1;
            rect.m_left -= 1;   
            rect.m_bottom += 1;
            int x = rect.m_left;

            /*
            * Loop through all the positions of the rect.
            */
            for (; x < rect.m_right; ++x)   // X loop
            {
                // Setting the Top-Left point of the CRect*
                ptn.m_x = (short)x; // *point Left
                int y = rect.m_top; // Reset North for each loop.
                for (; y < rect.m_bottom; ++y)  // Y loop
                {
                    ptn.m_y = (short)y; // *point North

                    if (!ptn.IsValidPoint())    // Invalid point (out of map bounds).
                    {
                        pChar->SysMessageDefault(DEFMSG_ITEMUSE_MULTI_FAIL);
                        return nullptr;
                    }

                    dword dwBlockFlags = (fShip) ? CAN_C_SWIM : CAN_C_WALK; // Flags to check: Ships should check for swimable tiles.
                    /*
                    * Intensive check returning the top Z point of the given X, Y coords
                    * It uses the dwBlockBlacks passed to check the new Z level.
                    * Also update those dwBlockFlags with all the flags found at the given location.
                    * 3rd param is set to also update Z with any house component found in the proccess.
                    */
                    ptn.m_z = g_World.GetHeightPoint2(ptn, dwBlockFlags, true);
                    if (abs(ptn.m_z - pt.m_z) > 4)  // Difference of Z > 4? so much, stop.
                    {
                        pChar->SysMessageDefault(DEFMSG_ITEMUSE_MULTI_BUMP);
                        return nullptr;
                    }
                    if (fShip)
                    {
                        if (!(dwBlockFlags & CAN_I_WATER))  // Ships must be placed on water.
                        {
                            pChar->SysMessageDefault(DEFMSG_ITEMUSE_MULTI_SHIPW);
                            return nullptr;
                        }
                    }
                    else if (dwBlockFlags & (CAN_I_WATER | CAN_I_BLOCK | CAN_I_CLIMB))  // Did the intensive check find some undesired flags? Stop.
                    {
                        pChar->SysMessageDefault(DEFMSG_ITEMUSE_MULTI_BLOCKED);
                        return nullptr;
                    }
                }
            }     



            /*
            * The above loop did not find any blocking item/char, now another loop is done to check for another multis with extended limitations:
            * You can't place your house within a range of 5 tiles bottom of the stairs of any house.
            * Additionally your house can't have it's bottom blocked by another multi in a 5 tiles margin.
            * Simplifying it: the Rect must have an additional +5 tiles of radious on both TOP and BOTTOM points.
            */
            rect.m_top -= 4; // 1 was already added before, so a +4 now is enough.
            rect.m_bottom += 4;
            
            x = rect.m_left;
            for (; x < rect.m_right; ++x)
            {
                ptn.m_x = (short)x;
                int y = rect.m_top;
                for (; y < rect.m_bottom; ++y)
                {
                    ptn.m_y = (short)y;
                    /*
                    * Search for any multi region on that point.
                    */
                    CRegion * pRegion = ptn.GetRegion(REGION_TYPE_MULTI | REGION_TYPE_AREA | REGION_TYPE_ROOM); 
                    /*
                    * If there is no region on that point (invalid point?) or the region has the NOBUILDING flag, stop
                    * Ships are allowed to bypass this check??
                    */
                    if ((pRegion == nullptr) || (pRegion->IsFlag(REGION_FLAG_NOBUILDING) && !fShip))
                    {
                        pChar->SysMessageDefault(DEFMSG_ITEMUSE_MULTI_FAIL);
                        return nullptr;
                    }
                }
            }
        }
    }
    
    CItem * pItemNew = CItem::CreateTemplate(pItemDef->GetID(), nullptr, pChar);
    if (pItemNew == nullptr)
    {
        pChar->SysMessageDefault(DEFMSG_ITEMUSE_MULTI_COLLAPSE);
        return nullptr;
    }
    CItemMulti * pMultiItem = dynamic_cast <CItemMulti*>(pItemNew);

    pItemNew->SetAttr(ATTR_MOVE_NEVER | (pDeed->m_Attr & (ATTR_MAGIC | ATTR_INVIS)));
    pItemNew->SetHue(pDeed->GetHue());
    pItemNew->MoveToUpdate(pt);

    if (pMultiItem)
    {
        pMultiItem->Multi_Setup(pChar, UID_CLEAR);
        pMultiItem->SetKeyNum("DEED_ID", pDeed->GetID());
    }

    if (pItemDef->IsType(IT_STONE_GUILD))
    {
        // Now name the guild
        CItemStone * pStone = dynamic_cast <CItemStone*>(pItemNew);
        pStone->AddRecruit(pChar, STONEPRIV_MASTER);
        pChar->GetClient()->addPromptConsole(CLIMODE_PROMPT_STONE_NAME, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_GUILDSTONE_NEW), pItemNew->GetUID());
    }
    else if (pItemDef->IsType(IT_SHIP))
    {
        pItemNew->Sound(Calc_GetRandVal(2) ? 0x12 : 0x13);
    }
    return pMultiItem;
}

void CItemMulti::OnComponentCreate(CItem * pComponent, bool fIsAddon)
{
    UNREFERENCED_PARAMETER(fIsAddon);
    CScript event("+t_house_component");
    pComponent->m_OEvents.r_LoadVal(event, RES_EVENTS);
    AddComponent(pComponent);
}

CMultiStorage::CMultiStorage()
{
}

CMultiStorage::~CMultiStorage()
{
}

void CMultiStorage::AddMulti(CItemMulti * pMulti)
{
    if (pMulti->IsType(IT_SHIP))
    {
        AddShip(static_cast<CItemShip*>(pMulti));
    }
    else
    {
        AddHouse(pMulti);
    }
}

void CMultiStorage::DelMulti(CItemMulti * pMulti)
{
    if (pMulti->IsType(IT_SHIP))
    {
        DelShip(static_cast<CItemShip*>(pMulti));
    }
    else
    {
        DelHouse(pMulti);
    }
}

void CMultiStorage::AddHouse(CItemMulti *pHouse)
{
    ADDTOCALLSTACK("CMultiStorage::AddHouse");
    if (!pHouse)
    {
        return;
    }
    if (GetHousePos(pHouse) >= 0)
    {
        return;
    }
    _iHousesTotal += pHouse->GetMultiCount();
    _lHouses.emplace_back(pHouse);
}

void CMultiStorage::DelHouse(CItemMulti *pHouse)
{
    ADDTOCALLSTACK("CMultiStorage::DelHouse");
    if (_lHouses.empty())
    {
        return;
    }
    for (std::vector<CItemMulti*>::iterator it = _lHouses.begin(); it != _lHouses.end(); ++it)
    {
        if (*it == pHouse)
        {
            _iHousesTotal -= pHouse->GetMultiCount();
            _lHouses.erase(it);
            return;
        }
    }
}

bool CMultiStorage::CanAddHouse(CChar *pChar, int16 iHouseCount)
{
    ADDTOCALLSTACK("CMultiStorage::CanAddHouse");
    if (iHouseCount == 0 || pChar->IsPriv(PRIV_GM))
    {
        return true;
    }
    uint8 iMaxHouses = pChar->_iMaxHouses;
    if (iMaxHouses == 0)
    {
        if (pChar->GetClient())
        {
            iMaxHouses = pChar->GetClient()->GetAccount()->_iMaxHouses;
        }
    }
    if (iMaxHouses == 0)
    {
        return true;
    }
    if (GetHouseCountTotal() + iHouseCount <= iMaxHouses)
    {
        return true;
    }
    pChar->SysMessageDefault(DEFMSG_HOUSE_ADD_LIMIT);
    return false;
}

bool CMultiStorage::CanAddHouse(CItemStone * pStone, int16 iHouseCount)
{
    UNREFERENCED_PARAMETER(pStone);
    UNREFERENCED_PARAMETER(iHouseCount);
    return true;
}

int16 CMultiStorage::GetHousePos(CItemMulti *pHouse)
{
    if (_lHouses.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lHouses.size(); ++i)
    {
        if (_lHouses[i] == pHouse)
        {
            return (int16)i;
        }
    }
    return -1;
}

int16 CMultiStorage::GetHouseCountTotal()
{
    return _iHousesTotal;
}

int16 CMultiStorage::GetHouseCountReal()
{
    return (int16)_lHouses.size();
}

CItemMulti * CMultiStorage::GetHouseAt(int16 iPos)
{
    return _lHouses[iPos];
}

void CMultiStorage::ClearHouses()
{
    _lHouses.clear();
}

void CMultiStorage::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CMultiStorage::r_Write");
    if (!_lHouses.empty())
    {
        for (std::vector<CItemMulti*>::iterator it = _lHouses.begin(); it != _lHouses.end(); ++it)
        {
            s.WriteKeyHex("ADDHOUSE", (*it)->GetUID());
        }
    }
    if (!_lShips.empty())
    {
        for (std::vector<CItemShip*>::iterator it = _lShips.begin(); it != _lShips.end(); ++it)
        {
            s.WriteKeyHex("ADDSHIP", (*it)->GetUID());
        }
    }
}


void CMultiStorage::AddShip(CItemShip *pShip)
{
    ADDTOCALLSTACK("CMultiStorage::AddShip");
    if (!pShip)
    {
        return;
    }
    if (GetShipPos(pShip) >= 0)
    {
        return;
    }
    _iShipsTotal += pShip->GetMultiCount();
    _lShips.emplace_back(pShip);
}

void CMultiStorage::DelShip(CItemShip *pShip)
{
    ADDTOCALLSTACK("CMultiStorage::DelShip");
    if (_lShips.empty())
    {
        return;
    }
    for (std::vector<CItemShip*>::iterator it = _lShips.begin(); it != _lShips.end(); ++it)
    {
        if (*it == pShip)
        {
            _iShipsTotal -= pShip->GetMultiCount();
            _lShips.erase(it);
            return;
        }
    }
}

bool CMultiStorage::CanAddShip(CChar *pChar, int16 iShipCount)
{
    ADDTOCALLSTACK("CMultiStorage::CanAddShip");
    if (iShipCount == 0 || pChar->IsPriv(PRIV_GM))
    {
        return true;
    }
    uint8 iMaxShips = pChar->_iMaxShips;
    if (iMaxShips == 0)
    {
        if (pChar->GetClient())
        {
            iMaxShips = pChar->GetClient()->GetAccount()->_iMaxShips;
        }
    }
    if (iMaxShips == 0)
    {
        return true;
    }
    if (GetShipCountTotal() + iShipCount <= iMaxShips)
    {
        return true;
    }
    pChar->SysMessageDefault(DEFMSG_HOUSE_ADD_LIMIT);
    return false;
}

bool CMultiStorage::CanAddShip(CItemStone * pStone, int16 iShipCount)
{
    UNREFERENCED_PARAMETER(pStone);
    UNREFERENCED_PARAMETER(iShipCount);
    return true;
}

int16 CMultiStorage::GetShipPos(CItemShip *pShip)
{
    if (_lShips.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lShips.size(); ++i)
    {
        if (_lShips[i] == pShip)
        {
            return (int16)i;
        }
    }
    return -1;
}

int16 CMultiStorage::GetShipCountTotal()
{
    return _iShipsTotal;
}

int16 CMultiStorage::GetShipCountReal()
{
    return (int16)_lShips.size();
}

CItemShip * CMultiStorage::GetShipAt(int16 iPos)
{
    return _lShips[iPos];
}

void CMultiStorage::ClearShips()
{
    _lShips.clear();
}
