// Actions specific to an NPC.

#include "../game/CPathFinder.h"
#include "../network/receive.h"
#include "../CServTime.h"
#include "../Triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

//////////////////////////
// CChar

bool CChar::NPC_FightArchery( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_FightArchery");
	if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_RANGED) )
		return( false );

	int iMinDist = 0;
	int iMaxDist = 0;

	// determine how far we can shoot with this bow
	CItem *pWeapon = m_uidWeapon.ItemFind();
	if ( pWeapon != NULL )
	{
		iMinDist = pWeapon->RangeH();
		iMaxDist = pWeapon->RangeL();
	}

	// if range is not set on the weapon, default to ini settings
	if ( !iMaxDist || (iMinDist == 0 && iMaxDist == 1) )
		iMaxDist = g_Cfg.m_iArcheryMaxDist;
	if ( !iMinDist )
		iMinDist = g_Cfg.m_iArcheryMinDist;

	int iDist = GetTopDist3D( pChar );
	if ( iDist > iMaxDist )	// way too far away . close in.
		return( false );

	if ( iDist > iMinDist )
		return( true );		// always use archery if distant enough

	if ( !Calc_GetRandVal( 2 ) )	// move away
	{
		// Move away
		NPC_Act_Follow( false, iMaxDist, true );
		return( true );
	}

	// Fine here.
	return( true );
}

CChar * CChar::NPC_FightFindBestTarget()
{
	ADDTOCALLSTACK("CChar::NPC_FightFindBestTarget");
	// Find the best target to attack, and switch to this
	// new target even if I'm already attacking someone.
	if ( !m_pNPC )
		return NULL;

	if ( Attacker() )
	{
		if ( !m_lastAttackers.size() )
			return NULL;

		int64 threat = 0;
		int iClosest = INT32_MAX;
		CChar *pChar = NULL;
		CChar *pClosest = NULL;
		SKILL_TYPE skillWeapon = Fight_GetWeaponSkill();

		for ( std::vector<LastAttackers>::iterator it = m_lastAttackers.begin(); it != m_lastAttackers.end(); ++it )
		{
			LastAttackers &refAttacker = *it;
			pChar = static_cast<CChar*>(static_cast<CGrayUID>(refAttacker.charUID).CharFind());
			if ( !pChar )
				continue;
			if ( pChar->IsStatFlag(STATF_DEAD|STATF_Stone|STATF_Invisible|STATF_Insubstantial|STATF_Hidden) )
			{
				pChar = NULL;
				continue;
			}
			if ( refAttacker.ignore )
			{
				bool bIgnore = true;
				if ( IsTrigUsed(TRIGGER_HITIGNORE) )
				{
					CScriptTriggerArgs Args;
					Args.m_iN1 = bIgnore;
					OnTrigger(CTRIG_HitIgnore, pChar, &Args);
					bIgnore = Args.m_iN1 ? true : false;
				}
				if ( bIgnore )
				{
					pChar = NULL;
					continue;
				}
			}
			if ( !pClosest )
				pClosest = pChar;

			int iDist = GetDist(pChar);
			if ( iDist > UO_MAP_VIEW_SIGHT )
				continue;
			if ( g_Cfg.IsSkillFlag(skillWeapon, SKF_RANGED) && (iDist < g_Cfg.m_iArcheryMinDist || iDist > g_Cfg.m_iArcheryMaxDist) )
				continue;
			if ( !CanSeeLOS(pChar) )
				continue;

			if ( (NPC_GetAiFlags() & NPC_AI_THREAT) && (threat < refAttacker.threat) )	// this char has more threat than others, let's switch to this target
			{
				pClosest = pChar;
				iClosest = iDist;
				threat = refAttacker.threat;
			}
			else if ( iDist < iClosest )	// this char is more closer to me than my current target, let's switch to this target
			{
				pClosest = pChar;
				iClosest = iDist;
			}
		}
		return pClosest ? pClosest : pChar;
	}

	// New target not found, check if I can keep attacking my current target
	CChar *pTarget = m_Fight_Targ.CharFind();
	if ( pTarget )
		return pTarget;

	return NULL;
}

void CChar::NPC_Act_Fight()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Fight");
	// I am in an attack mode.
	if ( !m_pNPC || !Fight_IsActive() )
		return;

	// Review our targets periodically.
	if ( ! IsStatFlag(STATF_Pet) ||
		m_pNPC->m_Brain == NPCBRAIN_BERSERK )
	{
		int iObservant = ( 130 - Stat_GetAdjusted(STAT_INT)) / 20;
		if ( ! Calc_GetRandVal( 2 + maximum( 0, iObservant )))
		{
			if ( NPC_LookAround())
				return;
		}
	}

	CChar * pChar = m_Fight_Targ.CharFind();
	if (pChar == NULL || !pChar->IsTopLevel()) // target is not valid anymore ?
		return;

	if (Attacker_GetIgnore(pChar))
	{
		if (!NPC_FightFindBestTarget())
		{
			Skill_Start(SKILL_NONE);
			StatFlag_Clear(STATF_War);
			m_Fight_Targ.InitUID();
			return;
		}
	}
	int iDist = GetDist( pChar );

	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD &&
		m_atFight.m_War_Swing_State == WAR_SWING_READY &&
		! Calc_GetRandVal(3))
	{
		// If a guard is ever too far away (missed a chance to swing)
		// Teleport me closer.
		NPC_LookAtChar( pChar, iDist );
	}


	// If i'm horribly damaged and smart enough to know it.
	int iMotivation = NPC_GetAttackMotivation( pChar );

	bool		fSkipHardcoded	= false;
	if ( IsTrigUsed(TRIGGER_NPCACTFIGHT) )
	{
		CGrayUID m_oldAct = m_Act_Targ;
		CScriptTriggerArgs Args( iDist, iMotivation );
		switch ( OnTrigger( CTRIG_NPCActFight, pChar, &Args ) )
		{
			case TRIGRET_RET_TRUE:	return;
			case TRIGRET_RET_FALSE:	fSkipHardcoded	= true;	break;
			case static_cast<TRIGRET_TYPE>(2):
			{
				SKILL_TYPE iSkillforced = static_cast<SKILL_TYPE>(Args.m_VarsLocal.GetKeyNum("skill", false));
				if (iSkillforced)
				{
					SPELL_TYPE iSpellforced = static_cast<SPELL_TYPE>(Args.m_VarsLocal.GetKeyNum("spell", false));
					if (g_Cfg.IsSkillFlag(iSkillforced, SKF_MAGIC))
						m_atMagery.m_Spell = iSpellforced;

					Skill_Start(iSkillforced);
					return;
				}
			}
			default:				break;
		}

		iDist		= static_cast<int>(Args.m_iN1);
		iMotivation = static_cast<int>(Args.m_iN2);
	}

	if ( ! IsStatFlag(STATF_Pet))
	{
		if ( iMotivation < 0 )
		{
			m_atFlee.m_iStepsMax = 20;	// how long should it take to get there.
			m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
			Skill_Start( NPCACT_FLEE );	// Run away!
			return;
		}
	}



	// Can only do that with full stamina !
	if ( !fSkipHardcoded && Stat_GetVal(STAT_DEX) >= Stat_GetAdjusted(STAT_DEX))
	{
		// If I am a dragon maybe I will breath fire.
		// NPCACT_BREATH
		if ( m_pNPC->m_Brain == NPCBRAIN_DRAGON &&
			iDist >= 1 &&
			iDist <= 8 &&
			CanSeeLOS( pChar,LOS_NB_WINDOWS )) //Dragon can breath through a window
		{
			if (!IsSetCombatFlags(COMBAT_NODIRCHANGE))
				UpdateDir( pChar );
			Skill_Start( NPCACT_BREATH );
			return;
		}


		// any special ammunition defined?

		//check Range
		int iRangeMin = 2;
		int iRangeMax = 9;
		CVarDefCont * pRange = GetDefKey("THROWRANGE",true);
		if (pRange)
		{
			int64 RVal[2];
			size_t iQty = Str_ParseCmds( const_cast<tchar*>(pRange->GetValStr()), RVal, COUNTOF(RVal));
			switch(iQty)
			{
				case 1:
					iRangeMax = static_cast<int>(RVal[0]);
					break;
				case 2:
					iRangeMin = static_cast<int>(RVal[0]);
					iRangeMax = static_cast<int>(RVal[1]);
					break;
			}
		}
		if (iDist >= iRangeMin && iDist <= iRangeMax && CanSeeLOS( pChar,LOS_NB_WINDOWS ))//NPCs can throw through a window
		{
			CVarDefCont * pRock = GetDefKey("THROWOBJ",true);
			if ( GetDispID() == CREID_OGRE || GetDispID() == CREID_ETTIN || GetDispID() == CREID_Cyclops || pRock )
			{
				ITEMID_TYPE id = ITEMID_NOTHING;
				if (pRock)
				{
					lpctstr t_Str = pRock->GetValStr();
					RESOURCE_ID_BASE rid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID( RES_ITEMDEF, t_Str ));
					ITEMID_TYPE obj = static_cast<ITEMID_TYPE>(rid.GetResIndex());
					if ( ContentFind( RESOURCE_ID(RES_ITEMDEF,obj), 0, 2 ) )
						id = ITEMID_NODRAW;
				}
				else 
				{
					if ( ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_AROCK), 0, 2 ) )
						id = ITEMID_NODRAW;
				}


				if ( id != ITEMID_NOTHING )
				{
					if (!IsSetCombatFlags(COMBAT_NODIRCHANGE))
						UpdateDir( pChar );
					Skill_Start( NPCACT_THROWING );
					return;
				}
			}
		}
	}

	// Maybe i'll cast a spell if I can. if so maintain a distance.
	if ( NPC_FightMagery( pChar ))
		return;

	if ( NPC_FightArchery( pChar ))
		return;

	// Move in for melee type combat.
	NPC_Act_Follow( false, CalcFightRange( m_uidWeapon.ItemFind() ), false );
}