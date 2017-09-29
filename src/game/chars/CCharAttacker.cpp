// Actions specific to an NPC.
#include "../../common/CUIDExtra.h"
#include "../clients/CClient.h"
#include "../CServerTime.h"
#include "../triggers.h"
#include "CChar.h"


// Add some enemy to my Attacker list
bool CChar::Attacker_Add( CChar * pChar, int64 threat )
{
	ADDTOCALLSTACK("CChar::Attacker_Add");
	CUID uid = pChar->GetUID();
	if ( m_lastAttackers.size() )	// Must only check for existing attackers if there are any attacker already.
	{
		for ( auto it = m_lastAttackers.begin(); it != m_lastAttackers.end(); ++it )
		{
			LastAttackers & refAttacker = *it;
			if ( refAttacker.charUID == uid )
				return true;	// found one, no actions needed so we skip
		}
	}
	else if ( IsTrigUsed(TRIGGER_COMBATSTART) )
	{
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatStart, pChar, 0);
		if ( tRet == TRIGRET_RET_TRUE )
			return false;
	}

	CScriptTriggerArgs Args;
	bool fIgnore = false;
	Args.m_iN1 = threat;
	Args.m_iN2 = fIgnore;
	if ( IsTrigUsed(TRIGGER_COMBATADD) )
	{
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatAdd, pChar, &Args);
		if ( tRet == TRIGRET_RET_TRUE )
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
	if ( !attacker.ignore )
	{
		//if ( GetTopSector()->GetCharComplexity() < 7 )
		//{
		sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_ATTACKO), GetName(), pChar->GetName());
		UpdateObjMessage(z, NULL, pChar->GetClient(), HUE_TEXT_DEF, TALKMODE_EMOTE);
		//}

		if ( pChar->IsClient() && pChar->CanSee(this) )
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
	if ( ! m_lastAttackers.size() )
		return -1;
	if (  m_lastAttackers.size() <= attackerIndex )
		return -1;
	LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);
	return refAttacker.amountDone;
}

// Retrieves the amount of time elapsed since the last hit to nID enemy
int64 CChar::Attacker_GetElapsed(size_t attackerIndex)
{
	ADDTOCALLSTACK("CChar::Attacker_GetElapsed");
	if ( ! m_lastAttackers.size() )
		return -1;
	if ( m_lastAttackers.size() <= attackerIndex )
		return -1;
	LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);
	return refAttacker.elapsed;
}

// Retrieves Threat value that nID enemy represents against me
int64 CChar::Attacker_GetThreat(size_t attackerIndex)
{
	ADDTOCALLSTACK("CChar::Attacker_GetThreat");
	if ( ! m_lastAttackers.size() )
		return -1;
	if ( m_lastAttackers.size() <= attackerIndex )
		return -1;
	LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);
	return refAttacker.threat ? refAttacker.threat : 0;
}

// Retrieves the character with most Threat
int64 CChar::Attacker_GetHighestThreat()
{
	if ( !m_lastAttackers.size() )
		return -1;
	int64 highThreat = 0;
	for ( size_t count = 0; count < m_lastAttackers.size(); count++ )
	{
		LastAttackers & refAttacker = m_lastAttackers.at(count);
		if ( refAttacker.threat > highThreat )
			highThreat = refAttacker.threat;
	}
	return highThreat;
}

// Retrieves the last character that I hit
CChar * CChar::Attacker_GetLast()
{
	int64 dwLastTime = INT32_MAX, dwCurTime = 0;
	CChar * retChar = NULL;
	for (size_t iAttacker = 0; iAttacker < m_lastAttackers.size(); ++iAttacker)
	{
		LastAttackers & refAttacker = m_lastAttackers.at(iAttacker);
		dwCurTime = refAttacker.elapsed;
		if (dwCurTime <= dwLastTime)
		{
			dwLastTime = dwCurTime;
			retChar = static_cast<CUID>(refAttacker.charUID).CharFind();
		}
	}
	return retChar;
}

// Set elapsed time (refreshing it?)
void CChar::Attacker_SetElapsed(CChar * pChar, int64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetElapsed(CChar)");
	return Attacker_SetElapsed( Attacker_GetID(pChar), value );
}


// Set elapsed time (refreshing it?)
void CChar::Attacker_SetElapsed(size_t attackerIndex, int64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetElapsed(size_t)");
	if ( ! m_lastAttackers.size() )
		return;
	if ( m_lastAttackers.size() <= attackerIndex )
		return;
	LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);
	refAttacker.elapsed = value;
}

// Damaged pChar
void CChar::Attacker_SetDam( CChar * pChar, int64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetDam(CChar)");
	return Attacker_SetDam( Attacker_GetID(pChar), value );
}


// Damaged pChar
void CChar::Attacker_SetDam(size_t attackerIndex, int64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetDam(size_t)");
	if ( ! m_lastAttackers.size() )
		return;
	if ( m_lastAttackers.size() <= attackerIndex)
		return;
	LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);
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
	if (!m_lastAttackers.size())
		return;
	if (m_lastAttackers.size() <= attackerIndex)
		return;
	LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);
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
	ADDTOCALLSTACK("CChar::Attacker_SetIgnore(int)");\
	if (!m_lastAttackers.size())
		return;
	if (m_lastAttackers.size() <= attackerIndex)
		return;
	LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);
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
	if (!m_lastAttackers.size())
		return false;
	if (m_lastAttackers.size() <= attackerIndex)
		return false;
	LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);
	return (refAttacker.ignore != 0);
}

// Clear the whole attacker's list, combat ended.
void CChar::Attacker_Clear()
{
	ADDTOCALLSTACK("CChar::Attacker_Clear");
	if ( IsTrigUsed(TRIGGER_COMBATEND) )
		OnTrigger(CTRIG_CombatEnd, this, 0);

	m_lastAttackers.clear();
    if (m_pNPC)
        StatFlag_Clear(STATF_War);
	if ( Fight_IsActive() )
	{
		Skill_Start(SKILL_NONE);
		m_Fight_Targ_UID.InitUID();
	}
	UpdateModeFlag();
}

// Get nID value of attacker list from the given pChar
int CChar::Attacker_GetID( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::Attacker_GetID(CChar)");
	if ( !pChar )
		return -1;
	if ( ! m_lastAttackers.size() )
		return -1;
	int count = 0;
	for ( auto it = m_lastAttackers.begin(); it != m_lastAttackers.end(); ++it )
	{
		LastAttackers & refAttacker = m_lastAttackers.at(count);
		CUID uid = refAttacker.charUID;
		if ( ! uid || !uid.CharFind() )
			continue;
		CChar * pMe = uid.CharFind()->GetChar();
		if ( ! pMe )
			continue;
		if ( pMe == pChar )
			return count;

		count++;
	}
	return -1;
}

// Get nID value of attacker list from the given pChar
int CChar::Attacker_GetID( CUID pChar )
{
	ADDTOCALLSTACK("CChar::Attacker_GetID(CUID)");
	return Attacker_GetID( pChar.CharFind()->GetChar() );
}

// Get UID value of attacker list from the given pChar
CChar * CChar::Attacker_GetUID(size_t attackerIndex)
{
	ADDTOCALLSTACK("CChar::Attacker_GetUID");
	if ( ! m_lastAttackers.size() )
		return NULL;
	if ( m_lastAttackers.size() <= attackerIndex)
		return NULL;
	LastAttackers & refAttacker = m_lastAttackers.at(attackerIndex);
	CChar * pChar = static_cast<CChar*>( static_cast<CUID>( refAttacker.charUID ).CharFind() );
	return pChar;
}

// Removing nID from list
bool CChar::Attacker_Delete(size_t attackerIndex, bool bForced, ATTACKER_CLEAR_TYPE type)
{
	ADDTOCALLSTACK("CChar::Attacker_Delete(size_t)");
	if (!m_lastAttackers.size() || (m_lastAttackers.size() <= attackerIndex))
		return false;
	LastAttackers &refAttacker = m_lastAttackers.at(attackerIndex);
	CChar *pChar = static_cast<CUID>(refAttacker.charUID).CharFind();
	if (!pChar)
		return false;

	if (IsTrigUsed(TRIGGER_COMBATDELETE))
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = bForced;
		Args.m_iN2 = static_cast<int>(type);
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatDelete, pChar, &Args);
		if (tRet == TRIGRET_RET_TRUE)
			return false;
	}
	std::vector<LastAttackers>::iterator it = m_lastAttackers.begin() + attackerIndex;

	m_lastAttackers.erase(it);
	if (m_Fight_Targ_UID == pChar->GetUID())
	{
		m_Fight_Targ_UID.InitUID();
		if (m_pNPC)
			Fight_Attack(NPC_FightFindBestTarget());
	}
	if (!m_lastAttackers.size())
		Attacker_Clear();
	return true;
}

// Removing pChar from list
bool CChar::Attacker_Delete(CChar * pChar, bool bForced, ATTACKER_CLEAR_TYPE type)
{		
	ADDTOCALLSTACK("CChar::Attacker_Delete(CChar)");
	if ( !pChar )
		return false;
	if ( ! m_lastAttackers.size() )
		return false;
	return Attacker_Delete( Attacker_GetID(pChar), bForced, type );
}

// Removing everyone
void CChar::Attacker_RemoveChar()
{
	ADDTOCALLSTACK("CChar::Attacker_RemoveChar");
	if ( m_lastAttackers.size() )
	{
		for ( size_t count = 0 ; count < m_lastAttackers.size(); count++)
		{
			LastAttackers & refAttacker = m_lastAttackers.at(count);
			CChar * pSrc = static_cast<CUID>(refAttacker.charUID).CharFind();
			if ( !pSrc )
				continue;
			pSrc->Attacker_Delete(pSrc->Attacker_GetID(this), false, ATTACKER_CLEAR_REMOVEDCHAR);
		}
	}
}

// Checking if Elapsed > serv.AttackerTimeout
void CChar::Attacker_CheckTimeout()
{
	ADDTOCALLSTACK("CChar::Attacker_CheckTimeout");
	if (m_lastAttackers.size() > 0)
	{
		for (size_t count = 0; count < m_lastAttackers.size(); count++)
		{
			LastAttackers & refAttacker = m_lastAttackers.at(count);
			CChar *pEnemy = static_cast<CUID>(refAttacker.charUID).CharFind();
			if ( pEnemy && ( ++(refAttacker.elapsed) > g_Cfg.m_iAttackerTimeout ) && (g_Cfg.m_iAttackerTimeout > 0) )
			{
				DEBUG_MSG(("AttackerTimeout elapsed for char '%s' (UID: 0%" PRIx32 "): "
					"deleted attacker '%s' (index: %d, UID: 0%" PRIx32 ")\n",
					GetName(), GetUID().GetObjUID(), pEnemy->GetName(), count, pEnemy->GetUID().GetObjUID() ));
				Attacker_Delete(count, true, ATTACKER_CLEAR_ELAPSED);
			}
				
		}
	}
}
