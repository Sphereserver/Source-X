#include "../../network/send.h"
#include "../clients/CClient.h"
#include "../components/CCPropsChar.h"
#include "../components/CCPropsItemWeapon.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharNPC.h"


// Attacking an item
// Item must be CAN_I_DAMAGEABLE
// Introduced in new (SA) clients.
bool CChar::Fight_Attack(CItem* pItemTarg, bool fToldByMaster)
{
	ADDTOCALLSTACK("CChar::Fight_Attack");

	if (!pItemTarg || IsStatFlag(STATF_DEAD))
	{
		Fight_Clear(pItemTarg, true);
		return false;
	}
	else if (!CanSee(pItemTarg))
	{
		Skill_Start(SKILL_NONE);
		return false;
	}

	// FIXME
	// CTRIG_Attack
	// Missing Trigger

	if (!IsStatFlag(STATF_WAR))
	{
		StatFlag_Clear(STATF_WAR);
		UpdateModeFlag();
		if (IsClient())
			GetClient()->addPlayerWarMode();
	}

	const SKILL_TYPE skillWeapon = Fight_GetWeaponSkill();
	const SKILL_TYPE skillActive = Skill_GetActive();

	m_Fight_Targ_UID = pItemTarg ? pItemTarg->GetUID() : CUID();
	Skill_Start(skillWeapon);
	return true;
}


// Clearing a fight where my target was an item
// Is not a "fight" but we must clear our vars.
bool CChar::Fight_Clear(CItem* pItem, bool fForced)
{
	ADDTOCALLSTACK("CChar::Fight_Clear");
	if (!pItem)
		return false;

	m_atFight.m_iWarSwingState = WAR_SWING_EQUIPPING;
	m_atFight.m_iRecoilDelay = 0;
	m_atFight.m_iSwingAnimationDelay = 0;
	m_atFight.m_iSwingAnimation = 0;
	m_atFight.m_iSwingIgnoreLastHitTag = 0;

	if (m_Fight_Targ_UID == pItem->GetUID())
		m_Fight_Targ_UID.InitUID();

	return (pItem != nullptr);
}

WAR_SWING_TYPE CChar::Fight_Hit(CItem* pItemTarg)
							{
	ADDTOCALLSTACK("CChar::Fight_Hit");

	if (!pItemTarg)
		return WAR_SWING_INVALID;

	CItem* pWeapon = m_uidWeapon.ItemFind();
	DAMAGE_TYPE iDmgType = Fight_GetWeaponDamType(pWeapon);
	bool fSwingNoRange = (bool)(IsSetCombatFlags(COMBAT_SWING_NORANGE));

	WAR_SWING_TYPE iHitCheck = Fight_CanHit(pItemTarg, fSwingNoRange);
	if (iHitCheck != WAR_SWING_READY)
		return iHitCheck;

	const WAR_SWING_TYPE iStageToSuspend = (IsSetCombatFlags(COMBAT_PREHIT) ? WAR_SWING_SWINGING : WAR_SWING_EQUIPPING);
	if (IsSetCombatFlags(COMBAT_FIRSTHIT_INSTANT) && (!m_atFight.m_iSwingIgnoreLastHitTag) && (m_atFight.m_iWarSwingState == iStageToSuspend))
	{
		const int64 iTimeDiff = ((CWorldGameTime::GetCurrentTime().GetTimeRaw() / MSECS_PER_TENTH) - GetKeyNum("LastHit"));
		if (iTimeDiff < 0)
		{
			return iStageToSuspend;
		}
	}

	CItem* pAmmo = nullptr;
	const SKILL_TYPE skill = Skill_GetActive();
	const int dist = GetTopDist3D(pItemTarg);
	const bool fSkillRanged = g_Cfg.IsSkillFlag(skill, SKF_RANGED);

	if (fSkillRanged)
	{
		if (IsStatFlag(STATF_HASSHIELD))		// this should never happen
		{
			SysMessageDefault(DEFMSG_ITEMUSE_BOW_SHIELD);
			return WAR_SWING_INVALID;
		}
		else if (!IsSetCombatFlags(COMBAT_ARCHERYCANMOVE) && !IsStatFlag(STATF_ARCHERCANMOVE))
		{
			// Only start the swing this much tenths of second after the char stopped moving.
			//  (Values changed between expansions. SE:0,25s / AOS:0,5s / pre-AOS:1,0s)
			if (m_pClient && ((CWorldGameTime::GetCurrentTime().GetTimeDiff(m_pClient->m_timeLastEventWalk) / MSECS_PER_TENTH) < g_Cfg.m_iCombatArcheryMovementDelay))
				return WAR_SWING_EQUIPPING;
		}

		if (pWeapon)
		{
			CResourceID ridAmmo = pWeapon->Weapon_GetRangedAmmoRes();
			if (ridAmmo)
			{
				pAmmo = pWeapon->Weapon_FindRangedAmmo(ridAmmo);
				if (!pAmmo && m_pPlayer)
				{
					SysMessageDefault(DEFMSG_COMBAT_ARCH_NOAMMO);
					return WAR_SWING_INVALID;
				}
			}
		}

		if (!fSwingNoRange || (m_atFight.m_iWarSwingState == WAR_SWING_SWINGING))
		{
			// If we are using Swing_NoRange, we can start the swing regardless of the distance, but we can land the hit only when we are at the right distance
			const WAR_SWING_TYPE swingTypeHold = fSwingNoRange ? m_atFight.m_iWarSwingState : WAR_SWING_READY;

			int	iMinDist = pWeapon ? pWeapon->GetRangeL() : g_Cfg.m_iArcheryMinDist;
			int	iMaxDist = pWeapon ? pWeapon->GetRangeH() : g_Cfg.m_iArcheryMaxDist;
			if (!iMaxDist || (iMinDist == 0 && iMaxDist == 1))
				iMaxDist = g_Cfg.m_iArcheryMaxDist;
			if (!iMinDist)
				iMinDist = g_Cfg.m_iArcheryMinDist;

			if (dist < iMinDist)
			{
				SysMessageDefault(DEFMSG_COMBAT_ARCH_TOOCLOSE);
				if (!IsSetCombatFlags(COMBAT_STAYINRANGE) || (m_atFight.m_iWarSwingState != WAR_SWING_SWINGING))
					return swingTypeHold;
				return WAR_SWING_EQUIPPING;
			}
			else if (dist > iMaxDist)
			{
				if (!IsSetCombatFlags(COMBAT_STAYINRANGE) || (m_atFight.m_iWarSwingState != WAR_SWING_SWINGING))
					return swingTypeHold;
				return WAR_SWING_EQUIPPING;
			}
		}
	}
	else
	{
		if (!fSwingNoRange || (m_atFight.m_iWarSwingState == WAR_SWING_SWINGING))
		{
			// If we are using PreHit_NoRange, we can start the swing regardless of the distance, but we can land the hit only when we are at the right distance
			const WAR_SWING_TYPE swingTypeHold = fSwingNoRange ? m_atFight.m_iWarSwingState : WAR_SWING_READY;

			int	iMinDist = pWeapon ? pWeapon->GetRangeL() : 0;
			int	iMaxDist = Fight_CalcRange(pWeapon);
			if ((dist < iMinDist) || (dist > iMaxDist))
			{
				if (!IsSetCombatFlags(COMBAT_STAYINRANGE) || (m_atFight.m_iWarSwingState != WAR_SWING_SWINGING))
					return swingTypeHold;
				return WAR_SWING_EQUIPPING;
			}
		}
	}

	// Setting the recoil time.
	if (m_atFight.m_iWarSwingState == WAR_SWING_EQUIPPING)
	{
		m_atFight.m_iSwingAnimation = (int16)GenerateAnimate(ANIM_ATTACK_WEAPON);

		// FIXME
		// CTRIG_HitTry
		// Missing Trigger

		// Wait for the recoil time.
		SetTimeoutD(m_atFight.m_iRecoilDelay);   
		m_atFight.m_iWarSwingState = WAR_SWING_READY;
		return WAR_SWING_READY;
	}

	// I have waited for the recoil time
	// Then, I can start the swing
	if (m_atFight.m_iWarSwingState == WAR_SWING_READY)
	{
		m_atFight.m_iWarSwingState = WAR_SWING_SWINGING;
		Reveal();

		if (!IsSetCombatFlags(COMBAT_NODIRCHANGE))
			UpdateDir(pItemTarg);

		byte iSwingAnimationDelayInSeconds;
		if (IsSetCombatFlags(COMBAT_PREHIT) && !IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH))
			iSwingAnimationDelayInSeconds = 1;
		else
		{
			if (IsSetCombatFlags(COMBAT_PREHIT))
				iSwingAnimationDelayInSeconds = (byte)(m_atFight.m_iRecoilDelay / 10);
			else
				iSwingAnimationDelayInSeconds = (byte)(m_atFight.m_iSwingAnimationDelay / 10);
		
			if (iSwingAnimationDelayInSeconds <= 0)
				iSwingAnimationDelayInSeconds = 1;
		}

		UpdateAnimate((ANIM_TYPE)m_atFight.m_iSwingAnimation, false, false, maximum(0, iSwingAnimationDelayInSeconds));

		// Now that I have waited the recoil time
		// Start the hit animation and wait for it to end
		SetTimeoutD(m_atFight.m_iSwingAnimationDelay);
		return WAR_SWING_SWINGING;
	}

	// PostSwing Behavior
	if (fSkillRanged && pWeapon)
	{
		ITEMID_TYPE AnimID = ITEMID_NOTHING;
		dword AnimHue = 0, AnimRender = 0;
		pWeapon->Weapon_GetRangedAmmoAnim(AnimID, AnimHue, AnimRender);
		pItemTarg->Effect(EFFECT_BOLT, AnimID, this, 18, 1, false, AnimHue, AnimRender);

		// Throwing weapons also have anim of the weapon returning after throw it.
		if (m_pClient && (skill == SKILL_THROWING))		
		{
			m_pClient->m_timeLastSkillThrowing = CWorldGameTime::GetCurrentTime().GetTimeRaw();
			m_pClient->m_pSkillThrowingTarg = pItemTarg;
			m_pClient->m_SkillThrowingAnimID = AnimID;
			m_pClient->m_SkillThrowingAnimHue = AnimHue;
			m_pClient->m_SkillThrowingAnimRender = AnimRender;
		}
	}

	// We made our Swing. 
	// Apply the damage and start the recoil.
	m_atFight.m_iWarSwingState = WAR_SWING_EQUIPPING;

	// We missed
	if (m_Act_Difficulty < 0)
	{
		// FIXME
		// CTRIG_HitMiss
		// Missing Trigger

		if (pAmmo && m_pPlayer)
		{
			if (40 >= Calc_GetRandVal(100))
			{
				pAmmo->UnStackSplit(1);
				pAmmo->MoveToDecay(pItemTarg->GetTopPoint(), g_Cfg.m_iDecay_Item);
			}
			else
				pAmmo->ConsumeAmount(1);
		}

		if (IsPriv(PRIV_DETAIL))
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_MISSS), pItemTarg->GetName());
		
		SOUND_TYPE iSound = SOUND_NONE;
		if (pWeapon)
			iSound = pWeapon->Weapon_GetSoundMiss();
		
		if (iSound == SOUND_NONE)
		{
			if (g_Cfg.IsSkillFlag(skill, SKF_RANGED))
			{
				static constexpr SOUND_TYPE sm_Snd_Miss_Ranged[] = { 0x233, 0x238 };
				iSound = sm_Snd_Miss_Ranged[Calc_GetRandVal(CountOf(sm_Snd_Miss_Ranged))];
			}
			else
			{
				static constexpr SOUND_TYPE sm_Snd_Miss[] = { 0x238, 0x239, 0x23a };
				iSound = sm_Snd_Miss[Calc_GetRandVal(CountOf(sm_Snd_Miss))];
			}
		}
		Sound(iSound);

		return WAR_SWING_EQUIPPING_NOWAIT;
	}

	if ( pAmmo )
		pAmmo->ConsumeAmount(1);

	// Check if the weapon will be damaged
	if ( pWeapon )
	{
		// int iDamageChance = (int)(Args.m_VarsLocal.GetKeyNum("ItemDamageChance"));
		// if (iDamageChance > Calc_GetRandVal(100))
		//	pWeapon->OnTakeDamage(iDmg, pItemTarg);
	}

	// FIXME
	// Apply damage!
	// We must look how items handle damage
	// Not really cool :)

	// Took my swing. Do Damage !
	int	iDmg = Fight_CalcDamage(pWeapon);
	pItemTarg->OnTakeDamage( iDmg, this, iDmgType );

	if (iDmg > 0)
	{
		// Check for passive skill gain
		if (m_pPlayer)
		{
			Skill_Experience(skill, m_Act_Difficulty);
			Skill_Experience(SKILL_TACTICS, m_Act_Difficulty);
		}
	}

	return WAR_SWING_EQUIPPING_NOWAIT;
}

WAR_SWING_TYPE CChar::Fight_CanHit(CItem* pItemTarg, bool fSwingNoRange)
{
	// We can't hit them. Char deleted? Target deleted? Am I dead or stoned? or Is target Dead, stone, invul, insub or slept?
	if (IsDisconnected() || IsStatFlag(STATF_DEAD | STATF_STONE))
	{
		return WAR_SWING_INVALID;
	}
	// We can't hit them right now. Because we can't see them or reach them (invis/hidden).
	// Why the target is freeze we are change the attack type to swinging? Player can still attack paralyzed or sleeping characters.
	// We make sure that the target is freeze or sleeping must wait ready for attack!
	else if (IsStatFlag(STATF_FREEZE | STATF_SLEEPING))
	{
		return WAR_SWING_SWINGING;
	}

	// Ignore the distance and the line of sight if fSwingNoRange is true, but only if i'm starting the swing. To land the hit i need to be in range.
	if (!fSwingNoRange ||
		(IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH) && (m_atFight.m_iWarSwingState == WAR_SWING_SWINGING)) ||
		(!IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH) && (m_atFight.m_iWarSwingState == WAR_SWING_READY)))
	{
		int dist = GetTopDist3D(pItemTarg);
		if (dist > GetVisualRange())
		{
			if (!IsSetCombatFlags(COMBAT_STAYINRANGE))
				return WAR_SWING_SWINGING; //Keep loading the hit or keep it loaded and ready.

			return WAR_SWING_INVALID;
		}
		word wLOSFlags = (g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_RANGED)) ? LOS_NB_WINDOWS : 0;
		if (!CanSeeLOS(pItemTarg, wLOSFlags, true))
			return WAR_SWING_SWINGING;
	}

	return WAR_SWING_READY;
}

