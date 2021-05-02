// Actions specific to an NPC.
#include "../clients/CClient.h"
#include "../triggers.h"
#include "CChar.h"


// Add some enemy to my Attacker list
bool CChar::Attacker_Add(CChar * pChar, int threat)
{
    ADDTOCALLSTACK("CChar::Attacker_Add");
    const dword uid = pChar->GetUID().GetObjUID();
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
        threat = (int)Args.m_iN1;
        fIgnore = (Args.m_iN2 != 0);
    }

    LastAttackers attacker;
    attacker.amountDone = 0;
    attacker.charUID = uid;
    attacker.elapsed = 0;
    attacker.threat = (m_pPlayer) ? 0 : threat;
    attacker.ignore = fIgnore;
    m_lastAttackers.emplace_back(std::move(attacker));

    // Record the start of the fight.
    Memory_Fight_Start(pChar);
    if (!attacker.ignore)
    {
        tchar *z = Str_GetTemp();
        CClient *pClient = pChar->GetClientActive();
        //if ( GetTopSector()->GetCharComplexity() < 7 )
        //{
        if (!( g_Cfg.m_iEmoteFlags & EMOTEF_ATTACKER ))
        {
    	  	HUE_TYPE emoteHue;
	       	if (m_EmoteHueOverride != 0) //Set EMOTECOLOROVERRIDE to ATTACKERS
       			emoteHue = m_EmoteHueOverride;
            else
                emoteHue = (HUE_TYPE)(g_Exp.m_VarDefs.GetKeyNum("EMOTE_DEF_COLOR"));
	        snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_ATTACKO), GetName(), pChar->GetName());
    	    UpdateObjMessage(z, nullptr, pClient, emoteHue, TALKMODE_EMOTE);
		}
        //}

        if (pClient && pChar->CanSee(this))
        {
            snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_ATTACKS), GetName());
            pClient->addBarkParse(z, this, HUE_TEXT_DEF, TALKMODE_EMOTE);
        }
    }
    return true;
}

// Retrieves damage done to nID enemy
int CChar::Attacker_GetDam(int attackerIndex) const
{
    ADDTOCALLSTACK("CChar::Attacker_GetDam");
    if (m_lastAttackers.empty())
        return -1;
    if ((attackerIndex < 0) || (m_lastAttackers.size() <= (size_t)attackerIndex))
        return -1;
    const LastAttackers & refAttacker = m_lastAttackers[(size_t)attackerIndex];
    return refAttacker.amountDone;
}

// Retrieves the amount of time elapsed since the last hit to nID enemy
int64 CChar::Attacker_GetElapsed(int attackerIndex) const
{
    ADDTOCALLSTACK("CChar::Attacker_GetElapsed");
    if (m_lastAttackers.empty())
        return -1;
    if ((attackerIndex < 0) || (m_lastAttackers.size() <= (size_t)attackerIndex))
        return -1;
    const LastAttackers & refAttacker = m_lastAttackers[(size_t)attackerIndex];
    return refAttacker.elapsed;
}

// Retrieves Threat value that nID enemy represents against me
int CChar::Attacker_GetThreat(int attackerIndex) const
{
    ADDTOCALLSTACK("CChar::Attacker_GetThreat");
    if (m_lastAttackers.empty())
        return -1;
    if ((attackerIndex < 0) || (m_lastAttackers.size() <= (size_t)attackerIndex))
        return -1;
    const LastAttackers & refAttacker = m_lastAttackers[(size_t)attackerIndex];
    return (refAttacker.threat > 0) ? refAttacker.threat : 0;
}

// Retrieves the character with most Threat
int CChar::Attacker_GetHighestThreat() const
{
    ADDTOCALLSTACK("CChar::Attacker_GetHighestThreat");
    if (m_lastAttackers.empty())
        return -1;

    int highThreat = 0;
    for (const LastAttackers & refAttacker : m_lastAttackers)
    {
        if (refAttacker.threat > highThreat)
            highThreat = refAttacker.threat;
    }
    return highThreat;
}

// Retrieves the last character that I hit
CChar * CChar::Attacker_GetLast() const
{
    ADDTOCALLSTACK("CChar::Attacker_GetLast");
    int64 iLastTime = INT64_MAX, iCurTime = 0;
    CChar * retChar = nullptr;
    for (const LastAttackers & refAttacker : m_lastAttackers)
    {
        iCurTime = refAttacker.elapsed;
        if (iCurTime <= iLastTime)
        {
            iLastTime = iCurTime;
            retChar = CUID::CharFindFromUID(refAttacker.charUID);
        }
    }
    return retChar;
}

// Set elapsed time (refreshing it?)
void CChar::Attacker_SetElapsed(const CChar * pChar, int64 value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetElapsed(CChar)");
    return Attacker_SetElapsed(Attacker_GetID(pChar), value);
}

// Set elapsed time (refreshing it?)
void CChar::Attacker_SetElapsed(int attackerIndex, int64 value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetElapsed(idx)");
    if (m_lastAttackers.empty())
        return;
    if ((attackerIndex < 0) || (m_lastAttackers.size() <= (size_t)attackerIndex))
        return;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    refAttacker.elapsed = value;
}

// Damaged pChar
void CChar::Attacker_SetDam(const CChar * pChar, int value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetDam(CChar)");
    return Attacker_SetDam(Attacker_GetID(pChar), value);
}

// Damaged pChar
void CChar::Attacker_SetDam(int attackerIndex, int value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetDam(idx)");
    if (m_lastAttackers.empty())
        return;
    if ((attackerIndex < 0) || (m_lastAttackers.size() <= (size_t)attackerIndex))
        return;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    refAttacker.amountDone = value;
}

// New Treat level
void CChar::Attacker_SetThreat(const CChar * pChar, int value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetThreat(CChar)");
    return Attacker_SetThreat(Attacker_GetID(pChar), value);
}

// New Treat level
void CChar::Attacker_SetThreat(int attackerIndex, int value)
{
    ADDTOCALLSTACK("CChar::Attacker_SetThreat(idx)");
    if (m_pPlayer)
        return;
    if (m_lastAttackers.empty())
        return;
    if ((attackerIndex < 0) || (m_lastAttackers.size() <= (size_t)attackerIndex))
        return;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    refAttacker.threat = value;
}

// Ignoring this pChar on Hit checks
void CChar::Attacker_SetIgnore(const CChar * pChar, bool fIgnore)
{
    ADDTOCALLSTACK("CChar::Attacker_SetIgnore(CChar)");
    return Attacker_SetIgnore(Attacker_GetID(pChar), fIgnore);
}

// Ignoring this pChar on Hit checks
void CChar::Attacker_SetIgnore(int attackerIndex, bool fIgnore)
{
    ADDTOCALLSTACK("CChar::Attacker_SetIgnore(idx)");
    if (m_lastAttackers.empty())
        return;
    if (m_lastAttackers.size() <= (size_t)attackerIndex)
        return;
    LastAttackers & refAttacker = m_lastAttackers[attackerIndex];
    refAttacker.ignore = fIgnore;
}

// I'm ignoring pChar?
bool CChar::Attacker_GetIgnore(const CChar * pChar) const
{
    ADDTOCALLSTACK("CChar::Attacker_GetIgnore(CChar)");
    return Attacker_GetIgnore(Attacker_GetID(pChar));
}

// I'm ignoring pChar?
bool CChar::Attacker_GetIgnore(int attackerIndex) const
{
    ADDTOCALLSTACK("CChar::Attacker_GetIgnore(idx)");
    if (m_lastAttackers.empty())
        return false;
    if ((attackerIndex < 0) || (m_lastAttackers.size() <= (size_t)attackerIndex))
        return false;
    const LastAttackers & refAttacker = m_lastAttackers[(size_t)attackerIndex];
    return (refAttacker.ignore != 0);
}

// Clear the whole attackers list: forget who attacked me, but if i'm fighting against someone don't stop me.
void CChar::Attacker_Clear()
{
    ADDTOCALLSTACK("CChar::Attacker_Clear");
    if (IsTrigUsed(TRIGGER_COMBATEND))
    {
        if (!Fight_IsActive() || !m_Fight_Targ_UID.IsValidUID() || !m_Fight_Targ_UID.CharFind())
        {
            OnTrigger(CTRIG_CombatEnd, this, 0);
        }
    }

    m_lastAttackers.clear();
    UpdateModeFlag();
}

// Get nID value of attacker list from the given pChar
int CChar::Attacker_GetID(const CChar * pChar) const
{
    ADDTOCALLSTACK("CChar::Attacker_GetID(CChar)");
    if (!pChar)
        return -1;
    if (m_lastAttackers.empty())
        return -1;
    int count = 0;
    for (std::vector<LastAttackers>::const_iterator it = m_lastAttackers.begin(), end = m_lastAttackers.end(); it != end; ++it)
    {
        const CUID uid(it->charUID);
        if (!uid.IsValidUID())
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

// Get nID value of attacker list from the given uidChar
int CChar::Attacker_GetID(const CUID& uidChar) const
{
    ADDTOCALLSTACK("CChar::Attacker_GetID(CUID)");
    return Attacker_GetID(uidChar.CharFind());
}

// Get CChar* from attacker list at the given attackerIndex
CChar * CChar::Attacker_GetUID(int attackerIndex) const
{
    ADDTOCALLSTACK("CChar::Attacker_GetUID");
    if (m_lastAttackers.empty())
        return nullptr;
    if ((attackerIndex < 0) || (m_lastAttackers.size() <= (size_t)attackerIndex))
        return nullptr;
    const LastAttackers & refAttacker = m_lastAttackers[(size_t)attackerIndex];
    CChar * pChar = CUID::CharFindFromUID(refAttacker.charUID);
    return pChar;
}

// Removing attacker pointed by iterator
bool CChar::Attacker_Delete(std::vector<LastAttackers>::iterator &itAttacker, bool fForced, ATTACKER_CLEAR_TYPE type)
{
    ADDTOCALLSTACK("CChar::Attacker_Delete(iterator)");
    if (m_lastAttackers.empty())
        return false;

    CChar *pChar = CUID::CharFindFromUID(itAttacker->charUID);
    if (pChar)
    {
        if (IsTrigUsed(TRIGGER_COMBATDELETE))
        {
            CScriptTriggerArgs Args;
            Args.m_iN1 = fForced;
            Args.m_iN2 = (int)type;
            TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatDelete, pChar, &Args);
            if ((tRet == TRIGRET_RET_TRUE) && !fForced)
                return false;
        }
    }

    itAttacker = m_lastAttackers.erase(itAttacker);		// update the iterator to prevent invalidation and crashes!

    if (pChar && (m_Fight_Targ_UID == pChar->GetUID()))
    {
        if (m_pNPC)
        {
            m_Fight_Targ_UID.InitUID();
            Fight_Attack(NPC_FightFindBestTarget());
        }
    }
    if (m_lastAttackers.empty())
        Attacker_Clear();
    return true;
}

// Removing nID from list
bool CChar::Attacker_Delete(int attackerIndex, bool fForced, ATTACKER_CLEAR_TYPE type)
{
    ADDTOCALLSTACK("CChar::Attacker_Delete(size_t)");
    if (m_lastAttackers.empty())
        return false;
    if ((attackerIndex < 0) || (m_lastAttackers.size() <= (size_t)attackerIndex))
        return false;

    std::vector<LastAttackers>::iterator it = m_lastAttackers.begin() + attackerIndex;
    return Attacker_Delete(it, fForced, type);
}

// Removing pChar from list
bool CChar::Attacker_Delete(const CChar * pChar, bool fForced, ATTACKER_CLEAR_TYPE type)
{
    ADDTOCALLSTACK("CChar::Attacker_Delete(CChar)");
    if (!pChar || m_lastAttackers.empty())
        return false;
    return Attacker_Delete(Attacker_GetID(pChar), fForced, type);
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
            CChar * pSrc = CUID::CharFindFromUID(refAttacker.charUID);
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
        for (int count = 0; count < (int)m_lastAttackers.size(); )
        {
            LastAttackers & refAttacker = m_lastAttackers[count];
            const CChar *pEnemy = CUID::CharFindFromUID(refAttacker.charUID);
            if (pEnemy)
            {
                // always advance refAttacker.elapsed, i might use it in scripts for a different purpose
                ++refAttacker.elapsed;
                if ( (refAttacker.elapsed > g_Cfg.m_iAttackerTimeout) && (g_Cfg.m_iAttackerTimeout > 0))
                {
                    if (!Attacker_Delete(count, false, ATTACKER_CLEAR_ELAPSED))
                        ++count;
                    continue;
                }
                ++count;
            }
            else
            {
                const bool fCleanupSuccess = Attacker_Delete(count, true, ATTACKER_CLEAR_FORCED);
                ASSERT(fCleanupSuccess);
                UNREFERENCED_PARAMETER(fCleanupSuccess);
                //if (!fCleanupSuccess)
                //    ++count;
            }
        }
    }
}
