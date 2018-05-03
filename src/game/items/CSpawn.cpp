
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../../common/CObjBaseTemplate.h"
#include "../../common/CUIDExtra.h"
#include "../chars/CChar.h"
#include "../chars/CCharNPC.h"
#include "../CObjBase.h"
#include "../CContainer.h"
#include "../CRegion.h"
#include "../CChampion.h"
#include "CSpawn.h"

/////////////////////////////////////////////////////////////////////////////


CSpawn::CSpawn(const CObjBase *pLink) : CComponent(COMP_SPAWN, pLink)
{
    ADDTOCALLSTACK("CSpawn::CSpawn");
    _iAmount = 1;
    _iPile = 1;
    _iMaxDist = 15;
    _iTimeLo = 15;
    _iTimeHi = 30;
    _idSpawn.InitUID();
    _pLink = static_cast<CItem*>(const_cast<CObjBase*>(pLink));
    _uidList.clear();
}

CSpawn::~CSpawn()
{
}

word CSpawn::GetAmount()
{
	//ADDTOCALLSTACK_INTENSIVE("CSpawn::GetAmount");
	return _iAmount;
}

word CSpawn::GetCurrentSpawned()
{
    return (uint16)_uidList.size();
}

word CSpawn::GetPile()
{
    return _iPile;
}

uint16 CSpawn::GetTimeLo()
{
    return _iTimeLo;
}

uint16 CSpawn::GetTimeHi()
{
    return _iTimeHi;
}

uint8 CSpawn::GetMaxDist()
{
    return _iMaxDist;
}
/*
CItem * CSpawn::GetSpawnItem()
{
    ADDTOCALLSTACK("CSpawn::SetAmount");
    const CItem *pLink = static_cast<const CItem*>(GetLink());
    CItem *pItem = const_cast<CItem*>(pLink);
    return pItem;
}*/

CResourceIDBase CSpawn::GetSpawnID()
{
    return _idSpawn;
}

void CSpawn::SetAmount(word iAmount)
{
	ADDTOCALLSTACK("CSpawn::SetAmount");
	_iAmount = iAmount;
}

CCharBase *CSpawn::TryChar(CREID_TYPE &id)
{
	ADDTOCALLSTACK("CSpawn:TryChar");
	CCharBase *pCharDef = CCharBase::FindCharBase(id);
	if ( pCharDef )
	{
        _idSpawn = CResourceID(RES_CHARDEF, id);
		return pCharDef;
	}
	return NULL;
}

CItemBase *CSpawn::TryItem(ITEMID_TYPE &id)
{
	ADDTOCALLSTACK("CSpawn:TryItem");
	CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( pItemDef )
	{
        _idSpawn = CResourceID(RES_ITEMDEF, id);
		return pItemDef;
	}
	return NULL;
}

CResourceDef *CSpawn::FixDef()
{
	ADDTOCALLSTACK("CSpawn:FixDef");

    const CItem *pItem = GetLink();
    if (_idSpawn.GetResType() != RES_UNKNOWN)
    {
        return g_Cfg.ResourceGetDef(_idSpawn);
    }

	// No type info here !?
	if (pItem->IsType(IT_SPAWN_CHAR) )
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(_idSpawn.GetResIndex());
		if ( id < SPAWNTYPE_START )
			return TryChar(id);

		// try a spawn group.
		CResourceIDBase rid = CResourceID(RES_SPAWN, id);
		CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
		if ( pDef )
		{
            _idSpawn = rid;
			return pDef;
		}
		return TryChar(id);
	}
	else
	{
		ITEMID_TYPE id = (ITEMID_TYPE)(_idSpawn.GetResIndex());
		if ( id < ITEMID_TEMPLATE )
			return TryItem(id);

		// try a template.
		CResourceIDBase rid = CResourceID(RES_TEMPLATE, id);
		CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
		if ( pDef )
		{
            _idSpawn = rid;
			return pDef;
		}
		return TryItem(id);
	}
}

uint CSpawn::WriteName(tchar *pszOut) const
{
	ADDTOCALLSTACK("CSpawn::GetName");
	lpctstr pszName = NULL;
	CResourceDef *pDef = g_Cfg.ResourceGetDef(_idSpawn);
	if ( pDef != NULL )
		pszName = pDef->GetName();
	if ( pDef == NULL || pszName == NULL || pszName[0] == '\0' )
		pszName = g_Cfg.ResourceGetName(_idSpawn);

	return sprintf(pszOut, " (%s)", pszName);
}

CItem * CSpawn::GetLink()
{
    return _pLink;
}

void CSpawn::Delete(bool fForce)
{
    ADDTOCALLSTACK("CSpawn::Delete");
    UNREFERENCED_PARAMETER(fForce);
    KillChildren();
}

void CSpawn::GenerateItem(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CSpawn::GenerateItem");

	CResourceIDBase rid = pDef->GetResourceID();
	ITEMID_TYPE id = (ITEMID_TYPE)(rid.GetResIndex());

    CItem *pSpawnItem = GetLink();
	CItemContainer *pCont = dynamic_cast<CItemContainer *>(pSpawnItem->GetParent());
	uchar iCount = pCont ? ((uchar)pCont->ContentCount(rid)) : (uchar)GetCurrentSpawned();

	if ( iCount >= GetAmount() )
		return;

	CItem *pItem = pSpawnItem->CreateTemplate(id);
	if ( pItem->CreateTemplate(id) == NULL )
		return;

    uint16 iAmountPile = (uint16)(minimum(UINT16_MAX,_iPile));
	if ( iAmountPile > 1 )
	{
		CItemBase *pItemDef = pItem->Item_GetDef();
		ASSERT(pItemDef);
		if ( pItemDef->IsStackableType() )
			SetAmount((word)Calc_GetRandVal(iAmountPile));
	}

	const_cast<CItem*>(pItem)->ClrAttr(pItem->m_Attr & (ATTR_OWNED | ATTR_MOVE_ALWAYS));
	pItem->SetDecayTime(g_Cfg.m_iDecay_Item);	// it will decay eventually to be replaced later
	pItem->MoveNearObj(pItem, _iMaxDist);
	AddObj(pItem->GetUID());
}

void CSpawn::GenerateChar(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CSpawn::GenerateChar");
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
	// Try placing this char near the spawn
	if ( !pChar->MoveNearObj(pItem, _iMaxDist ? (word)(Calc_GetRandVal(_iMaxDist) + 1) : 1), true )
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
	ADDTOCALLSTACK("CSpawn::DelObj");
	if ( !uid.IsValidUID() )
		return;

    if (_uidList.empty())
    {
        return;
    }

	for ( std::vector<CUID>::iterator it = _uidList.begin(); it != _uidList.end(); ++it )
	{
        if (*it == uid)
        {
            CObjBase *pObj = uid.ObjFind();
            if (!pObj->IsDeleted())
            {
                if (pObj)
                {
                    pObj->SetSpawn(nullptr);
                }

                if ((_pLink->GetType() == IT_SPAWN_CHAR) || (_pLink->GetType() == IT_SPAWN_CHAMPION))
                {
                    CChar *pChar = uid.CharFind();
                    if (pChar)
                        pChar->StatFlag_Clear(STATF_SPAWNED);
                }
            }
            _uidList.erase(it);
            break;
        }
	}
    GetLink()->ResendTooltip();
}

void CSpawn::AddObj(CUID uid)
{
	ADDTOCALLSTACK("CSpawn::AddObj");
	// NOTE: This function is also called when loading spawn items
	// on server startup. In this case, some objs UID still invalid
	// (not loaded yet) so just proceed without any checks.

    uint16 iMax = maximum(GetAmount(), 1);
    if (_uidList.size() >= iMax && GetLink()->IsType(IT_SPAWN_CHAR))  // char spawns have a limit, champions may spawn a lot of npcs
    {
        return;
    }

    bool fIsSpawnChar = (GetLink()->IsType(IT_SPAWN_CHAR) || GetLink()->IsType(IT_SPAWN_CHAMPION));

	if ( !g_Serv.IsLoading() )
	{
        if (!uid.IsValidUID())  // Only checking UIDs when server is running because some uids may no yet exist when loading worldsave.
        {
            return;
        }
		if (fIsSpawnChar)				// IT_SPAWN_CHAR and IT_SPAWN_CHAMPION can only spawn NPCs.
		{
			CChar *pChar = uid.CharFind();
			if ( !pChar || !pChar->m_pNPC )
				return;
		}
        else if (GetLink()->IsType(IT_SPAWN_ITEM) && !uid.ItemFind())		// IT_SPAWN_ITEM can only spawn items
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
    _uidList.emplace_back(uid);

	if ( !g_Serv.IsLoading() )  // Objs are linked to the spawn at each server start and must not be linked from here, since the Obj may not yet exist.
	{
		uid.ObjFind()->SetSpawn(this);
		if (fIsSpawnChar)
		{
			CChar *pChar = uid.CharFind();
			ASSERT(pChar->m_pNPC);
			pChar->StatFlag_Set(STATF_SPAWNED);
			pChar->m_ptHome = GetLink()->GetTopPoint();
			pChar->m_pNPC->m_Home_Dist_Wander = (word)_iMaxDist;
		}
	}
	if ( !g_Serv.IsLoading() )
        GetLink()->ResendTooltip();
}

void CSpawn::OnTick(bool fExec)
{
    ADDTOCALLSTACK("CSpawn::OnTick");
    ASSERT(GetLink());

    int64 iMinutes;

    if (_iTimeHi <= 0)
        iMinutes = Calc_GetRandLLVal(30) + 1;
    else
        iMinutes = minimum(_iTimeHi, _iTimeLo) + Calc_GetRandVal(abs(_iTimeHi - _iTimeLo));

    if (iMinutes <= 0)
        iMinutes = 1;
    if (!fExec)
    {
        if (GetLink()->IsTimerExpired())
        {
            GetLink()->SetTimeout(iMinutes * 60 * TICK_PER_SEC);	// set time to check again.
        }

        if (GetCurrentSpawned() >= GetAmount())
        {
            return;
        }
    }

    CResourceDef *pDef = FixDef();
    if (!pDef)
    {
        g_Log.EventError("Bad Spawn point uid=0%08x. Invalid id=%s %s\n", GetLink() ? (dword)GetLink()->GetUID() : 0, g_Cfg.ResourceGetName(_idSpawn), !GetLink() ? "PREMIO" : "");
        return;
    }

    if (GetLink()->IsType(IT_SPAWN_CHAR))
    {
        GenerateChar(pDef);
    }
    else if (GetLink()->IsType(IT_SPAWN_ITEM))
    {
        GenerateItem(pDef);
    }
}

void CSpawn::KillChildren()
{
	ADDTOCALLSTACK("CSpawn::KillChildren");
	if (_uidList.empty())
		return;

    for (std::vector<CUID>::iterator it = _uidList.begin(); it != _uidList.end(); ++it)
    {
        CChar *pChar = it->CharFind();
        if (pChar)
        {
            pChar->SetSpawn(nullptr);   // Just to prevent CObjBase to call DelObj.
            pChar->Delete();
            continue;
        }
        CItem *pItem = it->ItemFind();
        if (pItem)
        {
            pItem->SetSpawn(nullptr);   // Just to prevent CObjBase to call DelObj.
            pItem->Delete();
            continue;
        }
    }
    _uidList.clear();
}

CCharBase *CSpawn::SetTrackID()
{
	ADDTOCALLSTACK("CSpawn::SetTrackID");

    GetLink()->SetAttr(ATTR_INVIS);	// Indicate to GM's that it is invis.
	if (GetLink()->GetHue() == 0)
        GetLink()->SetHue(HUE_RED_DARK);

	if ( !GetLink()->IsType(IT_SPAWN_CHAR) )
	{
        GetLink()->SetDispID(ITEMID_WorldGem_lg);
		return NULL;
	}

	CCharBase *pCharDef = NULL;
	CResourceIDBase rid = _idSpawn;

	if ( rid.GetResType() == RES_CHARDEF )
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(rid.GetResIndex());
		pCharDef = CCharBase::FindCharBase(id);
	}
    GetLink()->SetDispID(pCharDef ? pCharDef->m_trackID : ITEMID_TRACK_WISP);	// They must want it to look like this.

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

lpctstr const CSpawn::sm_szLoadKeys[ISPW_QTY + 1] =
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
	NULL
};

bool CSpawn::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CSpawn::r_WriteVal");
    UNREFERENCED_PARAMETER(pSrc);
	EXC_TRY("WriteVal");

    int iCmd = FindTableSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    switch (iCmd)
    {
        case ISPW_AMOUNT:
        {
            sVal.FormatVal(GetAmount());
            return true;
        }
        case ISPW_COUNT:
        {
            sVal.FormatVal(GetCurrentSpawned());
            return true;
        }
        case ISPW_SPAWNID:
        case ISPW_MORE:
        case ISPW_MORE1:
        {
            sVal = g_Cfg.ResourceGetName(_idSpawn);
            return true;
        }
        case ISPW_MORE2:
        case ISPW_PILE:
        {
            if (GetLink()->GetType() == IT_SPAWN_ITEM)
            {
                sVal.FormatUSVal(_iPile);
            }
            else
            {
                sVal.FormatWVal(GetCurrentSpawned());
            }
            return true;
        }
        case ISPW_MAXDIST:
        case ISPW_MOREZ:
        {
            sVal.FormatUCVal(_iMaxDist);
            return true;
        }
        case ISPW_TIMELO:
        case ISPW_MOREX:
        {
            sVal.FormatUSVal(_iTimeLo);
            return true;
        }
        case ISPW_TIMEHI:
        case ISPW_MOREY:
        {
            sVal.FormatUSVal(_iTimeHi);
            return true;
        }
        case ISPW_MOREP:
        {

            tchar * pszBuffer = Str_GetTemp();
            sprintf(pszBuffer, "%" PRId16 ",%" PRId16 ",%" PRId8, _iTimeLo, _iTimeHi, _iMaxDist);

            sVal.Format(pszBuffer);
            return true;
        }
        default:
            return false;    // Match but no code executed.
    }
	EXC_CATCH;
	return false;   // No match, let the code continue.
}

bool CSpawn::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CSpawn::r_LoadVal");
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
            SetAmount(s.GetArgUSVal());
            return true;
        }
        case ISPW_SPAWNID:
        case ISPW_MORE:
        case ISPW_MORE1:
        {
            switch (GetLink()->GetType())
            {
                case IT_SPAWN_CHAR:
                {
                    _idSpawn = CResourceID(RES_CHARDEF, s.GetArgUVal());   // Ensuring there's no negative value
                    break;
                }
                case IT_SPAWN_ITEM:
                {
                    _idSpawn = CResourceID(RES_ITEMDEF, s.GetArgUVal());
                    break;
                }
                case IT_SPAWN_CHAMPION: // handled on CChampion
                {
                    _idSpawn = CResourceID(RES_CHAMPION, s.GetArgUVal());
                    CChampion *pChampion = static_cast<CChampion*>(GetLink()->GetComponent(COMP_CHAMPION));
                    ASSERT(pChampion);
                    pChampion->Init();
                    return true; // More CChampion's custom code done in Init() plus FixDef() and SetTrackID() are not needed so halt here.
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
                _pLink->RemoveFromView();
                _pLink->Update();
            }
            return true;
        }
        case ISPW_MORE2:
        case ISPW_PILE:
        {
            if (GetLink()->GetType() == IT_SPAWN_ITEM)
            {
                _iPile = s.GetArgUSVal();
                return true;
            }
            return false;   // More2 on char's spawns refers to GetCurrentSpawned() (for backwards) but it should not be modified, for that purpose use AddObj();
        }
        case ISPW_MAXDIST:
        case ISPW_MOREZ:
        {
            _iMaxDist = s.GetArgUCVal();
            return true;
        }
        case ISPW_TIMELO:
        case ISPW_MOREX:
        {
            _iTimeLo = s.GetArgSVal();
            return true;
        }
        case ISPW_TIMEHI:
        case ISPW_MOREY:
        {
            _iTimeHi = s.GetArgSVal();
            return true;
        }
        case ISPW_MOREP:
        {
            tchar *pszTemp = Str_GetTemp();
            strcpy(pszTemp, s.GetArgStr());
            GETNONWHITESPACE(pszTemp);
            size_t iArgs = 0;
            if (IsDigit(pszTemp[0]) || pszTemp[0] == '-')
            {
                tchar * ppVal[3];
                iArgs = Str_ParseCmds(pszTemp, ppVal, CountOf(ppVal), " ,\t");
                switch (iArgs)
                {
                    case 3: // m_z
                        _iMaxDist = (uchar)(ATOI(ppVal[2]));
                    case 2: // m_y
                        _iTimeHi = (uint16)(ATOI(ppVal[1]));
                    case 1: // m_x
                        _iTimeLo = (uint16)(ATOI(ppVal[0]));
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

void CSpawn::r_Write(CScript & s)
{
	ADDTOCALLSTACK("CSpawn:r_Write");
	EXC_TRY("Write");

	if ( GetAmount() != 1 )
		s.WriteKeyVal("AMOUNT", GetAmount());

    s.WriteKey("SPAWNID", g_Cfg.ResourceGetName(_idSpawn));
    if (GetLink()->GetType() == IT_SPAWN_ITEM)
    {
        s.WriteKeyVal("PILE", GetPile());
    }
    s.WriteKeyVal("TIMELO", GetTimeLo());
    s.WriteKeyVal("TIMEHI", GetTimeHi());
    s.WriteKeyVal("MAXDIST", GetMaxDist());

	if (GetCurrentSpawned() <= 0 )
		return;

	for ( std::vector<CUID>::iterator it = _uidList.begin(); it != _uidList.end(); ++it )
	{
		if ( !it->IsValidUID() )
			continue;
		CObjBase *pObj = it->ObjFind();
        if (pObj)
        {
            s.WriteKeyHex("ADDOBJ", pObj->GetUID());
        }
	}

	EXC_CATCH;
}

enum SPAWN_REF
{
    ISPR_AT,
    ISPR_QTY
};

lpctstr const CSpawn::sm_szRefKeys[ISPR_QTY + 1]
{
    "AT",
    NULL
};

bool CSpawn::r_GetRef(lpctstr & pszKey, CScriptObj *& pRef)
{
    ADDTOCALLSTACK("CSpawn::r_GetRef");
    int iCmd = FindTableHeadSorted(pszKey, sm_szRefKeys, CountOf(sm_szRefKeys) - 1);

    if (iCmd < 0)
    {
        return false;
    }
    pszKey += strlen(sm_szRefKeys[iCmd]);
    SKIP_SEPARATORS(pszKey);

    switch (iCmd)
    {
        case ISPR_AT:
        {
            int objIndex = Exp_GetVal(pszKey);
            if (objIndex < 0)
                return false;
            SKIP_SEPARATORS(pszKey);
            if (objIndex >= GetCurrentSpawned())
            {
                return false;
            }
            if (GetLink()->IsType(IT_SPAWN_ITEM))
            {
                CItem *pSpawnedItem = _uidList[objIndex].ItemFind();
                if (pSpawnedItem)
                {
                    pRef = pSpawnedItem;
                    return true;
                }
            }
            else if (GetLink()->IsType(IT_SPAWN_CHAR) || GetLink()->IsType(IT_SPAWN_CHAMPION))
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

lpctstr const CSpawn::sm_szVerbKeys[ISPV_QTY + 1]
{
    "DELOBJ",
    "RESET",
    "START",
    "STOP",
    NULL
};

bool CSpawn::r_Verb(CScript & s, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CSpawn::r_Verb");
    UNREFERENCED_PARAMETER(pSrc);
    int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys) - 1);
    if (iCmd < 0)
        return false;
    switch (iCmd)
    {
        case ISPV_DELOBJ:
        {
            DelObj(static_cast<CUID>(s.GetArgVal()));
            return true;
        }
        case ISPV_RESET:
            KillChildren();
            OnTick(false);
            return true;
        case ISPV_START:
            GetLink()->SetTimeout(0);
            return true;
        case ISPV_STOP:
            KillChildren();
            GetLink()->SetTimeout(-1);
            return true;
        default:
            break;
    }
    return false;
}

void CSpawn::Copy(CComponent * target)
{
    ADDTOCALLSTACK("CSpawn::Copy");
    CSpawn *pTarget = static_cast<CSpawn*>(target);
    if (!pTarget)
    {
        return;
    }
    _iPile = pTarget->GetPile();
    _iAmount = pTarget->GetAmount();
    _iTimeLo = pTarget->GetTimeLo();
    _iTimeHi = pTarget->GetTimeHi();
    _iMaxDist = pTarget->GetMaxDist();
    _idSpawn = pTarget->GetSpawnID();

    // Not copying created objects.
}
