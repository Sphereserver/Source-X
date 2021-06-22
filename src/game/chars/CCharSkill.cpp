
//  CChar is either an NPC or a Player.

#include <cmath>
#include "../../common/resource/sections/CSkillClassDef.h"
#include "../../common/resource/sections/CRegionResourceDef.h"
#include "../../common/resource/CResourceLock.h"
#include "../clients/CClient.h"
#include "../items/CItemVendable.h"
#include "../triggers.h"
#include "../CLog.h"
#include "../CServer.h"
#include "../CWorldMap.h"
#include "CChar.h"
#include "CCharNPC.h"


//----------------------------------------------------------------------
// Skills

SKILL_TYPE CChar::Skill_GetBest( uint iRank ) const
{
	ADDTOCALLSTACK("CChar::Skill_GetBest");
	// Get the top n best skills.

	if ( iRank >= g_Cfg.m_iMaxSkill )
		iRank = 0;

	dword * pdwSkills = new dword [(size_t)iRank + 1]();
	ASSERT(pdwSkills);

	dword dwSkillTmp;
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
			continue;

		dwSkillTmp = MAKEDWORD(i, Skill_GetBase((SKILL_TYPE)i));
		for ( size_t j = 0; j <= iRank; ++j )
		{
			if ( HIWORD(dwSkillTmp) >= HIWORD(pdwSkills[j]) )
			{
				memmove( &pdwSkills[j + 1], &pdwSkills[j], (iRank - j) * sizeof(dword) );
				pdwSkills[j] = dwSkillTmp;
				break;
			}
		}
	}

	dwSkillTmp = pdwSkills[ iRank ];
	delete[] pdwSkills;
	return (SKILL_TYPE)(LOWORD( dwSkillTmp ));
}

// Retrieves a random magic skill, if iVal is set it will only select from the ones with value > iVal
SKILL_TYPE CChar::Skill_GetMagicRandom(ushort uiVal)
{
	ADDTOCALLSTACK("CChar::Skill_GetMagicRandom");
	SKILL_TYPE skills[SKILL_QTY];
	int count = 0;
	for ( uchar i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		SKILL_TYPE skill = (SKILL_TYPE)i;
		if (!g_Cfg.IsSkillFlag(skill, SKF_MAGIC))
			continue;

		if (Skill_GetBase(skill) < uiVal)
			continue;

		++count;
		skills[(SKILL_TYPE)(count)] = skill;
	}
	if ( count )
		return skills[(SKILL_TYPE)(Calc_GetRandVal2(0, count))];

	return SKILL_NONE;
}

bool CChar::Skill_CanUse( SKILL_TYPE skill )
{
	ADDTOCALLSTACK("CChar::Skill_CanUse");
	if ( g_Cfg.IsSkillFlag( skill, SKF_DISABLED ) )
	{
		SysMessageDefault(DEFMSG_SKILL_NOSKILL);
		return false;
	}
	// Expansion checks? different flags for NPCs/Players?
	return true;
}

SKILL_TYPE CChar::Skill_GetMagicBest()
{
	ADDTOCALLSTACK("CChar::Skill_GetMagicBest");
	SKILL_TYPE skill = SKILL_NONE;
	ushort value = 0;
	for ( uchar i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		if (!g_Cfg.IsSkillFlag(skill, SKF_MAGIC))
			continue;

		SKILL_TYPE test = (SKILL_TYPE)i;
		ushort uiVal = Skill_GetBase(test);
		if (uiVal > value)
		{
			skill = test;
			value = uiVal;
		}
	}
	return skill;
}

SKILLLOCK_TYPE CChar::Skill_GetLock( SKILL_TYPE skill ) const
{
	if ( ! m_pPlayer )
		return( SKILLLOCK_UP );
	return( m_pPlayer->Skill_GetLock(skill));
}

ushort CChar::Skill_GetAdjusted( SKILL_TYPE skill ) const
{
	ADDTOCALLSTACK("CChar::Skill_GetAdjusted");
	// Get the skill adjusted for str,dex,int = 0-1000

	// m_SkillStat is used to figure out how much
	// of the total bonus comes from the stats
	// so if it's 80, then 20% (100% - 80%) comes from
	// the stat (str,int,dex) bonuses

	// example:

	// These are the cchar's stats:
	// m_Skill[x] = 50.0
	// m_Stat[str] = 50, m_Stat[int] = 30, m_Stat[dex] = 20

	// these are the skill "defs":
	// m_SkillStat = 80
	// m_StatBonus[str] = 50
	// m_StatBonus[int] = 50
	// m_StatBonus[dex] = 0

	// Pure bonus is:
	// 50% of str (25) + 50% of int (15) = 40

	// Percent of pure bonus to apply to raw skill is
	// 20% = 100% - m_SkillStat = 100 - 80

	// adjusted bonus is: 8 (40 * 0.2)

	// so the effective skill is 50 (the raw) + 8 (the bonus)
	// which is 58 in total.

	ASSERT( IsSkillBase( skill ));
	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef( skill );
	ushort uiAdjSkill = 0;

	if (pSkillDef != nullptr)
	{
		uint uiPureBonus =( pSkillDef->m_StatBonus[STAT_STR] * Stat_GetAdjusted(STAT_STR) ) +
							( pSkillDef->m_StatBonus[STAT_INT] * Stat_GetAdjusted(STAT_INT) ) +
							( pSkillDef->m_StatBonus[STAT_DEX] * Stat_GetAdjusted(STAT_DEX) );

		uiAdjSkill = (ushort)IMulDiv( pSkillDef->m_StatPercent, uiPureBonus, 10000 );
	}

	return ( Skill_GetBase(skill) + uiAdjSkill );
}

void CChar::Skill_AddBase( SKILL_TYPE skill, int iChange )
{
    ADDTOCALLSTACK("CChar::Skill_AddBase");
    ASSERT( IsSkillBase(skill));

    int iNewVal = m_Skill[skill] + iChange;
    if (iNewVal < 0)
        iNewVal = 0;
    else if (iNewVal > UINT16_MAX)
        iNewVal = UINT16_MAX;
    Skill_SetBase(skill, (ushort)iNewVal);
}

void CChar::Skill_SetBase( SKILL_TYPE skill, ushort uiValue )
{
	ADDTOCALLSTACK("CChar::Skill_SetBase");
	ASSERT( IsSkillBase(skill));

	bool fUpdateStats = false;
	if ( IsTrigUsed(TRIGGER_SKILLCHANGE) )
	{
		CScriptTriggerArgs args;
		args.m_iN1 = (int64)skill;
		args.m_iN2 = (int64)uiValue;
		if ( OnTrigger(CTRIG_SkillChange, this, &args) == TRIGRET_RET_TRUE )
			return;

		const llong iN2Old = args.m_iN2;
		if (args.m_iN2 > UINT16_MAX)
		{
			args.m_iN2 = UINT16_MAX;
		}
		else if (args.m_iN2 < 0)
		{
			args.m_iN2 = 0;
		}
		if (iN2Old != args.m_iN2)
		{
			g_Log.EventWarn("Trying to set skill '%s' to invalid value=%lld. Defaulting it to %" PRId64 ".\n", Skill_GetName(skill), iN2Old, args.m_iN2);
		}

		uiValue = (ushort)(args.m_iN2);
	}
	m_Skill[skill] = uiValue;

	if ( IsClientActive())
		m_pClient->addSkillWindow(skill);	// update the skills list

	if ( g_Cfg.m_iCombatDamageEra )
	{
		if ( skill == SKILL_ANATOMY || skill == SKILL_TACTICS || skill == SKILL_LUMBERJACKING )
			fUpdateStats = true;		// those skills are used to calculate the char damage bonus, so we must update the client status gump
	}

	// We need to update the AC given by the Shield when parrying increase.
	if (skill == SKILL_PARRYING && g_Cfg.m_iCombatParryingEra & PARRYERA_ARSCALING)
	{
		
		m_defense = (word)CalcArmorDefense();
		fUpdateStats = true;
	}
	
	if (fUpdateStats)
		UpdateStatsFlag();
}

ushort CChar::Skill_GetMax( SKILL_TYPE skill, bool ignoreLock ) const
{
	ADDTOCALLSTACK("CChar::Skill_GetMax");
	const CVarDefCont * pTagStorage = nullptr;
	TemporaryString tsSkillName;

	// What is my max potential in this skill ?
	if ( IsPlayer() )
	{
		const CSkillClassDef * pSkillClass = m_pPlayer->GetSkillClass();
		ASSERT(pSkillClass);
		ASSERT( IsSkillBase(skill) );

		snprintf(tsSkillName.buffer(), tsSkillName.capacity(), "OVERRIDE.SKILLCAP_%d", (int)skill);
		ushort uiSkillMax;
		if ( (pTagStorage = GetKey(tsSkillName, true)) != nullptr )
			uiSkillMax = (ushort)(pTagStorage->GetValNum());
		else
			uiSkillMax = pSkillClass->m_SkillLevelMax[skill];

		if ( !ignoreLock )
		{
			const ushort uiSkillLevel = Skill_GetBase(skill);
			const SKILLLOCK_TYPE sklock = m_pPlayer->Skill_GetLock(skill);
			if (sklock == SKILLLOCK_DOWN)
			{
				uiSkillMax = uiSkillLevel;
			}
			else if ((sklock == SKILLLOCK_LOCK) && (uiSkillLevel < uiSkillMax))
			{
				uiSkillMax = uiSkillLevel;
			}
		}

		return uiSkillMax;
	}
	else
	{
		if ( skill == (SKILL_TYPE)(g_Cfg.m_iMaxSkill) )
		{
			pTagStorage = GetKey("OVERRIDE.SKILLSUM", true);
			return pTagStorage ? (ushort)(pTagStorage->GetValNum()) : (ushort)(500 * g_Cfg.m_iMaxSkill);
		}

		ushort uiSkillMax = 1000;
		snprintf(tsSkillName.buffer(), tsSkillName.capacity(), "OVERRIDE.SKILLCAP_%d", (int)skill);
		if ( (pTagStorage = GetKey(tsSkillName, true)) != nullptr )
			uiSkillMax = (ushort)(pTagStorage->GetValNum());

		return uiSkillMax;
	}
}

uint CChar::Skill_GetSum() const
{
	ADDTOCALLSTACK("CChar::Skill_GetSum");
	uint iSkillSum = 0;
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
		iSkillSum += Skill_GetBase((SKILL_TYPE)i);

	return iSkillSum;
}

uint CChar::Skill_GetSumMax() const
{
    ADDTOCALLSTACK("CChar::Skill_GetSumMax");
	const CVarDefCont *pTagStorage = GetKey("OVERRIDE.SKILLSUM", true);
	if (pTagStorage)
		return (uint)pTagStorage->GetValNum();

	const CSkillClassDef* pSkillClass = m_pPlayer->GetSkillClass();
	ASSERT(pSkillClass);
    return pSkillClass->m_SkillSumMax;
}

void CChar::Skill_Decay()
{
	ADDTOCALLSTACK("CChar::Skill_Decay");
	// Decay the character's skill levels.

	SKILL_TYPE skillDeduct = SKILL_NONE;
	ushort uiSkillLevel = 0;

	// Look for a skill to deduct from
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		if ( g_Cfg.m_SkillIndexDefs.IsValidIndex(i) == false )
			continue;

		// Check that the skill is set to decrease and that it is not already at 0
		if ( (Skill_GetLock((SKILL_TYPE)i) != SKILLLOCK_DOWN) || (Skill_GetBase((SKILL_TYPE)i) <= 0) )
			continue;

		// Prefer to deduct from lesser skills
		if ( (skillDeduct != SKILL_NONE) && (uiSkillLevel > Skill_GetBase((SKILL_TYPE)i)) )
			continue;

		skillDeduct = (SKILL_TYPE)i;
		uiSkillLevel = Skill_GetBase(skillDeduct);
	}

	// debug message
#ifdef _DEBUG
	if ( ( g_Cfg.m_iDebugFlags & DEBUGF_ADVANCE_STATS ) && IsPriv( PRIV_DETAIL ) && GetPrivLevel() >= PLEVEL_GM )
	{
		if ( skillDeduct == SKILL_NONE )
			SysMessage("No suitable skill to reduce.\n");
		else
			SysMessagef("Reducing %s=%hu.%hu\n", g_Cfg.GetSkillKey(skillDeduct), uiSkillLevel/10, uiSkillLevel%10);
	}
#endif

	// deduct a point from the chosen skill
	if ( skillDeduct != SKILL_NONE )
	{
		--uiSkillLevel;
		Skill_SetBase(skillDeduct, uiSkillLevel);
	}
}

void CChar::Skill_Experience( SKILL_TYPE skill, int iDifficulty )
{
	ADDTOCALLSTACK("CChar::Skill_Experience");
	// Give the char credit for using the skill.
	// More credit for the more difficult. or none if too easy
	//
	// ARGS:
	//  difficulty = skill target from 0-100

	if ( !IsSkillBase(skill) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) )
		return;
	if ( m_pArea && m_pArea->IsFlag( REGION_FLAG_SAFE ))	// skills don't advance in safe areas.
		return;

	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(skill);
	if (pSkillDef == nullptr)
		return;

	iDifficulty *= 10;
	if ( iDifficulty < 1 )
		iDifficulty = 1;
	else if ( iDifficulty > 1000 )
		iDifficulty = 1000;

	if ( IsPlayer() )
	{
        if (Skill_GetSum() >= Skill_GetSumMax())
        {
			iDifficulty = 0;
        }
	}

	// ex. ADV_RATE=2000,500,25 -> easy
	// ex. ADV_RATE=8000,2000,100 -> hard
	// Assume 100 = a 1 for 1 gain
	// ex: 8000 = we must use it 80 times to gain .1
	// Higher the number = the less probable to advance.
	// Extrapolate a place in the range.

	// give a bonus or a penalty if the task was too hard or too easy.
	// no gain at all if it was WAY TOO easy
	ushort uiSkillLevel = Skill_GetBase( skill );
	const ushort uiSkillLevelFixed = (uiSkillLevel < 50) ? 50 : uiSkillLevel;
	const int iGainRadius = pSkillDef->m_GainRadius;
	if ((iGainRadius > 0) && ((iDifficulty + iGainRadius) < uiSkillLevelFixed))
	{
		if ( GetKeyNum("NOSKILLMSG") )
			SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_GAINRADIUS_NOT_MET) );
		return;
	}

	int64 iChance = pSkillDef->m_AdvRate.GetChancePercent(uiSkillLevel);
	int64 iSkillMax = Skill_GetMax(skill);	// max advance for this skill.

	CScriptTriggerArgs pArgs(0, iChance, iSkillMax);
	if ( IsTrigUsed(TRIGGER_SKILLGAIN) )
	{
		if ( Skill_OnCharTrigger( skill, CTRIG_SkillGain, &pArgs ) == TRIGRET_RET_TRUE )
			return;
	}
	if ( IsTrigUsed(TRIGGER_GAIN) )
	{
		if ( Skill_OnTrigger( skill, SKTRIG_GAIN, &pArgs ) == TRIGRET_RET_TRUE )
			return;
	}
	pArgs.GetArgNs( 0, &iChance, &iSkillMax );

	if ( iChance <= 0 )
		return;

	const int iRoll = Calc_GetRandVal(1000);
	if ( uiSkillLevelFixed < (ushort)iSkillMax )	// are we in position to gain skill ?
	{
		// slightly more chance of decay than gain
		if ( (iRoll * 3) <= int(iChance * 4) )
			Skill_Decay();

		if (iDifficulty > 0 )
		{
			if ( IsPriv(PRIV_DETAIL) && (GetPrivLevel() >= PLEVEL_GM) && (g_Cfg.m_iDebugFlags & DEBUGF_ADVANCE_STATS) )
			{
				SysMessagef( "%s=%hu.%hu Difficult=%d Gain Chance=%" PRId64 ".%" PRId64 "%% Roll=%d%%",
					pSkillDef->GetKey(),
					uiSkillLevel/10, (uiSkillLevel)%10,
					iDifficulty/10, iChance/10, iChance%10, iRoll/10 );
			}

			if ( iRoll <= iChance )
			{
				++uiSkillLevel;
				Skill_SetBase( skill, uiSkillLevel );
			}
		}
	}

	////////////////////////////////////////////////////////
	// Dish out any stat gains - even for failures.

	const uint uiStatSumMax = Stat_GetSumLimit();
	uint uiStatSum = Stat_GetSum();

	// Stat effects are unrelated to advance in skill !
	for ( int i = STAT_STR; i < STAT_BASE_QTY; ++i )
	{
		// Can't gain STR or DEX if morphed.
		if ( IsStatFlag( STATF_POLYMORPH ) && i != STAT_INT )
			continue;

		if ( Stat_GetLock((STAT_TYPE)i) != SKILLLOCK_UP)
			continue;

		const ushort uiStatVal = Stat_GetBase((STAT_TYPE)i);
		if ( uiStatVal <= 0 )	// some odd condition
			continue;
		
		/*Before there was uiStatSum >= uiStatSumMax:
		That condition prevented the decrease of stats when the player's Stats were equal to the StatCap */
		if (uiStatSum > uiStatSumMax)	// stat cap already reached
			break;

		const ushort uiStatMax = Stat_GetLimit((STAT_TYPE)i);
		if ( uiStatVal >= uiStatMax )
			continue;	// nothing grows past this. (even for NPC's)

		// You will tend toward these stat vals if you use this skill a lot
		const byte bStatTarg = pSkillDef->m_Stat[i];
		if ( uiStatVal >= bStatTarg )
			continue;		// you've got higher stats than this skill is good for

		// Adjust the chance by the percent of this that the skill uses
		iDifficulty = IMulDiv( uiStatVal, 1000, bStatTarg );
		iChance = g_Cfg.m_StatAdv[i].GetChancePercent(iDifficulty);
		if ( pSkillDef->m_StatPercent )
			iChance = ( iChance * pSkillDef->m_StatBonus[i] * pSkillDef->m_StatPercent ) / 10000;

		if ( iChance == 0 )
			continue;

		const bool fDecrease = Stat_Decrease((STAT_TYPE)i, skill);
		if (fDecrease)
			uiStatSum = Stat_GetSum();

		if ( (iChance > Calc_GetRandVal(1000)) && fDecrease)
		{
			Stat_SetBase((STAT_TYPE)i, (uiStatVal + 1));
			break;
		}
	}
}

bool CChar::Skill_CheckSuccess( SKILL_TYPE skill, int difficulty, bool bUseBellCurve ) const
{
	ADDTOCALLSTACK("CChar::Skill_CheckSuccess");
	// PURPOSE:
	//  Check a skill for success or fail.
	//  DO NOT give experience here.
	// ARGS:
	//  difficulty		= 0-100 = The point at which the equiv skill level has a 50% chance of success.
	//	bUseBellCurve	= check skill success chance using bell curve or a simple percent check?
	// RETURN:
	//	true = success in skill.

	if ( IsPriv(PRIV_GM) && skill != SKILL_PARRYING )	// GM's can't always succeed Parrying or they won't receive any damage on combat even without STATF_Invul set
		return true;

	if ( !IsSkillBase(skill) || (difficulty < 0) )	// auto failure.
		return false;

	difficulty *= 10;

#define SKILL_VARIANCE 100		// Difficulty modifier for determining success. 10.0 %
	int iSuccessChance = difficulty;
	if ( bUseBellCurve )
		iSuccessChance = Calc_GetSCurve( Skill_GetAdjusted(skill) - difficulty, SKILL_VARIANCE );

	return ( iSuccessChance >= Calc_GetRandVal(1000) );
}

bool CChar::Skill_UseQuick( SKILL_TYPE skill, int64 difficulty, bool bAllowGain, bool bUseBellCurve, bool bForceCheck )
{
	ADDTOCALLSTACK("CChar::Skill_UseQuick");
	// ARGS:
	//	skill			= skill to use
	//  difficulty		= 0-100
	//	bAllowGain		= can gain skill from this?
	//	bUseBellCurve	= check skill success chance using bell curve or a simple percent check?
	// Use a skill instantly. No wait at all.
	// No interference with other skills.

	if (g_Cfg.IsSkillFlag(skill, SKF_SCRIPTED) && !bForceCheck)
		return false;

	int64 result = Skill_CheckSuccess( skill, (int)difficulty, bUseBellCurve );
	CScriptTriggerArgs pArgs( 0 , difficulty, result);
	TRIGRET_TYPE ret = TRIGRET_RET_DEFAULT;

	if ( IsTrigUsed(TRIGGER_SKILLUSEQUICK) )
	{
		ret = Skill_OnCharTrigger( skill, CTRIG_SkillUseQuick, &pArgs );
		pArgs.GetArgNs( 0, &difficulty, &result);

		if ( ret == TRIGRET_RET_TRUE )
			return true;
		else if ( ret == TRIGRET_RET_FALSE )
			return false;
	}
	if ( IsTrigUsed(TRIGGER_USEQUICK) )
	{
		ret = Skill_OnTrigger( skill, SKTRIG_USEQUICK, &pArgs );
		pArgs.GetArgNs( 0, &difficulty, &result );

		if ( ret == TRIGRET_RET_TRUE )
			return true;
		else if ( ret == TRIGRET_RET_FALSE )
			return false;
	}

	if ( result )	// success
	{
		if ( bAllowGain )
			Skill_Experience( skill, (int)(difficulty) );
		return true;
	}
	else			// fail
	{
		if ( bAllowGain )
			Skill_Experience( skill, (int)(-difficulty) );
		return false;
	}
}

void CChar::Skill_Cleanup()
{
	ADDTOCALLSTACK("CChar::Skill_Cleanup");
	// We are starting the skill or ended dealing with it (started / succeeded / failed / aborted)
	m_Act_Difficulty = 0;
	m_Act_SkillCurrent = SKILL_NONE;
	_SetTimeoutD( m_pPlayer ? -1 : 1 );	// we should get a brain tick next time
}

lpctstr CChar::Skill_GetName( bool fUse ) const
{
	ADDTOCALLSTACK("CChar::Skill_GetName");
	// Name the current skill we are doing.

	SKILL_TYPE skill = Skill_GetActive();
	if ( skill <= SKILL_NONE )
		return( "Idling" );

	if ( IsSkillBase(skill))
	{
		if ( !fUse )
			return g_Cfg.GetSkillKey(skill);

		tchar * pszText = Str_GetTemp();
		snprintf( pszText, STR_TEMPLENGTH, "%s %s", g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_USING), g_Cfg.GetSkillKey(skill));
		return( pszText );
	}

	switch ( skill )
	{
		case NPCACT_FOLLOW_TARG:	return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_FOLLOWING) );
		case NPCACT_STAY:			return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_STAYING) );
		case NPCACT_GOTO:			return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_GOINGTO) );
		case NPCACT_WANDER:			return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_WANDERING) );
		case NPCACT_FLEE:			return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_FLEEING) );
		case NPCACT_TALK:			return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_TALKING) );
		case NPCACT_TALK_FOLLOW:	return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_TALKFOLLOW) );
		case NPCACT_GUARD_TARG:		return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_GUARDING) );
		case NPCACT_GO_HOME:		return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_GOINGHOME) );
		case NPCACT_BREATH:			return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_BREATHING) );
		case NPCACT_THROWING:		return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_THROWING) );
		case NPCACT_LOOKING:		return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_LOOKING) );
		case NPCACT_TRAINING:		return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_TRAINING) );
		case NPCACT_NAPPING:		return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_NAPPING) );
		case NPCACT_FOOD:			return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_SEARCHINGFOOD) );
		case NPCACT_RUNTO:			return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_RUNNINGTO) );
		default:					return( g_Cfg.GetDefaultMsg(DEFMSG_SKILLACT_THINKING) );
	}
}

ushort CChar::Skill_GetBase( SKILL_TYPE skill ) const
{
	ASSERT( IsSkillBase(skill));
	return( m_Skill[skill] );
}

void CChar::Skill_SetTimeout()
{
	ADDTOCALLSTACK("CChar::Skill_SetTimeout");
	_SetTimeout(Skill_GetTimeout());
}

int64 CChar::Skill_GetTimeout()
{
	ADDTOCALLSTACK("CChar::Skill_GetTimeout");
	SKILL_TYPE skill = Skill_GetActive();
	ASSERT( IsSkillBase(skill));

	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(skill);
	if (!pSkillDef)
		return 0;

	int iSkillLevel = Skill_GetBase(skill);
    int64 iTimeoutInTenths = pSkillDef->m_vcDelay.GetLinear(iSkillLevel);
	return (maximum(1, iTimeoutInTenths) * MSECS_PER_TENTH);
}

bool CChar::Skill_MakeItem_Success()
{
	ADDTOCALLSTACK("CChar::Skill_MakeItem_Success");
	// Deliver the goods

	CItem *pItem = CItem::CreateTemplate(m_atCreate.m_iItemID, nullptr, this);
	if ( !pItem )
		return false;

	int quality = 0;
	tchar *pszMsg = Str_GetTemp();
	word iSkillLevel = Skill_GetBase(Skill_GetActive());				// primary skill value.
	CItemVendable *pItemVend = dynamic_cast<CItemVendable *>(pItem);		// cast CItemVendable for setting quality and exp later

	if ( m_atCreate.m_dwAmount != 1 )
	{
		if ( pItem->IsType(IT_SCROLL) )
			pItem->m_itSpell.m_spelllevel = iSkillLevel;

		const CItemBase *ptItemDef = CItemBase::FindItemBase(m_atCreate.m_iItemID);
		ASSERT(ptItemDef);
		if (ptItemDef->IsStackableType())
		{
			pItem->SetAmount((word)m_atCreate.m_dwAmount);
		}
		else
		{
			for ( uint n = 1; n < m_atCreate.m_dwAmount; ++n )
			{
				CItem *ptItem = CItem::CreateTemplate(m_atCreate.m_iItemID, nullptr, this);
				ItemBounce(ptItem);
			}
		}
	}
	else if ( pItem->IsType(IT_SCROLL) )
	{
		// scrolls have the skill level of the inscriber ?
		pItem->m_itSpell.m_spelllevel = iSkillLevel;
	}
	else if ( pItem->IsType(IT_POTION) )
	{
		// Create the potion, set various properties,
	}
	else
	{
		// Only set the quality on single items.
		// Quality depends on the skill of the craftsman, and a random chance.
		// minimum quality is 1, maximum quality is 200.  100 is average.

		// How much variance? This is the difference in quality levels from what I can normally make.
		int variance = 2 - (int)(log10( 1.0 + double(Calc_GetRandVal(250)) ));	// this should result in a value between 0 and 2

		// Determine if lower or higher quality
		if ( !Calc_GetRandVal(2) )
			variance = -variance;		// worse than I can normally make

		// Determine which range I'm in
		quality = IMulDiv(iSkillLevel, 2, 10);	// default value for quality
		int qualityBase;
		if ( quality < 25 )			// 1 - 25		Shoddy
			qualityBase = 0;
		else if ( quality < 50 )	// 26 - 50		Poor
			qualityBase = 1;
		else if ( quality < 75 )	// 51 - 75		Below Average
			qualityBase = 2;
		else if ( quality < 125 )	// 76 - 125		Average
			qualityBase = 3;
		else if ( quality < 150 )	// 125 - 150	Above Average
			qualityBase = 4;
		else if ( quality < 175 )	// 151 - 175	Excellent
			qualityBase = 5;
		else						// 175 - 200	Superior
			qualityBase = 6;

		qualityBase += variance;
		if ( qualityBase < 0 )
			qualityBase = 0;
		if ( qualityBase > 6 )
			qualityBase = 6;

		switch ( qualityBase )
		{
			case 0:
				// Shoddy quality
				Str_CopyLimitNull(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_1), STR_TEMPLENGTH);
				quality = Calc_GetRandVal(25) + 1;
				break;
			case 1:
				// Poor quality
				Str_CopyLimitNull(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_2), STR_TEMPLENGTH);
				quality = Calc_GetRandVal(25) + 26;
				break;
			case 2:
				// Below average quality
				Str_CopyLimitNull(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_3), STR_TEMPLENGTH);
				quality = Calc_GetRandVal(25) + 51;
				break;
			case 3:
				// Average quality
				quality = Calc_GetRandVal(50) + 76;
				break;
			case 4:
				// Above average quality
				Str_CopyLimitNull(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_4), STR_TEMPLENGTH);
				quality = Calc_GetRandVal(25) + 126;
				break;
			case 5:
				// Excellent quality
				Str_CopyLimitNull(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_5), STR_TEMPLENGTH);
				quality = Calc_GetRandVal(25) + 151;
				break;
			case 6:
				// Superior quality
				Str_CopyLimitNull(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MAKESUCCESS_6), STR_TEMPLENGTH);
				quality = Calc_GetRandVal(25) + 176;
				break;
			default:
				// How'd we get here?
				quality = 1000;
				break;
		}

		if ( pItemVend )	// check that the item is vendable before setting quality
			pItemVend->SetQuality((word)quality);

		if ( (iSkillLevel > 999) && (quality > 175) && !IsSetOF(OF_NoItemNaming) )
		{
			// A GM made this, and it is of high quality
			tchar *szNewName = Str_GetTemp();
			snprintf(szNewName, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_GRANDMASTER_MARK), pItem->GetName(), GetName());
			pItem->SetName(szNewName);

			// TO-DO: before enable CRAFTEDBY we must find away to properly clear CRAFTEDBY value on all items when chars got deleted
			//pItem->SetDefNum("CRAFTEDBY", GetUID());
		}
	}

	// Item goes into ACT of player
	CUID uidOldAct = m_Act_UID;
	m_Act_UID = pItem->GetUID();
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
	if ( IsTrigUsed(TRIGGER_SKILLMAKEITEM) )
	{
		CScriptTriggerArgs Args(iSkillLevel, quality, uidOldAct.ObjFind());
		iRet = OnTrigger(CTRIG_SkillMakeItem, this, &Args);
	}
	m_Act_UID = uidOldAct;		// restore

	CObjBase *pItemCont = pItem->GetContainer();
	if ( iRet == TRIGRET_RET_TRUE )
	{
		if ( pItem->GetContainer() == pItemCont )
			pItem->Delete();
		return false;
	}
	else if ( iRet == TRIGRET_RET_DEFAULT )
	{
		if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOSFX) )
		{
			if ( pItem->IsType(IT_POTION) )
				Sound(0x240);
			else if ( pItem->IsType(IT_MAP) )
				Sound(0x255);
		}
		if ( *pszMsg )
			SysMessage(pszMsg);
	}

	// Experience gain on craftings
	if ( g_Cfg.m_bExperienceSystem && (g_Cfg.m_iExperienceMode & EXP_MODE_RAISE_CRAFT) )
	{
		int exp = 0;
		if ( pItemVend )
			exp = pItemVend->GetVendorPrice(0,0) / 100;	// calculate cost for buying this item if it is vendable (gain = +1 exp each 100gp)
		if ( exp )
			ChangeExperience(exp);
	}

	ItemBounce(pItem);
	return true;
}


int CChar::SkillResourceTest( const CResourceQtyArray * pResources )
{
	ADDTOCALLSTACK("CChar::SkillResourceTest");
	return pResources->IsResourceMatchAll(this);
}


bool CChar::Skill_MakeItem( ITEMID_TYPE id, CUID uidTarg, SKTRIG_TYPE stage, bool fSkillOnly, int iReplicationQty )
{
	ADDTOCALLSTACK("CChar::Skill_MakeItem");
	// "MAKEITEM"
	//
	// SKILL_ALCHEMY
	// SKILL_BLACKSMITHING
	// SKILL_BOWCRAFT
	// SKILL_CARPENTRY
	// SKILL_COOKING
	// SKILL_INSCRIPTION
	// SKILL_TAILORING:
	// SKILL_TINKERING,
	//
	// Confer the new item.
	// Test for consumable items.
	// Fail = do a partial consume of the resources.
	//
	// ARGS:
	//  uidTarg = item targetted to try to make this . (this item should be used to make somehow)
	// Skill_GetActive()
	//
	// RETURN:
	//   true = success.

	CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( !pItemDef )
		return false;

	CItem *pItemTarg = uidTarg.ItemFind();
	if ( pItemTarg && (stage == SKTRIG_SELECT) )
	{
        if (pItemDef->m_SkillMake.ContainsResourceType(RES_TYPEDEF) || pItemDef->m_BaseResources.ContainsResourceType(RES_TYPEDEF)
            || pItemDef->m_SkillMake.ContainsResourceType(RES_ITEMDEF))
        {
            // Is an item of a specific TYPE needed to craft this item?
            if ( !pItemDef->m_SkillMake.ContainsResourceMatch(pItemTarg) && !pItemDef->m_BaseResources.ContainsResourceMatch(pItemTarg) )
                return false;
        }
	}

	if ( !SkillResourceTest(&(pItemDef->m_SkillMake)) )
		return false;
	if ( fSkillOnly )
		return true;

	CItem *pItemDragging = LayerFind(LAYER_DRAGGING);
	if ( pItemDragging )
		ItemBounce(pItemDragging);

	iReplicationQty = ResourceConsume(&(pItemDef->m_BaseResources), iReplicationQty, (stage != SKTRIG_SUCCESS));
	if ( !iReplicationQty )
		return false;

	// Test or consume the needed resources.
	if ( stage == SKTRIG_FAIL )
	{
		// If fail only consume part of them.
		int iConsumePercent = -1;
		size_t i = pItemDef->m_SkillMake.FindResourceType(RES_SKILL);
		if ( i != SCONT_BADINDEX )
		{
			CSkillDef *pSkillDef = g_Cfg.GetSkillDef((SKILL_TYPE)(pItemDef->m_SkillMake[i].GetResIndex()));
			if ( pSkillDef && !pSkillDef->m_vcEffect.m_aiValues.empty() )
				iConsumePercent = pSkillDef->m_vcEffect.GetRandom();
		}

		if ( iConsumePercent < 0 )
			iConsumePercent = Calc_GetRandVal(50);

		ResourceConsumePart(&(pItemDef->m_BaseResources), iReplicationQty, iConsumePercent, false, pItemDef->GetResourceID().GetResIndex());
		return false;
	}

	if ( stage == SKTRIG_START )
	{
		// Start the skill.
		// Find the primary skill required.
		size_t i = pItemDef->m_SkillMake.FindResourceType(RES_SKILL);
		if ( i == SCONT_BADINDEX )
        {
            g_Log.EventError("Trying to make item=%s with invalid SKILLMAKE.\n", pItemDef->GetResourceName());
			return false;
        }

		m_Act_UID = uidTarg;	// targetted item to start the make process
		m_atCreate.m_iItemID = id;
		m_atCreate.m_dwAmount = (word)(iReplicationQty);

		CResourceQty RetMainSkill = pItemDef->m_SkillMake[i];
		return Skill_Start((SKILL_TYPE)(RetMainSkill.GetResIndex()), (int)(RetMainSkill.GetResQty() / 10));
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		m_atCreate.m_dwAmount = (word)(iReplicationQty); // how much resources we really consumed
		return Skill_MakeItem_Success();
	}

	return true;
}

int CChar::Skill_NaturalResource_Setup( CItem * pResBit )
{
	ADDTOCALLSTACK("CChar::Skill_NaturalResource_Setup");
	// RETURN: skill difficulty
	//  0-100
	ASSERT(pResBit);

	// Find the ore type located here based on color.
	const CRegionResourceDef * pOreDef = dynamic_cast<const CRegionResourceDef *>(g_Cfg.ResourceGetDef(pResBit->m_itResource.m_ridRes));
	if ( pOreDef == nullptr )
		return -1;

	return (pOreDef->m_vcSkill.GetRandom() / 10);
}

CItem * CChar::Skill_NaturalResource_Create( CItem * pResBit, SKILL_TYPE skill )
{
	ADDTOCALLSTACK("CChar::Skill_NaturalResource_Create");
	// Create some natural resource item.
	// skill = Effects qty of items returned.
	// SKILL_MINING
	// SKILL_FISHING
	// SKILL_LUMBERJACKING

	ASSERT(pResBit);

	// Find the ore type located here based on color.
	CRegionResourceDef * pOreDef = dynamic_cast<CRegionResourceDef *>(g_Cfg.ResourceGetDef(pResBit->m_itResource.m_ridRes));
	if ( !pOreDef )
		return nullptr;

	// Skill effects how much of the ore i can get all at once.
	if ( pOreDef->m_ReapItem == ITEMID_NOTHING )
		return nullptr;		// I intended for there to be nothing here

	// Reap amount is semi-random
	word wAmount = (word)pOreDef->m_vcReapAmount.GetRandomLinear( Skill_GetBase(skill) );
	if ( !wAmount )		// if REAPAMOUNT wasn't defined
	{
		wAmount = (word)(pOreDef->m_vcAmount.GetRandomLinear( Skill_GetBase(skill) ) / 2);
		word wMaxAmount = pResBit->GetAmount();
		if ( wAmount < 1 )
			wAmount = 1;
		if ( wAmount > wMaxAmount )
			wAmount = wMaxAmount;
	}

	//(Region)ResourceGather behaviour
	CScriptTriggerArgs	Args(0, 0, pResBit);
	Args.m_VarsLocal.SetNum("ResourceID",pOreDef->m_ReapItem);
	Args.m_iN1 = wAmount;
	TRIGRET_TYPE tRet = TRIGRET_RET_DEFAULT;
	if ( IsTrigUsed(TRIGGER_REGIONRESOURCEGATHER) )
		tRet = this->OnTrigger(CTRIG_RegionResourceGather, this, &Args);
	if ( IsTrigUsed(TRIGGER_RESOURCEGATHER) )
		tRet = pOreDef->OnTrigger("@ResourceGather", this, &Args);

	if ( tRet == TRIGRET_RET_TRUE )
		return nullptr;

	//Creating the 'id' variable with the local given through->by the trigger(s) instead on top of method
	ITEMID_TYPE id = (ITEMID_TYPE)(RES_GET_INDEX( Args.m_VarsLocal.GetKeyNum("ResourceID")));

	wAmount = pResBit->ConsumeAmount( (word)(Args.m_iN1) );	// amount i used up.
	if ( wAmount <= 0 )
		return nullptr;

	CItem * pItem = CItem::CreateScript( id, this );
	ASSERT(pItem);

	pItem->SetAmount( wAmount );
	return pItem;
}

bool CChar::Skill_Mining_Smelt( CItem * pItemOre, CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Skill_Mining_Smelt");
	// SKILL_MINING
	// pItemTarg = forge or another pile of ore.
	// RETURN: true = success.
	if ( pItemOre == nullptr || pItemOre == pItemTarg )
	{
		SysMessageDefault( DEFMSG_MINING_NOT_ORE );
		return false;
	}

	// The ore is on the ground
	if ( ! CanUse( pItemOre, true ))
	{
		SysMessagef(g_Cfg.GetDefaultMsg( DEFMSG_MINING_REACH ), pItemOre->GetName());
		return false;
	}

	if ( pItemOre->IsType( IT_ORE ) && pItemTarg != nullptr && pItemTarg->IsType( IT_ORE ))
	{
		// combine piles.
		if ( pItemTarg == pItemOre )
			return false;
		if ( pItemTarg->GetID() != pItemOre->GetID())
			return false;
		pItemTarg->SetAmountUpdate( pItemOre->GetAmount() + pItemTarg->GetAmount());
		pItemOre->Delete();
		return true;
	}

	if ( pItemTarg != nullptr && pItemTarg->IsTopLevel() && pItemTarg->IsType( IT_FORGE ))
		m_Act_p = pItemTarg->GetTopPoint();
	else
		m_Act_p = CWorldMap::FindItemTypeNearby( GetTopPoint(), IT_FORGE, 3, false );

	if ( !m_Act_p.IsValidPoint() || !CanTouch(m_Act_p))
	{
		SysMessageDefault( DEFMSG_MINING_FORGE );
		return false;
	}

	const CItemBase * pOreDef = pItemOre->Item_GetDef();
	if ( pOreDef->IsType( IT_INGOT ))
	{
		SysMessageDefault( DEFMSG_MINING_INGOTS );
		return false;
	}

	// Fire effect ?
	CItem * pItemEffect = CItem::CreateBase(ITEMID_FIRE);
	ASSERT(pItemEffect);
	CPointMap pt = m_Act_p;
	pt.m_z += 8;	// on top of the forge.
	pItemEffect->SetAttr( ATTR_MOVE_NEVER );
	pItemEffect->MoveToDecay( pt, MSECS_PER_SEC);
	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
		Sound( 0x2b );

	UpdateDir( m_Act_p );
	if ( pItemOre->IsAttr(ATTR_MAGIC|ATTR_BLESSED|ATTR_BLESSED2))	// not magic items
	{
		SysMessageDefault( DEFMSG_MINING_FIRE );
		return false;
	}

	tchar * pszMsg = Str_GetTemp();
	snprintf(pszMsg, STR_TEMPLENGTH, "%s %s", g_Cfg.GetDefaultMsg( DEFMSG_MINING_SMELT ), pItemOre->GetName());
	Emote(pszMsg);

	ushort iMiningSkill = Skill_GetAdjusted(SKILL_MINING);
	word iOreQty = pItemOre->GetAmount();
	word iResourceQty = 0;
	size_t iResourceTotalQty = pOreDef->m_BaseResources.size(); //This is the total amount of different resources obtained from smelting.		

	CScriptTriggerArgs Args(iMiningSkill, iResourceTotalQty);
	
	if ( pOreDef->IsType( IT_ORE ))
	{
		ITEMID_TYPE idIngot = (ITEMID_TYPE)(RES_GET_INDEX( pOreDef->m_ttOre.m_idIngot));
		const CItemBase* pBaseDef = CItemBase::FindItemBase(idIngot); //Usually a lingot, but could be a a gem also.
		if (!pBaseDef)
		{
			SysMessageDefault(DEFMSG_MINING_NOTHING);
			return false;
		}
		iResourceQty = 1;	// ingots per ore.
		iResourceTotalQty = 1; //Ores only gives one type of resouce.
		Args.m_iN2 = iResourceTotalQty;
		Args.m_VarsLocal.SetNum("resource.0.ID", pBaseDef->GetID());
		Args.m_VarsLocal.SetNum("resource.0.amount", iResourceQty);
	}
	else
	{
		// Smelting something like armor etc.
		// find the ingot or gem type resources.
		for ( size_t i = 0; i < iResourceTotalQty; ++i )
		{
			CResourceID rid = pOreDef->m_BaseResources[i].GetResourceID();
			if ( rid.GetResType() != RES_ITEMDEF )
				continue;

			ITEMID_TYPE id = (ITEMID_TYPE)(rid.GetResIndex());
			if (id == ITEMID_NOTHING)
				break;

			tchar* pszTmp = Str_GetTemp();
			snprintf(pszTmp, STR_TEMPLENGTH, "resource.%u.ID", (int)i);
			Args.m_VarsLocal.SetNum(pszTmp,(int64)id);

			iResourceQty = (word)(pOreDef->m_BaseResources[i].GetResQty());
			snprintf(pszTmp, STR_TEMPLENGTH, "resource.%u.amount", (int)i);
			Args.m_VarsLocal.SetNum(pszTmp, iResourceQty);
			
		}
	}

	if (IsTrigUsed(TRIGGER_SMELT) || IsTrigUsed(TRIGGER_ITEMSMELT))
	{
		switch (pItemOre->OnTrigger(ITRIG_Smelt, this, &Args))
		{
		case TRIGRET_RET_TRUE:	return false;
		default:				break;
		}
	}

	iMiningSkill = (ushort)Args.m_iN1;
	for (size_t i = 0; i < iResourceTotalQty; ++i)
	{
		tchar* pszTmp = Str_GetTemp();
		snprintf(pszTmp, STR_TEMPLENGTH, "resource.%u.ID", (int)i);
		const CItemBase* pBaseDef = CItemBase::FindItemBase((ITEMID_TYPE)(RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum(pszTmp))));
		
		//We have finished the ore or the item being smelted.
		if (iOreQty <= 0)
		{
			SysMessageDefault(DEFMSG_MINING_CONSUMED);
			return false;
		}

		if (pBaseDef == nullptr  || (!pBaseDef->IsType(IT_INGOT) && !pBaseDef->IsType(IT_GEM)))
		{
			SysMessageDefault( DEFMSG_MINING_CONSUMED );
			continue;
		}

		snprintf(pszTmp, STR_TEMPLENGTH, "resource.%u.amount", (int)i);
		iResourceQty =(word)Args.m_VarsLocal.GetKeyNum(pszTmp);
		iResourceQty *= iOreQty;	// max amount

		if (pBaseDef->IsType(IT_GEM))
		{
			// bounce the gems out of this.
			CItem* pGem = CItem::CreateScript(pBaseDef->GetID(), this);
			if (pGem)
			{
				pGem->SetAmount((word)(iResourceQty));
				ItemBounce(pGem);
			}
			continue;
		}

		// Try to make ingots
		if (iMiningSkill < pBaseDef->m_ttIngot.m_iSkillMin)
		{
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MINING_SKILL), pBaseDef->GetName());
				continue;
		}
		
		const int iSkillRange = pBaseDef->m_ttIngot.m_iSkillMax - pBaseDef->m_ttIngot.m_iSkillMin;
		int iSmeltingDifficulty = Calc_GetRandVal(iSkillRange);

		iSmeltingDifficulty = (pBaseDef->m_ttIngot.m_iSkillMin + iSmeltingDifficulty) / 10;
		if ( !iResourceQty || !Skill_UseQuick( SKILL_MINING, iSmeltingDifficulty))
		{
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MINING_NOTHING ), pItemOre->GetName());
			word iAmountLost = (word)(Calc_GetRandVal(pItemOre->GetAmount() / 2) + 1);
			pItemOre->ConsumeAmount(iAmountLost);	// lose up to half the resources.
			iOreQty -= iAmountLost;
			continue;
		}
		// Payoff - Amount of ingots i get.
		CItem * pIngots = CItem::CreateScript(pBaseDef->GetID(), this );
		if ( pIngots == nullptr )
		{
			SysMessageDefault( DEFMSG_MINING_NOTHING );
			continue;
		}
		pIngots->SetAmount(iResourceQty);
		ItemBounce( pIngots );
	}
	pItemOre->ConsumeAmount(pItemOre->GetAmount());
	return true;
}

bool CChar::Skill_Tracking( CUID uidTarg, DIR_TYPE & dirPrv, int iDistMax )
{
	ADDTOCALLSTACK("CChar::Skill_Tracking");
	// SKILL_TRACKING
	UNREFERENCED_PARAMETER(dirPrv);

	if ( !IsClientActive() )		// abort action if the client get disconnected
		return false;

	const CObjBase * pObj = uidTarg.ObjFind();
	if ( pObj == nullptr )
		return false;

	const CObjBaseTemplate * pObjTop = pObj->GetTopLevelObj();
	if ( pObjTop == nullptr )
		return false;

	int dist = GetTopDist3D(pObjTop);	// disconnect = INT16_MAX
	if ( dist > iDistMax )
		return false;

	if ( pObjTop->IsChar() )
	{
		// Prevent tracking of hidden staff
		const CChar * pChar = dynamic_cast<const CChar *>(pObjTop);
		if ( pChar && pChar->IsStatFlag(STATF_INSUBSTANTIAL) && pChar->GetPrivLevel() > GetPrivLevel() )
			return false;
	}

	const DIR_TYPE dir = GetDir(pObjTop);
	ASSERT(dir >= 0 && (uint)(dir) < CountOf(CPointBase::sm_szDirs));

	// Select tracking message based on distance
	lpctstr pszDef;
	if ( dist <= 0 )
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_0);
	else if ( dist < 16 )
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_1);
	else if ( dist < 32 )
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_2);
	else if ( dist < 100 )
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_3);
	else
		pszDef = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_4);

	if ( pszDef[0] )
	{
		tchar *pszMsg = Str_GetTemp();
		snprintf(pszMsg, STR_TEMPLENGTH, 
			pszDef, pObj->GetName(), (pObjTop->IsDisconnected() ? g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_RESULT_DISC) : CPointBase::sm_szDirs[dir]) );
		ObjMessage(pszMsg, this);
	}

	return true;		// keep the skill active
}

//************************************
// Skill handlers.

int CChar::Skill_Tracking( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Tracking");
	// SKILL_TRACKING
	// m_Act_UID = what am i tracking ?
	// m_atTracking.m_iPrvDir = the previous dir it was in.
	// m_atTracking.m_dwDistMax = the maximum tracking distance.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_START )
		return 0;	// already checked difficulty earlier

	if ( stage == SKTRIG_FAIL )
	{
		// This skill didn't fail, it just ended/went out of range, etc...
		ObjMessage( g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_END ), this ); // say this instead of the failure message
		return -SKTRIG_ABORT;
	}

	if ( stage == SKTRIG_STROKE )
	{
		if ( Skill_Stroke(false) == -SKTRIG_ABORT )
			return -SKTRIG_ABORT;

		//Use the maximum distance stored in m_AtTracking.m_dwDistMax(ACTARG2), calculated in Cmd_Skill_Tracking, when continuing to track the creature.
		if ( !Skill_Tracking( m_Act_UID, m_atTracking.m_iPrvDir, m_atTracking.m_dwDistMax))
			return -SKTRIG_ABORT;
		Skill_SetTimeout();			// next update.
		return -SKTRIG_STROKE;	    // keep it active.
	}

	return -SKTRIG_ABORT;
}

int CChar::Skill_Mining( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Mining");
	// SKILL_MINING
	// m_Act_p = the point we want to mine at.
	// m_Act_Prv_UID = Pickaxe/Shovel
	//
	// Test the chance of precious ore.
	// resource check  to IT_ORE. How much can we get ?
	// RETURN:
	//  Difficulty 0-100

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_FAIL )
		return 0;

	CItem *pTool = m_Act_Prv_UID.ItemFind();
	if ( !pTool )
	{
		SysMessageDefault(DEFMSG_MINING_TOOL);
		return -SKTRIG_ABORT;
	}

	if ( m_Act_p.m_x == -1 )
	{
		SysMessageDefault(DEFMSG_MINING_4);
		return -SKTRIG_QTY;
	}

    const CRegion *pArea = GetRegion();
    if (pArea->IsFlag(REGION_FLAG_NOMINING))
    {
        SysMessageDefault(DEFMSG_MINING_2);
        return -SKTRIG_QTY;
    }

	// 3D distance check and LOS
	const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_MINING);
	const int iTargRange = GetTopPoint().GetDist(m_Act_p);
	int iMaxRange = pSkillDef->m_Range;
	if ( !iMaxRange )
	{
		g_Log.EventError("Mining skill doesn't have a value for RANGE, defaulting to 2\n");
		iMaxRange = 2;
	}
	if ( iTargRange < 1 && !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOMINDIST) )
	{
		SysMessageDefault(DEFMSG_MINING_CLOSE);
		return -SKTRIG_QTY;
	}
	if ( iTargRange > iMaxRange )
	{
		SysMessageDefault(DEFMSG_MINING_REACH);
		return -SKTRIG_QTY;
	}
	if ( !CanSeeLOS(m_Act_p, nullptr, iMaxRange) )
	{
		SysMessageDefault(DEFMSG_MINING_LOS);
		return -SKTRIG_QTY;
	}

	// Resource check
	CItem *pResBit = CWorldMap::CheckNaturalResource(m_Act_p, (IT_TYPE)(m_atResource.m_ridType.GetResIndex()), stage == SKTRIG_START, this);
	if ( !pResBit )
	{
		SysMessageDefault(DEFMSG_MINING_1);
		return -SKTRIG_QTY;
	}
	if ( pResBit->GetAmount() == 0 )
	{
		SysMessageDefault(DEFMSG_MINING_2);
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_START )
	{
		m_atResource.m_dwStrokeCount = (word)(Calc_GetRandVal(5) + 2);
		return Skill_NaturalResource_Setup(pResBit);	// How difficult? 1-1000
	}

	CItem *pItem = Skill_NaturalResource_Create(pResBit, SKILL_MINING);
	if ( !pItem )
	{
		SysMessageDefault(DEFMSG_MINING_3);
		return -SKTRIG_FAIL;
	}

	if ( m_atResource.m_dwBounceItem )
		ItemBounce(pItem);
	else
		pItem->MoveToCheck(GetTopPoint(), this);	// put at my feet.

	return 0;
}

int CChar::Skill_Fishing( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Fishing");
	// SKILL_FISHING
	// m_Act_p = where to fish.
	// NOTE: don't check LOS else you can't fish off boats.
	// Check that we dont stand too far away
	// Make sure we aren't in a house
	//
	// RETURN:
	//   difficulty = 0-100

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_FAIL )
		return 0;

	if ( m_Act_p.m_x == -1 )
	{
		SysMessageDefault(DEFMSG_FISHING_4);
		return -SKTRIG_QTY;
	}

	CRegion *pRegion = GetTopPoint().GetRegion(REGION_TYPE_MULTI);		// are we in a house ?
	if ( pRegion && !pRegion->IsFlag(REGION_FLAG_SHIP) )
	{
		SysMessageDefault(DEFMSG_FISHING_3);
		return -SKTRIG_QTY;
	}
	if ( m_Act_p.GetRegion(REGION_TYPE_MULTI) )		// do not allow fishing through ship floor
	{
		SysMessageDefault(DEFMSG_FISHING_4);
		return -SKTRIG_QTY;
	}

	// 3D distance check and LOS
	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_FISHING);
	int iTargRange = GetTopPoint().GetDist(m_Act_p);
	int iMaxRange = pSkillDef->m_Range;
	if ( !iMaxRange )
	{
		g_Log.EventError("Fishing skill doesn't have a value for RANGE, defaulting to 4\n");
		iMaxRange = 4;
	}
	if ( iTargRange < 1 && !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOMINDIST) ) // you cannot fish under your legs
	{
		SysMessageDefault(DEFMSG_FISHING_CLOSE);
		return -SKTRIG_QTY;
	}
	if ( iTargRange > iMaxRange )
	{
		SysMessageDefault(DEFMSG_FISHING_REACH);
		return -SKTRIG_QTY;
	}
	if ( (m_pPlayer && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER)) || (m_pNPC && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_NPC)) )
	{
		if ( !CanSeeLOS(m_Act_p, nullptr, iMaxRange, LOS_FISHING) )
		{
			SysMessageDefault(DEFMSG_FISHING_LOS);
			return -SKTRIG_QTY;
		}
	}

	// Resource check
	CItem *pResBit = CWorldMap::CheckNaturalResource(m_Act_p, (IT_TYPE)(m_atResource.m_ridType.GetResIndex()), stage == SKTRIG_START, this);
	if ( !pResBit )
	{
		SysMessageDefault(DEFMSG_FISHING_1);
		return -SKTRIG_QTY;
	}
	if ( pResBit->GetAmount() == 0 )
	{
		SysMessageDefault(DEFMSG_FISHING_2);
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_START )
	{
		m_atResource.m_dwStrokeCount = 1;
		m_Act_UID = pResBit->GetUID();
		return Skill_NaturalResource_Setup(pResBit);	// How difficult? 1-1000
	}

	CItem *pItem = Skill_NaturalResource_Create(pResBit, SKILL_FISHING);
	if ( !pItem )
	{
		SysMessageDefault(DEFMSG_FISHING_2);
		return -SKTRIG_FAIL;
	}

	SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_FISHING_SUCCESS), pItem->GetName());
	if ( m_atResource.m_dwBounceItem )
		ItemBounce(pItem, false);
	else
		pItem->MoveToCheck(GetTopPoint(), this);	// put at my feet.
	return 0;
}

int CChar::Skill_Lumberjack( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Lumberjack");
	// SKILL_LUMBERJACK
	// m_Act_p = the point we want to chop/hack at.
	// m_Act_Prv_UID = Axe/Dagger
	// NOTE: The skill is used for hacking with IT_FENCE (e.g. i_dagger)
	//
	// RETURN:
	//   difficulty = 0-100

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_FAIL )
		return 0;

	CItem *pTool = m_Act_Prv_UID.ItemFind();
	if ( pTool == nullptr )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_TOOL);
		return -SKTRIG_ABORT;
	}

	if ( m_Act_p.m_x == -1 )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_6);
		return -SKTRIG_QTY;
	}

	// 3D distance check and LOS
	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_LUMBERJACKING);
	int iTargRange = GetTopPoint().GetDist(m_Act_p);
	int iMaxRange = pSkillDef->m_Range;
	if ( !pSkillDef->m_Range )
	{
		g_Log.EventError("Lumberjacking skill doesn't have a value for RANGE, defaulting to 2\n");
		iMaxRange = 2;
	}
	if ( iTargRange < 1 && !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_NOMINDIST) )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_CLOSE);
		return -SKTRIG_QTY;
	}
	if ( iTargRange > iMaxRange )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_REACH);
		return -SKTRIG_QTY;
	}
	if ( !CanSeeLOS(m_Act_p, nullptr, iMaxRange) )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_LOS);
		return -SKTRIG_QTY;
	}

	// Resource check
	CItem *pResBit = CWorldMap::CheckNaturalResource(m_Act_p, (IT_TYPE)(m_atResource.m_ridType.GetResIndex()), stage == SKTRIG_START, this);
	if ( !pResBit )
	{
		if ( pTool->IsType(IT_WEAPON_FENCE) )	//dagger
			SysMessageDefault(DEFMSG_LUMBERJACKING_3);	// not a tree
		else
			SysMessageDefault(DEFMSG_LUMBERJACKING_1);
		return -SKTRIG_QTY;
	}
	if ( pResBit->GetAmount() == 0 )
	{
		if ( pTool->IsType(IT_WEAPON_FENCE) )	//dagger
			SysMessageDefault(DEFMSG_LUMBERJACKING_4);	// no wood to harvest
		else
			SysMessageDefault(DEFMSG_LUMBERJACKING_2);
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_START )
	{
		m_atResource.m_dwStrokeCount = (word)(Calc_GetRandVal(5) + 2);
		return Skill_NaturalResource_Setup(pResBit);	// How difficult? 1-1000
	}

	if ( pTool->IsType(IT_WEAPON_FENCE) )	//dagger end
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_5);
		ItemBounce(CItem::CreateScript(ITEMID_KINDLING1, this));
		pResBit->ConsumeAmount(1);
		return 0;
	}

	CItem *pItem = Skill_NaturalResource_Create(pResBit, SKILL_LUMBERJACKING);
	if ( !pItem )
	{
		SysMessageDefault(DEFMSG_LUMBERJACKING_2);
		return -SKTRIG_FAIL;
	}

	if ( m_atResource.m_dwBounceItem )
		ItemBounce(pItem);
	else
		pItem->MoveToCheck(GetTopPoint(), this);	// put at my feet.
	return 0;
}

int CChar::Skill_DetectHidden( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_DetectHidden");
	// SKILL_DETECTINGHIDDEN
	// Look around for who is hiding.
	// Detect them based on skill diff.
	// ??? Hidden objects ?

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_START )
		return 10;		// difficulty based on who is hiding ?

	if ( stage == SKTRIG_FAIL || stage == SKTRIG_STROKE )
		return 0;

	if ( stage != SKTRIG_SUCCESS )
	{
		ASSERT(0);
		return -SKTRIG_QTY;
	}

	if ( !(g_Cfg.m_iRevealFlags & REVEALF_DETECTINGHIDDEN) )	// skill succeeded, but effect is disabled
		return 0;

	int iSkill = Skill_GetAdjusted(SKILL_DETECTINGHIDDEN);
	int iRadius = iSkill / 100;

	//If Effect property is defined on the Detect Hidden skill use it instead of the hardcoded radius value.
	CSkillDef * pSkillDef = g_Cfg.GetSkillDef(SKILL_DETECTINGHIDDEN);
	if (!pSkillDef->m_vcEffect.m_aiValues.empty())
		iRadius = pSkillDef->m_vcEffect.GetLinear(iSkill);

	CWorldSearch Area(GetTopPoint(), iRadius);
	bool bFound = false;
	for (;;)
	{
		CChar *pChar = Area.GetChar();
		if ( pChar == nullptr )
			break;
		if ( pChar == this || !pChar->IsStatFlag(STATF_INVISIBLE|STATF_HIDDEN) )
			continue;

		// Check chance to reveal the target
		int iSkillSrc = iSkill + Calc_GetRandVal(210) - 100;
		int iSkillTarg = pChar->Skill_GetAdjusted(SKILL_HIDING) + Calc_GetRandVal(210) - 100;
		if ( iSkillSrc < iSkillTarg )
			continue;

		pChar->Reveal();
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_DETECTHIDDEN_SUCC), pChar->GetName());
		bFound = true;
	}

	if ( !bFound )
		return -SKTRIG_FAIL;

	return 0;
}

int CChar::Skill_Cartography( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Cartography");
	// SKILL_CARTOGRAPHY
	// m_atCreate.m_iItemID = map we are making.
	// m_atCreate.m_dwAmount = amount of said item.

	if ( stage == SKTRIG_START )
	{
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
			Sound( 0x249 );
	}

	return( Skill_MakeItem( stage ));	// How difficult? 1-1000
}

int CChar::Skill_Musicianship( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Musicianship");
	// m_Act_UID = the intrument i targetted to play.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE  )
		return 0;
	if ( stage == SKTRIG_START )
		return Use_PlayMusic( m_Act_UID.ItemFind(), Calc_GetRandVal(90));	// How difficult? 1-1000. If no instrument, it immediately fails

	return 0;
}

int CChar::Skill_Peacemaking( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Peacemaking");
	// try to make all those listening peacable.
	// General area effect.
	// make peace if possible. depends on who is listening/fighting.

	if ( stage == SKTRIG_STROKE )
		return 0;

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	switch ( stage )
	{
		case SKTRIG_START:
		{
			// ACTARG1: UID of the instrument i want to play, instead of picking a random one
			CItem * pInstrument = nullptr;
			if (m_atBard.m_dwInstrumentUID != 0)
			{
				CObjBase * pObj = CUID::ObjFindFromUID(m_atBard.m_dwInstrumentUID);
				if (pObj && pObj->IsItem())
				{
					pInstrument = static_cast<CItem*>(pObj);
					if (pInstrument->GetType() != IT_MUSICAL)
					{
						DEBUG_WARN(("Invalid ACTARG1 when using skill Peacemaking. Expected zero or the UID of an item with type t_musical.\n"));
						pInstrument = nullptr;
					}
				}
			}
			
			// Basic skill check.
			int iDifficulty = Use_PlayMusic(pInstrument, Calc_GetRandVal(40));
			if (iDifficulty < -1)	// no instrument: immediate fail
				return -SKTRIG_ABORT;

			if ( !iDifficulty )
				iDifficulty = Calc_GetRandVal(40);	// Depend on evil of the creatures here.

			return iDifficulty;		// How difficult? 1-1000
		}

		case SKTRIG_FAIL:
			return 0;

		case SKTRIG_SUCCESS:
		{
			int peace = Skill_GetAdjusted(SKILL_PEACEMAKING);
			int iRadius = ( peace / 100 ) + 2;	// 2..12
			CWorldSearch Area(GetTopPoint(), iRadius);
			for (;;)
			{
				CChar *pChar = Area.GetChar();
				if ( pChar == nullptr )
					return -SKTRIG_FAIL;
				if (( pChar == this ) || !CanSee(pChar) )
					continue;

				int iBardingDiff = (int)pChar->GetKeyNum("BARDING.DIFF");

				int iPeaceDiff = pChar->Skill_GetAdjusted(SKILL_PEACEMAKING);
				if (iBardingDiff != 0)
					iPeaceDiff = ((iPeaceDiff + iBardingDiff) / 2);

				if ( iPeaceDiff > peace )
					SysMessagef("%s %s.", pChar->GetName(),g_Cfg.GetDefaultMsg( DEFMSG_PEACEMAKING_IGNORE ));
				else
				{
					int iProvoDiff = pChar->Skill_GetAdjusted(SKILL_PROVOCATION);
					if ( iBardingDiff != 0 )
						iProvoDiff = ((iProvoDiff + iBardingDiff) / 2);

					if ( iProvoDiff > peace )
					{
						SysMessagef("%s %s.", pChar->GetName(),g_Cfg.GetDefaultMsg( DEFMSG_PEACEMAKING_DISOBEY ));
						if ( pChar->Noto_IsEvil() )
							pChar->Fight_Attack(this);
					}
					else
						pChar->Fight_ClearAll();
				}
				break;
			}
			return 0;
		}
		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Enticement( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Enticement");
	// m_Act_UID = my target
	// Just keep playing and trying to allure them til we can't
	// Must have a musical instrument.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar *pChar = m_Act_UID.CharFind();
	if ( !pChar || !CanSee(pChar) )
		return -SKTRIG_QTY;

	switch ( stage )
	{
		case SKTRIG_START:
		{
			// ACTARG1: UID of the instrument i want to play, instead of picking a random one
			CItem * pInstrument = nullptr;
			if (m_atBard.m_dwInstrumentUID != 0)
			{
				CObjBase * pObj = CUID::ObjFindFromUID(m_atBard.m_dwInstrumentUID);
				if (pObj && pObj->IsItem())
				{
					pInstrument = static_cast<CItem*>(pObj);
					if (pInstrument->GetType() != IT_MUSICAL)
					{
						DEBUG_WARN(("Invalid ACTARG1 when using skill Enticement. Expected zero or the UID of an item with type t_musical.\n"));
						pInstrument = nullptr;
					}		
				}
			}

			// Basic skill check.
			CChar* pAct = m_Act_UID.CharFind();
			if (!pAct)
			{
				g_Log.EventError("Act empty in skill Enticement, trigger @Start.\n");
				return -SKTRIG_ABORT;
			}
			
			int iBaseDiff = (int)pChar->GetKeyNum("BARDING.DIFF");
			if (iBaseDiff != 0)
				iBaseDiff = iBaseDiff / 18;
			else
				iBaseDiff = 40;		// No TAG.BARDING.DIFF? Use default

			int iDifficulty = Use_PlayMusic(pInstrument, Calc_GetRandVal(iBaseDiff));	// How difficult? 1-100 (use RandBell). If no instrument, it immediately fails
			if (iDifficulty < -1)	// no instrument: immediate fail
				return -SKTRIG_ABORT;

			if ( !iDifficulty )
				iDifficulty = Calc_GetRandVal(40);	// Depend on evil of the creatures here.

			return iDifficulty;
		}

		case SKTRIG_FAIL:
			return 0;

		case SKTRIG_SUCCESS:
		{
			if ( pChar->m_pPlayer )
			{
				SysMessagef("%s", g_Cfg.GetDefaultMsg( DEFMSG_ENTICEMENT_PLAYER ));
				return -SKTRIG_ABORT;
			}
			else if ( pChar->IsStatFlag(STATF_WAR) )
			{
				SysMessagef("%s %s.", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_ENTICEMENT_BATTLE));
				return -SKTRIG_ABORT;
			}

			pChar->m_Act_p = GetTopPoint();
			pChar->NPC_WalkToPoint( ( pChar->m_Act_p.GetDist(pChar->GetTopPoint()) > 3) );
			return 0;
		}

		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Provocation(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Provocation");
	// m_Act_Prv_UID = provoke this person
	// m_Act_UID = against this person.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar *pCharProv = m_Act_Prv_UID.CharFind();
	CChar *pCharTarg = m_Act_UID.CharFind();

	if ( !pCharProv || !pCharTarg || ( pCharProv == this ) || ( pCharTarg == this ) || ( pCharProv == pCharTarg ) ||
		pCharProv->IsStatFlag(STATF_PET|STATF_CONJURED|STATF_STONE|STATF_DEAD|STATF_INVUL) ||
		pCharTarg->IsStatFlag(STATF_PET|STATF_CONJURED|STATF_STONE|STATF_DEAD|STATF_INVUL) )
	{
		SysMessageDefault(DEFMSG_PROVOCATION_UPSET);
		return -SKTRIG_QTY;
	}

	if ( pCharProv->m_pPlayer || pCharTarg->m_pPlayer )
	{
		SysMessageDefault(DEFMSG_PROVOCATION_PLAYER);
		return -SKTRIG_ABORT;
	}

	if ( !CanSee(pCharProv) || !CanSee(pCharTarg) )
		return -SKTRIG_ABORT;

	switch ( stage )
	{
		case SKTRIG_START:
		{
			// ACTARG1: UID of the instrument i want to play, instead of picking a random one
			CItem * pInstrument = nullptr;
			if (m_atBard.m_dwInstrumentUID != 0)
			{
				CObjBase * pObj = CUID::ObjFindFromUID(m_atBard.m_dwInstrumentUID);
				if (pObj && pObj->IsItem())
				{
					pInstrument = static_cast<CItem*>(pObj);
					if (pInstrument->GetType() != IT_MUSICAL)
					{
						DEBUG_WARN(("Invalid ACTARG1 when using skill Provocation. Expected zero or the UID of an item with type t_musical.\n"));
						pInstrument = nullptr;
					}
				}
			}

			// Basic skill check.
			CChar* pAct = dynamic_cast<CChar*>(m_Act_UID.CharFind());
			if (!pAct)
			{
				g_Log.EventError("Act empty in skill Provocation, trigger @Start.\n");
				return -SKTRIG_ABORT;
			}
			int iBaseDiff = (int)pAct->GetKeyNum("BARDING.DIFF");
			if (iBaseDiff != 0)
				iBaseDiff = iBaseDiff / 18;
			else
				iBaseDiff = 40;		// No TAG.BARDING.DIFF? Use default

			int iDifficulty = Use_PlayMusic(pInstrument, Calc_GetRandVal(iBaseDiff));	// How difficult? 1-100 (use RandBell). If no instrument, it immediately fails
			if (iDifficulty < -1)	// no instrument: immediate fail
				return -SKTRIG_ABORT;

			if ( !iDifficulty )
				iDifficulty = pCharProv->Stat_GetAdjusted(STAT_INT);	// Depend on evil of the creature.

			if ( pCharProv->Skill_GetAdjusted(SKILL_PROVOCATION) >= Skill_GetAdjusted(SKILL_PROVOCATION) )	// cannot provoke more experienced provoker
				iDifficulty = 0;

			if ( NPC_GetAllyGroupType(pCharProv->GetDispID()) == NPC_GetAllyGroupType(pCharTarg->GetDispID()) )
				m_atProvocation.m_dwIsAlly = 1;	// If still true on @Success, the action will fail because the provoked refuses to attack the target.

			return iDifficulty;
		}

		case SKTRIG_FAIL:
		{
			pCharProv->Fight_Attack(this);
			return 0;
		}

		case SKTRIG_SUCCESS:
			// They are just too good for this.
			if ( pCharProv->GetKarma() >= Calc_GetRandVal2(1000, 10000) )
			{
				pCharProv->Emote(g_Cfg.GetDefaultMsg(DEFMSG_PROVOCATION_EMOTE_1));
				return -SKTRIG_ABORT;
			}

			pCharProv->Emote(g_Cfg.GetDefaultMsg(DEFMSG_PROVOCATION_EMOTE_2));

			// He realizes that you are the real bad guy as well.
			if ( !pCharTarg->OnAttackedBy(this, true) )
				return -SKTRIG_ABORT;

			pCharProv->Memory_AddObjTypes(this, MEMORY_AGGREIVED|MEMORY_IRRITATEDBY);

			// If out of range we might get attacked ourself.
			if ( (pCharProv->GetTopDist3D(pCharTarg) > UO_MAP_VIEW_SIGHT) || (pCharProv->GetTopDist3D(this) > UO_MAP_VIEW_SIGHT) )
			{
				// Check that only "evil" monsters attack provoker back
				if ( pCharProv->Noto_IsEvil() )
					pCharProv->Fight_Attack(this);

				return -SKTRIG_ABORT;
			}

			// If the npcs are in the same ally groups, both can attack you
			if ( m_atProvocation.m_dwIsAlly != 0 )
			{
				if ( pCharProv->Noto_IsEvil() )
				{
					pCharProv->Fight_Attack(this);
					pCharTarg->Fight_Attack(this);
				}
				SysMessageDefault(DEFMSG_PROVOCATION_KIND);
				return -SKTRIG_ABORT;
			}
			

			// If the provoked NPC/PC is good, we are flagged criminal for it and guards are called.
			if ( pCharProv->Noto_GetFlag(this) == NOTO_GOOD )
			{
				// lose some karma for this.
				CheckCrimeSeen(SKILL_PROVOCATION, nullptr, pCharProv, g_Cfg.GetDefaultMsg( DEFMSG_PROVOKING_CRIME ));
				return -SKTRIG_ABORT;	// can't provoke a good target!
			}

			// If we provoke upon a good char we should go criminal for it but skill still succeed.
			if ( pCharTarg->Noto_GetFlag(this) == NOTO_GOOD )
				CheckCrimeSeen(SKILL_PROVOCATION, nullptr, pCharTarg, "provoking");

			pCharProv->Fight_Attack(pCharTarg); // Make the actual provoking.
			return 0;

		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Poisoning( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Poisoning");
	// Act_TargPrv = poison this weapon/food
	// Act_Targ = with this poison.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE )
		return 0;

	CItem * pPoison = m_Act_UID.ItemFind();
	if ( pPoison == nullptr || !pPoison->IsType(IT_POTION))
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_START )
		return Calc_GetRandVal( 60 );	// How difficult? 1-1000

	if ( stage == SKTRIG_FAIL )
		return 0;	// lose the poison sometimes ?

	if ( RES_GET_INDEX(pPoison->m_itPotion.m_Type) != SPELL_Poison )
		return -SKTRIG_ABORT;

	CItem * pItem = m_Act_Prv_UID.ItemFind();
	if ( pItem == nullptr )
		return -SKTRIG_QTY;

	if ( stage != SKTRIG_SUCCESS )
	{
		ASSERT(0);
		return -SKTRIG_ABORT;
	}

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
		Sound( 0x247 );	// powdering.

	switch ( pItem->GetType() )
	{
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
			pItem->m_itFood.m_poison_skill = (byte)(pPoison->m_itPotion.m_dwSkillQuality / 10);
			break;
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_SWORD:		// 13 =
		case IT_WEAPON_FENCE:		// 14 = can't be used to chop trees. (make kindling)
			pItem->m_itWeapon.m_poison_skill = (byte)(pPoison->m_itPotion.m_dwSkillQuality / 10);
			pItem->UpdatePropertyFlag();
			break;
		default:
			SysMessageDefault( DEFMSG_POISONING_WITEM );
			return -SKTRIG_QTY;
	}
	// skill + quality of the poison.
	SysMessageDefault( DEFMSG_POISONING_SUCCESS );
	pPoison->ConsumeAmount();
	return 0;
}

int CChar::Skill_Cooking( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Cooking");
	// m_atCreate.m_iItemID = create this item
	// m_Act_p = the heat source
	// m_Act_UID = the skill tool

	int iMaxDist = 3;

	if ( stage == SKTRIG_START )
	{
		m_Act_p = CWorldMap::FindItemTypeNearby( GetTopPoint(), IT_FIRE, iMaxDist, false );
		if ( ! m_Act_p.IsValidPoint())
		{
			m_Act_p = CWorldMap::FindItemTypeNearby( GetTopPoint(), IT_FORGE, iMaxDist, false );
			if ( ! m_Act_p.IsValidPoint())
			{
				m_Act_p = CWorldMap::FindItemTypeNearby( GetTopPoint(), IT_CAMPFIRE, iMaxDist, false );
				if ( ! m_Act_p.IsValidPoint())
				{
					SysMessageDefault( DEFMSG_COOKING_FIRE_SOURCE );
					return -SKTRIG_QTY;
				}
			}
		}
		UpdateDir( m_Act_p );	// toward the fire source
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( GetTopPoint().GetDist( m_Act_p ) > iMaxDist )
			return( -SKTRIG_FAIL );
	}

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Taming( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Taming");
	// m_Act_UID = creature to tame.
	// Check the min required skill for this creature.
	// Related to INT ?

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	CChar * pChar = m_Act_UID.CharFind();
	if ( pChar == nullptr )
		return -SKTRIG_QTY;

	if ( pChar == this )
	{
		SysMessageDefault( DEFMSG_TAMING_YMASTER );
		return -SKTRIG_QTY;
	}
	if ( pChar->m_pPlayer )
	{
		SysMessageDefault( DEFMSG_TAMING_CANT );
		return -SKTRIG_QTY;
	}

	CSkillDef* pSkillDef = g_Cfg.GetSkillDef(SKILL_TAMING);
	if (pSkillDef->m_Range <= 0)
		pSkillDef->m_Range = 10;

	if ( GetTopDist3D(pChar) > pSkillDef->m_Range)
	{
		SysMessageDefault( DEFMSG_TAMING_REACH );
		return -SKTRIG_QTY;
	}
	
	if ( !CanSeeLOS( pChar ) )
	{
		SysMessageDefault( DEFMSG_TAMING_LOS );
		return -SKTRIG_QTY;
	}
	UpdateDir( pChar );

	ASSERT( pChar->m_pNPC );

	int iTameBase = pChar->Skill_GetBase(SKILL_TAMING);
	if ( !IsPriv( PRIV_GM )) // if its a gm doing it, just check that its not
	{
		if ( pChar->IsStatFlag( STATF_PET ))		// is it tamable ?
		{
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_TAMING_TAME ), pChar->GetName());
			return -SKTRIG_QTY;
		}

		/* An NPC cannot be tamed if:
			Its Taming skill is equal to 0.
			Its Animal Lore is above 0 (no reason why, this is probably an old check)
			It's ID is either one of the playable characters (Human, Elf or Gargoyle).
		*/
		if ( !iTameBase || pChar->Skill_GetBase(SKILL_ANIMALLORE) || 
			pChar->IsPlayableCharacter())
		{
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_TAMING_TAMED ), pChar->GetName());
			return -SKTRIG_QTY;
		}

		if (IsSetOF(OF_PetSlots))
		{
			short iFollowerSlots = (short)pChar->GetDefNum("FOLLOWERSLOTS", true, 1);
			if (!FollowersUpdate(pChar, maximum(0, iFollowerSlots), true))
			{
				SysMessageDefault(DEFMSG_PETSLOTS_TRY_TAMING);
				return -SKTRIG_QTY;
			}
		}
		
	}

	if ( stage == SKTRIG_START )
	{
		int iDifficulty = iTameBase/10;
		if ( pChar->Memory_FindObjTypes( this, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_AGGREIVED ))	// I've attacked it before ?
			iDifficulty += 50;	// is it too much?

		m_atTaming.m_dwStrokeCount = (word)(Calc_GetRandVal(4) + 2);
		return iDifficulty;		// How difficult? 1-1000
	}

	if ( stage == SKTRIG_FAIL )
		return 0;

	if ( stage == SKTRIG_STROKE )
	{
		static lpctstr const sm_szTameSpeak[] =
		{
			g_Cfg.GetDefaultMsg( DEFMSG_TAMING_1 ),
			g_Cfg.GetDefaultMsg( DEFMSG_TAMING_2 ),
			g_Cfg.GetDefaultMsg( DEFMSG_TAMING_3 ),
			g_Cfg.GetDefaultMsg( DEFMSG_TAMING_4 )
		};

		if ( (m_atTaming.m_dwStrokeCount <= 0) || IsPriv( PRIV_GM ))
			return 0;

		tchar * pszMsg = Str_GetTemp();
		snprintf(pszMsg, STR_TEMPLENGTH, sm_szTameSpeak[ Calc_GetRandVal( CountOf( sm_szTameSpeak )) ], pChar->GetName());
		Speak(pszMsg);

		// Keep trying and updating the animation
		--m_atTaming.m_dwStrokeCount;
		Skill_SetTimeout();
		return -SKTRIG_STROKE;
	}

	ASSERT( stage == SKTRIG_SUCCESS );

	// Check if I tamed it before
    bool fTamedPrev = false;
	CItemMemory * pMemory = pChar->Memory_FindObjTypes( this, MEMORY_SPEAK );
	if ( pMemory && pMemory->m_itEqMemory.m_Action == NPC_MEM_ACT_TAMED)
	{
		// I did, no skill to tame it again
        fTamedPrev = true;

		tchar * pszMsg = Str_GetTemp();
		snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg( DEFMSG_TAMING_REMEMBER ), pChar->GetName());
		ObjMessage(pszMsg, pChar );
	}

    pChar->NPC_PetSetOwner(this);
    pChar->Stat_SetVal(STAT_FOOD, 50);	// this is good for something.
    pChar->m_Act_UID = GetUID();
    pChar->Skill_Start(NPCACT_FOLLOW_TARG);

    if (fTamedPrev)
        return -SKTRIG_QTY;	// no credit for this.

	SysMessageDefault( DEFMSG_TAMING_SUCCESS );

	// Create the memory of being tamed to prevent lame macroers
	pMemory = pChar->Memory_AddObjTypes(this, MEMORY_SPEAK);
	if ( pMemory )
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_TAMED;

	return 0;
}

int CChar::Skill_Lockpicking( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Lockpicking");
	// m_Act_UID = the item to be picked.
	// m_Act_Prv_UID = The pick.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE )
		return 0;

	CItem * pPick = m_Act_Prv_UID.ItemFind();
	if ( pPick == nullptr || ! pPick->IsType( IT_LOCKPICK ))
	{
		SysMessageDefault( DEFMSG_LOCKPICKING_NOPICK );
		return -SKTRIG_QTY;
	}

	CItem * pLock = m_Act_UID.ItemFind();
	if ( pLock == nullptr )
	{
		SysMessageDefault( DEFMSG_LOCKPICKING_WITEM );
		return -SKTRIG_QTY;
	}

	if ( pPick->GetTopLevelObj() != this )	// the pick is gone !
	{
		SysMessageDefault( DEFMSG_LOCKPICKING_PREACH );
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_FAIL )
	{
		pPick->OnTakeDamage( 1, this, DAMAGE_HIT_BLUNT );	// damage my pick
		return 0;
	}

	if ( !CanTouch( pLock ))	// we moved too far from the lock
	{
		SysMessageDefault( DEFMSG_LOCKPICKING_REACH );
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_START )
		return pLock->Use_LockPick( this, true, false );	// How difficult? 1-1000

	ASSERT( stage == SKTRIG_SUCCESS );

	if ( pLock->Use_LockPick( this, false, false ) < 0 )
		return -SKTRIG_FAIL;

	return 0;
}

int CChar::Skill_Hiding( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Hiding");
	// SKILL_Hiding
	// Skill required varies with terrain and situation ?
	// if we are carrying a light source then this should not work.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE )	// we shoud just stay in HIDING skill ?
		return 0;

	if ( stage == SKTRIG_FAIL )
	{
		Reveal();
		return 0;
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		ObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_HIDING_SUCCESS), this);
		StatFlag_Set(STATF_HIDDEN);
		Reveal(STATF_INVISIBLE);	// clear previous invisibility spell effect (this will not reveal the char because STATF_HIDDEN still set)
		UpdateMode(nullptr, true);
		if ( IsClientActive() )
		{
			GetClientActive()->removeBuff( BI_HIDDEN );
			GetClientActive()->addBuff( BI_HIDDEN , 1075655, 1075656 );
		}
		return 0;
	}

	if ( stage == SKTRIG_START )
	{
		// Make sure I'm not carrying a light ?
		for (const CSObjContRec* pObjRec : *this)
		{
			const CItem* pItem = static_cast<const CItem*>(pObjRec);
			if ( !CItemBase::IsVisibleLayer( pItem->GetEquipLayer()))
				continue;
			if ( pItem->Can( CAN_I_LIGHT ))
			{
				SysMessageDefault( DEFMSG_HIDING_TOOLIT );
				return -SKTRIG_QTY;
			}
		}

		return Calc_GetRandVal(70);	// How difficult? 1-1000
	}
	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Herding( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Herding");
	// m_Act_UID = move this creature.
	// m_Act_p = move to here.
	// How do I make them move fast ? or with proper speed ???

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar	*pChar = m_Act_UID.CharFind();
	CItem	*pCrook = m_Act_Prv_UID.ItemFind();
	if ( !pChar )
	{
		SysMessageDefault(DEFMSG_HERDING_LTARG);
		return -SKTRIG_QTY;
	}
	if ( !pCrook )
	{
		SysMessageDefault(DEFMSG_HERDING_NOCROOK);
		return -SKTRIG_QTY;
	}

	switch ( stage )
	{
		case SKTRIG_START:
		{
			if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOANIM ) )
				UpdateAnimate(ANIM_ATTACK_WEAPON);

			int iIntVal = pChar->Stat_GetAdjusted(STAT_INT) / 2;
			return iIntVal + Calc_GetRandVal(iIntVal);	// How difficult? 1-1000
		}

		case SKTRIG_FAIL:
			return 0;

		case SKTRIG_SUCCESS:
		{
			if ( IsPriv(PRIV_GM) )
			{
				if ( pChar->GetPrivLevel() > GetPrivLevel() )
				{
					SysMessagef("%s %s", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_HERDING_PLAYER) );
					return -SKTRIG_ABORT;
				}

				pChar->Spell_Teleport(m_Act_p, true, false);
			}
			else
			{
				if ( pChar->m_pPlayer || !pChar->m_pNPC || pChar->m_pNPC->m_Brain != NPCBRAIN_ANIMAL )
				{
					SysMessagef("%s %s", pChar->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_HERDING_PLAYER) );
					return -SKTRIG_ABORT;
				}

				pChar->m_Act_p = m_Act_p;
				pChar->Skill_Start(NPCACT_GOTO);
			}

			ObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_HERDING_SUCCESS), pChar);
			return 0;
		}

		default:
			break;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_SpiritSpeak( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_SpiritSpeak");

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_FAIL || stage == SKTRIG_STROKE )
		return 0;

	if ( stage == SKTRIG_START )
		return( Calc_GetRandVal( 90 ));		// How difficult? 1-1000. difficulty based on spirits near ?

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( IsStatFlag( STATF_SPIRITSPEAK ))
			return -SKTRIG_ABORT;
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
			Sound( 0x24a );

		SysMessageDefault( DEFMSG_SPIRITSPEAK_SUCCESS );
		Spell_Effect_Create( SPELL_NONE, LAYER_FLAG_SpiritSpeak, g_Cfg.GetSpellEffect(SPELL_NONE, 1), 4*60*MSECS_PER_TENTH, this );
		return 0;
	}

	ASSERT(0);
	return -SKTRIG_ABORT;
}

int CChar::Skill_Meditation( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Meditation");
	// SKILL_MEDITATION
	// Try to regen your mana even faster than normal.
	// Give experience only when we max out.

	if (stage == SKTRIG_ABORT)
	{
		if ( IsClientActive() )
			GetClientActive()->removeBuff(BI_ACTIVEMEDITATION);

		return -SKTRIG_ABORT;
	}
	if ( stage == SKTRIG_FAIL )
	{
		if ( IsClientActive() )
			GetClientActive()->removeBuff(BI_ACTIVEMEDITATION);
		return 0;
	}

	if ( stage == SKTRIG_START )
	{
		if ( Stat_GetVal(STAT_INT) >= Stat_GetMaxAdjusted(STAT_INT))
		{
			SysMessageDefault( DEFMSG_MEDITATION_PEACE_1 );
			return -SKTRIG_QTY;
		}

		m_atTaming.m_dwStrokeCount = 0;
		SysMessageDefault( DEFMSG_MEDITATION_TRY );
		return Calc_GetRandVal(100);	// How difficult? 1-1000. how hard to get started ?
	}
	if ( stage == SKTRIG_STROKE )
		return 0;

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( Stat_GetVal(STAT_INT) >= Stat_GetMaxAdjusted(STAT_INT))
		{
			if ( IsClientActive() )
				GetClientActive()->removeBuff(BI_ACTIVEMEDITATION);
			SysMessageDefault( DEFMSG_MEDITATION_PEACE_2 );
			return 0;	// only give skill credit now.
		}

		if ( m_atTaming.m_dwStrokeCount == 0 )
		{
			if ( IsClientActive() )
				GetClientActive()->addBuff(BI_ACTIVEMEDITATION, 1075657, 1075658);
			if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
				Sound( 0x0f9 );
		}
		++m_atTaming.m_dwStrokeCount;

		//If Effect property is defined on the Meditation skill use it instead of the hardcoded  value.
		CSkillDef * pSkillDef = g_Cfg.GetSkillDef(SKILL_MEDITATION);
		if (!pSkillDef->m_vcEffect.m_aiValues.empty())
			UpdateStatVal(STAT_INT, (ushort)pSkillDef->m_vcEffect.GetLinear(Skill_GetAdjusted(SKILL_MEDITATION)));
		else
			UpdateStatVal( STAT_INT, 1 );
		Skill_SetTimeout();		// next update (depends on skill)

		// Set a new possibility for failure ?
		// iDifficulty = Calc_GetRandVal(100);
		return -SKTRIG_STROKE;
	}
	return -SKTRIG_QTY;
}

int CChar::Skill_Healing( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Healing");
	// SKILL_VETERINARY:
	// SKILL_HEALING
	// m_Act_Prv_UID = bandages.
	// m_Act_UID = heal target.
	//
	// should depend on the severity of the wounds ?
	// should be just a fast regen over time ?
	// RETURN:
	//  = -3 = failure.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE )
		return 0;

	CItem * pBandage = m_Act_Prv_UID.ItemFind();
	if ( pBandage == nullptr )
	{
		SysMessageDefault( DEFMSG_HEALING_NOAIDS );
		return -SKTRIG_QTY;
	}
	if ( ! pBandage->IsType(IT_BANDAGE))
	{
		SysMessageDefault( DEFMSG_HEALING_WITEM );
		return -SKTRIG_QTY;
	}

	CObjBase * pObj = m_Act_UID.ObjFind();
	if ( !CanTouch(pObj))
	{
		SysMessageDefault( DEFMSG_HEALING_REACH );
		return -SKTRIG_QTY;
	}
	
	CChar * pChar = dynamic_cast<CChar*>(pObj);
    if (pChar && pChar->Can(CAN_C_NONSELECTABLE))
    {
        SysMessageDefault( DEFMSG_HEALING_NONCHAR );
        return -SKTRIG_QTY;
    }

    CItemCorpse * pCorpse = nullptr;	// resurrect by corpse
	if (pObj && pObj->IsItem())
	{
		pCorpse = dynamic_cast<CItemCorpse *>(pObj);
		if ( pCorpse == nullptr )
		{
			SysMessageDefault( DEFMSG_HEALING_NONCHAR );
			return -SKTRIG_QTY;
		}
		pChar = pCorpse->m_uidLink.CharFind();
	}

	if ( pChar == nullptr )
	{
		SysMessageDefault( DEFMSG_HEALING_BEYOND );
		return -SKTRIG_QTY;
	}

	if ( GetDist(pObj) > 2 )
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_TOOFAR ), pObj->GetName());
		if ( pChar != this )
			pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_ATTEMPT), GetName(), (pCorpse ? pCorpse->GetName() : g_Cfg.GetDefaultMsg(DEFMSG_HEALING_WHO)));

		return -SKTRIG_QTY;
	}

	if ( pCorpse && !pCorpse->IsCorpseResurrectable(this,pChar))
	{
		return -SKTRIG_QTY;
	}

	if ( !pChar->IsStatFlag( STATF_POISONED|STATF_DEAD ) && (pChar->Stat_GetVal(STAT_STR) >= pChar->Stat_GetMaxAdjusted(STAT_STR)) )
	{
		if ( pChar == this )
			SysMessageDefault( DEFMSG_HEALING_HEALTHY );
		else
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_NONEED ), pChar->GetName());

		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_FAIL )
	{
		// just consume the bandage on fail and give some credit for trying.
		pBandage->ConsumeAmount();

		if ( pChar != this )
			pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_ATTEMPTF), GetName(), (pCorpse ? pCorpse->GetName() : g_Cfg.GetDefaultMsg(DEFMSG_HEALING_WHO)));

		// Harm the creature ?
		return -SKTRIG_FAIL;
	}

	if ( stage == SKTRIG_START )
	{
		if ( pChar != this )
		{
			tchar * pszMsg = Str_GetTemp();
			snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg( DEFMSG_HEALING_TO ), pChar->GetName());
			Emote(pszMsg);
		}
		else
		{
			Emote( g_Cfg.GetDefaultMsg( DEFMSG_HEALING_SELF ) );
		}
		if ( pCorpse || pChar->IsStatFlag(STATF_DEAD))	// resurrect
			return( 85 + Calc_GetRandVal(25));
		if ( pChar->IsStatFlag( STATF_POISONED ))	// poison level
			return( 50 + Calc_GetRandVal(50));

		return Calc_GetRandVal(80);	// Normal healing, How difficult? 1-1000
	}

	ASSERT( stage == SKTRIG_SUCCESS );
	pBandage->ConsumeAmount();

	CItem * pBloodyBandage = CItem::CreateScript(Calc_GetRandVal(2) ? ITEMID_BANDAGES_BLOODY1 : ITEMID_BANDAGES_BLOODY2, this );
	ItemBounce(pBloodyBandage);

	const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(Skill_GetActive());
	if (pSkillDef == nullptr)
		return -SKTRIG_QTY;

	if (pCorpse || pChar->IsStatFlag(STATF_DEAD))
	{
		pChar->Spell_Resurrection(pCorpse, this);
		return 0;
	}

	int iSkillLevel = Skill_GetAdjusted( Skill_GetActive());
	if ( pChar->IsStatFlag( STATF_POISONED ))
	{
		if ( g_Cfg.Calc_CurePoisonChance(pChar->LayerFind(LAYER_FLAG_Poison), iSkillLevel, IsPriv(PRIV_GM) ))
		{
			pChar->SetPoisonCure(true);
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_CURE_1), (pChar == this) ? g_Cfg.GetDefaultMsg(DEFMSG_HEALING_YOURSELF) : (pChar->GetName()));
			if (pChar != this)
				pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_CURE_2), GetName());
		}
		else 
		{
			SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_CURE_3));
			pChar->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_HEALING_CURE_4));
			return -SKTRIG_ABORT;
		}
		return 0;
	}

	// LAYER_FLAG_Bandage
	pChar->UpdateStatVal( STAT_STR, (ushort)(pSkillDef->m_vcEffect.GetLinear(iSkillLevel)) );
	return 0;
}

int CChar::Skill_RemoveTrap( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_RemoveTrap");
	// m_Act_UID = trap
	// Is it a trap ?

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE  )
		return 0;

	CItem * pTrap = m_Act_UID.ItemFind();
	if ( pTrap == nullptr || ! pTrap->IsType(IT_TRAP))
	{
		SysMessageDefault( DEFMSG_REMOVETRAPS_WITEM );
		return -SKTRIG_QTY;
	}
	if ( ! CanTouch(pTrap))
	{
		SysMessageDefault( DEFMSG_REMOVETRAPS_REACH );
		return -SKTRIG_QTY;
	}
	if ( stage == SKTRIG_START )
	{
		return Calc_GetRandVal(95);		// How difficult? 1-1000
	}
	if ( stage == SKTRIG_FAIL )
	{
		Use_Item( pTrap );	// set it off ?
		return 0;
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		pTrap->SetTrapState( IT_TRAP_INACTIVE, ITEMID_NOTHING, 5*60 );	// disable it
		return 0;
	}
	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Begging( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Begging");
	// m_Act_UID = Our begging target..

	CChar * pChar = m_Act_UID.CharFind();
	if ( pChar == nullptr || pChar == this )
		return -SKTRIG_QTY;

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_START )
	{
		UpdateAnimate( ANIM_BOW );
		SysMessagef(g_Cfg.GetDefaultMsg( DEFMSG_BEGGING_START ), pChar->GetName());
		// How difficult? 1-1000
		return( pChar->Stat_GetAdjusted(STAT_INT));		// How difficult? 1-1000
	}
	if ( stage == SKTRIG_STROKE )
	{
		if ( m_pNPC )
			return -SKTRIG_STROKE;	// Keep it active.
		return 0;
	}
	if ( stage == SKTRIG_FAIL )
		return 0;	// Might they do something bad ?

	if ( stage == SKTRIG_SUCCESS )
		return 0;	// Now what ? Not sure how to make begging successful. Give something from my inventory ?

	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Magery( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Magery");
	// SKILL_MAGERY
	//  m_Act_p = location to cast to.
	//  m_Act_Prv_UID = the source of the spell.
	//  m_Act_UID = target for the spell.
	//  m_atMagery.m_iSpell = the spell.

	if ( stage == SKTRIG_STROKE )
		return 0;

	if ( stage == SKTRIG_ABORT )
	{
		Spell_CastFail(true);
		return -SKTRIG_ABORT;
	}

	if ( stage == SKTRIG_FAIL )
	{
		Spell_CastFail();
		return 0;
	}
	if ( stage == SKTRIG_SUCCESS )
	{
		const CSpellDef * tSpell = g_Cfg.GetSpellDef( m_atMagery.m_iSpell );
		if (tSpell == nullptr)
			return -SKTRIG_ABORT; //Before we returned 0, thus allowing the skill to continue its execution and possible gaining a skill increase when the spell ID was invalid.

		if ( IsClientActive() && IsSetMagicFlags( MAGICF_PRECAST ) && !tSpell->IsSpellType( SPELLFLAG_NOPRECAST ))
		{
			this->GetClientActive()->Cmd_Skill_Magery( this->m_atMagery.m_iSpell, this->GetClientActive()->m_Targ_Prv_UID.ObjFind() );
			return -SKTRIG_QTY;		// don't increase skill at this point. The client should select a target first.
		}
		else
		{
			if ( !Spell_CastDone())
				return -SKTRIG_ABORT;
			return 0;
		}
	}
	if ( stage == SKTRIG_START )	// How difficult? 1-1000
		return Spell_CastStart();	// NOTE: this should call SetTimeout();

	ASSERT(0);
	return -SKTRIG_ABORT;
}

int CChar::Skill_Fighting( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Fighting");
	// SKILL_ARCHERY:
	// SKILL_SWORDSMANSHIP:
	// SKILL_MACEFIGHTING:
	// SKILL_FENCING:
	// SKILL_WRESTLING:
	// SKILL_THROWING:
	//
	// m_Fight_Targ_UID = attack target.

	if ( stage == SKTRIG_START )
	{
		/*
		The two lines below need to be moved after the @SkillStart/@Start triggers, otherwise if we are using a custom combat system
		and we are forcing a miss (actdiff < 0) combat will be blocked because it will not pass the @SkillStart/@Start triggers check.
		*/
		//m_atFight.m_iWarSwingState = WAR_SWING_EQUIPPING;
		//Fight_HitTry();	// this cleans up itself, executes the code related to the current m_iWarSwingState and sets the needed timers.

		//When combat starts we need to set hit chance, otherwise ACTDIFF will be 0 and thus an automatic success.
		return g_Cfg.Calc_CombatChanceToHit(this,m_Fight_Targ_UID.CharFind());	// How difficult? 1-10000
	}

	if ( stage == SKTRIG_STROKE )
	{
		// Hit or miss my current target.
		if ( !IsStatFlag(STATF_WAR) )
        {
			return -SKTRIG_ABORT;
        }

        if (m_atFight.m_iWarSwingState == WAR_SWING_EQUIPPING)
        {
			// calculate the chance at every hit
			m_Act_Difficulty = g_Cfg.Calc_CombatChanceToHit(this, m_Fight_Targ_UID.CharFind());
            if ( !Skill_CheckSuccess(Skill_GetActive(), m_Act_Difficulty, false) )
                m_Act_Difficulty = -m_Act_Difficulty;	// will result in failure
        }
        Fight_HitTry();	// this cleans up itself, executes the code related to the current m_iWarSwingState and sets the needed timers.
		return -SKTRIG_STROKE;	// Stay in the skill till we hit.
	}

    if ((stage == SKTRIG_FAIL) || (stage == SKTRIG_ABORT))
    {
        m_atFight.m_iRecoilDelay = 0;
        m_atFight.m_iSwingAnimationDelay = 0;
        m_atFight.m_iSwingAnimation = 0;
        m_atFight.m_iSwingIgnoreLastHitTag = 0;
        return 0;
    }

	return -SKTRIG_QTY;
}

int CChar::Skill_MakeItem( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_MakeItem");
	// SKILL_ALCHEMY:
	// SKILL_BLACKSMITHING:
	// SKILL_BOWCRAFT:
	// SKILL_CARPENTRY:
	// SKILL_COOKING:
	// SKILL_INSCRIPTION:
	// SKILL_TAILORING:
	// SKILL_TINKERING:
	//
	// m_Act_UID = the item we want to be part of this process.
	// m_atCreate.m_iItemID = new item we are making
	// m_atCreate.m_dwAmount = amount of said item.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_START )
		return m_Act_Difficulty;	// keep the already set difficulty

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( ! Skill_MakeItem( m_atCreate.m_iItemID, m_Act_UID, SKTRIG_SUCCESS, false, (m_atCreate.m_dwAmount ? m_atCreate.m_dwAmount : 1) ))
			return -SKTRIG_ABORT;
		return 0;
	}
	if ( stage == SKTRIG_FAIL )
	{
		Skill_MakeItem( m_atCreate.m_iItemID, m_Act_UID, SKTRIG_FAIL );
		return 0;
	}
	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Tailoring( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Tailoring");
	if ( stage == SKTRIG_SUCCESS )
	{
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
			Sound( SOUND_SNIP );	// snip noise
	}

	return( Skill_MakeItem( stage ));
}

int CChar::Skill_Inscription( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Inscription");
	if ( stage == SKTRIG_START )
	{
		// Can we even attempt to make this scroll ?
		// m_atCreate.m_iItemID = create this item
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
			Sound( 0x249 );
	}

	return( Skill_MakeItem( stage ));	// How difficult? 1-1000
}

int CChar::Skill_Blacksmith( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Blacksmith");
	// m_atCreate.m_iItemID = create this item
	// m_Act_p = the anvil.
	// m_Act_UID = the hammer.

	int iMaxDist = 2;
	if ( stage == SKTRIG_START )
	{
		m_Act_p = CWorldMap::FindItemTypeNearby( GetTopPoint(), IT_FORGE, iMaxDist, false );
		if ( ! m_Act_p.IsValidPoint())
		{
			SysMessageDefault( DEFMSG_SMITHING_FORGE );
			return -SKTRIG_QTY;
		}
		UpdateDir( m_Act_p );			// toward the forge
		m_atCreate.m_dwStrokeCount = 2;	// + Calc_GetRandVal( 4 )
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( GetTopPoint().GetDist( m_Act_p ) > iMaxDist )
			return( -SKTRIG_FAIL );
	}

	return( Skill_MakeItem( stage ));	// How difficult? 1-1000
}

int CChar::Skill_Carpentry( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Carpentry");
	// m_Act_UID = the item we want to be part of this process.
	// m_atCreate.m_iItemID = new item we are making
	// m_atCreate.m_dwAmount = amount of said item.

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOSFX ) )
		Sound( 0x23d );

	if ( stage == SKTRIG_START )
		m_atCreate.m_dwStrokeCount = 2;	// + Calc_GetRandVal( 3 )

	return( Skill_MakeItem( stage ));	// How difficult? 1-1000
}

int CChar::Skill_Scripted( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Scripted");
	if (stage == SKTRIG_SUCCESS || stage == SKTRIG_FAIL || stage == SKTRIG_ABORT)
		Skill_Cleanup();

	if ( stage == SKTRIG_FAIL || stage == SKTRIG_START || stage == SKTRIG_STROKE || stage == SKTRIG_SUCCESS )
		return 0;

	return -SKTRIG_QTY;	// something odd
}

int CChar::Skill_Information( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Information");
	// SKILL_ANIMALLORE:
	// SKILL_ARMSLORE:
	// SKILL_ANATOMY:
	// SKILL_ITEMID:
	// SKILL_EVALINT:
	// SKILL_FORENSICS:
	// SKILL_TASTEID:
	// Difficulty should depend on the target item !!!??
	// m_Act_UID = target.

	if ( ! IsClientActive())	// purely informational
		return -SKTRIG_QTY;

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_FAIL || stage == SKTRIG_STROKE )
		return 0;

	SKILL_TYPE skill = Skill_GetActive();
	int iSkillLevel = Skill_GetAdjusted(skill);
	if ( stage == SKTRIG_START )
		return GetClientActive()->OnSkill_Info( skill, m_Act_UID, iSkillLevel, true );	// How difficult? 1-1000
	if ( stage == SKTRIG_SUCCESS )
		return GetClientActive()->OnSkill_Info( skill, m_Act_UID, iSkillLevel, false );

	ASSERT(0);
	return -SKTRIG_QTY;
}

int CChar::Skill_Act_Napping( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Act_Napping");
	// NPCACT_NAPPING:
	// we are taking a small nap. keep napping til we wake. (or move)
	// AFK command

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_START )
	{
		_SetTimeout(2000);
		return 0;
	}

	if ( stage == SKTRIG_STROKE )
	{
		if ( m_Act_p != GetTopPoint())
			return -SKTRIG_QTY;	// we moved.
		_SetTimeout(8000);
		Speak( "z", HUE_WHITE, TALKMODE_WHISPER );
		return -SKTRIG_STROKE;	// Stay in the skill till we hit.
	}

	return -SKTRIG_QTY;	// something odd
}

int CChar::Skill_Act_Breath( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Act_Breath");
	// NPCACT_BREATH
	// A Dragon I assume.
	// m_Fight_Targ_UID = my target.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE || stage == SKTRIG_FAIL )
		return 0;

	CChar * pChar = m_Fight_Targ_UID.CharFind();
	if ( pChar == nullptr )
		return -SKTRIG_QTY;

	m_Act_p = pChar->GetTopPoint();
	if (!IsSetCombatFlags(COMBAT_NODIRCHANGE))
		UpdateDir( m_Act_p );

	if ( stage == SKTRIG_START )
	{
		UpdateStatVal( STAT_DEX, -10 );
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOANIM ) )
			UpdateAnimate( ANIM_MON_Stomp );

		_SetTimeout(3000);
		return 0;
	}

	ASSERT( stage == SKTRIG_SUCCESS );

	if ( !CanSeeLOS(pChar) )
		return -SKTRIG_QTY;

	const CPointMap& pntMe = GetTopPoint();
	if ( pntMe.GetDist( m_Act_p ) > UO_MAP_VIEW_SIGHT )
		m_Act_p.StepLinePath( pntMe, UO_MAP_VIEW_SIGHT );

	int iDamage = (int)(GetDefNum("BREATH.DAM", true));

	if ( !iDamage )
	{
		iDamage = (Stat_GetVal(STAT_STR) * 5) / 100;
		if ( iDamage < 1 )
			iDamage = 1;
		else if ( iDamage > 200 )
			iDamage = 200;
	}

	HUE_TYPE hue = (HUE_TYPE)(GetDefNum("BREATH.HUE", true));
	ITEMID_TYPE id = (ITEMID_TYPE)(GetDefNum("BREATH.ANIM", true));
	EFFECT_TYPE effect = static_cast<EFFECT_TYPE>(GetDefNum("BREATH.TYPE",true));
	DAMAGE_TYPE iDmgType = (DAMAGE_TYPE)(GetDefNum("BREATH.DAMTYPE", true));

	/* AOS damage types (used by COMBAT_ELEMENTAL_ENGINE)*/
	int iDmgPhysical = 0, iDmgFire = 0, iDmgCold = 0, iDmgPoison = 0, iDmgEnergy = 0;

	if ( !id )
		id = ITEMID_FX_FIRE_BALL;
	if ( !effect )
		effect = EFFECT_BOLT;
	
	if (!iDmgType)
		iDmgType = DAMAGE_FIRE;

	if (iDmgType & DAMAGE_FIRE)
		iDmgFire = 100;
	else if (iDmgType & DAMAGE_COLD)
		iDmgCold = 100;
	else if (iDmgType & DAMAGE_POISON)
		iDmgPoison = 100;
	else if (iDmgType & DAMAGE_ENERGY)
		iDmgEnergy = 100;
	else
		iDmgPhysical = 100;
	iDmgType |= DAMAGE_BREATH;

	Sound( 0x227 );
	pChar->Effect( effect, id, this, 20, 30, false, hue );
	pChar->OnTakeDamage( iDamage, this, iDmgType, iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy);
	return 0;
}

int CChar::Skill_Act_Throwing( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Act_Throwing");
	// NPCACT_THROWING
	// m_Fight_Targ_UID = my target.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT; 

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar * pChar = m_Fight_Targ_UID.CharFind();
	if ( pChar == nullptr )
		return -SKTRIG_QTY;

	m_Act_p = pChar->GetTopPoint();
	if (!IsSetCombatFlags(COMBAT_NODIRCHANGE))
		UpdateDir( m_Act_p );

	if ( stage == SKTRIG_START )
	{
		UpdateStatVal( STAT_DEX, -(ushort)( 4 + Calc_GetRandVal(6) ) );
		if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOANIM ) )
			UpdateAnimate( ANIM_MON_Stomp );

		_SetTimeout(3000);
		return 0;
	}

	if ( stage != SKTRIG_SUCCESS )
		return -SKTRIG_QTY;

	CPointMap pntMe = GetTopPoint();
	if ( pntMe.GetDist( m_Act_p ) > UO_MAP_VIEW_SIGHT )
		m_Act_p.StepLinePath( pntMe, UO_MAP_VIEW_SIGHT );

	SoundChar( CRESND_GETHIT );

	// a rock or a boulder ?
	ITEMID_TYPE id = ITEMID_NOTHING;
	int iDamage = 0;
	DAMAGE_TYPE iDmgType = DAMAGE_HIT_BLUNT|DAMAGE_THROWN;

	// AOS damage types (used by COMBAT_ELEMENTAL_ENGINE)
	int iDmgPhysical = 0, iDmgFire = 0, iDmgCold = 0, iDmgPoison = 0, iDmgEnergy = 0;

	CVarDefCont * pDam = GetDefKey("THROWDAM",true);
	
	if ( pDam )
	{
		int64 DVal[2];
		size_t iQty = Str_ParseCmds( const_cast<tchar *>(pDam->GetValStr()), DVal, CountOf(DVal));
		switch(iQty)
		{
			case 1:
				iDamage = (int)(DVal[0]);
				break;
			case 2:
				iDamage = (int)(DVal[0] + Calc_GetRandLLVal( DVal[1] - DVal[0] ));
				break;
		}
	}
	/*Set  the damage type if THROWDAMTYPE is set*/
	CVarDefCont  * pDamType = GetDefKey("THROWDAMTYPE", true);
	if (pDamType)
	{
		iDmgType = (DAMAGE_TYPE)pDamType->GetValNum();
		if (iDmgType & DAMAGE_FIRE)
			iDmgFire = 100;
		else if (iDmgType & DAMAGE_COLD)
			iDmgCold = 100;
		else if (iDmgType & DAMAGE_POISON)
			iDmgPoison = 100;
		else if (iDmgType & DAMAGE_ENERGY)
			iDmgEnergy = 100;
		else
			iDmgPhysical = 100;
		iDmgType |= DAMAGE_THROWN;
	}
	
	CVarDefCont * pRock = GetDefKey("THROWOBJ",true);
    if ( pRock )
	{
		lpctstr t_Str = pRock->GetValStr();
		CResourceID rid = static_cast<CResourceID>(g_Cfg.ResourceGetID( RES_ITEMDEF, t_Str ));
		id = (ITEMID_TYPE)(rid.GetResIndex());
		if (!iDamage)
			iDamage = Stat_GetVal(STAT_DEX)/4 + Calc_GetRandVal( Stat_GetVal(STAT_DEX)/4 );
	}
	else
	{
		if ( Calc_GetRandVal( 3 ) )
		{
			id = (ITEMID_TYPE)(ITEMID_ROCK_B_LO + Calc_GetRandVal(ITEMID_ROCK_B_HI-ITEMID_ROCK_B_LO));
			if (!iDamage)
				iDamage = Stat_GetVal(STAT_DEX)/4 + Calc_GetRandVal( Stat_GetVal(STAT_DEX)/4 );
		}
		else
		{
			id = (ITEMID_TYPE)(ITEMID_ROCK_2_LO + Calc_GetRandVal(ITEMID_ROCK_2_HI-ITEMID_ROCK_2_LO));
			if (!iDamage)
				iDamage = 2 + Calc_GetRandVal( Stat_GetVal(STAT_DEX)/4 );
		}
	}

	if ( id != ITEMID_NOTHING )
	{
		CItem *pItemRock = CItem::CreateScript(id, this);
		if ( pItemRock )
		{
			pItemRock->SetAttr(ATTR_DECAY);
			pItemRock->MoveToCheck(m_Act_p, this);
			pItemRock->Effect(EFFECT_BOLT, id, this);
		}
		if ( ! Calc_GetRandVal( pChar->GetTopPoint().GetDist( m_Act_p )))	// did it hit?
			pChar->OnTakeDamage( iDamage, this, iDmgType,iDmgPhysical,iDmgFire,iDmgCold,iDmgPoison,iDmgEnergy);
	}

	return 0;
}

int CChar::Skill_Act_Training( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Act_Training");
	// NPCACT_TRAINING
	// finished some traing maneuver.

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_START )
	{
		_SetTimeout(1);
		return 0;	// How difficult? 1-1000
	}
	if ( stage == SKTRIG_STROKE )
		return 0;
	if ( stage != SKTRIG_SUCCESS )
		return -SKTRIG_QTY;

	if ( m_Act_Prv_UID == m_uidWeapon )
	{
		CItem *pItem = m_Act_UID.ItemFind();
		if ( pItem )
		{
			switch ( pItem->GetType() )
			{
				case IT_TRAIN_DUMMY:
					Use_Train_Dummy(pItem, false);
					break;
				case IT_TRAIN_PICKPOCKET:
					Use_Train_PickPocketDip(pItem, false);
					break;
				case IT_ARCHERY_BUTTE:
					Use_Train_ArcheryButte(pItem, false);
					break;
				default:
					break;
			}
		}
	}
	return 0;
}

//************************************
// General skill stuff.
ANIM_TYPE CChar::Skill_GetAnim( SKILL_TYPE skill )
{
	switch ( skill )
	{
		/*case SKILL_FISHING:	// softcoded
			return ANIM_ATTACK_2H_BASH;*/
		case SKILL_BLACKSMITHING:
			return ANIM_ATTACK_1H_SLASH;
		case SKILL_MINING:
			return ANIM_ATTACK_1H_BASH;
		case SKILL_LUMBERJACKING:
			return ANIM_ATTACK_2H_SLASH;
		default:
			return static_cast<ANIM_TYPE>(-1);
	}
}

SOUND_TYPE CChar::Skill_GetSound( SKILL_TYPE skill )
{
	switch ( skill )
	{
		/*case SKILL_FISHING:	// softcoded
			return 0x364;*/
		case SKILL_ALCHEMY:
			return 0x242;
		case SKILL_TAILORING:
			return 0x248;
		case SKILL_CARTOGRAPHY:
		case SKILL_INSCRIPTION:
			return 0x249;
		case SKILL_BOWCRAFT:
			return 0x055;
		case SKILL_BLACKSMITHING:
			return 0x02a;
		case SKILL_CARPENTRY:
			return 0x23d;
		case SKILL_MINING:
			return Calc_GetRandVal(2) ? 0x125 : 0x126;
		case SKILL_LUMBERJACKING:
			return 0x13e;
		default:
			return SOUND_NONE;
	}
}

int CChar::Skill_Stroke( bool fResource )
{
	// fResource means decreasing m_atResource.m_dwStrokeCount instead of m_atCreate.m_dwStrokeCount
	SKILL_TYPE skill = Skill_GetActive();
	SOUND_TYPE sound = SOUND_NONE;
	int64 delay = Skill_GetTimeout();
	ANIM_TYPE anim = ANIM_WALK_UNARM;
	if ( m_atCreate.m_dwStrokeCount > 1 )
	{
		if ( !g_Cfg.IsSkillFlag(skill, SKF_NOSFX) )
			sound = Skill_GetSound(skill);
		if ( !g_Cfg.IsSkillFlag(skill, SKF_NOANIM) )
			anim = Skill_GetAnim(skill);
	}

	if ( IsTrigUsed(TRIGGER_SKILLSTROKE) || (IsTrigUsed(TRIGGER_STROKE)) )
	{
		CScriptTriggerArgs args;
		args.m_VarsLocal.SetNum("Skill", skill);
		args.m_VarsLocal.SetNum("Sound", sound);
		args.m_VarsLocal.SetNum("Delay", delay);
		args.m_VarsLocal.SetNum("Anim", anim);
		args.m_iN1 = 1;	//UpdateDir() ?
		if ( fResource )
			args.m_VarsLocal.SetNum("Strokes", m_atResource.m_dwStrokeCount);
		else
			args.m_VarsLocal.SetNum("Strokes", m_atCreate.m_dwStrokeCount);

		if ( OnTrigger(CTRIG_SkillStroke, this, &args) == TRIGRET_RET_TRUE )
			return -SKTRIG_ABORT;
		if ( Skill_OnTrigger(skill, SKTRIG_STROKE, &args) == TRIGRET_RET_TRUE )
			return -SKTRIG_ABORT;

		sound = (SOUND_TYPE)(args.m_VarsLocal.GetKeyNum("Sound"));
		delay = args.m_VarsLocal.GetKeyNum("Delay");
		anim = static_cast<ANIM_TYPE>(args.m_VarsLocal.GetKeyNum("Anim"));

		if ( args.m_iN1 == 1 )
			UpdateDir(m_Act_p);
		if ( fResource )
			m_atResource.m_dwStrokeCount = (word)(args.m_VarsLocal.GetKeyNum("Strokes"));
		else
			m_atCreate.m_dwStrokeCount = (word)(args.m_VarsLocal.GetKeyNum("Strokes"));
	}

	if ( sound )
		Sound(sound);
	if ( anim )
		UpdateAnimate(anim);	// keep trying and updating the animation

	if ( fResource )
	{
		if ( m_atResource.m_dwStrokeCount )
			--m_atResource.m_dwStrokeCount;
		if ( m_atResource.m_dwStrokeCount < 1 )
			return SKTRIG_SUCCESS;
	}
	else
	{
		if ( m_atCreate.m_dwStrokeCount )
			--m_atCreate.m_dwStrokeCount;
		if ( m_atCreate.m_dwStrokeCount < 1 )
			return SKTRIG_SUCCESS;
	}

	if ( delay < 10 )
		delay = 10;

	//Skill_SetTimeout();	// old behaviour, removed to keep up dynamic delay coming in with the trigger @SkillStroke
	SetTimeout(delay);
	return -SKTRIG_STROKE;	// keep active.
}

int CChar::Skill_Stage( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Stage");
	if ( stage == SKTRIG_STROKE )
	{
		if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_CRAFT) )
			return Skill_Stroke(false);

		if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_GATHER) )
			return Skill_Stroke(true);
	}

	if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_SCRIPTED) )
		return Skill_Scripted(stage);
	else if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_FIGHT) )
		return Skill_Fighting(stage);
	else if ( g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_MAGIC) )
		return Skill_Magery(stage);
	/*else if ( g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_CRAFT ) )
		return Skill_MakeItem(stage);*/
	else switch ( Skill_GetActive() )
	{
		case SKILL_NONE:	// idling.
		case SKILL_PARRYING:
		case SKILL_CAMPING:
		case SKILL_MAGICRESISTANCE:
		case SKILL_TACTICS:
		case SKILL_STEALTH:
			return 0;
		case SKILL_ALCHEMY:
		case SKILL_BOWCRAFT:
		case SKILL_TINKERING:
			return Skill_MakeItem(stage);
		case SKILL_ANATOMY:
		case SKILL_ANIMALLORE:
		case SKILL_ITEMID:
		case SKILL_ARMSLORE:
		case SKILL_EVALINT:
		case SKILL_FORENSICS:
		case SKILL_TASTEID:
			return Skill_Information(stage);
		case SKILL_BEGGING:
			return Skill_Begging(stage);
		case SKILL_BLACKSMITHING:
			return Skill_Blacksmith(stage);
		case SKILL_PEACEMAKING:
			return Skill_Peacemaking(stage);
		case SKILL_CARPENTRY:
			return Skill_Carpentry(stage);
		case SKILL_CARTOGRAPHY:
			return Skill_Cartography(stage);
		case SKILL_COOKING:
			return Skill_Cooking(stage);
		case SKILL_DETECTINGHIDDEN:
			return Skill_DetectHidden(stage);
		case SKILL_ENTICEMENT:
			return Skill_Enticement(stage);
		case SKILL_HEALING:
		case SKILL_VETERINARY:
			return Skill_Healing(stage);
		case SKILL_FISHING:
			return Skill_Fishing(stage);
		case SKILL_HERDING:
			return Skill_Herding(stage);
		case SKILL_HIDING:
			return Skill_Hiding(stage);
		case SKILL_PROVOCATION:
			return Skill_Provocation(stage);
		case SKILL_INSCRIPTION:
			return Skill_Inscription(stage);
		case SKILL_LOCKPICKING:
			return Skill_Lockpicking(stage);
		case SKILL_MAGERY:
		case SKILL_NECROMANCY:
		case SKILL_CHIVALRY:
		case SKILL_BUSHIDO:
		case SKILL_NINJITSU:
		case SKILL_SPELLWEAVING:
		case SKILL_MYSTICISM:
			return Skill_Magery(stage);
		case SKILL_SNOOPING:
			return Skill_Snooping(stage);
		case SKILL_MUSICIANSHIP:
			return Skill_Musicianship(stage);
		case SKILL_POISONING:
			return Skill_Poisoning(stage);
		case SKILL_ARCHERY:
		case SKILL_SWORDSMANSHIP:
		case SKILL_MACEFIGHTING:
		case SKILL_FENCING:
		case SKILL_WRESTLING:
		case SKILL_THROWING:
			return Skill_Fighting(stage);
		case SKILL_SPIRITSPEAK:
			return Skill_SpiritSpeak(stage);
		case SKILL_STEALING:
			return Skill_Stealing(stage);
		case SKILL_TAILORING:
			return Skill_Tailoring(stage);
		case SKILL_TAMING:
			return Skill_Taming(stage);
		case SKILL_TRACKING:
			return Skill_Tracking(stage);
		case SKILL_LUMBERJACKING:
			return Skill_Lumberjack(stage);
		case SKILL_MINING:
			return Skill_Mining(stage);
		case SKILL_MEDITATION:
			return Skill_Meditation(stage);
		case SKILL_REMOVETRAP:
			return Skill_RemoveTrap(stage);
		case NPCACT_BREATH:
			return Skill_Act_Breath(stage);
		case NPCACT_THROWING:
			return Skill_Act_Throwing(stage);
		case NPCACT_TRAINING:
			return Skill_Act_Training(stage);
		case NPCACT_NAPPING:
			return Skill_Act_Napping(stage);

		default:
			if ( !IsSkillBase(Skill_GetActive()) )
			{
				if ( stage == SKTRIG_STROKE )
					return -SKTRIG_STROKE;	// keep these active. (NPC modes)
				return 0;
			}
	}

	SysMessageDefault(DEFMSG_SKILL_NOSKILL);
	return -SKTRIG_QTY;
}

void CChar::Skill_Fail( bool fCancel )
{
	ADDTOCALLSTACK("CChar::Skill_Fail");
	// This is the normal skill check failure.
	// Other types of failure don't come here.
	//
	// ARGS:
	//	fCancel = no credit.
	//  else We still get some credit for having tried.

	SKILL_TYPE skill = Skill_GetActive();
	
	if ( skill == SKILL_NONE )
		return;

	if ( !IsSkillBase(skill) )
	{
		Skill_Cleanup();
		return;
	}

	if ( m_Act_Difficulty > 0 )
		m_Act_Difficulty = -m_Act_Difficulty;

	if ( !fCancel )
	{
		if ( IsTrigUsed(TRIGGER_SKILLFAIL) )
		{
			if ( Skill_OnCharTrigger(skill, CTRIG_SkillFail) == TRIGRET_RET_TRUE )
				fCancel = true;
		}
		if ( IsTrigUsed(TRIGGER_FAIL) && !fCancel )
		{
			if ( Skill_OnTrigger(skill, SKTRIG_FAIL) == TRIGRET_RET_TRUE )
				fCancel = true;
		}
	}
	else
	{
		if ( IsTrigUsed(TRIGGER_SKILLABORT) )
		{
			if ( Skill_OnCharTrigger(skill, CTRIG_SkillAbort) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return;
			}
		}
		if ( IsTrigUsed(TRIGGER_ABORT) )
		{
			if ( Skill_OnTrigger(skill, SKTRIG_ABORT) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return;
			}
		}
	}

	if ( Skill_Stage((fCancel)? SKTRIG_ABORT:SKTRIG_FAIL) >= 0 )
	{
		// Get some experience for failure ?
		if ( !fCancel )
			Skill_Experience(skill, m_Act_Difficulty);
	}

	Skill_Cleanup();
}

TRIGRET_TYPE CChar::Skill_OnTrigger( SKILL_TYPE skill, SKTRIG_TYPE stage )
{
	CScriptTriggerArgs pArgs;
	return Skill_OnTrigger(skill, stage, &pArgs);
}

TRIGRET_TYPE CChar::Skill_OnCharTrigger( SKILL_TYPE skill, CTRIG_TYPE stage )
{
	CScriptTriggerArgs pArgs;
	return Skill_OnCharTrigger(skill, stage, &pArgs);
}


TRIGRET_TYPE CChar::Skill_OnTrigger( SKILL_TYPE skill, SKTRIG_TYPE  stage, CScriptTriggerArgs *pArgs )
{
	ADDTOCALLSTACK("CChar::Skill_OnTrigger");
	if ( !IsSkillBase(skill) )
		return TRIGRET_RET_DEFAULT;

	if ( !(stage == SKTRIG_SELECT || stage == SKTRIG_GAIN || stage == SKTRIG_USEQUICK || stage == SKTRIG_WAIT || stage == SKTRIG_TARGETCANCEL) )
		m_Act_SkillCurrent = skill;

	pArgs->m_iN1 = skill;
	if ( g_Cfg.IsSkillFlag(skill, SKF_MAGIC) )
		pArgs->m_VarsLocal.SetNum("spell", m_atMagery.m_iSpell, true, false);

	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
	if ( pSkillDef && pSkillDef->HasTrigger(stage) )
	{
		// RES_SKILL
		CResourceLock s;
		if ( pSkillDef->ResourceLock(s) )
			iRet = CScriptObj::OnTriggerScript(s, CSkillDef::sm_szTrigName[stage], this, pArgs);
	}

	return iRet;
}

TRIGRET_TYPE CChar::Skill_OnCharTrigger( SKILL_TYPE skill, CTRIG_TYPE ctrig, CScriptTriggerArgs *pArgs )
{
	ADDTOCALLSTACK("CChar::Skill_OnCharTrigger");
	if ( !IsSkillBase(skill) )
		return TRIGRET_RET_DEFAULT;

	if ( !(ctrig == CTRIG_SkillSelect || ctrig == CTRIG_SkillGain || ctrig == CTRIG_SkillUseQuick || ctrig == CTRIG_SkillWait || ctrig == CTRIG_SkillTargetCancel) )
		m_Act_SkillCurrent = skill;

	pArgs->m_iN1 = skill;
	if ( g_Cfg.IsSkillFlag(skill, SKF_MAGIC) )
		pArgs->m_VarsLocal.SetNum("spell", m_atMagery.m_iSpell, true, false);

	return OnTrigger(ctrig, this, pArgs);
}


int CChar::Skill_Done()
{
	ADDTOCALLSTACK("CChar::Skill_Done");
	// We just finished using a skill. ASYNC timer expired.
	// m_Act_Skill = the skill.
	// Consume resources that have not already been consumed.
	// Confer the benefits of the skill.
	// calc skill gain based on this.
	//
	// RETURN: Did we succeed or fail ?
	//   0 = success
	//	 -SKTRIG_STROKE = stay in skill. (stroke)
	//   -SKTRIG_FAIL = we must print the fail msg. (credit for trying)
	//   -SKTRIG_ABORT = we must print the fail msg. (But get no credit, cancelled )
	//   -SKTRIG_QTY = special failure. clean up the skill but say nothing. (no credit)

	SKILL_TYPE skill = Skill_GetActive();
	if ( skill == SKILL_NONE )	// we should not be coming here (timer should not have expired)
	{
		Skill_Cleanup();
		return -SKTRIG_QTY;
	}

	// multi stroke tried stuff here first.
	// or stuff that never really fails.
	int iRet = Skill_Stage(SKTRIG_STROKE);
	if ( iRet < 0 )
		return iRet;

	if ( m_Act_Difficulty < 0 )		// was Bound to fail, but we had to wait for the timer anyhow.
		return -SKTRIG_FAIL;

	if ( IsTrigUsed(TRIGGER_SKILLSUCCESS) )
	{
		if ( Skill_OnCharTrigger(skill, CTRIG_SkillSuccess) == TRIGRET_RET_TRUE )
			return -SKTRIG_ABORT;
	}
	if ( IsTrigUsed(TRIGGER_SUCCESS) )
	{
		if ( Skill_OnTrigger(skill, SKTRIG_SUCCESS) == TRIGRET_RET_TRUE )
			return -SKTRIG_ABORT;
	}

	// Success for the skill.
	iRet = Skill_Stage(SKTRIG_SUCCESS);
	if ( iRet < 0 )
		return iRet;

	// Success = Advance the skill
	Skill_Experience(skill, m_Act_Difficulty);

	Skill_Cleanup();
	return -SKTRIG_SUCCESS;
}

bool CChar::Skill_Wait( SKILL_TYPE skilltry )
{
	ADDTOCALLSTACK("CChar::Skill_Wait");
	// Some sort of push button skill.
	// We want to do some new skill. Can we ?
	// If this is the same skill then tell them to wait.

	SKILL_TYPE skill = Skill_GetActive();
	CScriptTriggerArgs pArgs(skilltry, skill);

	if ( IsTrigUsed(TRIGGER_SKILLWAIT) )
	{
		switch ( Skill_OnCharTrigger(skilltry, CTRIG_SkillWait, &pArgs) )
		{
			case TRIGRET_RET_TRUE:
				return true;
			case TRIGRET_RET_FALSE:
				Skill_Fail(true);
				return false;
			default:
				break;
		}
	}
	if ( IsTrigUsed(TRIGGER_WAIT) )
	{
		switch ( Skill_OnTrigger(skilltry, SKTRIG_WAIT, &pArgs) )
		{
			case TRIGRET_RET_TRUE:
				return true;
			case TRIGRET_RET_FALSE:
				Skill_Fail(true);
				return false;
			default:
				break;
		}
	}

	if ( IsStatFlag(STATF_DEAD|STATF_SLEEPING|STATF_FREEZE|STATF_STONE) || Can(CAN_C_STATUE) )
	{
		SysMessageDefault(DEFMSG_SKILLWAIT_1);
		return true;
	}

	if ( skill == SKILL_NONE )	// not currently doing anything.
	{
		if ((skilltry == SKILL_STEALTH) || ((skilltry == SKILL_SNOOPING) && (g_Cfg.m_iRevealFlags & REVEALF_SNOOPING)) || ((skilltry == SKILL_STEALING) && (g_Cfg.m_iRevealFlags & REVEALF_STEALING)))
			return false;
		else
			Reveal();
		return false;
	}

	if ( IsStatFlag(STATF_WAR) )
	{
		SysMessageDefault(DEFMSG_SKILLWAIT_2);
		return true;
	}

	// Cancel passive actions
	if ( skilltry != skill )
	{
		if ( skill == SKILL_MEDITATION || skill == SKILL_HIDING || skill == SKILL_STEALTH )		// SKILL_SPIRITSPEAK ?
		{
			Skill_Fail(true);
			return false;
		}
	}

	SysMessageDefault(DEFMSG_SKILLWAIT_3);
	return true;
}

// Assume the container is not locked.
// return: true = snoop or can't open at all.
//  false = instant open.
bool CChar::Skill_Snoop_Check(const CItemContainer * pItem)
{
	ADDTOCALLSTACK("CChar::Skill_Snoop_Check");

	if (pItem == nullptr)
		return true;

	if (pItem->IsContainer())
	{
		if ((g_Cfg.m_iTradeWindowSnooping == false) && (pItem->IsItemInTrade() == true))
			return false;
	}

	if (!IsPriv(PRIV_GM))
	{
		switch (pItem->GetType())
		{
		case IT_SHIP_HOLD_LOCK:
		case IT_SHIP_HOLD:
			// Must be on board a ship to open the hatch.
			ASSERT(m_pArea);
			if (m_pArea->GetResourceID() != pItem->m_uidLink)
			{
				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_HATCH_FAIL));
				return true;
			}
			break;
		case IT_EQ_BANK_BOX:
			// Some sort of cheater.
			return false;
		default:
			break;
		}
	}

	CChar * pCharMark = nullptr;
	if (!IsTakeCrime(pItem, &pCharMark) || pCharMark == nullptr)
		return false;

	if (Skill_Wait(SKILL_SNOOPING))
		return true;

	m_Act_UID = pItem->GetUID();
	Skill_Start(SKILL_SNOOPING);
	return true;
}

// SKILL_SNOOPING
// m_Act_UID = object to snoop into.
// RETURN:
// -SKTRIG_QTY = no chance. and not a crime
// -SKTRIG_FAIL = no chance and caught.
// 0-100 = difficulty = percent chance of failure.
int CChar::Skill_Snooping(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Snooping");

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if ( stage == SKTRIG_STROKE )
		return 0;

	// Assume the container is not locked.
	CItemContainer * pCont = dynamic_cast <CItemContainer *>(m_Act_UID.ItemFind());
	if (pCont == nullptr)
		return (-SKTRIG_QTY);

	CChar * pCharMark;
	if (!IsTakeCrime(pCont, &pCharMark) || pCharMark == nullptr)
		return 0;	// Not a crime really.

	if (GetTopDist3D(pCharMark) > 1)
	{
		SysMessageDefault(DEFMSG_SNOOPING_REACH);
		return (-SKTRIG_QTY);
	}

	if (!CanTouch(pCont))
	{
		SysMessageDefault(DEFMSG_SNOOPING_CANT);
		return (-SKTRIG_QTY);
	}

	if (stage == SKTRIG_START)
	{
		PLEVEL_TYPE plevel = GetPrivLevel();
		if (plevel < pCharMark->GetPrivLevel())
		{
			SysMessageDefault(DEFMSG_SNOOPING_CANT);
			return -SKTRIG_QTY;
		}

		// return the difficulty.
		return ((Skill_GetAdjusted(SKILL_SNOOPING) < Calc_GetRandVal(1000)) ? 100 : 0);
	}

	// did anyone see this ?
	CheckCrimeSeen(SKILL_SNOOPING, pCharMark, pCont, g_Cfg.GetDefaultMsg(DEFMSG_SNOOPING_ATTEMPTING));
	Noto_Karma(-4, INT32_MIN, true);

	if (stage == SKTRIG_FAIL)
	{
		SysMessageDefault(DEFMSG_SNOOPING_FAILED);
		if ((Skill_GetAdjusted(SKILL_HIDING) / 2) < Calc_GetRandVal(1000))
			Reveal();
	}

	if (stage == SKTRIG_SUCCESS)
	{
		if (IsClientActive())
			m_pClient->addContainerSetup(pCont);	// open the container
	}
	return 0;
}

// m_Act_UID = object to steal.
// RETURN:
// -SKTRIG_QTY = no chance. and not a crime
// -SKTRIG_FAIL = no chance and caught.
// 0-100 = difficulty = percent chance of failure.
int CChar::Skill_Stealing(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Stealing");

	if ( stage == SKTRIG_ABORT )
		return -SKTRIG_ABORT;

	if (stage == SKTRIG_STROKE )
		return 0;

	CItem * pItem = m_Act_UID.ItemFind();
	CChar * pCharMark = nullptr;
	if (pItem == nullptr)	// on a chars head ? = random steal.
	{
		pCharMark = m_Act_UID.CharFind();
		if (pCharMark == nullptr)
		{
			SysMessageDefault(DEFMSG_STEALING_NOTHING);
			return (-SKTRIG_QTY);
		}
		CItemContainer * pPack = pCharMark->GetPack();
		if (pPack == nullptr)
		{
		cantsteal:
			SysMessageDefault(DEFMSG_STEALING_EMPTY);
			return (-SKTRIG_QTY);
		}
		pItem = pPack->ContentFindRandom();
		if (pItem == nullptr)
		{
			goto cantsteal;
		}
		m_Act_UID = pItem->GetUID();
	}

	// Special cases.
	CContainer * pContainer = dynamic_cast <CContainer *> (pItem->GetContainer());
	if (pContainer)
	{
		CItemCorpse * pCorpse = dynamic_cast <CItemCorpse *> (pContainer);
		if (pCorpse)
		{
			SysMessageDefault(DEFMSG_STEALING_CORPSE);
			return -SKTRIG_ABORT;
		}
	}
	CItem * pCItem = dynamic_cast <CItem *> (pItem->GetContainer());
	if (pCItem)
	{
		if (pCItem->GetType() == IT_GAME_BOARD)
		{
			SysMessageDefault(DEFMSG_STEALING_GAMEBOARD);
			return -SKTRIG_ABORT;
		}
		if (pCItem->GetType() == IT_EQ_TRADE_WINDOW)
		{
			SysMessageDefault(DEFMSG_STEALING_TRADE);
			return -SKTRIG_ABORT;
		}
	}
	CItemCorpse * pCorpse = dynamic_cast <CItemCorpse *> (pItem);
	if (pCorpse)
	{
		SysMessageDefault(DEFMSG_STEALING_CORPSE);
		return -SKTRIG_ABORT;
	}
	if (pItem->IsType(IT_TRAIN_PICKPOCKET))
	{
		SysMessageDefault(DEFMSG_STEALING_PICKPOCKET);
		return -SKTRIG_QTY;
	}
	if (pItem->IsType(IT_GAME_PIECE))
	{
		return -SKTRIG_QTY;
	}
	if (!CanTouch(pItem))
	{
		SysMessageDefault(DEFMSG_STEALING_REACH);
		return -SKTRIG_ABORT;
	}
	if (!CanMove(pItem) || !CanCarry(pItem))
	{
		SysMessageDefault(DEFMSG_STEALING_HEAVY);
		return -SKTRIG_ABORT;
	}
	if (!IsTakeCrime(pItem, &pCharMark))
	{
		SysMessageDefault(DEFMSG_STEALING_NONEED);

		// Just pick it up ?
		return -SKTRIG_QTY;
	}
	if (m_pArea->IsFlag(REGION_FLAG_SAFE))
	{
		SysMessageDefault(DEFMSG_STEALING_STOP);
		return -SKTRIG_QTY;
	}

	//Reveal();	// If we take an item off the ground we are revealed.

	bool fGround = false;
	if (pCharMark != nullptr)
	{
		if (GetTopDist3D(pCharMark) > 2)
		{
			SysMessageDefault(DEFMSG_STEALING_MARK);
			return -SKTRIG_QTY;
		}
		if (m_pArea->IsFlag(REGION_FLAG_NO_PVP) && pCharMark->m_pPlayer && !IsPriv(PRIV_GM))
		{
			SysMessageDefault(DEFMSG_STEALING_SAFE);
			return -1;
		}
		if (GetPrivLevel() < pCharMark->GetPrivLevel())
			return -SKTRIG_FAIL;
		if (stage == SKTRIG_START)
			return g_Cfg.Calc_StealingItem(this, pItem, pCharMark);
	}
	else
	{
		// stealing off the ground should always succeed.
		// it's just a matter of getting caught.
		if (stage == SKTRIG_START)
			return 1;	// town stuff on the ground is too easy.

		fGround = true;
	}

	// Deliver the goods.

	if ((stage == SKTRIG_SUCCESS) || fGround)
	{
		pItem->ClrAttr(ATTR_OWNED);	// Now it's mine
		CItemContainer * pPack = GetPack();
		if (pItem->GetParent() != pPack && pPack)
		{
			pItem->RemoveFromView();
			// Put in my invent.
			pPack->ContentAdd(pItem);
		}
	}
	
	if ((stage == SKTRIG_SUCCESS) && (g_Cfg.m_iRevealFlags & REVEALF_STEALING_SUCCESS))
		Reveal();
	else if ((stage == SKTRIG_FAIL) && (g_Cfg.m_iRevealFlags & REVEALF_STEALING_FAIL))
		Reveal();

	if (m_Act_Difficulty == 0)
		return 0;	// Too easy to be bad. hehe

					// You should only be able to go down to -1000 karma by stealing.
	if (CheckCrimeSeen(SKILL_STEALING, pCharMark, pItem, (stage == SKTRIG_FAIL) ? g_Cfg.GetDefaultMsg(DEFMSG_STEALING_YOUR) : g_Cfg.GetDefaultMsg(DEFMSG_STEALING_SOMEONE)))
		Noto_Karma(-100, -1000, true);

	return 0;
}

/*
The Focus skill is used passively and it works automatically only if FEATURES_AOS_UPDATE_B is enabled.
The skill increase the amount of stamina gained by 1 for each 10% points of Focus and increase the amount
of mana by 1 for each 20%  points of Focus.
The Skill_Focus method is called from Stats_Regen64 method  found in CCharStat.cpp
*/
int CChar::Skill_Focus(STAT_TYPE stat)
{
	ADDTOCALLSTACK("CChar::Skill_Focus");

	if (g_Cfg.IsSkillFlag(SKILL_FOCUS, SKF_SCRIPTED))
		return -SKTRIG_QTY;

	ushort iFocusValue = Skill_GetAdjusted(SKILL_FOCUS);

	//By giving the character skill focus value as difficulty, the chance to succeed is always around 50%
	if (Skill_UseQuick(SKILL_FOCUS, iFocusValue/10)) 
	{
		ushort uiGain = 0;
		switch (stat)
		{
		case STAT_DEX:
			uiGain = iFocusValue / 100;
			break;
		case STAT_INT:
			uiGain = iFocusValue / 200;
			break;
		default:
			return -SKTRIG_QTY;
		}
		return uiGain;
	}
	return -SKTRIG_QTY;
	
}
bool CChar::Skill_Start( SKILL_TYPE skill, int iDifficultyIncrease )
{
	ADDTOCALLSTACK("CChar::Skill_Start");
	// We have all the info we need to do the skill. (targeting etc)
	// Set up how long we have to wait before we get the desired results from this skill.
	// Set up any animations/sounds in the mean time.
	// Calc if we will succeed or fail.
	// RETURN:
	//  false = failed outright with no wait. "You have no chance of taming this"

	if ( g_Serv.IsLoading() )
	{
		if ( skill != SKILL_NONE && !IsSkillBase(skill) && !IsSkillNPC(skill) )
		{
			DEBUG_ERR(("UID:0%x Bad Skill %d for '%s'\n", (dword)GetUID(), skill, GetName()));
			return false;
		}
		m_Act_SkillCurrent = skill;
		return true;
	}

	if ( !Skill_CanUse(skill) )
		return false;

	SKILL_TYPE skActive = Skill_GetActive();
	if (skActive != SKILL_NONE )
		Skill_Fail(true);		// fail previous skill unfinished. (with NO skill gain!)

	if ( skill != SKILL_NONE )
	{
		// Do the cleanup only at the end the skill, not at the start: we may want to set some parameters before the skill call
		// (like ACTARG1/2 for magery, they are respectively m_atMagery.m_iSpell	and m_atMagery.m_iSummonID and are set in
		// CClient::OnTarg_Skill_Magery, which then calls Skill_Start).
		// Skill_Cleanup();
		CScriptTriggerArgs pArgs;
		pArgs.m_iN1 = skill;

		// Some skill can start right away. Need no targetting.
		// 0-100 scale of Difficulty
		if ( IsTrigUsed(TRIGGER_SKILLPRESTART) )
		{
			if ( Skill_OnCharTrigger(skill, CTRIG_SkillPreStart) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return false;
			}
		}
		if ( IsTrigUsed(TRIGGER_PRESTART) )
		{
			if ( Skill_OnTrigger(skill, SKTRIG_PRESTART) == TRIGRET_RET_TRUE )
			{
				Skill_Cleanup();
				return false;
			}
		}

		m_Act_SkillCurrent = skill;
		m_Act_Difficulty = Skill_Stage(SKTRIG_START);

		if (m_Act_Difficulty >= 0)	// If m_Act_Difficulty == -1 then the skill stage has failed, so preserve this result for later.
			m_Act_Difficulty += iDifficultyIncrease;

		// Execute the @START trigger and pass various craft parameters there
		const bool fCraftSkill = g_Cfg.IsSkillFlag(skill, SKF_CRAFT);
		const bool fGatherSkill = g_Cfg.IsSkillFlag(skill, SKF_GATHER);
		CResourceID pResBase(RES_ITEMDEF, fCraftSkill ? m_atCreate.m_iItemID : 0, 0);

		if ( fCraftSkill )
		{
			m_atCreate.m_dwStrokeCount = 1;		// This matches the new strokes amount used on OSI.
			pArgs.m_VarsLocal.SetNum("CraftItemdef", pResBase.GetPrivateUID());
            // pArgs.m_VarsLocal.SetStr("CraftItemdef", g_Cfg.ResourceGetName(pResBase), false);
			pArgs.m_VarsLocal.SetNum("CraftStrokeCnt", m_atCreate.m_dwStrokeCount);
			pArgs.m_VarsLocal.SetNum("CraftAmount", m_atCreate.m_dwAmount);
		}
		if ( fGatherSkill )
		{
			m_atResource.m_dwBounceItem = 1;
			pArgs.m_VarsLocal.SetNum("GatherStrokeCnt", m_atResource.m_dwStrokeCount);
		}


		if ( IsTrigUsed(TRIGGER_SKILLSTART) )
		{
			//If we are using a combat skill and m_Act_Difficult(actdiff) is < 0 combat will be blocked.
			if ( (Skill_OnCharTrigger(skill, CTRIG_SkillStart, &pArgs) == TRIGRET_RET_TRUE) || (m_Act_Difficulty < 0) )
			{
				Skill_Cleanup();
				return false;
			}
		}

		if ( IsTrigUsed(TRIGGER_START) )
		{
			//If we are using a combat skill and m_Act_Difficulty(actdiff) is < 0 combat will be blocked.
			if ( (Skill_OnTrigger(skill, SKTRIG_START, &pArgs) == TRIGRET_RET_TRUE) || (m_Act_Difficulty < 0) )
			{
				Skill_Cleanup();
				return false;
			}
		}

		if ( fCraftSkill )
		{
			// read crafting parameters
			pResBase = CResourceID( (dword)(pArgs.m_VarsLocal.GetKeyNum("CraftItemdef")), 0 );
			m_atCreate.m_dwStrokeCount = (word)pArgs.m_VarsLocal.GetKeyNum("CraftStrokeCnt");
			m_atCreate.m_dwStrokeCount = maximum(1,m_atCreate.m_dwStrokeCount);
			m_atCreate.m_iItemID = (ITEMID_TYPE)(pResBase.GetResIndex());
			m_atCreate.m_dwAmount = (word)(pArgs.m_VarsLocal.GetKeyNum("CraftAmount"));
		}
		if ( fGatherSkill )
			m_atResource.m_dwStrokeCount = (word)(pArgs.m_VarsLocal.GetKeyNum("GatherStrokeCnt"));

		// Casting sound & animation when starting, Skill_Stroke() will do it the next times.
		if ( fCraftSkill || fGatherSkill )
		{
			skActive = Skill_GetActive();

			if ( !g_Cfg.IsSkillFlag(skActive, SKF_NOSFX) )
				Sound(Skill_GetSound(skActive));

			if ( !g_Cfg.IsSkillFlag(skActive, SKF_NOANIM) )
				UpdateAnimate(Skill_GetAnim(skActive));
		}

		if ( IsSkillBase(skill) )
		{
			const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(skill);
			if ( pSkillDef )
			{
				int iWaitTime = pSkillDef->m_vcDelay.GetLinear(Skill_GetBase(skill));
                if (iWaitTime > 0)
                {
                    SetTimeoutD(iWaitTime);		// How long before complete skill.
                }
			}
		}

        if (_IsTimerExpired())
        {
            _SetTimeoutD(1);		// the skill should have set it's own delay!?
        }
		
		//When combat starts, the first @HitTry trigger will be called after the @SkillStart/@Start (as it was before).
		const bool fFightSkill = g_Cfg.IsSkillFlag(skill, SKF_FIGHT);
		if ( fFightSkill )
		{
			m_atFight.m_iWarSwingState = WAR_SWING_EQUIPPING;
			Fight_HitTry();	// this cleans up itself, executes the code related to the current m_iWarSwingState and sets the needed timers.
		}

		if ( m_Act_Difficulty > 0 )
		{
			if ( !Skill_CheckSuccess(skill, m_Act_Difficulty, !fFightSkill) )
				m_Act_Difficulty = -m_Act_Difficulty;	// will result in failure
		}
	}

	// emote the action i am taking.
	if ( (g_Cfg.m_iDebugFlags & DEBUGF_NPC_EMOTE) || IsStatFlag(STATF_EMOTEACTION) )
		Emote(Skill_GetName(true));

	return true;
}
