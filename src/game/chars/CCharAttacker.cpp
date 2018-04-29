// Actions specific to an NPC.
#include "../../common/CUIDExtra.h"
#include "../clients/CClient.h"
#include "../CServerTime.h"
#include "../triggers.h"
#include "CChar.h"


// Add some enemy to my Attacker list
bool CChar::Attacker_Add(CChar * pChar, int64 threat)
{
    ADDTOCALLSTACK("CChar::Attacker_Add");
    CUID uid = pChar->GetUID();
    if (!m_lastAttackers.empty())	// Must only check for existing attackers if there are any attacker already.
    {
        for (auto it = m_lastAttackers.begin(), end = m_lastAttackers.end(); it != end; ++it)
        {
            LastAttackers & refAttacker = *it;
            if (refAttacker.charUID == uid)
                return true;	// found one, no actions needed so we skip
        }
    }
    else if (IsTrigUsed(TRIGGER_COMBATSTART))
    {
        TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatStart, pChar, 0);
        if (tRet == TRIGRET_RET_TRUE)
            return false;
    }

    CScriptTriggerArgs Args;
    bool fIgnore = false;
    Args.m_iN1 = threat;
    Args.m_iN2 = fIgnore;
    if (IsTrigUsed(TRIGGER_COMBATADD))
    {
        TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatAdd, pChar, &Args);
        if (tRet == TRIGRET_RET_TRUE)
            return false;
        threat = Args.m_iN1;
        fIgnore = (Args.m_iN2 != 0);
    }

    LastAttackers attacker;
    attacker.amountDone = 0;
    attacker.charUID = uid;
    attacker.elapsed = 0;
    attacker.threat = (m_pPlayer) ? 0 : threat;
    attacker.ignore = fIgnore;
    m_lastAttackers.push_back(attacker);

    // Record the start of the fight.
    Memory_Fight_Start(pChar);
    tchar *z = Str_GetTemp();
    if (!attacker.ignore)
    {
        //if ( GetTopSector()->GetCharComplexity() < 7 )
        //{
        sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_ATTACKO), GetName(), pChar->GetName());
        UpdateObjMessage(z, NULL, pChar->GetClient(), HUE_TEXT_DEF, TALKMODE_EMOTE);
        //}

        if (pChar->IsClient() && pChar->CanSee(this))
        {
            sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_ATTACKS), GetName());
            pChar->GetClient()->addBarkParse(z, this, HUE_TEXT_DEF, TALKMODE_EMOTE);
        }
    }
    return true;
}

// Retrieves damage done to nID enemy
int64 CChar::Attacker_GetDam(size_t attackerIndex)
{
    ADDTOCALLSTACK("CChar::Attacker_GetDam");
    if (m_lastAttackers.empty())
        return -1;
    if (m_lastAttackers.size() <= attackerIndex)
        return -1;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    return refAttacker.amountDone;
}

// Retrieves the amount of time elapsed since the last hit to nID enemy
int64 CChar::Attacker_GetElapsed(size_t attackerIndex)
{
    ADDTOCALLSTACK("CChar::Attacker_GetElapsed");
    if (m_lastAttackers.empty())
        return -1;
    if (m_lastAttackers.size() <= attackerIndex)
        return -1;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    return refAttacker.elapsed;
}

// Retrieves Threat value that nID enemy represents against me
int64 CChar::Attacker_GetThreat(size_t attackerIndex)
{
    ADDTOCALLSTACK("CChar::Attacker_GetThreat");
    if (m_lastAttackers.empty())
        return -1;
    if (m_lastAttackers.size() <= attackerIndex)
        return -1;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    return refAttacker.threat ? refAttacker.threat : 0;
}

// Retrieves the character with most Threat
int64 CChar::Attacker_GetHighestThreat()
{
    ADDTOCALLSTACK("CChar::Attacker_GetHighestThreat");
    if (m_lastAttackers.empty())
        return -1;

    int64 highThreat = 0;
    for (auto it = m_lastAttackers.begin(), end = m_lastAttackers.end(); it != end; ++it)
    {
        LastAttackers & refAttacker = *it;
        if (refAttacker.threat > highThreat)
            highThreat = refAttacker.threat;
    }
    return highThreat;
}

// Retrieves the last character that I hit
CChar * CChar::Attacker_GetLast()
{
    ADDTOCALLSTACK("CChar::Attacker_GetLast");
    int64 dwLastTime = INT32_MAX, dwCurTime = 0;
    CChar * retChar = NULL;
    for (auto it = m_lastAttackers.begin(), end = m_lastAttackers.end(); it != end; ++it)
    {
        LastAttackers & refAttacker = *it;
        dwCurTime = refAttacker.elapsed;
        if (dwCurTime <= dwLastTime)
        {
            dwLastTime = dwCurTime;
            retChar = CUID(refAttacker.charUID).CharFind();
        }
    }
    return retChar;
}

// Set elapsed time (refreshing it?)
void CChar::Attacker_SetElapsed(CChar * pChar, int64 value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetElapsed(CChar)");
    return Attacker_SetElapsed(Attacker_GetID(pChar), value);
}


// Set elapsed time (refreshing it?)
void CChar::Attacker_SetElapsed(size_t attackerIndex, int64 value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetElapsed(size_t)");
    if (m_lastAttackers.empty())
        return;
    if (m_lastAttackers.size() <= attackerIndex)
        return;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    refAttacker.elapsed = value;
}

// Damaged pChar
void CChar::Attacker_SetDam(CChar * pChar, int64 value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetDam(CChar)");
    return Attacker_SetDam(Attacker_GetID(pChar), value);
}

// Damaged pChar
void CChar::Attacker_SetDam(size_t attackerIndex, int64 value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetDam(size_t)");
    if (m_lastAttackers.empty())
        return;
    if (m_lastAttackers.size() <= attackerIndex)
        return;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    refAttacker.amountDone = value;
}

// New Treat level
void CChar::Attacker_SetThreat(CChar * pChar, int64 value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetThreat(CChar)");
    return Attacker_SetThreat(Attacker_GetID(pChar), value);
}

// New Treat level
void CChar::Attacker_SetThreat(size_t attackerIndex, int64 value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetThreat(int)");
    if (m_pPlayer)
        return;
    if (m_lastAttackers.empty())
        return;
    if (m_lastAttackers.size() <= attackerIndex)
        return;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    refAttacker.threat = value;
}

// Ignoring this pChar on Hit checks
void CChar::Attacker_SetIgnore(CChar * pChar, bool fIgnore)
{
    ADDTOCALLSTACK("CChar::Attacker_SetIgnore(CChar)");
    return Attacker_SetIgnore(Attacker_GetID(pChar), fIgnore);
}

// Ignoring this pChar on Hit checks
void CChar::Attacker_SetIgnore(size_t attackerIndex, bool fIgnore)
{
    ADDTOCALLSTACK("CChar::Attacker_SetIgnore(int)");
    if (m_lastAttackers.empty())
        return;
    if (m_lastAttackers.size() <= attackerIndex)
        return;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    refAttacker.ignore = fIgnore;
}

// I'm ignoring pChar?
bool CChar::Attacker_GetIgnore(CChar * pChar)
{
    ADDTOCALLSTACK("CChar::Attacker_GetIgnore(CChar)");
    return Attacker_GetIgnore(Attacker_GetID(pChar));
}

// I'm ignoring pChar?
bool CChar::Attacker_GetIgnore(size_t attackerIndex)
{
    ADDTOCALLSTACK("CChar::Attacker_GetIgnore(size_t)");
    if (m_lastAttackers.empty())
        return false;
    if (m_lastAttackers.size() <= attackerIndex)
        return false;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    return (refAttacker.ignore != 0);
}

// Clear the whole attacker's list, combat ended.
void CChar::Attacker_Clear()
{
    ADDTOCALLSTACK("CChar::Attacker_Clear");
    if (IsTrigUsed(TRIGGER_COMBATEND))
        OnTrigger(CTRIG_CombatEnd, this, 0);

    m_lastAttackers.clear();
    if (m_pNPC)
        StatFlag_Clear(STATF_WAR);
    if (Fight_IsActive())
    {
        Skill_Start(SKILL_NONE);
        m_Fight_Targ_UID.InitUID();
    }
    UpdateModeFlag();
}

// Get nID value of attacker list from the given pChar
int CChar::Attacker_GetID(CChar * pChar) const
{
    ADDTOCALLSTACK("CChar::Attacker_GetID(CChar)");
    if (!pChar)
        return -1;
    if (m_lastAttackers.empty())
        return -1;
    int count = 0;
    for (std::vector<LastAttackers>::const_iterator it = m_lastAttackers.begin(), end = m_lastAttackers.end(); it != end; ++it)
    {
        CUID uid = it->charUID;
        if (!uid)
            continue;

        const CChar* pUIDChar = uid.CharFind();
        if (!pUIDChar)
            continue;

        if (pUIDChar == pChar)
            return count;

        ++count;
    }
    return -1;
}

// Get nID value of attacker list from the given pChar
int CChar::Attacker_GetID(CUID pChar)
{
    ADDTOCALLSTACK("CChar::Attacker_GetID(CUID)");
    return Attacker_GetID(pChar.CharFind()->GetChar());
}

// Get UID value of attacker list from the given pChar
CChar * CChar::Attacker_GetUID(size_t attackerIndex)
{
    ADDTOCALLSTACK("CChar::Attacker_GetUID");
    if (m_lastAttackers.empty())
        return NULL;
    if (m_lastAttackers.size() <= attackerIndex)
        return NULL;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    CChar * pChar = CUID(refAttacker.charUID).CharFind();
    return pChar;
}

// Removing attacker pointed by iterator
bool CChar::Attacker_Delete(std::vector<LastAttackers>::iterator &itAttacker, bool bForced, ATTACKER_CLEAR_TYPE type)
{
    ADDTOCALLSTACK("CChar::Attacker_Delete(iterator)");
    if (m_lastAttackers.empty())
        return false;

    CChar *pChar = CUID(itAttacker->charUID).CharFind();
    if (!pChar)
        return false;

    if (IsTrigUsed(TRIGGER_COMBATDELETE))
    {
        CScriptTriggerArgs Args;
        Args.m_iN1 = bForced;
        Args.m_iN2 = (int)type;
        TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatDelete, pChar, &Args);
        if (tRet == TRIGRET_RET_TRUE)
            return false;
    }

    itAttacker = m_lastAttackers.erase(itAttacker);		// update the iterator to prevent invalidation and crashes!

    if (m_Fight_Targ_UID == pChar->GetUID())
    {
        m_Fight_Targ_UID.InitUID();
        if (m_pNPC)
            Fight_Attack(NPC_FightFindBestTarget());
    }
    if (m_lastAttackers.empty())
        Attacker_Clear();
    return true;
}

// Removing nID from list
bool CChar::Attacker_Delete(size_t attackerIndex, bool bForced, ATTACKER_CLEAR_TYPE type)
{
    ADDTOCALLSTACK("CChar::Attacker_Delete(size_t)");
    if (m_lastAttackers.empty() || (m_lastAttackers.size() <= attackerIndex))
        return false;

    std::vector<LastAttackers>::iterator it = m_lastAttackers.begin() + attackerIndex;
    return Attacker_Delete(it, bForced, type);
}

// Removing pChar from list
bool CChar::Attacker_Delete(CChar * pChar, bool bForced, ATTACKER_CLEAR_TYPE type)
{
    ADDTOCALLSTACK("CChar::Attacker_Delete(CChar)");
    if (!pChar || m_lastAttackers.empty())
        return false;
    return Attacker_Delete(Attacker_GetID(pChar), bForced, type);
}

// Removing everyone
void CChar::Attacker_RemoveChar()
{
    ADDTOCALLSTACK("CChar::Attacker_RemoveChar");
    if (!m_lastAttackers.empty())
    {
        for (auto it = m_lastAttackers.begin(), end = m_lastAttackers.end(); it != end; ++it)
        {
            LastAttackers & refAttacker = *it;
            CChar * pSrc = CUID(refAttacker.charUID).CharFind();
            if (!pSrc)
                continue;
            pSrc->Attacker_Delete(this, false, ATTACKER_CLEAR_REMOVEDCHAR);
        }
    }
}

// Checking if Elapsed > serv.AttackerTimeout
void CChar::Attacker_CheckTimeout()
{
    ADDTOCALLSTACK("CChar::Attacker_CheckTimeout");
    if (!m_lastAttackers.empty())
    {
        // do not iterate with an iterator here, since Attacker_Delete can invalidate both current and end iterators!
        for (size_t count = 0; count < m_lastAttackers.size(); )
        {
            LastAttackers & refAttacker = m_lastAttackers[count];
            CChar *pEnemy = CUID(refAttacker.charUID).CharFind();
            if (pEnemy && (++(refAttacker.elapsed) > g_Cfg.m_iAttackerTimeout) && (g_Cfg.m_iAttackerTimeout > 0))
            {
                DEBUG_MSG(("AttackerTimeout elapsed for char '%s' (UID: 0%" PRIx32 "): "
                    "deleted attacker '%s' (index: %d, UID: 0%" PRIx32 ")\n",
                    GetName(), GetUID().GetObjUID(), pEnemy->GetName(), count, pEnemy->GetUID().GetObjUID()));
                if (!Attacker_Delete(count, true, ATTACKER_CLEAR_ELAPSED))
                    ++count;
            }
            else
                ++count;
        }
    }
}
