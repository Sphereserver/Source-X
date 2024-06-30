#include "../../common/CException.h"
#include "../../common/CLog.h"
#include "../../common/sphereproto.h"
#include "../chars/CChar.h"
#include "../chars/CCharNPC.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../CWorldSearch.h"
#include "CItem.h"
#include "CItemCorpse.h"


CItemCorpse::CItemCorpse( ITEMID_TYPE id, CItemBase * pItemDef ) :
    CTimedObject(PROFILE_ITEMS),
	CItemContainer( id, pItemDef )
{
}

CItemCorpse::~CItemCorpse()
{
	EXC_TRY("Cleanup in destructor");

	// Must remove early because virtuals will fail in child destructor.
	DeletePrepare();

	EXC_CATCH;
}

bool CItemCorpse::IsCorpseResurrectable(CChar * pCharHealer, CChar * pCharGhost) const
{
	if (!IsType(IT_CORPSE))
	{
		DEBUG_ERR(("Corpse (0%x) doesn't have type T_CORPSE! (it has %d)\n", (dword)GetUID(), GetType()));
		return false;
	}

	//Check when the corpse is targetted, that the ghost player is still dead.
	if (!pCharGhost->IsStatFlag(STATF_DEAD))
	{
		return false;
	}
	
	//Check if the ghost is visible when targetting the corpse.
	if (pCharGhost->IsStatFlag(STATF_INSUBSTANTIAL))
	{
		pCharHealer->SysMessageDefault(DEFMSG_HEALING_RES_MANIFEST);
		return false;
	}
	//Check if the ghost is in line of sight of its corpse.
	if (!pCharGhost->CanSeeLOS(this))
	{
		pCharHealer->SysMessageDefault(DEFMSG_HEALING_RES_LOS);
		return false;
	}

	//Check if the is ghost is within 2 tiles from its corpse.
	if (pCharGhost->GetDist(this) > 2)
	{
		pCharHealer->SysMessageDefault(DEFMSG_HEALING_RES_TOOFAR);
		return false;
	}

	//Check if the corpse is not inside a container.
	if (!IsTopLevel())
	{
		pCharHealer->SysMessageDefault(DEFMSG_HEALING_CORPSEG);
		return false;
	}

	CRegion* pRegion = GetTopPoint().GetRegion(REGION_TYPE_AREA | REGION_TYPE_MULTI);
	if (pRegion == nullptr)
	{
		return false;
	}

	//Antimagic check.
	if (pRegion->IsFlag(REGION_ANTIMAGIC_ALL | REGION_ANTIMAGIC_RECALL_IN | REGION_ANTIMAGIC_TELEPORT))
	{
		pCharHealer->SysMessageDefault(DEFMSG_HEALING_AM);
		if (pCharGhost != pCharHealer)
			pCharGhost->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_ATTEMPT), pCharHealer->GetName(), GetName());

		return false;
	}

	return true;
}
CChar *CItemCorpse::IsCorpseSleeping() const
{
	ADDTOCALLSTACK("CItemCorpse::IsCorpseSleeping");
	// Is this corpse really a sleeping person ?
	// CItemCorpse
	if ( !IsType(IT_CORPSE) )
	{
		DEBUG_ERR(("Corpse (0%x) doesn't have type T_CORPSE! (it has %d)\n", (dword)GetUID(), GetType()));
		return nullptr;
	}

	CChar *pCharCorpse = m_uidLink.CharFind();
	if ( pCharCorpse && pCharCorpse->IsStatFlag(STATF_SLEEPING) && !GetTimeStampS() )
		return pCharCorpse;

	return nullptr;
}

int CItemCorpse::GetWeight(word amount) const
{
	UnreferencedParameter(amount);
	// GetAmount is messed up.
	// true weight == container item + contents.
	return( 1 + CContainer::GetTotalWeight());
}


bool CChar::CheckCorpseCrime( CItemCorpse *pCorpse, bool fLooting, bool fTest )
{
	ADDTOCALLSTACK("CChar::CheckCorpseCrime");
	// fLooting = looting as apposed to carving.
	// RETURN: true = criminal act !

	if ( !pCorpse || !g_Cfg.m_fLootingIsACrime )
		return false;

	CChar *pCharGhost = pCorpse->m_uidLink.CharFind();
	if ( !pCharGhost || pCharGhost == this )
		return false;

	if ( pCharGhost->Noto_GetFlag(this) == NOTO_GOOD )
	{
		if ( !fTest )
		{
			// Anyone saw me doing this?
			CheckCrimeSeen(SKILL_NONE, pCharGhost, pCorpse, fLooting ? g_Cfg.GetDefaultMsg(DEFMSG_LOOTING_CRIME) : nullptr);
			Noto_Criminal();
		}
		return true;
	}
	return false;
}

CItemCorpse *CChar::FindMyCorpse( bool ignoreLOS, int iRadius ) const
{
	ADDTOCALLSTACK("CChar::FindMyCorpse");
	// If they are standing on their own corpse then res the corpse !
	auto Area = CWorldSearchHolder::GetInstance(GetTopPoint(), iRadius);
	for (;;)
	{
		CItem *pItem = Area->GetItem();
		if ( !pItem )
			break;
		if ( !pItem->IsType(IT_CORPSE) )
			continue;
		CItemCorpse *pCorpse = dynamic_cast<CItemCorpse*>(pItem);
		if ( !pCorpse || (pCorpse->m_uidLink != GetUID()) )
			continue;
		if ( pCorpse->m_itCorpse.m_BaseID != _iPrev_id )	// not morphed type
			continue;
		if ( !ignoreLOS && !CanSeeLOS(pCorpse) )
			continue;
		return pCorpse;
	}
	return nullptr;
}

// Create the char corpse when i die (STATF_DEAD) or fall asleep (STATF_SLEEPING)
// Summoned (STATF_CONJURED) and some others creatures have no corpse.
CItemCorpse * CChar::MakeCorpse( bool fFrontFall )
{
	ADDTOCALLSTACK("CChar::MakeCorpse");

	uint uiFlags = (uint)(m_TagDefs.GetKeyNum("DEATHFLAGS"));
	if (uiFlags & DEATH_NOCORPSE)
		return nullptr;
	if (IsStatFlag(STATF_CONJURED) && !(uiFlags & (DEATH_NOCONJUREDEFFECT|DEATH_HASCORPSE)))
	{
		Effect(EFFECT_XYZ, ITEMID_FX_SPELL_FAIL, this, 1, 30);
		return nullptr;
	}

	CItemCorpse *pCorpse = dynamic_cast<CItemCorpse *>(CItem::CreateScript(ITEMID_CORPSE, this));
	if (pCorpse == nullptr)	// weird internal error
		return nullptr;

	tchar *pszMsg = Str_GetTemp();
	snprintf(pszMsg, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_CORPSE_OF), GetName());
	pCorpse->SetName(pszMsg);
	pCorpse->SetHue(GetHue());
	pCorpse->SetCorpseType(GetDispID());
	pCorpse->SetAttr(ATTR_MOVE_NEVER);
	pCorpse->m_itCorpse.m_BaseID = _iPrev_id;	// id the corpse type here !
	pCorpse->m_itCorpse.m_facing_dir = m_dirFace;
	pCorpse->m_uidLink = GetUID();
    pCorpse->m_ModMaxWeight = g_Cfg.Calc_MaxCarryWeight(this); // set corpse maxweight to prevent weird exploits like when someone place many items on an player corpse just to make this player get stuck on resurrect

	if (fFrontFall)
		pCorpse->m_itCorpse.m_facing_dir = (DIR_TYPE)(m_dirFace|DIR_MASK_RUNNING);

	int64 iDecayTimer = -1;	// never decay
	if (IsStatFlag(STATF_DEAD))
	{
		iDecayTimer = (m_pPlayer) ? g_Cfg.m_iDecay_CorpsePlayer : g_Cfg.m_iDecay_CorpseNPC;
		pCorpse->SetTimeStampS(CWorldGameTime::GetCurrentTime().GetTimeRaw());	// death time
		if (Attacker_GetLast())
			pCorpse->m_itCorpse.m_uidKiller = Attacker_GetLast()->GetUID();
		else
			pCorpse->m_itCorpse.m_uidKiller.InitUID();
	}
	else	// sleeping (not dead)
	{
		pCorpse->SetTimeStampS(0);
		pCorpse->m_itCorpse.m_uidKiller = GetUID();
	}

	if ((m_pNPC && m_pNPC->m_bonded) || IsStatFlag(STATF_CONJURED|STATF_SLEEPING))
		pCorpse->m_itCorpse.m_carved = 1;	// corpse of bonded and summoned creatures (or sleeping players) can't be carved

	if ( !(uiFlags & DEATH_NOLOOTDROP) )		// move non-newbie contents of the pack to corpse
		DropAll( pCorpse );
	
    if (iDecayTimer != -1)
    {
        pCorpse->MoveToDecay(GetTopPoint(), iDecayTimer);
    }
    else
    {
        pCorpse->MoveTo(GetTopPoint());
    }
	return pCorpse;
}

// We are creating a char from the current char and the corpse.
// Move the items from the corpse back onto us.
bool CChar::RaiseCorpse( CItemCorpse * pCorpse )
{
	ADDTOCALLSTACK("CChar::RaiseCorpse");

	if ( !pCorpse )
		return false;

	if ( !pCorpse->IsContainerEmpty() )
	{
		CItemContainer *pPack = GetPackSafe();
        //Looping 2x to equip items first then send rest to pack
		for ( CSObjContRec *pObjRec : pCorpse->GetIterationSafeContReverse() )
		{
			CItem* pItem = static_cast<CItem*>(pObjRec);
			if ( pItem->IsType(IT_HAIR) || pItem->IsType(IT_BEARD) )	// hair on corpse was copied!
				continue;

			if ( pItem->GetContainedLayer() )
				ItemEquip(pItem);
		}
        for (CSObjContRec* pObjRec : pCorpse->GetIterationSafeContReverse())
        {
            CItem* pItem = static_cast<CItem*>(pObjRec);
            if (pItem->IsType(IT_HAIR) || pItem->IsType(IT_BEARD))	// hair on corpse was copied!
                continue;

            if (pPack)
                pPack->ContentAdd(pItem);
        }

		pCorpse->ContentsDump( GetTopPoint() );		// drop left items on ground
	}

	UpdateAnimate((pCorpse->m_itCorpse.m_facing_dir & DIR_MASK_RUNNING) ? ANIM_DIE_FORWARD : ANIM_DIE_BACK, true, true);
	pCorpse->Delete();
	return true;
}
