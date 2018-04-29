
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../../common/CObjBaseTemplate.h"
#include "../../common/CUIDExtra.h"
#include "../chars/CChar.h"
#include "../chars/CCharNPC.h"
#include "../CObjBase.h"
#include "../CContainer.h"
#include "../CRegion.h"
#include "CSpawn.h"

/////////////////////////////////////////////////////////////////////////////

uint16 CSpawn::GetAmount()
{
	//ADDTOCALLSTACK("CSpawn::GetAmount");
	return m_iAmount;
}

uint16 CSpawn::GetCurrentSpawned()
{
    return (uint16)m_objs.size();
}

uint16 CSpawn::GetPile()
{
    return m_pile;
}

word CSpawn::GetTimeLo()
{
    return m_TimeLoMin;
}

word CSpawn::GetTimeHi()
{
    return m_TimeHiMin;
}

byte CSpawn::GetDistMax()
{
    return m_DistMax;
}

CItem * CSpawn::GetSpawnItem()
{
    ADDTOCALLSTACK("CSpawn::SetAmount");
    const CItem *pLink = static_cast<const CItem*>(GetLink());
    CItem *pItem = const_cast<CItem*>(pLink);
    return pItem;
}

CResourceIDBase CSpawn::GetSpawnID()
{
    return _SpawnID;
}

void CSpawn::SetAmount(uint16 iAmount)
{
	ADDTOCALLSTACK("CSpawn::SetAmount");
	m_iAmount = iAmount;
}

inline CCharBase *CSpawn::TryChar(CREID_TYPE &id)
{
	ADDTOCALLSTACK("CSpawn:TryChar");
	CCharBase *pCharDef = CCharBase::FindCharBase(id);
	if ( pCharDef )
	{
        _SpawnID = CResourceID(RES_CHARDEF, id);
		return pCharDef;
	}
	return NULL;
}

inline CItemBase *CSpawn::TryItem(ITEMID_TYPE &id)
{
	ADDTOCALLSTACK("CSpawn:TryItem");
	CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( pItemDef )
	{
		
        _SpawnID = CResourceID(RES_ITEMDEF, id);
		return pItemDef;
	}
	return NULL;
}

CResourceDef *CSpawn::FixDef()
{
	ADDTOCALLSTACK("CSpawn:FixDef");

    const CItem *pItem = static_cast<const CItem*>(GetLink());
	if ( _SpawnID.GetResType() != RES_UNKNOWN )
		return static_cast<CResourceDef *>(g_Cfg.ResourceGetDef(_SpawnID));

	// No type info here !?
	if (pItem->IsType(IT_SPAWN_CHAR) )
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(_SpawnID.GetResIndex());
		if ( id < SPAWNTYPE_START )
			return TryChar(id);

		// try a spawn group.
		CResourceIDBase rid = CResourceID(RES_SPAWN, id);
		CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
		if ( pDef )
		{
            _SpawnID = rid;
			return pDef;
		}
		return TryChar(id);
	}
	else
	{
		ITEMID_TYPE id = (ITEMID_TYPE)(_SpawnID.GetResIndex());
		if ( id < ITEMID_TEMPLATE )
			return TryItem(id);

		// try a template.
		CResourceIDBase rid = CResourceID(RES_TEMPLATE, id);
		CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
		if ( pDef )
		{
            _SpawnID = rid;
			return pDef;
		}
		return TryItem(id);
	}
}

int CSpawn::GetName(tchar *pszOut) const
{
	ADDTOCALLSTACK("CSpawn:GetName");
	lpctstr pszName = NULL;
	CResourceDef *pDef = g_Cfg.ResourceGetDef(_SpawnID);
	if ( pDef != NULL )
		pszName = pDef->GetName();
	if ( pDef == NULL || pszName == NULL || pszName[0] == '\0' )
		pszName = g_Cfg.ResourceGetName(_SpawnID);

	return sprintf(pszOut, " (%s)", pszName);
}

CSpawn::CSpawn(const CObjBase *pLink) : CComponent(COMP_SPAWN, pLink)
{
	ADDTOCALLSTACK("CSpawn::CSpawn");
	m_iAmount = 1;
    m_pile = 1;
}

CSpawn::~CSpawn()
{
	ADDTOCALLSTACK("CSpawn::~CSpawn");
}

void CSpawn::GenerateItem(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CSpawn:GenerateItem");

	CResourceIDBase rid = pDef->GetResourceID();
	ITEMID_TYPE id = (ITEMID_TYPE)(rid.GetResIndex());

    CItem *pSpawnItem = GetSpawnItem();
	CItemContainer *pCont = dynamic_cast<CItemContainer *>(pSpawnItem->GetParent());
	byte iCount = pCont ? (uchar)(pCont->ContentCount(rid)) : GetCurrentSpawned();

	if ( iCount >= GetAmount() )
		return;

	CItem *pItem = pSpawnItem->CreateTemplate(id);
	if ( pItem->CreateTemplate(id) == NULL )
		return;

    uint16 iAmountPile = (uint16)(minimum(UINT16_MAX,m_pile));
	if ( iAmountPile > 1 )
	{
		CItemBase *pItemDef = pItem->Item_GetDef();
		ASSERT(pItemDef);
		if ( pItemDef->IsStackableType() )
			SetAmount((uint16)Calc_GetRandVal(iAmountPile));
	}

	const_cast<CItem*>(pItem)->ClrAttr(pItem->m_Attr & (ATTR_OWNED | ATTR_MOVE_ALWAYS));
	pItem->SetDecayTime(g_Cfg.m_iDecay_Item);	// it will decay eventually to be replaced later
	pItem->MoveNearObj(pItem, m_DistMax);
	AddObj(pItem->GetUID());
}

void CSpawn::GenerateChar(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CSpawn:GenerateChar");
    const CItem *pItem = static_cast<const CItem*>(GetLink());
	if ( !pItem->IsTopLevel() )
		return;

	CResourceIDBase rid = pDef->GetResourceID();
	if ( rid.GetResType() == RES_SPAWN )
	{
		const CSRandGroupDef *pSpawnGroup = static_cast<const CSRandGroupDef *>(pDef);
		ASSERT(pSpawnGroup);
		size_t i = pSpawnGroup->GetRandMemberIndex();
		if ( i != pSpawnGroup->BadMemberIndex() )
			rid = pSpawnGroup->GetMemberID(i);
	}

	if ( (rid.GetResType() != RES_CHARDEF) && (rid.GetResType() != RES_UNKNOWN) )
		return;

	CChar *pChar = CChar::CreateBasic(static_cast<CREID_TYPE>(rid.GetResIndex()));
	if ( !pChar )
		return;

	CPointMap pt = pItem->GetTopPoint();
	pChar->NPC_LoadScript(true);
	pChar->StatFlag_Set(STATF_SPAWNED);
	word iDistMax = m_DistMax;
	// Try placing this char near the spawn
	if ( !pChar->MoveNearObj(pItem, iDistMax ? (word)(Calc_GetRandVal(iDistMax) + 1) : 1) )
	{
		// If this fails, try placing the char ON the spawn
		if (!pChar->MoveTo(pt))
		{
			DEBUG_ERR(("Spawner UID:0%" PRIx32 " is unable to place a character inside the world, deleted the character", (dword)pItem->GetUID()));
			pChar->Delete();
			return;
		}
	}

	// Check if the NPC can spawn in this region
	CRegion *pRegion = pt.GetRegion(REGION_TYPE_AREA);
	if ( !pRegion || (pRegion->IsGuarded() && pChar->Noto_IsEvil()) )
	{
		pChar->Delete();
		return;
	}

	AddObj(pChar->GetUID());
	pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement and attachment to the spawnitem
	pChar->Update();

	size_t iCount = pItem->GetTopSector()->GetCharComplexity();
	if ( iCount > g_Cfg.m_iMaxCharComplexity )
		g_Log.Event(LOGL_WARN, "%" PRIuSIZE_T " chars at %s. Sector too complex!\n", iCount, pItem->GetTopSector()->GetBasePoint().WriteUsed());
}

void CSpawn::DelObj(CUID uid)
{
	ADDTOCALLSTACK("CSpawn:DelObj");
	if ( !uid.IsValidUID() )
		return;

	for ( std::vector<CUID>::iterator it = m_objs.begin(); it != m_objs.end(); ++it )
	{
        if (*it == uid)
        {

            CObjBase *pObj = uid.ObjFind();
            pObj->SetSpawn(nullptr);

            if (GetType() == IT_SPAWN_CHAR)
            {
                CChar *pChar = uid.CharFind();
                if (pChar)
                {
                    pChar->StatFlag_Clear(STATF_SPAWNED);
                }
            }

            m_objs.erase(it);
            break;
        }
	}
    const CItem *pLink = static_cast<const CItem*>(GetLink());
    CItem *pItem = const_cast<CItem*>(pLink);
    pItem->ResendTooltip();
}

void CSpawn::AddObj(CUID uid)
{
	ADDTOCALLSTACK("CSpawn:AddObj");
	// NOTE: This function is also called when loading spawn items
	// on server startup. In this case, some objs UID still invalid
	// (not loaded yet) so just proceed without any checks.

    const CItem *pItem = static_cast<const CItem*>(GetLink());
    bool fIsSpawnChar = (pItem->IsType(IT_SPAWN_CHAR) || pItem->IsType(IT_SPAWN_CHAMPION));
    if (!uid.IsValidUID())
    {
        return;
    }
	if ( !g_Serv.IsLoading() )
	{
		if (fIsSpawnChar)				// IT_SPAWN_CHAR and IT_SPAWN_CHAMPION can only spawn NPCs.
		{
			CChar *pChar = uid.CharFind();
			if ( !pChar || !pChar->m_pNPC )
				return;
		}
        else if (pItem->IsType(IT_SPAWN_ITEM) && !uid.ItemFind())		// IT_SPAWN_ITEM can only spawn items
        {
            return;
        }

		CSpawn *pPrevSpawn = static_cast<CSpawn*>(uid.ObjFind()->GetComponent(COMP_SPAWN));
		if ( pPrevSpawn )
		{
			if ( pPrevSpawn == this )		// obj already linked to this spawn
				return;
			pPrevSpawn->DelObj(uid);		// obj linked to other spawn, remove the link before proceed
		}
	}

	uint16 iMax = maximum(GetAmount(), 1);
    if (iMax <= m_objs.size())
    {
        g_Log.EventError("Trying to add more Objects than the maximum defined of '%d'",iMax);
        return;
    }
    m_objs.emplace_back(uid);

	if ( !g_Serv.IsLoading() )  // Objs are linked to the spawn at each server start and must not be linked from here, since the Obj may not yet exist.
	{
		uid.ObjFind()->SetSpawn(this);
		if (fIsSpawnChar)
		{
			CChar *pChar = uid.CharFind();
			ASSERT(pChar->m_pNPC);
			pChar->StatFlag_Set(STATF_SPAWNED);
			pChar->m_ptHome = pItem->GetTopPoint();
			pChar->m_pNPC->m_Home_Dist_Wander = (word)(m_DistMax);
		}
	}
	if ( !g_Serv.IsLoading() )
        GetSpawnItem()->ResendTooltip();
}

void CSpawn::OnTick(bool fExec)
{
	ADDTOCALLSTACK("CSpawn:OnTick");

	int64 iMinutes;
    CItem *pItem = GetSpawnItem();

	if ( m_TimeHiMin <= 0 )
		iMinutes = Calc_GetRandLLVal(30) + 1;
	else
		iMinutes = minimum(m_TimeHiMin, m_TimeLoMin) + Calc_GetRandLLVal(abs(m_TimeHiMin - m_TimeLoMin));

	if ( iMinutes <= 0 )
		iMinutes = 1;

	if ( !fExec || pItem->IsTimerExpired() )
        pItem->SetTimeout(iMinutes * 60 * TICK_PER_SEC);	// set time to check again.

	if ( !fExec || GetCurrentSpawned() >= GetAmount() )
		return;

	CResourceDef *pDef = FixDef();
	if ( !pDef )
	{
		CResourceIDBase rid = _SpawnID;
		g_Log.EventError("Bad Spawn point uid=0%x. Invalid id=%s\n", (dword)pItem->GetUID(), g_Cfg.ResourceGetName(rid));
		return;
	}

	if (pItem->IsType(IT_SPAWN_ITEM) )
		GenerateItem(pDef);
	else if (pItem->IsType(IT_SPAWN_CHAR) || pItem->IsType(IT_SPAWN_CHAMPION))
		GenerateChar(pDef);
}

void CSpawn::KillChildren()
{
	ADDTOCALLSTACK("CSpawn:KillChildren");
	if (m_objs.size() <= 0 )
		return;

    m_objs.clear();

	OnTick(false);
}

CCharBase *CSpawn::SetTrackID()
{
	ADDTOCALLSTACK("CSpawn:SetTrackID");

    CItem *pItem = GetSpawnItem();
	pItem->SetAttr(ATTR_INVIS);	// Indicate to GM's that it is invis.
	if (pItem->GetHue() == 0)
        pItem->SetHue(HUE_RED_DARK);

	if ( !pItem->IsType(IT_SPAWN_CHAR) )
	{
        pItem->SetDispID(ITEMID_WorldGem_lg);
		return NULL;
	}

	CCharBase *pCharDef = NULL;
	CResourceIDBase rid = _SpawnID;

	if ( rid.GetResType() == RES_CHARDEF )
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(rid.GetResIndex());
		pCharDef = CCharBase::FindCharBase(id);
	}
    pItem->SetDispID(pCharDef ? pCharDef->m_trackID : ITEMID_TRACK_WISP);	// They must want it to look like this.

	return pCharDef;
}

enum ISPW_TYPE
{
	ISPW_ADDOBJ,
	ISPW_AMOUNT,
	ISPW_AT,
	ISPW_COUNT,
	ISPW_DELOBJ,
	ISPW_RESET,
	ISPW_START,
	ISPW_STOP,
	ISPW_QTY
};

lpctstr const CSpawn::sm_szLoadKeys[ISPW_QTY + 1] =
{
	"ADDOBJ",
	"AMOUNT",
	"AT",
	"COUNT",
	"DELOBJ",
	"RESET",
	"START",
	"STOP",
	NULL
};

bool CSpawn::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSpawn:r_WriteVal");
	EXC_TRY("WriteVal");

    int iCmd = FindTableSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    switch (iCmd)
    {
        case ISPW_AMOUNT:
        {
            sVal.FormatVal(GetAmount());
            return true;
        }
        case ISPW_AT:
        {
            pszKey += 3;
            int objIndex = Exp_GetVal(pszKey);
            const CItem *pLink = static_cast<const CItem*>(GetLink());
            CItem *pItem = const_cast<CItem*>(pLink);
            if (pItem->IsType(IT_SPAWN_ITEM))
            {
                CItem *pSpawnedItem = m_objs[objIndex].ItemFind();
                if (pSpawnedItem)
                {
                    return pSpawnedItem->r_WriteVal(pszKey, sVal, pSrc);
                }
            }
            else if (pItem->IsType(IT_SPAWN_CHAR) || pItem->IsType(IT_SPAWN_CHAMPION))
            {
                CChar *pSpawnedChar = m_objs[objIndex].CharFind();
                if (pSpawnedChar)
                {
                    return pSpawnedChar->r_WriteVal(pszKey, sVal, pSrc);
                }
            }
            return true;
        }
        case ISPW_COUNT:
        {
            sVal.FormatVal(GetCurrentSpawned());
            return true;
        }
        default:
            return true;    // Match but no code executed.
            break;
    }
	EXC_CATCH;
	return false;   // No match, let the code continue.
}

bool CSpawn::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CSpawn:r_LoadVal");
	EXC_TRY("LoadVal");

	int iCmd = FindTableSorted(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
	if ( iCmd < 0 )
		return false;

	switch ( iCmd )
	{
		case ISPW_ADDOBJ:
		{
			AddObj(static_cast<CUID>(s.GetArgVal()));
			return true;
		}
		case ISPW_AMOUNT:
		{
			SetAmount((byte)(s.GetArgVal()));
			return true;
		}
		case ISPW_DELOBJ:
		{
			DelObj(static_cast<CUID>(s.GetArgVal()));
			return true;
		}
		case ISPW_RESET:
			KillChildren();
			return true;
		case ISPW_START:
            GetSpawnItem()->SetTimeout(0);
			return true;
		case ISPW_STOP:
			KillChildren();
            GetSpawnItem()->SetTimeout(-1);
			return true;
		default:
            return true;
			break;
	}
	EXC_CATCH;
	return false;
}

void CSpawn::r_Write(CScript & s)
{
	ADDTOCALLSTACK("CSpawn:r_Write");
	EXC_TRY("Write");
    //GetSpawnItem()->r_Write(s);

	if ( GetAmount() != 1 )
		s.WriteKeyVal("AMOUNT", GetAmount());

	word iTotal = GetCurrentSpawned();
	if ( iTotal <= 0 )
		return;

	for ( std::vector<CUID>::iterator it = m_objs.begin(); it != m_objs.end(); ++it )
	{
		if ( !it->IsValidUID() )
			continue;
		CObjBase *pObj = m_objs[*it].ObjFind();
        if (pObj)
        {
            s.WriteKeyHex("ADDOBJ", pObj->GetUID());
        }
	}

	EXC_CATCH;
}

bool CSpawn::r_GetRef(LPCTSTR & pszKey, CScriptObj *& pRef)
{
    UNREFERENCED_PARAMETER(pszKey);
    UNREFERENCED_PARAMETER(pRef);
    return false;
}

bool CSpawn::r_Verb(CScript & s, CTextConsole * pSrc)
{
    UNREFERENCED_PARAMETER(s);
    UNREFERENCED_PARAMETER(pSrc);
    return false;
}
