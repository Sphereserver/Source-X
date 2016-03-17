//  CChar is either an NPC or a Player.

#include "../common/CException.h"
#include "CChar.h"
#include "Triggers.h"

//----------------------------------------------------------------------

void CChar::Stat_AddMod( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_AddMod");
	ASSERT(i >= 0 && i < STAT_QTY);
	m_Stat[i].m_mod	+= iVal;

	short iMaxValue = Stat_GetMax(i);		// make sure the current value is not higher than new max value
	if ( m_Stat[i].m_val > iMaxValue )
		m_Stat[i].m_val = iMaxValue;

	UpdateStatsFlag();
}

void CChar::Stat_SetMod( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetMod");
	ASSERT(i >= 0 && i < STAT_QTY);
	short iStatVal = Stat_GetMod(i);
	if ( IsTrigUsed(TRIGGER_STATCHANGE) && !IsTriggerActive("CREATE") )
	{
		if ( i >= STAT_STR && i <= STAT_DEX )
		{
			CScriptTriggerArgs args;
			args.m_iN1 = i+8;	// shift by 8 to indicate modSTR, modINT, modDEX
			args.m_iN2 = iStatVal;
			args.m_iN3 = iVal;
			if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
				return;
			// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
			iVal = static_cast<short>(args.m_iN3);
		}
	}
	m_Stat[i].m_mod = iVal;

	short iMaxValue = Stat_GetMax(i);		// make sure the current value is not higher than new max value
	if ( m_Stat[i].m_val > iMaxValue )
		m_Stat[i].m_val = iMaxValue;

	UpdateStatsFlag();
}

short CChar::Stat_GetMod( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetMod");
	ASSERT(i >= 0 && i < STAT_QTY);
	return m_Stat[i].m_mod;
}

void CChar::Stat_SetVal( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetVal");
	if (i > STAT_BASE_QTY || i == STAT_FOOD) // Food must trigger Statchange. Redirect to Base value
	{
		Stat_SetBase(i, iVal);
		return;
	}
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food
	m_Stat[i].m_val = iVal;
}

short CChar::Stat_GetVal( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetVal");
	if ( i > STAT_BASE_QTY || i == STAT_FOOD ) // Food must trigger Statchange. Redirect to Base value
		return Stat_GetBase(i);
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food
	return m_Stat[i].m_val;
}


void CChar::Stat_SetMax( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetMax");
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food

	if ( g_Cfg.m_iStatFlag && ((g_Cfg.m_iStatFlag & STAT_FLAG_DENYMAX) || (m_pPlayer && g_Cfg.m_iStatFlag & STAT_FLAG_DENYMAXP) || (m_pNPC && g_Cfg.m_iStatFlag & STAT_FLAG_DENYMAXN)) )
		m_Stat[i].m_max = 0;
	else
	{
		int iStatVal = Stat_GetMax(i);
		if ( IsTrigUsed(TRIGGER_STATCHANGE) && !IsTriggerActive("CREATE") )
		{
			if ( i >= STAT_STR && i <= STAT_FOOD )		// only STR, DEX, INT, FOOD fire MaxHits, MaxMana, MaxStam, MaxFood for @StatChange
			{
				CScriptTriggerArgs args;
				args.m_iN1 = i + 4;		// shift by 4 to indicate MaxHits, etc..
				args.m_iN2 = iStatVal;
				args.m_iN3 = iVal;
				if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
					return;
				// do not restore argn1 to i, bad things will happen! leave it untouched. (matex)
				iVal = static_cast<short>(args.m_iN3);
			}
		}
		m_Stat[i].m_max = iVal;

		short iMaxValue = Stat_GetMax(i);		// make sure the current value is not higher than new max value
		if ( m_Stat[i].m_val > iMaxValue )
			m_Stat[i].m_val = iMaxValue;

		if ( i == STAT_STR )
			UpdateHitsFlag();
		else if ( i == STAT_INT )
			UpdateManaFlag();
		else if ( i == STAT_DEX )
			UpdateStamFlag();
	}
}

short CChar::Stat_GetMax( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetMax");
	short val;
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food
	if ( m_Stat[i].m_max < 1 )
	{
		if ( i == STAT_FOOD )
		{
			CCharBase * pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			val = pCharDef->m_MaxFood;
		}
		else
			val	= Stat_GetAdjusted(i);

		if ( i == STAT_INT )
		{
			if ( (g_Cfg.m_iRacialFlags & RACIALF_ELF_WISDOM) && IsElf() )
				val += 20;		// elves always have +20 max mana (Wisdom racial trait)
		}
		return (val < 0 ? (m_pPlayer ? 1 : 0) : val);
	}
	val	= m_Stat[i].m_max;
	if ( i >= STAT_BASE_QTY )
		val += m_Stat[i].m_mod;
	return (val < 0 ? (m_pPlayer ? 1 : 0) : val);
}

short CChar::Stat_GetSum() const
{
	ADDTOCALLSTACK("CChar::Stat_GetSum");
	short iStatSum = 0;
	for ( int i = 0; i < STAT_BASE_QTY; i++ )
		iStatSum += Stat_GetBase(static_cast<STAT_TYPE>(i));

	return iStatSum;
}

short CChar::Stat_GetAdjusted( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetAdjusted");
	short val = Stat_GetBase(i) + Stat_GetMod(i);
	if ( i == STAT_KARMA )
		val = static_cast<short>(maximum(g_Cfg.m_iMinKarma, minimum(g_Cfg.m_iMaxKarma, val)));
	else if ( i == STAT_FAME )
		val = static_cast<short>(maximum(0, minimum(g_Cfg.m_iMaxFame, val)));

	return val;
}

short CChar::Stat_GetBase( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetBase");
	ASSERT(i >= 0 && i < STAT_QTY);

	if ( i == STAT_FAME && m_Stat[i].m_base < 0 )
		return 0;
	return m_Stat[i].m_base;
}

void CChar::Stat_AddBase( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_AddBase");
	Stat_SetBase( i, Stat_GetBase( i ) + iVal );
}

void CChar::Stat_SetBase( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetBase");
	ASSERT(i >= 0 && i < STAT_QTY);

	int iStatVal = Stat_GetBase(i);
	if (IsTrigUsed(TRIGGER_STATCHANGE) && !g_Serv.IsLoading() && !IsTriggerActive("CREATE"))
	{
		// Only Str, Dex, Int, Food fire @StatChange here
		if (i >= STAT_STR && i <= STAT_FOOD)
		{
			CScriptTriggerArgs args;
			args.m_iN1 = i;
			args.m_iN2 = iStatVal;
			args.m_iN3 = iVal;
			if (OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE)
				return;
			// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
			iVal = static_cast<short>(args.m_iN3);

			if (i != STAT_FOOD && m_Stat[i].m_max < 1) // MaxFood cannot depend on something, otherwise if the Stat depends on STR, INT, DEX, fire MaxHits, MaxMana, MaxStam
			{
				args.m_iN1 = i+4; // Shift by 4 to indicate MaxHits, MaxMana, MaxStam
				args.m_iN2 = iStatVal;
				args.m_iN3 = iVal;
				if (OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE)
					return;
				// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
				iVal = static_cast<short>(args.m_iN3);
			}
		}
	}
	switch ( i )
	{
		case STAT_STR:
			{
				CCharBase * pCharDef = Char_GetDef();
				if ( pCharDef && !pCharDef->m_Str )
					pCharDef->m_Str = iVal;
			}
			break;
		case STAT_INT:
			{
				CCharBase * pCharDef = Char_GetDef();
				if ( pCharDef && !pCharDef->m_Int )
					pCharDef->m_Int = iVal;
			}
			break;
		case STAT_DEX:
			{
				CCharBase * pCharDef = Char_GetDef();
				if ( pCharDef && !pCharDef->m_Dex )
					pCharDef->m_Dex = iVal;
			}
			break;
		case STAT_FOOD:
			break;
		case STAT_KARMA:
			iVal = static_cast<short>(maximum(g_Cfg.m_iMinKarma, minimum(g_Cfg.m_iMaxKarma, iVal)));
			break;
		case STAT_FAME:
			iVal = static_cast<short>(maximum(0, minimum(g_Cfg.m_iMaxFame, iVal)));
			break;
		default:
			throw CGrayError(LOGL_CRIT, 0, "Stat_SetBase: index out of range");
	}
	
	m_Stat[i].m_base = iVal;

	short iMaxValue = Stat_GetMax(i);		// make sure the current value is not higher than new max value
	if ( m_Stat[i].m_val > iMaxValue )
		m_Stat[i].m_val = iMaxValue;

	UpdateStatsFlag();
	if ( !g_Serv.IsLoading() && i == STAT_KARMA )
		NotoSave_Update();
}

short CChar::Stat_GetLimit( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetLimit");
	const CVarDefCont * pTagStorage = NULL;
	TemporaryString sStatName;

	if ( m_pPlayer )
	{
		const CSkillClassDef * pSkillClass = m_pPlayer->GetSkillClass();
		ASSERT(pSkillClass);
		if ( i == STAT_QTY )
		{
			pTagStorage = GetKey("OVERRIDE.STATSUM", true);
			return pTagStorage ? (static_cast<short>(pTagStorage->GetValNum())) : static_cast<short>(pSkillClass->m_StatSumMax);
		}
		ASSERT( i >= 0 && i < STAT_BASE_QTY );

		sprintf(sStatName, "OVERRIDE.STATCAP_%d", static_cast<int>(i));
		short iStatMax;
		if ( (pTagStorage = GetKey(sStatName, true)) != NULL )
			iStatMax = static_cast<short>(pTagStorage->GetValNum());
		else
			iStatMax = pSkillClass->m_StatMax[i];

		if ( m_pPlayer->Stat_GetLock(i) >= SKILLLOCK_DOWN )
		{
			short iStatLevel = Stat_GetBase(i);
			if ( iStatLevel < iStatMax )
				iStatMax = iStatLevel;
		}
		return( iStatMax );
	}
	else
	{
		if ( i == STAT_QTY )
		{
			pTagStorage = GetKey("OVERRIDE.STATSUM", true);
			return pTagStorage ? static_cast<short>(pTagStorage->GetValNum()) : 300;
		}

		short iStatMax = 100;
		sprintf(sStatName, "OVERRIDE.STATCAP_%d", static_cast<int>(i));
		if ( (pTagStorage = GetKey(sStatName, true)) != NULL )
			iStatMax = static_cast<short>(pTagStorage->GetValNum());

		return( iStatMax );
	}
}

bool CChar::Stats_Regen(INT64 iTimeDiff)
{
	ADDTOCALLSTACK("CChar::Stats_Regen");
	// Calling regens in all stats and checking REGEN%s/REGEN%sVAL where %s is hits/stam... to check values/delays
	// Food decay called here too.
	// calling @RegenStat for each stat if proceed.
	// iTimeDiff is the next tick the stats are going to regen.

	for (STAT_TYPE i = STAT_STR; i <= STAT_FOOD; i = static_cast<STAT_TYPE>(i + 1))
	{
		if (g_Cfg.m_iRegenRate[i] < 0)
			continue;
		int iRate = Stats_GetRegenVal(i, true);
		if (iRate < 0)
			iRate = 0;

		short mod = Stats_GetRegenVal(i,false);
		if ((i == STAT_STR) && (g_Cfg.m_iRacialFlags & RACIALF_HUMAN_TOUGH) && IsHuman())
			mod += 2;		// Humans always have +2 hitpoint regeneration (Tough racial trait)

		m_Stat[i].m_regen += static_cast<unsigned short>(iTimeDiff);
		if (m_Stat[i].m_regen < iRate)
			continue;

		short StatLimit = Stat_GetMax(i);
		int HitsHungerLoss = g_Cfg.m_iHitsHungerLoss ? g_Cfg.m_iHitsHungerLoss : 0;

		if (IsTrigUsed(TRIGGER_REGENSTAT))
		{
			CScriptTriggerArgs Args;
			Args.m_VarsLocal.SetNum("StatID", i, true);
			Args.m_VarsLocal.SetNum("Value", mod, true);
			Args.m_VarsLocal.SetNum("StatLimit", StatLimit, true);
			if (i == STAT_FOOD)
				Args.m_VarsLocal.SetNum("HitsHungerLoss", HitsHungerLoss);

			if (OnTrigger(CTRIG_RegenStat, this, &Args) == TRIGRET_RET_TRUE)
			{
				m_Stat[i].m_regen = 0;
				continue;
			}

			i = static_cast<STAT_TYPE>(Args.m_VarsLocal.GetKeyNum("StatID", true));
			if (i < STAT_STR)
				i = STAT_STR;
			else if (i > STAT_FOOD)
				i = STAT_FOOD;
			mod = static_cast<short>(Args.m_VarsLocal.GetKeyNum("Value", true));
			StatLimit = static_cast<unsigned short>(Args.m_VarsLocal.GetKeyNum("StatLimit", true));
			if (i == STAT_FOOD)
				HitsHungerLoss = static_cast<int>(Args.m_VarsLocal.GetKeyNum("HitsHungerLoss", true));
		}
		if (mod == 0)
			continue;

		m_Stat[i].m_regen = 0;

		if (i == STAT_FOOD)
			OnTickFood(mod, HitsHungerLoss);
		else
			UpdateStatVal(i, mod, StatLimit);
	}
	return true;
}
unsigned short CChar::Stats_GetRegenVal(STAT_TYPE iStat, bool bGetTicks)
{
	ADDTOCALLSTACK("CChar::Stats_GetRegenVal");
	// Return regen rates and regen val for the given stat.
	// bGetTicks = true returns the regen ticks
	// bGetTicks = false returns the values of regeneration.

	LPCTSTR stat = "";
	switch ( iStat )
	{
		case STAT_STR:
			stat = "HITS";
			break;
		case STAT_INT:
			stat = "MANA";
			break;
		case STAT_DEX:
			stat = "STAM";
			break;
		case STAT_FOOD:
			stat = "FOOD";
			break;
		default:
			break;
	}

	if ( iStat <= STAT_FOOD )
	{
		char sRegen[21];
		if ( bGetTicks )
		{
			sprintf(sRegen, "REGEN%s", stat);
			unsigned short iRate = static_cast<unsigned short>( GetDefNum(sRegen, true) ) * TICK_PER_SEC;
			if ( iRate )
				return maximum(0, iRate);

			return static_cast<unsigned short>(maximum(0, g_Cfg.m_iRegenRate[iStat]));
		}
		else
		{
			sprintf(sRegen, "REGENVAL%s", stat);
			return static_cast<unsigned short>(maximum(1, GetDefNum(sRegen, true)));
		}
	}
	return 0;
}

SKILLLOCK_TYPE CChar::Stat_GetLock(STAT_TYPE stat)
{
	if (!m_pPlayer)
		return SKILLLOCK_UP;	// Always raising status for NPCs.
	return m_pPlayer->Stat_GetLock(stat);
};

void CChar::Stat_SetLock(STAT_TYPE stat, SKILLLOCK_TYPE state)
{
	if (!m_pPlayer)
		return;
	return m_pPlayer->Stat_SetLock(stat,state);
};

bool CChar::Stat_Decrease(STAT_TYPE stat, SKILL_TYPE skill)
{
	// Stat to decrease
	// Skill = is this being called from Skill_Gain? if so we check this skill's bonuses.
	if ( !m_pPlayer )
		return false;

	// Check for stats degrade.
	int iStatSumAvg = Stat_GetLimit(STAT_QTY);
	int iStatSum = Stat_GetSum() + 1;	// +1 here assuming we are going to have +1 stat at some point thus we are calling this function
	if (iStatSum < iStatSumAvg)		//No need to lower any stat.
		return true;

	int iminval;
	if ( skill )
		iminval = Stat_GetMax(stat);
	else
		iminval = Stat_GetAdjusted(stat);

	// We are at a point where our skills can degrade a bit.
	int iStatSumMax = iStatSumAvg + iStatSumAvg / 4;
	int iChanceForLoss = Calc_GetSCurve(iStatSumMax - iStatSum, (iStatSumMax - iStatSumAvg) / 4);
	if ( iChanceForLoss > Calc_GetRandVal(1000) )
	{
		// Find the stat that was used least recently and degrade it.
		int imin = -1;
		int ival = 0;
		for ( int i = STAT_STR; i<STAT_BASE_QTY; i++ )
		{
			if ( static_cast<STAT_TYPE>(i) == stat )
				continue;
			if ( !Stat_GetLock( static_cast<STAT_TYPE>(i) ) == SKILLLOCK_DOWN )
				continue;

			if ( skill )
			{
				const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(skill);
				ival = pSkillDef->m_StatBonus[i];
			}
			else
				ival = Stat_GetBase( static_cast<STAT_TYPE>(i) );

			if ( iminval > ival )
			{
				imin = i;
				iminval = ival;
			}
		}

		if ( imin < 0 )
			return false;

		short iStatVal = Stat_GetBase(static_cast<STAT_TYPE>(imin));
		if ( iStatVal > 10 )
		{
			Stat_SetBase(static_cast<STAT_TYPE>(imin), static_cast<short>(iStatVal - 1));
			return true;
		}
	}
	return false;
}
