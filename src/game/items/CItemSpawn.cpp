
#include "../../common/CException.h"
#include "../../common/CObjBaseTemplate.h"
#include "../../common/CRegion.h"
#include "../../common/CUIDExtra.h"
#include "../chars/CChar.h"
#include "../chars/CCharNPC.h"
#include "../../common/CLog.h"
#include "../CContainer.h"
#include "CItemSpawn.h"

/////////////////////////////////////////////////////////////////////////////

byte CItemSpawn::GetAmount()
{
	//ADDTOCALLSTACK("CItemSpawn::GetAmount");
	return m_iAmount;
}

void CItemSpawn::SetAmount(byte iAmount)
{
	ADDTOCALLSTACK("CItemSpawn::SetAmount");
	m_iAmount = iAmount;
}

inline CCharBase *CItemSpawn::TryChar(CREID_TYPE &id)
{
	ADDTOCALLSTACK("CitemSpawn:TryChar");
	CCharBase *pCharDef = CCharBase::FindCharBase(id);
	if ( pCharDef )
	{
		m_itSpawnChar.m_CharID = CResourceID(RES_CHARDEF, id);
		return pCharDef;
	}
	return NULL;
}

inline CItemBase *CItemSpawn::TryItem(ITEMID_TYPE &id)
{
	ADDTOCALLSTACK("CitemSpawn:TryItem");
	CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( pItemDef )
	{
		m_itSpawnItem.m_ItemID = CResourceID(RES_ITEMDEF, id);
		return pItemDef;
	}
	return NULL;
}

CResourceDef *CItemSpawn::FixDef()
{
	ADDTOCALLSTACK("CitemSpawn:FixDef");

	CResourceIDBase rid = (IsType(IT_SPAWN_ITEM) ? m_itSpawnItem.m_ItemID : m_itSpawnChar.m_CharID);
	if ( rid.GetResType() != RES_UNKNOWN )
		return static_cast<CResourceDef *>(g_Cfg.ResourceGetDef(rid));

	// No type info here !?
	if ( IsType(IT_SPAWN_CHAR) )
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(rid.GetResIndex());
		if ( id < SPAWNTYPE_START )
			return TryChar(id);

		// try a spawn group.
		rid = CResourceID(RES_SPAWN, id);
		CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
		if ( pDef )
		{
			m_itSpawnChar.m_CharID = rid;
			return pDef;
		}
		return TryChar(id);
	}
	else
	{
		ITEMID_TYPE id = static_cast<ITEMID_TYPE>(rid.GetResIndex());
		if ( id < ITEMID_TEMPLATE )
			return TryItem(id);

		// try a template.
		rid = CResourceID(RES_TEMPLATE, id);
		CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
		if ( pDef )
		{
			m_itSpawnItem.m_ItemID = rid;
			return pDef;
		}
		return TryItem(id);
	}
}

int CItemSpawn::GetName(tchar *pszOut) const
{
	ADDTOCALLSTACK("CitemSpawn:GetName");
	CResourceIDBase rid;
	if ( IsType(IT_SPAWN_ITEM) )
		rid = m_itSpawnItem.m_ItemID;
	else
		rid = m_itSpawnChar.m_CharID;	// name the spawn type

	lpctstr pszName = NULL;
	CResourceDef *pDef = g_Cfg.ResourceGetDef(rid);
	if ( pDef != NULL )
		pszName = pDef->GetName();
	if ( pDef == NULL || pszName == NULL || pszName[0] == '\0' )
		pszName = g_Cfg.ResourceGetName(rid);

	return sprintf(pszOut, " (%s)", pszName);
}

CItemSpawn::CItemSpawn(ITEMID_TYPE id, CItemBase *pDef) : CItem(ITEMID_WorldGem, pDef)
{
	ADDTOCALLSTACK("CItemSpawn::CItemSpawn");
	UNREFERENCED_PARAMETER(id);		//forced in CItem(ITEMID_WorldGem, ...)
	m_currentSpawned = 0;
	m_iAmount = 1;
}

CItemSpawn::~CItemSpawn()
{
	ADDTOCALLSTACK("CItemSpawn::~CItemSpawn");
}

byte CItemSpawn::GetCount()
{
	ADDTOCALLSTACK("CitemSpawn:GetCount");
	return m_currentSpawned;
}

void CItemSpawn::GenerateItem(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CitemSpawn:GenerateItem");

	CResourceIDBase rid = pDef->GetResourceID();
	ITEMID_TYPE id = static_cast<ITEMID_TYPE>(rid.GetResIndex());

	CItemContainer *pCont = dynamic_cast<CItemContainer *>(GetParent());
	byte iCount = pCont ? (uchar)(pCont->ContentCount(rid)) : GetCount();
	if ( iCount >= GetAmount() )
		return;

	CItem *pItem = CreateTemplate(id);
	if ( pItem == NULL )
		return;

	word iAmountPile = (word)(minimum(UINT16_MAX,m_itSpawnItem.m_pile));
	if ( iAmountPile > 1 )
	{
		CItemBase *pItemDef = pItem->Item_GetDef();
		ASSERT(pItemDef);
		if ( pItemDef->IsStackableType() )
			pItem->SetAmount((word)Calc_GetRandVal(iAmountPile) + 1);
	}

	pItem->SetAttr(m_Attr & (ATTR_OWNED | ATTR_MOVE_ALWAYS));
	pItem->SetDecayTime(g_Cfg.m_iDecay_Item);	// it will decay eventually to be replaced later
	pItem->MoveNearObj(this, m_itSpawnItem.m_DistMax);
	AddObj(pItem->GetUID());
}

void CItemSpawn::GenerateChar(CResourceDef *pDef)
{
	ADDTOCALLSTACK("CitemSpawn:GenerateChar");
	if ( !IsTopLevel() )
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

	CPointMap pt = GetTopPoint();
	pChar->NPC_LoadScript(true);
	pChar->StatFlag_Set(STATF_Spawned);
	word iDistMax = m_itSpawnChar.m_DistMax;
	// Try placing this char near the spawn
	if ( !pChar->MoveNearObj(this, iDistMax ? (word)(Calc_GetRandVal(iDistMax) + 1) : 1) )
	{
		// If this fails, try placing the char ON the spawn
		if (!pChar->MoveTo(pt))
		{
			DEBUG_ERR(("Spawner UID:0%" PRIx32 " is unable to place a character inside the world, deleted the character", (dword)this->GetUID()));
			pChar->Delete();
			return;
		}
	}

	// Check if the NPC can spawn in this region
	CRegionBase *pRegion = pt.GetRegion(REGION_TYPE_AREA);
	if ( !pRegion || (pRegion->IsGuarded() && pChar->Noto_IsEvil()) )
	{
		pChar->Delete();
		return;
	}

	AddObj(pChar->GetUID());
	pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement and attachment to the spawnitem
	pChar->Update();

	size_t iCount = GetTopSector()->GetCharComplexity();
	if ( iCount > g_Cfg.m_iMaxCharComplexity )
		g_Log.Event(LOGL_WARN, "%" PRIuSIZE_T " chars at %s. Sector too complex!\n", iCount, GetTopSector()->GetBasePoint().WriteUsed());
}

void CItemSpawn::DelObj(CUID uid)
{
	ADDTOCALLSTACK("CitemSpawn:DelObj");
	if ( !uid.IsValidUID() )
		return;

	byte iMax = GetCount();
	for ( byte i = 0; i < iMax; i++ )
	{
		if ( m_obj[i] != uid )
			continue;

		CObjBase *pObj = uid.ObjFind();
		pObj->m_uidSpawnItem.InitUID();

		m_currentSpawned--;
		if ( GetType() == IT_SPAWN_CHAR )
			uid.CharFind()->StatFlag_Clear(STATF_Spawned);

		while ( m_obj[i + 1].IsValidUID() )				// searching for any entry higher than this one...
		{
			m_obj[i] = m_obj[i + 1];					// and moving it 1 position to keep values 'together'.
			i++;
		}
		m_obj[i].InitUID();								// Finished moving higher entries (if any) so we free the last entry.
		break;
	}
	ResendTooltip();
}

void CItemSpawn::AddObj(CUID uid)
{
	ADDTOCALLSTACK("CitemSpawn:AddObj");
	// NOTE: This function is also called when loading spawn items
	// on server startup. In this case, some objs UID still invalid
	// (not loaded yet) so just proceed without any checks.

	bool bIsSpawnChar = IsType(IT_SPAWN_CHAR);
	if ( !g_Serv.IsLoading() )
	{
		if ( !uid.IsValidUID() )
			return;

		if ( bIsSpawnChar )				// IT_SPAWN_CHAR can only spawn NPCs
		{
			CChar *pChar = uid.CharFind();
			if ( !pChar || !pChar->m_pNPC )
				return;
		}
		else if ( !uid.ItemFind() )		// IT_SPAWN_ITEM can only spawn items
			return;

		CItemSpawn *pPrevSpawn = static_cast<CItemSpawn*>(uid.ObjFind()->m_uidSpawnItem.ItemFind());
		if ( pPrevSpawn )
		{
			if ( pPrevSpawn == this )		// obj already linked to this spawn
				return;
			pPrevSpawn->DelObj(uid);		// obj linked to other spawn, remove the link before proceed
		}
	}

	byte iMax = maximum(GetAmount(), 1);
	for (byte i = 0; i < iMax; i++ )
	{
		if ( !m_obj[i].IsValidUID() )
		{
			m_obj[i] = uid;
			m_currentSpawned++;

			// objects are linked to the spawn at each server start
			if ( !g_Serv.IsLoading() )
			{
				uid.ObjFind()->m_uidSpawnItem = GetUID();
				if ( bIsSpawnChar )
				{
					CChar *pChar = uid.CharFind();
					ASSERT(pChar->m_pNPC);
					pChar->StatFlag_Set(STATF_Spawned);
					pChar->m_ptHome = GetTopPoint();
					pChar->m_pNPC->m_Home_Dist_Wander = (word)(m_itSpawnChar.m_DistMax);
				}
			}
			break;
		}
	}
	if ( !g_Serv.IsLoading() )
		ResendTooltip();
}

void CItemSpawn::OnTick(bool fExec)
{
	ADDTOCALLSTACK("CitemSpawn:OnTick");

	int64 iMinutes;

	if ( m_itSpawnChar.m_TimeHiMin <= 0 )
		iMinutes = Calc_GetRandLLVal(30) + 1;
	else
		iMinutes = minimum(m_itSpawnChar.m_TimeHiMin, m_itSpawnChar.m_TimeLoMin) + Calc_GetRandLLVal(abs(m_itSpawnChar.m_TimeHiMin - m_itSpawnChar.m_TimeLoMin));

	if ( iMinutes <= 0 )
		iMinutes = 1;

	if ( !fExec || IsTimerExpired() )
		SetTimeout(iMinutes * 60 * TICK_PER_SEC);	// set time to check again.

	if ( !fExec || m_currentSpawned >= GetAmount() )
		return;

	CResourceDef *pDef = FixDef();
	if ( !pDef )
	{
		CResourceIDBase rid = IsType(IT_SPAWN_ITEM) ? m_itSpawnItem.m_ItemID : m_itSpawnChar.m_CharID;
		DEBUG_ERR(("Bad Spawn point uid=0%x, id=%s\n", (dword)GetUID(), g_Cfg.ResourceGetName(rid)));
		return;
	}

	if ( IsType(IT_SPAWN_ITEM) )
		GenerateItem(pDef);
	else
		GenerateChar(pDef);
}

void CItemSpawn::KillChildren()
{
	ADDTOCALLSTACK("CitemSpawn:KillChildren");
	if (m_currentSpawned <= 0 )
		return;

	for (byte i = 0; i < m_currentSpawned; i++ )
	{
		CObjBase *pObj = m_obj[i].ObjFind();
		if ( !pObj )
			continue;
		pObj->m_uidSpawnItem.InitUID();
		pObj->Delete();
		m_obj[i].InitUID();

	}
	m_currentSpawned = 0;

	OnTick(false);
}

CCharBase *CItemSpawn::SetTrackID()
{
	ADDTOCALLSTACK("CitemSpawn:SetTrackID");
	SetAttr(ATTR_INVIS);	// Indicate to GM's that it is invis.
	if (GetHue() == 0)
		SetHue(HUE_RED_DARK);

	if ( !IsType(IT_SPAWN_CHAR) )
	{
		SetDispID(ITEMID_WorldGem_lg);
		return NULL;
	}

	CCharBase *pCharDef = NULL;
	CResourceIDBase rid = m_itSpawnChar.m_CharID;

	if ( rid.GetResType() == RES_CHARDEF )
	{
		CREID_TYPE id = static_cast<CREID_TYPE>(rid.GetResIndex());
		pCharDef = CCharBase::FindCharBase(id);
	}
	if ( pCharDef )		// They must want it to look like this.
		SetDispID(pCharDef ? pCharDef->m_trackID : ITEMID_TRACK_WISP);

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

lpctstr const CItemSpawn::sm_szLoadKeys[ISPW_QTY + 1] =
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

bool CItemSpawn::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CitemSpawn:r_WriteVal");
	EXC_TRY("WriteVal");
	if (!strnicmp(pszKey, "amount", 6))
	{
		sVal.FormatVal(GetAmount());
		return true;
	}
	else if ( !strnicmp(pszKey, "at.", 3) )
	{
		pszKey += 3;
		int objIndex = Exp_GetVal(pszKey);
		if ( m_obj[objIndex].ItemFind() )
			return m_obj[objIndex].ItemFind()->r_WriteVal(pszKey, sVal, pSrc);
		else if ( m_obj[objIndex].CharFind() )
			return m_obj[objIndex].CharFind()->r_WriteVal(pszKey, sVal, pSrc);
		return true;
	}
	else if ( !strnicmp(pszKey, "count", 5) )
	{
		sVal.FormatVal(GetCount());
		return true;
	}
	EXC_CATCH;
	return CItem::r_WriteVal(pszKey, sVal, pSrc);
}

bool CItemSpawn::r_LoadVal(CScript & s)
{
	ADDTOCALLSTACK("CitemSpawn:r_LoadVal");
	EXC_TRY("LoadVal");

	int iCmd = FindTableSorted(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
	if ( iCmd < 0 )
		return CItem::r_LoadVal(s);

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
			SetTimeout(0);
			return true;
		case ISPW_STOP:
			KillChildren();
			SetTimeout(-1);
			return true;
		default:
			break;
	}
	EXC_CATCH;
	return false;
}

void  CItemSpawn::r_Write(CScript & s)
{
	ADDTOCALLSTACK("CitemSpawn:r_Write");
	EXC_TRY("Write");
	CItem::r_Write(s);

	if ( GetAmount() != 1 )
		s.WriteKeyVal("AMOUNT", GetAmount());

	word iTotal = GetCount();
	if ( iTotal <= 0 )
		return;

	for ( byte i = 0; i < iTotal; i++ )
	{
		if ( !m_obj[i].IsValidUID() )
			continue;
		CObjBase *pObj = m_obj[i].ObjFind();
		if ( pObj )
			s.WriteKeyHex("ADDOBJ", pObj->GetUID());
	}

	EXC_CATCH;
}
