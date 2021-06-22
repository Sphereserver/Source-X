//  CChar is either an NPC or a Player.

#include "../../common/resource/sections/CSkillClassDef.h"
#include "../../common/CException.h"
#include "../../common/CScriptTriggerArgs.h"
#include "../triggers.h"
#include "../CServer.h"
#include "../CWorldGameTime.h"
#include "../CWorldTickingList.h"
#include "CChar.h"

//----------------------------------------------------------------------

void CChar::Stat_AddMod( STAT_TYPE i, int iVal )
{
	ADDTOCALLSTACK("CChar::Stat_AddMod");
	ASSERT(i >= 0 && i < STAT_QTY);
    if (iVal == 0)
        return;

    Stat_SetMod(i, Stat_GetMod(i) + iVal);
}

void CChar::Stat_SetMod( STAT_TYPE i, int iVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetMod");
	ASSERT((i >= 0) && (i < STAT_QTY)); // allow for food

	const int iStatVal = Stat_GetMod(i);
	if ( IsTrigUsed(TRIGGER_STATCHANGE) && !IsTriggerActive("CREATE") )
	{
		if ( i >= STAT_STR && i <= STAT_DEX )
		{
			CScriptTriggerArgs args;
			args.m_iN1 = i + 8LL;	// shift by 8 to indicate modSTR, modINT, modDEX
			args.m_iN2 = iStatVal;
			args.m_iN3 = iVal;
			if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
				return;
			// do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
			
			iVal = (int)(args.m_iN3);
		}
	}

	const int iPrevVal = iVal;
	if (iVal > UINT16_MAX)
		iVal = UINT16_MAX;
	else if (iVal < -UINT16_MAX)
		iVal = -UINT16_MAX;
	if (iVal != iPrevVal)
		g_Log.EventWarn("Trying to set MOD%s to invalid value=%d. Defaulting it to %d.\n", g_Cfg.GetStatName(i), iPrevVal, iVal);

	m_Stat[i].m_mod = iVal;

	if ( i == STAT_STR && iVal < iStatVal )
	{
		// ModSTR is being decreased, so check if the char still have enough STR to use current equipped items
		Stat_StrCheckEquip();
	}

    if (!IsSetOF(OF_StatAllowValOverMax))
    {
        const ushort uiMaxValue = Stat_GetMaxAdjusted(i);		// make sure the current value is not higher than new max value
        if ( m_Stat[i].m_val > uiMaxValue )
            m_Stat[i].m_val = uiMaxValue;
    }

	UpdateStatsFlag();
}

int CChar::Stat_GetMod( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetMod");
	ASSERT(i >= 0 && i < STAT_QTY);
	return m_Stat[i].m_mod;
}

void CChar::Stat_SetMaxMod( STAT_TYPE i, int iVal )
{
    ADDTOCALLSTACK("CChar::Stat_SetMaxMod");
	ASSERT((i >= 0) && (i < STAT_QTY)); // allow for food

    int iStatVal = Stat_GetMaxMod(i);
    if ( IsTrigUsed(TRIGGER_STATCHANGE) && !IsTriggerActive("CREATE") )
    {
        if ( (i >= STAT_STR) && (i <= STAT_DEX) )
        {
            CScriptTriggerArgs args;
            args.m_iN1 = i + 12LL;	// shift by 12 to indicate modMaxHits, modMaxMana, modMaxStam
            args.m_iN2 = iStatVal;
            args.m_iN3 = iVal;
            if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
                return;
            // do not restore argn1 to i, bad things will happen! leave i untouched. (matex)
            iVal = (int)(args.m_iN3);
        }
    }

	const int iPrevVal = iVal;
	if (iVal > UINT16_MAX)
		iVal = UINT16_MAX;
	else if (iVal < -UINT16_MAX)
		iVal = -UINT16_MAX;
	if (iVal != iPrevVal)
		g_Log.EventWarn("Trying to set MODMAX%s to invalid value=%d. Defaulting it to %d.\n", g_Cfg.GetStatName(i), iPrevVal, iVal);

    m_Stat[i].m_maxMod = iVal;

    if (!IsSetOF(OF_StatAllowValOverMax))
    {
        const ushort uiMaxValue = Stat_GetMaxAdjusted(i);		// make sure the current value is not higher than new max value
        if ( m_Stat[i].m_val > uiMaxValue )
            m_Stat[i].m_val = uiMaxValue;
    }

    UpdateStatsFlag();
}

void CChar::Stat_AddMaxMod( STAT_TYPE i, int iVal )
{
    ADDTOCALLSTACK("CChar::Stat_AddMaxMod");
	ASSERT((i >= 0) && (i < STAT_QTY)); // allow for food
    if (iVal == 0)
        return;

	const int iPrevVal = iVal;
	if (iVal > UINT16_MAX)
		iVal = UINT16_MAX;
	else if (iVal < -UINT16_MAX)
		iVal = -UINT16_MAX;
	if (iVal != iPrevVal)
		g_Log.EventWarn("Trying to add MODMAX%s to invalid value=%d. Defaulting it to %d.\n", g_Cfg.GetStatName(i), iPrevVal, iVal);

    m_Stat[i].m_maxMod += iVal;

    if (!IsSetOF(OF_StatAllowValOverMax))
    {
        const ushort uiMaxValue = Stat_GetMaxAdjusted(i);		// make sure the current value is not higher than new max value
        if ( m_Stat[i].m_val > uiMaxValue )
            m_Stat[i].m_val = uiMaxValue;
    }

    UpdateStatsFlag();
}

int CChar::Stat_GetMaxMod( STAT_TYPE i ) const
{
    ADDTOCALLSTACK("CChar::Stat_GetMaxMod");
	ASSERT((i >= 0) && (i < STAT_QTY)); // allow for food
    return m_Stat[i].m_maxMod;
}

void CChar::Stat_SetVal( STAT_TYPE i, ushort uiVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetVal");
	if (i > STAT_BASE_QTY || i == STAT_FOOD) // Food must trigger Statchange. Redirect to Base value
	{
		Stat_SetBase(i, uiVal);
		return;
	}

	ASSERT((i >= 0) && (i < STAT_QTY)); // allow for food
	m_Stat[i].m_val = uiVal;

    if ((i == STAT_STR) && (uiVal == 0))
    {   // Ensure this char will tick and die
        CWorldTickingList::AddCharPeriodic(this, false);
    }
}

void CChar::Stat_AddVal( STAT_TYPE i, int iVal )
{
    ADDTOCALLSTACK("CChar::Stat_AddVal");
    if (iVal == 0)
        return;

    if (i > STAT_BASE_QTY || i == STAT_FOOD) // Food must trigger Statchange. Redirect to Base value
    {
        Stat_AddBase(i, iVal);
        return;
    }

    ASSERT((i >= 0) && (i < STAT_QTY)); // allow for food
    iVal = m_Stat[i].m_val + iVal;
    m_Stat[i].m_val = (ushort)(maximum(0, iVal));

    if ((i == STAT_STR) && (iVal <= 0))
    {   // Ensure this char will tick and die
		CWorldTickingList::AddCharPeriodic(this, false);
    }
}

ushort CChar::Stat_GetVal( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetVal");
	if ( i > STAT_BASE_QTY || i == STAT_FOOD ) // Food must trigger Statchange. Redirect to Base value
		return Stat_GetBase(i);

	ASSERT((i >= 0) && (i < STAT_QTY)); // allow for food
	return m_Stat[i].m_val;
}

void CChar::Stat_SetMax( STAT_TYPE i, ushort uiVal )
{
	ADDTOCALLSTACK("CChar::Stat_SetMax");
	ASSERT((i >= 0) && (i < STAT_QTY)); // allow for food

	if ( g_Cfg._uiStatFlag && ((g_Cfg._uiStatFlag & STAT_FLAG_DENYMAX) || (m_pPlayer && (g_Cfg._uiStatFlag & STAT_FLAG_DENYMAXP)) || (m_pNPC && (g_Cfg._uiStatFlag & STAT_FLAG_DENYMAXN))) )
    {
		m_Stat[i].m_max = 0;
    }
	else
	{
		if ( IsTrigUsed(TRIGGER_STATCHANGE) && !IsTriggerActive("CREATE") )
		{
			if ( i >= STAT_STR && i < STAT_QTY )		// only STR, DEX, INT, FOOD fire MaxHits, MaxMana, MaxStam, MaxFood for @StatChange
			{
				CScriptTriggerArgs args;
				args.m_iN1 = i + 4LL;		// shift by 4 to indicate MaxHits, etc..
				args.m_iN2 = Stat_GetMax(i);
				args.m_iN3 = uiVal;
				if ( OnTrigger(CTRIG_StatChange, this, &args) == TRIGRET_RET_TRUE )
					return;
				// do not restore argn1 to i, bad things will happen! leave it untouched. (matex)
				uiVal = (ushort)(args.m_iN3);
			}
		}
		m_Stat[i].m_max = uiVal;

        if (!IsSetOF(OF_StatAllowValOverMax))
        {
            const ushort uiMaxValue = Stat_GetMaxAdjusted(i);		// make sure the current value is not higher than new max value
            if ( m_Stat[i].m_val > uiMaxValue )
                m_Stat[i].m_val = uiMaxValue;
        }

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
	ASSERT((i >= 0) && (i < STAT_QTY)); // allow for food

    ushort uiVal;
	if ( m_Stat[i].m_max < 1 )
	{
		if ( i == STAT_FOOD )
		{
			const CCharBase * pCharDef = Char_GetDef();
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
	return uiVal;
}

ushort CChar::Stat_GetMaxAdjusted( STAT_TYPE i ) const
{
    ADDTOCALLSTACK("CChar::Stat_GetMaxAdjusted");
    return ushort(Stat_GetMax(i) + Stat_GetMaxMod(i));
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
    return ushort(Stat_GetBase(i) + Stat_GetMod(i));
}

ushort CChar::Stat_GetBase( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetBase");
	ASSERT(i >= 0 && i < STAT_QTY);

	return m_Stat[i].m_base;
}

void CChar::Stat_AddBase( STAT_TYPE i, int iVal )
{
	ADDTOCALLSTACK("CChar::Stat_AddBase");
    if (iVal == 0)
        return;

	Stat_SetBase( i, ushort(Stat_GetBase(i) + iVal) );
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
				args.m_iN1 = i + 4LL; // Shift by 4 to indicate MaxHits, MaxMana, MaxStam
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
		Stat_StrCheckEquip();
	}

    if (!IsSetOF(OF_StatAllowValOverMax))
    {
        const ushort uiMaxValue = Stat_GetMaxAdjusted(i);		// make sure the current value is not higher than new max value
        if ( m_Stat[i].m_val > uiMaxValue )
            m_Stat[i].m_val = uiMaxValue;
    }

	UpdateStatsFlag();
}

ushort CChar::Stat_GetLimit( STAT_TYPE i ) const
{
	ADDTOCALLSTACK("CChar::Stat_GetLimit");
	const CVarDefCont * pTagStorage = nullptr;
	TemporaryString tsStatName;

	if ( m_pPlayer )
	{
		const CSkillClassDef * pSkillClass = m_pPlayer->GetSkillClass();
		ASSERT(pSkillClass);
		ASSERT( i >= 0 && i < STAT_BASE_QTY );

        ushort uiStatMax;
		snprintf(tsStatName.buffer(), tsStatName.capacity(), "OVERRIDE.STATCAP_%d", (int)i);
		if ( (pTagStorage = GetKey(tsStatName, true)) != nullptr )
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
		snprintf(tsStatName.buffer(), tsStatName.capacity(), "OVERRIDE.STATCAP_%d", (int)i);
		if ( (pTagStorage = GetKey(tsStatName, true)) != nullptr )
			uiStatMax = (ushort)(pTagStorage->GetValNum());

		return uiStatMax;
	}
}

uint CChar::Stat_GetSumLimit() const
{
    ADDTOCALLSTACK("CChar::Stat_GetSumLimit");
    // The return value is uint, but the value supported by the packets is a word (which is smaller)
	const CVarDefCont* pTagStorage = GetKey("OVERRIDE.STATSUM", true);
    if ( m_pPlayer )
    {
        const CSkillClassDef * pSkillClass = m_pPlayer->GetSkillClass();
        ASSERT(pSkillClass);
        return pTagStorage ? ((uint)(pTagStorage->GetValNum())) : (uint)(pSkillClass->m_StatSumMax);
    }
    return pTagStorage ? (uint)(pTagStorage->GetValNum()) : 300;
}

bool CChar::Stats_Regen()
{
	ADDTOCALLSTACK("CChar::Stats_Regen");
	// Calling regens in all stats and checking REGEN%s/REGEN%sVAL where %s is hits/stam... to check values/delays
	// Food decay called here too.
	// calling @RegenStat for each stat if proceed.
	int iHitsHungerLoss = g_Cfg.m_iHitsHungerLoss ? g_Cfg.m_iHitsHungerLoss : 0;
    const int64 iCurTime = CWorldGameTime::GetCurrentTime().GetTimeRaw();
	for (STAT_TYPE i = STAT_STR; i <= STAT_FOOD; i = (STAT_TYPE)(i + 1))
	{
        const int64 iRegenDelay = Stats_GetRegenRate(i); // Get chars regen[n] delay (if none, check's for sphere.ini's value)
        if (iRegenDelay < 0)    // No value (on both char & ini)? then do not regen this stat.
        {
            continue;
        }
        if (m_Stat[i].m_regenLast + iRegenDelay > iCurTime) // Check if enough time elapsed till the next regen.
        {
            continue;
        }
        m_Stat[i].m_regenLast = iCurTime; // in msecs

		const ushort uiStatVal = Stat_GetVal(i);
		int iMod = (int)Stats_GetRegenVal(i);
        ushort uiStatLimit = Stat_GetMaxAdjusted(i);
		int iFocusGain = 0;
        if (uiStatVal <= uiStatLimit)
        {
            if ((i == STAT_STR) && (g_Cfg.m_iRacialFlags & RACIALF_HUMAN_TOUGH) && IsHuman())
                iMod += 2;		// Humans always have +2 hitpoint regeneration (Tough racial trait)

            if (g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B 
				&& (i == STAT_DEX || i == STAT_INT) 
				&& uiStatVal < uiStatLimit)
            {
                iFocusGain = Skill_Focus(i);
				if (iFocusGain < 0)
					iFocusGain = 0;
            }
        }
        else
        {
            iMod = -1;
        }
		
		if (IsTrigUsed(TRIGGER_REGENSTAT))
		{
			CScriptTriggerArgs Args;
			Args.m_VarsLocal.SetNum("StatID", i, true);
			Args.m_VarsLocal.SetNum("Value", iMod, true);
			Args.m_VarsLocal.SetNum("StatLimit", uiStatLimit, true);

			if (i == STAT_DEX || i == STAT_INT)
				Args.m_VarsLocal.SetNum("FocusValue", iFocusGain);

			if (i == STAT_FOOD)
				Args.m_VarsLocal.SetNum("HitsHungerLoss", iHitsHungerLoss);

			if (OnTrigger(CTRIG_RegenStat, this, &Args) == TRIGRET_RET_TRUE)
			{
                // Setting the last regen time to 0 will make this trigger be called over and over and over, without a delay, which
                //  can suck quite a bit cpu.
				//m_Stat[i].m_regenLast = 0;
				continue;
			}

			i = (STAT_TYPE)(Args.m_VarsLocal.GetKeyNum("StatID"));
			if (i < STAT_STR)
				i = STAT_STR;
			else if (i > STAT_FOOD)
				i = STAT_FOOD;
            iMod = (int)(Args.m_VarsLocal.GetKeyNum("Value"));
			uiStatLimit = (ushort)(Args.m_VarsLocal.GetKeyNum("StatLimit"));

			if (i == STAT_DEX || i == STAT_INT)
			{
				iFocusGain = (int)(Args.m_VarsLocal.GetKeyNum("FocusValue"));
				iMod += iFocusGain;
			}

			if (i == STAT_FOOD)
				iHitsHungerLoss = (int)(Args.m_VarsLocal.GetKeyNum("HitsHungerLoss"));
		}
		if (iMod == 0)
			continue;

		if (i == STAT_FOOD)
			OnTickFood((ushort)iMod, iHitsHungerLoss);
		else
			UpdateStatVal(i, iMod, uiStatLimit);
	}
	return true;
}

int64 CChar::Stats_GetRegenRate(STAT_TYPE iStat)
{
    ADDTOCALLSTACK("CChar::Stats_GetRegenRate");
    // Return regen rate for the given stat.

    ASSERT ( (iStat >= STAT_STR) && (iStat <= STAT_FOOD) );
    int64 iRate = m_Stat[iStat].m_regenRate;
    if (iRate < 0)    // never regen
        return 0;
    if (iRate == 0)
        iRate = g_Cfg.m_iRegenRate[iStat];
    return iRate;
}

void CChar::Stats_SetRegenRate(STAT_TYPE iStat, int64 iRateMilliseconds)
{
    ADDTOCALLSTACK("CChar::Stats_SetRegenRate");
    // Sets regen rate for the given stat (in milliseconds).

    ASSERT ( (iStat >= STAT_STR) && (iStat <= STAT_FOOD) );
    m_Stat[iStat].m_regenRate = iRateMilliseconds;
}

ushort CChar::Stats_GetRegenVal(STAT_TYPE iStat)
{
	ADDTOCALLSTACK("CChar::Stats_GetRegenVal");
	// Return regen val for the given stat.

	ASSERT ( (iStat >= STAT_STR) && (iStat <= STAT_FOOD) );
    const ushort uiVal = m_Stat[iStat].m_regenVal;
	return maximum(1, uiVal);
}

void CChar::Stats_SetRegenVal(STAT_TYPE iStat, ushort uiVal)
{
    ADDTOCALLSTACK("CChar::Stats_SetRegenVal");
    // Sets regen val for the given stat.

    ASSERT ( (iStat >= STAT_STR) && (iStat <= STAT_FOOD) );
    m_Stat[iStat].m_regenVal = uiVal;
}

void CChar::Stats_AddRegenVal(STAT_TYPE iStat, int iVal)
{
    ADDTOCALLSTACK("CChar::Stats_AddRegenVal");
    // Updated the regen val for the given stat.
    ASSERT ( (iStat >= STAT_STR) && (iStat <= STAT_FOOD) );
    if (iVal == 0)
        return;

    m_Stat[iStat].m_regenVal = ushort(m_Stat[iStat].m_regenVal + iVal);
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
    ADDTOCALLSTACK("CChar::Stat_Decrease");
	// Stat to decrease
	// Skill = is this being called from Skill_Gain? if so we check this skill's bonuses.
	if ( !m_pPlayer )
		return false;

	// Check for stats degrade.
	const uint uiStatSumLimit = Stat_GetSumLimit();	// Maximum reachable statsum.
	const uint uiStatSum = Stat_GetSum() + 1;		// Current statsum +1, assuming we are going to have +1 stat at some point thus we are calling this function
	
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

void CChar::Stat_StrCheckEquip()
{
	for (CSObjContRec* pObjRec : GetIterationSafeCont())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if (!CanEquipStr(pItem))
		{
			SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
			ItemBounce(pItem, false);
		}
	}
}