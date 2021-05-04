//  CChar is either an NPC or a Player.

#include "../clients/CClient.h"
#include "../items/CItem.h"
#include "../items/CItemCorpse.h"
#include "../components/CCPropsChar.h"
#include "../components/CCSpawn.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../CWorldTickingList.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

static const int MASK_RETURN_FOLLOW_LINKS = 0x02;

bool CChar::Use_MultiLockDown( CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Use_MultiLockDown");
	ASSERT(pItemTarg);
	ASSERT(m_pArea);

	if ( pItemTarg->IsType(IT_KEY) || !pItemTarg->IsMovableType() || !pItemTarg->IsTopLevel() )
		return false;

	if ( !pItemTarg->m_uidLink.IsValidUID() )
	{
		// If we are in a house then lock down the item.
		pItemTarg->m_uidLink.SetPrivateUID(m_pArea->GetResourceID());
		SysMessageDefault(DEFMSG_MULTI_LOCKDOWN);
		return true;
	}
	if ( pItemTarg->m_uidLink == m_pArea->GetResourceID() )
	{
		pItemTarg->m_uidLink.InitUID();
		SysMessageDefault(DEFMSG_MULTI_LOCKUP);
		return true;
	}

	return false;
}

void CChar::Use_CarveCorpse( CItemCorpse * pCorpse, CItem * pItemCarving )
{
	ADDTOCALLSTACK("CChar::Use_CarveCorpse");

	if (!pItemCarving)
		return;

	CREID_TYPE CorpseID = pCorpse->m_itCorpse.m_BaseID;
	CCharBase *pCorpseDef = CCharBase::FindCharBase(CorpseID);
	if ( !pCorpseDef || pCorpse->m_itCorpse.m_carved )
	{
		SysMessageDefault(DEFMSG_CARVE_CORPSE_NOTHING);
		return;
	}

	CChar *pChar = pCorpse->m_uidLink.CharFind();
	CPointMap pnt = pCorpse->GetTopLevelObj()->GetTopPoint();

	UpdateAnimate(ANIM_BOW);
	if ( pCorpse->m_TagDefs.GetKeyNum("BLOOD") )
	{
		CItem *pBlood = CItem::CreateBase(ITEMID_BLOOD4);
		ASSERT(pBlood);
		pBlood->SetHue(pCorpseDef->_wBloodHue);
		pBlood->MoveToDecay(pnt, 5 * MSECS_PER_SEC);
	}

	word iResourceQty = 0;
	size_t iResourceTotalQty = pCorpseDef->m_BaseResources.size();

	CScriptTriggerArgs Args(iResourceTotalQty,0,pItemCarving);

	for (size_t i = 0; i < iResourceTotalQty; ++i)
	{
		
		const CResourceID& rid = pCorpseDef->m_BaseResources[i].GetResourceID();
		if (rid.GetResType() != RES_ITEMDEF)
			continue;

		ITEMID_TYPE id = (ITEMID_TYPE)(rid.GetResIndex());
		if (id == ITEMID_NOTHING)
			break;

		tchar* pszTmp = Str_GetTemp();
		snprintf(pszTmp, STR_TEMPLENGTH, "resource.%u.ID", (int)i);
		Args.m_VarsLocal.SetNum(pszTmp, (int64)id);

		iResourceQty = (word)pCorpseDef->m_BaseResources[i].GetResQty();
		snprintf(pszTmp, STR_TEMPLENGTH, "resource.%u.amount", (int)i);
		Args.m_VarsLocal.SetNum(pszTmp, iResourceQty);
	}
	if (IsTrigUsed(TRIGGER_CARVECORPSE) || IsTrigUsed(TRIGGER_ITEMCARVECORPSE))
	{
		switch (static_cast<CItem*>(pCorpse)->OnTrigger(ITRIG_CarveCorpse, this, &Args))
		{
		case TRIGRET_RET_TRUE:	return;
		default:				break;
		}
	}

	size_t iItems = 0;
	for ( size_t i = 0; i < iResourceTotalQty; ++i )
	{
		/*llong iQty = pCorpseDef->m_BaseResources[i].GetResQty();
		const CResourceID& rid = pCorpseDef->m_BaseResources[i].GetResourceID();
		if ( rid.GetResType() != RES_ITEMDEF )
			continue;

		ITEMID_TYPE id = (ITEMID_TYPE)(rid.GetResIndex());
		if ( id == ITEMID_NOTHING )
			break;*/

		tchar* pszTmp = Str_GetTemp();
		snprintf(pszTmp, STR_TEMPLENGTH, "resource.%u.ID", (int)i);
		ITEMID_TYPE id = (ITEMID_TYPE)RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum(pszTmp));
		if (id == ITEMID_NOTHING)
			break; 

		snprintf(pszTmp, STR_TEMPLENGTH, "resource.%u.amount", (int)i);
		iResourceQty =(word)Args.m_VarsLocal.GetKeyNum(pszTmp);

		++ iItems;
		CItem *pPart = CItem::CreateTemplate(id, nullptr, this);
		ASSERT(pPart);
		switch ( pPart->GetType() )
		{
			case IT_FOOD:
			case IT_FOOD_RAW:
			case IT_MEAT_RAW:
				SysMessageDefault(DEFMSG_CARVE_CORPSE_MEAT);
				//pPart->m_itFood.m_MeatType = CorpseID;
				break;
			case IT_HIDE:
				SysMessageDefault(DEFMSG_CARVE_CORPSE_HIDES);
				//pPart->m_itSkin.m_creid = CorpseID;
				if ( (g_Cfg.m_iRacialFlags & RACIALF_HUMAN_WORKHORSE) && IsHuman() )	// humans always find 10% bonus when gathering hides, ores and logs (Workhorse racial trait)
					iResourceQty = iResourceQty * 110 / 100;
				break;
			case IT_FEATHER:
				SysMessageDefault(DEFMSG_CARVE_CORPSE_FEATHERS);
				//pPart->m_itSkin.m_creid = CorpseID;
				break;
			case IT_WOOL:
				SysMessageDefault(DEFMSG_CARVE_CORPSE_WOOL);
				//pPart->m_itSkin.m_creid = CorpseID;
				break;
			/*case IT_DRAGON_SCALE:			// TO-DO (typedef IT_DRAGON_SCALE doesn't exist yet)
				SysMessageDefault(DEFMSG_CARVE_CORPSE_SCALES);
				//pPart->m_itSkin.m_creid = CorpseID;
				break;*/
			default:
				break;
		}

		if (iResourceQty > 1 )
			pPart->SetAmount((word)iResourceQty);

		if ( pChar && pChar->m_pPlayer )
		{
			tchar *pszMsg = Str_GetTemp();
			snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_CORPSE_NAME), pPart->GetName(), pChar->GetName());
			pPart->SetName(pszMsg);
			pPart->m_uidLink = pChar->GetUID();
			pPart->MoveToDecay(pnt, pPart->GetDecayTime());
			continue;
		}
		pCorpse->ContentAdd(pPart);
	}

	if ( iItems < 1 )
		SysMessageDefault(DEFMSG_CARVE_CORPSE_NOTHING);

	CheckCorpseCrime(pCorpse, false, false);
	pCorpse->m_itCorpse.m_carved = 1;			// mark as been carved
	pCorpse->m_itCorpse.m_uidKiller = GetUID();	// by you

	if ( pChar && pChar->m_pPlayer )
		pCorpse->SetTimeout(0);		// reset corpse timer to make it turn bones
}

void CChar::Use_MoonGate( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::Use_MoonGate");
	ASSERT(pItem);

    // If telepad is linked to an obj, use this obj P instead telepad static MOREP
    // This is required on telepads pointing to dynamic dests (like moongates inside moving boats)
    const CObjBase *pLink = pItem->m_uidLink.ObjFind();
    if (pLink && !pLink->IsTopLevel())
        return;
    CPointMap pt;
    if (pLink)
        pt = pLink->GetTopPoint();
    else
        pt = pItem->m_itTelepad.m_ptMark;

	if ( pItem->IsType(IT_MOONGATE) )
	{
		// RES_MOONGATES
		// What gate are we at ?
		size_t i = 0;
		size_t iCount = g_Cfg.m_MoonGates.size();
        const CPointMap& ptTop = GetTopPoint();
		for ( ; i < iCount; ++i )
		{
			if ( ptTop.GetDist(g_Cfg.m_MoonGates[i]) <= UO_MAP_VIEW_SIZE_DEFAULT )
				break;
		}

		// Set it's current destination based on the moon phases.
		// ensure iTrammelPhrase isn't smaller than iFeluccaPhase, to avoid uint underflow in next calculation
		size_t iTrammelPhase = CWorldGameTime::GetMoonPhase(false) % iCount;
		size_t iFeluccaPhase = CWorldGameTime::GetMoonPhase(true) % iCount;
		if ( iTrammelPhase < iFeluccaPhase )
			iTrammelPhase += iCount;

		size_t iMoongateIndex = (i + (iTrammelPhase - iFeluccaPhase)) % iCount;
		ASSERT(g_Cfg.m_MoonGates.IsValidIndex(iMoongateIndex));
		pt = g_Cfg.m_MoonGates[iMoongateIndex];
	}

	if ( m_pNPC )
	{
		if ( pItem->m_itTelepad.m_fPlayerOnly )
			return;
		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )	// guards won't leave the guarded region
		{
            bool fToGuardedArea = false;
            const CRegion *pArea = pt.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA);
            if (!pArea)
                return;
            if ( pArea->IsGuarded() )
                fToGuardedArea = true;

            if (IsSetOF(OF_GuardOutsideGuardedArea))
            {
                // I come from a guarded area, so i don't want to leave it unprotected; otherwise, i don't care if my destination region is guarded or not
                const CRegion * pAreaHome = m_ptHome.GetRegion( REGION_TYPE_AREA );
                if (!pAreaHome || (pAreaHome->IsGuarded() && !fToGuardedArea))
                    return;
            }
            else
            {
                // I can only go in guarded areas
                if (!fToGuardedArea)
                    return;
            }
			
		}
		if ( Noto_IsCriminal() )	// criminals won't enter on guarded regions
		{
            const CRegion *pArea = pt.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA);
			if ( !pArea || pArea->IsGuarded() )
				return;
		}
	}

	bool fCheckAntiMagic = pItem->IsAttr(ATTR_DECAY);
	bool fDisplayEffect = !pItem->m_itTelepad.m_fQuiet;
	Spell_Teleport(pt, true, fCheckAntiMagic, fDisplayEffect);
}

bool CChar::Use_Kindling( CItem * pKindling )
{
	ADDTOCALLSTACK("CChar::Use_Kindling");
	ASSERT(pKindling);
	if ( !pKindling->IsTopLevel() )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_KINDLING_CONT);
		return false;
	}

	if ( !Skill_UseQuick(SKILL_CAMPING, Calc_GetRandLLVal(30)) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_KINDLING_FAIL);
		return false;
	}

	pKindling->SetID(ITEMID_CAMPFIRE);
	pKindling->SetAttr(ATTR_MOVE_NEVER|ATTR_CAN_DECAY);
	pKindling->SetTimeoutS((4 + (int64)pKindling->GetAmount()) * 60);
	pKindling->SetAmount(1);	// all kindling is set to one fire
	pKindling->m_itLight.m_pattern = LIGHT_LARGE;
	pKindling->Update();
	pKindling->Sound(0x226);
	return true;
}

bool CChar::Use_Cannon_Feed( CItem * pCannon, CItem * pFeed )
{
	ADDTOCALLSTACK("CChar::Use_Cannon_Feed");
	if ( pFeed && pCannon && pCannon->IsType(IT_CANNON_MUZZLE) )
	{
		if ( !CanUse(pCannon, false) )
			return false;
		if ( !CanUse(pFeed, true) )
			return false;

		if ( pFeed->GetID() == ITEMID_REAG_SA )
		{
			if ( pCannon->m_itCannon.m_Load & 1 )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_CANNON_HPOWDER);
				return false;
			}
			pCannon->m_itCannon.m_Load |= 1;
			SysMessageDefault(DEFMSG_ITEMUSE_CANNON_LPOWDER);
			return true;
		}

		if ( pFeed->IsType(IT_CANNON_BALL) )
		{
			if ( pCannon->m_itCannon.m_Load & 2 )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_CANNON_HSHOT);
				return false;
			}
			pCannon->m_itCannon.m_Load |= 2;
			SysMessageDefault(DEFMSG_ITEMUSE_CANNON_LSHOT);
			return true;
		}
	}

	SysMessageDefault(DEFMSG_ITEMUSE_CANNON_EMPTY);
	return false;
}

bool CChar::Use_Train_Dummy( CItem * pItem, bool fSetup )
{
	ADDTOCALLSTACK("CChar::Use_Train_Dummy");
	// IT_TRAIN_DUMMY
	// Dummy animation timer prevents over dclicking.
	ASSERT(pItem);

	if ( !pItem->IsTopLevel() || (GetDist(pItem) > 1) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_TRAININGDUMMY_TOOFAR);
		return false;
	}
	else if ( IsStatFlag(STATF_ONHORSE) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_TRAININGDUMMY_MOUNT);
		return false;
	}

	SKILL_TYPE skill = Fight_GetWeaponSkill();
	if ( g_Cfg.IsSkillFlag(skill, SKF_RANGED) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_TRAININGDUMMY_RANGED);
		return false;
	}

	// Start action
	if ( fSetup )
	{
		if ( Skill_GetActive() == NPCACT_TRAINING )
			return true;

		char skilltag[38];
		snprintf(skilltag, sizeof(skilltag), "OVERRIDE.PracticeMax.SKILL_%d", (int)(skill & ~0xD2000000));
		CVarDefCont *pSkillTag = pItem->GetKey(skilltag, true);
		word iMaxSkill = pSkillTag ? (word)pSkillTag->GetValNum() : (word)g_Cfg.m_iSkillPracticeMax;
		if ( Skill_GetBase(skill) > iMaxSkill )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_TRAININGDUMMY_SKILL);
			return false;
		}

		int iAnimDelay = g_Cfg.Calc_CombatAttackSpeed(this, m_uidWeapon.ItemFind());
		UpdateAnimate(ANIM_ATTACK_WEAPON, true, false, (byte)maximum(0,(iAnimDelay-1) / 10));
		m_Act_Prv_UID = m_uidWeapon;
		m_Act_UID = pItem->GetUID();
		Skill_Start(NPCACT_TRAINING);
		return true;
	}

	// Finish action
	if ( Skill_GetActive() != NPCACT_TRAINING )
		return false;

	pItem->SetAnim((ITEMID_TYPE)(pItem->GetDispID() + 1), 3 * 1000);
	static const SOUND_TYPE sm_TrainingDummySounds[] = { 0x3A4, 0x3A6, 0x3A9, 0x3AE, 0x3B4, 0x3B6 };
	pItem->Sound(sm_TrainingDummySounds[Calc_GetRandVal(CountOf(sm_TrainingDummySounds))]);
	Skill_Experience(skill, Calc_GetRandVal(40));
	return true;
}

bool CChar::Use_Train_PickPocketDip( CItem * pItem, bool fSetup )
{
	ADDTOCALLSTACK("CChar::Use_Train_PickPocketDip");
	// IT_TRAIN_PICKPOCKET
	// Train dummy.
	ASSERT(pItem);

	if ( !pItem->IsTopLevel() || (GetDist(pItem) > 1) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_TOOFAR);
		return true;
	}
	else if ( IsStatFlag(STATF_ONHORSE) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_MOUNT);
		return false;
	}

	// Start action
	if ( fSetup )
	{
		if ( Skill_GetActive() == NPCACT_TRAINING )
			return true;

		if ( Skill_GetBase(SKILL_STEALING) > g_Cfg.m_iSkillPracticeMax )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_SKILL);
			return true;
		}

		m_Act_Prv_UID = m_uidWeapon;
		m_Act_UID = pItem->GetUID();
		Skill_Start(NPCACT_TRAINING);
		return true;
	}

	// Finish action
	if ( Skill_GetActive() != NPCACT_TRAINING )
		return false;

	pItem->Sound(SOUND_RUSTLE);
	if ( Skill_UseQuick(SKILL_STEALING, Calc_GetRandVal(40)) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_SUCCESS);
		pItem->SetAnim(pItem->GetDispID(), 3 * 1000);
	}
	else
	{
		SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_FAIL);
		pItem->Sound(SOUND_GLASS_BREAK4);
		pItem->SetAnim((ITEMID_TYPE)(pItem->GetDispID() + 1), 3 * 1000);
	}
	Skill_Experience(SKILL_STEALING, Calc_GetRandVal(40));
	return true;
}

bool CChar::Use_Train_ArcheryButte( CItem * pButte, bool fSetup )
{
	ADDTOCALLSTACK("CChar::Use_Train_ArcheryButte");
	// IT_ARCHERY_BUTTE
	ASSERT(pButte);

	// If standing right next to the butte, gather the arrows/bolts
	int iDist = GetDist(pButte);
	if ( (iDist < 2) && pButte->m_itArcheryButte.m_iAmmoCount )
	{
		CItem *pRemovedAmmo = CItem::CreateBase((ITEMID_TYPE)pButte->m_itArcheryButte.m_ridAmmoType.GetResIndex());
		ASSERT(pRemovedAmmo);
		pRemovedAmmo->SetAmount((word)pButte->m_itArcheryButte.m_iAmmoCount);
		ItemBounce(pRemovedAmmo, false);
		SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_GATHER);

		pButte->m_itArcheryButte.m_ridAmmoType.Clear();
		pButte->m_itArcheryButte.m_iAmmoCount = 0;
		return true;
	}

	CItem *pWeapon = m_uidWeapon.ItemFind();
	SKILL_TYPE skill = pWeapon ? pWeapon->Weapon_GetSkill() : SKILL_NONE;
	if ( !pWeapon || !g_Cfg.IsSkillFlag(skill, SKF_RANGED) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_RANGED);
		return true;
	}

	// Check distance
	if ( iDist < 5 )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_TOOCLOSE);
		return false;
	}
	else if ( iDist > 6 )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_TOOFAR);
		return false;
	}

	// Check position alignment
	CPointMap ptChar = GetTopPoint();
	CPointMap ptButte = pButte->GetTopPoint();
	if ( pButte->GetDispID() == ITEMID_ARCHERYBUTTE_S )
	{
		if ( ptChar.m_x != ptButte.m_x )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_WRONGALIGN);
			return false;
		}
		else if ( ptChar.m_y < ptButte.m_y )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_WRONGPOS);
			return false;
		}
	}
	else
	{
		if ( ptChar.m_y != ptButte.m_y )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_WRONGALIGN);
			return false;
		}
		else if ( ptChar.m_x < ptButte.m_x )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_WRONGPOS);
			return false;
		}
	}

	// Start action
	if ( fSetup )
	{
		if (Skill_GetActive() == NPCACT_TRAINING)
			return true;

		char skilltag[38];
		snprintf(skilltag, sizeof(skilltag), "OVERRIDE.PracticeMax.SKILL_%d", (int)(skill & ~0xD2000000));
		CVarDefCont *pSkillTag = pButte->GetKey(skilltag, true);
		word iMaxSkill = pSkillTag ? (word)pSkillTag->GetValNum() : (word)g_Cfg.m_iSkillPracticeMax;
		if ( Skill_GetBase(skill) > iMaxSkill )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_SKILL);
			return false;
		}

		int iAnimDelay = g_Cfg.Calc_CombatAttackSpeed(this, pWeapon);
		UpdateAnimate(ANIM_ATTACK_WEAPON, true, false, (byte)maximum(0,(iAnimDelay-1) / 10) );
		m_Act_Prv_UID = m_uidWeapon;
		m_Act_UID = pButte->GetUID();
		Skill_Start(NPCACT_TRAINING);
		return true;
	}

	// Finish action
	if ( Skill_GetActive() != NPCACT_TRAINING )
		return false;

	CItem *pAmmo = nullptr;
	const CResourceID ridAmmo(pWeapon->Weapon_GetRangedAmmoRes());
	if (ridAmmo.IsValidUID())
	{
		pAmmo = pWeapon->Weapon_FindRangedAmmo(ridAmmo);
		if ( !pAmmo )
		{
			SysMessageDefault(DEFMSG_COMBAT_ARCH_NOAMMO);
			return false;
		}
	}

	// If there is a different ammo type on the butte, it must be removed first
	ITEMID_TYPE ButteAmmoID = (ITEMID_TYPE)pButte->m_itArcheryButte.m_ridAmmoType.GetResIndex();
	ITEMID_TYPE WeaponAmmoID = ITEMID_NOTHING;
	if ( pAmmo )
	{
		WeaponAmmoID = pAmmo->GetID();
		if ( ButteAmmoID && (ButteAmmoID != WeaponAmmoID) )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_CLEAN);
			return false;
		}
		pAmmo->ConsumeAmount();
	}

	// Ammo animation
	ITEMID_TYPE AnimID = ITEMID_NOTHING;
	dword AnimHue = 0, AnimRender = 0;
	pWeapon->Weapon_GetRangedAmmoAnim(AnimID, AnimHue, AnimRender);
	pButte->Effect(EFFECT_BOLT, AnimID, this, 18, 1, false, AnimHue, AnimRender);

	if ( m_pClient && (skill == SKILL_THROWING) )		// throwing weapons also have anim of the weapon returning after throw it
	{
		m_pClient->m_timeLastSkillThrowing = CWorldGameTime::GetCurrentTime().GetTimeRaw();
		m_pClient->m_pSkillThrowingTarg = pButte;
		m_pClient->m_SkillThrowingAnimID = AnimID;
		m_pClient->m_SkillThrowingAnimHue = AnimHue;
		m_pClient->m_SkillThrowingAnimRender = AnimRender;
	}

	if ( Skill_UseQuick(skill, Calc_GetRandVal(40)) )
	{
		static lpctstr const sm_Txt_ArcheryButte_Success[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_HIT1),
			g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_HIT2),
			g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_HIT3),
			g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_HIT4)
		};
		Emote(sm_Txt_ArcheryButte_Success[Calc_GetRandVal(CountOf(sm_Txt_ArcheryButte_Success))]);
		Sound(pWeapon->Weapon_GetSoundHit());

		if ( WeaponAmmoID )
		{
			pButte->m_itArcheryButte.m_ridAmmoType = CResourceIDBase(RES_ITEMDEF, (int)WeaponAmmoID);
			++ pButte->m_itArcheryButte.m_iAmmoCount;
		}
	}
	else
	{
		Emote(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_MISS));
		Sound(pWeapon->Weapon_GetSoundMiss());
	}

	Skill_Experience(skill, Calc_GetRandVal(40));
	return true;
}

bool CChar::Use_Item_Web( CItem * pItemWeb )
{
	ADDTOCALLSTACK("CChar::Use_Item_Web");
	// IT_WEB
	// IT_EQ_STUCK
	// Try to break out of the web.
	// Or just try to damage it.
	//
	// RETURN: true = held in place.
	//  false = walk thru it.

	if ( GetDispID() == CREID_GIANT_SPIDER || !pItemWeb || !pItemWeb->IsTopLevel() || IsStatFlag(STATF_DEAD|STATF_INSUBSTANTIAL) || IsPriv(PRIV_GM) )
		return false;	// just walk through it

	// Try to break it.

    if (pItemWeb->m_itWeb.m_dwHitsCur == 0)
        pItemWeb->m_itWeb.m_dwHitsCur = 60 + Calc_GetRandVal(250);
    else if (pItemWeb->m_itWeb.m_dwHitsCur > INT32_MAX)
        pItemWeb->m_itWeb.m_dwHitsCur = INT32_MAX;

	// Since broken webs become spider silk, we should get out of here now if we aren't in a web.
	CItem *pFlag = LayerFind(LAYER_FLAG_Stuck);
	if ( CanMove(pItemWeb, false) )
	{
		if ( pFlag )
			pFlag->Delete();
		return false;
	}

	if ( pFlag && pFlag->IsTimerSet() )
	{
		// don't allow me to try to damage it too often
		return true;
	}

    int iCharStr = Stat_GetAdjusted(STAT_STR);
	const int iDmg = pItemWeb->OnTakeDamage(iCharStr, this);
	switch ( iDmg )
	{
		case 0:			// damage blocked
		case 1:			// web survived
		default:		// unknown
			if ( GetTopPoint() == pItemWeb->GetTopPoint() )		// is character still stuck on the web?
				break;

		case 2:			// web turned into silk
		case INT32_MAX:	// web destroyed
			if ( pFlag )
				pFlag->Delete();
			return false;
	}

	// Stuck in it still.
	if ( !pFlag )
	{
		if ( iDmg < 0 )
			return false;

		// First time message.
		pFlag = CItem::CreateBase(ITEMID_WEB1_1);
		ASSERT(pFlag);
		pFlag->SetAttr(ATTR_DECAY);
		pFlag->SetType(IT_EQ_STUCK);
		pFlag->m_uidLink = pItemWeb->GetUID();		

        int iStuckTimerSeconds = 2; // Mininum stuck timer value is 2 seconds.
        iCharStr = ((100 - minimum(100, iCharStr)) * (int)pItemWeb->m_itWeb.m_dwHitsCur) / 10;
        iStuckTimerSeconds = minimum(10, iStuckTimerSeconds + iCharStr); //Maximum stuck timer value is 10 seconds

		pFlag->SetTimeout(iStuckTimerSeconds * MSECS_PER_SEC);
		LayerAdd(pFlag, LAYER_FLAG_Stuck);
	}
	else
	{
		if ( iDmg < 0 )
		{
			pFlag->Delete();
			return false;
		}
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SWEB_STUCK), pItemWeb->GetName());
	}

	return true;
}

int CChar::Use_PlayMusic( CItem * pInstrument, int iDifficultyToPlay )
{
	ADDTOCALLSTACK("CChar::Use_PlayMusic");
	// SKILL_ENTICEMENT, SKILL_MUSICIANSHIP,
	// ARGS:
	//	iDifficultyToPlay = 0-100
	// RETURN:
	//  >=0 = success
	//  -1 = too hard for u.
	//  -2 = can't play. no instrument.

	if ( !pInstrument )
	{
		pInstrument = ContentFind(CResourceID(RES_TYPEDEF, IT_MUSICAL), 0, 1);
		if ( !pInstrument )
		{
			SysMessageDefault(DEFMSG_MUSICANSHIP_NOTOOL);
			return -2;
		}
	}

	bool fSuccess = Skill_UseQuick(SKILL_MUSICIANSHIP, iDifficultyToPlay, (Skill_GetActive() != SKILL_MUSICIANSHIP));
	Sound(pInstrument->Use_Music(fSuccess));
	if ( fSuccess )
		return iDifficultyToPlay;	// success

	// Skill gain for SKILL_MUSICIANSHIP failure will need to be triggered
	// manually, since Skill_UseQuick isn't going to do it for us in this case
	if ( Skill_GetActive() == SKILL_MUSICIANSHIP )
		Skill_Experience(SKILL_MUSICIANSHIP, -iDifficultyToPlay);

	SysMessageDefault(DEFMSG_MUSICANSHIP_POOR);
	return -1;		// fail
}

bool CChar::Use_Repair( CItem * pItemArmor )
{
	ADDTOCALLSTACK("CChar::Use_Repair");
	// Attempt to repair the item.
	// If it is repairable.

	if ( !pItemArmor || !pItemArmor->Armor_IsRepairable() )
	{
		SysMessageDefault(DEFMSG_REPAIR_NOT);
		return false;
	}

	if ( pItemArmor->IsItemEquipped() )
	{
		SysMessageDefault(DEFMSG_REPAIR_WORN);
		return false;
	}
	if ( !CanUse(pItemArmor, true) )
	{
		SysMessageDefault(DEFMSG_REPAIR_REACH);
		return false;
	}

	// Quickly use arms lore skill, but don't gain any skill until later on
	int iArmsLoreDiff = Calc_GetRandVal(30);
	if ( !Skill_UseQuick(SKILL_ARMSLORE, iArmsLoreDiff, false) )
	{
		// apply arms lore skillgain for failure
		Skill_Experience(SKILL_ARMSLORE, -iArmsLoreDiff);
		SysMessageDefault(DEFMSG_REPAIR_UNK);
		return false;
	}

	if ( pItemArmor->m_itArmor.m_dwHitsCur >= pItemArmor->m_itArmor.m_wHitsMax )
	{
		SysMessageDefault(DEFMSG_REPAIR_FULL);
		return false;
	}

	m_Act_p = CWorldMap::FindItemTypeNearby(GetTopPoint(), IT_ANVIL, 2, false);
	if ( !m_Act_p.IsValidPoint() )
	{
		SysMessageDefault(DEFMSG_REPAIR_ANVIL);
		return false;
	}

	CItemBase *pItemDef = pItemArmor->Item_GetDef();
	ASSERT(pItemDef);

	// Use up some raw materials to repair.
	int iTotalHits = pItemArmor->m_itArmor.m_wHitsMax;
	int iDamageHits = pItemArmor->m_itArmor.m_wHitsMax - pItemArmor->m_itArmor.m_dwHitsCur;
	int iDamagePercent = IMulDiv(100, iDamageHits, iTotalHits);

	size_t iMissing = ResourceConsumePart(&(pItemDef->m_BaseResources), 1, iDamagePercent / 2, true);
	if ( iMissing != SCONT_BADINDEX )
	{
		// Need this to repair.
		const CResourceDef *pCompDef = g_Cfg.ResourceGetDef(pItemDef->m_BaseResources.at(iMissing).GetResourceID());
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_LACK_1), pCompDef ? pCompDef->GetName() : g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_LACK_2));
		return false;
	}

	UpdateDir(m_Act_p);
	UpdateAnimate(ANIM_ATTACK_1H_SLASH);

	// quarter the skill to make it.
	// + more damaged items should be harder to repair.
	// higher the percentage damage the closer to the skills to make it.

	size_t iRes = pItemDef->m_SkillMake.FindResourceType(RES_SKILL);
	if ( iRes == SCONT_BADINDEX )
		return false;

	CResourceQty RetMainSkill = pItemDef->m_SkillMake[iRes];
	int iSkillLevel = (int)(RetMainSkill.GetResQty()) / 10;
	int iDifficulty = IMulDiv(iSkillLevel, iDamagePercent, 100);
	if ( iDifficulty < iSkillLevel / 4 )
		iDifficulty = iSkillLevel / 4;

	// apply arms lore skillgain now
	lpctstr pszText;
	Skill_Experience(SKILL_ARMSLORE, iArmsLoreDiff);
	bool fSuccess = Skill_UseQuick((SKILL_TYPE)(RetMainSkill.GetResIndex()), iDifficulty);
	if ( fSuccess )
	{
		pItemArmor->m_itArmor.m_dwHitsCur = (word)(iTotalHits);
		pszText = g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_1);
	}
	else
	{
		/*****************************
		// not sure if this is working!
		******************************/
		// Failure
		if ( !Calc_GetRandVal(6) )
		{
			pszText = g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_2);
			-- pItemArmor->m_itArmor.m_wHitsMax;
			-- pItemArmor->m_itArmor.m_dwHitsCur;
		}
		else if ( !Calc_GetRandVal(3) )
		{
			pszText = g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_3);
			-- pItemArmor->m_itArmor.m_dwHitsCur;
		}
		else
			pszText = g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_4 );

		iDamagePercent = Calc_GetRandVal(iDamagePercent);	// some random amount
	}

	ResourceConsumePart(&(pItemDef->m_BaseResources), 1, iDamagePercent / 2, false);
	if ( pItemArmor->m_itArmor.m_dwHitsCur <= 0 )
		pszText = g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_5);

	tchar *pszMsg = Str_GetTemp();
	snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_MSG), pszText, pItemArmor->GetName());
	Emote(pszMsg);

	if ( pItemArmor->m_itArmor.m_dwHitsCur <= 0 )
		pItemArmor->Delete();
	else
		pItemArmor->UpdatePropertyFlag();

	return fSuccess;
}

void CChar::Use_EatQty( CItem * pFood, ushort uiQty )
{
	ADDTOCALLSTACK("CChar::Use_EatQty");
	// low level eat
	ASSERT(pFood);
	if ( uiQty <= 0 )
		return;

	if ( uiQty > pFood->GetAmount() )
        uiQty = pFood->GetAmount();

	ushort uiRestore = 0;
	if ( pFood->m_itFood.m_foodval )
		uiRestore = pFood->m_itFood.m_foodval;
	else
		uiRestore = pFood->Item_GetDef()->GetVolume();	// some food should have more value than other !

	if ( uiRestore < 1 )
		uiRestore = 1;

	int iSpace = Stat_GetMaxAdjusted(STAT_FOOD) - Stat_GetVal(STAT_FOOD);
	if ( iSpace <= 0 )
		return;

	if ( (uiQty > 1) && ((uiRestore * uiQty) > iSpace) )
    {
        uiQty = ushort(iSpace / uiRestore);
        uiQty = maximum(1, uiQty);
    }

	switch ( pFood->GetType() )
	{
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
			if ( pFood->m_itFood.m_poison_skill )	// was the food poisoned?
				SetPoison(pFood->m_itFood.m_poison_skill * 10, 1 + (pFood->m_itFood.m_poison_skill / 50), this);
		default:
			break;
	}

	UpdateDir(pFood);
	EatAnim(pFood->GetName(), uiRestore * uiQty);
	pFood->ConsumeAmount(uiQty);
}

bool CChar::Use_Eat( CItem * pItemFood, ushort uiQty )
{
	ADDTOCALLSTACK("CChar::Use_Eat");
	// What we can eat should depend on body type.
	// How much we can eat should depend on body size and current fullness.
	//
	// ??? monsters should be able to eat corpses / raw meat
	// IT_FOOD or IT_FOOD_RAW
	// NOTE: Some foods like apples are stackable !

	if ( !CanMove(pItemFood) )
	{
		SysMessageDefault(DEFMSG_FOOD_CANTMOVE);
		return false;
	}

    ushort uiFoodMax = Stat_GetMaxAdjusted(STAT_FOOD);
	if ( uiFoodMax == 0 )
	{
		SysMessageDefault(DEFMSG_FOOD_CANTEAT);
		return false;
	}

	// Is this edible by me ?
	if ( !Food_CanEat(pItemFood) )
	{
		SysMessageDefault(DEFMSG_FOOD_RCANTEAT);
		return false;
	}

	if ( Stat_GetVal(STAT_FOOD) >= uiFoodMax )
	{
		SysMessageDefault(DEFMSG_FOOD_CANTEATF);
		return false;
	}

	Use_EatQty(pItemFood, uiQty);

	lpctstr pMsg;
	int index = IMulDiv(Stat_GetVal(STAT_FOOD), 5, uiFoodMax);
	switch ( index )
	{
		case 0:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_1);
			break;
		case 1:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_2);
			break;
		case 2:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_3);
			break;
		case 3:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_4);
			break;
		case 4:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_5);
			break;
		case 5:
		default:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_6);
			break;
	}
	SysMessage(pMsg);
	return true;
}

void CChar::Use_Drink( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::Use_Drink");
	// IT_POTION:
	// IT_DRINK:
	// IT_PITCHER:
	// IT_WATER_WASH:
	// IT_BOOZE:

	if ( !CanMove(pItem) )
	{
		SysMessageDefault(DEFMSG_DRINK_CANTMOVE);
		return;
	}

	const CItemBase *pItemDef = pItem->Item_GetDef();
	ITEMID_TYPE idbottle = (ITEMID_TYPE)pItemDef->m_ttDrink.m_ridEmpty.GetResIndex();

	if ( pItem->IsType(IT_BOOZE) )
	{
		// Beer wine and liquor. vary strength of effect. m_itBooze.m_EffectStr
		int iStrength = Calc_GetRandVal(300) + 10;

		// Create ITEMID_PITCHER if drink a pitcher.
		// GLASS or MUG or Bottle ?

		// Get you Drunk, but check to see if we already are drunk
		CItem *pDrunkLayer = LayerFind(LAYER_FLAG_Drunk);
		if ( pDrunkLayer )
		{
			// lengthen/strengthen the effect
			Spell_Effect_Remove(pDrunkLayer);
			pDrunkLayer->m_itSpell.m_spellcharges += 10;
			if ( pDrunkLayer->m_itSpell.m_spelllevel < 500 )
				pDrunkLayer->m_itSpell.m_spelllevel += (word)(iStrength);
			Spell_Effect_Add(pDrunkLayer);
		}
		else
		{
			CItem *pSpell = Spell_Effect_Create(SPELL_Liquor, LAYER_FLAG_Drunk, g_Cfg.GetSpellEffect(SPELL_Liquor, iStrength), 150*MSECS_PER_TENTH, this);
			pSpell->m_itSpell.m_spellcharges = 10;	// how long to last.
		}
	}
	else if ( pItem->IsType(IT_POTION) )
	{
		// Time limit on using potions.
		if ( LayerFind(LAYER_FLAG_PotionUsed) )
		{
			SysMessageDefault(DEFMSG_DRINK_POTION_DELAY);
			return;
		}

		// Convey the effect of the potion.
		int iSkillQuality = pItem->m_itPotion.m_dwSkillQuality;
		int iEnhance = (int)GetPropNum(COMP_PROPS_CHAR, PROPCH_ENHANCEPOTIONS, true);
		if ( iEnhance )
			iSkillQuality += IMulDiv(iSkillQuality, iEnhance, 100);

		OnSpellEffect((SPELL_TYPE)(RES_GET_INDEX(pItem->m_itPotion.m_Type)), this, iSkillQuality, pItem);

		// Give me the marker that i've used a potion.
		Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_PotionUsed, g_Cfg.GetSpellEffect(SPELL_NONE, iSkillQuality), 150, this);
	}
	else if ( pItem->IsType(IT_DRINK) && IsSetOF(OF_DrinkIsFood) )
	{
		ushort uiRestore = 0;
		if ( pItem->m_itDrink.m_foodval )
			uiRestore = (ushort)(pItem->m_itDrink.m_foodval);
		else
			uiRestore = (ushort)(pItem->Item_GetDef()->GetVolume());

		if ( uiRestore < 1 )
			uiRestore = 1;

		if ( Stat_GetVal(STAT_FOOD) >= Stat_GetMaxAdjusted(STAT_FOOD) )
		{
			SysMessageDefault(DEFMSG_DRINK_FULL);
			return;
		}

		Stat_AddVal(STAT_FOOD, + uiRestore);
		if ( pItem->m_itFood.m_poison_skill )
			SetPoison(pItem->m_itFood.m_poison_skill * 10, 1 + (pItem->m_itFood.m_poison_skill / 50), this);
	}

	//Sound(sm_DrinkSounds[Calc_GetRandVal(CountOf(sm_DrinkSounds))]);
	UpdateAnimate(ANIM_EAT);
	pItem->ConsumeAmount();

	// Create the empty bottle ?
	if ( idbottle != ITEMID_NOTHING )
		ItemBounce(CItem::CreateScript(idbottle, this), false);
}

CChar * CChar::Use_Figurine( CItem * pItem, bool fCheckFollowerSlots )
{
	ADDTOCALLSTACK("CChar::Use_Figurine");
	// NOTE: The figurine is NOT destroyed.
	
	if ( !pItem )
		return nullptr;

	if ( pItem->m_uidLink.IsValidUID() && pItem->m_uidLink.IsChar() && (pItem->m_uidLink != GetUID()) && !IsPriv(PRIV_GM) )
	{
		SysMessageDefault(DEFMSG_MSG_FIGURINE_NOTYOURS);
		return nullptr;
	}

	// Create a new NPC if there's no one linked to this figurine
    bool fCreatedNewNpc = false;
	CChar *pPet = pItem->m_itFigurine.m_UID.CharFind();
	if ( !pPet )
	{
		CREID_TYPE id = pItem->m_itFigurine.m_ID;
		if ( !id )
		{
			id = CItemBase::FindCharTrack(pItem->GetID());
			if ( !id )
			{
				DEBUG_ERR(("FIGURINE id 0%x, no creature\n", pItem->GetDispID()));
				return nullptr;
			}
		}
		fCreatedNewNpc = true;
		pPet = CreateNPC(id);
		ASSERT(pPet);
		pPet->SetName(pItem->GetName());
        const HUE_TYPE iMountHue = pItem->GetHue();
		if (iMountHue)
		{
			pPet->_wPrev_Hue = iMountHue;
			pPet->SetHue(iMountHue);
		}
	}

	if ( fCheckFollowerSlots && IsSetOF(OF_PetSlots) )
	{
		const short iFollowerSlots = (short)pPet->GetDefNum("FOLLOWERSLOTS", true, 1);
		if ( !FollowersUpdate(pPet, (maximum(0, iFollowerSlots)), true) )
		{
			SysMessageDefault(DEFMSG_PETSLOTS_TRY_CONTROL);
			if ( fCreatedNewNpc )
				pPet->Delete();
			return nullptr;
		}
	}

	if ( pPet->IsDisconnected() )
		pPet->StatFlag_Clear(STATF_RIDDEN);		// pull the creature out of IDLE space

	pItem->m_itFigurine.m_UID.InitUID();
	pPet->m_dirFace = m_dirFace;
	pPet->NPC_PetSetOwner(this);
	pPet->MoveToChar(pItem->GetTopLevelObj()->GetTopPoint());
	pPet->Update();
	pPet->Skill_Start(SKILL_NONE);	// was NPCACT_RIDDEN
	pPet->SoundChar(CRESND_IDLE);
	return pPet;
}

bool CChar::FollowersUpdate( CChar * pChar, short iFollowerSlots, bool fCheckOnly )
{
	ADDTOCALLSTACK("CChar::FollowersUpdate");
	// Attemp to update followers on this character based on pChar
	// This is supossed to be called only when OF_PetSlots is enabled, so no need to check it here.

    if (!fCheckOnly && IsTrigUsed(TRIGGER_FOLLOWERSUPDATE))
    {
        CScriptTriggerArgs Args;
        Args.m_iN1 = (iFollowerSlots >= 0) ? 0 : 1;
        Args.m_iN2 = abs(iFollowerSlots);
        if (OnTrigger(CTRIG_FollowersUpdate, pChar, &Args) == TRIGRET_RET_TRUE)
            return false;

        if (Args.m_iN1 == 1)
        {
            iFollowerSlots = -(short)(Args.m_iN2);
        }
        else
        {
            iFollowerSlots = (short)(Args.m_iN2);
        }
	}

	short iCurFollower = (short)(GetDefNum("CURFOLLOWER", true));
	short iMaxFollower = (short)(GetDefNum("MAXFOLLOWER", true));
	short iSetFollower = iCurFollower + iFollowerSlots;

	if ( (iSetFollower > iMaxFollower) && !IsPriv(PRIV_GM) )
		return false;

	if ( !fCheckOnly )
	{
        SetDefNum("CURFOLLOWER", maximum(iSetFollower, 0));
		UpdateStatsFlag();
	}
	return true;
}

bool CChar::Use_Key( CItem * pKey, CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Use_Key");
	ASSERT(pKey);
	ASSERT(pKey->IsType(IT_KEY));
	if ( !pItemTarg )
	{
		SysMessageDefault(DEFMSG_MSG_KEY_TARG);
		return false;
	}

	if ( pKey != pItemTarg && pItemTarg->IsType(IT_KEY) )
	{
		// We are trying to copy a key ?
		if ( !CanUse(pItemTarg, true) )
		{
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_REACH);
			return false;
		}

		if ( !pKey->m_itKey.m_UIDLock.IsValidUID() && !pItemTarg->m_itKey.m_UIDLock.IsValidUID())
		{
			SysMessageDefault(DEFMSG_MSG_KEY_BLANKS);
			return false;
		}
		if ( pItemTarg->m_itKey.m_UIDLock.IsValidUID() && pKey->m_itKey.m_UIDLock.IsValidUID())
		{
			SysMessageDefault(DEFMSG_MSG_KEY_NOTBLANKS);
			return false;
		}

		// Need tinkering tools ???
		if ( !Skill_UseQuick(SKILL_TINKERING, 30 + Calc_GetRandLLVal(40)) )
		{
			SysMessageDefault(DEFMSG_MSG_KEY_FAILC);
			return false;
		}
		if ( pItemTarg->m_itKey.m_UIDLock.IsValidUID())
			pKey->m_itKey.m_UIDLock = pItemTarg->m_itKey.m_UIDLock;
		else
			pItemTarg->m_itKey.m_UIDLock = pKey->m_itKey.m_UIDLock;
		return true;
	}

	if ( !pKey->m_itKey.m_UIDLock.IsValidUID())
	{
		SysMessageDefault(DEFMSG_MSG_KEY_ISBLANK);
		return false;
	}
	if ( pKey == pItemTarg )	// rename the key
	{
		if ( IsClientActive() )
			GetClientActive()->addPromptConsole(CLIMODE_PROMPT_NAME_KEY, g_Cfg.GetDefaultMsg(DEFMSG_MSG_KEY_SETNAME), pKey->GetUID());
		return false;
	}

	if ( !CanUse(pItemTarg, false) )
	{
		SysMessageDefault(DEFMSG_MSG_KEY_CANTREACH);
		return false;
	}

	if ( m_pArea->GetResourceID().GetObjUID() == pKey->m_itKey.m_UIDLock.GetObjUID() )
	{
		if ( Use_MultiLockDown(pItemTarg) )
			return true;
	}

	if ( !pItemTarg->m_itContainer.m_UIDLock.IsValidUID())	// or m_itContainer.m_UIDLock
	{
		SysMessageDefault(DEFMSG_MSG_KEY_NOLOCK);
		return false;
	}
	if ( !pKey->IsKeyLockFit(pItemTarg->m_itContainer.m_UIDLock) )	// or m_itKey
	{
		SysMessageDefault(DEFMSG_MSG_KEY_WRONGLOCK);
		return false;
	}

	return Use_KeyChange(pItemTarg);
}

bool CChar::Use_KeyChange( CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Use_KeyChange");
	// Lock or unlock the item.
	switch ( pItemTarg->GetType() )
	{
		case IT_SIGN_GUMP:
			// We may rename the sign.
			if ( IsClientActive() )
				GetClientActive()->addPromptConsole(CLIMODE_PROMPT_NAME_SIGN, g_Cfg.GetDefaultMsg(DEFMSG_MSG_KEY_TARG_SIGN), pItemTarg->GetUID());
			return true;
		case IT_CONTAINER:
			pItemTarg->SetType(IT_CONTAINER_LOCKED);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_CONT_LOCK);
			break;
		case IT_CONTAINER_LOCKED:
			pItemTarg->SetType(IT_CONTAINER);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_CONT_ULOCK);
			break;
		case IT_SHIP_HOLD:
			pItemTarg->SetType(IT_SHIP_HOLD_LOCK);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_HOLD_LOCK);
			break;
		case IT_SHIP_HOLD_LOCK:
			pItemTarg->SetType(IT_SHIP_HOLD);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_HOLD_ULOCK);
			break;
		case IT_DOOR:
		case IT_DOOR_OPEN:
			pItemTarg->SetType(IT_DOOR_LOCKED);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_DOOR_LOCK);
			break;
		case IT_DOOR_LOCKED:
			pItemTarg->SetType(IT_DOOR);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_DOOR_ULOCK);
			break;
		case IT_SHIP_TILLER:
			if ( IsClientActive() )
				GetClientActive()->addPromptConsole(CLIMODE_PROMPT_NAME_SHIP, g_Cfg.GetDefaultMsg(DEFMSG_MSG_SHIPNAME_PROMT), pItemTarg->GetUID());
			return true;
		case IT_SHIP_PLANK:
			pItemTarg->Ship_Plank(false);	// just close it.
			if ( pItemTarg->GetType() == IT_SHIP_SIDE_LOCKED )
			{
				pItemTarg->SetType(IT_SHIP_SIDE);
				SysMessageDefault(DEFMSG_MSG_KEY_TARG_SHIP_ULOCK);
				break;
			}
			// Then fall thru and lock it.
		case IT_SHIP_SIDE:
			pItemTarg->SetType(IT_SHIP_SIDE_LOCKED);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_SHIP_LOCK);
			break;
		case IT_SHIP_SIDE_LOCKED:
			pItemTarg->SetType(IT_SHIP_SIDE);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_SHIP_ULOCK);
			break;
		default:
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_NOLOCK);
			return false;
	}

	pItemTarg->Sound(0x049);
	return true;
}

bool CChar::Use_Seed( CItem * pSeed, CPointMap * pPoint )
{
	ADDTOCALLSTACK("CChar::Use_Seed");
	// Use the seed at the current point on the ground or some new point that i can touch.
	// IT_SEED from IT_FRUIT

	ASSERT(pSeed);
	CPointMap pt;
	if ( pPoint )
		pt = *pPoint;
	else if ( pSeed->IsTopLevel() )
		pt = pSeed->GetTopPoint();
	else
		pt = GetTopPoint();

	if ( !CanTouch(pt) )
	{
		SysMessageDefault(DEFMSG_MSG_SEED_REACH);
		return false;
	}

	// is there soil here ? IT_DIRT
	if ( !IsPriv(PRIV_GM) && !CWorldMap::IsItemTypeNear(pt, IT_DIRT, 0, false) )
	{
		SysMessageDefault(DEFMSG_MSG_SEED_TARGSOIL);
		return false;
	}

	const CItemBase *pItemDef = pSeed->Item_GetDef();
	ITEMID_TYPE idReset = (ITEMID_TYPE)pItemDef->m_ttFruit.m_ridReset.GetResIndex();
	if ( idReset == 0 )
	{
		SysMessageDefault(DEFMSG_MSG_SEED_NOGOOD);
		return false;
	}

	// Already a plant here ?
	CWorldSearch AreaItems(pt);
	for (;;)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( pItem->IsType(IT_TREE) || pItem->IsType(IT_FOLIAGE) )		// there's already a tree here
		{
			SysMessageDefault(DEFMSG_MSG_SEED_ATREE);
			return false;
		}
		if ( pItem->IsType(IT_CROPS) )		// there's already a plant here
			pItem->Delete();
	}

	// plant it and consume the seed.

	CItem *pPlant = CItem::CreateScript(idReset, this);
	ASSERT(pPlant);

	pPlant->MoveToUpdate(pt);
	if ( pPlant->IsType(IT_CROPS) || pPlant->IsType(IT_FOLIAGE) )
		pPlant->Plant_CropReset();
	else
		pPlant->SetDecayTime(10 * g_Cfg.m_iDecay_Item);

	pSeed->ConsumeAmount();
	return true;
}

bool CChar::Use_BedRoll( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::Use_BedRoll");
	// IT_BEDROLL

	ASSERT(pItem);
	switch ( pItem->GetDispID() )
	{
		case ITEMID_BEDROLL_C:
			if ( !pItem->IsTopLevel() )
			{
			putonground:
				SysMessageDefault(DEFMSG_ITEMUSE_BEDROLL);
				return true;
			}
			pItem->SetID(Calc_GetRandVal(2) ? ITEMID_BEDROLL_O_EW : ITEMID_BEDROLL_O_NS);
			pItem->Update();
			return true;
		case ITEMID_BEDROLL_C_NS:
			if ( !pItem->IsTopLevel() )
				goto putonground;
			pItem->SetID(ITEMID_BEDROLL_O_NS);
			pItem->Update();
			return true;
		case ITEMID_BEDROLL_C_EW:
			if ( !pItem->IsTopLevel() )
				goto putonground;
			pItem->SetID(ITEMID_BEDROLL_O_EW);
			pItem->Update();
			return true;
		case ITEMID_BEDROLL_O_EW:
		case ITEMID_BEDROLL_O_NS:
			pItem->SetID(ITEMID_BEDROLL_C);
			pItem->Update();
			return true;
		default:
			return false;
	}
}

int CChar::Do_Use_Item(CItem *pItem, bool fLink)
{
	ADDTOCALLSTACK("CChar::Do_Use_Item");
	if (!pItem)
		return false;

	if (m_pNPC && (IsTrigUsed(TRIGGER_DCLICK) ||
	               IsTrigUsed(TRIGGER_ITEMDCLICK)))        // for players, DClick was called before this function
	{
		if (pItem->OnTrigger(ITRIG_DCLICK, this) == TRIGRET_RET_TRUE)
			return false;
	}

    CCSpawn *pSpawn = GetSpawn();
	if (pSpawn)
		pSpawn->DelObj(pItem->GetUID());    // remove this item from it's spawn when DClicks it

	int fAction = true;
	switch(pItem->GetType())
	{
		case IT_ITEM_STONE:
		{
			// Give them this item
			if (pItem->m_itItemStone.m_wAmount == UINT16_MAX)
			{
				SysMessageDefault(DEFMSG_MSG_IT_IS_DEAD);
				return true;
			}
			if (pItem->m_itItemStone.m_wRegenTime)
			{
				if (pItem->IsTimerSet())
				{
					SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_STONEREG_TIME), GetTimerSAdjusted());
					return true;
				}
				pItem->SetTimeoutS(pItem->m_itItemStone.m_wRegenTime);
			}
			ItemBounce(CItem::CreateTemplate(pItem->m_itItemStone.m_iItemID, GetPackSafe(), this));
			if (pItem->m_itItemStone.m_wAmount != 0)
			{
				--pItem->m_itItemStone.m_wAmount;
				if (pItem->m_itItemStone.m_wAmount == 0)
					pItem->m_itItemStone.m_wAmount = UINT16_MAX;
			}
			return true;
		}

		case IT_SEED:
			return Use_Seed(pItem, nullptr);

		case IT_BEDROLL:
			return Use_BedRoll(pItem);

		case IT_KINDLING:
			return Use_Kindling(pItem);

		case IT_SPINWHEEL:
		{
			if (fLink)
				return false;

			// Just make them spin
			dword id = pItem->GetDispID();
			switch (id)
			{
			case ITEMID_SPININGWHEEL_ELVEN_S:
				id = ITEMID_SPININGWHEEL_ELVEN_S_ANIMATED;
				break;
			case ITEMID_SPININGWHEEL_ELVEN_E:
				id = ITEMID_SPININGWHEEL_ELVEN_E_ANIMATED;
				break;
			default:
			case ITEMID_SPINWHEEL1:
			case ITEMID_SPINWHEEL2:
				id += 1;
				break;
			}
			pItem->SetAnim((ITEMID_TYPE)id, 2 * 1000);
			SysMessageDefault(DEFMSG_ITEMUSE_SPINWHEEL);
			return true;
		}

		case IT_TRAIN_DUMMY:
		{
			if (fLink)
				return false;

			Use_Train_Dummy(pItem, true);
			return true;
		}

		case IT_TRAIN_PICKPOCKET:
		{
			if (fLink)
				return false;

			Use_Train_PickPocketDip(pItem, true);
			return true;
		}

		case IT_ARCHERY_BUTTE:
		{
			if (fLink)
				return false;

			Use_Train_ArcheryButte(pItem, true);
			return true;
		}

		case IT_LOOM:
		{
			if (fLink)
				return false;

			SysMessageDefault(DEFMSG_ITEMUSE_LOOM);
			return true;
		}

		case IT_BEE_HIVE:
		{
			if (fLink)
				return false;

			// Get honey from it
			ITEMID_TYPE id = ITEMID_NOTHING;
			if (!pItem->m_itBeeHive.m_iHoneyCount)
				SysMessageDefault(DEFMSG_ITEMUSE_BEEHIVE);
			else
			{
				switch(Calc_GetRandVal(3))
				{
					case 1:
						id = ITEMID_JAR_HONEY;
						break;
					case 2:
						id = ITEMID_BEE_WAX;
						break;
				}
			}
			if (id)
			{
				ItemBounce(CItem::CreateScript(id, this));
				--pItem->m_itBeeHive.m_iHoneyCount;
			}
			else
            {
				SysMessageDefault(DEFMSG_ITEMUSE_BEEHIVE_STING);
				OnTakeDamage(Calc_GetRandVal(5), this, DAMAGE_POISON | DAMAGE_GENERAL);
			}
			pItem->SetTimeoutS(15 * 60);
			return true;
		}

		case IT_MUSICAL:
		{
			if (!Skill_Wait(SKILL_MUSICIANSHIP))
			{
				m_Act_UID = pItem->GetUID();
				Skill_Start(SKILL_MUSICIANSHIP);
			}
			break;
		}

		case IT_CROPS:
		case IT_FOLIAGE:
		{
			// Pick cotton/hay/etc
			fAction = pItem->Plant_Use(this);
			break;
		}

		case IT_FIGURINE:
		{
			// Create the creature here
			if (Use_Figurine(pItem) != nullptr)
				pItem->Delete();
			return true;
		}

		case IT_TRAP:
		case IT_TRAP_ACTIVE:
		{
			// Activate the trap (plus any linked traps)
			int iDmg = pItem->Use_Trap();
			if (CanTouch(pItem->GetTopLevelObj()->GetTopPoint()))
				OnTakeDamage(iDmg, nullptr, DAMAGE_HIT_BLUNT | DAMAGE_GENERAL);
			break;
		}

		case IT_SWITCH:
		{
			// Switches can be linked to gates and doors and such.
			// Flip the switch graphic.
			pItem->SetSwitchState();
			break;
		}

		case IT_PORT_LOCKED:
			if (!fLink && !IsPriv(PRIV_GM))
			{
				SysMessageDefault(DEFMSG_ITEMUSE_PORT_LOCKED);
				return true;
			}
			FALLTHROUGH;
		case IT_PORTCULIS:
			// Open a metal gate vertically
			pItem->Use_Portculis();
			break;

		case IT_DOOR_LOCKED:
			if (!ContentFindKeyFor(pItem))
			{
				SysMessageDefault(DEFMSG_MSG_KEY_DOORLOCKED);
				if (!pItem->IsTopLevel())
					return false;
				if (pItem->IsAttr(ATTR_MAGIC))    // show it's magic face
				{
					ITEMID_TYPE id = (GetDispID() & DOOR_NORTHSOUTH) ? ITEMID_DOOR_MAGIC_SI_NS
					                                                 : ITEMID_DOOR_MAGIC_SI_EW;
					CItem *pFace = CItem::CreateBase(id);
					ASSERT(pFace);
					pFace->MoveToDecay(pItem->GetTopPoint(), 4 * MSECS_PER_SEC);
				}
				if (!IsPriv(PRIV_GM))
					return true;
			}
			FALLTHROUGH;
		case IT_DOOR_OPEN:
		case IT_DOOR:
		{
			bool fOpen = pItem->Use_DoorNew(fLink);
			if (fLink || !fOpen)    // don't link if we are just closing the door
				return true;
		}
			break;

		case IT_SHIP_PLANK:
		{
			// Close the plank if I'm inside the ship
			if (m_pArea->IsFlag(REGION_FLAG_SHIP) && (m_pArea->GetResourceID().GetObjUID() == pItem->m_uidLink.GetObjUID()))
			{
				if (pItem->m_itShipPlank.m_wSideType == IT_SHIP_SIDE_LOCKED && !ContentFindKeyFor(pItem))
				{
					SysMessageDefault(DEFMSG_ITEMUSE_SHIPSIDE);
					return true;
				}
				return pItem->Ship_Plank(false);
			}
			else if (pItem->IsTopLevel())
			{
				// Teleport to plank if I'm outside the ship
				CPointMap pntTarg = pItem->GetTopPoint();
				++pntTarg.m_z;
				Spell_Teleport(pntTarg, true, false, false);
			}
			return true;
		}

		case IT_SHIP_SIDE_LOCKED:
			if (!IsPriv(PRIV_GM) && !ContentFindKeyFor(pItem))
			{
				SysMessageDefault(DEFMSG_ITEMUSE_SHIPSIDE);
				return true;
			}
			FALLTHROUGH;
		case IT_SHIP_SIDE:
			// Open the plank
			pItem->Ship_Plank(true);
			return true;

		case IT_GRAIN:
		case IT_GRASS:
		case IT_GARBAGE:
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
		{
			if (fLink)
				return false;

			Use_Eat(pItem);
			return true;
		}

		case IT_POTION:
		case IT_DRINK:
		case IT_PITCHER:
		case IT_WATER_WASH:
		case IT_BOOZE:
		{
			if (fLink)
				return false;

			Use_Drink(pItem);
			return true;
		}

		case IT_LIGHT_OUT:        // can the light be lit?
		case IT_LIGHT_LIT:        // can the light be doused?
			fAction = pItem->Use_Light();
			break;

		case IT_CLOTHING:
		case IT_ARMOR:
		case IT_ARMOR_LEATHER:
		case IT_SHIELD:
		case IT_WEAPON_MACE_CROOK:
		case IT_WEAPON_MACE_PICK:
		case IT_WEAPON_MACE_SMITH:
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_SWORD:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_BOW:
		case IT_WEAPON_AXE:
		case IT_WEAPON_XBOW:
		case IT_WEAPON_MACE_STAFF:
		case IT_JEWELRY:
		case IT_WEAPON_THROWING:
        case IT_WEAPON_WHIP:
		{
			if (fLink)
				return false;

			return ItemEquip(pItem);
		}

		case IT_WEB:
		{
			if (fLink)
				return false;

			Use_Item_Web(pItem);
			return true;
		}

		case IT_SPY_GLASS:
		{
			if (fLink)
				return false;

			// Spyglass will tell you the moon phases
			static lpctstr const sm_sPhases[8] =
					{
							g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M1),
							g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M2),
							g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M3),
							g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M4),
							g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M5),
							g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M6),
							g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M7),
							g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M8)
					};
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_TR), sm_sPhases[CWorldGameTime::GetMoonPhase(false)]);
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_FE), sm_sPhases[CWorldGameTime::GetMoonPhase(true)]);

			if (m_pArea && m_pArea->IsFlag(REGION_FLAG_SHIP))
				ObjMessage(pItem->Use_SpyGlass(this), this);
			return true;
		}

		case IT_SEXTANT:
		{
			if (fLink)
				return false;

			if ((GetTopPoint().m_map <= 1) && (GetTopPoint().m_x > UO_SIZE_X_REAL))    // dungeons and T2A lands
				ObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SEXTANT_T2A), this);
			else
			{
				tchar *pszMsg = Str_GetTemp();
				snprintf(pszMsg, STR_TEMPLENGTH, 
					g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SEXTANT), m_pArea->GetName(), pItem->Use_Sextant(GetTopPoint()));
				ObjMessage(pszMsg, this);
			}
			return true;
		}

		default:
			fAction = false;
	}
	return fAction | MASK_RETURN_FOLLOW_LINKS;
}

bool CChar::Use_Item(CItem *pItem, bool fLink)
{
	ADDTOCALLSTACK("CChar::Use_Item");
	int result = Do_Use_Item(pItem, fLink);
	if ((result & MASK_RETURN_FOLLOW_LINKS) == MASK_RETURN_FOLLOW_LINKS)
    {
		CItem *pLinkItem = pItem;
		for (int i = 0; i < 64; ++i)
		{ // dumb protection for endless loop
			pLinkItem = pLinkItem->m_uidLink.ItemFind();
			if (pLinkItem == nullptr || pLinkItem == pItem)
				break;
			result |= Do_Use_Item(pLinkItem, true);
		}
	}
	return (result & ~MASK_RETURN_FOLLOW_LINKS) ? true : false;
}

bool CChar::Use_Obj( CObjBase * pObj, bool fTestTouch, bool fScript  )
{
	ADDTOCALLSTACK("CChar::Use_Obj");
	if ( !pObj )
		return false;
	if ( IsClientActive() )
		return GetClientActive()->Event_DoubleClick(pObj->GetUID(), false, fTestTouch, fScript);
	else
		return Use_Item(dynamic_cast<CItem*>(pObj), fTestTouch);
}

bool CChar::ItemEquipArmor( bool fForce )
{
	ADDTOCALLSTACK("CChar::ItemEquipArmor");
	// Equip ourselves as best as possible.

	CItemContainer *pPack = GetPack();
	if ( !pPack || !Can(CAN_C_EQUIP) )
		return false;

    int iBestScore[LAYER_HORSE] = {};
    CItem *pBestArmor[LAYER_HORSE] = {};

	if ( !fForce )
	{
		// Block those layers that are already used
		for ( size_t i = 0; i < CountOf(iBestScore); ++i )
		{
			pBestArmor[i] = LayerFind((LAYER_TYPE)i);
			if ( pBestArmor[i] != nullptr )
				iBestScore[i] = INT32_MAX;
		}
	}

	for (CSObjContRec* pObjRec : *pPack)
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		int iScore = pItem->Armor_GetDefense();
		if ( !iScore )	// might not be armor
			continue;

		// Can I even equip this?
		LAYER_TYPE layer = CanEquipLayer(pItem, LAYER_QTY, nullptr, true);
		if ((layer == LAYER_NONE) || (layer >= LAYER_HORSE))
			continue;

		if ( iScore > iBestScore[layer] )
		{
			iBestScore[layer] = iScore;
			pBestArmor[layer] = pItem;
		}
	}

	// Equip all the stuff we found
	for ( size_t i = 0; i < CountOf(iBestScore); ++i )
	{
		if ( pBestArmor[i] )
			ItemEquip(pBestArmor[i], this);
	}

	return true;
}

bool CChar::ItemEquipWeapon( bool fForce )
{
	ADDTOCALLSTACK("CChar::ItemEquipWeapon");
	// Find my best weapon and equip it
	if ( !fForce && m_uidWeapon.IsValidUID() )	// we already have a weapon equipped
		return true;

	CItemContainer *pPack = GetPack();

	if ( !pPack || !Can(CAN_C_USEHANDS) )
		return false;

	// Loop through all my weapons and come up with a score for it's usefulness

	CItem *pBestWeapon = nullptr;
	int iWeaponScoreMax = NPC_GetWeaponUseScore(nullptr);	// wrestling

	for (CSObjContRec* pObjRec : *pPack)
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		int iWeaponScore = NPC_GetWeaponUseScore(pItem);
		if ( iWeaponScore > iWeaponScoreMax )
		{
			iWeaponScoreMax = iWeaponScore;
			pBestWeapon = pItem;
		}
	}

	if ( pBestWeapon )
		return ItemEquip(pBestWeapon);

	return true;
}
