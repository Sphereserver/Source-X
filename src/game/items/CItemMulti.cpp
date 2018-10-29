
#include "../../common/CException.h"
#include "../../common/CUIDExtra.h"
#include "../../common/sphereproto.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"
#include "../CWorld.h"
#include "../CResourceLock.h"
#include "../triggers.h"
#include "CItemMulti.h"
#include "CItemShip.h"
#include "CItemContainer.h"

/////////////////////////////////////////////////////////////////////////////

CItemMulti::CItemMulti(ITEMID_TYPE id, CItemBase * pItemDef, bool fTurnable) :	// CItemBaseMulti
    CTimedObject(PROFILE_MULTIS),
    CItem(id, pItemDef),
    CMultiMovable(fTurnable)
{
    CItemBaseMulti * pItemBase = static_cast<CItemBaseMulti*>(Base_GetDef());
    m_shipSpeed.period = pItemBase->m_shipSpeed.period;
    m_shipSpeed.tiles = pItemBase->m_shipSpeed.tiles;
    _eSpeedMode = pItemBase->m_SpeedMode;

    m_pRegion = nullptr;
    _pOwner.InitUID();
    _pMovingCrate.InitUID();
    _pGuild.InitUID();

    _iHouseType = HOUSE_PRIVATE;
    _iMultiCount = pItemBase->_iMultiCount;

    _iBaseStorage = pItemBase->_iBaseStorage;
    _iBaseVendors = pItemBase->_iBaseVendors;
    _iLockdownsPercent = pItemBase->_iLockdownsPercent;
    _iIncreasedStorage = 0;
}

CItemMulti::~CItemMulti()
{

    if (!m_pRegion)
    {
        return;
    }

    if (_pGuild)
    {
        SetGuild(UID_UNUSED);
    }
    if (_pOwner)
    {
        SetOwner(UID_UNUSED);
    }
    if (!_lCoowners.empty())
    {
        for (size_t i = 0; i < _lCoowners.size(); ++i)
        {
            CUID charUID = _lCoowners[i];
            CChar *pChar = charUID.CharFind();
            if (pChar)
            {
                DelCoowner(charUID);
            }
        }
    }
    if (!_lFriends.empty())
    {
        for (size_t i = 0; i < _lFriends.size(); ++i)
        {
            CUID charUID = _lFriends[i];
            CChar *pChar = charUID.CharFind();
            if (pChar)
            {
                DelFriend(charUID);
            }
        }
    }
    if (!_lVendors.empty())
    {
        for (size_t i = 0; i < _lVendors.size(); ++i)
        {
            CUID charUID = _lVendors[i];
            CChar *pChar = charUID.CharFind();
            if (pChar)
            {
                DelVendor(charUID);
            }
        }
    }
    if (!_lLockDowns.empty())
    {
        for (size_t i = 0; i < _lLockDowns.size(); ++i)
        {
            CUID itemUID = _lLockDowns[i];
            CItem *pItem = itemUID.ItemFind();
            if (pItem)
            {
                UnlockItem(itemUID);
            }
        }
    }
    if (!_lComps.empty())
    {
        for (size_t i = 0; i < _lComps.size(); ++i)
        {
            CUID itemUID = _lComps[i];
            CItem *pItem = itemUID.ItemFind();
            if (pItem)
            {
                DelComp(itemUID);
            }
        }
    }
    if (!_lSecureContainers.empty())
    {
        for (size_t i = 0; i < _lSecureContainers.size(); ++i)
        {
            CUID itemUID = _lSecureContainers[i];
            CItemContainer *pItem = static_cast<CItemContainer*>(itemUID.ItemFind());
            if (pItem)
            {
                Release(itemUID);
            }
        }
    }
    if (!_lAddons.empty())
    {
        for (size_t i = 0; i < _lAddons.size(); ++i)
        {
            CUID itemUID = _lAddons[i];
            CItemMulti *pItem = static_cast<CItemMulti*>(itemUID.ItemFind());
            if (pItem)
            {
                DelAddon(itemUID);
            }
        }
    }

    CItemContainer *pMovingCrate = static_cast<CItemContainer*>(GetMovingCrate(false).ItemFind());
    if (pMovingCrate)
    {
        SetMovingCrate(UID_UNUSED);
    }
    MultiUnRealizeRegion();	// unrealize before removed from ground.
    DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
                        // NOTE: ??? This is dangerous to iterators. The "next" item may no longer be valid !

                        // Attempt to remove all the accessory junk.
                        // NOTE: assume we have already been removed from Top Level
    delete m_pRegion;
}

void CItemMulti::Delete(bool bforce)
{
    RemoveAllComps();
    CObjBase::Delete(bforce);
}

const CItemBaseMulti * CItemMulti::Multi_GetDef() const
{
    return(static_cast <const CItemBaseMulti *>(Base_GetDef()));
}

CRegion * CItemMulti::GetRegion() const
{
    return m_pRegion;
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
    return (dynamic_cast <const CItemBaseMulti *> (CItemBase::FindItemBase(id)));
}

bool CItemMulti::MultiRealizeRegion()
{
    ADDTOCALLSTACK("CItemMulti::MultiRealizeRegion");
    // Add/move a region for the multi so we know when we are in it.
    // RETURN: ignored.

    if (!IsTopLevel() || IsType(IT_MULTI_ADDON))
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
    CRectMap rect = pMultiDef->m_rect;
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
    m_pRegion->_pMultiLink = this;

    return m_pRegion->RealizeRegion();
}

void CItemMulti::MultiUnRealizeRegion()
{
    ADDTOCALLSTACK("CItemMulti::MultiUnRealizeRegion");
    if (m_pRegion == nullptr)
    {
        return;
    }
    m_pRegion->_pMultiLink = nullptr;

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

bool CItemMulti::Multi_CreateComponent(ITEMID_TYPE id, short dx, short dy, char dz, dword dwKeyCode)
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
            // Do not break, those need fNeedKey set to true.
        }
        case IT_DOOR:
        case IT_CONTAINER:
            fNeedKey = true;
            break;
        default:
            break;
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
    AddComp(pItem->GetUID());
    if (IsType(IT_MULTI_ADDON))
    {
        fNeedKey = false;
    }
    return fNeedKey;
}

void CItemMulti::Multi_Setup(CChar *pChar, dword dwKeyCode)
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

    if (IsType(IT_MULTI_ADDON))   // Addons doesn't require keys and are not added to any list.
    {
        return;
    }

    Multi_GetSign();	// set the m_uidLink

    if (pChar)
    {
        SetOwner(pChar->GetUID());
    }

    if (pChar)
    {
        m_itShip.m_UIDCreator = pChar->GetUID();
        if (IsType(IT_SHIP))
        {
            if (g_Cfg._fAutoShipKeys)
            {
                GenerateKey(pChar->GetUID(), true);
            }
        }
        else
        {
            if (g_Cfg._fAutoHouseKeys)
            {
                GenerateKey(pChar->GetUID(), true);
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
    if (pItem->m_uidLink == GetUID())
    {
        return true;
    }
    CItemMulti *pMulti = const_cast<CItemMulti*>(this);
    CUID uidItem = pItem->GetUID();
    if (pMulti->GetLockedItemPos(uidItem) != -1)
    {
        return true;
    }
    if (pMulti->GetSecuredContainerPos(uidItem) != -1)
    {
        return true;
    }
    if (pMulti->GetAddonPos(uidItem) != -1)
    {
        return true;
    }
    if (pMulti->GetCompPos(uidItem) != -1)
    {
        return true;
    }
    return false;
}

CItem * CItemMulti::Multi_FindItemComponent(int iComp) const
{
    ADDTOCALLSTACK("CItemMulti::Multi_FindItemComponent");

    if ((int)_lComps.size() > iComp)
    {
        return _lComps[iComp].ItemFind();
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
    if (!CMultiMovable::OnTick())
    {
        return CItem::OnTick();
    }
    return true;
}

void CItemMulti::OnMoveFrom()
{
    ADDTOCALLSTACK("CItemMulti::OnMoveFrom");
    // Being removed from the top level.
    // Might just be moving.

    if (IsType(IT_MULTI_ADDON)) // Addons doesn't have region, don't try to unrealize it.
    {
        CItemMulti *pMulti = m_pRegion->_pMultiLink;
        if (pMulti)
        {
            pMulti->DelAddon(GetUID());
        }
        m_pRegion = nullptr;
    }
    else
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
    if (IsType(IT_MULTI_ADDON)) // Addons doesn't have region, don't try to realize it.
    {
        CRegionWorld *pRegion = dynamic_cast<CRegionWorld*>(GetTopPoint().GetRegion(REGION_TYPE_HOUSE));
        if (!pRegion)
        {
            pRegion = dynamic_cast<CRegionWorld*>(GetTopPoint().GetRegion(REGION_TYPE_AREA));
        }
        ASSERT(pRegion);
        m_pRegion = pRegion;

        CItemMulti *pMulti = m_pRegion->_pMultiLink;
        if (pMulti)
        {
            pMulti->AddAddon(GetUID());
        }
    }
    else
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

void CItemMulti::RevokePrivs(CUID uidSrc)
{
    ADDTOCALLSTACK("CItemMulti::RevokePrivs");
    if (!uidSrc.IsValidUID())
    {
        return;
    }
    CUID charUID = uidSrc;
    if (IsOwner(charUID))
    {
        SetOwner(UID_UNUSED);
    }
    else if (GetCoownerPos(charUID) >= 0)
    {
        DelCoowner(charUID);
    }
    else if (GetFriendPos(charUID) >= 0)
    {
        DelFriend(charUID);
    }
    else if (GetBanPos(charUID) >= 0)
    {
        DelBan(charUID);
    }
    else if (GetAccessPos(charUID) >= 0)
    {
        DelAccess(charUID);
    }
    else if (GetHouseVendorPos(charUID) >= 0)
    {
        DelVendor(charUID);
    }
}

void CItemMulti::SetOwner(CUID uidOwner)
{
    ADDTOCALLSTACK("CItemMulti::SetOwner");
    CChar *pOldOwner = _pOwner.CharFind();
    _pOwner.InitUID();
    if (pOldOwner)  // Old Owner may not exist, was removed?
    {
        pOldOwner->GetMultiStorage()->DelMulti(GetUID());
        RemoveKeys(pOldOwner->GetUID());
    }
    if (!uidOwner.IsValidUID())
    {
        return;
    }
    RevokePrivs(uidOwner);
    CChar *pOwner = uidOwner.CharFind();
    if (pOwner)
    {
        pOwner->GetMultiStorage()->AddMulti(GetUID(), HP_OWNER);
    }
    _pOwner = uidOwner;
}

bool CItemMulti::IsOwner(CUID pTarget)
{
    return (_pOwner == pTarget);
}

CUID CItemMulti::GetOwner()
{
    if (_pOwner && _pOwner.IsValidUID())
    {
        return _pOwner;
    }
    return UID_UNUSED;
}

void CItemMulti::SetGuild(CUID uidGuild)
{
    ADDTOCALLSTACK("CItemMulti::SetGuild");
    CItemStone *pGuild = static_cast<CItemStone*>(GetGuild().ItemFind());
    _pGuild.InitUID();
    if (pGuild)  // Old Guild may not exist, was it removed...?
    {
        pGuild->GetMultiStorage()->DelMulti(GetUID());   // ... if not, unlink it from this multi.
    }
    if (!uidGuild.IsValidUID())  // Just clearing it
    {
        return;
    }
    _pGuild = uidGuild;   // Set the Guild* to a new guildstone.
    pGuild = static_cast<CItemStone*>(uidGuild.ItemFind());
    pGuild->GetMultiStorage()->AddMulti(GetUID(), HP_GUILD);
}

bool CItemMulti::IsGuild(CUID pTarget)
{
    return (_pGuild == pTarget);
}

CUID CItemMulti::GetGuild()
{
    if (_pGuild && _pGuild.IsValidUID())
    {
        return _pGuild;
    }
    return UID_UNUSED;
}

void CItemMulti::AddCoowner(CUID uidCoowner)
{
    ADDTOCALLSTACK("CItemMulti::AddCoowner");
    if (!uidCoowner.IsValidUID())
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetCoownerPos(uidCoowner) >= 0)
        {
            return;
        }
    }
    CChar *pCoowner = uidCoowner.CharFind();
    if (!pCoowner)
    {
        return;
    }
    RevokePrivs(uidCoowner);
    pCoowner->GetMultiStorage()->AddMulti(GetUID(), HP_COOWNER);
    _lCoowners.emplace_back(uidCoowner);
}

void CItemMulti::DelCoowner(CUID uidCoowner)
{
    ADDTOCALLSTACK("CItemMulti::DelCoowner");
    for (size_t i = 0; i < _lCoowners.size(); ++i)
    {
        CUID uid = _lCoowners[i];
        if (uid == uidCoowner)
        {
            CChar *pCoowner = uidCoowner.CharFind();
            if (!pCoowner)
            {
                continue;
            }
            pCoowner->GetMultiStorage()->DelMulti(GetUID());
            RemoveKeys(uid);
            _lCoowners.erase(_lCoowners.begin() + i);
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
    ADDTOCALLSTACK("CItemMulti::GetCoownerPos");
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
    if (!uidFriend.IsValidUID())
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetFriendPos(uidFriend) >= 0)
        {
            return;
        }
    }
    CChar *pFriend = uidFriend.CharFind();
    if (!pFriend)
    {
        return;
    }
    RevokePrivs(uidFriend);
    pFriend->GetMultiStorage()->AddMulti(GetUID(), HP_FRIEND);
    _lFriends.emplace_back(uidFriend);
}

void CItemMulti::DelFriend(CUID uidFriend)
{
    ADDTOCALLSTACK("CItemMulti::DelFriend");
    for (size_t i = 0; i < _lFriends.size(); ++i)
    {
        if (_lFriends[i] == uidFriend)
        {
            CChar *pFriend = uidFriend.CharFind();
            if (!pFriend)
            {
                continue;
            }
            pFriend->GetMultiStorage()->DelMulti(GetUID());
            RemoveKeys(uidFriend);
            _lFriends.erase(_lFriends.begin() + i);
            return;
        }
    }
}

size_t CItemMulti::GetFriendCount()
{
    return _lFriends.size();
}

int CItemMulti::GetFriendPos(CUID uidFriend)
{
    ADDTOCALLSTACK("CItemMulti::GetFriendPos");
    if (_lFriends.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lFriends.size(); ++i)
    {
        if (_lFriends[i] == uidFriend)
        {
            return (int)i;
        }
    }
    return -1;
}

void CItemMulti::AddBan(CUID uidBan)
{
    ADDTOCALLSTACK("CItemMulti::AddBan");
    if (!uidBan)
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetBanPos(uidBan) >= 0)
        {
            return;
        }
    }
    CChar *pBan = uidBan.CharFind();
    if (!pBan)
    {
        return;
    }
    RevokePrivs(uidBan);
    pBan->GetMultiStorage()->AddMulti(GetUID(), HP_BAN);
    _lBans.emplace_back(uidBan);
}

void CItemMulti::DelBan(CUID uidBan)
{
    ADDTOCALLSTACK("CItemMulti::DelBan");
    for (size_t i = 0; i < _lBans.size(); ++i)
    {
        if (_lBans[i] == uidBan)
        {
            CChar *pBan = uidBan.CharFind();
            pBan->GetMultiStorage()->DelMulti(GetUID());
            _lBans.erase(_lBans.begin() + i);
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
    ADDTOCALLSTACK("CItemMulti::GetBanPos");
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

void CItemMulti::AddAccess(CUID uidAccess)
{
    ADDTOCALLSTACK("CItemMulti::AddAccess");
    if (!uidAccess)
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetAccessPos(uidAccess) >= 0)
        {
            return;
        }
    }
    CChar *pAccess = uidAccess.CharFind();
    if (!pAccess)
    {
        return;
    }
    RevokePrivs(uidAccess);
    pAccess->GetMultiStorage()->AddMulti(GetUID(), HP_ACCESSONLY);
    _lAccesses.emplace_back(uidAccess);
}

void CItemMulti::DelAccess(CUID uidAccess)
{
    ADDTOCALLSTACK("CItemMulti::DelAccess");
    for (size_t i = 0; i < _lAccesses.size(); ++i)
    {
        if (_lAccesses[i] == uidAccess)
        {
            CChar *pAccess = uidAccess.CharFind();
            pAccess->GetMultiStorage()->DelMulti(GetUID());
            _lAccesses.erase(_lAccesses.begin() + i);
            return;
        }
    }
}

size_t CItemMulti::GetAccessCount()
{
    return _lAccesses.size();
}

int CItemMulti::GetAccessPos(CUID uidAccess)
{
    if (_lAccesses.empty())
    {
        return -1;
    }

    for (size_t i = 0; i < _lAccesses.size(); ++i)
    {
        if (_lAccesses[i] == uidAccess)
        {
            return (int)i;
        }
    }
    return -1;
}

void CItemMulti::Eject(CUID uidChar)
{
    if (!uidChar.IsValidUID())
    {
        return;
    }
    CChar *pChar = uidChar.CharFind();
    ASSERT(pChar);
    pChar->Spell_Teleport(m_uidLink.ItemFind()->GetTopPoint(), true, false);
}

void CItemMulti::EjectAll(CUID uidCharNoTp)
{
    CWorldSearch Area(m_pRegion->m_pt, Multi_GetMaxDist());
    Area.SetSearchSquare(true);
    CChar *pCharNoTp = uidCharNoTp.CharFind();
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
        if (pChar == pCharNoTp) // Eject all but owner? ie enter customize
        {
            continue;
        }
        Eject(pChar->GetUID());
    }
}

CItem *CItemMulti::GenerateKey(CUID uidTarget, bool fDupeOnBank)
{
    ADDTOCALLSTACK("CItemMulti::GenerateKey");
    if (!uidTarget.IsValidUID())
    {
        return nullptr;
    }
    CChar *pTarget = uidTarget.CharFind();
    if (!pTarget)
    {
        return nullptr;
    }

    // Create the key to the door.
    ITEMID_TYPE id = IsAttr(ATTR_MAGIC) ? ITEMID_KEY_MAGIC : ITEMID_KEY_COPPER;
    CItem *pKey = CreateScript(id, pTarget);
    ASSERT(pKey);
    pKey->SetType(IT_KEY);
    if (g_Cfg.m_fAutoNewbieKeys)
    {
        pKey->SetAttr(ATTR_NEWBIE);
    }
    pKey->SetAttr(m_Attr&ATTR_MAGIC);
    pKey->m_itKey.m_UIDLock.SetPrivateUID(GetUID());
    pKey->m_uidLink = GetUID();

    if (fDupeOnBank)
    {
        // Put in your bankbox
        CItem* pKeyDupe = CItem::CreateDupeItem(pKey);
        CItemContainer* pContBank = pTarget->GetBank();
        if (pContBank)
        {
            pContBank->ContentAdd(pKeyDupe);
            pTarget->SysMessageDefault(DEFMSG_MSG_KEY_DUPEBANK);
        }
        else	// it should never happen...
        {
            delete pKeyDupe;
            g_Log.EventWarn("Can't add multi key to char '%s' (UID 0x" PRIx32 "): no bank box!", pTarget->GetName(), (dword)pTarget->GetUID());
        }
    }

    // Put in your pack
    CItemContainer* pContPack = pTarget->GetPackSafe();
    if (pContPack)
    {
        pContPack->ContentAdd(pKey);
    }
    else	// it should never happen...
    {
        delete pKey;
        g_Log.EventWarn("Can't add multi key to char '%s' (UID 0x" PRIx32 "): no backpack!", pTarget->GetName(), (dword)pTarget->GetUID());
    }

    return pKey;
}

void CItemMulti::RemoveKeys(CUID uidTarget)
{
    ADDTOCALLSTACK("CItemMulti::RemoveKeys");
    if (!uidTarget.IsValidUID())
    {
        return;
    }
    CChar *pTarget = uidTarget.CharFind();
    if (!pTarget)
    {
        return;
    }
    CUID uidHouse = GetUID();
    CItem *pItemNext = nullptr;
    for (CItem* pItemKey = pTarget->GetPack()->GetContentHead(); pItemKey != nullptr; pItemKey = pItemNext)
    {
        pItemNext = pItemKey->GetNext();
        if (pItemKey->m_uidLink == uidHouse)
        {
            pItemKey->Delete();
        }
    }

    for (CItem* pItemKey = pTarget->GetPack()->GetContentHead(); pItemKey != nullptr; pItemKey = pItemNext)
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

void CItemMulti::Redeed(bool fDisplayMsg, bool fMoveToBank, CUID uidChar)
{
    ADDTOCALLSTACK("CItemMulti::Redeed");
    if (GetKeyNum("REMOVED") > 0) // Just don't pass from here again, to avoid duplicated deeds.
    {
        return;
    }

    CChar *pOwner = GetOwner().CharFind();
    ITEMID_TYPE itDeed;
    if (IsType(IT_SHIP))
    {
        itDeed = ITEMID_SHIP_PLANS1;
    }
    else
    {
        itDeed = ITEMID_DEED1;
    }
    TRIGRET_TYPE tRet = TRIGRET_RET_FALSE;
    bool fTransferAll = false;
    CItem *pDeed = CItem::CreateBase(itDeed <= ITEMID_NOTHING ? itDeed : ITEMID_DEED1);
    tchar *pszName = Str_GetTemp();
    CItemBaseMulti * pItemBase = static_cast<CItemBaseMulti*>(Base_GetDef());
    snprintf(pszName, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_DEED_NAME), pItemBase->GetName());
    pDeed->SetName(pszName);

    bool fIsAddon = IsType(IT_MULTI_ADDON);
    if (fIsAddon)
    {
        CItemMulti *pMulti = static_cast<CItemMulti*>(m_uidLink.ItemFind());
        if (pMulti)
        {
            pMulti->DelAddon(GetUID());
        }
    }
    CScriptTriggerArgs args(pDeed);
    args.m_iN1 = itDeed;
    args.m_iN2 = 1; // Transfer / Redeed all items to the moving crate.
    args.m_iN3 = fMoveToBank; // Transfer the Moving Crate to the owner's bank.
    if (IsTrigUsed(TRIGGER_REDEED))
    {
        tRet = OnTrigger(ITRIG_Redeed, uidChar.CharFind(), &args);
        if (args.m_iN2 == 0)
        {
            fMoveToBank = false;
        }
        else
        {
            fTransferAll = true;
            fMoveToBank = args.m_iN3 ? true : false;
        }
    }
    RemoveAllComps();
    if (!fIsAddon) // Addons doesn't have to transfer anything but themselves.
    {
        if (fTransferAll)
        {
            TransferAllItemsToMovingCrate(TRANSFER_ALL);    // Whatever is left unlisted.
        }
    }

    if (!pOwner)
    {
        return;
    }

    if (!fIsAddon)
    {
        if (fMoveToBank)
        {
            TransferMovingCrateToBank();
        }
        pOwner->GetMultiStorage()->DelMulti(GetUID());
    }
    if (tRet == TRIGRET_RET_TRUE)
    {
        pDeed->Delete();
        return;
    }
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

void CItemMulti::SetMovingCrate(CUID uidCrate)
{
    ADDTOCALLSTACK("CItemMulti::SetMovingCrate");
    CItemContainer *pCurrentCrate = static_cast<CItemContainer*>(GetMovingCrate(false).ItemFind());
    CItemContainer *pNewCrate = static_cast<CItemContainer*>(uidCrate.ItemFind());
    
    if (!uidCrate.IsValidUID() || !pNewCrate)
    {
        _pMovingCrate.InitUID();
        return;
    }
    if (pCurrentCrate && pCurrentCrate->GetCount() > 0)
    {
        pCurrentCrate->SetCrateOfMulti(UID_UNUSED);
        pNewCrate->ContentsTransfer(pCurrentCrate, false);
        pCurrentCrate->Delete();
    }
    _pMovingCrate = uidCrate;
    pNewCrate->m_uidLink = GetUID();
    CScript event("events +t_moving_crate");
    pNewCrate->r_LoadVal(event);
    pNewCrate->SetCrateOfMulti(GetUID());
}

CUID CItemMulti::GetMovingCrate(bool fCreate)
{
    ADDTOCALLSTACK("CItemMulti::GetMovingCrate");
    if (_pMovingCrate.IsValidUID())
    {
        return _pMovingCrate;
    }
    if (!fCreate)
    {
        return UID_UNUSED;
    }
    CItemContainer *pCrate = static_cast<CItemContainer*>(CItem::CreateBase(ITEMID_CRATE1));
    ASSERT(pCrate);
    CUID uidCrate = pCrate->GetUID();
    CPointMap pt = GetTopPoint();
    pt.m_z -= 20;
    pCrate->MoveTo(pt); // Move the crate to z -20
    pCrate->Update();
    SetMovingCrate(uidCrate);
    return uidCrate;
}

void CItemMulti::TransferAllItemsToMovingCrate(TRANSFER_TYPE iType)
{
    ADDTOCALLSTACK("CItemMulti::TransferAllItemsToMovingCrate");
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());
    ASSERT(pCrate);
    //Transfer Types
    bool fTransferAddons = ((iType & TRANSFER_ADDONS) || (iType & TRANSFER_ALL));
    bool fTransferAll = (iType & TRANSFER_ALL);
    bool fTransferLockDowns = ((iType & TRANSFER_LOCKDOWNS) || (iType & TRANSFER_ALL));
    bool fTransferSecuredContainers = ((iType &TRANSFER_SECURED) || (iType & TRANSFER_ALL));
    if (fTransferLockDowns) // Try to move locked down items, also to reduce the CWorldSearch impact.
    {
        TransferLockdownsToMovingCrate();
    }
    if (fTransferSecuredContainers)
    {
        TransferSecuredToMovingCrate();
    }
    if (fTransferAddons)
    {
        RedeedAddons();
    }
    CPointMap ptArea;
    if (IsType(IT_MULTI_ADDON))
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
        if (pItem->GetTopPoint().GetRegion(REGION_TYPE_HOUSE) != GetTopPoint().GetRegion(REGION_TYPE_HOUSE))
        {
            continue;
        }
        if (pItem->IsType(IT_STONE_GUILD))
        {
            continue;
        }
        if (pItem->GetUID() == GetUID() || pItem->GetUID() == pCrate->GetUID()) // Multi itself or the Moving Crate, neither is not handled here.
        {
            continue;
        }
        if (GetCompPos(pItem->GetUID()) != -1)   // Components should never be transfered.
        {
            continue;
        }
        if (!fTransferAll)
        {
            if (!fTransferLockDowns && pItem->IsAttr(ATTR_LOCKEDDOWN)) // Skip this item if its locked down and we don't want to TRANSFER_LOCKDOWNS.
            {
                continue;
            }
            else if (!fTransferSecuredContainers && pItem->IsAttr(ATTR_SECURE))  // Skip this item if it's secured (container) and we don't want to TRANSFER_SECURED.
            {
                continue;
            }
            else if (pItem->IsType(IT_MULTI_ADDON) || pItem->IsType(IT_MULTI))  // If the item is a house Addon, redeed it.
            {
                if (fTransferAddons)    // Shall be transfered, but addons needs an special transfer code by redeeding.
                {
                    static_cast<CItemMulti*>(pItem)->Redeed(false, false);
                    Area.RestartSearch();	// we removed an item and this will mess the search loop, so restart to fix it.
                    continue;
                }
                else
                {
                    continue;
                }
            }
            else if (!Multi_IsPartOf(pItem))  // Items not linked to, or listed on, this multi.
            {
                continue;
            }
        }
        pItem->RemoveFromView();
        pCrate->ContentAdd(pItem);
    }
    if (pCrate->GetCount() == 0)
    {
        pCrate->Delete();
    }
}

void CItemMulti::TransferLockdownsToMovingCrate()
{
    ADDTOCALLSTACK("CItemMulti::TransferLockdownsToMovingCrate");
    if (_lLockDowns.empty())
    {
        return;
    }
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());
    if (!pCrate)
    {
        return;
    }
    for (size_t i = 0; i < _lLockDowns.size(); ++i)
    {
        CItem *pItem = _lLockDowns[i].ItemFind();
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

void CItemMulti::TransferSecuredToMovingCrate()
{
    ADDTOCALLSTACK("CItemMulti::TransferSecuredToMovingCrate");
    if (_lSecureContainers.empty())
    {
        return;
    }
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());
    if (!pCrate)
    {
        return;
    }
    for (size_t i = 0; i < _lSecureContainers.size(); ++i)
    {
        CItemContainer *pItem = static_cast<CItemContainer*>(_lSecureContainers[i].ItemFind());
        if (pItem)  // Move all valid items.
        {
            CScript event("events -t_house_secure");
            pItem->r_LoadVal(event);
            pCrate->ContentAdd(pItem);
            pItem->ClrAttr(ATTR_SECURE);
            pItem->m_uidLink.InitUID();
        }
    }
    _lLockDowns.clear();    // Clear the list, asume invalid items should be cleared too.
}

void CItemMulti::RedeedAddons()
{
    ADDTOCALLSTACK("CItemMulti::RedeedAddons");
    if (_lAddons.empty())
    {
        return;
    }
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());
    if (!pCrate)
    {
        return;
    }
    std::vector<CUID> vAddons = _lAddons;
    for (size_t i = 0; i < vAddons.size(); ++i)
    {
        CItemMulti *pAddon = static_cast<CItemMulti*>(vAddons[i].ItemFind());
        if (pAddon)  // Move all valid items.
        {
            pAddon->Redeed(false, false);
        }
    }
    vAddons.clear();
    _lAddons.clear();    // Clear the list, asume invalid items should be cleared too.
}

void CItemMulti::TransferMovingCrateToBank()
{
    ADDTOCALLSTACK("CItemMulti::TransferMovingCrateToBank");
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(false).ItemFind());
    CChar *pOwner = GetOwner().CharFind();
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

void CItemMulti::AddAddon(CUID uidAddon)
{
    ADDTOCALLSTACK("CItemMulti::AddAddon");
    if (!uidAddon.IsValidUID())
    {
        return;
    }
    CItemMulti *pAddon = static_cast<CItemMulti*>(uidAddon.ItemFind());
    if (!pAddon)
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (!pAddon->IsType(IT_MULTI_ADDON))
        {
            return;
        }
        if (GetAddonPos(uidAddon) >= 0)
        {
            return;
        }
    }
    _lAddons.emplace_back(uidAddon);
}

void CItemMulti::DelAddon(CUID uidAddon)
{
    ADDTOCALLSTACK("CItemMulti::DelAddon");
    for (size_t i = 0; i < _lAddons.size(); ++i)
    {
        if (_lAddons[i] == uidAddon)
        {
            _lAddons.erase(_lAddons.begin() + i);
            return;
        }
    }
}

int CItemMulti::GetAddonPos(CUID uidAddon)
{
    if (_lAddons.empty())
    {
        return -1;
    }

    for (size_t i = 0; i < _lAddons.size(); ++i)
    {
        if (_lAddons[i] == uidAddon)
        {
            return (int)i;
        }
    }
    return -1;
}

size_t CItemMulti::GetAddonCount()
{
    return _lAddons.size();
}

void CItemMulti::AddComp(CUID uidComponent)
{
    ADDTOCALLSTACK("CItemMulti::AddComp");
    if (!uidComponent.IsValidUID())
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetCompPos(uidComponent) >= 0)
        {
            return;
        }
    }
    CItem *pComp = uidComponent.ItemFind();
    if (pComp)
    {
        pComp->SetComponentOfMulti(GetUID());
    }
    _lComps.emplace_back(uidComponent);
}

void CItemMulti::DelComp(CUID uidComponent)
{
    ADDTOCALLSTACK("CItemMulti::DelComponent");
    for (size_t i = 0; i < _lComps.size(); ++i)
    {
        if (_lComps[i] == uidComponent)
        {
            _lComps.erase(_lComps.begin() + i);
            break;
        }
    }
    if (!uidComponent.IsValidUID()) // Doing this after the erase to force check the vector, just in case...
    {
        return;
    }
    CItem *pComp = uidComponent.ItemFind();
    if (pComp)
    {
        pComp->SetComponentOfMulti(UID_UNUSED);
    }
}

int CItemMulti::GetCompPos(CUID uidComponent)
{
    if (_lComps.empty())
    {
        return -1;
    }

    for (size_t i = 0; i < _lComps.size(); ++i)
    {
        if (_lComps[i] == uidComponent)
        {
            return (int)i;
        }
    }
    return -1;
}

size_t CItemMulti::GetCompCount()
{
    return _lComps.size();
}

void CItemMulti::RemoveAllComps()
{
    ADDTOCALLSTACK("CItemMulti::RemoveAllComps");
    if (_lComps.empty())
    {
        return;
    }
    std::vector<CUID> lCopy = _lComps;
    for (size_t i = 0; i < lCopy.size(); ++i)
    {
        CItem *pComp = lCopy[i].ItemFind();
        if (pComp)
        {
            pComp->Delete();
        }
    }
    _lComps.clear();
}

void CItemMulti::GenerateBaseComponents(bool & fNeedKey, dword &dwKeyCode)
{
    ADDTOCALLSTACK("CItemMulti::GenerateBaseComponents");
    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    ASSERT(pMultiDef);
    for (size_t i = 0; i < pMultiDef->m_Components.size(); i++)
    {
        const CItemBaseMulti::CMultiComponentItem &component = pMultiDef->m_Components.at(i);
        fNeedKey |= Multi_CreateComponent(component.m_id, component.m_dx, component.m_dy, component.m_dz, dwKeyCode);
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
    return (int16)(_lLockDowns.size() + _lSecureContainers.size());
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
    return (GetMaxStorage() - (GetMaxStorage() - (GetMaxStorage() * (uint16)GetLockdownsPercent()) / 100));
}

uint8 CItemMulti::GetLockdownsPercent()
{
    return _iLockdownsPercent;
}

void CItemMulti::SetLockdownsPercent(uint8 iPercent)
{
    _iLockdownsPercent = iPercent;
}

void CItemMulti::LockItem(CUID uidItem)
{
    ADDTOCALLSTACK("CItemMulti::LockItem");
    if (!uidItem.IsValidUID())
    {
        return;
    }
    CItem *pItem = uidItem.ItemFind();
    ASSERT(pItem);
    if (!g_Serv.IsLoading())
    {
        if (GetLockedItemPos(uidItem) >= 0)
        {
            return;
        }
        pItem->SetAttr(ATTR_LOCKEDDOWN);
        pItem->m_uidLink = GetUID();
        CScript event("events +t_house_lockdown");
        pItem->r_LoadVal(event);
    }
    pItem->SetLockDownOfMulti(uidItem);
    _lLockDowns.emplace_back(uidItem);
}

void CItemMulti::UnlockItem(CUID uidItem)
{
    ADDTOCALLSTACK("CItemMulti::UnlockItem");
    for (size_t i = 0; i < _lLockDowns.size(); ++i)
    {
        if (_lLockDowns[i] == uidItem)
        {
            _lLockDowns.erase(_lLockDowns.begin() + i);
            break;
        }
    }
    CItem *pItem = uidItem.ItemFind();
    if (!pItem)
    {
        return;
    }
    pItem->ClrAttr(ATTR_LOCKEDDOWN);
    pItem->m_uidLink.InitUID();
    CScript event("events -t_house_lockdown");
    pItem->r_LoadVal(event);
    pItem->SetLockDownOfMulti(UID_UNUSED);
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
    return _lLockDowns.size() + _lSecureContainers.size();
}

void CItemMulti::Secure(CUID uidContainer)
{
    ADDTOCALLSTACK("CItemMulti::Secure");
    if (!uidContainer.IsValidUID())
    {
        return;
    }
    CItemContainer *pContainer = static_cast<CItemContainer*>(uidContainer.ItemFind());
    ASSERT(pContainer);
    if (!g_Serv.IsLoading())
    {
        if (GetSecuredContainerPos(uidContainer) >= 0)
        {
            return;
        }
        pContainer->SetAttr(ATTR_SECURE);
        pContainer->m_uidLink = GetUID();
        CScript event("events +t_house_secure");
        pContainer->r_LoadVal(event);

    }
    pContainer->SetSecuredOfMulti(GetUID());
    _lSecureContainers.emplace_back(uidContainer);
}

void CItemMulti::Release(CUID uidContainer)
{
    ADDTOCALLSTACK("CItemMulti::Release");
    for (size_t i = 0; i < _lSecureContainers.size(); ++i)
    {
        if (_lSecureContainers[i] == uidContainer)
        {
            _lSecureContainers.erase(_lSecureContainers.begin() + i);
            break;
        }
    }
    CItemContainer *pContainer = static_cast<CItemContainer*>(uidContainer.ItemFind());
    if (!pContainer)
    {
        return;
    }
    pContainer->ClrAttr(ATTR_SECURE);
    pContainer->m_uidLink.InitUID();
    CScript event("events -t_house_secure");
    pContainer->r_LoadVal(event);
    pContainer->SetSecuredOfMulti(UID_UNUSED);
}

int CItemMulti::GetSecuredContainerPos(CUID uidContainer)
{
    ADDTOCALLSTACK("CItemMulti::GetSecuredContainerPos");
    if (_lSecureContainers.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < _lSecureContainers.size(); ++i)
    {
        if (_lSecureContainers[i] == uidContainer)
        {
            return (int)i;
        }
    }
    return -1;
}

int16 CItemMulti::GetSecuredItemsCount()
{
    ADDTOCALLSTACK("CItemMulti::GetSecuredItemsCount");
    size_t iCount = 0;
    for (size_t i = 0; i < _lSecureContainers.size(); ++i)
    {
        CItemContainer *pContainer = static_cast<CItemContainer*>(_lSecureContainers[i].ItemFind());
        if (pContainer)
        {
            iCount += pContainer->GetCount();
        }
    }
    return (int16)iCount;
}

size_t CItemMulti::GetSecuredContainersCount()
{
    return _lSecureContainers.size();
}

void CItemMulti::AddVendor(CUID uidVendor)
{
    ADDTOCALLSTACK("CItemMulti::AddVendor");
    if (!uidVendor.IsValidUID())
    {
        return;
    }
    if ((g_Serv.IsLoading() == false) && (GetHouseVendorPos(uidVendor) >= 0))
    {
        return;
    }
    CChar *pVendor = uidVendor.CharFind();
    if (!pVendor)
    {
        return;
    }
    RevokePrivs(uidVendor);
    pVendor->GetMultiStorage()->AddMulti(GetUID(), HP_VENDOR);
    _lVendors.emplace_back(uidVendor);
}

void CItemMulti::DelVendor(CUID uidVendor)
{
    ADDTOCALLSTACK("CItemMulti::DelVendor");
    for (size_t i = 0; i < _lVendors.size(); ++i)
    {
        if (_lVendors[i] == uidVendor)
        {
            CChar *pVendor = uidVendor.CharFind();
            ASSERT(pVendor);
            pVendor->GetMultiStorage()->DelMulti(GetUID());
            _lVendors.erase(_lVendors.begin() + i);
            break;
        }
    }
}

int CItemMulti::GetHouseVendorPos(CUID uidVendor)
{
    ADDTOCALLSTACK("CItemMulti::GetHouseVendorPos");
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
    SHR_ACCESS,
    SHR_ADDON,
    SHR_BAN,
    SHR_COMP,
    SHR_COOWNER,
    SHR_FRIEND,
    SHR_GUILD,
    SHR_LOCKDOWN,
    SHR_MOVINGCRATE,
    SHR_OWNER,
    SHR_REGION,
    SHR_SECURED,
    SHR_VENDOR,
    SHR_QTY
};

lpctstr const CItemMulti::sm_szRefKeys[SHR_QTY + 1] =
{
    "ACCESS",
    "ADDON",
    "BAN",
    "COMP",
    "COOWNER",
    "FRIEND",
    "GUILD",
    "LOCKDOWN",
    "MOVINGCRATE",
    "OWNER",
    "REGION",
    "SECURED",
    "VENDOR",
    nullptr
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
                CChar *pAccess = _lAccesses[i].CharFind();
                if (pAccess)
                {
                    pRef = pAccess;
                    return true;
                }
            }
            return false;
        }
        case SHR_ADDON:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lAddons.size() > i)
            {
                CItemMulti *pAddon = static_cast<CItemMulti*>(_lAddons[i].ItemFind());
                if (pAddon)
                {
                    pRef = pAddon;
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
                CChar *pBan = _lBans[i].CharFind();
                if (pBan)
                {
                    pRef = pBan;
                    return true;
                }
            }
            return false;
        }
        case SHR_COMP:
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
        }
        case SHR_COOWNER:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lCoowners.size() > i)
            {
                CChar *pCoowner = _lCoowners[i].CharFind();
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
                CChar *pFriend = _lFriends[i].CharFind();
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
                CItem *plockdown = _lLockDowns[i].ItemFind();
                if (plockdown)
                {
                    pRef = plockdown;
                    return true;
                }
            }
            return false;
        }
        case SHR_MOVINGCRATE:
        {
            SKIP_SEPARATORS(pszKey);
            pRef = static_cast<CItemContainer*>(GetMovingCrate(false).ItemFind());
            return true;
        }
        case SHR_OWNER:
        {
            SKIP_SEPARATORS(pszKey);
            pRef = _pOwner.CharFind();
            return true;
        }
        case SHR_GUILD:
        {
            SKIP_SEPARATORS(pszKey);
            pRef = static_cast<CItemStone*>(_pGuild.ItemFind());
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
        case SHR_SECURED:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lSecureContainers.size() > i)
            {
                CItem *pItem = _lSecureContainers[i].ItemFind();
                if (pItem)
                {
                    CItemContainer *pCont = static_cast<CItemContainer*>(pItem);
                    pRef = pCont;
                    return true;
                }
            }
            return false;
        }
        case SHR_VENDOR:
        {
            int i = Exp_GetVal(pszKey);
            SKIP_SEPARATORS(pszKey);
            if ((int)_lVendors.size() > i)
            {
                CChar *pVendor = _lVendors[i].CharFind();
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
    SHV_DELADDON,
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
    SHV_RELEASE,
    SHV_REMOVEALLCOMPS,
    SHV_REMOVEKEYS,
    SHV_UNLOCKITEM,
    SHV_QTY
};

lpctstr const CItemMulti::sm_szVerbKeys[SHV_QTY + 1] =
{
    "DELACCESS",
    "DELADDON",
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
    "RELEASE",
    "REMOVEALLCOMPS",
    "REMOVEKEYS",
    "UNLOCKITEM",
    nullptr
};

bool CItemMulti::r_Verb(CScript & s, CTextConsole * pSrc) // Execute command from script
{
    ADDTOCALLSTACK("CItemMulti::r_Verb");
    EXC_TRY("Verb");
    // Speaking in this multis region.
    // return: true = command for the multi.

    if (CMultiMovable::r_Verb(s, pSrc))
    {
        return true;
    }
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
                DelAccess(uidAccess);
            }
            break;
        }
        case SHV_DELADDON:
        {
            CUID uidAddon = s.GetArgDWVal();
            if (!uidAddon.IsValidUID())
            {
                _lAddons.clear();
            }
            else
            {
                DelAddon(uidAddon);
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
                DelBan(uidBan);
            }
            break;
        }
        case SHV_DELCOMPONENT:
        {
            CUID uidComp = s.GetArgDWVal();
            if (!uidComp.IsValidUID())
            {
                _lComps.clear();
            }
            else
            {
                DelComp(uidComp);
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
                DelCoowner(uidCoowner);
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
                DelFriend(uidFriend);
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
            CUID uidItem = (CUID)s.GetArgDWVal();
            if (!uidItem.IsValidUID())
            {
                _lLockDowns.clear();
            }
            else
            {
                ;
                UnlockItem(uidItem);
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
            Redeed(fShowMsg, fMoveToBank, pSrc->GetChar()->GetUID());
            break;
        }
        case SHV_RELEASE:
        {
            CUID uidRelease = s.GetArgDWVal();
            if (!uidRelease.IsValidUID())
            {
                _lAccesses.clear();
            }
            else
            {
                Release(uidRelease);
            }
            break;
        }
        case SHV_REMOVEKEYS:
        {
            CUID uidChar = s.GetArgDWVal();
            RemoveKeys(uidChar);
            break;
        }
        case SHV_REMOVEALLCOMPS:
        {
            RemoveAllComps();
            break;
        }
        case SHV_GENERATEBASECOMPONENTS:
        {
            bool fNeedKey = false;
            dword uid = UID_CLEAR;
            GenerateBaseComponents(fNeedKey, uid);
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
    SHL_ACCESSES,
    SHL_ADDACCESS,
    SHL_ADDADDON,
    SHL_ADDBAN,
    SHL_ADDCOMP,
    SHL_ADDCOOWNER,
    SHL_ADDFRIEND,
    SHL_ADDKEY,
    SHL_ADDONS,
    SHL_ADDVENDOR,
    SHL_BANS,
    SHL_BASESTORAGE,
    SHL_BASEVENDORS,
    SHL_COMPS,
    SHL_COOWNERS,
    SHL_CURRENTSTORAGE,
    SHL_FRIENDS,
    SHL_GETACCESSPOS,
    SHL_GETADDONPOS,
    SHL_GETBANPOS,
    SHL_GETCOMPPOS,
    SHL_GETCOOWNERPOS,
    SHL_GETFRIENDPOS,
    SHL_GETHOUSEVENDORPOS,
    SHL_GETLOCKEDITEMPOS,
    SHL_GETSECUREDCONTAINERPOS,
    SHL_GETSECUREDCONTAINERS,
    SHL_GETSECUREDITEMS,
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
    SHL_PRIV,
    SHL_REGION,
    SHL_REMOVEKEYS,
    SHL_SECURE,
    SHL_VENDORS,
    SHL_QTY
};

const lpctstr CItemMulti::sm_szLoadKeys[SHL_QTY + 1] =
{
    "ACCESSES",
    "ADDACCESS",
    "ADDADDON",
    "ADDBAN",
    "ADDCOMP",
    "ADDCOOWNER",
    "ADDFRIEND",
    "ADDKEY",
    "ADDONS",
    "ADDVENDOR",
    "BANS",
    "BASESTORAGE",
    "BASEVENDORS",
    "COMPS",
    "COOWNERS",
    "CURRENTSTORAGE",
    "FRIENDS",
    "GETACCESSPOS",
    "GETADDONPOS",
    "GETBANPOS",
    "GETCOMPPOS",
    "GETCOOWNERPOS",
    "GETFRIENDPOS",
    "GETHOUSEVENDORPOS",
    "GETLOCKEDITEMPOS",
    "GETSECUREDCONTAINERPOS",
    "GETSECUREDCONTAINERS",
    "GETSECUREDITEMS",
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
    "PRIV",
    "REGION",
    "REMOVEKEYS",
    "SECURE",
    "VENDORS",
    nullptr
};

void CItemMulti::r_Write(CScript & s)
{
    ADDTOCALLSTACK_INTENSIVE("CItemMulti::r_Write");
    CItem::r_Write(s);
    if (m_pRegion)
    {
        m_pRegion->r_WriteBody(s, "REGION.");
    }
    if (_pGuild)
    {
        s.WriteKeyHex("GUILD", _pGuild);
    }
    if (_pOwner)
    {
        s.WriteKeyHex("OWNER", _pOwner);
    }
    if (!_lCoowners.empty())
    {
        for (size_t i = 0; i < _lCoowners.size(); ++i)
        {
            CUID uid = _lCoowners[i];
            if (uid.IsValidUID())
            {
                s.WriteKeyHex("ADDCOOWNER", uid);
            }
        }
    }
    if (!_lFriends.empty())
    {
        for (size_t i = 0; i < _lFriends.size(); ++i)
        {
            CUID uid = _lFriends[i];
            if (uid)
            {
                s.WriteKeyHex("ADDFRIEND", uid);
            }
        }
    }
    if (!_lVendors.empty())
    {
        for (size_t i = 0; i < _lVendors.size(); ++i)
        {
            CUID uid = _lVendors[i];
            if (uid)
            {
                s.WriteKeyHex("ADDVENDOR", uid);
            }
        }
    }
    if (!_lLockDowns.empty())
    {
        for (size_t i = 0; i < _lLockDowns.size(); ++i)
        {
            CUID uid = _lLockDowns[i];
            if (uid)
            {
                s.WriteKeyHex("LOCKITEM", uid);
            }
        }
    }
    if (!_lComps.empty())
    {
        for (size_t i = 0; i < _lComps.size(); ++i)
        {
            CUID uid = _lComps[i];
            if (uid)
            {
                s.WriteKeyHex("ADDCOMP", uid);
            }
        }
    }
    if (!_lSecureContainers.empty())
    {
        for (size_t i = 0; i < _lSecureContainers.size(); ++i)
        {
            CUID uid = _lSecureContainers[i];
            if (uid)
            {
                s.WriteKeyHex("SECURE", uid);
            }
        }
    }
    if (!_lAddons.empty())
    {
        for (size_t i = 0; i < _lAddons.size(); ++i)
        {
            CUID uid = _lAddons[i];
            if (uid)
            {
                s.WriteKeyHex("ADDADDON", uid);
            }
        }
    }
    CUID uidCrate = GetMovingCrate(false);
    if (uidCrate.IsValidUID())
    {
        s.WriteKeyHex("MOVINGCRATE", uidCrate);
    }
}

bool CItemMulti::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CItemMulti::r_WriteVal");
    if (CMultiMovable::r_WriteVal(pszKey, sVal, pSrc))
    {
        return true;
    }
    int iCmd = FindTableHeadSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iCmd >= 0)
    {
        pszKey += strlen(sm_szLoadKeys[iCmd]);
    }
    SKIP_SEPARATORS(pszKey);
    switch (iCmd)
    {
        // House Permissions
        case SHL_PRIV:
            sVal.FormatU8Val((uint8)pSrc->GetChar()->GetMultiStorage()->GetPriv(GetUID()));
            break;
        case SHL_OWNER:
        {
            CChar *pOwner = GetOwner().CharFind();
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
            CItemStone *pGuild = static_cast<CItemStone*>(GetGuild().ItemFind());
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
            sVal.FormatBVal(IsOwner(uidOwner));
            break;
        }
        case SHL_GETCOOWNERPOS:
        {
            CUID uidCoowner = static_cast<CUID>(Exp_GetVal(pszKey));
            sVal.FormatVal(GetCoownerPos(uidCoowner));
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
            sVal.FormatVal(GetFriendPos(uidFriend));
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
            sVal.FormatVal(GetAccessPos(uidAccess));
            break;
        }
        case SHL_ADDONS:
        {
            sVal.FormatSTVal(GetAddonCount());
            break;
        }
        case SHL_GETADDONPOS:
        {
            CUID uidAddon = static_cast<CUID>(Exp_GetVal(pszKey));
            sVal.FormatVal(GetAddonPos(uidAddon));
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
            sVal.FormatVal(GetBanPos(uidBan));
            break;
        }

        // House General
        case SHL_HOUSETYPE:
        {
            sVal.FormatU8Val((uint8)_iHouseType);
            break;
        }
        case SHL_GETCOMPPOS:
        {
            CUID uidComp = static_cast<CUID>(Exp_GetVal(pszKey));
            sVal.FormatVal(GetCompPos(uidComp));
            break;
        }
        case SHL_MOVINGCRATE:
        {
            bool fCreate = false;

            if (strlen(pszKey) <= 2) // check for <MovingCrate 0/1> (whitespace + number = 2 len)
            {
                fCreate = Exp_GetVal(pszKey);
            }
            else if (!IsStrEmpty(pszKey) && _pMovingCrate.IsValidUID())    // If there's an already existing Moving Crate and args len is greater than 1, it should have a keyword to send to the crate.
            {
                CItemContainer *pCrate = static_cast<CItemContainer*>(_pMovingCrate.ItemFind());
                ASSERT(pCrate); // Removed crates should init _pMovingCrate's uid?
                return pCrate->r_WriteVal(pszKey, sVal, pSrc);
            }
            if (fCreate)
            {
                GetMovingCrate(true).ItemFind();
            }
            if (_pMovingCrate.IsValidUID())
            {
                sVal.FormatDWVal(_pMovingCrate);
            }
            else
            {
                sVal.FormatDWVal(0);
            }
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
                pKey = GenerateKey(uidOwner, fDupeOnBank);
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
        case SHL_COMPS:
        {
            sVal.FormatSTVal(GetCompCount());
            break;
        }
        case SHL_LOCKDOWNS:
        {
            sVal.FormatSTVal(GetLockdownCount());
            break;
        }
        case SHL_GETSECUREDCONTAINERPOS:
        {
            CUID uidCont = (CUID)Exp_GetDWVal(pszKey);
            if (uidCont.IsValidUID())
            {
                sVal.FormatVal(GetSecuredContainerPos(uidCont));
                break;
            }
            return false;
        }
        case SHL_GETSECUREDCONTAINERS:
        {
            sVal.FormatSTVal(GetSecuredContainersCount());
            break;
        }
        case SHL_GETSECUREDITEMS:
        {
            sVal.Format16Val(GetSecuredItemsCount());
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
            return CItem::r_WriteVal(pszKey, sVal, pSrc);
    }
    return true;
}

bool CItemMulti::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CItemMulti::r_LoadVal");
    EXC_TRY("LoadVal");

    if (CMultiMovable::r_LoadVal(s))
    {
        return true;
    }
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
            CUID dwCrate = s.GetArgDWVal();
            if (dwCrate.IsValidUID())
            {
                if ((int)dwCrate == 1) // fix for 'movingcrate 1'
                {
                    GetMovingCrate(true);
                }
                else
                {
                    SetMovingCrate(static_cast<CUID>(dwCrate));
                }
            }
            else
            {
                SetMovingCrate(UID_UNUSED);
            }
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
                GenerateKey(uidOwner, fDupeOnBank);
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
                CChar *pOwner = GetOwner().CharFind();
                if (pOwner)
                {
                    return pOwner->r_LoadVal(s);
                }
                return false;
            }
            CUID uid = (CUID)s.GetArgDWVal();
            if (!uid.IsValidUID())
            {
                SetOwner(UID_UNUSED);
            }
            else
            {
                SetOwner(uid);
            }
            break;
        }
        case SHL_GUILD:
        {
            lpctstr pszKey = s.GetKey();
            pszKey += 5;
            if (*pszKey == '.')
            {
                CItemStone *pGuild = static_cast<CItemStone*>(GetGuild().ItemFind());
                if (pGuild)
                {
                    return pGuild->r_LoadVal(s);
                }
                return false;
            }
            CUID uid = (CUID)s.GetArgDWVal();
            if (!uid.IsValidUID())
            {
                SetGuild(UID_UNUSED);
                break;
            }
            CItem *pItem = uid.ItemFind();
            if (pItem)
            {
                if (pItem->IsType(IT_STONE_GUILD))
                {
                    SetGuild(uid);
                    return true;
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
            AddCoowner(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDFRIEND:
        {
            AddFriend(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDBAN:
        {
            AddBan(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDACCESS:
        {
            AddAccess(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDADDON:
        {
            AddAddon(static_cast<CUID>(s.GetArgDWVal()));
            break;
        }

        // House Storage
        case SHL_ADDCOMP:
        {
            AddComp(((CUID)s.GetArgDWVal()));
            break;
        }
        case SHL_ADDVENDOR:
        {
            AddVendor(((CUID)s.GetArgDWVal()));
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
            CUID uidLock = (CUID)s.GetArgDWVal();
            if (uidLock.IsValidUID())
            {
                LockItem(uidLock);
            }
            break;
        }
        case SHL_SECURE:
        {
            CUID uidSecured = (CUID)s.GetArgDWVal();
            CItem *pItem = uidSecured.ItemFind();
            if (pItem && (pItem->IsType(IT_CONTAINER) || pItem->IsType(IT_CONTAINER_LOCKED)))
            {
                CItemContainer *pSecured = dynamic_cast<CItemContainer*>(pItem); //ensure it's a container.
                if (pSecured)
                {
                    Secure(uidSecured);
                    break;
                }
            }
            return false;
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
        if (!pChar->GetMultiStorage()->CanAddShip(pChar->GetUID(), pMultiDef->_iMultiCount))
        {
            return nullptr;
        }
    }
    else if (pItemDef->IsType(IT_MULTI) || pItemDef->IsType(IT_MULTI_CUSTOM))
    {
        if (!pChar->GetMultiStorage()->CanAddHouse(pChar->GetUID(), pMultiDef->_iMultiCount))
        {
            return nullptr;
        }
    }

    /*
    * There is a small difference in the coordinates where the mouse is in the screen and the ones received,
    * let's remove that difference.
    * Note: this fix is added here, before GM check, because otherwise they will place houses on wrong position.
    */
    if (CItemBase::IsID_Multi(pItemDef->GetID()) || pItemDef->IsType(IT_MULTI_ADDON))
    {
        pt.m_y -= (short)(pMultiDef->m_rect.m_bottom - 1);
    }

    if (!pChar->IsPriv(PRIV_GM))
    {
        if ((pMultiDef != nullptr) && !(pDeed->IsAttr(ATTR_MAGIC)))
        {
            CRect rect;
            if (pMultiDef)
            {
                rect = pMultiDef->m_rect; // Create a rect equal to the multi's one.
            }
            else
            {
                rect.OffsetRect(1, 1);  // Guildstones pass through here, also other non multi items placed from deeds.
            }
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
    CScript eComponent("+t_house_component");
    pComponent->m_OEvents.r_LoadVal(eComponent, RES_EVENTS);

    switch (pComponent->GetType())
    {
        case IT_DOOR:
        {
            CScript event("events +t_house_door");
            pComponent->r_LoadVal(event);
            pComponent->SetType(IT_DOOR_LOCKED);
            break;
        }
        case IT_CONTAINER:
        {
            CScript event("events +t_house_container");
            pComponent->r_LoadVal(event);
            pComponent->SetType(IT_CONTAINER_LOCKED);
            break;
        }
        case IT_SHIP_SIDE:
        {
            pComponent->SetType(IT_SHIP_SIDE_LOCKED);
            break;
        }
        case IT_SHIP_HOLD:
        {
            pComponent->SetType(IT_SHIP_HOLD_LOCK);
            break;
        }
        case IT_TELEPAD:
        {
            CScript event("events +t_house_telepad");
            pComponent->r_LoadVal(event);
        }
        default:
            break;
    }

    if (pComponent->GetHue() == HUE_DEFAULT)
    {
        pComponent->SetHue(GetHue());
    }
}

CMultiStorage::CMultiStorage(CUID uidSrc)
{
    _uidSrc = uidSrc;
}

CMultiStorage::~CMultiStorage()
{
    for (std::map<CUID, HOUSE_PRIV>::iterator it = _lHouses.begin(); it != _lHouses.end(); ++it)
    {
        CUID uid = (*it).first;
        CItemMulti *pMulti = static_cast<CItemMulti*>(uid.ItemFind());
        if (_uidSrc.IsChar())
        {
            pMulti->RevokePrivs(_uidSrc);
        }
        else if (_uidSrc.IsItem())   //Only guild/town stones
        {
            pMulti->SetGuild(UID_UNUSED);
        }
    }
    for (std::map<CUID, HOUSE_PRIV>::iterator it = _lShips.begin(); it != _lShips.end(); ++it)
    {
        CUID uid = (*it).first;
        CItemShip *pShip = static_cast<CItemShip*>(uid.ItemFind());
        if (_uidSrc.IsChar())
        {
            pShip->RevokePrivs(_uidSrc);
        }
        else if (_uidSrc.IsItem())   //Only guild/town stones
        {
            pShip->SetGuild(UID_UNUSED);
        }
    }
    _uidSrc.InitUID();
}

void CMultiStorage::AddMulti(CUID uidMulti, HOUSE_PRIV ePriv)
{
    if (!uidMulti.IsValidUID())
    {
        return;
    }
    CItemMulti *pMulti = static_cast<CItemMulti*>(uidMulti.ItemFind());
    if (!pMulti)
    {
        return;
    }
    CObjBase *pSrc = _uidSrc.ObjFind();
    bool isChar = pSrc && pSrc->IsChar();
    if (pMulti->IsType(IT_SHIP))
    {
        if (_lShips.empty() && isChar)
        {
            CScript event("events +e_ship_priv");
            pSrc->r_LoadVal(event);
        }
        AddShip(uidMulti, ePriv);
    }
    else
    {
        if (_lHouses.empty() && isChar)
        {
            CScript event("events +e_house_priv");
            pSrc->r_LoadVal(event);
        }
        AddHouse(uidMulti, ePriv);
    }
}

void CMultiStorage::DelMulti(CUID uidMulti)
{
    CItemMulti *pMulti = static_cast<CItemMulti*>(uidMulti.ItemFind());
    CObjBase *pSrc = _uidSrc.ObjFind();
    if (pMulti->IsType(IT_SHIP))
    {
        DelShip(uidMulti);
        if (_lShips.empty() && pSrc)
        {
            CScript event("events -e_ship_priv");
            pSrc->r_LoadVal(event);
        }
    }
    else
    {
        DelHouse(uidMulti);
        if (_lHouses.empty() && pSrc)
        {
            CScript event("events -e_house_priv");
            pSrc->r_LoadVal(event);
        }
    }
    pMulti->RevokePrivs(_uidSrc);
}

void CMultiStorage::AddHouse(CUID uidHouse, HOUSE_PRIV ePriv)
{
    ADDTOCALLSTACK("CMultiStorage::AddHouse");
    if (!uidHouse.IsValidUID())
    {
        return;
    }
    if (GetHousePos(uidHouse) >= 0)
    {
        return;
    }
    CItemMulti *pMulti = static_cast<CItemMulti*>(uidHouse.ItemFind());
    _iHousesTotal += pMulti->GetMultiCount();
    _lHouses[uidHouse] = ePriv;
}

void CMultiStorage::DelHouse(CUID uidHouse)
{
    ADDTOCALLSTACK("CMultiStorage::DelHouse");
    if (_lHouses.empty())
    {
        return;
    }

    if (_lHouses.find(uidHouse) != _lHouses.end())
    {
        CItemMulti *pMulti = static_cast<CItemMulti*>(uidHouse.ItemFind());
        _iHousesTotal -= pMulti->GetMultiCount();
        _lHouses.erase(uidHouse);
        return;
    }
}

HOUSE_PRIV CMultiStorage::GetPriv(CUID uidMulti)
{
    ADDTOCALLSTACK("CMultiStorage::GetPrivMulti");
    CItemMulti *pMulti = static_cast<CItemMulti*>(uidMulti.ItemFind());
    if (pMulti->IsType(IT_MULTI))
    {
        return _lHouses[uidMulti];
    }
    else if (pMulti->IsType(IT_SHIP))
    {
        return _lShips[uidMulti];
    }
    return HP_NONE;
}

bool CMultiStorage::CanAddHouse(CUID uidChar, int16 iHouseCount)
{
    ADDTOCALLSTACK("CMultiStorage::CanAddHouse");
    if (uidChar.IsItem())
    {
        return true;    // Guilds can have unlimited houses atm.
    }
    CChar *pChar = uidChar.CharFind();
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

int16 CMultiStorage::GetHousePos(CUID uidHouse)
{
    ADDTOCALLSTACK("CMultiStorage::GetHousePos");
    if (_lHouses.empty())
    {
        return -1;
    }
    int16 i = 0;
    for (std::map<CUID, HOUSE_PRIV>::iterator it = _lHouses.begin(); it != _lHouses.end(); ++it)
    {
        if ((*it).first == uidHouse)
        {
            return i;
        }
        ++i;
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

CUID CMultiStorage::GetHouseAt(int16 iPos)
{
    std::map<CUID, HOUSE_PRIV>::iterator it = _lHouses.begin();
    std::advance(it, iPos);
    return (*it).first;
}

void CMultiStorage::ClearHouses()
{
    _lHouses.clear();
}

void CMultiStorage::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CMultiStorage::r_Write");
    UNREFERENCED_PARAMETER(s);
}

void CMultiStorage::AddShip(CUID uidShip, HOUSE_PRIV ePriv)
{
    ADDTOCALLSTACK("CMultiStorage::AddShip");
    if (!uidShip.IsValidUID())
    {
        return;
    }
    if (GetShipPos(uidShip) >= 0)
    {
        return;
    }
    CItemShip *pShip = static_cast<CItemShip*>(uidShip.ItemFind());
    if (!pShip)
    {
        return;
    }
    _iShipsTotal += pShip->GetMultiCount();
    _lShips[uidShip] = ePriv;
}

void CMultiStorage::DelShip(CUID uidShip)
{
    ADDTOCALLSTACK("CMultiStorage::DelShip");
    if (_lShips.empty())
    {
        return;
    }

    if (_lShips.find(uidShip) != _lShips.end())
    {
        CItemShip *pShip = static_cast<CItemShip*>(uidShip.ItemFind());
        if (pShip)
        {
            _iShipsTotal -= pShip->GetMultiCount();
        }
        _lShips.erase(uidShip);
        return;
    }
}

bool CMultiStorage::CanAddShip(CUID uidChar, int16 iShipCount)
{
    ADDTOCALLSTACK("CMultiStorage::CanAddShip");
    if (uidChar.IsItem())
    {
        return true; // Guilds can have unlimited ships atm.
    }
    CChar *pChar = uidChar.CharFind();
    if (!pChar)
    {
        return false;
    }
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

int16 CMultiStorage::GetShipPos(CUID uidShip)
{
    if (_lShips.empty())
    {
        return -1;
    }
    int16 i = 0;
    for (std::map<CUID, HOUSE_PRIV>::iterator it = _lShips.begin(); it != _lShips.end(); ++it)
    {
        if ((*it).first == uidShip)
        {
            return i;
        }
        ++i;
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

CUID CMultiStorage::GetShipAt(int16 iPos)
{
    std::map<CUID, HOUSE_PRIV>::iterator it = _lShips.begin();
    std::advance(it, iPos);
    return (*it).first;
}

void CMultiStorage::ClearShips()
{
    _lShips.clear();
}
