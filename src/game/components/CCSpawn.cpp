#include "../../common/resource/sections/CRandGroupDef.h"
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../chars/CChar.h"
#include "../chars/CCharNPC.h"
#include "../CObjBase.h"
#include "../CContainer.h"
#include "../CRegion.h"
#include "../CServer.h"
#include "../CWorldMap.h"
#include "CCChampion.h"
#include "CCSpawn.h"
#include <algorithm>


/////////////////////////////////////////////////////////////////////////////
std::vector<CCSpawn*> CCSpawn::_vBadSpawns; // static

void CCSpawn::AddBadSpawn()
{
    ADDTOCALLSTACK("CCSpawn::AddBadSpawn");
    if (_fIsBadSpawn == true)
    {
        return;
    }
    THREAD_UNIQUE_LOCK_SET;
    if (std::find(_vBadSpawns.cbegin(), _vBadSpawns.cend(), this) == _vBadSpawns.cend())
    {
        _vBadSpawns.emplace_back(this); //only if it's not inserted already.
    }
    _fIsBadSpawn = true;
}

void CCSpawn::DelBadSpawn()
{
    ADDTOCALLSTACK("CCSpawn::DelBadSpawn");
    THREAD_UNIQUE_LOCK_SET;
    if (!_vBadSpawns.empty())
    {
        _vBadSpawns.erase(std::find(_vBadSpawns.begin(), _vBadSpawns.end(), this));
    }
    _fIsBadSpawn = false;
}

CCSpawn *CCSpawn::GetBadSpawn(int index)
{
    ADDTOCALLSTACK("CCSpawn::GetBadSpawn");
    CCSpawn* ret = nullptr;
    if (!_vBadSpawns.empty())
    {
        if ((index < 0) || (index >= (int)_vBadSpawns.size()))
        {
            index = 0;
        }
        ret = _vBadSpawns[index];
    }
    return ret;
}

CCSpawn::CCSpawn(CItem *pLink, bool fIsChampion) : CComponent(COMP_SPAWN), _fIsChampion(fIsChampion)
{
    //ADDTOCALLSTACK_INTENSIVE("CCSpawn::CCSpawn");
    _pLink = pLink;
    _iAmount = 1;
    _iPile = 1;
    _iMaxDist = 15;
    _iTimeLo = 15;
    _iTimeHi = 30;
    _fKillingChildren = false;
    _fIsBadSpawn = false;
}

CCSpawn::~CCSpawn()
{
    KillChildren();
    DelBadSpawn();
}

CItem * CCSpawn::GetLink() const
{
    return _pLink;
}

uint16 CCSpawn::GetAmount() const
{
    return _iAmount;
}

uint16 CCSpawn::GetCurrentSpawned() const
{
    return (uint16)_uidList.size();
}

uint16 CCSpawn::GetPile() const
{
    return _iPile;
}

uint16 CCSpawn::GetTimeLo() const
{
    return _iTimeLo;
}

uint16 CCSpawn::GetTimeHi() const
{
    return _iTimeHi;
}

uint8 CCSpawn::GetDistanceMax() const
{
    return _iMaxDist;
}

const CResourceIDBase& CCSpawn::GetSpawnID() const
{
    return _idSpawn;
}

void CCSpawn::SetAmount(uint16 iAmount)
{
    //ADDTOCALLSTACK("CCSpawn::SetAmount");
    _iAmount = iAmount;
}

const CResourceDef* CCSpawn::_FixDef()
{
    ADDTOCALLSTACK("CCSpawn:FixDef");

    const CItem* pItem = GetLink();
    const uint uiItemUID = pItem->GetUID().GetObjUID();
    if (!_idSpawn.IsValidUID())
    {
        g_Log.EventDebug("CCSpawn::FixDef found invalid spawn (UID=0%x) with a SpawnID not set yet.\n", uiItemUID);
        return nullptr;
    }

    const RES_TYPE resType = _idSpawn.GetResType();
    const CResourceDef* pResDef;
    if (resType != RES_UNKNOWN)
    {
        pResDef = g_Cfg.ResourceGetDef(_idSpawn);
        if (pResDef)
            return pResDef;     // valid spawn
    }

    const int iIndex = _idSpawn.GetResIndex();
    if (iIndex == 0)
    {
        g_Log.EventDebug("CCSpawn::FixDef found invalid spawn (UID=0%x) with SpawnID=0.\n", uiItemUID);
        return nullptr;
    }
    if (resType == RES_UNKNOWN)
        g_Log.EventDebug("CCSpawn::FixDef found invalid spawn (UID=0%x) without type info and SpawnID=0%x. Attempting to fix it.\n", uiItemUID, iIndex);
    else
        g_Log.EventDebug("CCSpawn::FixDef found invalid spawn (UID=0%x) with SpawnID=0%x. Attempting to fix it.\n", uiItemUID, iIndex);

    if (pItem->IsType(IT_SPAWN_CHAR))
    {
        auto _TryChar = [this](CREID_TYPE idChar) -> const CResourceDef*
        {
            const CResourceDef* pResDef = CCharBase::FindCharBase(idChar);
            if (pResDef)
                _idSpawn = CResourceIDBase(RES_CHARDEF, idChar);
            return pResDef;
        };

        CREID_TYPE idChar = (CREID_TYPE)iIndex;
        if (idChar >= SPAWNTYPE_START)
        {
            // try a spawn group.
            const CResourceIDBase rid(RES_SPAWN, iIndex);
            pResDef = g_Cfg.ResourceGetDef(rid);
            if (pResDef)
            {
                g_Log.EventDebug("CCSpawn::FixDef fixed on spawner with UID=0%x a SPAWN type resource from Resource ID 0%" PRIx32 " to 0%" PRIx32 ".\n", uiItemUID, _idSpawn.GetPrivateUID(), rid.GetPrivateUID());
                _idSpawn = rid;
                return pResDef;
            }
            pResDef = _TryChar(idChar);
            if (pResDef)
            {
                g_Log.EventDebug("CCSpawn::FixDef found on spawner with UID=0%x Index 0%x being not a SPAWN, but >= SPAWNTYPE_START.\n", uiItemUID, iIndex);
                g_Log.EventDebug("CCSpawn::FixDef fixed on spawner with UID=0%x a CHAR type resource from Resource ID 0%" PRIx32 " to 0%" PRIx32 ".\n", uiItemUID, _idSpawn.GetPrivateUID(), rid.GetPrivateUID());
            }
            else
            {
                g_Log.EventDebug("CCSpawn::FixDef found on spawner with UID=0%x Index 0%x < SPAWNTYPE_START being not a SPAWN nor a CHAR. Can't fix this.\n", uiItemUID, iIndex);
            }
            return pResDef;
        }
        else
        {
            // it should be a char
            const CResourceIDBase ridPrev = _idSpawn;
            pResDef = _TryChar(idChar);
            if (pResDef)
                g_Log.EventDebug("CCSpawn::FixDef fixed on spawner with UID=0%x a CHAR type resource from Resource ID 0%" PRIx32 " to 0%" PRIx32 ".\n", uiItemUID, _idSpawn.GetPrivateUID(), ridPrev.GetPrivateUID());
            else
                g_Log.EventDebug("CCSpawn::FixDef found on spawner with UID=0%x Index 0%x being not a CHAR, but < SPAWNTYPE_START. Can't fix this.\n", uiItemUID, iIndex);
            return pResDef;
        }
    }
    else
    {
        auto _TryItem = [this](ITEMID_TYPE idItem) -> const CResourceDef*
        {
            const CResourceDef* pResDef = CItemBase::FindItemBase(idItem);
            if (pResDef)
                _idSpawn = CResourceIDBase(RES_ITEMDEF, idItem);
            return pResDef;
        };

        ITEMID_TYPE idItem = (ITEMID_TYPE)iIndex;
        if (idItem >= ITEMID_TEMPLATE)
        {
            // try a template.
            const CResourceIDBase rid(RES_TEMPLATE, iIndex);
            pResDef = g_Cfg.ResourceGetDef(rid);
            if (pResDef)
            {
                g_Log.EventDebug("CCSpawn::FixDef fixed on spawner with UID=0%x a TEMPLATE type resource from Resource ID 0%" PRIx32 " to 0%" PRIx32 ".\n", uiItemUID, _idSpawn.GetPrivateUID(), rid.GetPrivateUID());
                _idSpawn = rid;
                return pResDef;
            }
            pResDef = _TryItem(idItem);
            if (pResDef)
            {
                g_Log.EventDebug("CCSpawn::FixDef found on spawner with UID=0%x Index 0%x being not a TEMPLATE, but >= ITEMID_TEMPLATE.\n", uiItemUID, iIndex);
                g_Log.EventDebug("CCSpawn::FixDef fixed on spawner with UID=0%x a ITEM type resource from Resource ID 0%" PRIx32 " to 0%" PRIx32 ".\n", uiItemUID, _idSpawn.GetPrivateUID(), rid.GetPrivateUID());
            }
            else
            {
                g_Log.EventDebug("CCSpawn::FixDef found on spawner with UID=0%x Index 0%x < ITEMID_TEMPLATE being not a TEMPLATE nor a ITEM. Can't fix this.\n", uiItemUID, iIndex);
            }
            return pResDef;
        }
        else
        {
            // it should be an item
            const CResourceID ridPrev = _idSpawn;
            pResDef = _TryItem(idItem);
            if (pResDef)
                g_Log.EventDebug("CCSpawn::FixDef fixed on spawner with UID=0%x an ITEM type resource from Resource ID 0%" PRIx32 " to 0%" PRIx32 ".\n", uiItemUID, _idSpawn.GetPrivateUID(), ridPrev.GetPrivateUID());
            else
                g_Log.EventDebug("CCSpawn::FixDef found on spawner with UID=0%x Index 0%x being not an ITEM, but < ITEMID_TEMPLATE. Can't fix this.\n", uiItemUID, iIndex);
            return pResDef;
        }
    }
}

const CResourceDef *CCSpawn::FixDef()
{
    const CResourceDef* pRef = _FixDef();
    if (pRef == nullptr && _fIsChampion == false)
    {
        AddBadSpawn();
    }
    return pRef;
}

uint CCSpawn::WriteName(tchar *ptcOut) const
{
    ADDTOCALLSTACK("CCSpawn::GetName");
    lpctstr ptcName = nullptr;
    const CResourceDef *pDef = g_Cfg.ResourceGetDef(_idSpawn);
    if (pDef != nullptr)
        ptcName = pDef->GetName();
    if (pDef == nullptr || ptcName == nullptr || ptcName[0] == '\0')
        ptcName = g_Cfg.ResourceGetName(_idSpawn);

    return snprintf(ptcOut, STR_TEMPLENGTH, " (%s)", ptcName);
}

void CCSpawn::Delete(bool fForce)
{
    ADDTOCALLSTACK("CCSpawn::Delete");
    UNREFERENCED_PARAMETER(fForce);
    KillChildren();
    if (_fIsBadSpawn == true)
    {
        DelBadSpawn();
    }
}

void CCSpawn::GenerateItem()
{
    ADDTOCALLSTACK("CCSpawn::GenerateItem");
    
    CItem *pSpawnItem = GetLink();
    if (!pSpawnItem->IsTopLevel())
        return;

    const CResourceDef *pDef = FixDef();
    if (!pDef)
    {
        g_Log.EventError("Bad spawn point (UID=0%" PRIx32 ") is trying to generate an item/template. Invalid spawn index 0%x (ResourceID=0%" PRIx32 ").\n", (dword)pSpawnItem->GetUID(), _idSpawn.GetResIndex(), _idSpawn.GetPrivateUID());
        AddBadSpawn();
        return;
    }

    CResourceIDBase rid(pDef->GetResourceID());

    if (IsTrigUsed(TRIGGER_PRESPAWN))
    {
        CScriptTriggerArgs args(rid.GetResIndex());
        if (pSpawnItem->OnTrigger(ITRIG_PreSpawn, &g_Serv, &args) == TRIGRET_RET_TRUE)
        {
            return;
        }
        rid = CResourceIDBase(RES_ITEMDEF, (int)args.m_iN1);
    }
    const ITEMID_TYPE id = (ITEMID_TYPE)(rid.GetResIndex());
    CItem *pItem = CItem::CreateTemplate(id);
    if (!pItem)
    {
        return;
    }

    const uint16 iAmountPile = (uint16)(minimum(UINT16_MAX, _iPile));
    if (iAmountPile > 1)
    {
        const CItemBase *pItemDef = pItem->Item_GetDef();
        ASSERT(pItemDef);
        if (pItemDef->IsStackableType())
        {
            SetAmount((uint16)Calc_GetRandVal(iAmountPile));
        }
    }

    pItem->ClrAttr(pItem->m_Attr & (ATTR_OWNED | ATTR_MOVE_ALWAYS));
    pItem->SetDecayTime(g_Cfg.m_iDecay_Item);	// it will decay eventually to be replaced later
    if (IsTrigUsed(TRIGGER_SPAWN))
    {
        CScriptTriggerArgs args;
        args.m_pO1 = pItem;
        if (pSpawnItem->OnTrigger(ITRIG_Spawn, &g_Serv, &args) == TRIGRET_RET_TRUE)
        {
            pItem->Delete();
            return;
        }
    }
    if (pItem->GetTopPoint().IsValidPoint() == false)//Try to placed the item if the @Spawn trigger didn't set a valid P for it.
    {
        const CPointMap& ptSpawn = pSpawnItem->GetTopPoint();
        if (_iMaxDist == 0)
        {
            if (!pItem->MoveTo(ptSpawn))
                goto move_failed;
        }
        else if (!pItem->MoveNear(ptSpawn, (word)(Calc_GetRandVal(_iMaxDist) + 1)))
        {
        move_failed:
            // If this fails, try placing the char ON the spawn
            if (!pItem->MoveTo(ptSpawn))
            {
                DEBUG_ERR(("Spawner UID=0%" PRIx32 " is unable to place an item inside the world, deleted the item.", (dword)pItem->GetUID()));
                pItem->Delete();
                return;
            }
        }
    }
    pItem->Update();
    AddObj(pItem->GetUID());
}


CChar* CCSpawn::GenerateChar(CResourceIDBase rid)
{
    ADDTOCALLSTACK("CCSpawn::GenerateChar(rid)");

    CItem *pSpawnItem = GetLink();
    if (!pSpawnItem->IsTopLevel())
    {
        return nullptr;
    }
    if (IsTrigUsed(TRIGGER_PRESPAWN))
    {
        CScriptTriggerArgs args(rid.GetResIndex());
        if (pSpawnItem->OnTrigger(ITRIG_PreSpawn, &g_Serv, &args) == TRIGRET_RET_TRUE)
        {
            return nullptr;
        }
        rid = CResourceIDBase(RES_CHARDEF, (int)args.m_iN1);
    }

    RES_TYPE iRidType = rid.GetResType();
    iRidType = rid.GetResType();

    if ((iRidType != RES_CHARDEF) && (iRidType != RES_UNKNOWN))
    {
        g_Log.EventError("Spawner UID=0%" PRIx32 " tried to GenerateChar with invalid ResType=%d (ResourceID=0% " PRIx32 ").\n", (dword)pSpawnItem->GetUID(), (int)iRidType, rid.GetPrivateUID() );
        return nullptr;
    }


    CChar *pChar = CChar::CreateBasic( (CREID_TYPE)rid.GetResIndex() );
    if (!pChar)
    {
        return nullptr;
    }

    const CPointMap& pt = pSpawnItem->GetTopPoint();
    pChar->NPC_LoadScript(true);
    pChar->StatFlag_Set(STATF_SPAWNED);

    if (IsTrigUsed(TRIGGER_SPAWN))
    {
        CScriptTriggerArgs args;
        args.m_pO1 = pChar;
        if (pSpawnItem->OnTrigger(ITRIG_Spawn, &g_Serv, &args) == TRIGRET_RET_TRUE)
        {
            pChar->Delete();
            return nullptr;
        }
    }

    // Try placing this char near the spawn
    if (pChar->GetTopPoint().IsValidPoint() == false)// Try to place it only if the @Spawn trigger didn't set it a valid P.
    {
        ushort iPlacingTries = 0;
        while (!pChar->MoveNear(pt, _iMaxDist ? (word)(Calc_GetRandVal(_iMaxDist) + 1) : 1) || pChar->IsStuck(false))
        {
            ++iPlacingTries;
            if (iPlacingTries <= 3)
            {
                continue;
            }

            // If this fails, try placing the char ON the spawn
            if (!pChar->MoveTo(pt))
            {
                DEBUG_ERR(("Spawner UID=0%" PRIx32 " is unable to place a character inside the world, deleted the character.", (dword)pSpawnItem->GetUID()));
                pChar->Delete();
                return nullptr;
            }
            break;
        }
    }

    // Check if the NPC can spawn in this region
    const CRegion *pRegion = pt.GetRegion(REGION_TYPE_AREA);
    if (!pRegion || (pRegion->IsGuarded() && pChar->Noto_IsEvil()))
    {
        g_Log.EventWarn("Spawner UID=0%" PRIx32 " is trying to spawn an evil NPC into a guarded area. Deleting the NPC.\n", (dword)pSpawnItem->GetUID());
        pChar->Delete();
        return nullptr;
    }

    AddObj(pChar->GetUID());
    pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement and attachment to the spawnitem
    pChar->Update();

    pSpawnItem->GetTopSector()->CheckCharComplexity();

    return pChar;
}

CResourceIDBase CCSpawn::GetCharRid()
{
    ADDTOCALLSTACK("CCSpawn::GetCharRid");
    const CItem* pSpawnItem = static_cast<const CItem*>(GetLink());

    CResourceIDBase rid;
    const CResourceDef* pDef = FixDef();
    if (!pDef)
    {
        g_Log.EventError("Bad spawn point (UID=0%" PRIx32 ") is trying to generate a char/spawngroup. Invalid spawn index 0%x (ResourceID=0%" PRIx32 ").\n", (dword)pSpawnItem->GetUID(), _idSpawn.GetResIndex(), _idSpawn.GetPrivateUID());
        AddBadSpawn();
        return rid;
    }

    rid = pDef->GetResourceID();
    RES_TYPE iRidType = rid.GetResType();
    if (iRidType == RES_SPAWN)
    {
        const CRandGroupDef* pSpawnGroup = static_cast<const CRandGroupDef*>(pDef);
        ASSERT(pSpawnGroup);
        size_t i = pSpawnGroup->GetRandMemberIndex();
        if (i != SCONT_BADINDEX)
        {
            rid = pSpawnGroup->GetMemberID(i);
        }
    }
    return rid;
}

void CCSpawn::GenerateChar()
{
    ADDTOCALLSTACK("CCSpawn::GenerateChar");
    GenerateChar(GetCharRid());
}

void CCSpawn::DelObj(const CUID& uid)
{
    ADDTOCALLSTACK("CCSpawn::DelObj");
    if (_fKillingChildren)
    {
        return; // Speeding up the KillChildren proccess and avoiding not needed code called on 'childrens'.
    }

    if (!uid.IsValidUID())
    {
        return;
    }

    if (_uidList.empty())
    {
        return;
    }

    CItem *pSpawnItem = static_cast<CItem*>(GetLink());
    auto itObj = std::find(_uidList.begin(), _uidList.end(), uid);
    if (itObj != _uidList.end())
    {
        CObjBase *pSpawnedObj = uid.ObjFind();
        if (pSpawnedObj && !pSpawnedObj->IsDeleted())
        {
            pSpawnedObj->SetSpawn(nullptr);
            const IT_TYPE iSpawnType = pSpawnItem->GetType();
            if ((iSpawnType == IT_SPAWN_CHAR) || (iSpawnType == IT_SPAWN_CHAMPION))
            {
                CChar *pSpawnedChar = dynamic_cast<CChar*>(pSpawnedObj);
                if (pSpawnedChar)
                    pSpawnedChar->StatFlag_Clear(STATF_SPAWNED);
            }
        }
        _uidList.erase(itObj);
    }
    pSpawnItem->UpdatePropertyFlag();
}

void CCSpawn::AddObj(const CUID& uid)
{
    ADDTOCALLSTACK("CCSpawn::AddObj");
    // NOTE: This function is also called when loading spawn items
    // on server startup. In this case, some objs UID still invalid
    // (not loaded yet) so just proceed without any checks.

    const uint16 uiAmount = GetAmount();
    const uint16 uiMax = maximum(uiAmount, 1);
    CItem *pSpawnItem = static_cast<CItem*>(GetLink());
    if ((_uidList.size() >= uiMax) && !pSpawnItem->IsType(IT_SPAWN_CHAMPION))  // char/item spawns have a limit, champions may spawn a lot of npcs
    {
        return;
    }

    bool fIsSpawnChar = (pSpawnItem->IsType(IT_SPAWN_CHAR) || pSpawnItem->IsType(IT_SPAWN_CHAMPION));

    if (!g_Serv.IsLoading())
    {
        // Only checking UIDs when server is running because some of them may not yet exist when loading worldsave.
        if (!uid.IsValidUID())
        {
            return;
        }

        if (fIsSpawnChar)
        {
            // IT_SPAWN_CHAR and IT_SPAWN_CHAMPION can only spawn NPCs.
            const CChar *pChar = uid.CharFind();
            if (!pChar || !pChar->m_pNPC)
                return;
        }
        else if (pSpawnItem->IsType(IT_SPAWN_ITEM) && !uid.ItemFind())
        {
            // IT_SPAWN_ITEM can only spawn items
            return;
        }

        CObjBase *pSpawnedObj = uid.ObjFind();
        ASSERT(pSpawnedObj);
        CCSpawn *pPrevSpawn = pSpawnedObj->GetSpawn();
        if (pPrevSpawn)
        {
            if (pPrevSpawn == this)	    // obj already linked to this spawn
                return;
            pPrevSpawn->DelObj(uid);    // obj linked to other spawn, remove the link before proceeding
        }

        // Objs are linked to the spawn at each server start and must not be linked from there, since the Obj may not yet exist.
        pSpawnedObj->SetSpawn(this);
        if (fIsSpawnChar)
        {
            CChar *pChar = static_cast<CChar*>(pSpawnedObj);
            ASSERT(pChar->m_pNPC);
            pChar->StatFlag_Set(STATF_SPAWNED);
            pChar->m_ptHome = pSpawnItem->GetTopPoint();
            pChar->m_pNPC->m_Home_Dist_Wander = (word)_iMaxDist;
        }
        pSpawnItem->UpdatePropertyFlag();
    }

    // Done with checks, let's add this.
    _uidList.emplace_back(uid); 
}

CCRET_TYPE CCSpawn::OnTickComponent()
{
    ADDTOCALLSTACK("CCSpawn::OnTickComponent");
    int64 iMinutes;
    CItem *pSpawnItem = GetLink();
    if (_iTimeHi <= 0)
    {
        iMinutes = Calc_GetRandLLVal(30) + 1;
    }
    else
    {
        iMinutes = Calc_GetRandVal2(_iTimeLo, _iTimeHi);
    }

    if (iMinutes <= 0)
    {
        iMinutes = 1;
    }
    pSpawnItem->_SetTimeoutS(iMinutes * 60);	// set time to check again.

    if (GetCurrentSpawned() >= GetAmount())
    {
        return CCRET_TRUE;
    }

    if (pSpawnItem->IsType(IT_SPAWN_CHAR) || pSpawnItem->IsType(IT_SPAWN_CHAMPION))
    {
        GenerateChar();
    }
    else if (pSpawnItem->IsType(IT_SPAWN_ITEM))
    {
        GenerateItem();
    }
    else
    {
        ASSERT(0);  // Should never happen
    }
    return CCRET_TRUE;
}

void CCSpawn::KillChildren()
{
    ADDTOCALLSTACK("CCSpawn::KillChildren");
    if (_uidList.empty())
    {
        return;
    }

    _fKillingChildren = true;
    for (std::vector<CUID>::iterator it = _uidList.begin(), itEnd = _uidList.end(); it != itEnd; ++it)
    {
        CObjBase* pObj = it->ObjFind();
        CChar *pChar = dynamic_cast<CChar*>(pObj);
        if (pChar)
        {
            pChar->SetSpawn(nullptr);   // Just to prevent CObjBase to call DelObj.
            pChar->Delete();
            continue;
        }
        CItem *pItem = dynamic_cast<CItem*>(pObj);
        if (pItem)
        {
            pItem->SetSpawn(nullptr);   // Just to prevent CObjBase to call DelObj.
            pItem->Delete();
            continue;
        }
    }
    _uidList.clear();
    _fKillingChildren = false;
}

const CCharBase *CCSpawn::SetTrackID()
{
    ADDTOCALLSTACK("CCSpawn::SetTrackID");

    CItem *pItem = static_cast<CItem*>(GetLink());
    pItem->SetAttr(ATTR_INVIS);	// Indicate to GM's that it is invis.
    if (pItem->GetHue() == 0)
    {
        pItem->SetHue(HUE_RED_DARK);
    }

    if (!pItem->IsType(IT_SPAWN_CHAR))
    {
        pItem->SetDispID(ITEMID_WorldGem_lg);
        return nullptr;
    }

    const CCharBase *pCharDef = nullptr;
    const CResourceID& rid = _idSpawn;

    if (rid.GetResType() == RES_CHARDEF)
    {
        CREID_TYPE id = (CREID_TYPE)(rid.GetResIndex());
        pCharDef = CCharBase::FindCharBase(id);
    }
    pItem->SetDispID(pCharDef ? pCharDef->m_trackID : ITEMID_TRACK_WISP);	// They must want it to look like this.

    return pCharDef;
}

enum ISPW_TYPE
{
    ISPW_ADDOBJ,
    ISPW_AMOUNT,
    ISPW_COUNT,
    ISPW_MAXDIST,
    ISPW_MORE,
    ISPW_MORE1,
    ISPW_MORE2,
    ISPW_MOREP,
    ISPW_MOREX,
    ISPW_MOREY,
    ISPW_MOREZ,
    ISPW_PILE,
    ISPW_SPAWNID,
    ISPW_TIMEHI,
    ISPW_TIMELO,
    ISPW_QTY
};

lpctstr const CCSpawn::sm_szLoadKeys[ISPW_QTY + 1] =
{
    "ADDOBJ",
    "AMOUNT",
    "COUNT",
    "MAXDIST",
    "MORE",
    "MORE1",    // spawn's id
    "MORE2",    // pile amount (for items)
    "MOREP",    // morex,morey,morez
    "MOREX",    // TimeLo
    "MOREY",    // TimeHi
    "MOREZ",    // MaxDist
    "PILE",
    "SPAWNID",
    "TIMEHI",
    "TIMELO",
    nullptr
};

bool CCSpawn::r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole *pSrc)
{
    ADDTOCALLSTACK("CCSpawn::r_WriteVal");
    UNREFERENCED_PARAMETER(pSrc);
    EXC_TRY("WriteVal");

    int iCmd = FindTableSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iCmd < 0)
    {
        return false;
    }
    CItem *pSpawnItem = GetLink();
    switch (iCmd)
    {
        case ISPW_AMOUNT:
        {
            sVal.FormatU16Val(GetAmount());
            return true;
        }
        case ISPW_COUNT:
        {
            sVal.FormatU16Val(GetCurrentSpawned());
            return true;
        }
        case ISPW_SPAWNID:
        case ISPW_MORE:
        case ISPW_MORE1:
        {
            if (_idSpawn.IsValidResource())
            {
                sVal = g_Cfg.ResourceGetName(_idSpawn);
            }
            else
            {
                sVal.FormatVal(0);
            }
            return true;
        }
        case ISPW_MORE2:
        case ISPW_PILE:
        {
            if (pSpawnItem->GetType() == IT_SPAWN_ITEM)
            {
                sVal.FormatU16Val(_iPile);
            }
            else
            {
                sVal.FormatU16Val(GetCurrentSpawned());
            }
            return true;
        }
        case ISPW_MAXDIST:
        case ISPW_MOREZ:
        {
            sVal.FormatU8Val(_iMaxDist);
            return true;
        }
        case ISPW_TIMELO:
        case ISPW_MOREX:
        {
            sVal.FormatU16Val(_iTimeLo);
            return true;
        }
        case ISPW_TIMEHI:
        case ISPW_MOREY:
        {
            sVal.FormatU16Val(_iTimeHi);
            return true;
        }
        case ISPW_MOREP:
        {
            sVal.Format("%" PRIu16 ",%" PRIu16 ",%" PRIu8, _iTimeLo, _iTimeHi, _iMaxDist);
            return true;
        }
        default:
            return false;    // Match but no code executed.
    }
    EXC_CATCH;
    return false;   // No match, let the code continue.
}

bool CCSpawn::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CCSpawn::r_LoadVal");
    EXC_TRY("LoadVal");

    int iCmd = FindTableSorted(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iCmd < 0)
    {
        return false;
    }
    CItem *pSpawnItem = GetLink();

    switch (iCmd)
    {
        case ISPW_ADDOBJ:
        {
            AddObj(CUID(s.GetArgDWVal()));
            return true;
        }
        case ISPW_AMOUNT:
        {
            SetAmount(s.GetArgU16Val());
            return true;
        }
        case ISPW_SPAWNID:
        case ISPW_MORE:
        case ISPW_MORE1:
        {
            const dword dwPrivateUID = s.GetArgDWVal();
            if (!CUID::IsValidUID(dwPrivateUID))
            {
                return true;
            }
            CResourceIDBase ridArg(dwPrivateUID);    // Not using CResourceID because res_chardef, spawn, itemdef, template do not use the "page" arg
            const int iRidIndex = ridArg.GetResIndex();
            const int iRidType  = ridArg.GetResType();
            switch (pSpawnItem->GetType())
            {
                case IT_SPAWN_CHAR:
                {
                    if ((iRidType == RES_CHARDEF) || (iRidType == RES_SPAWN))
                    {
                        // If i have the ResType probably i passed a Defname
                        _idSpawn = ridArg;
                        break;
                    }
                    // Otherwise i passed a raw number
                    if (iRidIndex < SPAWNTYPE_START)
                    {
                        _idSpawn = CResourceIDBase(RES_CHARDEF, iRidIndex);
                    }
                    else
                    {
                        // it should be a spawn group.
                        CResourceIDBase ridTemp(RES_SPAWN, iRidIndex);
                        CResourceDef *pDef = g_Cfg.ResourceGetDef(ridTemp);
                        if (pDef)
                        {
                            _idSpawn = ridTemp;
                        }
                        else
                        {
                            _idSpawn = CResourceIDBase(RES_CHARDEF, iRidIndex);
                            g_Log.EventDebug("Setting to spawner with UID=0%x SpawnID=0%x being not a SPAWN, but >= SPAWNTYPE_START.\n", (dword)pSpawnItem->GetUID(), iRidIndex);
                            FixDef();
                        }
                    }
                    break;
                }
                case IT_SPAWN_ITEM:
                {
                    if ((iRidType == RES_ITEMDEF) || (iRidType == RES_TEMPLATE))
                    {
                        // If i have the ResType probably i passed a Defname
                        _idSpawn = ridArg;
                        break;
                    }
                    // Otherwise i passed a raw number
                    if (iRidIndex < ITEMID_TEMPLATE)
                    {
                        _idSpawn = CResourceIDBase(RES_ITEMDEF, iRidIndex);   // Ensuring there's no negative value
                    }
                    else
                    {
                        // try a template
                        CResourceIDBase ridTemp(RES_TEMPLATE, iRidIndex);
                        CResourceDef *pDef = g_Cfg.ResourceGetDef(ridTemp);
                        if (pDef)
                        {
                            _idSpawn = ridTemp;
                        }
                        else
                        {
                            _idSpawn = CResourceIDBase(RES_ITEMDEF, iRidIndex);
                            g_Log.EventDebug("Setting to spawner with UID=0%x SpawnID=0%x being not a ITEM, but >= ITEMID_TEMPLATE.\n", (dword)pSpawnItem->GetUID(), iRidIndex);
                            FixDef();
                        }
                    }
                    break;
                }
                default:
                {
                    return false;
                }
            }
            if (!g_Serv.IsLoading())
            {
                FixDef();
                SetTrackID();
                CItem *pLink = GetLink();
                pLink->RemoveFromView();
                pLink->Update();
            }
            return true;
        }
        case ISPW_MORE2:
        case ISPW_PILE:
        {
            if (pSpawnItem->GetType() == IT_SPAWN_ITEM)
            {
                _iPile = s.GetArgU16Val();
                return true;
            }
            return false;   // More2 on char's spawns refers to GetCurrentSpawned() (for backwards) but it should not be modified, for that purpose use AddObj();
        }
        case ISPW_MAXDIST:
        case ISPW_MOREZ:
        {
            _iMaxDist = s.GetArgU8Val();
            return true;
        }
        case ISPW_TIMELO:
        case ISPW_MOREX:
        {
            _iTimeLo = s.GetArgU16Val();
            return true;
        }
        case ISPW_TIMEHI:
        case ISPW_MOREY:
        {
            _iTimeHi = s.GetArgU16Val();
            return true;
        }
        case ISPW_MOREP:
        {
            tchar *pszTemp = Str_GetTemp();
            Str_CopyLimitNull(pszTemp, s.GetArgStr(), STR_TEMPLENGTH);
            GETNONWHITESPACE(pszTemp);
            size_t iArgs = 0;
            if (IsDigit(pszTemp[0]) || pszTemp[0] == '-')
            {
                tchar * ppVal[3];
                iArgs = Str_ParseCmds(pszTemp, ppVal, CountOf(ppVal), " ,\t");
                switch (iArgs)
                {
                    case 3: // m_z
                        _iMaxDist = (uint8)(atoi(ppVal[2]));
                    case 2: // m_y
                        _iTimeHi = (uint16)(atoi(ppVal[1]));
                    case 1: // m_x
                        _iTimeLo = (uint16)(atoi(ppVal[0]));
                        break;
                    default:
                    case 0:
                        return false;
                }
            }
            return true;
        }
        default:
            return false;
    }
    EXC_CATCH;
    return false;
}

void CCSpawn::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCSpawn:r_Write");
    EXC_TRY("Write");

    CItem *pItem = GetLink();
    if (_fIsChampion == false)
    {
        if (!FixDef())
        {
            g_Log.EventWarn("Saving a bad spawn point (UID=0%" PRIx32 "). Invalid spawn index 0%x (ResourceID=0%" PRIx32 ").\n", (dword)pItem->GetUID(), _idSpawn.GetResIndex(), _idSpawn.GetPrivateUID());
        }
        if (_idSpawn.IsValidResource())
        {
            s.WriteKeyStr("SPAWNID", g_Cfg.ResourceGetName(_idSpawn));
        }
    }

    uint16 uiAmount = GetAmount();
    if (uiAmount != 1)
    {
        s.WriteKeyVal("AMOUNT", uiAmount);
    }
    uint16 uiPile = GetPile();
    if ((uiPile > 1) && (pItem->GetType() == IT_SPAWN_ITEM))
    {
        s.WriteKeyVal("PILE", uiPile);
    }
    s.WriteKeyVal("TIMELO", GetTimeLo());
    s.WriteKeyVal("TIMEHI", GetTimeHi());
    s.WriteKeyVal("MAXDIST", GetDistanceMax());

    if (GetCurrentSpawned() <= 0)
    {
        return;
    }

    for (const CUID& uid : _uidList)
    {
        if (!uid.IsValidUID())
            continue;
        const CObjBase *pObj = uid.ObjFind();
        if (pObj)
        {
            s.WriteKeyHex("ADDOBJ", uid.GetObjUID());
        }
    }

    EXC_CATCH;
}

enum SPAWN_REF
{
    ISPR_AT,
    ISPR_QTY
};

lpctstr const CCSpawn::sm_szRefKeys[ISPR_QTY + 1]
{
    "AT",
    nullptr
};

bool CCSpawn::r_GetRef(lpctstr & ptcKey, CScriptObj *& pRef)
{
    ADDTOCALLSTACK("CCSpawn::r_GetRef");
    int iCmd = FindTableSorted(ptcKey, sm_szRefKeys, CountOf(sm_szRefKeys) - 1);

    if (iCmd < 0)
    {
        return false;
    }

    CItem *pItem = static_cast<CItem*>(GetLink());

    ptcKey += strlen(sm_szRefKeys[iCmd]);
    SKIP_SEPARATORS(ptcKey);

    switch (iCmd)
    {
        case ISPR_AT:
        {
            int objIndex = Exp_GetVal(ptcKey);
            if (objIndex < 0)
                return false;
            SKIP_SEPARATORS(ptcKey);
            if (objIndex >= GetCurrentSpawned())
            {
                return false;
            }
            if (pItem->IsType(IT_SPAWN_ITEM))
            {
                CItem *pSpawnedItem = _uidList[objIndex].ItemFind();
                if (pSpawnedItem)
                {
                    pRef = pSpawnedItem;
                    return true;
                }
            }
            else if (pItem->IsType(IT_SPAWN_CHAR) || pItem->IsType(IT_SPAWN_CHAMPION))
            {
                CChar *pSpawnedChar = _uidList[objIndex].CharFind();
                if (pSpawnedChar)
                {
                    pRef = pSpawnedChar;
                    return true;
                }
            }
        }
        default:
            break;
    }
    return false;
}

enum SPAWN_VERB
{
    ISPV_DELOBJ,
    ISPV_RESET,
    ISPV_START,
    ISPV_STOP,
    ISPV_QTY
};

lpctstr const CCSpawn::sm_szVerbKeys[ISPV_QTY + 1]
{
    "DELOBJ",
    "RESET",
    "START",
    "STOP",
    nullptr
};

bool CCSpawn::r_Verb(CScript & s, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CCSpawn::r_Verb");
    UNREFERENCED_PARAMETER(pSrc);
    int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys) - 1);
    if (iCmd < 0)
    {
        return false;
    }

    CItem *pItem = static_cast<CItem*>(GetLink());
    switch (iCmd)
    {
        case ISPV_DELOBJ:
        {
            DelObj(CUID(s.GetArgDWVal()));
            return true;
        }
        case ISPV_RESET:
            KillChildren();
            OnTickComponent();
            return true;
        case ISPV_START:
            pItem->SetTimeout(0);
            return true;
        case ISPV_STOP:
            KillChildren();
            pItem->SetTimeout(-1);
            return true;
        default:
            break;
    }
    return false;
}

void CCSpawn::Copy(const CComponent * target)
{
    ADDTOCALLSTACK("CCSpawn::Copy");
    const CCSpawn *pTarget = static_cast<const CCSpawn*>(target);
    if (!pTarget)
    {
        return;
    }
    _iPile = pTarget->GetPile();
    _iAmount = pTarget->GetAmount();
    _iTimeLo = pTarget->GetTimeLo();
    _iTimeHi = pTarget->GetTimeHi();
    _iMaxDist = pTarget->GetDistanceMax();
    _idSpawn = pTarget->GetSpawnID();

    // Not copying created objects.
}
