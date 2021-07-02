#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../../common/sphereproto.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"
#include "../CServer.h"
#include "../CWorldMap.h"
#include "../triggers.h"
#include "CItemMulti.h"
#include "CItemShip.h"
#include "CItemContainer.h"
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////

CItemMulti::CItemMulti(ITEMID_TYPE id, CItemBase * pItemDef, bool fTurnable) :    // CItemBaseMulti
    CTimedObject(PROFILE_MULTIS),
    CItem(id, pItemDef),
    CCMultiMovable(fTurnable)
{
    CItemBaseMulti * pItemBase = static_cast<CItemBaseMulti*>(Base_GetDef());
    _shipSpeed.period = pItemBase->_shipSpeed.period;
    _shipSpeed.tiles = pItemBase->_shipSpeed.tiles;
    _eSpeedMode = pItemBase->m_SpeedMode;

    m_pRegion = nullptr;

    _iHouseType = HOUSE_PRIVATE;
    _iMultiCount = pItemBase->_iMultiCount;

    _uiBaseStorage = pItemBase->_iBaseStorage;
    _uiBaseVendors = pItemBase->_iBaseVendors;
    _uiLockdownsPercent = pItemBase->_iLockdownsPercent;
    _uiIncreasedStorage = 0;
    _fIsAddon = false;
}

CItemMulti::~CItemMulti()
{
    EXC_TRY("Cleanup in destructor");

    ADDTOCALLSTACK("CItemMulti::~CItemMulti");

    if (!m_pRegion)
    {
        return;
    }

    if (_uidGuild.IsValidUID())
    {
        SetGuild(CUID());
    }
    if (_uidOwner.IsValidUID())
    {
        SetOwner(CUID());
    }

    RemoveAllKeys();
    if (!_lCoowners.empty())
    {
		for (const CUID& charUID : _lCoowners)
		{
			DeleteCoowner(charUID, false);
		}
    }
    if (!_lFriends.empty())
    {
		for (const CUID& charUID : _lFriends)
		{
			DeleteFriend(charUID, false);
		}
    }
    if (!_lVendors.empty())
    {
		for (const CUID& charUID : _lVendors)
		{
			DeleteVendor(charUID, false);
		}
    }

    if (!_lLockDowns.empty())
    {
        UnlockAllItems();
    }
    if (!_lComps.empty())
    {
        for (const CUID& itemUID : _lComps)
        {
            DeleteComponent(itemUID, false);
        }
        _lComps.clear();
    }
    if (!_lSecureContainers.empty())
    {
        for (const CUID& itemUID : _lSecureContainers)
        {
            Release(itemUID, false);
        }
        _lSecureContainers.clear();
    }
    if (!_lAddons.empty())
    {
        /*
        for (const CUID& itemUID : _lAddons)
        {
            CItemMulti *pItem = static_cast<CItemMulti*>(itemUID.ItemFind());
            if (pItem)
            {
                DeleteAddon(itemUID);
            }
        }
        */
        _lAddons.clear();
    }

    CItemContainer *pMovingCrate = static_cast<CItemContainer*>(GetMovingCrate(false).ItemFind());
    if (pMovingCrate)
    {
        SetMovingCrate(CUID());
    }

    MultiUnRealizeRegion();    // unrealize before removed from ground.

    // Must remove early because virtuals will fail in child destructor.
    // Attempt to remove all the accessory junk.
    // NOTE: assume we have already been removed from Top Level
    DeletePrepare();
    delete m_pRegion;

    EXC_CATCH;
}

bool CItemMulti::Delete(bool fForce)
{
    RemoveAllComponents();
    return CObjBase::Delete(fForce);
}

const CItemBaseMulti * CItemMulti::Multi_GetDef() const noexcept
{
    return static_cast <const CItemBaseMulti *>(Base_GetDef());
}

CRegion * CItemMulti::GetRegion() const noexcept
{
    return m_pRegion;
}

int CItemMulti::GetSideDistanceFromCenter(DIR_TYPE dir) const
{
    ADDTOCALLSTACK("CItemMulti::GetSideDistanceFromCenter");
    const CItemBaseMulti* pMultiDef = Multi_GetDef();
    ASSERT(pMultiDef);
    return pMultiDef->GetDistanceDir(dir);
}

int CItemMulti::Multi_GetDistanceMax() const
{
    ADDTOCALLSTACK("CItemMulti::Multi_GetDistanceMax");
    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    ASSERT(pMultiDef);
    return pMultiDef->GetDistanceMax();
}

const CItemBaseMulti * CItemMulti::Multi_GetDefByID(ITEMID_TYPE id) // static
{
    ADDTOCALLSTACK("CItemMulti::Multi_GetDefByID");
    const CItemBase * pItemBase = CItemBase::FindItemBase(id);
    return dynamic_cast <const CItemBaseMulti *>(pItemBase);
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
        g_Log.EventError("Bad Multi type 0%x, uid=0%x.\n", GetID(), (dword)GetUID());
        return false;
    }

    if (m_pRegion == nullptr)
    {
        const CResourceID rid(GetUID().GetPrivateUID(), 0);
        m_pRegion = new CRegionWorld(rid);
    }

    // Get Background region.
    const CPointMap& pt = GetTopPoint();
    const CRegionWorld * pRegionBack = dynamic_cast <CRegionWorld*> (pt.GetRegion(REGION_TYPE_AREA));
    if (!pRegionBack)
    {
        g_Log.EventError("Can't realize multi region at invalid P=%hd,%hd,%hhd,%hhu. Multi uid=0%x.\n", pt.m_x, pt.m_y, pt.m_z, pt.m_map, (dword)GetUID());
        return false;
    }
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
    snprintf(pszTemp, SCRIPT_MAX_LINE_LEN, "%s (%s)", pRegionBack->GetName(), GetName());
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
    CWorldSearch Area(m_pRegion->m_pt, Multi_GetDistanceMax());
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

    CPointMap pt(GetTopPoint());
    pt.m_x += dx;
    pt.m_y += dy;
    pt.m_z += dz;

    bool fNeedKey = false;

    switch (pItem->GetType())
    {
        case IT_SIGN_GUMP:
        case IT_SHIP_TILLER:
            pItem->m_uidLink = GetUID();
            FALLTHROUGH;
        case IT_KEY:    // it will get locked down with the house ?
            pItem->m_itKey.m_UIDLock.SetPrivateUID(dwKeyCode);    // Set the key id for the key/sign.
            m_uidLink.SetPrivateUID(pItem->GetUID());
            // Do not break, those need fNeedKey set to true.
            FALLTHROUGH;
        case IT_DOOR:
        case IT_CONTAINER:
            fNeedKey = true;
            break;
        default:
            break;
    }

    pItem->SetAttr(ATTR_MOVE_NEVER | (m_Attr&(ATTR_MAGIC | ATTR_INVIS)));
    pItem->m_uidLink = GetUID();    // lock it down with the structure.

    if (pItem->IsTypeLockable() || pItem->IsTypeLocked())
    {
        pItem->m_itContainer.m_UIDLock.SetPrivateUID(dwKeyCode);    // Set the key id for the door/key/sign.
        pItem->m_itContainer.m_dwLockComplexity = 10000;    // never pickable.
    }

    pItem->r_ExecSingle("EVENTS +ei_house_component");
    pItem->MoveToUpdate(pt);
    OnComponentCreate(pItem, false);    // TODO: how do i know if that's an addon?
    AddComponent(pItem->GetUID());
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
    GenerateBaseComponents(&fNeedKey, dwKeyCode);

    if (IsType(IT_MULTI_ADDON))   // Addons doesn't require keys and are not added to any list.
    {
        return;
    }

    Multi_GetSign();    // set the m_uidLink

    if (pChar)
    {
        SetOwner(pChar->GetUID());

        if (fNeedKey)
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
    pChar->r_Call("f_multi_setup", pChar, nullptr, nullptr, nullptr);
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
    const CUID uidItem = pItem->GetUID();
    if (GetLockedItemIndex(uidItem) != -1)
    {
        return true;
    }
    if (GetSecuredContainerIndex(uidItem) != -1)
    {
        return true;
    }
    if (GetAddonIndex(uidItem) != -1)
    {
        return true;
    }
    if (GetComponentIndex(uidItem) != -1)
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
        return _lComps[(size_t)iComp].ItemFind();
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

    CWorldSearch Area(GetTopPoint(), Multi_GetDistanceMax());
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

bool CItemMulti::_OnTick()
{
    if (!CCMultiMovable::_OnTick())
    {
        return CItem::_OnTick();
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
            pMulti->DeleteAddon(GetUID());
        }
        m_pRegion = nullptr;
    }
    else
    {
        ASSERT(m_pRegion);
        m_pRegion->UnRealizeRegion();
    }
}

bool CItemMulti::MoveTo(const CPointMap& pt, bool fForceFix) // Put item on the ground here.
{
    ADDTOCALLSTACK("CItemMulti::MoveTo");
    // Move this item to it's point in the world. (ground/top level)
    if (!CItem::MoveTo(pt, fForceFix))
    {
        return false;
    }

    // Multis need special region info to track when u are inside them.
    // Add new region info.
    if (IsType(IT_MULTI_ADDON)) // Addons doesn't have region, don't try to realize it.
    {
        const CPointMap& ptTop = GetTopPoint();
        CRegionWorld *pRegion = dynamic_cast<CRegionWorld*>(ptTop.GetRegion(REGION_TYPE_HOUSE));
        if (!pRegion)
        {
            pRegion = dynamic_cast<CRegionWorld*>(ptTop.GetRegion(REGION_TYPE_AREA));
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
        if (m_pRegion)
        {
            m_pRegion->UnRealizeRegion();
        }
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
    TALKMODE_TYPE mode = TALKMODE_SAY;

    for (size_t i = 0; i < pMultiDef->m_Speech.size(); ++i)
    {
        CResourceLink * pLink = pMultiDef->m_Speech[i].GetRef();
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

void CItemMulti::RevokePrivs(const CUID& uidSrc)
{
    ADDTOCALLSTACK("CItemMulti::RevokePrivs");
    if (!uidSrc.IsValidUID())
    {
        return;
    }
    if (IsOwner(uidSrc))
    {
        SetOwner(CUID());
    }
    else if (GetCoownerIndex(uidSrc) >= 0)
    {
        DeleteCoowner(uidSrc, true);
    }
    else if (GetFriendIndex(uidSrc) >= 0)
    {
        DeleteFriend(uidSrc, true);
    }
    else if (GetBanIndex(uidSrc) >= 0)
    {
        DeleteBan(uidSrc, true);
    }
    else if (GetAccessIndex(uidSrc) >= 0)
    {
        DeleteAccess(uidSrc, true);
    }
    else if (GetVendorIndex(uidSrc) >= 0)
    {
        DeleteVendor(uidSrc, true);
    }
}

void CItemMulti::SetOwner(const CUID& uidOwner)
{
    ADDTOCALLSTACK("CItemMulti::SetOwner");
    CChar *pOldOwner = _uidOwner.CharFind();
    _uidOwner.InitUID();
    if (pOldOwner)  // Old Owner may not exist, was removed?
    {
        if (!pOldOwner->m_pPlayer)
        {
            g_Log.EventWarn("Multi (UID 0%x) owned by a NPC (UID 0%x)?\n", (dword)GetUID(), (dword)pOldOwner->GetUID());
        }
        else
        {
            CMultiStorage* pMultiStorage = pOldOwner->m_pPlayer->GetMultiStorage();
            if (pMultiStorage)
            {
                pMultiStorage->DelMulti(GetUID());
            }
        }
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
        if (!pOwner->m_pPlayer)
        {
            g_Log.EventWarn("Multi (UID 0%x): trying to set owner to a non-playing character (UID 0%x)?\n", (dword)GetUID(), (dword)uidOwner);
        }
        else
        {
            CMultiStorage* pMultiStorage = pOwner->m_pPlayer->GetMultiStorage();
            if (pMultiStorage)
            {
                pMultiStorage->AddMulti(GetUID(), HP_OWNER);
            }
        }
    }
    _uidOwner = uidOwner;
}

bool CItemMulti::IsOwner(const CUID& uidTarget) const
{
    return (_uidOwner == uidTarget);
}

CUID CItemMulti::GetOwner() const
{
    if (_uidOwner.IsValidUID())
    {
        return _uidOwner;
    }
    return {};
}

void CItemMulti::SetGuild(const CUID& uidGuild)
{
    ADDTOCALLSTACK("CItemMulti::SetGuild");
    CItemStone *pGuildStone = static_cast<CItemStone*>(GetGuildStone().ItemFind());
    _uidGuild.InitUID();
    if (pGuildStone)  // Old Guild may not exist, was it removed...?
    {
        CMultiStorage* pMultiStorage = pGuildStone->GetMultiStorage();
        if (pMultiStorage)
        {
            pMultiStorage->DelMulti(GetUID());   // ... if not, unlink it from this multi.
        }
    }
    if (!uidGuild.IsValidUID())  // Just clearing it
    {
        return;
    }
    _uidGuild = uidGuild;   // Set the Guild* to a new guildstone.
    pGuildStone = static_cast<CItemStone*>(uidGuild.ItemFind());
    CMultiStorage* pMultiStorage = pGuildStone->GetMultiStorage();
    ASSERT(pMultiStorage);
    pMultiStorage->AddMulti(GetUID(), HP_GUILD);
}

bool CItemMulti::IsGuild(const CUID& uidTarget) const
{
    return (_uidGuild == uidTarget);
}

CUID CItemMulti::GetGuildStone() const
{
    if (_uidGuild.IsValidUID())
    {
        return _uidGuild;
    }
    return {};
}

void CItemMulti::AddCoowner(const CUID& uidCoowner)
{
    ADDTOCALLSTACK("CItemMulti::AddCoowner");
    if (!uidCoowner.IsValidUID())
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetCoownerIndex(uidCoowner) >= 0)
        {
            return;
        }
    }
    CChar *pCoowner = uidCoowner.CharFind();
    if (!pCoowner || !pCoowner->m_pPlayer)
    {
        return;
    }
    RevokePrivs(uidCoowner);
    CMultiStorage* pMultiStorage = pCoowner->m_pPlayer->GetMultiStorage();
    ASSERT(pMultiStorage);
    pMultiStorage->AddMulti(GetUID(), HP_COOWNER);
    _lCoowners.emplace_back(uidCoowner);
}

void CItemMulti::DeleteCoowner(const CUID& uidCoowner, bool fRemoveFromList)
{
    ADDTOCALLSTACK("CItemMulti::DeleteCoowner");
    for (size_t i = 0; i < _lCoowners.size(); ++i)
    {
        const CUID& uid = _lCoowners[i];
        if (uid == uidCoowner)
        {
            if (fRemoveFromList)
            {
                _lCoowners.erase(_lCoowners.begin() + i);
            }

            CChar *pCoowner = uidCoowner.CharFind();
            if (!pCoowner || !pCoowner->m_pPlayer)
            {
                continue;
            }

            CMultiStorage* pMultiStorage = pCoowner->m_pPlayer->GetMultiStorage();
            if (pMultiStorage)
            {
                pMultiStorage->DelMulti(GetUID());
            }
            RemoveKeys(uid);
            return;
        }
    }
}

size_t CItemMulti::GetCoownerCount() const
{
    return _lCoowners.size();
}

int CItemMulti::GetCoownerIndex(const CUID& uidTarget) const
{
    ADDTOCALLSTACK("CItemMulti::GetCoownerIndex");
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

void CItemMulti::AddFriend(const CUID& uidFriend)
{
    ADDTOCALLSTACK("CItemMulti::AddFriend");
    if (!uidFriend.IsValidUID())
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetFriendIndex(uidFriend) >= 0)
        {
            return;
        }
    }
    CChar *pFriend = uidFriend.CharFind();
    if (!pFriend || !pFriend->m_pPlayer)
    {
        return;
    }
    RevokePrivs(uidFriend);
    CMultiStorage* pMultiStorage = pFriend->m_pPlayer->GetMultiStorage();
    ASSERT(pMultiStorage);
    pMultiStorage->AddMulti(GetUID(), HP_FRIEND);
    _lFriends.emplace_back(uidFriend);
}

void CItemMulti::DeleteFriend(const CUID& uidFriend, bool fRemoveFromList)
{
    ADDTOCALLSTACK("CItemMulti::DeleteFriend");
    for (size_t i = 0; i < _lFriends.size(); ++i)
    {
        if (_lFriends[i] == uidFriend)
        {
            if (fRemoveFromList)
            {
                _lFriends.erase(_lFriends.begin() + i);
            }

            CChar *pFriend = uidFriend.CharFind();
            if (!pFriend || !pFriend->m_pPlayer)
            {
                continue;
            }

            CMultiStorage* pMultiStorage = pFriend->m_pPlayer->GetMultiStorage();
            if (pMultiStorage)
            {
                pMultiStorage->DelMulti(GetUID());
            }
            RemoveKeys(uidFriend);
            return;
        }
    }
}

size_t CItemMulti::GetFriendCount() const
{
    return _lFriends.size();
}

int CItemMulti::GetFriendIndex(const CUID& uidFriend) const
{
    ADDTOCALLSTACK("CItemMulti::GetFriendIndex");
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

void CItemMulti::AddBan(const CUID& uidBan)
{
    ADDTOCALLSTACK("CItemMulti::AddBan");
    if (!uidBan.IsValidUID())
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetBanIndex(uidBan) >= 0)
        {
            return;
        }
    }
    CChar *pBan = uidBan.CharFind();
    if (!pBan || !pBan->m_pPlayer)
    {
        return;
    }
    RevokePrivs(uidBan);
    CMultiStorage* pMultiStorage = pBan->m_pPlayer->GetMultiStorage();
    ASSERT(pMultiStorage);
    pMultiStorage->AddMulti(GetUID(), HP_BAN);
    _lBans.emplace_back(uidBan);
}

void CItemMulti::DeleteBan(const CUID& uidBan, bool fRemoveFromList)
{
    ADDTOCALLSTACK("CItemMulti::DeleteBan");
    for (size_t i = 0; i < _lBans.size(); ++i)
    {
        if (_lBans[i] == uidBan)
        {
            if (fRemoveFromList)
            {
                _lBans.erase(_lBans.begin() + i);
            }

            CChar *pBan = uidBan.CharFind();
            if (pBan && pBan->m_pPlayer)
            {
                CMultiStorage* pMultiStorage = pBan->m_pPlayer->GetMultiStorage();
                if (pMultiStorage)
                {
                    pMultiStorage->DelMulti(GetUID());
                }
            }
            return;
        }
    }
}

size_t CItemMulti::GetBanCount() const
{
    return _lBans.size();
}

int CItemMulti::GetBanIndex(const CUID& uidBan) const
{
    ADDTOCALLSTACK("CItemMulti::GetBanIndex");
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

void CItemMulti::AddAccess(const CUID& uidAccess)
{
    ADDTOCALLSTACK("CItemMulti::AddAccess");
    if (!uidAccess.IsValidUID())
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetAccessIndex(uidAccess) >= 0)
        {
            return;
        }
    }
    CChar *pAccess = uidAccess.CharFind();
    if (!pAccess || !pAccess->m_pPlayer)
    {
        return;
    }
    RevokePrivs(uidAccess);
    CMultiStorage* pMultiStorage = pAccess->m_pPlayer->GetMultiStorage();
    ASSERT(pMultiStorage);
    pMultiStorage->AddMulti(GetUID(), HP_ACCESSONLY);
    _lAccesses.emplace_back(uidAccess);
}

void CItemMulti::DeleteAccess(const CUID& uidAccess, bool fRemoveFromList)
{
    ADDTOCALLSTACK("CItemMulti::DeleteAccess");
    for (size_t i = 0; i < _lAccesses.size(); ++i)
    {
        if (_lAccesses[i] == uidAccess)
        {
            if (fRemoveFromList)
            {
                _lAccesses.erase(_lAccesses.begin() + i);
            }

            CChar *pAccess = uidAccess.CharFind();
            if (pAccess && pAccess->m_pPlayer)
            {
                CMultiStorage* pMultiStorage = pAccess->m_pPlayer->GetMultiStorage();
                if (pMultiStorage)
                {
                    pMultiStorage->DelMulti(GetUID());
                }
            }
            return;
        }
    }
}

size_t CItemMulti::GetAccessCount() const
{
    return _lAccesses.size();
}

int CItemMulti::GetAccessIndex(const CUID& uidAccess) const
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

void CItemMulti::Eject(const CUID& uidChar)
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
    CWorldSearch Area(m_pRegion->m_pt, Multi_GetDistanceMax());
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

CItem *CItemMulti::GenerateKey(const CUID& uidTarget, bool fDupeOnBank)
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
        pContBank->ContentAdd(pKeyDupe);
        pTarget->SysMessageDefault(DEFMSG_MSG_KEY_DUPEBANK);
    }

    // Put in your pack
    CItemContainer* pContPack = pTarget->GetPackSafe();
    pContPack->ContentAdd(pKey);
    return pKey;
}

void CItemMulti::RemoveKeys(const CUID& uidTarget)
{
    ADDTOCALLSTACK("CItemMulti::RemoveKeys");
    if (!uidTarget.IsValidUID())
    {
        return;
    }

    CChar* pTarget = uidTarget.CharFind();
    if (!pTarget)
    {
        return;
    }

    const CUID uidHouse(GetUID());
    CItemContainer* pTargPack = pTarget->GetPack();
    if (pTargPack)
    {
        for (CSObjContRec* pObjRec : pTargPack->GetIterationSafeCont())
        {
            CItem* pItemKey = static_cast<CItem*>(pObjRec);
            if (pItemKey->m_uidLink == uidHouse)
            {
                pItemKey->Delete();
            }
        }
    }
}

void CItemMulti::RemoveAllKeys()
{
    ADDTOCALLSTACK("CItemMulti::RemoveAllKeys");
    if (g_Cfg._fAutoHouseKeys == 0) // Only when AutoHouseKeys != 0
    {
        return;
    }
    RemoveKeys(GetOwner());
    if (!_lCoowners.empty())
    {
        for (const CUID& itCoowner : _lCoowners)
        {
            RemoveKeys(itCoowner);
        }
    }
    if (!_lFriends.empty())
    {
        for (const CUID& itFriend : _lFriends)
        {
            RemoveKeys(itFriend);
        }
    }
    if (!_lAccesses.empty())
    {
        for (const CUID& itAccess : _lAccesses)
        {
            RemoveKeys(itAccess);
        }
    }
}

int16 CItemMulti::GetMultiCount() const
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

    TRIGRET_TYPE tRet = TRIGRET_RET_FALSE;
    bool fTransferAll = false;

    ITEMID_TYPE itDeed = IsType(IT_SHIP) ? ITEMID_SHIP_PLANS1 : ITEMID_DEED1;
    CItem *pDeed = CItem::CreateBase(itDeed);
    ASSERT(pDeed);

    tchar *pszName = Str_GetTemp();
    const CItemBaseMulti * pItemBase = static_cast<const CItemBaseMulti*>(Base_GetDef());
    snprintf(pszName, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_DEED_NAME), pItemBase->GetName());
    pDeed->SetName(pszName);

    bool fIsAddon = IsType(IT_MULTI_ADDON);
    if (fIsAddon)
    {
        CItemMulti *pMulti = static_cast<CItemMulti*>(m_uidLink.ItemFind());
        if (pMulti)
        {
            pMulti->DeleteAddon(GetUID());
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
    RemoveAllComponents();
    if (!fIsAddon) // Addons doesn't have to transfer anything but themselves.
    {
        if (fTransferAll)
        {
            TransferAllItemsToMovingCrate(TRANSFER_ALL);    // Whatever is left unlisted.
        }
    }

    CChar* pOwner = GetOwner().CharFind();
    if (!pOwner || !pOwner->m_pPlayer)
    {
        return;
    }

    if (!fIsAddon)
    {
        if (fMoveToBank)
        {
            TransferMovingCrateToBank();
        }
        CMultiStorage* pMultiStorage = pOwner->m_pPlayer->GetMultiStorage();
        if (pMultiStorage)
        {
            pMultiStorage->DelMulti(GetUID());
        }
    }
    if (tRet == TRIGRET_RET_TRUE)
    {
        pDeed->Delete();
        return;
    }

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

    SetKeyNum("REMOVED", 1);
    Delete();
}

void CItemMulti::SetMovingCrate(const CUID& uidCrate)
{
    ADDTOCALLSTACK("CItemMulti::SetMovingCrate");
    CItemContainer *pCurrentCrate = static_cast<CItemContainer*>(GetMovingCrate(false).ItemFind());
    CItemContainer *pNewCrate = static_cast<CItemContainer*>(uidCrate.ItemFind());

    if (!uidCrate.IsValidUID() || !pNewCrate)
    {
        _uidMovingCrate.InitUID();
        return;
    }
    if (pCurrentCrate && !pCurrentCrate->IsContainerEmpty())
    {
        pCurrentCrate->SetCrateOfMulti(CUID());
        pNewCrate->ContentsTransfer(pCurrentCrate, false);
        pCurrentCrate->Delete();
    }
    _uidMovingCrate = uidCrate;
    pNewCrate->m_uidLink = GetUID();
    pNewCrate->r_ExecSingle("EVENTS +ei_moving_crate");
    pNewCrate->SetCrateOfMulti(GetUID());
}

CUID CItemMulti::GetMovingCrate(bool fCreate)
{
    ADDTOCALLSTACK("CItemMulti::GetMovingCrate");
    if (_uidMovingCrate.IsValidUID())
    {
        return _uidMovingCrate;
    }
    if (!fCreate)
    {
        return CUID();
    }
    CItemContainer *pCrate = static_cast<CItemContainer*>(CItem::CreateBase(ITEMID_CRATE1));
    ASSERT(pCrate);
    const CUID& uidCrate = pCrate->GetUID();
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
    const bool fTransferAddons = ((iType & TRANSFER_ADDONS) || (iType & TRANSFER_ALL));
    const bool fTransferAll = (iType & TRANSFER_ALL);
    const bool fTransferLockDowns = ((iType & TRANSFER_LOCKDOWNS) || (iType & TRANSFER_ALL));
    const bool fTransferSecuredContainers = ((iType &TRANSFER_SECURED) || (iType & TRANSFER_ALL));
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
    CWorldSearch Area(ptArea, Multi_GetDistanceMax());    // largest area.
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
        if (GetComponentIndex(pItem->GetUID()) != -1)   // Components should never be transfered.
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
                    Area.RestartSearch();    // we removed an item and this will mess the search loop, so restart to fix it.
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
    if (pCrate->IsContainerEmpty())
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
            pItem->r_ExecSingle("EVENTS -ei_house_lockdown");
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
            pItem->r_ExecSingle("EVENTS -ei_house_secure");
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
        if (!pCrate->IsContainerEmpty())
        {
            if (pOwner)
            {
                pCrate->RemoveFromView();
                CItemContainer *pBank = pOwner->GetBank();
                ASSERT(pBank);
                pBank->ContentAdd(pCrate);
            }
        }
        else
        {
            pCrate->Delete();
        }
    }
}

void CItemMulti::AddAddon(const CUID& uidAddon)
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
        if (GetAddonIndex(uidAddon) >= 0)
        {
            return;
        }
    }
    _lAddons.emplace_back(uidAddon);
}

void CItemMulti::DeleteAddon(const CUID& uidAddon)
{
    ADDTOCALLSTACK("CItemMulti::DeleteAddon");
    for (size_t i = 0; i < _lAddons.size(); ++i)
    {
        if (_lAddons[i] == uidAddon)
        {
            _lAddons.erase(_lAddons.begin() + i);
            return;
        }
    }
}

int CItemMulti::GetAddonIndex(const CUID& uidAddon) const
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

size_t CItemMulti::GetAddonCount() const
{
    return _lAddons.size();
}

void CItemMulti::AddComponent(const CUID& uidComponent)
{
    ADDTOCALLSTACK("CItemMulti::AddComponent");
    if (!uidComponent.IsValidUID())
    {
        return;
    }
    if (!g_Serv.IsLoading())
    {
        if (GetComponentIndex(uidComponent) >= 0)
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

void CItemMulti::DeleteComponent(const CUID& uidComponent, bool fRemoveFromList)
{
    ADDTOCALLSTACK("CItemMulti::DeleteComponent");
    if (fRemoveFromList)
    {
        _lComps.erase(std::find(_lComps.begin(), _lComps.end(), uidComponent));
    }

    if (!uidComponent.IsValidUID()) // Doing this after the erase to force check the vector, just in case...
    {
        return;
    }
    CItem *pComp = uidComponent.ItemFind();
    if (pComp)
    {
        pComp->SetComponentOfMulti(CUID());
    }
}

int CItemMulti::GetComponentIndex(const CUID& uidComponent) const
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

size_t CItemMulti::GetComponentCount() const
{
    return _lComps.size();
}

void CItemMulti::RemoveAllComponents()
{
    ADDTOCALLSTACK("CItemMulti::RemoveAllComponents");
    if (_lComps.empty())
    {
        return;
    }
    const std::vector<CUID> lCopy(_lComps);
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

void CItemMulti::GenerateBaseComponents(bool *pfNeedKey, dword dwKeyCode)
{
    ADDTOCALLSTACK("CItemMulti::GenerateBaseComponents");
    const CItemBaseMulti * pMultiDef = Multi_GetDef();
    ASSERT(pMultiDef);
    for (size_t i = 0; i < pMultiDef->m_Components.size(); ++i)
    {
        const CItemBaseMulti::CMultiComponentItem &component = pMultiDef->m_Components[i];
        *pfNeedKey |= Multi_CreateComponent(component.m_id, component.m_dx, component.m_dy, component.m_dz, dwKeyCode);
    }
}

void CItemMulti::SetBaseStorage(uint16 iLimit)
{
    _uiBaseStorage = iLimit;
}

uint16 CItemMulti::GetBaseStorage() const
{
    return _uiBaseStorage;
}

void CItemMulti::SetIncreasedStorage(uint16 uiIncrease)
{
    _uiIncreasedStorage = uiIncrease;
}

uint16 CItemMulti::GetIncreasedStorage() const
{
    return _uiIncreasedStorage;
}

uint16 CItemMulti::GetMaxStorage() const
{
    return _uiBaseStorage + ((_uiBaseStorage * _uiIncreasedStorage) / 100);
}

uint16 CItemMulti::GetCurrentStorage() const
{
    return (int16)(_lLockDowns.size() + _lSecureContainers.size());
}

void CItemMulti::SetBaseVendors(uint8 iLimit)
{
    _uiBaseVendors = iLimit;
}

uint8 CItemMulti::GetBaseVendors() const
{
    return _uiBaseVendors;
}

uint8 CItemMulti::GetMaxVendors() const
{
    return (uint8)(_uiBaseVendors + (_uiBaseVendors * GetIncreasedStorage()) / 100);
}

uint16 CItemMulti::GetMaxLockdowns() const
{
    uint16 uiMaxStorage = GetMaxStorage();
    return (uiMaxStorage - (uiMaxStorage - (uiMaxStorage * (uint16)GetLockdownsPercent()) / 100));
}

uint8 CItemMulti::GetLockdownsPercent() const
{
    return _uiLockdownsPercent;
}

void CItemMulti::SetLockdownsPercent(uint8 iPercent)
{
    _uiLockdownsPercent = iPercent;
}

void CItemMulti::LockItem(const CUID& uidItem)
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
        if (GetLockedItemIndex(uidItem) >= 0)
        {
            return;
        }
        pItem->SetAttr(ATTR_LOCKEDDOWN);
        pItem->m_uidLink = GetUID();
        pItem->r_ExecSingle("EVENTS +ei_house_lockdown");
    }
    pItem->SetLockDownOfMulti(uidItem);
    _lLockDowns.emplace_back(uidItem);
}

void CItemMulti::UnlockItem(const CUID& uidItem, bool fRemoveFromList)
{
    ADDTOCALLSTACK("CItemMulti::UnlockItem");
    if (fRemoveFromList)
    {
        _lLockDowns.erase(std::find(_lLockDowns.begin(), _lLockDowns.end(), uidItem));
    }

    CItem *pItem = uidItem.ItemFind();
    if (!pItem)
    {
        return;
    }
    pItem->ClrAttr(ATTR_LOCKEDDOWN);
    pItem->m_uidLink.InitUID();
    pItem->r_ExecSingle("EVENTS -ei_house_lockdown");
    pItem->SetLockDownOfMulti(CUID());
}

void CItemMulti::UnlockAllItems()
{
    ADDTOCALLSTACK("CItemMulti::UnlockAllItems");
    for (const CUID& uidLockeddown : _lLockDowns)
    {
        CItem *pItem = uidLockeddown.ItemFind(true);
        if (!pItem)
            continue;

        UnlockItem(uidLockeddown, false);
    }
    _lLockDowns.clear();
}

int CItemMulti::GetLockedItemIndex(const CUID& uidItem) const
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

size_t CItemMulti::GetLockdownCount() const
{
    return _lLockDowns.size() + _lSecureContainers.size();
}

void CItemMulti::Secure(const CUID& uidContainer)
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
        if (GetSecuredContainerIndex(uidContainer) >= 0)
        {
            return;
        }
        pContainer->SetAttr(ATTR_SECURE);
        pContainer->m_uidLink = GetUID();
        pContainer->r_ExecSingle("EVENTS +ei_house_secure");

    }
    pContainer->SetSecuredOfMulti(GetUID());
    _lSecureContainers.emplace_back(uidContainer);
}

void CItemMulti::Release(const CUID& uidContainer, bool fRemoveFromList)
{
    ADDTOCALLSTACK("CItemMulti::Release");
    if (fRemoveFromList)
    {
        _lLockDowns.erase(std::remove(_lLockDowns.begin(), _lLockDowns.end(), uidContainer));
    }

    CItemContainer *pContainer = static_cast<CItemContainer*>(uidContainer.ItemFind());
    if (!pContainer)
    {
        return;
    }
    pContainer->ClrAttr(ATTR_SECURE);
    pContainer->m_uidLink.InitUID();
    pContainer->r_ExecSingle("EVENTS -ei_house_secure");
    pContainer->SetSecuredOfMulti(CUID());
}

int CItemMulti::GetSecuredContainerIndex(const CUID& uidContainer) const
{
    ADDTOCALLSTACK("CItemMulti::GetSecuredContainerIndex");
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

int16 CItemMulti::GetSecuredItemsCount() const
{
    ADDTOCALLSTACK("CItemMulti::GetSecuredItemsCount");
    size_t iCount = 0;
    for (size_t i = 0; i < _lSecureContainers.size(); ++i)
    {
        CItemContainer *pContainer = static_cast<CItemContainer*>(_lSecureContainers[i].ItemFind());
        if (pContainer)
        {
            iCount += pContainer->GetContentCount();
        }
    }
    return (int16)iCount;
}

size_t CItemMulti::GetSecuredContainersCount() const
{
    return _lSecureContainers.size();
}

void CItemMulti::AddVendor(const CUID& uidVendor)
{
    ADDTOCALLSTACK("CItemMulti::AddVendor");
    if (!uidVendor.IsValidUID())
    {
        return;
    }
    if ((g_Serv.IsLoading() == false) && (GetVendorIndex(uidVendor) >= 0))
    {
        return;
    }
    CChar *pVendor = uidVendor.CharFind();
    if (!pVendor || !pVendor->m_pPlayer)
    {
        return;
    }
    RevokePrivs(uidVendor);
    CMultiStorage* pMultiStorage = pVendor->m_pPlayer->GetMultiStorage();
    ASSERT(pMultiStorage);
    pMultiStorage->AddMulti(GetUID(), HP_VENDOR);
    _lVendors.emplace_back(uidVendor);
}

void CItemMulti::DeleteVendor(const CUID& uidVendor, bool fRemoveFromList)
{
    ADDTOCALLSTACK("CItemMulti::DeleteVendor");
    for (size_t i = 0; i < _lVendors.size(); ++i)
    {
        if (_lVendors[i] == uidVendor)
        {
            if (fRemoveFromList)
            {
                _lVendors.erase(_lVendors.begin() + i);
            }

            CChar *pVendor = uidVendor.CharFind();
            if (pVendor && pVendor->m_pPlayer)
            {
                CMultiStorage* pMultiStorage = pVendor->m_pPlayer->GetMultiStorage();
                if (pMultiStorage)
                {
                    pMultiStorage->DelMulti(GetUID());
                }
            }
            break;
        }
    }
}

int CItemMulti::GetVendorIndex(const CUID& uidVendor) const
{
    ADDTOCALLSTACK("CItemMulti::GetVendorIndex");
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

bool CItemMulti::r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK("CItemMulti::r_GetRef");
    int iCmd = FindTableHeadSorted(ptcKey, sm_szRefKeys, CountOf(sm_szRefKeys) - 1);

    if (iCmd >= 0)
    {
        ptcKey += strlen(sm_szRefKeys[iCmd]);
    }

    switch (iCmd)
    {
        case SHR_ACCESS:
        {
            const size_t idx = Exp_GetSTVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);
            if (_lAccesses.size() > idx)
            {
                CChar *pAccess = _lAccesses[idx].CharFind();
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
            const size_t idx = Exp_GetSTVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);
            if (_lAddons.size() > idx)
            {
                CItemMulti *pAddon = static_cast<CItemMulti*>(_lAddons[idx].ItemFind());
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
            const size_t idx = Exp_GetSTVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);
            if (_lBans.size() > idx)
            {
                CChar *pBan = _lBans[idx].CharFind();
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
            const int idx = Exp_GetVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);
            CItem *pComp = Multi_FindItemComponent(idx);
            if (pComp)
            {
                pRef = pComp;
                return true;
            }
            return false;
        }
        case SHR_COOWNER:
        {
            const size_t idx = Exp_GetSTVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);
            if (_lCoowners.size() > idx)
            {
                CChar *pCoowner = _lCoowners[idx].CharFind();
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
            const size_t idx = Exp_GetSTVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);
            if (_lFriends.size() > idx)
            {
                CChar *pFriend = _lFriends[idx].CharFind();
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
            const size_t idx = Exp_GetSTVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);
            if (_lLockDowns.size() > idx)
            {
                CItem *plockdown = _lLockDowns[idx].ItemFind();
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
            SKIP_SEPARATORS(ptcKey);
            pRef = static_cast<CItemContainer*>(GetMovingCrate(false).ItemFind());
            return true;
        }
        case SHR_OWNER:
        {
            SKIP_SEPARATORS(ptcKey);
            pRef = _uidOwner.CharFind();
            return true;
        }
        case SHR_GUILD:
        {
            SKIP_SEPARATORS(ptcKey);
            pRef = static_cast<CItemStone*>(_uidGuild.ItemFind());
            return true;
        }
        case SHR_REGION:
        {
            SKIP_SEPARATORS(ptcKey);
            if (m_pRegion != nullptr)
            {
                pRef = m_pRegion; // Addons do not have region so return it only when not nullptr.
                return true;
            }
            return false;
        }
        case SHR_SECURED:
        {
            const size_t idx = Exp_GetSTVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);
            if (_lSecureContainers.size() > idx)
            {
                CItem *pItem = _lSecureContainers[idx].ItemFind();
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
            const size_t idx = Exp_GetSTVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);
            if (_lVendors.size() > idx)
            {
                CChar *pVendor = _lVendors[idx].CharFind();
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

    return(CItem::r_GetRef(ptcKey, pRef));
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

    if (CCMultiMovable::r_Verb(s, pSrc))
    {
        return true;
    }
    const int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys) - 1);
    switch (iCmd)
    {
        case SHV_DELACCESS:
        {
            const CUID uidAccess(s.GetArgDWVal());
            if (!uidAccess.IsValidUID())
            {
                _lAccesses.clear();
            }
            else
            {
                DeleteAccess(uidAccess, true);
            }
            break;
        }
        case SHV_DELADDON:
        {
            const CUID uidAddon(s.GetArgDWVal());
            if (!uidAddon.IsValidUID())
            {
                _lAddons.clear();
            }
            else
            {
                DeleteAddon(uidAddon);
            }
            break;
        }
        case SHV_DELBAN:
        {
            const CUID uidBan(s.GetArgDWVal());
            if (!uidBan.IsValidUID())
            {
                _lBans.clear();
            }
            else
            {
                DeleteBan(uidBan, true);
            }
            break;
        }
        case SHV_DELCOMPONENT:
        {
            const CUID uidComp(s.GetArgDWVal());
            if (!uidComp.IsValidUID())
            {
                _lComps.clear();
            }
            else
            {
                DeleteComponent(uidComp, true);
            }
            break;
        }
        case SHV_DELCOOWNER:
        {
            const CUID uidCoowner(s.GetArgDWVal());
            if (!uidCoowner.IsValidUID())
            {
                _lCoowners.clear();
            }
            else
            {
                DeleteCoowner(uidCoowner, true);
            }
            break;
        }
        case SHV_DELFRIEND:
        {
            const CUID uidFriend(s.GetArgDWVal());
            if (!uidFriend.IsValidUID())
            {
                _lFriends.clear();
            }
            else
            {
                DeleteFriend(uidFriend, true);
            }
            break;
        }
        case SHV_DELVENDOR:
        {
            const CUID uidVendor(s.GetArgDWVal());
            if (!uidVendor.IsValidUID())
            {
                _lVendors.clear();
            }
            else
            {
                DeleteVendor(uidVendor, true);
            }
            break;
        }
        case SHV_UNLOCKITEM:
        {
            const CUID uidItem(s.GetArgDWVal());
            if (!uidItem.IsValidUID())
            {
                UnlockAllItems();
            }
            else
            {
                UnlockItem(uidItem, true);
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
            CUID uidChar(s.GetArgDWVal());
            CChar *pCharSrc = uidChar.CharFind();
            Multi_Setup(pCharSrc, 0);
            break;
        }
        case SHV_REDEED:
        {
            int64 piCmd[2];
            Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
            const bool fShowMsg = piCmd[0] ? true : false;
            const bool fMoveToBank = piCmd[1] ? true : false;
            CUID charUID;
            if (pSrc && pSrc->GetChar())
            {
                charUID.SetPrivateUID(pSrc->GetChar()->GetUID());
            }
            else if (GetOwner().CharFind())
            {
                charUID.SetPrivateUID(GetOwner());
            }
            else
            {
                g_Log.EventError("Trying to redeed %s (0x%" PRIx32 ") with no redeed target, removing it instead.\n", GetName(), (dword)GetUID());
                Delete();
                return true;
            }
            Redeed(fShowMsg, fMoveToBank, charUID);
            break;
        }
        case SHV_RELEASE:
        {
            const CUID uidRelease(s.GetArgDWVal());
            if (!uidRelease.IsValidUID())
            {
                _lAccesses.clear();
            }
            else
            {
                Release(uidRelease, true);
            }
            break;
        }
        case SHV_REMOVEKEYS:
        {
            const CUID uidChar(s.GetArgDWVal());
            RemoveKeys(uidChar);
            break;
        }
        case SHV_REMOVEALLCOMPS:
        {
            RemoveAllComponents();
            break;
        }
        case SHV_GENERATEBASECOMPONENTS:
        {
            bool fNeedKey = false;
            GenerateBaseComponents(&fNeedKey, UID_CLEAR);
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
    if (_uidGuild.IsValidUID())
    {
        s.WriteKeyHex("GUILD", (dword)_uidGuild);
    }
    if (_uidOwner.IsValidUID())
    {
        s.WriteKeyHex("OWNER", (dword)_uidOwner);
    }

    // House general
    if (_iHouseType)
    {
        s.WriteKeyHex("HOUSETYPE", (dword)_iHouseType);
    }
    if (!_lCoowners.empty())
    {
        for (const CUID& uid : _lCoowners)
        {
            if (uid.IsValidUID())
            {
                s.WriteKeyHex("ADDCOOWNER", (dword)uid);
            }
        }
    }
    if (!_lFriends.empty())
    {
        for (const CUID& uid : _lFriends)
        {
            if (uid.IsValidUID())
            {
                s.WriteKeyHex("ADDFRIEND", (dword)uid);
            }
        }
    }

    if (!_lComps.empty())
    {
        for (const CUID& uid : _lComps)
        {
            if (uid.IsValidUID())
            {
                s.WriteKeyHex("ADDCOMP", (dword)uid);
            }
        }
    }
    if (!_lAddons.empty())
    {
        for (const CUID& uid : _lAddons)
        {
            if (uid.IsValidUID())
            {
                s.WriteKeyHex("ADDADDON", (dword)uid);
            }
        }
    }
    if (!_lSecureContainers.empty())
    {
        for (const CUID& uid : _lSecureContainers)
        {
            if (uid.IsValidUID())
            {
                s.WriteKeyHex("SECURE", (dword)uid);
            }
        }
    }

    if (!_lLockDowns.empty())
    {
        for (const CUID& uid : _lLockDowns)
        {
            if (uid.IsValidUID())
            {
                s.WriteKeyHex("LOCKITEM", (dword)uid);
            }
        }
    }
    if (auto val = GetLockdownsPercent())
    {
        s.WriteKeyVal("LOCKDOWNSPERCENT", val);
    }

    const CUID uidCrate = GetMovingCrate(false);
    if (uidCrate.IsValidUID())
    {
        s.WriteKeyHex("MOVINGCRATE", uidCrate.GetObjUID());
    }

    // House vendors
    if (!_lVendors.empty())
    {
        for (const CUID& uid : _lVendors)
        {
            if (uid.IsValidUID())
            {
                s.WriteKeyHex("ADDVENDOR", (dword)uid);
            }
        }
    }
    if (auto val = GetBaseVendors())
    {
        s.WriteKeyVal("BASEVENDORS", val);
    }

    // House Storage
    if (auto val = GetBaseStorage())
    {
        s.WriteKeyVal("BASESTORAGE", val);
    }
    if (auto val = GetIncreasedStorage())
    {
        s.WriteKeyVal("INCREASEDSTORAGE", val);
    }
}

bool CItemMulti::r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren)
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
    ADDTOCALLSTACK("CItemMulti::r_WriteVal");
    if (CCMultiMovable::r_WriteVal(ptcKey, sVal, pSrc))
    {
        return true;
    }
    int iCmd = FindTableHeadSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iCmd >= 0)
    {
        ptcKey += strlen(sm_szLoadKeys[iCmd]);
    }
    SKIP_SEPARATORS(ptcKey);
    switch (iCmd)
    {
        // House Permissions
        case SHL_PRIV:
        {
            ASSERT(pSrc);
            if (CChar* pSrcChar = pSrc->GetChar())
            {
                if (pSrcChar->m_pPlayer)
                {
                    CMultiStorage* pMultiStorage = pSrcChar->m_pPlayer->GetMultiStorage();
                    ASSERT(pMultiStorage);
                    sVal.FormatU8Val((uint8)pMultiStorage->GetPriv(GetUID()));
                    break;
                }
            }
            sVal.FormatVal(0);
            break;
        }
        case SHL_OWNER:
        {
            CChar *pOwner = GetOwner().CharFind();
            if (!IsStrEmpty(ptcKey))
            {
                if (pOwner)
                {
                    return pOwner->r_WriteVal(ptcKey, sVal, pSrc);
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
            CItemStone *pGuild = static_cast<CItemStone*>(GetGuildStone().ItemFind());
            if (!IsStrEmpty(ptcKey))
            {
                if (pGuild)
                {
                    return pGuild->r_WriteVal(ptcKey, sVal, pSrc);
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
            CUID uidOwner(Exp_GetVal(ptcKey));
            sVal.FormatBVal(IsOwner(uidOwner));
            break;
        }
        case SHL_GETCOOWNERPOS:
        {
            CUID uidCoowner(Exp_GetVal(ptcKey));
            sVal.FormatVal(GetCoownerIndex(uidCoowner));
            break;
        }
        case SHL_COOWNERS:
        {
            sVal.FormatSTVal(GetCoownerCount());
            break;
        }
        case SHL_GETFRIENDPOS:
        {
            CUID uidFriend(Exp_GetVal(ptcKey));
            sVal.FormatVal(GetFriendIndex(uidFriend));
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
            CUID uidAccess(Exp_GetVal(ptcKey));
            sVal.FormatVal(GetAccessIndex(uidAccess));
            break;
        }
        case SHL_ADDONS:
        {
            sVal.FormatSTVal(GetAddonCount());
            break;
        }
        case SHL_GETADDONPOS:
        {
            CUID uidAddon(Exp_GetVal(ptcKey));
            sVal.FormatVal(GetAddonIndex(uidAddon));
            break;
        }
        case SHL_BANS:
        {
            sVal.FormatSTVal(GetBanCount());
            break;
        }
        case SHL_GETBANPOS:
        {
            CUID uidBan(Exp_GetVal(ptcKey));
            sVal.FormatVal(GetBanIndex(uidBan));
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
            CUID uidComp(Exp_GetVal(ptcKey));
            sVal.FormatVal(GetComponentIndex(uidComp));
            break;
        }
        case SHL_MOVINGCRATE:
        {
            bool fCreate = false;

            if (strlen(ptcKey) <= 2) // check for <MovingCrate 0/1> (whitespace + number = 2 len)
            {
                fCreate = Exp_GetVal(ptcKey);
            }
            else if (!IsStrEmpty(ptcKey) && _uidMovingCrate.IsValidUID())    // If there's an already existing Moving Crate and args len is greater than 1, it should have a keyword to send to the crate.
            {
                CItemContainer *pCrate = static_cast<CItemContainer*>(_uidMovingCrate.ItemFind());
                ASSERT(pCrate); // Removed crates should init _uidMovingCrate's uid?
                return pCrate->r_WriteVal(ptcKey, sVal, pSrc);
            }
            if (fCreate)
            {
                GetMovingCrate(true);
            }
            if (_uidMovingCrate.IsValidUID())
            {
                sVal.FormatDWVal(_uidMovingCrate);
            }
            else
            {
                sVal.FormatDWVal(0);
            }
            break;
        }
        case SHL_ADDKEY:
        {
            ptcKey += 6;
            CUID uidOwner(Exp_GetVal(ptcKey));
            bool fDupeOnBank = Exp_GetVal(ptcKey) ? 1 : 0;
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
            sVal.FormatSTVal(GetComponentCount());
            break;
        }
        case SHL_LOCKDOWNS:
        {
            sVal.FormatSTVal(GetLockdownCount());
            break;
        }
        case SHL_GETSECUREDCONTAINERPOS:
        {
            CUID uidCont(Exp_GetDWVal(ptcKey));
            if (uidCont.IsValidUID())
            {
                sVal.FormatVal(GetSecuredContainerIndex(uidCont));
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
            sVal.FormatVal(GetVendorIndex(CUID(Exp_GetVal(ptcKey))));
            break;
        }
        case SHL_GETLOCKEDITEMPOS:
        {
            sVal.FormatVal(GetLockedItemIndex(CUID(Exp_GetVal(ptcKey))));
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
            return (fNoCallParent ? false : CItem::r_WriteVal(ptcKey, sVal, pSrc));
    }
    return true;
}

bool CItemMulti::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CItemMulti::r_LoadVal");
    EXC_TRY("LoadVal");

    if (CCMultiMovable::r_LoadVal(s))
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
            script.CopyParseState(s);
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
            CUID uidCrate(s.GetArgDWVal());
            if (uidCrate.IsValidUID())
            {
                if (uidCrate.GetPrivateUID() == 1) // fix for 'movingcrate 1'
                {
                    GetMovingCrate(true);
                }
                else
                {
                    SetMovingCrate(uidCrate);
                }
            }
            else
            {
                SetMovingCrate(CUID());
            }
            break;
        }
        case SHL_ADDKEY:
        {
            int64 piCmd[2];
            int iLen = Str_ParseCmds(s.GetArgStr(), piCmd, CountOf(piCmd));
            CUID uidOwner((dword)piCmd[0]);
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
            lpctstr ptcKey = s.GetKey();
            ptcKey += 5;
            if (*ptcKey == '.')
            {
                CChar *pOwner = GetOwner().CharFind();
                if (pOwner)
                {
                    return pOwner->r_LoadVal(s);
                }
                return false;
            }
            CUID uid(s.GetArgDWVal());
            if (!uid.IsValidUID())
            {
                SetOwner(CUID());
            }
            else
            {
                SetOwner(uid);
            }
            break;
        }
        case SHL_GUILD:
        {
            lpctstr ptcKey = s.GetKey();
            ptcKey += 5;
            if (*ptcKey == '.')
            {
                CItemStone *pGuild = static_cast<CItemStone*>(GetGuildStone().ItemFind());
                if (pGuild)
                {
                    return pGuild->r_LoadVal(s);
                }
                return false;
            }
            CUID uid(s.GetArgDWVal());
            if (!uid.IsValidUID())
            {
                SetGuild(CUID());
                break;
            }
            const CItem *pItem = uid.ItemFind();
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
            AddCoowner(CUID(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDFRIEND:
        {
            AddFriend(CUID(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDBAN:
        {
            AddBan(CUID(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDACCESS:
        {
            AddAccess(CUID(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDADDON:
        {
            AddAddon(CUID(s.GetArgDWVal()));
            break;
        }

        // House Storage
        case SHL_ADDCOMP:
        {
            AddComponent(CUID(s.GetArgDWVal()));
            break;
        }
        case SHL_ADDVENDOR:
        {
            AddVendor(CUID(s.GetArgDWVal()));
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
            CUID uidLock(s.GetArgDWVal());
            if (uidLock.IsValidUID())
            {
                LockItem(uidLock);
            }
            break;
        }
        case SHL_SECURE:
        {
            CUID uidSecured(s.GetArgDWVal());
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

CItem *CItemMulti::Multi_Create(CChar *pChar, const CItemBase * pItemDef, CPointMap & pt, CItem *pDeed)
{
    if (!pChar || !pChar->m_pPlayer)
    {
        return nullptr;
    }

    ASSERT(pItemDef);
    const bool fShip = pItemDef->IsType(IT_SHIP);    // must be in water.

    // pMultiDef can be nullptr, because we can also add simple items with a deed, not only multis.
    const CItemBaseMulti* pMultiDef = dynamic_cast <const CItemBaseMulti*> (pItemDef);

    /*
    * First thing to do is to check if the character creating the multi is allowed to have it
    */
    if (fShip)
    {
        ASSERT(pMultiDef);
        CMultiStorage* pMultiStorage = pChar->m_pPlayer->GetMultiStorage();
        ASSERT(pMultiStorage);
        if (!pMultiStorage->CanAddShip(pChar->GetUID(), pMultiDef->_iMultiCount))
        {
            return nullptr;
        }
    }
    else if (pItemDef->IsType(IT_MULTI) || pItemDef->IsType(IT_MULTI_CUSTOM))
    {
        ASSERT(pMultiDef);
        CMultiStorage* pMultiStorage = pChar->m_pPlayer->GetMultiStorage();
        ASSERT(pMultiStorage);
        if (!pMultiStorage->CanAddHouse(pChar->GetUID(), pMultiDef->_iMultiCount))
        {
            return nullptr;
        }
    }
    CScriptTriggerArgs args;
    args.m_VarsLocal.SetStrNew("check_blockradius", "-1, -1, 1, 1");    // Values are West, Norht, East, South
    args.m_VarsLocal.SetStrNew("check_multiradius", "0, -5, 0, 5");
    args.m_VarsLocal.SetStrNew("id", g_Cfg.ResourceGetName(CResourceID(RES_ITEMDEF, pItemDef->GetID())));
    args.m_VarsLocal.SetStrNew("p", pt.WriteUsed());
    TRIGRET_TYPE tRet;
    pChar->r_Call("f_multi_onplacement_check", pChar, &args, nullptr, &tRet);
    if (tRet == TRIGRET_RET_TRUE)
    {
        return nullptr;
    }

    /*
    * There is a small difference in the coordinates where the mouse is in the screen and the ones received,
    * let's remove that difference.
    * Note: this fix is added here, before GM check, because otherwise they will place houses on wrong position.
    */
    if (pMultiDef && (CItemBase::IsID_Multi(pItemDef->GetID()) || pItemDef->IsType(IT_MULTI_ADDON)))
    // or is this happening only for houses? if (CItemBase::IsID_House(pItemDef->GetID()))
    {
        pt.m_y -= (short)(pMultiDef->m_rect.m_bottom - 1);
    }

    if (!pChar->IsPriv(PRIV_GM) && tRet != TRIGRET_RET_HALFBAKED)
    {
        CRect rectBlockRadius;
        rectBlockRadius.Read(args.m_VarsLocal.GetKeyStr("check_blockradius"));

        CRect rectMultiRadius;
        rectMultiRadius.Read(args.m_VarsLocal.GetKeyStr("check_multiradius"));

        if (!pDeed->IsAttr(ATTR_MAGIC))
        {
            CRect rect;
            CRect baseRect;
            if (pMultiDef)
            {
                baseRect = pMultiDef->m_rect; // Create a rect equal to the multi's one.
            }
            else
            {
                baseRect.OffsetRect(1, 1);  // Guildstones pass through here, also other non multi items placed from deeds.
            }
            baseRect.m_map = pt.m_map;          // set it's map to the current map.
            baseRect.OffsetRect(pt.m_x, pt.m_y);// fill the rect.
            rect = baseRect;
            CPointMap ptn = pt;             // A copy to work on.

            // Check for chars in the way, just search for any char in the house area, no extra tiles, it's enough for them to do not be inside the house.
            CWorldSearch Area(pt, std::max(rect.GetWidth(), rect.GetHeight()));
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
            *   - The area of search gets increased by 1* tile: There must be no blocking items from a 1x1* gap outside the house.
            *   - All coord points inside that rect must be valid and have, as much, a Z difference of +-4.
            *
            * *1 tile by default, values can be overriden with local.check_BlockRadius in the f_house_onplacement_check function.
            */
            rect += rectBlockRadius;
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
                    ptn.m_z = CWorldMap::GetHeightPoint2(ptn, dwBlockFlags, true);
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
            * You can't place your house within a range of 5* tiles bottom of the stairs of any house.
            * Additionally your house can't have it's bottom blocked by another multi in a 5* tiles margin.
            * Simplifying it: the Rect must have an additional +5* tiles of radius on both TOP and BOTTOM points.
            *
            * *5 North & South tiles by default, values can be overriden with local.check_MultiRadius in the f_house_onplacement_check function.
            */
            rect = baseRect;
            rect += rectMultiRadius;

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

    pItemNew->SetAttr(ATTR_MOVE_NEVER | (pDeed->m_Attr & (ATTR_MAGIC | ATTR_INVIS)));
    pItemNew->SetHue(pDeed->GetHue());
    pItemNew->MoveToUpdate(pt);

    CItemMulti * pMultiItem = dynamic_cast <CItemMulti*>(pItemNew);
    if (pMultiItem)
    {
        pMultiItem->Multi_Setup(pChar, UID_CLEAR);
    }

    if (pItemDef->IsType(IT_STONE_GUILD))
    {
        // Now name the guild
        CItemStone * pStone = dynamic_cast <CItemStone*>(pItemNew);
        ASSERT(pStone);
        pStone->AddRecruit(pChar, STONEPRIV_MASTER);
        if (CClient* pClient = pChar->GetClientActive())
        {
            pClient->addPromptConsole(CLIMODE_PROMPT_STONE_NAME, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_GUILDSTONE_NEW), pItemNew->GetUID());
        }
    }
    else if (pItemDef->IsType(IT_SHIP))
    {
        pItemNew->Sound(Calc_GetRandVal(2) ? 0x12 : 0x13);
    }
    return pItemNew;
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
            pComponent->r_ExecSingle("EVENTS +ei_house_door");
            pComponent->SetType(IT_DOOR_LOCKED);
            break;
        }
        case IT_CONTAINER:
        {
            pComponent->r_ExecSingle("EVENTS  +ei_house_container");
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
            pComponent->r_ExecSingle("EVENTS +ei_house_telepad");
            break;
        }
        default:
            break;
    }

    if (pComponent->GetHue() == HUE_DEFAULT)
    {
        pComponent->SetHue(GetHue());
    }
}

CMultiStorage::CMultiStorage(const CUID& uidSrc)
{
    _uidSrc = uidSrc;
    _iShipsTotal = 0;
    _iHousesTotal = 0;
}

CMultiStorage::~CMultiStorage()
{
    for (auto& pair : _lHouses)
    {
        const CUID& uid = pair.first;
        CItemMulti *pMulti = static_cast<CItemMulti*>(uid.ItemFind());
        if (!pMulti)
            continue;

        if (_uidSrc.IsChar())
        {
            pMulti->RevokePrivs(_uidSrc);
        }
        else if (_uidSrc.IsItem())   //Only guild/town stones
        {
            pMulti->SetGuild(CUID());
        }
    }

    for (auto& pair : _lShips)
    {
        const CUID& uid = pair.first;
        CItemShip *pShip = static_cast<CItemShip*>(uid.ItemFind());
        if (!pShip)
            continue;

        if (_uidSrc.IsChar())
        {
            pShip->RevokePrivs(_uidSrc);
        }
        else if (_uidSrc.IsItem())   //Only guild/town stones
        {
            pShip->SetGuild(CUID());
        }
    }

    _uidSrc.InitUID();
}

void CMultiStorage::AddMulti(const CUID& uidMulti, HOUSE_PRIV ePriv)
{
    if (!uidMulti.IsValidUID())
    {
        return;
    }
    const CItemMulti *pMulti = static_cast<CItemMulti*>(uidMulti.ItemFind());
    if (!pMulti)
    {
        return;
    }
    CObjBase *pSrc = _uidSrc.ObjFind();
    const bool isChar = pSrc && pSrc->IsChar();
    if (pMulti->IsType(IT_SHIP))
    {
        if (_lShips.empty() && isChar)
        {
            pSrc->r_ExecSingle("EVENTS +e_ship_priv");
        }
        AddShip(uidMulti, ePriv);
    }
    else
    {
        if (_lHouses.empty() && isChar)
        {
            pSrc->r_ExecSingle("EVENTS +e_house_priv");
        }
        AddHouse(uidMulti, ePriv);
    }
}

void CMultiStorage::DelMulti(const CUID& uidMulti)
{
    CItemMulti *pMulti = static_cast<CItemMulti*>(uidMulti.ItemFind());
    CObjBase *pSrc = _uidSrc.ObjFind();
    if (pMulti->IsType(IT_SHIP))
    {
        DelShip(uidMulti);
        if (_lShips.empty() && pSrc)
        {
            pSrc->r_ExecSingle("EVENTS -e_ship_priv");
        }
    }
    else
    {
        DelHouse(uidMulti);
        if (_lHouses.empty() && pSrc)
        {
            pSrc->r_ExecSingle("EVENTS -e_house_priv");
        }
    }
    pMulti->RevokePrivs(_uidSrc);
}

void CMultiStorage::AddHouse(const CUID& uidHouse, HOUSE_PRIV ePriv)
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
    const CItemMulti *pMulti = static_cast<CItemMulti*>(uidHouse.ItemFind());
    _iHousesTotal += pMulti->GetMultiCount();
    _lHouses[uidHouse] = ePriv;
}

void CMultiStorage::DelHouse(const CUID& uidHouse)
{
    ADDTOCALLSTACK("CMultiStorage::DelHouse");
    if (_lHouses.empty())
    {
        return;
    }

    if (_lHouses.find(uidHouse) != _lHouses.end())
    {
        const CItemMulti *pMulti = static_cast<CItemMulti*>(uidHouse.ItemFind());
        _iHousesTotal -= pMulti->GetMultiCount();
        _lHouses.erase(uidHouse);
        return;
    }
}

HOUSE_PRIV CMultiStorage::GetPriv(const CUID& uidMulti)
{
    ADDTOCALLSTACK("CMultiStorage::GetPrivMulti");
    const CItemMulti *pMulti = static_cast<CItemMulti*>(uidMulti.ItemFind());
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

bool CMultiStorage::CanAddHouse(const CUID& uidChar, int16 iHouseCount) const
{
    ADDTOCALLSTACK("CMultiStorage::CanAddHouse");
    if (uidChar.IsItem())
    {
        return true;    // Guilds can have unlimited houses atm.
    }

    const CChar *pChar = uidChar.CharFind();
    if (!pChar || !pChar->m_pPlayer)
    {
        return false;
    }
    if (iHouseCount == 0 || pChar->IsPriv(PRIV_GM))
    {
        return true;
    }

    uint8 iMaxHouses = pChar->m_pPlayer->_iMaxHouses;
    if (iMaxHouses == 0)
    {
        if (pChar->GetClientActive())
        {
            iMaxHouses = pChar->GetClientActive()->GetAccount()->_iMaxHouses;
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

int16 CMultiStorage::GetHousePos(const CUID& uidHouse) const
{
    ADDTOCALLSTACK("CMultiStorage::GetHousePos");
    if (_lHouses.empty())
    {
        return -1;
    }
    int16 i = 0;
    for (MultiOwnedCont::const_iterator it = _lHouses.begin(); it != _lHouses.end(); ++it)
    {
        if (it->first == uidHouse)
        {
            return i;
        }
        ++i;
    }
    return -1;
}

int16 CMultiStorage::GetHouseCountTotal() const
{
    return _iHousesTotal;
}

int16 CMultiStorage::GetHouseCountReal() const
{
    return (int16)_lHouses.size();
}

CUID CMultiStorage::GetHouseAt(int16 iPos) const
{
    MultiOwnedCont::const_iterator it = _lHouses.begin();
    std::advance(it, iPos);
    return it->first;
}

void CMultiStorage::ClearHouses()
{
    _lHouses.clear();
}

void CMultiStorage::r_Write(CScript & s) const
{
    ADDTOCALLSTACK("CMultiStorage::r_Write");
    UNREFERENCED_PARAMETER(s);
}

void CMultiStorage::AddShip(const CUID& uidShip, HOUSE_PRIV ePriv)
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
    const CItemShip *pShip = static_cast<CItemShip*>(uidShip.ItemFind());
    if (!pShip)
    {
        return;
    }
    _iShipsTotal += pShip->GetMultiCount();
    _lShips[uidShip] = ePriv;
}

void CMultiStorage::DelShip(const CUID& uidShip)
{
    ADDTOCALLSTACK("CMultiStorage::DelShip");
    if (_lShips.empty())
    {
        return;
    }

    if (_lShips.find(uidShip) != _lShips.end())
    {
        const CItemShip *pShip = static_cast<CItemShip*>(uidShip.ItemFind());
        if (pShip)
        {
            _iShipsTotal -= pShip->GetMultiCount();
        }
        _lShips.erase(uidShip);
        return;
    }
}

bool CMultiStorage::CanAddShip(const CUID& uidChar, int16 iShipCount)
{
    ADDTOCALLSTACK("CMultiStorage::CanAddShip");
    if (uidChar.IsItem())
    {
        return true; // Guilds can have unlimited ships atm.
    }

    const CChar *pChar = uidChar.CharFind();
    if (!pChar || !pChar->m_pPlayer)
    {
        return false;
    }
    if (iShipCount == 0 || pChar->IsPriv(PRIV_GM))
    {
        return true;
    }

    uint8 iMaxShips = pChar->m_pPlayer->_iMaxShips;
    if (iMaxShips == 0)
    {
        if (pChar->GetClientActive())
        {
            iMaxShips = pChar->GetClientActive()->GetAccount()->_iMaxShips;
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

int16 CMultiStorage::GetShipPos(const CUID& uidShip)
{
    if (_lShips.empty())
    {
        return -1;
    }
    int16 i = 0;
    for (MultiOwnedCont::iterator it = _lShips.begin(); it != _lShips.end(); ++it)
    {
        if (it->first == uidShip)
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
    MultiOwnedCont::const_iterator it = _lShips.begin();
    std::advance(it, iPos);
    return it->first;
}

void CMultiStorage::ClearShips()
{
    _lShips.clear();
}
