// The physics calculations of the world.

#include "CServerConfig.h"
#include "chars/CChar.h"
#include "chars/CCharNPC.h"

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
	return( iQty * WEIGHT_UNITS );
}

//********************************
// Combat

int CServerConfig::Calc_CombatAttackSpeed( CChar * pChar, CItem * pWeapon )
{
	ADDTOCALLSTACK("CServerConfig::Calc_CombatAttackSpeed");
	// Calculate the swing speed value on chars
	// RETURN:
	//  Time in tenths of a sec. (for entire swing, not just time to hit)
	//  Should never return a value < 0 to avoid break combat timer/sequence

	ASSERT(pChar);
	if ( pChar->m_pNPC && pChar->m_pNPC->m_Brain == NPCBRAIN_GUARD && m_fGuardsInstantKill )
		return 1;

	int iSwingSpeedIncrease = (int)(pChar->GetDefNum("INCREASESWINGSPEED", true));
	int iBaseSpeed = 50;	// Base Wrestling speed (on ML formula it's 2.50)
	if ( pWeapon )			// If we have a weapon, base speed should match weapon's value.
		iBaseSpeed = pWeapon->GetSpeed();

	switch ( g_Cfg.m_iCombatSpeedEra )
	{
		case 0:
		{
			// pre-AOS formula (Sphere custom)		(default m_iSpeedScaleFactor = 15000, uses DEX instead STAM and calculate delay using weapon WEIGHT if weapon SPEED is not set)
			if ( pWeapon && iBaseSpeed )
			{
				int iSwingSpeed = maximum(1, (pChar->Stat_GetAdjusted(STAT_DEX) + 100) * iBaseSpeed);
				iSwingSpeed = (g_Cfg.m_iSpeedScaleFactor * TICK_PER_SEC) / iSwingSpeed;
				if ( iSwingSpeed < 5 )
					iSwingSpeed = 5;
				return iSwingSpeed;
			}

			int iSwingSpeed = MulDivLL(100 - pChar->Stat_GetAdjusted(STAT_DEX), 40, 100);	// base speed is just the char DEX range (0 ~ 40)
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
			return iSwingSpeed;
		}

		case 1:
		{
			// pre-AOS formula	(default m_iSpeedScaleFactor = 15000)
			int iSwingSpeed = maximum(1, (pChar->Stat_GetVal(STAT_DEX) + 100) * iBaseSpeed);
			iSwingSpeed = (g_Cfg.m_iSpeedScaleFactor * TICK_PER_SEC) / iSwingSpeed;
			if ( iSwingSpeed < 1 )
				iSwingSpeed = 1;
			return iSwingSpeed;
		}

		case 2:
		{
			// AOS formula		(default m_iSpeedScaleFactor = 40000)
			int iSwingSpeed = (pChar->Stat_GetVal(STAT_DEX) + 100) * iBaseSpeed;
			iSwingSpeed = maximum(1, iSwingSpeed * (100 + iSwingSpeedIncrease) / 100);
			iSwingSpeed = ((g_Cfg.m_iSpeedScaleFactor * TICK_PER_SEC) / iSwingSpeed) / 2;
			if ( iSwingSpeed < 12 )		//1.25
				iSwingSpeed = 12;
			return iSwingSpeed;
		}

		default:
		case 3:
		{
			// SE formula		(default m_iSpeedScaleFactor = 80000)
			int iSwingSpeed = maximum(1, iBaseSpeed * (100 + iSwingSpeedIncrease) / 100);
			iSwingSpeed = (g_Cfg.m_iSpeedScaleFactor / ((pChar->Stat_GetVal(STAT_DEX) + 100) * iSwingSpeed)) - 2;	// get speed in ticks of 0.25s each
			if ( iSwingSpeed < 5 )
				iSwingSpeed = 5;
			iSwingSpeed = (iSwingSpeed * TICK_PER_SEC) / 4;		// convert 0.25s ticks into ms
			return iSwingSpeed;
		}

		case 4:
		{
			// ML formula		(doesn't use m_iSpeedScaleFactor and it's only compatible with ML speed format eg. 0.25 ~ 5.00 instead 0 ~ 50)
			int iSwingSpeed = ((iBaseSpeed * 4) - (pChar->Stat_GetVal(STAT_DEX) / 30)) * (100 / (100 + iSwingSpeedIncrease));	// get speed in ticks of 0.25s each
			if ( iSwingSpeed < 5 )
				iSwingSpeed = 5;
			iSwingSpeed = (iSwingSpeed * TICK_PER_SEC) / 4;		// convert 0.25s ticks into ms
			return iSwingSpeed;
		}
		case 5:
		{
			//Heroes Shard Swing Speed Formula
			iSwingSpeedIncrease = maximum(-100, minimum(100, iSwingSpeedIncrease));
			int iSwingSpeed = (60 * (100 - iBaseSpeed))/100 - ((int)MulMulDiv(pChar->Stat_GetVal(STAT_DEX),100,pChar->Stat_GetMax(STAT_DEX)) * (100 + iSwingSpeedIncrease)) / 1000;
			return maximum(10,iSwingSpeed);
		}
	}
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
	if (pChar->m_pNPC && pChar->m_pNPC->m_Brain == NPCBRAIN_GUARD && m_fGuardsInstantKill)
		return 100;
	SKILL_TYPE skillAttacker = pChar->Fight_GetWeaponSkill();
	SKILL_TYPE skillTarget = pCharTarg->Fight_GetWeaponSkill();
	switch (m_iCombatHitChanceEra)
	{
		default:
		case 0:
		{
			// Sphere custom formula
			if (pCharTarg->IsStatFlag(STATF_Sleeping | STATF_Freeze))
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

			return(Calc_GetRandVal(iDiff));	// always need to have some chance. );
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
			int iAttackerHitChance = static_cast<int>(pChar->GetDefNum("INCREASEHITCHANCE", true));
			if ((g_Cfg.m_iRacialFlags & RACIALF_GARG_DEADLYAIM) && pChar->IsGargoyle())
			{
				// Racial traits: Deadly Aim. Gargoyles always have +5 Hit Chance Increase and a minimum of 20.0 Throwing skill (not shown in skills gump)
				if (skillAttacker == SKILL_THROWING && iAttackerSkill < 200)
					iAttackerSkill = 200;
				iAttackerHitChance += 5;
			}
			iAttackerSkill = ((iAttackerSkill / 10) + 20) * (100 + minimum(iAttackerHitChance, 45));
			int iTargetSkill = ((pCharTarg->Skill_GetBase(skillTarget) / 10) + 20) * (100 + minimum(static_cast<int>(pCharTarg->GetDefNum("INCREASEDEFCHANCE", true)), 45));

			int iChance = iAttackerSkill * 100 / (iTargetSkill * 2);
			if (iChance < 2)
				iChance = 2;	// minimum hit chance is 2%
			else if (iChance > 100)
				iChance = 100;
			return(iChance);
		}
		case 3:
		{
			//Heroes Hit Chance Formula
			int iAttackerBonus = 100 + maximum(-100,minimum(100,(int)pChar->GetDefNum("INCREASEHITCHANCE", true)));
			int iAttackerSkill = pChar->Skill_GetAdjusted(skillAttacker) * iAttackerBonus;

			int iDefenderBonus = 100 + maximum(-100,minimum(100,(int)pCharTarg->GetDefNum("INCREASEDEFCHANCE", true)));
			int iDefenderStat = pCharTarg->Stat_GetBase(STAT_DEX) * iDefenderBonus * 2;

			return (iAttackerSkill * 100) / (25000 + iAttackerSkill + iDefenderStat);
		}
	}
}

int CServerConfig::Calc_FameKill( CChar * pKill )
{
	ADDTOCALLSTACK("CServerConfig::Calc_FameKill");
	// Translate the fame for a Kill.

	int iFameChange = pKill->Stat_GetAdjusted(STAT_FAME);

	// Check if the victim is a PC, then higher gain/loss.
	if ( pKill->m_pPlayer )
		iFameChange /= 10;
	else
		iFameChange /= 200;

	return( iFameChange );
}

int CServerConfig::Calc_KarmaKill( CChar * pKill, NOTO_TYPE NotoThem )
{
	ADDTOCALLSTACK("CServerConfig::Calc_KarmaKill");
	// Karma change on kill ?

	int iKarmaChange = -pKill->Stat_GetAdjusted(STAT_KARMA);
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

	return( iKarmaChange );
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

	return( iKarmaChange );
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
	
	// int iDifficulty = iDexMark/2 + (iSkillMark/5) + Calc_GetRandVal(iDexMark/2) + MulDivLL( iWeightItem, 4, WEIGHT_UNITS );
	// Melt mod:
    int iDifficulty = (iSkillMark/5) + Calc_GetRandVal(iDexMark/2) + MulDivLL( iWeightItem, 4, WEIGHT_UNITS );
	
	if ( pItem->IsItemEquipped())
		iDifficulty += iDexMark/2 + pCharMark->Stat_GetAdjusted(STAT_INT);		// This is REALLY HARD to do.
	if ( pCharThief->IsStatFlag( STATF_War )) // all keyed up.
		iDifficulty += Calc_GetRandVal( iDexMark/2 );
	
	// return( iDifficulty );
	// Melt mod:
	return( iDifficulty / 2 );
}

bool CServerConfig::Calc_CrimeSeen( CChar * pCharThief, CChar * pCharViewer, SKILL_TYPE SkillToSee, bool fBonus )
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

	int iChanceToSee = ( pCharViewer->Stat_GetAdjusted(STAT_DEX) + pCharViewer->Stat_GetAdjusted(STAT_INT)) * 50;
	if ( SkillToSee != SKILL_NONE )
		iChanceToSee = 1000+(pCharViewer->Skill_GetBase(SkillToSee) - pCharThief->Skill_GetBase(SkillToSee));	// snooping or stealing.
	else
		iChanceToSee += 400;

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
	char *z = Str_GetTemp();
	CPointMap zeroPoint;
	zeroPoint.Read(strcpy(z, g_Cfg.m_sZeroPoint));

	int iLat = (pntCoords.m_y - zeroPoint.m_y) * 360 * 60 / g_MapList.GetY(zeroPoint.m_map);
	int iLong;
	if ( pntCoords.m_map <= 1 )
		iLong = (pntCoords.m_x - zeroPoint.m_x) * 360 * 60 / UO_SIZE_X_REAL;
	else
		iLong = (pntCoords.m_x - zeroPoint.m_x) * 360 * 60 / g_MapList.GetX(pntCoords.m_map);

	tchar * pTemp = Str_GetTemp();
	sprintf( pTemp, "%io %i'%s, %io %i'%s",
		abs(iLat / 60),  abs(iLat % 60),  (iLat <= 0) ? "N" : "S",
		abs(iLong / 60), abs(iLong % 60), (iLong >= 0) ? "E" : "W");

	return pTemp;
}
