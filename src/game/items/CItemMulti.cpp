
#include "../../common/CException.h"
#include "../../common/CUIDExtra.h"
#include "../../common/sphereproto.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"
#include "../CWorld.h"
#include "CItemMulti.h"

/////////////////////////////////////////////////////////////////////////////

CItemMulti::CItemMulti(ITEMID_TYPE id, CItemBase * pItemDef) :	// CItemBaseMulti
    CItem(id, pItemDef)
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
    if (pMultiDef == NULL)
    {
        DEBUG_ERR(("Bad Multi type 0%x, uid=0%x\n", GetID(), (dword)GetUID()));
        return false;
    }

    if (m_pRegion == NULL)
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
            pItem->m_itKey.m_UIDLock.SetPrivateUID(dwKeyCode);	// Set the key id for the key/sign.
            m_uidLink.SetPrivateUID(pItem->GetUID());
            fNeedKey = true;
            break;
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

void CItemMulti::Multi_Create(CChar * pChar, dword dwKeyCode)
{
    ADDTOCALLSTACK("CItemMulti::Multi_Create");
    // Create house or Ship extra stuff.
    // ARGS:
    //  dwKeyCode = set the key code for the doors/sides to this in case it's a drydocked ship.
    // NOTE:
    //  This can only be done after the house is given location.

    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    // We are top level.
    if (pMultiDef == NULL || !IsTopLevel())
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
    if (pChar)
    {
        SetOwner(pChar->GetUID());
        pChar->AddHouse(GetUID());
    }

    if (IsAddon())   // Addons doesn't require keys.
    {
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
        return _lComponents[iComp].ItemFind();
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
    if (pTiller == nullptr)
    {
        pTiller = Multi_FindItemType(IsType(IT_SHIP) ? IT_SHIP_TILLER : IT_SIGN_GUMP);
        if (pTiller == nullptr)
        {
            return(this);
        }
        m_uidLink = pTiller->GetUID();
    }
    return pTiller;
}

void CItemMulti::OnHearRegion(lpctstr pszCmd, CChar * pSrc)
{
    ADDTOCALLSTACK("CItemMulti::OnHearRegion");
    // IT_SHIP or IT_MULTI

    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    if (pMultiDef == NULL)
        return;
    TALKMODE_TYPE		mode = TALKMODE_SAY;

    for (size_t i = 0; i < pMultiDef->m_Speech.GetCount(); i++)
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

void CItemMulti::SetOwner(CUID uidOwner)
{
    ADDTOCALLSTACK("CItemMulti::SetOwner");
    if (!uidOwner.IsValidUID())
    {
        g_Log.EventError("Setting wrong Owner UID '0x#08x' to multi with UID '0x#08x'\n",uidOwner, GetUID());
        return;
    }
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
    pNewOwner->AddHouse(uidHouse);
    _uidOwner = uidOwner;
}

bool CItemMulti::IsOwner(CUID uidTarget)
{
    return (_uidOwner == uidTarget);
}

void CItemMulti::AddCoowner(CUID uidCoowner)
{
    ADDTOCALLSTACK("CItemMulti::AddCoowner");
    if (!g_Serv.IsLoading())
    {
        CChar *pCoowner = uidCoowner.CharFind();
        if (!pCoowner)
        {
            return;
        }
        if (GetCoownerPos(uidCoowner) >= 0)
        {
            return;
        }
    }
    _lCoowners.emplace_back(uidCoowner);
}

void CItemMulti::DelCoowner(CUID uidCoowner)
{
    ADDTOCALLSTACK("CItemMulti::DelCoowner");
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

size_t CItemMulti::GetCoownerCount()
{
    return _lCoowners.size();
}

int CItemMulti::GetCoownerPos(CUID uidTarget)
{
    if (_lCoowners.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lCoowners.size(); ++i)
    {
        if (_lCoowners[i] == uidTarget)
        {
            return (int)i;
        }
    }
    return -1;
}

void CItemMulti::AddFriend(CUID uidFriend)
{
    ADDTOCALLSTACK("CItemMulti::AddFriend");
    if (!g_Serv.IsLoading())
    {
        CChar *pFriend = uidFriend.CharFind();
        if (!pFriend)
        {
            return;
        }
        if (GetFriendPos(uidFriend) >= 0)
        {
            return;
        }
    }
    _lFriends.emplace_back(uidFriend);
}

void CItemMulti::DelFriend(CUID uidFriend)
{
    ADDTOCALLSTACK("CItemMulti::DelFriend");
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

size_t CItemMulti::GetFriendCount()
{
    return _lFriends.size();
}

int CItemMulti::GetFriendPos(CUID uidTarget)
{
    if (_lFriends.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lFriends.size(); ++i)
    {
        if (_lFriends[i] == uidTarget)
        {
            return (int)i;
        }
    }
    return -1;
}

void CItemMulti::AddBan(CUID uidBan)
{
    if (!g_Serv.IsLoading())
    {
        CChar *pBan = uidBan.CharFind();
        if (!pBan)
        {
            return;
        }
        if (GetBanPos(uidBan) >= 0)
        {
            return;
        }
    }
    _lBans.emplace_back(uidBan);
}

void CItemMulti::DelBan(CUID uidBan)
{
    ADDTOCALLSTACK("CItemMulti::DelFriend");
    for (std::vector<CUID>::iterator it = _lBans.begin(); it != _lBans.end(); ++it)
    {
        if (*it == uidBan)
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

int CItemMulti::GetBanPos(CUID uidBan)
{
    if (_lBans.empty())
    {
        return -1;
    }

    for (size_t i = 0; i < _lBans.size(); ++i)
    {
        if (_lBans[i] == uidBan)
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

void CItemMulti::Redeed(bool fDisplayMsg, bool fMoveToBank)
{
    ADDTOCALLSTACK("CItemMulti::Redeed");
    CChar *pOwner = _uidOwner.CharFind();

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
        pOwner->DelHouse(GetUID());
    }


    if (GetKeyNum("REMOVED", true) > 0) // Just don't pass from here again, to avoid duplicated deeds.
    {
        return;
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
    SetKeyNum("REMOVED", 1);
    Delete();
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
        pCrate->Update();
        SetMovingCrate(pCrate->GetUID());
        CScript event("events +t_moving_crate");
        pCrate->r_LoadVal(event);
        return pCrate->GetUID();
    }
    return UID_UNUSED;
}

void CItemMulti::TransferAllItemsToMovingCrate(TRANSFER_TYPE iType)
{
    ADDTOCALLSTACK("CItemMulti::TransferAllItemsToMovingCrate");
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());

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
        if (pItem == NULL)
            break;
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
        if (GetComponentPos(pItem->GetUID()))   // Components should never be transfered.
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
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());
    if (!pCrate)
    {
        return;
    }
    for (std::vector<CUID>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
    {
        CItem *pItem = it->ItemFind();
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
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(false).ItemFind());
    if (pCrate && _uidOwner.IsValidUID())
    {
        if (pCrate->GetCount() > 0)
        {
            CChar *pOwner = _uidOwner.CharFind();
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

void CItemMulti::AddComponent(CUID uidComponent)
{
    ADDTOCALLSTACK("CItemMulti::AddComponent");
    if (!g_Serv.IsLoading())
    {
        CItem *pComponent = uidComponent.ItemFind();
        if (!pComponent)
        {
            return;
        }
        if (GetComponentPos(uidComponent) >= 0)
        {
            return;
        }
    }
    _lComponents.emplace_back(uidComponent);
}

void CItemMulti::DelComponent(CUID uidComponent)
{
    ADDTOCALLSTACK("CItemMulti::DelComponent");
    if (!uidComponent.IsValidUID())
    {
        return;
    }
    for (std::vector<CUID>::iterator it = _lComponents.begin(); it != _lComponents.end(); ++it)
    {
        if (*it == uidComponent)
        {
            _lComponents.erase(it);
            return;
        }
    }
}

int CItemMulti::GetComponentPos(CUID uidComponent)
{
    if (_lComponents.empty())
    {
        return -1;
    }

    for (size_t i = 0; i < _lComponents.size(); ++i)
    {
        if (_lComponents[i] == uidComponent)
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
    std::vector<CUID> _lCopy = _lComponents;
    for (std::vector<CUID>::iterator it = _lCopy.begin(); it != _lCopy.end(); ++it)
    {
        CItem *pComp = it->ItemFind();
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
    size_t iQty = pMultiDef->m_Components.GetCount();
    for (size_t i = 0; i < iQty; i++)
    {
        const CItemBaseMulti::CMultiComponentItem &component = pMultiDef->m_Components.ElementAt(i);
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
    return (GetMaxStorage() - (GetMaxStorage() - (GetMaxStorage() * GetLockdownsPercent())/100));
}

uint8 CItemMulti::GetLockdownsPercent()
{
    return _iLockdownsPercent;
}

void CItemMulti::SetLockdownsPercent(uint8 iPercent)
{
    _iLockdownsPercent = iPercent;
}

void CItemMulti::LockItem(CUID uidItem, bool fUpdateFlags)
{
    if (!g_Serv.IsLoading())
    {
        CItem *pItem = uidItem.ItemFind();
        if (!pItem)
        {
            return;
        }
        if (GetLockedItemPos(uidItem) >= 0)
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
    _lLockDowns.push_back(uidItem);
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
            CScript event("events -t_house_lockdown");
            pItem->r_LoadVal(event);
        }
    }
}

int CItemMulti::GetLockedItemPos(CUID uidItem)
{
    if (_lLockDowns.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lLockDowns.size(); ++i)
    {
        if (_lLockDowns[i] == uidItem)
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

void CItemMulti::AddVendor(CUID uidVendor)
{
    if (!g_Serv.IsLoading())
    {
        CChar *pVendor = uidVendor.CharFind();
        if (!pVendor)
        {
            return;
        }
    }
    if (GetHouseVendorPos(uidVendor) >= 0)
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

int CItemMulti::GetHouseVendorPos(CUID uidVendor)
{
    if (_lVendors.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lVendors.size(); ++i)
    {
        if (_lVendors[i] == uidVendor)
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
    SHR_BAN,
    SHR_COMPONENT,
    SHR_COOWNER,
    SHR_FRIEND,
    SHR_LOCKDOWN,
    SHR_OWNER,
    SHR_REGION,
    SHR_VENDOR,
    SHR_QTY
};

lpctstr const CItemMulti::sm_szRefKeys[SHR_QTY + 1] =
{
    "BAN",
    "COMPONENT",
    "COOWNER",
    "FRIEND",
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

    if (iCmd < 0)
    {
        return false;
    }

    pszKey += strlen(sm_szRefKeys[iCmd]);
    SKIP_SEPARATORS(pszKey);

    switch (iCmd)
    {
        case SHR_BAN:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lBans.size() > i)
            {
                pRef = _lBans[i].CharFind();
                return true;
            }
            return false;
        }
        case SHR_COMPONENT:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            pRef = Multi_FindItemComponent(i);
            return false;
            break;
        }
        case SHR_COOWNER:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lCoowners.size() > i)
            {
                pRef = _lCoowners[i].CharFind();
                return true;
            }
            return false;
        }
        case SHR_FRIEND:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lFriends.size() > i)
            {
                pRef = _lFriends[i].CharFind();
                return true;
            }
            return false;
        }
        case SHR_LOCKDOWN:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lLockDowns.size() > i)
            {
                pRef = _lLockDowns[i].ItemFind();
                return true;
            }
            return false;
        }
        case SHR_OWNER:
        {
            pRef = _uidOwner.CharFind();
            return true;
        }
        case SHR_REGION:
        {
            SKIP_SEPARATORS(pszKey);
            if (m_pRegion)
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
                pRef = _lVendors[i].CharFind();
                return true;
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
    SVH_DELCOMPONENT,
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
    "DELCOMPONENT",
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
        case SVH_DELCOMPONENT:
        {
            CUID uidComponent = s.GetArgDWVal();
            if (!uidComponent.IsValidUID())
            {
                _lComponents.clear();
            }
            else
            {
                DelComponent(uidComponent);
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
                DelVendor(uidVendor);
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
                UnlockItem(uidItem, fUpdateFlags);
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
    SHL_DELBAN,
    SHL_DELCOOWNER,
    SHL_DELCOMPONENT,
    SHL_DELFRIEND,
    SHL_FRIENDS,
    SHL_GETBANPOS,
    SHL_GETCOMPONENTPOS,
    SHL_GETCOOWNERPOS,
    SHL_GETFRIENDPOS,
    SHL_GETHOUSEVENDORPOS,
    SHL_GETLOCKEDITEMPOS,
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
    "DELBAN",
    "DELCOMPONENT",
    "DELCOOWNER",
    "DELFRIEND",
    "FRIENDS",
    "GETBANPOS",
    "GETCOMPONENTPOS",
    "GETCOOWNERPOS",
    "GETFRIENDPOS",
    "GETHOUSEVENDORPOS",
    "GETLOCKEDITEMPOS",
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
    if (!_lLockDowns.empty())
    {
        for (std::vector<CUID>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
        {
            s.WriteKeyHex("LOCKITEM", *it);
        }
    }
    if (!_lVendors.empty())
    {
        for (std::vector<CUID>::iterator it = _lVendors.begin(); it != _lVendors.end(); ++it)
        {
            s.WriteKeyHex("ADDVENDOR", *it);
        }
    }
    if (!_lComponents.empty())
    {
        for (std::vector<CUID>::iterator it = _lComponents.begin(); it != _lComponents.end(); ++it)
        {
            s.WriteKeyHex("ADDCOMPONENT", *it);
        }
    }
}

bool CItemMulti::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CItemMulti::r_WriteVal");
    int iCmd = FindTableHeadSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iCmd < 0)
    {
        return CItem::r_WriteVal(pszKey, sVal, pSrc);
    }

    pszKey += strlen(sm_szLoadKeys[iCmd]);
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
            break;
        }
        // House Permissions
        case SHL_ISOWNER:
        {
            CUID uidOwner = static_cast<CUID>(Exp_GetVal(pszKey));
            sVal.FormatBVal(IsOwner(uidOwner));
            break;
        }
        case SHL_OWNER:
            sVal.FormatDWVal(_uidOwner);
            break;
        case SHL_GETCOOWNERPOS:
        {
            CUID uidCoowner = static_cast<CUID>(Exp_GetVal(pszKey));
            sVal.FormatVal(GetCoownerPos(uidCoowner));
            break;
        }
        case SHL_COOWNERS:
            sVal.FormatSTVal(GetCoownerCount());
            break;
        case SHL_GETFRIENDPOS:
        {
            CUID uidFriend = static_cast<CUID>(Exp_GetVal(pszKey));
            sVal.FormatVal(GetFriendPos(uidFriend));
            break;
        }
        case SHL_FRIENDS:
            sVal.FormatSTVal(GetFriendCount());
            break;
        case SHL_BANS:
            sVal.FormatSTVal(GetBanCount());
            break;
        case SHL_GETBANPOS:
        {
            CUID uidBan = static_cast<CUID>(Exp_GetVal(pszKey));
            sVal.FormatVal(GetBanPos(uidBan));
            break;
        }

        // House General
        case SHL_HOUSETYPE:
            sVal.FormatU8Val((uint8)_iHouseType);
            break;
        case SHL_GETCOMPONENTPOS:
        {
            CUID uidComp = static_cast<CUID>(Exp_GetVal(pszKey));
            sVal.FormatVal(GetComponentPos(uidComp));
            break;
        }
        case SHL_MOVINGCRATE:
            sVal.FormatDWVal(GetMovingCrate(true));
            break;
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
            sVal.FormatVal(GetHouseVendorPos((CUID)Exp_GetVal(pszKey)));
            break;
        }
        case SHL_GETLOCKEDITEMPOS:
        {
            sVal.FormatVal(GetLockedItemPos((CUID)Exp_GetVal(pszKey)));
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
            break;
    }
    return true;
}

bool CItemMulti::r_LoadVal(CScript & s)
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

        // misc
        case SHL_HOUSETYPE:
        {
            _iHouseType = (HOUSE_TYPE)s.GetArgU8Val();
            break;
        }
        case SHL_MOVINGCRATE:
        {
            SetMovingCrate(static_cast<CUID>(s.GetArgDWVal()));
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
            SetOwner(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDCOOWNER:
        {
            AddCoowner(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_DELCOOWNER:
        {
            DelCoowner(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDFRIEND:
        {
            AddFriend(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_DELFRIEND:
        {
            DelFriend(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDBAN:
        {
            AddBan(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_DELBAN:
        {
            DelBan(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }

            // House Storage
        case SHL_ADDCOMPONENT:
        {
            AddComponent((CUID)s.GetArgDWVal());
            break;
        }
        case SHL_ADDVENDOR:
        {
            AddVendor((CUID)s.GetArgDWVal());
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
            CUID uidItem = (dword)piCmd[0];
            bool fUpdateFlags = piCmd[1] ? true : false;
            LockItem(uidItem, fUpdateFlags);
            break;
        }
        default:
            break;
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
    UNREFERENCED_PARAMETER(fIsAddon);
    CScript event("+t_house_component");
    const_cast<CItem*>(pComponent)->m_OEvents.r_LoadVal(event, RES_EVENTS);
    AddComponent(pComponent->GetUID());
}
