// Actions specific to an NPC.
#include "../../common/CLog.h"
#include "../../common/CScriptTriggerArgs.h"
#include "../../network/receive.h"
#include "../components/CCPropsItemWeapon.h"
#include "../CPathFinder.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

//////////////////////////
// CChar

bool CChar::NPC_FightArchery(CChar * pChar)
{
    ADDTOCALLSTACK("CChar::NPC_FightArchery");
    ASSERT(m_pNPC);

    if (!g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_RANGED))
        return false;

    int iMinDist = 0;
    int iMaxDist = 0;

    // determine how far we can shoot with this bow
    CItem *pWeapon = m_uidWeapon.ItemFind();
    if (pWeapon != nullptr)
    {
        iMinDist = GetRangeL();
        iMaxDist = GetRangeH();
    }

    // if range is not set on the weapon, default to ini settings
    if (!iMaxDist || (iMinDist == 0 && iMaxDist == 1))
        iMaxDist = g_Cfg.m_iArcheryMaxDist;
    if (!iMinDist)
        iMinDist = g_Cfg.m_iArcheryMinDist;

    int iDist = GetTopDist3D(pChar);
    if (iDist > iMaxDist)	// way too far away . close in.
        return false;

    if (iDist > iMinDist)
        return true;		// always use archery if distant enough

    if (!Calc_GetRandVal(2))	// move away
    {
        // Move away
        NPC_Act_Follow(false, iMaxDist, true);
        return true;
    }

    // Fine here.
    return true;
}

CChar * CChar::NPC_FightFindBestTarget()
{
    ADDTOCALLSTACK("CChar::NPC_FightFindBestTarget");
    ASSERT(m_pNPC);
    // Find the best target to attack, and switch to this
    // new target even if I'm already attacking someone.

    if (GetAttackersCount())
    {
        int64 threat = 0;
        int iClosest = INT32_MAX;
        CChar *pChar = nullptr;
        CChar *pClosest = nullptr;
        SKILL_TYPE skillWeapon = Fight_GetWeaponSkill();

        // Do NOT use iterators here, since in this loop the m_lastAttackers vector can be altered, and so the iterator, making it invalid
        //for (std::vector<LastAttackers>::iterator it = m_lastAttackers.begin(); it != m_lastAttackers.end(); ++it)
        for (size_t i = 0; i < m_lastAttackers.size(); )
        {
            LastAttackers &refAttacker = m_lastAttackers[i];
            pChar = CUID(refAttacker.charUID).CharFind();
            if (!pChar)
            {
                ++i;
                continue;
            }
            if (!pChar->Fight_IsAttackable())
            {
                ++i;
                continue;
            }
            if (refAttacker.ignore)
            {
                bool bIgnore = true;
                if (IsTrigUsed(TRIGGER_HITIGNORE))
                {
                    CScriptTriggerArgs Args;
                    Args.m_iN1 = bIgnore;
                    OnTrigger(CTRIG_HitIgnore, pChar, &Args);
                    bIgnore = Args.m_iN1 ? true : false;
                }
                if (bIgnore)
                {
                    ++i;
                    continue;
                }
            }
            if (!pClosest)
                pClosest = pChar;

            int iDist = GetDist(pChar);
            /*if (iDist > GetVisualRange())     // does this cause a deadlock sometimes?
            {
                Attacker_Delete(i, false, ATTACKER_CLEAR_DISTANCE);
                //++i; // Do NOT increment here!
                if (m_lastAttackers.empty())
                    break;
                continue;
            } */
            if (g_Cfg.IsSkillFlag(skillWeapon, SKF_RANGED) && ((iDist < g_Cfg.m_iArcheryMinDist) || (iDist > g_Cfg.m_iArcheryMaxDist)))
            {
                ++i;
                continue;
            }
            if (!CanSeeLOS(pChar))
            {
                ++i;
                continue;
            }

            if ((NPC_GetAiFlags() & NPC_AI_THREAT) && (threat < refAttacker.threat))	// this char has more threat than others, let's switch to this target
            {
                pClosest = pChar;
                iClosest = iDist;
                threat = refAttacker.threat;
            }
            else if (iDist < iClosest)	// this char is more closer to me than my current target, let's switch to this target
            {
                pClosest = pChar;
                iClosest = iDist;
            }
            ++i;
        }
        return pClosest ? pClosest : pChar;
    }

    // New target not found, check if I can keep attacking my current target
    CChar *pTarget = m_Fight_Targ_UID.CharFind();
    if (pTarget)
        return pTarget;

    return nullptr;
}

void CChar::NPC_Act_Fight()
{
    ADDTOCALLSTACK("CChar::NPC_Act_Fight");
    ASSERT(m_pNPC);

    // I am in an attack mode.
    if (!Fight_IsActive())
    {
        Fight_ClearAll();
        return;
    }

    // Review our targets periodically.
    if (!IsStatFlag(STATF_PET) || (m_pNPC->m_Brain == NPCBRAIN_BERSERK))
    {
        int iObservant = (130 - Stat_GetAdjusted(STAT_INT)) / 20;
        if (!Calc_GetRandVal(2 + maximum(0, iObservant)))
        {
            if (NPC_LookAround())
            {
                _SetTimeoutD(5); // half of a second until the next check
                return;
            }
        }
    }

    CChar * pChar = m_Fight_Targ_UID.CharFind();
    if (pChar == nullptr || !pChar->IsTopLevel()) // target is not valid anymore ?
        return;

    if (Attacker_GetIgnore(pChar))
    {
        if (!NPC_FightFindBestTarget())
        {
            Skill_Start(SKILL_NONE);
            StatFlag_Clear(STATF_WAR);
            m_Fight_Targ_UID.InitUID();
            return;
        }
    }
    int iDist = GetDist(pChar);

    if ((m_pNPC->m_Brain == NPCBRAIN_GUARD) &&
        (m_atFight.m_iWarSwingState == WAR_SWING_READY) &&
        !Calc_GetRandVal(3))
    {
        // If a guard is ever too far away (missed a chance to swing)
        // Teleport me closer.
        NPC_LookAtChar(pChar, iDist);
    }

    // If i'm horribly damaged and smart enough to know it.
    int iMotivation = NPC_GetAttackMotivation(pChar);

    bool fSkipHardcoded = false;
    if (IsTrigUsed(TRIGGER_NPCACTFIGHT))
    {
        CScriptTriggerArgs Args(iDist, iMotivation);
        switch (OnTrigger(CTRIG_NPCActFight, pChar, &Args))
        {
        case TRIGRET_RET_TRUE:
            return;
        case TRIGRET_RET_FALSE:
            fSkipHardcoded = true;
            break;
        case (TRIGRET_TYPE)(2) :
        {
            SKILL_TYPE iSkillforced = (SKILL_TYPE)RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("skill"));
            if (iSkillforced)
            {
                SPELL_TYPE iSpellforced = (SPELL_TYPE)RES_GET_INDEX(Args.m_VarsLocal.GetKeyNum("spell"));
                if (g_Cfg.IsSkillFlag(iSkillforced, SKF_MAGIC))
                {
                    m_atMagery.m_iSpell = iSpellforced;
                    m_Act_UID = m_Fight_Targ_UID; // Setting the spell's target.
                }

                Skill_Start(iSkillforced);
                return;
            }
        }
        default:
            break;
        }

        iDist = (int)(Args.m_iN1);
        iMotivation = (int)(Args.m_iN2);
    }

    if (!IsStatFlag(STATF_PET))
    {
        if (iMotivation < 0)
        {
            m_atFlee.m_iStepsMax = 20;	// how long should it take to get there.
            m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
            Skill_Start(NPCACT_FLEE);	// Run away!
            return;
        }
    }

    // Can only do that with full stamina !
    if (!fSkipHardcoded && (Stat_GetVal(STAT_DEX) >= Stat_GetAdjusted(STAT_DEX)))
    {
        // If I am a dragon maybe I will breath fire.
        // NPCACT_BREATH
        if (m_pNPC->m_Brain == NPCBRAIN_DRAGON &&
            iDist >= 1 &&
            iDist <= 8 &&
            CanSeeLOS(pChar, LOS_NB_WINDOWS)) //Dragon can breath through a window
        {
            if (!IsSetCombatFlags(COMBAT_NODIRCHANGE))
                UpdateDir(pChar);
            Skill_Start(NPCACT_BREATH);
            return;
        }

        // any special ammunition defined?

        //check Range
        int iRangeMin = 2;
        int iRangeMax = 9;
        const CVarDefCont * pRange = GetDefKey("THROWRANGE", true);
        if (pRange)
        {
            int iRangeTot = CBaseBaseDef::ConvertRangeStr(pRange->GetValStr());
            iRangeMin = RANGE_GET_LO(iRangeTot);
            iRangeMax = RANGE_GET_HI(iRangeTot);
        }

        if (iDist >= iRangeMin && iDist <= iRangeMax && CanSeeLOS(pChar, LOS_NB_WINDOWS))//NPCs can throw through a window
        {
            const CVarDefCont * pRock = GetDefKey("THROWOBJ", true);
            const CREID_TYPE iDispID = GetDispID();
            if (iDispID == CREID_OGRE || iDispID == CREID_ETTIN || iDispID == CREID_CYCLOPS || pRock)
            {
                ITEMID_TYPE id = ITEMID_NOTHING;
                if (pRock)
                {
                    lpctstr t_Str = pRock->GetValStr();
                    CResourceID rid = g_Cfg.ResourceGetID(RES_ITEMDEF, t_Str);
                    ITEMID_TYPE obj = (ITEMID_TYPE)(rid.GetResIndex());
                    if (ContentFind(CResourceID(RES_ITEMDEF, obj), 0, 2))
                        id = ITEMID_NODRAW;
                }
                else
                {
                    if (ContentFind(CResourceID(RES_TYPEDEF, IT_AROCK), 0, 2))
                        id = ITEMID_NODRAW;
                }


                if (id != ITEMID_NOTHING)
                {
                    if (!IsSetCombatFlags(COMBAT_NODIRCHANGE))
                        UpdateDir(pChar);
                    Skill_Start(NPCACT_THROWING);
                    return;
                }
            }
        }
    }

    // Maybe i'll cast a spell if I can. if so maintain a distance.
    if (NPC_FightMagery(pChar))
    {
        return;
    }

    if (NPC_FightArchery(pChar))
    {
        return;
    }

    // Move in for melee type combat.
    int iRange = Fight_CalcRange(m_uidWeapon.ItemFind());
    if (!NPC_Act_Follow(false, iRange, false))
    {
        // Enemy gone?
        m_Act_UID.InitUID();
        _SetTimeoutD(1);
        return;
    }
    if (!_IsTimerSet()) // Nothing could be done, tick again in a while
    {
        NPC_LookAround();
        if (!_IsTimerSet())
        {
            DEBUG_MSG(("%s [0x%04x] found nothing to do in the fight routines.\n", GetName(), (dword)GetUID()));
            _SetTimeoutS(1);
        }
    }
}
