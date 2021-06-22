// The physics calculations of the world.

#include "CServerConfig.h"
#include "CServerTime.h"
#include "chars/CChar.h"
#include "chars/CCharNPC.h"
#include "components/CCPropsChar.h"


//********************************
// Movement

int CServerConfig::Calc_MaxCarryWeight( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CServerConfig::Calc_MaxCarryWeight");
	// How much weight can i carry before i can carry no more. (and move at all)
	// Amount of weight that can be carried Max:
	// based on str 
	// RETURN: 
	//  Weight in tenths of stones i should be able to carry.

	ASSERT(pChar);
	int iQty = 40 + ( pChar->Stat_GetAdjusted(STAT_STR) * 35 / 10 ) + pChar->m_ModMaxWeight;
	if ( iQty < 0 )
		iQty = 0;
	if ( (m_iRacialFlags & RACIALF_HUMAN_STRONGBACK) && pChar->IsHuman())
		iQty += 60;		//Humans can always carry +60 stones (Strong Back racial trait)
	return (iQty * WEIGHT_UNITS);
}

//********************************
// Combat

int CServerConfig::Calc_CombatAttackSpeed( const CChar * pChar, const CItem * pWeapon ) const
{
	ADDTOCALLSTACK("CServerConfig::Calc_CombatAttackSpeed");
	// Calculate the swing speed value on chars
	// RETURN:
	//  Time in tenths of a sec. (for entire swing, not just time to hit)
	//  Should never return a value < 0 to avoid break combat timer/sequence

	ASSERT(pChar);
	if ( pChar->m_pNPC && (pChar->m_pNPC->m_Brain == NPCBRAIN_GUARD) && m_fGuardsInstantKill )
		return 1;

    const int iSpeedScaleFactor = g_Cfg.m_iSpeedScaleFactor;
	const int iSwingSpeedIncrease = (int)(pChar->GetPropNum(COMP_PROPS_CHAR, PROPCH_INCREASESWINGSPEED, true));
	int iBaseSpeed = 50;	// Base Wrestling speed (on ML formula it's 2.50)
	if ( pWeapon )			// If we have a weapon, base speed should match weapon's value.
		iBaseSpeed = pWeapon->GetSpeed();
    int iSwingSpeed = 100;
    
	switch ( g_Cfg.m_iCombatSpeedEra )
	{
		case 0:
		{
			// pre-AOS formula (Sphere custom)		(default m_iSpeedScaleFactor = 15000, uses DEX instead STAM and calculate delay using weapon WEIGHT if weapon SPEED is not set)
			if ( pWeapon && iBaseSpeed )
			{
				iSwingSpeed = (pChar->Stat_GetAdjusted(STAT_DEX) + 100) * iBaseSpeed;
				iSwingSpeed = maximum(1, iSwingSpeed);
				iSwingSpeed = (iSpeedScaleFactor * 10) / iSwingSpeed;
				if ( iSwingSpeed < 5 )
					iSwingSpeed = 5;
				break;
			}

			iSwingSpeed = IMulDiv(100 - pChar->Stat_GetAdjusted(STAT_DEX), 40, 100);	// base speed is just the char DEX range (0 ~ 40)
			if ( iSwingSpeed < 5 )
				iSwingSpeed = 5;
			else
				iSwingSpeed += 5;

			if ( pWeapon )
			{
				int iWeightMod = (pWeapon->GetWeight() * 10) / (4 * WEIGHT_UNITS);	// tenths of stone
				if ( pWeapon->GetEquipLayer() == LAYER_HAND2 )	// 2-handed weapons are slower
					iWeightMod += iSwingSpeed / 2;
				iSwingSpeed += iWeightMod;
			}
			else
				iSwingSpeed += 2;
			break;
		}

		case 1:
		{
			// pre-AOS formula	(default m_iSpeedScaleFactor = 15000)
			iSwingSpeed = (pChar->Stat_GetVal(STAT_DEX) + 100) * iBaseSpeed;
			iSwingSpeed = maximum(1, iSwingSpeed);
			iSwingSpeed = (iSpeedScaleFactor * 10) / iSwingSpeed;
			if ( iSwingSpeed < 1 )
				iSwingSpeed = 1;
			break;
		}

		case 2:
		{
			// AOS formula		(default m_iSpeedScaleFactor = 40000)
			iSwingSpeed = (pChar->Stat_GetVal(STAT_DEX) + 100) * iBaseSpeed;
			iSwingSpeed = iSwingSpeed * (100 + iSwingSpeedIncrease) / 100;
			iSwingSpeed = maximum(1, iSwingSpeed);
			iSwingSpeed = ((iSpeedScaleFactor * 10) / iSwingSpeed) / 2;
			if ( iSwingSpeed < 12 )		//1.25
				iSwingSpeed = 12;
			break;
		}

		default:
		case 3:
		{
			// SE formula		(default m_iSpeedScaleFactor = 80000)
			iSwingSpeed = iBaseSpeed * (100 + iSwingSpeedIncrease) / 100;
			iSwingSpeed = maximum(1, iSwingSpeed);
			iSwingSpeed = (iSpeedScaleFactor / ((pChar->Stat_GetVal(STAT_DEX) + 100) * iSwingSpeed)) - 2;	// get speed in ticks of 0.25s each
			if ( iSwingSpeed < 5 )
				iSwingSpeed = 5;
			iSwingSpeed = (iSwingSpeed * 10) / 4;		// convert tenths of second in 0.25s ticks (OSI ticking period)
			break;
		}

		case 4:
		{
			// ML formula		(doesn't use m_iSpeedScaleFactor and it's only compatible with ML speed format eg. 0.25 ~ 5.00 instead 0 ~ 50)
			iSwingSpeed = ((iBaseSpeed * 4) - (pChar->Stat_GetVal(STAT_DEX) / 30)) * (100 / (100 + iSwingSpeedIncrease));	// get speed in ticks of 0.25s each
			if ( iSwingSpeed < 5 )
				iSwingSpeed = 5;
			iSwingSpeed = (iSwingSpeed * 10) / 4;		// convert tenths of second in 0.25s ticks (OSI ticking period)
			break;
		}
	}
    return iSwingSpeed;
}

int CServerConfig::Calc_CombatChanceToHit(CChar * pChar, CChar * pCharTarg)
{
	ADDTOCALLSTACK("CServerConfig::Calc_CombatChanceToHit");
	// Combat: Compare attacker skill vs target skill
	// to calculate the hit chance on combat.
	//
	// RETURN:
	//  0-100 percent chance to hit.

	if (!pCharTarg)
		return 50;	// must be a training dummy
	if (pChar->m_pNPC && (pChar->m_pNPC->m_Brain == NPCBRAIN_GUARD) && m_fGuardsInstantKill)
		return 100;
	SKILL_TYPE skillAttacker = pChar->Fight_GetWeaponSkill();
	SKILL_TYPE skillTarget = pCharTarg->Fight_GetWeaponSkill();
	switch (m_iCombatHitChanceEra)
	{
		default:
		case 0:
		{
			// Sphere custom formula
			if (pCharTarg->IsStatFlag(STATF_SLEEPING | STATF_FREEZE))
				return(Calc_GetRandVal(10));

			int iSkillVal = pChar->Skill_GetAdjusted(skillAttacker);

			// Offensive value mostly based on your skill and TACTICS.
			// 0 - 1000
			int iSkillAttack = (iSkillVal + pChar->Skill_GetAdjusted(SKILL_TACTICS)) / 2;
			// int iSkillAttack = ( iSkillVal * 3 + pChar->Skill_GetAdjusted( SKILL_TACTICS )) / 4;

			// Defensive value mostly based on your tactics value and random DEX,
			// 0 - 1000
			int iSkillDefend = pCharTarg->Skill_GetAdjusted(SKILL_TACTICS);

			// Make it easier to hit people havin a bow or crossbow due to the fact that its
			// not a very "mobile" weapon, nor is it fast to change position while in
			// a fight etc. Just use 90% of the statvalue when defending so its easier
			// to hit than defend == more fun in combat.
			int iStam = pCharTarg->Stat_GetVal(STAT_DEX);
			if (g_Cfg.IsSkillFlag(skillTarget, SKF_RANGED) &&
				!g_Cfg.IsSkillFlag(skillAttacker, SKF_RANGED))
				// The defender uses ranged weapon and the attacker is not.
				// Make just a bit easier to hit.
				iSkillDefend = (iSkillDefend + iStam * 9) / 2;
			else
				// The defender is using a nonranged, or they both use bows.
				iSkillDefend = (iSkillDefend + iStam * 10) / 2;

			int iDiff = (iSkillAttack - iSkillDefend) / 5;

			iDiff = (iSkillVal - iDiff) / 10;
			if (iDiff < 0)
				iDiff = 0;	// just means it's very easy.
			else if (iDiff > 100)
				iDiff = 100;	// just means it's very hard.

			return Calc_GetRandVal(iDiff);	// always need to have some chance. );
		}
		case 1:
		{
			// pre-AOS formula
			int iAttackerSkill = pChar->Skill_GetBase(skillAttacker) + 500;
			int iTargetSkill = pCharTarg->Skill_GetBase(skillTarget) + 500;

			int iChance = iAttackerSkill * 100 / (iTargetSkill * 2);
			if (iChance < 0)
				iChance = 0;
			else if (iChance > 100)
				iChance = 100;
			return iChance;
		}
		case 2:
		{
			// AOS formula
			int iAttackerSkill = pChar->Skill_GetBase(skillAttacker);
			int iAttackerHitChance = (int)(pChar->GetPropNum(COMP_PROPS_CHAR, PROPCH_INCREASEHITCHANCE, true));
			if ((g_Cfg.m_iRacialFlags & RACIALF_GARG_DEADLYAIM) && pChar->IsGargoyle())
			{
				// Racial traits: Deadly Aim. Gargoyles always have +5 Hit Chance Increase and a minimum of 20.0 Throwing skill (not shown in skills gump)
				if (skillAttacker == SKILL_THROWING && iAttackerSkill < 200)
					iAttackerSkill = 200;
				iAttackerHitChance += 5;
			}
			iAttackerSkill = ((iAttackerSkill / 10) + 20) * (100 + minimum(iAttackerHitChance, 45));

			int iTargetIncreaseDefChance = (int)(pChar->GetPropNum(COMP_PROPS_CHAR, PROPCH_INCREASEDEFCHANCE, true));
			int iTargetSkill = ((pCharTarg->Skill_GetBase(skillTarget) / 10) + 20) * (100 + minimum(iTargetIncreaseDefChance, 45));

			int iChance = iAttackerSkill * 100 / (iTargetSkill * 2);
			if (iChance < 2)
				iChance = 2;	// minimum hit chance is 2%
			else if (iChance > 100)
				iChance = 100;
			return iChance;
		}
	}
}

int CServerConfig::Calc_CombatChanceToParry(CChar* pChar, CItem*& pItemParry)
{
	ADDTOCALLSTACK("CServerConfig::Calc_CombatChanceToParry");
	// Check if target will block the hit
	// Legacy pre-SE formula
	const bool fCanShield = g_Cfg.m_iCombatParryingEra & PARRYERA_SHIELDBLOCK;
	const bool fCanOneHanded = g_Cfg.m_iCombatParryingEra & PARRYERA_ONEHANDBLOCK;
	const bool fCanTwoHanded = g_Cfg.m_iCombatParryingEra & PARRYERA_TWOHANDBLOCK;

	const int iParrying = pChar->Skill_GetBase(SKILL_PARRYING);
	/*
	While the difficulty range is 0-100 (without decimal) we initialize iParryChance to -1 for avoiding the player
	to gain parrying skill when his combination of weapon/shield does not match the values set in the CombatParryingEra  in the sphere.ini.
	*/
	int iParryChance = -1;
	if (g_Cfg.m_iFeatureSE & FEATURE_SE_NINJASAM && g_Cfg.m_iCombatParryingEra & PARRYERA_SEFORMULA )   // Samurai Empire formula
	{
		const int iBushido = pChar->Skill_GetBase(SKILL_BUSHIDO);
		int iChanceSE = 0, iChanceLegacy = 0;

		if (fCanShield && pChar->IsStatFlag(STATF_HASSHIELD))	// parry using shield
		{
			pItemParry = pChar->LayerFind(LAYER_HAND2);
			iParryChance = (iParrying - iBushido) / 40;
			if ((iParrying >= 1000) || (iBushido >= 1000))
				iParryChance += 5;
			if (iParryChance < 0)
				iParryChance = 0;
		}
		else if (pChar->m_uidWeapon.IsItem())		// parry using weapon
		{
			CItem* pTempItemParry = pChar->m_uidWeapon.ItemFind();
			if (fCanOneHanded && (pTempItemParry->GetEquipLayer() == LAYER_HAND1))
			{
				pItemParry = pTempItemParry;

				iChanceSE = iParrying * iBushido / 48000;
				if ((iParrying >= 1000) || (iBushido >= 1000))
					iChanceSE += 5;

				iChanceLegacy = iParrying / 80;
				if (iParrying >= 1000)
					iChanceLegacy += 5;

				iParryChance = maximum(iChanceSE, iChanceLegacy);
			}
			else if (fCanTwoHanded && (pTempItemParry->GetEquipLayer() == LAYER_HAND2))
			{
				pItemParry = pTempItemParry;

				iChanceSE = iParrying * iBushido / 41140;
				if ((iParrying >= 1000) || (iBushido >= 1000))
					iChanceSE += 5;

				iChanceLegacy = iParrying / 80;
				if (iParrying >= 1000)
					iChanceLegacy += 5;

				iParryChance = maximum(iChanceSE, iChanceLegacy);
			}
		}
	}
	else    // Legacy formula (pre Samurai Empire)
	{
		if (fCanShield && pChar->IsStatFlag(STATF_HASSHIELD))	// parry using shield
		{
			pItemParry = pChar->LayerFind(LAYER_HAND2);
			iParryChance = iParrying / 40;
		}
		else if (pChar->m_uidWeapon.IsItem())		// parry using weapon
		{
			CItem* pTempItemParry = pChar->m_uidWeapon.ItemFind();
			if ((fCanOneHanded && (pTempItemParry->GetEquipLayer() == LAYER_HAND1)) ||
				(fCanTwoHanded && (pTempItemParry->GetEquipLayer() == LAYER_HAND2)))
			{
				pItemParry = pTempItemParry;
				iParryChance = iParrying / 80;
			}
		}

		if ((iParryChance > 0) && (iParrying >= 1000))
			iParryChance += 5;
	}
	/*
	Had to replace <= with < otherwise we will never be able to increase the parrying  skill if the Parrying skill is too low.
	For example, without a shield and using the legacy formula we will not be able to get a gain in the skill if the character
	Parrying skill is less than 8.0.
	*/
	if (iParryChance < 0)
		return 0;

	int iDex = pChar->Stat_GetAdjusted(STAT_DEX);
	if (iDex < 80)
	{
		const float fDexMod = (80 - iDex) / 100.0f;
		iParryChance = int((float)iParryChance * (1.0f - fDexMod));
	}

	return iParryChance;

}

int CServerConfig::Calc_FameKill( CChar * pKill )
{
	ADDTOCALLSTACK("CServerConfig::Calc_FameKill");
	// Translate the fame for a Kill.

	int iFameChange = pKill->GetFame();

	// Check if the victim is a PC, then higher gain/loss.
	if ( pKill->m_pPlayer )
		iFameChange /= 10;
	else
		iFameChange /= 200;

	return iFameChange;
}

int CServerConfig::Calc_KarmaKill( CChar * pKill, NOTO_TYPE NotoThem )
{
	ADDTOCALLSTACK("CServerConfig::Calc_KarmaKill");
	// Karma change on kill ?

	int iKarmaChange = -(pKill->GetKarma());
	if ( NotoThem >= NOTO_CRIMINAL )
	{
		// No bad karma for killing a criminal or my aggressor.
		if ( iKarmaChange < 0 )
			iKarmaChange = 0;
	}
		
	// Check if the victim is a PC, then higher gain/loss.
	if ( pKill->m_pPlayer )
	{
		// If killing a 'good' PC we should always loose at least
		// 500 karma
		if ( iKarmaChange < 0 && iKarmaChange >= -5000 )
			iKarmaChange = -5000;

		iKarmaChange /= 10;
	}
	else	// Or is it was a NPC, less gain/loss.
	{
		// Always loose at least 20 karma if you kill a 'good' NPC
		if ( iKarmaChange < 0 && iKarmaChange >= -1000 )
			iKarmaChange = -1000;

		iKarmaChange /= 20;	// Not as harsh penalty as with player chars.
	}

	return iKarmaChange;
}

int CServerConfig::Calc_KarmaScale( int iKarma, int iKarmaChange )
{
	ADDTOCALLSTACK("CServerConfig::Calc_KarmaScale");
	// Scale the karma based on the current level.
	// Should be harder to gain karma than to loose it.

	if ( iKarma > 0 )
	{
		// Your a good guy. Harder to be a good guy.
		if ( iKarmaChange < 0 )
			iKarmaChange *= 2;	// counts worse against you.
		else
			iKarmaChange /= 2;	// counts less for you.
	}

	// Scale the karma at higher levels.
	if ( iKarmaChange > 0 && iKarmaChange < iKarma/64 )
		return 0;

	return iKarmaChange;
}

//********************************
// Stealing

int CServerConfig::Calc_StealingItem( CChar * pCharThief, CItem * pItem, CChar * pCharMark )
{
	ADDTOCALLSTACK("CServerConfig::Calc_StealingItem");
	// Chance to steal and retrieve the item successfully.
	// weight of the item
	//  heavier items should be more difficult.
	//	thiefs skill/ dex
	//  marks skill/dex
	//  marks war mode ?
	// NOTE:
	//  Items on the ground can always be stolen. chance of being seen is another matter.
	// RETURN:
	//  0-100 percent chance to hit on a d100 roll.
	//  0-100 percent difficulty against my SKILL_STEALING skill.

	ASSERT(pCharThief);
	ASSERT(pCharMark);

	int iDexMark = pCharMark->Stat_GetAdjusted(STAT_DEX);
	int iSkillMark = pCharMark->Skill_GetAdjusted( SKILL_STEALING );
	int iWeightItem = pItem->GetWeight();
	
	// int iDifficulty = iDexMark/2 + (iSkillMark/5) + Calc_GetRandVal(iDexMark/2) + IMulDivLL( iWeightItem, 4, WEIGHT_UNITS );
	// Melt mod:
    int iDifficulty = (iSkillMark/5) + Calc_GetRandVal(iDexMark/2) + IMulDiv( iWeightItem, 4, WEIGHT_UNITS );
	
	if ( pItem->IsItemEquipped())
		iDifficulty += iDexMark/2 + pCharMark->Stat_GetAdjusted(STAT_INT);		// This is REALLY HARD to do.
	if ( pCharThief->IsStatFlag( STATF_WAR )) // all keyed up.
		iDifficulty += Calc_GetRandVal( iDexMark/2 );
	
	// return( iDifficulty );
	// Melt mod:
	return (iDifficulty / 2);
}

bool CServerConfig::Calc_CrimeSeen( const CChar * pCharThief, const CChar * pCharViewer, SKILL_TYPE SkillToSee, bool fBonus ) const
{
	ADDTOCALLSTACK("CServerConfig::Calc_CrimeSeen");
	// Chance to steal without being seen by a specific person
	//	weight of the item
	//	distance from crime. (0=i am the mark)
	//	Thiefs skill/ dex
	//  viewers skill

	if ( SkillToSee == SKILL_NONE )	// takes no skill.
		return true;

	ASSERT(pCharViewer);
	ASSERT(pCharThief);

	if ( pCharViewer->IsPriv(PRIV_GM) || pCharThief->IsPriv(PRIV_GM))
	{
		if ( pCharViewer->GetPrivLevel() < pCharThief->GetPrivLevel())
			return false;	// never seen
		if ( pCharViewer->GetPrivLevel() > pCharThief->GetPrivLevel())
			return true;		// always seen.
	}

	int iChanceToSee;
	if ( (SkillToSee == SKILL_SNOOPING) || (SkillToSee == SKILL_STEALING) )
		iChanceToSee = 1000 + (pCharViewer->Skill_GetBase(SkillToSee) - pCharThief->Skill_GetBase(SkillToSee));
	else
		iChanceToSee = 400 + ( pCharViewer->Stat_GetAdjusted(STAT_DEX) + pCharViewer->Stat_GetAdjusted(STAT_INT)) * 50;

	// the targets chance of seeing.
	if ( fBonus )
	{
		// Up by 30 % if it's me.
		iChanceToSee += iChanceToSee/3;
		if ( iChanceToSee < 50 ) // always atleast 5% chance.
			iChanceToSee=50;
	}
	else
	{
		// the bystanders chance of seeing.
		if ( iChanceToSee < 10 ) // always atleast 1% chance.
			iChanceToSee=10;
	}

	if ( Calc_GetRandVal(1000) > iChanceToSee )
		return false;

	return true;
}

lpctstr CServerConfig::Calc_MaptoSextant( CPointMap pntCoords )
{
	ADDTOCALLSTACK("CServerConfig::Calc_MaptoSextant");
	// Conversion from map square to degrees, minutes
	tchar *z = Str_GetTemp();
	Str_CopyLimitNull(z, g_Cfg.m_sZeroPoint.GetBuffer(), STR_TEMPLENGTH);
	CPointMap zeroPoint(z);

	int iLat = (pntCoords.m_y - zeroPoint.m_y) * 360 * 60 / g_MapList.GetMapSizeY(zeroPoint.m_map);
	int iLong;
	if ( pntCoords.m_map <= 1 )
		iLong = (pntCoords.m_x - zeroPoint.m_x) * 360 * 60 / UO_SIZE_X_REAL;
	else
		iLong = (pntCoords.m_x - zeroPoint.m_x) * 360 * 60 / g_MapList.GetMapSizeX(pntCoords.m_map);

	tchar * pTemp = Str_GetTemp();
	snprintf( pTemp, STR_TEMPLENGTH, "%io %i'%s, %io %i'%s",
		abs(iLat / 60),  abs(iLat % 60),  (iLat <= 0) ? "N" : "S",
		abs(iLong / 60), abs(iLong % 60), (iLong >= 0) ? "E" : "W");

	return pTemp;
}

ushort CServerConfig::Calc_SpellManaCost(CChar* pCharCaster, const CSpellDef* pSpell, CObjBase* pObj)
{
	ADDTOCALLSTACK("CServerConfig::Calc_SpellManaCost");

	ASSERT(pCharCaster);
	ASSERT(pSpell);

	bool fScroll = false;
	if (pObj != pCharCaster)
	{
		const CItem* pItem = dynamic_cast <const CItem*> (pObj);
		if (pItem)
		{
			const IT_TYPE iType = pItem->GetType();
			if (iType == IT_WAND)
				return 0; //Spells from wands cost no mana.
			else if (iType == IT_SCROLL)
				fScroll = true;
		}
	}

	const CCPropsChar* pCCPChar = pCharCaster->GetComponentProps<CCPropsChar>();
	const CCPropsChar* pBaseCCPChar = pCharCaster->Base_GetDef()->GetComponentProps<CCPropsChar>();
	const int iLowerManaCost = (int)pCharCaster->GetPropNum(pCCPChar, PROPCH_LOWERMANACOST, pBaseCCPChar);
	ushort iCost = (ushort)(pSpell->m_wManaUse * ((100 - iLowerManaCost) / 100));
	
	if ( fScroll )
		return iCost / 2; //spells cast from scrolls consume half of the mana.
	
	return iCost;
}

size_t CServerConfig::Calc_SpellReagentsConsume(CChar* pCharCaster, const CSpellDef* pSpell, CObjBase* pObj, bool fTest)
{
	ADDTOCALLSTACK("CServerConfig::Calc_SpellReagentsConsume");
	
	ASSERT(pCharCaster);
	ASSERT(pSpell);

	//Check for reagents
	if (g_Cfg.m_fReagentsRequired && !pCharCaster->m_pNPC && (pObj == pCharCaster))
	{
		const CResourceQtyArray* pReagents = &(pSpell->m_Reags);
		const CCPropsChar* pCCPChar = pCharCaster->GetComponentProps<CCPropsChar>();
		const CCPropsChar* pBaseCCPChar = pCharCaster->Base_GetDef()->GetComponentProps<CCPropsChar>();
		const int iLowerReagentCost = (int)pCharCaster->GetPropNum(pCCPChar, PROPCH_LOWERREAGENTCOST, pBaseCCPChar); //Also used for reducing Tithing points.
		if ( Calc_GetRandVal(100) >= iLowerReagentCost)
		{
			CContainer* pCont = static_cast<CContainer*>(pCharCaster);
			const size_t iMissing = pCont->ResourceConsumePart(pReagents, 1, 100, fTest);
			if (iMissing != SCONT_BADINDEX)
				return iMissing;
		}
	}
	return SCONT_BADINDEX;
}

ushort CServerConfig::Calc_SpellTithingCost(CChar* pCharCaster, const CSpellDef* pSpell, CObjBase* pObj)
{
	ADDTOCALLSTACK("CServerConfig::Calc_SpellTithingCost");

	ASSERT(pCharCaster);
	ASSERT(pSpell);

	//Check for tithing points.
	if (g_Cfg.m_fReagentsRequired && !pCharCaster->m_pNPC && (pObj == pCharCaster))
	{
		
		const CCPropsChar* pCCPChar = pCharCaster->GetComponentProps<CCPropsChar>();
		const CCPropsChar* pBaseCCPChar = pCharCaster->Base_GetDef()->GetComponentProps<CCPropsChar>();
		const int iLowerReagentCost = (int)pCharCaster->GetPropNum(pCCPChar, PROPCH_LOWERREAGENTCOST, pBaseCCPChar); //Also used for reducing Tithing points.
		if (Calc_GetRandVal(100) >= iLowerReagentCost)
			return (ushort)pSpell->m_wTithingUse; //Default amount of Tithing points consumed.
	}
	return 0; //No tithing points consumed.
}

bool CServerConfig::Calc_CurePoisonChance(const CItem* pPoison, int iCureLevel, bool fIsGm)
{
	ADDTOCALLSTACK("CServerConfig::Calc_CurePoisonChance");

	if (!pPoison)
		return false;

	if (fIsGm)
		return true;

	int iCureChance = 0, iPoisonLevel = pPoison->m_itSpell.m_spelllevel;

	//Override the Cure Poison Chance.
	const CVarDefCont* pTagStorage = pPoison->GetKey("OVERRIDE.CUREPOISONCHANCE", true);
	if (pTagStorage)
	{
		iCureChance = (int)pTagStorage->GetValNum();
		return (Calc_GetRandVal(100) <= iCureChance);
	}

	if (!IsSetMagicFlags(MAGICF_OSIFORMULAS))
	{
		iCureChance = Calc_GetSCurve(iCureLevel - iPoisonLevel, 100);
		return (Calc_GetRandVal(1000) <= iCureChance);
	}
	//If we use MAGICF_OSIFORMULAS, the poison level is in the 0-4+ range.
	if (!iPoisonLevel) //Lesser Poison (iPoisonLevel 0) is always cured no matter the potion or spell/skill level value
		return true;

	//Cure Chance taken from: 
	if (iCureLevel < 410)	//Lesser Cure Potion or our healing/veterinary/magery skill is less than 41.0 https://www.uoguide.com/Lesser_Cure_Potion
	{
		switch (iPoisonLevel)
		{
		case 1:
			iCureChance = 35;
			break;
		case 2:
			iCureChance = 15;
			break;
		case 3:
			iCureChance = 10;
			break;
		default:
			iCureChance = 5;
			break;
		}
	}
	else if (iCureLevel < 1010) //Cure Potion or our healing/veterinary/magery skill is between 41.0 and 100.9 https://www.uoguide.com/Cure_Potion 
	{
		switch (iPoisonLevel)
		{
		case 1:
			iCureChance = 95;
			break;
		case 2:
			iCureChance = 45;
			break;
		case 3:
			iCureChance = 25;
			break;
		default:
			iCureChance = 15;
			break;
		}
	}
	else //Greater Cure Potion or our healing/veterinary/magery skill is equal or above 101.0 https://www.uoguide.com/Greater_Cure_Potion
	{
		switch (iPoisonLevel)
		{
		case 1:
			iCureChance = 100;
			break;
		case 2:
			iCureChance = 75;
			break;
		case 3:
			iCureChance = 45;
			break;
		default:
			iCureChance = 25;
			break;
		}
	}

	return (Calc_GetRandVal(100) <= iCureChance);
}