//  CChar is either an NPC or a Player.

#include "../../common/CException.h"
#include "../../common/CScriptTriggerArgs.h"
#include "../triggers.h"
#include "../CWorld.h"
#include "CChar.h"

//----------------------------------------------------------------------

void CChar::Stat_AddMod( STAT_TYPE i, short iVal )
{
	ADDTOCALLSTACK("CChar::Stat_AddMod");
	ASSERT(i >= 0 && i < STAT_QTY);
	m_Stat[i].m_mod	+= iVal;

	const ushort uiMaxValue = Stat_GetMax(i);		// make sure the current value is not higher than new max value
	if ( m_Stat[i].m_val > uiMaxValue )
		m_Stat[i].m_val = uiMaxValue;

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
			iVal = (short)(args.m_iN3);
		}
	}

	m_Stat[i].m_mod = iVal;

	if ( i == STAT_STR && iVal < iStatVal )
	{
		// ModSTR is being decreased, so check if the char still have enough STR to use current equipped items
		CItem *pItemNext = nullptr;
		for ( CItem *pItem = GetContentHead(); pItem != nullptr; pItem = pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( !CanEquipStr(pItem) )
			{
				SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
				ItemBounce(pItem, false);
			}
		}
	}

	const ushort uiMaxValue = Stat_GetMax(i);		// make sure the current value is not higher than new max value
	if ( m_Stat[i].m_val > uiMaxValue )
		m_Stat[i].m_val = uiMaxValue;

	UpdateStatsFlag();
}

short CChar::Stat_GetMod( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetMod");
	ASSERT(i >= 0 && i < STAT_QTY);
	return m_Stat[i].m_mod;
}

void CChar::Stat_SetVal( STAT_TYPE i, ushort uiVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetVal");
	if (i > STAT_BASE_QTY || i == STAT_FOOD) // Food must trigger Statchange. Redirect to Base value
	{
		Stat_SetBase(i, uiVal);
		return;
	}
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food
	m_Stat[i].m_val = uiVal;
}

ushort CChar::Stat_GetVal( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetVal");
	if ( i > STAT_BASE_QTY || i == STAT_FOOD ) // Food must trigger Statchange. Redirect to Base value
		return Stat_GetBase(i);
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food
	return m_Stat[i].m_val;
}


void CChar::Stat_SetMax( STAT_TYPE i, ushort uiVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetMax");
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food

	if ( g_Cfg.m_iStatFlag && ((g_Cfg.m_iStatFlag & STAT_FLAG_DENYMAX) || (m_pPlayer && (g_Cfg.m_iStatFlag & STAT_FLAG_DENYMAXP)) || (m_pNPC && (g_Cfg.m_iStatFlag & STAT_FLAG_DENYMAXN))) )
		m_Stat[i].m_max = 0;
	else
	{
		if ( IsTrigUsed(TRIGGER_STATCHANGE) && !IsTriggerActive("CREATE") )
		{
			if ( i >= STAT_STR && i <= STAT_FOOD )		// only STR, DEX, INT, FOOD fire MaxHits, MaxMana, MaxStam, MaxFood for @StatChange
			{
				CScriptTriggerArgs args;
				args.m_iN1 = i + 4;		// shift by 4 to indicate MaxHits, etc..
				args.m_iN2 = Stat_GetMax(i);
				args.m_iN3 = uiVal;
				if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
					return;
				// do not restore argn1 to i, bad things will happen! leave it untouched. (matex)
				uiVal = (ushort)(args.m_iN3);
			}
		}
		m_Stat[i].m_max = uiVal;

		const ushort uiMaxValue = Stat_GetMax(i);		// make sure the current value is not higher than new max value
		if ( m_Stat[i].m_val > uiMaxValue )
			m_Stat[i].m_val = uiMaxValue;

		if ( i == STAT_STR )
			UpdateHitsFlag();
		else if ( i == STAT_INT )
			UpdateManaFlag();
		else if ( i == STAT_DEX )
			UpdateStamFlag();
	}
}

ushort CChar::Stat_GetMax( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetMax");
	ushort uiVal;
	ASSERT(i >= 0 && i < STAT_QTY); // allow for food
	if ( m_Stat[i].m_max < 1 )
	{
		if ( i == STAT_FOOD )
		{
			CCharBase * pCharDef = Char_GetDef();
			ASSERT(pCharDef);
            uiVal = pCharDef->m_MaxFood;
		}
		else
            uiVal = Stat_GetAdjusted(i);

		if ( i == STAT_INT )
		{
			if ( (g_Cfg.m_iRacialFlags & RACIALF_ELF_WISDOM) && IsElf() )
                uiVal += 20;		// elves always have +20 max mana (Wisdom racial trait)
		}
		return uiVal;
	}
    uiVal = m_Stat[i].m_max;
	if ( i >= STAT_BASE_QTY )
        uiVal += m_Stat[i].m_mod;
	return uiVal;
}

uint CChar::Stat_GetSum() const
{
	ADDTOCALLSTACK("CChar::Stat_GetSum");
	ushort uiStatSum = 0;
	for ( int i = 0; i < STAT_BASE_QTY; ++i )
		uiStatSum += Stat_GetBase((STAT_TYPE)i);

	return uiStatSum;
}

ushort CChar::Stat_GetAdjusted( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetAdjusted");
    return (Stat_GetBase(i) + Stat_GetMod(i));
}

ushort CChar::Stat_GetBase( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetBase");
	ASSERT(i >= 0 && i < STAT_QTY);

	return m_Stat[i].m_base;
}

void CChar::Stat_AddBase( STAT_TYPE i, ushort uiVal )
{
	ADDTOCALLSTACK("CChar::Stat_AddBase");
	Stat_SetBase( i, Stat_GetBase(i) + uiVal );
}

void CChar::Stat_SetBase( STAT_TYPE i, ushort uiVal )
{ 
	ADDTOCALLSTACK("CChar::Stat_SetBase");
	ASSERT(i >= 0 && i < STAT_QTY);

	ushort uiStatVal = Stat_GetBase(i);
	if (IsTrigUsed(TRIGGER_STATCHANGE) && !g_Serv.IsLoading() && !IsTriggerActive("CREATE"))
	{
		// Only Str, Dex, Int, Food fire @StatChange here
		if (i >= STAT_STR && i <= STAT_FOOD)
		{
			CScriptTriggerArgs args;
			args.m_iN1 = i;
			args.m_iN2 = uiStatVal;
			args.m_iN3 = uiVal;
			if (OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE)
				return;
			// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
			uiVal = (ushort)(args.m_iN3);

			if (i != STAT_FOOD && m_Stat[i].m_max < 1) // MaxFood cannot depend on something, otherwise if the Stat depends on STR, INT, DEX, fire MaxHits, MaxMana, MaxStam
			{
				args.m_iN1 = i+4; // Shift by 4 to indicate MaxHits, MaxMana, MaxStam
				args.m_iN2 = uiStatVal;
				args.m_iN3 = uiVal;
				if (OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE)
					return;
				// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
				uiVal = (ushort)(args.m_iN3);
			}
		}
	}
	switch ( i )
	{
		case STAT_STR:
			{
				CCharBase * pCharDef = Char_GetDef();
				if ( pCharDef && !pCharDef->m_Str )
					pCharDef->m_Str = uiVal;
			}
			break;
		case STAT_INT:
			{
				CCharBase * pCharDef = Char_GetDef();
				if ( pCharDef && !pCharDef->m_Int )
					pCharDef->m_Int = uiVal;
			}
			break;
		case STAT_DEX:
			{
				CCharBase * pCharDef = Char_GetDef();
				if ( pCharDef && !pCharDef->m_Dex )
					pCharDef->m_Dex = uiVal;
			}
			break;
		case STAT_FOOD:
			break;
		default:
			throw CSError(LOGL_CRIT, 0, "Stat_SetBase: index out of range");
	}

	m_Stat[i].m_base = uiVal;

	if ( (i == STAT_STR) && (uiVal < uiStatVal) )
	{
		// STR is being decreased, so check if the char still have enough STR to use current equipped items
		CItem *pItemNext = nullptr;
		for ( CItem *pItem = GetContentHead(); pItem != nullptr; pItem = pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( !CanEquipStr(pItem) )
			{
				SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
				ItemBounce(pItem, false);
			}
		}
	}

	const ushort uiMaxValue = Stat_GetMax(i);    // make sure the current value is not higher than new max value
	if ( m_Stat[i].m_val > uiMaxValue )
		m_Stat[i].m_val = uiMaxValue;

	UpdateStatsFlag();
}

ushort CChar::Stat_GetLimit( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetLimit");
	const CVarDefCont * pTagStorage = nullptr;
	TemporaryString tsStatName;
	tchar* pszStatName = static_cast<tchar *>(tsStatName);

	if ( m_pPlayer )
	{
		const CSkillClassDef * pSkillClass = m_pPlayer->GetSkillClass();
		ASSERT(pSkillClass);
		ASSERT( i >= 0 && i < STAT_BASE_QTY );

		sprintf(pszStatName, "OVERRIDE.STATCAP_%d", (int)i);
		ushort uiStatMax;
		if ( (pTagStorage = GetKey(pszStatName, true)) != nullptr )
			uiStatMax = (ushort)(pTagStorage->GetValNum());
		else
			uiStatMax = pSkillClass->m_StatMax[i];

		if ( m_pPlayer->Stat_GetLock(i) >= SKILLLOCK_DOWN )
		{
			ushort uiStatLevel = Stat_GetBase(i);
			if ( uiStatLevel < uiStatMax )
				uiStatMax = uiStatLevel;
		}
		return uiStatMax;
	}
	else
	{
		ushort uiStatMax = 100;
		sprintf(pszStatName, "OVERRIDE.STATCAP_%d", (int)i);
		if ( (pTagStorage = GetKey(pszStatName, true)) != nullptr )
			uiStatMax = (ushort)(pTagStorage->GetValNum());

		return uiStatMax;
	}
}

uint CChar::Stat_GetSumLimit() const
{
    ADDTOCALLSTACK("CChar::Stat_GetSumLimit");
    // The return value is uint, but the value supported by the packets is a word (which is smaller)
    if ( m_pPlayer )
    {
        const CSkillClassDef * pSkillClass = m_pPlayer->GetSkillClass();
        ASSERT(pSkillClass);
        const CVarDefCont * pTagStorage = GetKey("OVERRIDE.STATSUM", true);
        return pTagStorage ? ((uint)(pTagStorage->GetValNum())) : (uint)(pSkillClass->m_StatSumMax);
    }
    else
    {
        const CVarDefCont * pTagStorage = GetKey("OVERRIDE.STATSUM", true);
        return pTagStorage ? (uint)(pTagStorage->GetValNum()) : 300;
    }
}


bool CChar::Stats_Regen()
{
	ADDTOCALLSTACK("CChar::Stats_Regen");
	// Calling regens in all stats and checking REGEN%s/REGEN%sVAL where %s is hits/stam... to check values/delays
	// Food decay called here too.
	// calling @RegenStat for each stat if proceed.
	int iHitsHungerLoss = g_Cfg.m_iHitsHungerLoss ? g_Cfg.m_iHitsHungerLoss : 0;
    const int64 iCurTime = CServerTime::GetCurrentTime().GetTimeRaw();
	for (STAT_TYPE i = STAT_STR; i <= STAT_FOOD; i = (STAT_TYPE)(i + 1))
	{
        int64 iRegenDelay = Stats_GetRegenVal(i, true) * MSECS_PER_SEC; // Get chars regen[n] delay (if none, check's for sphere.ini's value)
        if (iRegenDelay < 0)    // No value (on both char & ini)? then do not regen this stat.
        {
            continue;
        }
        if (m_Stat[i].m_regen > iCurTime) // Check if enough time elapsed till the next regen.
        {
            continue;
        }
        m_Stat[i].m_regen = iCurTime + iRegenDelay; // in msecs

		ushort uiMod = (ushort)Stats_GetRegenVal(i, false);
		if ((i == STAT_STR) && (g_Cfg.m_iRacialFlags & RACIALF_HUMAN_TOUGH) && IsHuman())
            uiMod += 2;		// Humans always have +2 hitpoint regeneration (Tough racial trait)
		
		if (g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B)
		{
			int iGain = Skill_Focus(i);
			if (iGain > 0)
                uiMod += (ushort)iGain;
		}
		ushort uiStatLimit = Stat_GetMax(i);

		if (IsTrigUsed(TRIGGER_REGENSTAT))
		{
			CScriptTriggerArgs Args;
			Args.m_VarsLocal.SetNum("StatID", i, true);
			Args.m_VarsLocal.SetNum("Value", uiMod, true);
			Args.m_VarsLocal.SetNum("StatLimit", uiStatLimit, true);
			if (i == STAT_FOOD)
				Args.m_VarsLocal.SetNum("HitsHungerLoss", iHitsHungerLoss);

			if (OnTrigger(CTRIG_RegenStat, this, &Args) == TRIGRET_RET_TRUE)
			{
				m_Stat[i].m_regen = 0;
				continue;
			}

			i = (STAT_TYPE)(Args.m_VarsLocal.GetKeyNum("StatID"));
			if (i < STAT_STR)
				i = STAT_STR;
			else if (i > STAT_FOOD)
				i = STAT_FOOD;
            uiMod = (ushort)(Args.m_VarsLocal.GetKeyNum("Value"));
			uiStatLimit = (ushort)(Args.m_VarsLocal.GetKeyNum("StatLimit"));
			if (i == STAT_FOOD)
				iHitsHungerLoss = (int)(Args.m_VarsLocal.GetKeyNum("HitsHungerLoss"));
		}
		if (uiMod == 0)
			continue;

		if (i == STAT_FOOD)
			OnTickFood(uiMod, iHitsHungerLoss);
		else
			UpdateStatVal(i, uiMod, uiStatLimit);
	}
	return true;
}
ushort CChar::Stats_GetRegenVal(STAT_TYPE iStat, bool fGetTicks)
{
	ADDTOCALLSTACK("CChar::Stats_GetRegenVal");
	// Return regen rates and regen val for the given stat.
	// fGetTicks = true returns the regen ticks (in seconds)
	// fGetTicks = false returns the values of regeneration.

	lpctstr stat = "";
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
		if ( fGetTicks )
		{
			sprintf(sRegen, "REGEN%s", stat);   // in seconds
			ushort iRate = (ushort)(GetDefNum(sRegen, true)); // in seconds
			if ( iRate )
				return iRate; //maximum(0, iRate);

			return (ushort)(maximum(0, g_Cfg.m_iRegenRate[iStat])); // in seconds
		}
		else
		{
			sprintf(sRegen, "REGENVAL%s", stat);
			ushort iRate = (ushort)GetDefNum(sRegen, true);
			return maximum(1, iRate);
		}
	}
	return 0;
}

SKILLLOCK_TYPE CChar::Stat_GetLock(STAT_TYPE stat)
{
	if (!m_pPlayer)
		return SKILLLOCK_UP;	// Always raising status for NPCs.
	return m_pPlayer->Stat_GetLock(stat);
}

void CChar::Stat_SetLock(STAT_TYPE stat, SKILLLOCK_TYPE state)
{
	if (!m_pPlayer)
		return;
	return m_pPlayer->Stat_SetLock(stat,state);
}

short CChar::GetKarma() const
{
    return (short)(maximum(g_Cfg.m_iMinKarma, minimum(g_Cfg.m_iMaxKarma, m_iKarma)));
}

void CChar::SetKarma(short iNewKarma)
{
    m_iKarma = (short)(maximum(g_Cfg.m_iMinKarma, minimum(g_Cfg.m_iMaxKarma, iNewKarma)));
    if ( !g_Serv.IsLoading() )
        NotoSave_Update();
}

ushort CChar::GetFame() const
{
    return (ushort)(minimum(g_Cfg.m_iMaxFame, m_uiFame));
}

void CChar::SetFame(ushort uiNewFame)
{
    m_uiFame = (ushort)(minimum(g_Cfg.m_iMaxFame, uiNewFame));
}

bool CChar::Stat_Decrease(STAT_TYPE stat, SKILL_TYPE skill)
{
	// Stat to decrease
	// Skill = is this being called from Skill_Gain? if so we check this skill's bonuses.
	if ( !m_pPlayer )
		return false;

	// Check for stats degrade.
	const uint uiStatSumLimit = Stat_GetSumLimit();
	const uint uiStatSum = Stat_GetSum() + 1;	// +1 here assuming we are going to have +1 stat at some point thus we are calling this function
	
	/*Before there was iStatSum < iStatSumAvg:
	In that case, if the sum of the player's Stats(iStatSum) was at StatCap - 1 (before adding the +1 above) the selected Stat will not increase
	unless one of the other stats was set for decreasing (stat arrow down) */
	if (uiStatSum <= uiStatSumLimit)		//No need to lower any stat.
		return true;

	uint uiMinval;
	if ( skill != SKILL_NONE )
        uiMinval = Stat_GetMax(stat);
	else
        uiMinval = Stat_GetAdjusted(stat);

	// We are at a point where our skills can degrade a bit.
	uint uiStatSumMax = uiStatSumLimit + uiStatSumLimit / 4;
	const int iChanceForLoss = Calc_GetSCurve(uiStatSumMax - uiStatSumLimit, (uiStatSumMax - uiStatSumLimit) / 4);
	if ( iChanceForLoss > Calc_GetRandVal(1000) )
	{
		// Find the stat that was used least recently and degrade it.
		int iMin = -1;
		uint uiVal = 0;
		for ( int i = STAT_STR; i<STAT_BASE_QTY; ++i )
		{
			if ( (STAT_TYPE)i == stat )
				continue;
			if ( Stat_GetLock( (STAT_TYPE)(i) ) != SKILLLOCK_DOWN )
				continue;

			if ( skill != SKILL_NONE )
			{
				const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(skill);
				uiVal = pSkillDef->m_StatBonus[i];
			}
			else
				uiVal = Stat_GetBase( (STAT_TYPE)(i) );

			if ( uiMinval > uiVal )
			{
				iMin = i;
                uiMinval = uiVal;
			}
		}

		if ( iMin < 0 )
			return false;

		ushort uiStatVal = Stat_GetBase((STAT_TYPE)iMin);
		if ( uiStatVal > 10 )
		{
			Stat_SetBase((STAT_TYPE)iMin, (ushort)(uiStatVal - 1));
			return true;
		}
	}
	return false;
}
