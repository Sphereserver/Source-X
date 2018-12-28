
// Fight/Criminal actions/Noto.

#include "../../common/CUIDExtra.h"
#include "../../network/send.h"
#include "../clients/CClient.h"
#include "../components/CCPropsChar.h"
#include "../components/CCPropsItemWeapon.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

// 1.0 seconds is the minimum animation duration ("delay")
#define COMBAT_MIN_SWING_ANIMATION_DELAY (int16)10  // in tenths of second

// I noticed a crime.
void CChar::OnNoticeCrime( CChar * pCriminal, const CChar * pCharMark )
{
	ADDTOCALLSTACK("CChar::OnNoticeCrime");
	if ( !pCriminal || pCriminal == this || pCriminal == pCharMark || pCriminal->IsPriv(PRIV_GM) ) //This never happens: || (pCriminal->m_pNpc && pCriminal->GetNPCBrain() == NPCBRAIN_GUARD) )
		return;
    NOTO_TYPE iNoto = pCharMark->Noto_GetFlag(pCriminal);
    if (iNoto == NOTO_CRIMINAL || iNoto == NOTO_EVIL)
		return;

	// Make my owner criminal too (if I have one)
	/*CChar * pOwner = pCriminal->GetOwner();
	if ( pOwner != nullptr && pOwner != this )
		OnNoticeCrime( pOwner, pCharMark );*/

	if ( m_pPlayer )
	{
		// I have the option of attacking the criminal. or calling the guards.
		bool bCriminal = true;
		if (IsTrigUsed(TRIGGER_SEECRIME))
		{
			CScriptTriggerArgs Args;
			Args.m_iN1 = bCriminal;
			Args.m_pO1 = const_cast<CChar*>(pCharMark);
			OnTrigger(CTRIG_SeeCrime, pCriminal, &Args);
			bCriminal = Args.m_iN1 ? true : false;
		}
		if (bCriminal)
			Memory_AddObjTypes( pCriminal, MEMORY_SAWCRIME );
		return;
	}

	// NPC's can take other actions.

	ASSERT(m_pNPC);
	bool fMyMaster = NPC_IsOwnedBy( pCriminal );

	if ( this != pCharMark )	// it's not me.
	{
		if ( fMyMaster )	// I won't rat you out.
			return;
	}
	else
	{
		// I being the victim can retaliate.
		Memory_AddObjTypes( pCriminal, MEMORY_SAWCRIME );
		OnHarmedBy( pCriminal );
	}

	if ( !NPC_CanSpeak() )
		return;	// I can't talk anyhow.

	if (GetNPCBrainGroup() != NPCBRAIN_HUMAN)
	{
		// Good monsters don't call for guards outside guarded areas.
		if (!m_pArea || !m_pArea->IsGuarded())
			return;
	}

	if (m_pNPC->m_Brain != NPCBRAIN_GUARD)
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_CRIM));

	// Find a guard.
	CallGuards( pCriminal );
}

// I am commiting a crime.
// Did others see me commit or try to commit the crime.
//  SkillToSee = NONE = everyone can notice this.
// RETURN:
//  true = somebody saw me.
bool CChar::CheckCrimeSeen( SKILL_TYPE SkillToSee, CChar * pCharMark, const CObjBase * pItem, lpctstr pAction )
{
	ADDTOCALLSTACK("CChar::CheckCrimeSeen");

	bool fSeen = false;

	// Who notices ?

	if (m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD) // guards only fight for justice, they can't commit a crime!!?
		return false;

	CWorldSearch AreaChars( GetTopPoint(), UO_MAP_VIEW_SIZE_DEFAULT );
	for (;;)
	{
		CChar * pChar = AreaChars.GetChar();
		if ( pChar == nullptr )
			break;
		if ( this == pChar )
			continue;	// I saw myself before.
		if (pChar->GetPrivLevel() > GetPrivLevel()) // If a GM sees you it it not a crime.
			continue;
		if ( ! pChar->CanSeeLOS( this, LOS_NB_WINDOWS )) //what if I was standing behind a window when I saw a crime? :)
			continue;

		bool fYour = ( pCharMark == pChar );


		char *z = Str_GetTemp();
		if ( pItem && pAction )
		{
			if ( pCharMark )
				snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_2), GetName(), pAction, fYour ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_YOUR) : pCharMark->GetName(), fYour ? "" : g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_S), pItem->GetName());
			else
				snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_1), GetName(), pAction, pItem->GetName());
			pChar->ObjMessage(z, this);
		}
		fSeen = true;

		// They are not a criminal til someone calls the guards !!!
		if ( SkillToSee == SKILL_SNOOPING )
		{
			if (IsTrigUsed(TRIGGER_SEESNOOP))
			{
				CScriptTriggerArgs Args(pAction);
				Args.m_iN1 = SkillToSee ? SkillToSee : pCharMark->Skill_GetActive();;
				Args.m_iN2 = pItem ? (dword)pItem->GetUID() : 0;    // here i can modify pItem via scripts, so it isn't really const
				Args.m_pO1 = pCharMark;
				TRIGRET_TYPE iRet = pChar->OnTrigger(CTRIG_SeeSnoop, this, &Args);

				if (iRet == TRIGRET_RET_TRUE)
					continue;
				else if (iRet == TRIGRET_RET_DEFAULT)
				{
					if (!g_Cfg.Calc_CrimeSeen(this, pChar, SkillToSee, fYour))
						continue;
				}
			}
			// Off chance of being a criminal. (hehe)
			if ( Calc_GetRandVal(100) < g_Cfg.m_iSnoopCriminal )
				pChar->OnNoticeCrime( this, pCharMark );
			if ( pChar->m_pNPC )
				pChar->NPC_OnNoticeSnoop( this, pCharMark );
		}
		else
		{
			pChar->OnNoticeCrime( this, pCharMark );
		}
	}
	return fSeen;
}

void CChar::CallGuards()
{
	ADDTOCALLSTACK("CChar::CallGuards");
	if (!m_pArea || !m_pArea->IsGuarded() || IsStatFlag(STATF_DEAD))
		return;

	if (g_World.GetTimeDiff(m_timeLastCallGuards + (1 * MSECS_PER_SEC)) > 0)	// Spam check, not calling this more than once per second, which will cause an excess of calls and checks on crowded areas because of the 2 CWorldSearch.
		return;

	// We don't have any target yet, let's check everyone nearby
	CChar * pCriminal;
	CWorldSearch AreaCrime(GetTopPoint(), UO_MAP_VIEW_SIZE_DEFAULT);
	while ((pCriminal = AreaCrime.GetChar()) != nullptr)
	{
		if (pCriminal == this)
			continue;
		if (!pCriminal->m_pArea->IsGuarded())
			continue;
		if (!CanDisturb(pCriminal))	// don't allow guards to be called on someone we can't disturb
			continue;

		// Mark person as criminal if I saw him criming
		// Only players call guards this way. NPC's flag criminal instantly
		if (m_pPlayer && Memory_FindObjTypes(pCriminal, MEMORY_SAWCRIME))
			pCriminal->Noto_Criminal(this);

		if (!pCriminal->IsStatFlag(STATF_CRIMINAL) && !(pCriminal->Noto_IsEvil() && g_Cfg.m_fGuardsOnMurderers))
			continue;

		CallGuards(pCriminal);
	}
	return;
}

// I just yelled for guards.
bool CChar::CallGuards( CChar * pCriminal )
{
	ADDTOCALLSTACK("CChar::CallGuards2");
	if ( !m_pArea || (pCriminal == this) )
		return false;
	if (IsStatFlag(STATF_DEAD) || (pCriminal && (pCriminal->IsStatFlag(STATF_DEAD | STATF_INVUL) || pCriminal->IsPriv(PRIV_GM) || !pCriminal->m_pArea->IsGuarded())))
		return false;

    if (g_World.GetTimeDiff(m_timeLastCallGuards + (1 * MSECS_PER_SEC)) > 0)	// Spam check, not calling this more than once per second
        return false;

    m_timeLastCallGuards = g_World.GetCurrentTime().GetTimeRaw();

	CChar *pGuard = nullptr;
	if (m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD)
	{
		// I'm a guard, why summon someone else to do my work? :)
		pGuard = this;
	}
	else
	{
		// Search for a free guards nearby
		CWorldSearch AreaGuard(GetTopPoint(), UO_MAP_VIEW_SIGHT);
		CChar *pGuardFound = nullptr;
		while ((pGuardFound = AreaGuard.GetChar()) != nullptr)
		{
			if (pGuardFound->m_pNPC && (pGuardFound->m_pNPC->m_Brain == NPCBRAIN_GUARD) && // Char found must be a guard
				(pGuardFound->m_Fight_Targ_UID == pCriminal->GetUID() || !pGuardFound->IsStatFlag(STATF_WAR)))	// and will be eligible to fight this target if it's not already on a fight or if its already attacking this target (to avoid spamming docens of guards at the same target).
			{
				pGuard = pGuardFound;
				break;
			}
		}
	}

	const CVarDefCont *pVarDefGuards = pCriminal->m_pArea->m_TagDefs.GetKey("OVERRIDE.GUARDS");
	CResourceID rid = g_Cfg.ResourceGetIDType(RES_CHARDEF, (pVarDefGuards ? pVarDefGuards->GetValStr() : "GUARDS"));
	if (IsTrigUsed(TRIGGER_CALLGUARDS))
	{
		CScriptTriggerArgs Args(pGuard);
		Args.m_iN1 = rid.GetResIndex();
		Args.m_iN2 = 0;
		Args.m_VarObjs.Insert(1, pCriminal, true);

		if (OnTrigger(CTRIG_CallGuards, pCriminal, &Args) == TRIGRET_RET_TRUE)
			return false;

		if ( (int)Args.m_iN1 != rid.GetResIndex())
			rid = CResourceID(RES_CHARDEF, (int)Args.m_iN1);
		if (Args.m_iN2 > 0)	//ARGN2: If set to 1, a new guard will be spawned regardless of whether a nearby guard is available.
			pGuard = nullptr;
	}
	if (!pGuard)		//	spawn a new guard
	{
		if (!rid.IsValidUID())
			return false;
		pGuard = CChar::CreateNPC((CREID_TYPE)rid.GetResIndex());
		if (!pGuard)
			return false;

		if (pCriminal->m_pArea->m_TagDefs.GetKeyNum("RED"))
			pGuard->m_TagDefs.SetNum("NAME.HUE", g_Cfg.m_iColorNotoEvil, true);
		pGuard->Spell_Effect_Create(SPELL_Summon, LAYER_SPELL_Summon, g_Cfg.GetSpellEffect(SPELL_Summon, 1000), int(g_Cfg.m_iGuardLingerTime/1000));
		pGuard->Spell_Teleport(pCriminal->GetTopPoint(), false, false);
	}
	pGuard->NPC_LookAtCharGuard(pCriminal, true);
    return true;
}

// i notice a Crime or attack against me ..
// Actual harm has taken place.
// Attack back.
void CChar::OnHarmedBy( CChar * pCharSrc )
{
	ADDTOCALLSTACK("CChar::OnHarmedBy");

	bool fFightActive = Fight_IsActive();
	Memory_AddObjTypes(pCharSrc, MEMORY_HARMEDBY);

	if (fFightActive && m_Fight_Targ_UID.CharFind())
	{
		// In war mode already
		if ( m_pPlayer )
			return;
		if ( Calc_GetRandVal( 10 ))
			return;
		// NPC will Change targets.
	}

	if ( m_pNPC && NPC_IsOwnedBy(pCharSrc, false) )
		NPC_PetDesert();

	// I will Auto-Defend myself.
	Fight_Attack(pCharSrc);
}

// We have been attacked in some way by this CChar.
// Might not actually be doing any real damage. (yet)
//
// They may have just commanded their pet to attack me.
// Cast a bad spell at me.
// Fired projectile at me.
// Attempted to provoke me ?
//
// RETURN: true = ok.
//  false = we are immune to this char ! (or they to us)
bool CChar::OnAttackedBy(CChar * pCharSrc, bool fCommandPet, bool fShouldReveal)
{
	ADDTOCALLSTACK("CChar::OnAttackedBy");

	if (pCharSrc == nullptr)
		return true;	// field spell ?
	if (pCharSrc == this)
		return true;	// self induced
	if (IsStatFlag(STATF_DEAD))
		return false;

	if (fShouldReveal)
		pCharSrc->Reveal();	// fix invis exploit

	// Am i already attacking the source anyhow
	if ( Fight_IsActive() && (m_Fight_Targ_UID == pCharSrc->GetUID()) )
		return true;

	Memory_AddObjTypes(pCharSrc, MEMORY_HARMEDBY | MEMORY_IRRITATEDBY);
	Attacker_Add(pCharSrc);

	// Are they a criminal for it ? Is attacking me a crime ?
	if (Noto_GetFlag(pCharSrc) == NOTO_GOOD)
	{
		if (IsClient())	// I decide if this is a crime.
			OnNoticeCrime(pCharSrc, this);
		else
		{
			// If it is a pet then this a crime others can report.
			CChar * pCharMark = IsStatFlag(STATF_PET) ? NPC_PetGetOwner() : this;
			pCharSrc->CheckCrimeSeen(Skill_GetActive(), pCharMark, nullptr, nullptr);
		}
	}

	if (!fCommandPet)
	{
		// possibly retaliate. (auto defend)
		OnHarmedBy(pCharSrc);
	}

	return true;
}

// Armor layers that can be damaged on combat
// PS: Hand layers (weapons/shields) are not included here
static const LAYER_TYPE sm_ArmorDamageLayers[] = { LAYER_SHOES, LAYER_PANTS, LAYER_SHIRT, LAYER_HELM, LAYER_GLOVES, LAYER_COLLAR, LAYER_HALF_APRON, LAYER_CHEST, LAYER_TUNIC, LAYER_ARMS, LAYER_CAPE, LAYER_ROBE, LAYER_SKIRT, LAYER_LEGS };

// Layers covering the armor zone
static const LAYER_TYPE sm_ArmorLayerHead[] = { LAYER_HELM };															// ARMOR_HEAD
static const LAYER_TYPE sm_ArmorLayerNeck[] = { LAYER_COLLAR };															// ARMOR_NECK
static const LAYER_TYPE sm_ArmorLayerBack[] = { LAYER_SHIRT, LAYER_CHEST, LAYER_TUNIC, LAYER_CAPE, LAYER_ROBE };		// ARMOR_BACK
static const LAYER_TYPE sm_ArmorLayerChest[] = { LAYER_SHIRT, LAYER_CHEST, LAYER_TUNIC, LAYER_ROBE };					// ARMOR_CHEST
static const LAYER_TYPE sm_ArmorLayerArms[] = { LAYER_ARMS, LAYER_CAPE, LAYER_ROBE };									// ARMOR_ARMS
static const LAYER_TYPE sm_ArmorLayerHands[] = { LAYER_GLOVES };														// ARMOR_HANDS
static const LAYER_TYPE sm_ArmorLayerLegs[] = { LAYER_PANTS, LAYER_SKIRT, LAYER_HALF_APRON, LAYER_ROBE, LAYER_LEGS };	// ARMOR_LEGS
static const LAYER_TYPE sm_ArmorLayerFeet[] = { LAYER_SHOES, LAYER_LEGS };												// ARMOR_FEET
static const LAYER_TYPE sm_ArmorLayerShield[] = { LAYER_HAND2 };

struct CArmorLayerType
{
	int m_iCoverage;	// Percentage of humanoid body area
	const LAYER_TYPE * m_pLayers;
};

static const CArmorLayerType sm_ArmorLayers[ARMOR_QTY] =
{
	{ 15,	sm_ArmorLayerHead },	// ARMOR_HEAD, 15% of the armour value will be applied.
	{ 7,	sm_ArmorLayerNeck },	// ARMOR_NECK, 7% of the armour value will be applied.
	{ 0,	sm_ArmorLayerBack },	// ARMOR_BACK, 0% of the armour value will be applied.
	{ 35,	sm_ArmorLayerChest },	// ARMOR_CHEST, 35% of the armour value will be applied.
	{ 14,	sm_ArmorLayerArms },	// ARMOR_ARMS, 14% of the armour value will be applied.
	{ 7,	sm_ArmorLayerHands },	// ARMOR_HANDS, 7% of the armour value will be applied.
	{ 22,	sm_ArmorLayerLegs },	// ARMOR_LEGS, 22% of the armour value will be applied.
	{ 0,	sm_ArmorLayerFeet },	// ARMOR_FEET, 0% of the armour value will be applied.
	{100,	sm_ArmorLayerShield }	// ARMOR_SHIELD, 100% of the armour value will be applied, this only used if CombatParryEra flag PARRYERA_ARSCALING is enabled
};

// When armor is added or subtracted check this.
// This is the general AC number printed.
// Tho not really used to compute damage.
int CChar::CalcArmorDefense() const
{
	ADDTOCALLSTACK("CChar::CalcArmorDefense");

	// If Combat Elemental Engine is enabled, we don't need to calculate the AC because RESPHYSICAL is used.
	if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
		 return 0;

	int iDefenseTotal = 0;
	int iArmorCount = 0;
	int ArmorRegionMax[ARMOR_QTY];
	for ( int i = 0; i < ARMOR_QTY; ++i )
		ArmorRegionMax[i] = 0;

	for ( CItem* pItem=GetContentHead(); pItem!=nullptr; pItem=pItem->GetNext() )
	{
		int iDefense = pItem->Armor_GetDefense();
		if ( !iDefense && !pItem->IsType(IT_SPELL) )
			continue;

		// reverse of sm_ArmorLayers
		switch ( pItem->GetEquipLayer() )
		{
			case LAYER_HELM:		// 6
				if (IsSetCombatFlags(COMBAT_STACKARMOR))
					ArmorRegionMax[ ARMOR_HEAD ] += iDefense;
				else
					ArmorRegionMax[ ARMOR_HEAD ] = maximum( ArmorRegionMax[ ARMOR_HEAD ], iDefense );
				break;
			case LAYER_COLLAR:	// 10 = gorget or necklace.
				if (IsSetCombatFlags(COMBAT_STACKARMOR))
					ArmorRegionMax[ ARMOR_NECK ] += iDefense;
				else
					ArmorRegionMax[ ARMOR_NECK ] = maximum( ArmorRegionMax[ ARMOR_NECK ], iDefense );
				break;
			case LAYER_SHIRT:
			case LAYER_CHEST:	// 13 = armor chest
			case LAYER_TUNIC:	// 17 = jester suit
				if (IsSetCombatFlags(COMBAT_STACKARMOR)) {
					ArmorRegionMax[ ARMOR_CHEST ] += iDefense;
					ArmorRegionMax[ ARMOR_BACK ] += iDefense;
				} else {
					ArmorRegionMax[ ARMOR_CHEST ] = maximum( ArmorRegionMax[ ARMOR_CHEST ], iDefense );
					ArmorRegionMax[ ARMOR_BACK ] = maximum( ArmorRegionMax[ ARMOR_BACK ], iDefense );
				}
				break;
			case LAYER_ARMS:		// 19 = armor
				if (IsSetCombatFlags(COMBAT_STACKARMOR))
					ArmorRegionMax[ ARMOR_ARMS ] += iDefense;
				else
					ArmorRegionMax[ ARMOR_ARMS ] = maximum( ArmorRegionMax[ ARMOR_ARMS ], iDefense );
				break;
			case LAYER_PANTS:
			case LAYER_SKIRT:
			case LAYER_HALF_APRON:
				if (IsSetCombatFlags(COMBAT_STACKARMOR))
					ArmorRegionMax[ ARMOR_LEGS ] += iDefense;
				else
					ArmorRegionMax[ ARMOR_LEGS ] = maximum( ArmorRegionMax[ ARMOR_LEGS ], iDefense );
				break;
			case LAYER_SHOES:
				if (IsSetCombatFlags(COMBAT_STACKARMOR))
					ArmorRegionMax[ ARMOR_FEET ] += iDefense;
				else
					ArmorRegionMax[ ARMOR_FEET ] = maximum( ArmorRegionMax[ ARMOR_FEET ], iDefense );
				break;
			case LAYER_GLOVES:	// 7
				if (IsSetCombatFlags(COMBAT_STACKARMOR))
					ArmorRegionMax[ ARMOR_HANDS ] += iDefense;
				else
					ArmorRegionMax[ ARMOR_HANDS ] = maximum( ArmorRegionMax[ ARMOR_HANDS ], iDefense );
				break;
			case LAYER_CAPE:		// 20 = cape
				if (IsSetCombatFlags(COMBAT_STACKARMOR)) {
					ArmorRegionMax[ ARMOR_BACK ] += iDefense;
					ArmorRegionMax[ ARMOR_ARMS ] += iDefense;
				} else {
					ArmorRegionMax[ ARMOR_BACK ] = maximum( ArmorRegionMax[ ARMOR_BACK ], iDefense );
					ArmorRegionMax[ ARMOR_ARMS ] = maximum( ArmorRegionMax[ ARMOR_ARMS ], iDefense );
				}
				break;
			case LAYER_ROBE:		// 22 = robe over all.
				if (IsSetCombatFlags(COMBAT_STACKARMOR)) {
					ArmorRegionMax[ ARMOR_CHEST ] += iDefense;
					ArmorRegionMax[ ARMOR_BACK ] += iDefense;
					ArmorRegionMax[ ARMOR_ARMS ] += iDefense;
					ArmorRegionMax[ ARMOR_LEGS ] += iDefense;
				} else {
					ArmorRegionMax[ ARMOR_CHEST ] = maximum( ArmorRegionMax[ ARMOR_CHEST ], iDefense );
					ArmorRegionMax[ ARMOR_BACK ] = maximum( ArmorRegionMax[ ARMOR_BACK ], iDefense );
					ArmorRegionMax[ ARMOR_ARMS ] = maximum( ArmorRegionMax[ ARMOR_ARMS ], iDefense );
					ArmorRegionMax[ ARMOR_LEGS ] = maximum( ArmorRegionMax[ ARMOR_LEGS ], iDefense );
				}
				break;
			case LAYER_LEGS:
				if (IsSetCombatFlags(COMBAT_STACKARMOR)) {
					ArmorRegionMax[ ARMOR_LEGS ] += iDefense;
					ArmorRegionMax[ ARMOR_FEET ] += iDefense;
				} else {
					ArmorRegionMax[ ARMOR_LEGS ] = maximum( ArmorRegionMax[ ARMOR_LEGS ], iDefense );
					ArmorRegionMax[ ARMOR_FEET ] = maximum( ArmorRegionMax[ ARMOR_FEET ], iDefense );
				}
				break;
			case LAYER_HAND2:
				if ( pItem->IsType( IT_SHIELD ))
				{
					/*
					If CombatParryingEra flag PARRYERA_SCALING is enabled:
					Displayed AR = ((Parrying Skill * Base AR of Shield) � 200) + 1
					For all shields the maximum AR will be reached before your parry skill reaches GM level.
					Also note that the maximum displayed AR for a shield cannot exceed Base AR � 2.
					See: http://web.archive.org/web/20000306210936/http://uo.stratics.com:80/parr.htm
					Else, CombatParryingEra flag PARRYERA_SCALING is disabled and only a flat 7% AR of the shield will be used.
					*/
					BODYPART_TYPE shieldZone = ARMOR_HANDS;
					if (g_Cfg.m_iCombatParryingEra & PARRYERA_ARSCALING)
					{
						shieldZone = ARMOR_SHIELD;
						int uShieldAC = ((Skill_GetBase(SKILL_PARRYING) * iDefense) / 2000) + 1;
						iDefense = minimum(iDefense / 2, uShieldAC);
					}
					if (IsSetCombatFlags(COMBAT_STACKARMOR)) //Don't understand, you can't stack shields
						ArmorRegionMax[shieldZone] += iDefense;
					else
						ArmorRegionMax[shieldZone] = maximum(ArmorRegionMax[shieldZone], iDefense);
				}
				break;
			case LAYER_SPELL_Protection:
				iDefenseTotal += pItem->m_itSpell.m_spelllevel * 100;
				break;
			default:
				continue;
		}
		++iArmorCount;
	}

	if ( iArmorCount )
	{
		for ( int i=0; i<ARMOR_QTY; ++i )
			iDefenseTotal += sm_ArmorLayers[i].m_iCoverage * ArmorRegionMax[i];
	}

	return maximum(( iDefenseTotal / 100 ) + m_ModAr, 0);
}

// Someone hit us.
// iDmg already defined, here we just apply armor related calculations
//
// uType: damage flags
//  DAMAGE_GOD
//  DAMAGE_HIT_BLUNT
//  DAMAGE_MAGIC
//  ...
//
// iDmg[Physical/Fire/Cold/Poison/Energy]: % of each type to split the damage into
//  Eg: iDmgPhysical=70 + iDmgCold=30 will split iDmg value into 70% physical + 30% cold
//
// RETURN: damage done
//  -1		= already dead / invalid target.
//  0		= no damage.
//  INT32_MAX	= killed.
int CChar::OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType, int iDmgPhysical, int iDmgFire, int iDmgCold, int iDmgPoison, int iDmgEnergy )
{
	ADDTOCALLSTACK("CChar::OnTakeDamage");

	if ( pSrc == nullptr )
		pSrc = this;

	if ( IsStatFlag(STATF_DEAD) )	// already dead
		return -1;

	if ( !(uType & DAMAGE_GOD) )
	{
		if ( IsStatFlag(STATF_INVUL|STATF_STONE) )
		{
effect_bounce:
			Effect( EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16 );
			return 0;
		}
		if ( (uType & DAMAGE_FIRE) && Can(CAN_C_FIRE_IMMUNE) )
			goto effect_bounce;
		// I can't take damage from my pets, the only exception is for BRAIN_BERSERK pets
		if ( pSrc->m_pNPC && (pSrc->NPC_PetGetOwner() == this) && (pSrc->m_pNPC->m_Brain != NPCBRAIN_BERSERK) )
			goto effect_bounce;
		if ( m_pArea )
		{
			if ( m_pArea->IsFlag(REGION_FLAG_SAFE) )
			{
				// TODO: add a sysmessage
				goto effect_bounce;
			}
			if ( m_pArea->IsFlag(REGION_FLAG_NO_PVP) && m_pPlayer)
			{
				if (pSrc->m_pPlayer)	// player attacking player
				{
					// TODO: add a sysmessage
					goto effect_bounce;
				}
				if (pSrc->m_pNPC)
				{
					const CChar* pOwner = pSrc->NPC_PetGetOwnerRecursive();
					if (pOwner && pOwner->m_pPlayer)	// pet attacking player
						goto effect_bounce;
				}
			}
		}
	}

	// Make some notoriety checks
	// Don't reveal attacker if the damage has DAMAGE_NOREVEAL flag set (this is set by default for poison and spell damage)
	if ( !OnAttackedBy(pSrc, false, !(uType & DAMAGE_NOREVEAL)) )
		return 0;

	// Apply Necromancy cursed effects
	if ( IsAosFlagEnabled(FEATURE_AOS_UPDATE_B) )
	{
		CItem * pEvilOmen = LayerFind(LAYER_SPELL_Evil_Omen);
		if ( pEvilOmen )
		{
			iDmg += iDmg / 4;
			pEvilOmen->Delete();
		}

		CItem * pBloodOath = LayerFind(LAYER_SPELL_Blood_Oath);
		if ( pBloodOath && pBloodOath->m_uidLink == pSrc->GetUID() && !(uType & DAMAGE_FIXED) )	// if DAMAGE_FIXED is set we are already receiving a reflected damage, so we must stop here to avoid an infinite loop.
		{
			iDmg += iDmg / 10;
			pSrc->OnTakeDamage(iDmg * (100 - pBloodOath->m_itSpell.m_spelllevel) / 100, this, DAMAGE_MAGIC|DAMAGE_FIXED);
		}
	}

	const CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);
    const CCPropsChar *pCCPChar = GetCCPropsChar(), *pBaseCCPChar = pCharDef->GetCCPropsChar();

	// MAGICF_IGNOREAR bypasses defense completely
	if ( (uType & DAMAGE_MAGIC) && IsSetMagicFlags(MAGICF_IGNOREAR) )
		uType |= DAMAGE_FIXED;

	// Apply armor calculation
	if ( !(uType & (DAMAGE_GOD|DAMAGE_FIXED)) )
	{
		if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
		{
			// AOS elemental combat
			if ( iDmgPhysical == 0 )		// if physical damage is not set, let's assume it as the remaining value
				iDmgPhysical = 100 - (iDmgFire + iDmgCold + iDmgPoison + iDmgEnergy);

			int iPhysicalDamage = iDmg * iDmgPhysical * (100 - (int)GetPropNum(pCCPChar, PROPCH_RESPHYSICAL, pBaseCCPChar));
			int iFireDamage = iDmg * iDmgFire * (100 - (int)GetPropNum(pCCPChar, PROPCH_RESFIRE, pBaseCCPChar));
			int iColdDamage = iDmg * iDmgCold * (100 - (int)GetPropNum(pCCPChar, PROPCH_RESCOLD, pBaseCCPChar));
			int iPoisonDamage = iDmg * iDmgPoison * (100 - (int)GetPropNum(pCCPChar, PROPCH_RESPOISON, pBaseCCPChar));
			int iEnergyDamage = iDmg * iDmgEnergy * (100 - (int)GetPropNum(pCCPChar, PROPCH_RESENERGY, pBaseCCPChar));

			iDmg = (iPhysicalDamage + iFireDamage + iColdDamage + iPoisonDamage + iEnergyDamage) / 10000;
		}
		else
		{
			// pre-AOS armor rating (AR)
			int iArmorRating = pCharDef->m_defense + m_defense;

			int iArMax = iArmorRating * Calc_GetRandVal2(7,35) / 100;
			int iArMin = iArMax / 2;

			int iDef = Calc_GetRandVal2( iArMin, (iArMax - iArMin) + 1 );
			if ( uType & DAMAGE_MAGIC )		// magical damage halves effectiveness of defense
				iDef /= 2;

			iDmg -= iDef;
		}
	}

	CScriptTriggerArgs Args( iDmg, uType, (int64)(0) );
	Args.m_VarsLocal.SetNum("ItemDamageLayer", sm_ArmorDamageLayers[Calc_GetRandVal(CountOf(sm_ArmorDamageLayers))]);
	Args.m_VarsLocal.SetNum("ItemDamageChance", 40);

	if ( IsTrigUsed(TRIGGER_GETHIT) )
	{
		if ( OnTrigger( CTRIG_GetHit, pSrc, &Args ) == TRIGRET_RET_TRUE )
			return 0;
		iDmg = (int)(Args.m_iN1);
		uType = (DAMAGE_TYPE)(Args.m_iN2);
	}

	int iItemDamageChance = (int)(Args.m_VarsLocal.GetKeyNum("ItemDamageChance"));
	if ( (iItemDamageChance > Calc_GetRandVal(100)) && !Can(CAN_C_NONHUMANOID) )
	{
		LAYER_TYPE iHitLayer = (LAYER_TYPE)(Args.m_VarsLocal.GetKeyNum("ItemDamageLayer"));
		CItem *pItemHit = LayerFind(iHitLayer);
		if ( pItemHit )
			pItemHit->OnTakeDamage(iDmg, pSrc, uType);
	}

	// Remove stuck/paralyze effect
	if ( !(uType & DAMAGE_NOUNPARALYZE) )
	{
		CItem * pParalyze = LayerFind(LAYER_SPELL_Paralyze);
		if ( pParalyze )
			pParalyze->Delete();

		CItem * pStuck = LayerFind(LAYER_FLAG_Stuck);
		if ( pStuck )
			pStuck->Delete();

		if ( IsStatFlag(STATF_FREEZE) )
		{
			StatFlag_Clear(STATF_FREEZE);
			UpdateMode();
		}
	}

    if (IsSetCombatFlags(COMBAT_SLAYER))
    {
		CItem *pWeapon = nullptr;
		if (uType & DAMAGE_MAGIC)	// If the damage is magic
		{
			pWeapon = pSrc->LayerFind(LAYER_HAND1);	// Search for an equipped spellbook
			if ((pWeapon) && (!pWeapon->IsTypeSpellbook()))	// If there is nothing on the hand, or the item is not a spellbook.
			{
				pWeapon = pSrc->m_uidWeapon.ItemFind();	// then force a weapon find.
			}
		}
        int iDmgBonus = 1;
        CCFaction *pSlayer = nullptr;
        const CCFaction *pFaction = GetFaction();
        const CCFaction *pSrCCFaction = pSrc->GetFaction();
        if (pWeapon)
        {
            pSlayer = pWeapon->GetFaction();
            if (pSlayer && pWeapon->GetSlayer()->GetFactionID() != FACTION_NONE)
            {
                if (m_pNPC) // I'm an NPC attacked (Should the attacker be a player to get the bonus?).
                {
                    if (pSlayer->GetFactionID() != FACTION_NONE)
                    {
                        iDmgBonus = pSlayer->GetSlayerDamageBonus(pWeapon->GetSlayer());
                    }
                }
                else if (m_pPlayer && pSrc->m_pNPC) // Wielding a slayer type against its opposite will cause the attacker to take more damage
                {
                    if (pSrCCFaction->GetFactionID() != FACTION_NONE)
                    {
                        iDmgBonus = pSlayer->GetSlayerDamagePenalty(pSrCCFaction);
                    }
                }
            }
        }
        if (iDmgBonus == 1) // Couldn't find a weapon, a Slayer flag or a suitable flag for the target...
        {
            CItem *pTalisman = pSrc->LayerFind(LAYER_TALISMAN); // then lets try with a Talisman
            if (pTalisman)
            {
                pSlayer = pTalisman->GetSlayer();
                if (pSlayer  && pSlayer->GetFactionID() != FACTION_NONE)
                {
                    if (m_pNPC) // I'm an NPC attacked (Should the attacker be a player to get the bonus?).
                    {
                        if (pFaction->GetFactionID() != FACTION_NONE)
                        {
                            iDmgBonus = pFaction->GetSlayerDamageBonus(pSlayer);
                        }
                    }
                    else if (m_pPlayer && pSrc->m_pNPC) // Wielding a slayer type against its opposite will cause the attacker to take more damage
                    {
                        if (pSrCCFaction->GetFactionID() != FACTION_NONE)
                        {
                            iDmgBonus = pFaction->GetSlayerDamagePenalty(pSrCCFaction);
                        }
                    }
                }
            }
        }
        if (iDmgBonus > 1)
        {
            iDmg *= iDmgBonus;
        }
    }

	// Disturb magic spells (only players can be disturbed)
	if ( m_pPlayer && (pSrc != this) && !(uType & DAMAGE_NODISTURB) && g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_MAGIC) )
	{
		// Check if my spell can be interrupted
		int iDisturbChance = 0;
		int iSpellSkill = -1;
		const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
		if ( pSpellDef && pSpellDef->GetPrimarySkill(&iSpellSkill) )
			iDisturbChance = pSpellDef->m_Interrupt.GetLinear(Skill_GetBase((SKILL_TYPE)iSpellSkill));

		if ( iDisturbChance && IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
		{
			// Protection spell can cancel the disturb
			CItem *pProtectionSpell = LayerFind(LAYER_SPELL_Protection);
			if ( pProtectionSpell )
			{
				const int iChance = pProtectionSpell->m_itSpell.m_spelllevel;
				if ( iChance > Calc_GetRandVal(1000) )
					iDisturbChance = 0;
			}
		}

		if ( iDisturbChance > Calc_GetRandVal(1000) )
			Skill_Fail();
	}

	if ( pSrc && (pSrc != this) )
	{
		// Update attacker list
		bool bAttackerExists = false;
        const uint uiSrcUID = pSrc->GetUID().GetPrivateUID();
		for (LastAttackers& refAttacker : m_lastAttackers)
		{
			if ( refAttacker.charUID == uiSrcUID )
			{
				refAttacker.elapsed = 0;
				refAttacker.amountDone += maximum( 0, iDmg );
				refAttacker.threat += maximum( 0, iDmg );
				bAttackerExists = true;
				break;
			}
		}
		if (bAttackerExists == false)
		{
			LastAttackers attacker;
			attacker.charUID = uiSrcUID;
			attacker.elapsed = 0;
			attacker.amountDone = maximum( 0, iDmg );
			attacker.threat = maximum( 0, iDmg );
			m_lastAttackers.emplace_back(std::move(attacker));
		}

		// A physical blow of some sort.
		if (uType & (DAMAGE_HIT_BLUNT|DAMAGE_HIT_PIERCE|DAMAGE_HIT_SLASH))
		{
			// Check if Reactive Armor will reflect some damage back
			if ( IsStatFlag(STATF_REACTIVE) && !(uType & DAMAGE_GOD) )
			{
				if ( GetTopDist3D(pSrc) < 2 )
				{
					int iReactiveDamage = iDmg / 5;
					if ( iReactiveDamage < 1 )
						iReactiveDamage = 1;

					iDmg -= iReactiveDamage;
					pSrc->OnTakeDamage( iReactiveDamage, this, (DAMAGE_TYPE)(DAMAGE_FIXED), iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy );
					pSrc->Sound( 0x1F1 );
					pSrc->Effect( EFFECT_OBJ, ITEMID_FX_CURSE_EFFECT, this, 10, 16 );
				}
			}
		}
	}

	if (iDmg <= 0)
		return 0;

	// Apply damage
	SoundChar(CRESND_GETHIT);
	UpdateStatVal( STAT_STR, -iDmg);
	if ( pSrc->IsClient() )
		pSrc->GetClient()->addHitsUpdate( this );	// always send updates to src

	if ( IsAosFlagEnabled( FEATURE_AOS_DAMAGE ) )
	{
		if ( IsClient() )
			m_pClient->addShowDamage( iDmg, (dword)(GetUID()) );
		if ( pSrc->IsClient() && (pSrc != this) )
			pSrc->m_pClient->addShowDamage( iDmg, (dword)(GetUID()) );
		else
		{
			CChar * pSrcOwner = pSrc->GetOwner();
			if ( pSrcOwner != nullptr )
			{
				if ( pSrcOwner->IsClient() )
					pSrcOwner->m_pClient->addShowDamage( iDmg, (dword)(GetUID()) );
			}
		}
	}

	if ( Stat_GetVal(STAT_STR) <= 0 )
	{
		// We will die from this. Make sure the killer is set correctly, otherwise the person we are currently attacking will get credit for killing us.
		m_Fight_Targ_UID = pSrc->GetUID();
		return iDmg;
	}

	if (m_atFight.m_War_Swing_State != WAR_SWING_SWINGING)	// don't interrupt my swing animation
		UpdateAnimate(ANIM_GET_HIT);

	return iDmg;
}

//*******************************************************************************
// Fight specific memories.

//********************************************************

byte CChar::GetRangeL() const
{
    if (_iRange == 0)
        return Char_GetDef()->GetRangeL();
    return (byte)(_iRange & 0xff);
}

byte CChar::GetRangeH() const
{
    if (_iRange == 0)
        return Char_GetDef()->GetRangeH();
    return (byte)((_iRange >> 8) & 0xff);
}

// What sort of weapon am i using?
SKILL_TYPE CChar::Fight_GetWeaponSkill() const
{
	ADDTOCALLSTACK_INTENSIVE("CChar::Fight_GetWeaponSkill");
	CItem * pWeapon = m_uidWeapon.ItemFind();
	if ( pWeapon == nullptr )
		return SKILL_WRESTLING;
	return pWeapon->Weapon_GetSkill();
}

DAMAGE_TYPE CChar::Fight_GetWeaponDamType(const CItem* pWeapon) const
{
    ADDTOCALLSTACK_INTENSIVE("CChar::Fight_GetWeaponDamType");
    DAMAGE_TYPE iDmgType = DAMAGE_HIT_BLUNT;
    if ( pWeapon )
    {
        CVarDefCont *pDamTypeOverride = pWeapon->GetKey("OVERRIDE.DAMAGETYPE", true);
        if ( pDamTypeOverride )
        {
            iDmgType = (DAMAGE_TYPE)(pDamTypeOverride->GetValNum());
        }
        else
        {
            CItemBase *pWeaponDef = pWeapon->Item_GetDef();
            switch ( pWeaponDef->GetType() )
            {
                case IT_WEAPON_SWORD:
                case IT_WEAPON_AXE:
                case IT_WEAPON_THROWING:
                    iDmgType |= DAMAGE_HIT_SLASH;
                    break;
                case IT_WEAPON_FENCE:
                case IT_WEAPON_BOW:
                case IT_WEAPON_XBOW:
                    iDmgType |= DAMAGE_HIT_PIERCE;
                    break;
                default:
                    break;
            }
        }
    }
    return iDmgType;
}

// Am i in an active fight mode ?
bool CChar::Fight_IsActive() const
{
	ADDTOCALLSTACK("CChar::Fight_IsActive");
	if ( ! IsStatFlag(STATF_WAR))
		return false;

	SKILL_TYPE iSkillActive = Skill_GetActive();
	switch ( iSkillActive )
	{
		case SKILL_ARCHERY:
		case SKILL_FENCING:
		case SKILL_MACEFIGHTING:
		case SKILL_SWORDSMANSHIP:
		case SKILL_WRESTLING:
		case SKILL_THROWING:
        case NPCACT_BREATH:
        case NPCACT_THROWING:
			return true;

		default:
			break;
	}

	if ( iSkillActive == Fight_GetWeaponSkill() )
		return true;

	return g_Cfg.IsSkillFlag( iSkillActive, SKF_FIGHT );
}

// Calculating base DMG (also used for STATUS value)
int CChar::Fight_CalcDamage( const CItem * pWeapon, bool bNoRandom, bool bGetMax ) const
{
	ADDTOCALLSTACK("CChar::Fight_CalcDamage");

	if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD && g_Cfg.m_fGuardsInstantKill )
		return( 20000 );	// swing made.

	int iDmgMin = 0;
	int iDmgMax = 0;
	STAT_TYPE iStatBonus = (STAT_TYPE)GetPropNum(COMP_PROPS_CHAR, PROPCH_COMBATBONUSSTAT, true);
	int iStatBonusPercent = (int)GetPropNum(COMP_PROPS_CHAR, PROPCH_COMBATBONUSPERCENT, true);
	if ( pWeapon != nullptr )
	{
		iDmgMin = pWeapon->Weapon_GetAttack(false);
		iDmgMax = pWeapon->Weapon_GetAttack(true);
	}
	else
	{
		iDmgMin = m_attackBase;
		iDmgMax = iDmgMin + m_attackRange;

		// Horrific Beast (necro spell) changes char base damage to 5-15
		if (g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B)
		{
			CItem * pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if (pPoly && pPoly->m_itSpell.m_spell == SPELL_Horrific_Beast)
			{
				iDmgMin += pPoly->m_itSpell.m_PolyStr;
				iDmgMax += pPoly->m_itSpell.m_PolyDex;
			}
		}
	}

	if ( m_pPlayer )	// only players can have damage bonus
	{
		int iIncreaseDam = (int)GetPropNum(COMP_PROPS_CHAR, PROPCH_INCREASEDAM, true);
		int iDmgBonus = minimum(iIncreaseDam, 100);		// Damage Increase is capped at 100%

		// Racial Bonus (Berserk), gargoyles gains +15% Damage Increase per each 20 HP lost
		if ((g_Cfg.m_iRacialFlags & RACIALF_GARG_BERSERK) && IsGargoyle())
		{
			int iStrDiff = (Stat_GetMaxAdjusted(STAT_STR) - Stat_GetVal(STAT_STR));
			iDmgBonus += minimum(15 * (iStrDiff / 20), 60);		// value is capped at 60%
		}

		// Horrific Beast (necro spell) add +25% Damage Increase
		if (g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B)
		{
			CItem * pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if (pPoly && pPoly->m_itSpell.m_spell == SPELL_Horrific_Beast)
				iDmgBonus += 25;
		}

		switch ( g_Cfg.m_iCombatDamageEra )
		{
			default:
			case 0:
			{
				// Sphere damage bonus (custom)
				if ( !iStatBonus )
					iStatBonus = STAT_STR;
				if ( !iStatBonusPercent )
					iStatBonusPercent = 10;
				iDmgBonus += Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;
				break;
			}

			case 1:
			{
				// pre-AOS damage bonus
				iDmgBonus += (Skill_GetBase(SKILL_TACTICS) - 500) / 10;

				iDmgBonus += Skill_GetBase(SKILL_ANATOMY) / 50;
				if ( Skill_GetBase(SKILL_ANATOMY) >= 1000 )
					iDmgBonus += 10;

				if ( pWeapon != nullptr && pWeapon->IsType(IT_WEAPON_AXE) )
				{
					iDmgBonus += Skill_GetBase(SKILL_LUMBERJACKING) / 50;
					if ( Skill_GetBase(SKILL_LUMBERJACKING) >= 1000 )
						iDmgBonus += 10;
				}

				if ( !iStatBonus )
					iStatBonus = STAT_STR;
				if ( !iStatBonusPercent )
					iStatBonusPercent = 20;
				iDmgBonus += Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;
				break;
			}

			case 2:
			{
				// AOS damage bonus
				iDmgBonus += Skill_GetBase(SKILL_TACTICS) / 16;
				if ( Skill_GetBase(SKILL_TACTICS) >= 1000 )
					iDmgBonus += 6;		//6.25

				iDmgBonus += Skill_GetBase(SKILL_ANATOMY) / 20;
				if ( Skill_GetBase(SKILL_ANATOMY) >= 1000 )
					iDmgBonus += 5;

				if ( pWeapon != nullptr && pWeapon->IsType(IT_WEAPON_AXE) )
				{
					iDmgBonus += Skill_GetBase(SKILL_LUMBERJACKING) / 50;
					if ( Skill_GetBase(SKILL_LUMBERJACKING) >= 1000 )
						iDmgBonus += 10;
				}

				if ( Stat_GetAdjusted(STAT_STR) >= 100 )
					iDmgBonus += 5;

				if ( !iStatBonus )
					iStatBonus = STAT_STR;
				if ( !iStatBonusPercent )
					iStatBonusPercent = 30;
				iDmgBonus += Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;
				break;
			}
		}

		iDmgMin += iDmgMin * iDmgBonus / 100;
		iDmgMax += iDmgMax * iDmgBonus / 100;
	}

	if ( bNoRandom )
		return( bGetMax ? iDmgMax : iDmgMin );
	else
		return( Calc_GetRandVal2(iDmgMin, iDmgMax) );
}

bool CChar::Fight_IsAttackable()
{
	ADDTOCALLSTACK("CChar::IsAttackable");
	return !IsDisconnected() && !IsStatFlag(STATF_DEAD|STATF_STONE|STATF_INVISIBLE|STATF_INSUBSTANTIAL|STATF_HIDDEN|STATF_INVUL);
}

// Clear all my active targets. Toggle out of war mode.
void CChar::Fight_ClearAll()
{
	ADDTOCALLSTACK("CChar::Fight_ClearAll");
	Attacker_Clear();
	if ( Fight_IsActive() )
	{
		Skill_Start(SKILL_NONE);
		m_Fight_Targ_UID.InitUID();
	}

    m_atFight.m_iRecoilDelay = 0;
    m_atFight.m_iSwingAnimationDelay = 0;
    m_atFight.m_iSwingAnimation = 0;
    m_atFight.m_iSwingIgnoreLastHitTag = 0;

	UpdateModeFlag();
}

// I no longer want to attack this char.
bool CChar::Fight_Clear(CChar *pChar, bool bForced)
{
	ADDTOCALLSTACK("CChar::Fight_Clear");
	if ( !pChar || !Attacker_Delete(pChar, bForced, ATTACKER_CLEAR_FORCED) )
		return false;

    m_atFight.m_iRecoilDelay = 0;
    m_atFight.m_iSwingAnimationDelay = 0;
    m_atFight.m_iSwingAnimation = 0;
    m_atFight.m_iSwingIgnoreLastHitTag = 0;

	// Go to my next target.
	if (m_Fight_Targ_UID == pChar->GetUID())
		m_Fight_Targ_UID.InitUID();

	if ( m_pNPC )
	{
		pChar = NPC_FightFindBestTarget();
		if ( pChar )
			Fight_Attack(pChar);
		else
			Fight_ClearAll();
	}

	return (pChar != nullptr);	// I did not know about this ?
}

// We want to attack some one.
// But they won't notice til we actually hit them.
// This is just my intent.
// RETURN:
//  true = new attack is accepted.
bool CChar::Fight_Attack( CChar *pCharTarg, bool btoldByMaster )
{
	ADDTOCALLSTACK("CChar::Fight_Attack");

	if ( !pCharTarg || pCharTarg == this || pCharTarg->IsStatFlag(STATF_DEAD) || IsStatFlag(STATF_DEAD) )
	{
		// Not a valid target.
		Fight_Clear(pCharTarg, true);
		return false;
	}
	else if ( GetPrivLevel() <= PLEVEL_Guest && pCharTarg->m_pPlayer && pCharTarg->GetPrivLevel() > PLEVEL_Guest )
	{
		SysMessageDefault(DEFMSG_MSG_GUEST);
		Fight_Clear(pCharTarg);
		return false;
	}
	else if ( m_pNPC && !CanSee(pCharTarg) )
	{
		Attacker_Delete(pCharTarg, true, ATTACKER_CLEAR_DISTANCE);
		Skill_Start(SKILL_NONE);
		return false;
	}

	CChar *pTarget = pCharTarg;

	if ( g_Cfg.m_fAttackingIsACrime )
	{
		if ( pCharTarg->Noto_GetFlag(this) == NOTO_GOOD )
			CheckCrimeSeen(SKILL_NONE, pTarget, nullptr, nullptr);
	}

	int64 threat = 0;
	if ( btoldByMaster )
		threat = ATTACKER_THREAT_TOLDBYMASTER + Attacker_GetHighestThreat();

	if ( ((IsTrigUsed(TRIGGER_ATTACK)) || (IsTrigUsed(TRIGGER_CHARATTACK))) && m_Fight_Targ_UID != pCharTarg->GetUID() )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = threat;
		if ( OnTrigger(CTRIG_Attack, pTarget, &Args) == TRIGRET_RET_TRUE )
			return false;
		threat = Args.m_iN1;
	}

    if (!Attacker_Add(pTarget, threat))
    {
        return false;
    }
    if (Attacker_GetIgnore(pTarget))
    {
        return false;
    }

	// I'm attacking (or defending)
	if ( !IsStatFlag(STATF_WAR) )
	{
		StatFlag_Set(STATF_WAR);
		UpdateModeFlag();
		if ( IsClient() )
			GetClient()->addPlayerWarMode();
	}

	SKILL_TYPE skillWeapon = Fight_GetWeaponSkill();
	SKILL_TYPE skillActive = Skill_GetActive();

    if ((skillActive == skillWeapon) && (m_Fight_Targ_UID == pCharTarg->GetUID()))	// already attacking this same target using the same skill
    {
        return true;
    }
    else if (g_Cfg.IsSkillFlag(skillActive, SKF_MAGIC))	// don't start another fight skill when already casting spells
    {
        return true;
    }

    if (m_pNPC && !btoldByMaster)		// call FindBestTarget when this CChar is a NPC and was not commanded to attack, otherwise it attack directly
    {
        pTarget = NPC_FightFindBestTarget();
    }

	m_Fight_Targ_UID = pTarget ? pTarget->GetUID() : CUID(UID_UNUSED);
	Skill_Start(skillWeapon);
	return true;
}

// A timer has expired so try to take a hit.
// I am ready to swing or already swinging.
// but i might not be close enough.
void CChar::Fight_HitTry()
{
	ADDTOCALLSTACK("CChar::Fight_HitTry");

	ASSERT( Fight_IsActive() );

	CChar *pCharTarg = m_Fight_Targ_UID.CharFind();
	if ( !pCharTarg || (pCharTarg && !pCharTarg->Fight_IsAttackable()) )
	{
		// I can't hit this target, try switch to another one
		if (m_pNPC)
		{
			if ( !Fight_Attack(NPC_FightFindBestTarget()) )
			{
				Skill_Start(SKILL_NONE);
				m_Fight_Targ_UID.InitUID();
				if ( m_pNPC )
					StatFlag_Clear(STATF_WAR);
			}
		}
		return;
	}

    bool fPreHit_HadInstaHit = false, fPreHit_LastHitTag_Newer = false;
    int64 iPreHit_LastHitTag_FullHit = 0;  // Time required to perform a normal hit, without the PreHit delay reduction.
    if (m_atFight.m_War_Swing_State == WAR_SWING_EQUIPPING)
    {
        if (IsSetCombatFlags(COMBAT_PREHIT))
        {
            fPreHit_HadInstaHit = (!m_atFight.m_iRecoilDelay && !m_atFight.m_iSwingAnimationDelay);
            Fight_SetDefaultSwingDelays();
            const int64 iTimeCur = g_World.GetCurrentTime().GetTimeRaw() / MSECS_PER_TENTH;
            // Time required to perform the previous normal hit, without the PreHit delay reduction.
            const int64 iPreHit_LastHitTag_FullHit_Prev = GetKeyNum("LastHit");   // TAG.LastHit is in tenths of second
            // Time required to perform the shortened hit with PreHit.
            const int64 iPreHit_LastHitTag_PreHit = iTimeCur + COMBAT_MIN_SWING_ANIMATION_DELAY;   // it's the new m_iRecoilDelay (0) + the new m_iSwingAnimationDelay (COMBAT_MIN_SWING_ANIMATION_DELAY)
            iPreHit_LastHitTag_FullHit = iTimeCur + m_atFight.m_iRecoilDelay + m_atFight.m_iSwingAnimationDelay;
            if (iPreHit_LastHitTag_PreHit > iPreHit_LastHitTag_FullHit_Prev)
            {
                fPreHit_LastHitTag_Newer = true;
                if (fPreHit_HadInstaHit)
                {
                    // First hit with PreHit -> no recoil, only the minimum swing animation delay
                    m_atFight.m_iSwingIgnoreLastHitTag = 1;
                    m_atFight.m_iRecoilDelay = 0;
                    m_atFight.m_iSwingAnimationDelay = COMBAT_MIN_SWING_ANIMATION_DELAY;
                }
            }
        }
        else
        {
            Fight_SetDefaultSwingDelays();
        }
    }

    WAR_SWING_TYPE retHit = Fight_Hit(pCharTarg);

    if (IsSetCombatFlags(COMBAT_PREHIT))
    {
        // This needs to be set after the @HitTry call, because we may want to change the default iRecoilDelay and iSwingAnimationDelay in that trigger.
        if ((m_atFight.m_War_Swing_State == WAR_SWING_READY) && (retHit == WAR_SWING_READY))
        {
            if (fPreHit_LastHitTag_Newer)
            {
                // This protects against allowing shortened hits every time a char stops and starts attacking again, independently of this being the first, second, third or whatever hit.
                SetKeyNum("LastHit", iPreHit_LastHitTag_FullHit);
            }
            if (!fPreHit_HadInstaHit)
            {
                // This workaround is needed because, without it, if the player exits and enters war mode after having landed the first but not the second hit,
                //  resets the old delays and there's nothing to check if he can do again a PreHit, so he again hits near-instantly.
                // This happens only between the first and second hit because Tag.LastHit is set in the first hit @HitTry trigger and ignored only for the second hit.
                m_atFight.m_iSwingIgnoreLastHitTag = 0;
            }
        }
    }

	switch ( retHit )
	{
		case WAR_SWING_INVALID:		// target is invalid
		{
			Fight_Clear(pCharTarg);
			if ( m_pNPC )
            {
				Fight_Attack(NPC_FightFindBestTarget());
            }
			return;
		}
		case WAR_SWING_EQUIPPING:	// keep hitting the same target
        case WAR_SWING_EQUIPPING_NOWAIT:
		{
            if ((m_atFight.m_War_Swing_State == WAR_SWING_EQUIPPING)
                || (IsSetCombatFlags(COMBAT_SWING_NORANGE) && (m_atFight.m_War_Swing_State == WAR_SWING_READY)) ) // Ready to start a new swing, check swingTypeHold in Fight_Hit
            {
                if (retHit == WAR_SWING_EQUIPPING_NOWAIT)
                {
                    // Reactivate as soon as possible (without waiting for a new tick) the fighting routines, which are normally called by OnTick(), which in turn calls
                    //  OnTickSkill() -> Skill_Done() -> Skill_Stage() -> Skill_Fighting() ->
                    //  -> Fight_HitTry() (which is this method) -> Fight_Hit() (which sets the recoil and swing delays and more) ...
                    // If i use SetTimeout(1), i will lose a tick, since i'll start to set the timers for the new swing only on the next tick, not on the current.
                    OnTickSkill();
                }
                else
                {
                
                    // Wait a bit, then check again if i can hit. If i don't wait, the condition that leaded to this point will always be the same,
                    //  and the combat code and this function will be called recursively.
                    SetTimeoutD(1);
                }
            }
			return;
		}
		case WAR_SWING_READY:		// probably too far away, can't take my swing right now
		{
			if ( m_pNPC )
            {
				Fight_Attack(NPC_FightFindBestTarget());	// keep attacking the same char or change the targ
            }
            if (!IsTimerSet())	// If i haven't landed the hit yet...
            {
                // Player & NPC: wait some time and check again if i can land the hit
                // NPC: also keeps its AI alive, so that in NPCActFight the NPC can further approach his target.
                SetTimeoutD(1);
            }
			return;
		}
		case WAR_SWING_SWINGING:	// must come back here again to complete
            if (!IsTimerSet())
            {
                // This happens (only with both PreHit and Swing_NoRange on) if i can't land the hit right now, otherwise retHit
                //  should be WAR_SWING_EQUIPPING. If this isn't the case, there's something wrong (asserts are placed to intercept this situations).
                SetTimeoutD(1);
                ASSERT(IsSetCombatFlags(COMBAT_PREHIT|COMBAT_SWING_NORANGE));
            }
			return;
		default:
			break;
	}

	ASSERT(0);
}

// Distance from which I can hit
int CChar::Fight_CalcRange( CItem * pWeapon ) const
{
	ADDTOCALLSTACK("CChar::Fight_CalcRange");

	int iCharRange = GetRangeH();
	int iWeaponRange = pWeapon ? pWeapon->GetRangeH() : 0;

	return ( maximum(iCharRange , iWeaponRange) );
}

void CChar::Fight_SetDefaultSwingDelays()
{
    ADDTOCALLSTACK("CChar::Fight_SetDefaultSwingDelays");
    // Be wary that with the new animation packet the anim delay ("speed") is always 1.0 s, no matter the value we send, and then the char will keep waiting the remaining time

    int iAttackSpeed = g_Cfg.Calc_CombatAttackSpeed(this, m_uidWeapon.ItemFind());
    if (IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH))
    {
        m_atFight.m_iRecoilDelay = 0;    // We don't have an actual recoil: the hit animation has the duration of the delay between hits, so the char is always doing a smooth, slow attack animation
        m_atFight.m_iSwingAnimationDelay = (int16)(maximum(iAttackSpeed,COMBAT_MIN_SWING_ANIMATION_DELAY) - COMBAT_MIN_SWING_ANIMATION_DELAY);
    }
    else 
    {
        m_atFight.m_iRecoilDelay = (int16)(iAttackSpeed - COMBAT_MIN_SWING_ANIMATION_DELAY);
        m_atFight.m_iSwingAnimationDelay = COMBAT_MIN_SWING_ANIMATION_DELAY;
    }
}

WAR_SWING_TYPE CChar::Fight_CanHit(CChar * pCharSrc, bool fSwingNoRange)
{
	ADDTOCALLSTACK("CChar::Fight_CanHit");
	// Very basic check on possibility to hit
	//return:
	//  WAR_SWING_INVALID	= target is invalid
	//	WAR_SWING_EQUIPPING	= recoiling weapon / swing made
	//  WAR_SWING_READY		= Ready to hit, will switch to WAR_SWING_SWINGING ASAP.
	//  WAR_SWING_SWINGING	= taking my swing now
    if (IsDisconnected()) // Was the char deleted? (without this check, the server would crash!)
    {
        return WAR_SWING_INVALID;
    }
    else if (IsStatFlag(STATF_DEAD) || !pCharSrc->Fight_IsAttackable())
    {
        return WAR_SWING_INVALID;
    }
    else if (IsStatFlag(STATF_SLEEPING | STATF_FREEZE | STATF_STONE))
    {
        return WAR_SWING_EQUIPPING; // Can't hit now, but may want to hit later.
    }
    if (pCharSrc->m_pArea && pCharSrc->m_pArea->IsFlag(REGION_FLAG_SAFE))
    {
        return WAR_SWING_INVALID;
    }

    // Ignore the distance and the line of sight if fSwingNoRange is true, but only if i'm starting the swing. To land the hit i need to be in range.
    if (!fSwingNoRange ||
        (IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH) && (m_atFight.m_War_Swing_State == WAR_SWING_SWINGING)) || 
        (!IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH) && (m_atFight.m_War_Swing_State == WAR_SWING_READY)))
    {
        int dist = GetTopDist3D(pCharSrc);
        if (dist > GetVisualRange())
        {
            if (!IsSetCombatFlags(COMBAT_STAYINRANGE))
                return WAR_SWING_SWINGING; //Keep loading the hit or keep it loaded and ready.

            return WAR_SWING_INVALID;
        }
        word wLOSFlags = (g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_RANGED )) ? LOS_NB_WINDOWS : 0;
        if (!CanSeeLOS(pCharSrc, wLOSFlags, true))
            return IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH) ? WAR_SWING_SWINGING : WAR_SWING_EQUIPPING;
    }

	// I am on ship. Should be able to combat only inside the ship to avoid free sea and ground characters hunting
	if ((m_pArea != pCharSrc->m_pArea) && !IsSetCombatFlags(COMBAT_ALLOWHITFROMSHIP))
	{
		if (m_pArea && m_pArea->IsFlag(REGION_FLAG_SHIP))
		{
			SysMessageDefault(DEFMSG_COMBAT_OUTSIDESHIP);
			return WAR_SWING_INVALID;
		}
		if (pCharSrc->m_pArea && pCharSrc->m_pArea->IsFlag(REGION_FLAG_SHIP))
		{
			SysMessageDefault(DEFMSG_COMBAT_INSIDESHIP);
			return WAR_SWING_INVALID;
		}
	}

	return WAR_SWING_READY;
}

// Attempt to hit our target.
// Calculating damage
// Damaging target ( OnTakeDamage() / @GetHit )
// pCharTarg = the target.
// RETURN:
//  WAR_SWING_INVALID	= target is invalid
//  WAR_SWING_EQUIPPING	= recoiling weapon / swing made
//  WAR_SWING_READY		= can't take my swing right now. but I'm ready to hit
//  WAR_SWING_SWINGING	= taking my swing now
WAR_SWING_TYPE CChar::Fight_Hit( CChar * pCharTarg )
{
	ADDTOCALLSTACK("CChar::Fight_Hit");

	if ( !pCharTarg || (pCharTarg == this) )
		return WAR_SWING_INVALID;

    CItem *pWeapon = m_uidWeapon.ItemFind();
	DAMAGE_TYPE iDmgType = Fight_GetWeaponDamType(pWeapon);
    bool fSwingNoRange = (bool)(IsSetCombatFlags(COMBAT_SWING_NORANGE));

	if ( IsTrigUsed(TRIGGER_HITCHECK) )
	{
		CScriptTriggerArgs pArgs;
		pArgs.m_iN1 = m_atFight.m_War_Swing_State;
		pArgs.m_iN2 = iDmgType;
        pArgs.m_VarsLocal.SetNum("Recoil_NoRange", (int)fSwingNoRange);
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_HitCheck, pCharTarg, &pArgs);
		if ( tRet == TRIGRET_RET_TRUE )
			return (WAR_SWING_TYPE)pArgs.m_iN1;
		if ( tRet == -1 )
			return WAR_SWING_INVALID;

		m_atFight.m_War_Swing_State = (WAR_SWING_TYPE)(pArgs.m_iN1);
        iDmgType = (DAMAGE_TYPE)(pArgs.m_iN2);
        fSwingNoRange = (bool)pArgs.m_VarsLocal.GetKeyNum("Recoil_NoRange");

        if (tRet != -2)     // if @HitCheck returns -2, just continue with the hardcoded stuff
        {
            if ( (m_atFight.m_War_Swing_State == WAR_SWING_SWINGING) && (iDmgType & DAMAGE_FIXED) )
            {
                if ( tRet == TRIGRET_RET_DEFAULT )
                    return WAR_SWING_EQUIPPING;

                if ( iDmgType == DAMAGE_HIT_BLUNT )		// if type did not change in the trigger, default iDmgType is set
                {
                    pWeapon = m_uidWeapon.ItemFind();
                    iDmgType = Fight_GetWeaponDamType(pWeapon);
                }
                if ( iDmgType & DAMAGE_FIXED )
                    iDmgType &= ~DAMAGE_FIXED;

                const CCPropsChar *pCCPChar = GetCCPropsChar(), *pBaseCCPChar = Base_GetDef()->GetCCPropsChar();

                pCharTarg->OnTakeDamage(
                    Fight_CalcDamage(m_uidWeapon.ItemFind()),
                    this,
                    iDmgType,
                    (int)GetPropNum(pCCPChar, PROPCH_DAMPHYSICAL, pBaseCCPChar),
                    (int)GetPropNum(pCCPChar, PROPCH_DAMFIRE,     pBaseCCPChar),
                    (int)GetPropNum(pCCPChar, PROPCH_DAMCOLD,     pBaseCCPChar),
                    (int)GetPropNum(pCCPChar, PROPCH_DAMPOISON,   pBaseCCPChar),
                    (int)GetPropNum(pCCPChar, PROPCH_DAMENERGY,   pBaseCCPChar)
                );

                return WAR_SWING_EQUIPPING;
            }
        }
	}

	WAR_SWING_TYPE iHitCheck = Fight_CanHit(pCharTarg, fSwingNoRange);
	if (iHitCheck != WAR_SWING_READY)
		return iHitCheck;

	// Guards should remove conjured NPCs
	if ( m_pNPC && (m_pNPC->m_Brain == NPCBRAIN_GUARD) && pCharTarg->m_pNPC && pCharTarg->IsStatFlag(STATF_CONJURED) )
	{
		pCharTarg->Delete();
		return WAR_SWING_EQUIPPING; //WAR_SWING_EQUIPPING_NOWAIT;
	}

    // Fix of the bounce back effect with dir update for clients to be able to run in combat easily
    if ( IsClient() && IsSetCombatFlags(COMBAT_FACECOMBAT) )
    {
        DIR_TYPE dirOpponent = GetDir(pCharTarg, m_dirFace);
        if ( (dirOpponent != m_dirFace) && (dirOpponent != GetDirTurn(m_dirFace, -1)) && (dirOpponent != GetDirTurn(m_dirFace, 1)) )
            return WAR_SWING_READY;
    }

    if ( IsSetCombatFlags(COMBAT_PREHIT) && (m_atFight.m_War_Swing_State == WAR_SWING_EQUIPPING) && (!m_atFight.m_iSwingIgnoreLastHitTag) )
    {
        int64 iTimeDiff = ((g_World.GetCurrentTime().GetTimeRaw() / MSECS_PER_TENTH) - GetKeyNum("LastHit"));
        if (iTimeDiff < 0)
        {
            return WAR_SWING_EQUIPPING;
        }
    }    
	
	CItem *pAmmo = nullptr;
	const SKILL_TYPE skill = Skill_GetActive();
	const int dist = GetTopDist3D(pCharTarg);
	const bool bSkillRanged = g_Cfg.IsSkillFlag(skill, SKF_RANGED);

	if (bSkillRanged)
	{
		if ( IsStatFlag(STATF_HASSHIELD) )		// this should never happen
		{
			SysMessageDefault(DEFMSG_ITEMUSE_BOW_SHIELD);
			return WAR_SWING_INVALID;
		}
		else if ( !IsSetCombatFlags(COMBAT_ARCHERYCANMOVE) && !IsStatFlag(STATF_ARCHERCANMOVE) )
		{
			// Only start the swing this much tenths of second after the char stopped moving.
			//  (Values changed between expansions. SE:0,25s / AOS:0,5s / pre-AOS:1,0s)
			if ( m_pClient && ( -(g_World.GetTimeDiff(m_pClient->m_timeLastEventWalk) / MSECS_PER_TENTH) < g_Cfg.m_iCombatArcheryMovementDelay) )
				return WAR_SWING_EQUIPPING;
		}

        if ( pWeapon )
        {
            CResourceIDBase ridAmmo = pWeapon->Weapon_GetRangedAmmoRes();
            if ( ridAmmo )
            {
                pAmmo = pWeapon->Weapon_FindRangedAmmo(ridAmmo);
                if ( !pAmmo && m_pPlayer )
                {
                    SysMessageDefault(DEFMSG_COMBAT_ARCH_NOAMMO);
                    return WAR_SWING_INVALID;
                }
            }
        }

        if (!fSwingNoRange || (m_atFight.m_War_Swing_State == WAR_SWING_SWINGING))
        {
            // If we are using Swing_NoRange, we can start the swing regardless of the distance, but we can land the hit only when we are at the right distance
            const WAR_SWING_TYPE swingTypeHold = fSwingNoRange ? m_atFight.m_War_Swing_State : WAR_SWING_READY;

            int	iMinDist = pWeapon ? pWeapon->GetRangeL() : g_Cfg.m_iArcheryMinDist;
            int	iMaxDist = pWeapon ? pWeapon->GetRangeH() : g_Cfg.m_iArcheryMaxDist;
            if ( !iMaxDist || (iMinDist == 0 && iMaxDist == 1) )
                iMaxDist = g_Cfg.m_iArcheryMaxDist;
            if ( !iMinDist )
                iMinDist = g_Cfg.m_iArcheryMinDist;

            if ( dist < iMinDist )
            {
                SysMessageDefault(DEFMSG_COMBAT_ARCH_TOOCLOSE);
                if ( !IsSetCombatFlags(COMBAT_STAYINRANGE) || (m_atFight.m_War_Swing_State != WAR_SWING_SWINGING) )
                    return swingTypeHold;
                return WAR_SWING_EQUIPPING;
            }
            else if ( dist > iMaxDist )
            {
                if ( !IsSetCombatFlags(COMBAT_STAYINRANGE) || (m_atFight.m_War_Swing_State != WAR_SWING_SWINGING) )
                    return swingTypeHold;
                return WAR_SWING_EQUIPPING;
            }
        }		
	}
	else
	{
        if (!fSwingNoRange || (m_atFight.m_War_Swing_State == WAR_SWING_SWINGING))
        {
            // If we are using PreHit_NoRange, we can start the swing regardless of the distance, but we can land the hit only when we are at the right distance
            const WAR_SWING_TYPE swingTypeHold = fSwingNoRange ? m_atFight.m_War_Swing_State : WAR_SWING_READY;

		    int	iMinDist = pWeapon ? pWeapon->GetRangeL() : 0;
		    int	iMaxDist = Fight_CalcRange(pWeapon);
		    if ( (dist < iMinDist) || (dist > iMaxDist) )
		    {
			    if ( !IsSetCombatFlags(COMBAT_STAYINRANGE) || (m_atFight.m_War_Swing_State != WAR_SWING_SWINGING) )
				    return swingTypeHold;
			    return WAR_SWING_EQUIPPING;
		    }
        }
	}

    // Do i have to wait for the recoil time?
    if (m_atFight.m_War_Swing_State == WAR_SWING_EQUIPPING)
    {
        m_atFight.m_iSwingAnimation = (int16)GenerateAnimate(ANIM_ATTACK_WEAPON);

        if ( IsTrigUsed(TRIGGER_HITTRY) )
        {
            int16& iARGN1Var = IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH) ? m_atFight.m_iSwingAnimationDelay : m_atFight.m_iRecoilDelay;
            int16& iAnimDelayVar = IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH) ? m_atFight.m_iRecoilDelay : m_atFight.m_iSwingAnimationDelay;

            CScriptTriggerArgs Args(iARGN1Var, 0, pWeapon);
            Args.m_VarsLocal.SetNum("Anim", m_atFight.m_iSwingAnimation);
            Args.m_VarsLocal.SetNum("AnimDelay", iAnimDelayVar);
            if ( OnTrigger(CTRIG_HitTry, pCharTarg, &Args) == TRIGRET_RET_TRUE )
                return WAR_SWING_READY;

            m_atFight.m_iSwingAnimation = (int16)(Args.m_VarsLocal.GetKeyNum("Anim"));
            iARGN1Var = (int16)(Args.m_iN1);
            iAnimDelayVar = (int16)(Args.m_VarsLocal.GetKeyNum("AnimDelay"));
            //if (m_atFight.m_iSwingAnimation < (ANIM_TYPE)-1)
            //    m_atFight.m_iSwingAnimation = (int16)animSwingDefault;
            if ( m_atFight.m_iRecoilDelay < 1 )
                m_atFight.m_iRecoilDelay = 1;
            if ( m_atFight.m_iSwingAnimationDelay < 0 )
                m_atFight.m_iSwingAnimationDelay = 0;
        }

        SetTimeoutD(m_atFight.m_iRecoilDelay);   // Wait for the recoil time.
        m_atFight.m_War_Swing_State = WAR_SWING_READY;
        return WAR_SWING_READY;
    }

	// I have waited for the recoil time, then i can start the swing
	if ( m_atFight.m_War_Swing_State == WAR_SWING_READY )
	{
		m_atFight.m_War_Swing_State = WAR_SWING_SWINGING;
		Reveal();

		if ( !IsSetCombatFlags(COMBAT_NODIRCHANGE) )
        {
			UpdateDir(pCharTarg);
        }
        byte iSwingAnimationDelayInSeconds = (byte)(m_atFight.m_iSwingAnimationDelay / 10);
        //if ((m_atFight.m_iSwingAnimationDelay % 10) >= 5)
        //    iSwingAnimationDelayInSeconds += 1; // round up
		UpdateAnimate((ANIM_TYPE)m_atFight.m_iSwingAnimation, false, false, maximum(0,iSwingAnimationDelayInSeconds) );

        // Now that i have waited the recoil time, start the hit animation and wait for it to end
        SetTimeoutD(m_atFight.m_iSwingAnimationDelay);
		return WAR_SWING_SWINGING;
	}

	if ( bSkillRanged && pWeapon )
	{
		// Post-swing behavior
		ITEMID_TYPE AnimID = ITEMID_NOTHING;
		dword AnimHue = 0, AnimRender = 0;
		pWeapon->Weapon_GetRangedAmmoAnim(AnimID, AnimHue, AnimRender);
		pCharTarg->Effect(EFFECT_BOLT, AnimID, this, 18, 1, false, AnimHue, AnimRender);

		if ( m_pClient && (skill == SKILL_THROWING) )		// throwing weapons also have anim of the weapon returning after throw it
		{
			m_pClient->m_timeLastSkillThrowing = g_World.GetCurrentTime().GetTimeRaw();
			m_pClient->m_pSkillThrowingTarg = pCharTarg;
			m_pClient->m_SkillThrowingAnimID = AnimID;
			m_pClient->m_SkillThrowingAnimHue = AnimHue;
			m_pClient->m_SkillThrowingAnimRender = AnimRender;
		}
	}

	// We made our swing. Apply the damage and start the recoil.
	m_atFight.m_War_Swing_State = WAR_SWING_EQUIPPING;

	// We missed
	if ( m_Act_Difficulty < 0 )
	{
		if ( IsTrigUsed(TRIGGER_HITMISS) )
		{
			CScriptTriggerArgs Args(0, 0, pWeapon);
			if ( pAmmo )
				Args.m_VarsLocal.SetNum("Arrow", pAmmo->GetUID());
			if ( OnTrigger(CTRIG_HitMiss, pCharTarg, &Args) == TRIGRET_RET_TRUE )
				return WAR_SWING_EQUIPPING_NOWAIT;

			if ( Args.m_VarsLocal.GetKeyNum("ArrowHandled") != 0 )		// if arrow is handled by script, do nothing with it further!
				pAmmo = nullptr;
		}

		if ( pAmmo && m_pPlayer && (40 >= Calc_GetRandVal(100)) )
		{
			pAmmo->UnStackSplit(1);
			pAmmo->MoveToDecay(pCharTarg->GetTopPoint(), g_Cfg.m_iDecay_Item);
		}

		if ( IsPriv(PRIV_DETAIL) )
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_MISSS), pCharTarg->GetName());
		if ( pCharTarg->IsPriv(PRIV_DETAIL) )
			pCharTarg->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_MISSO), GetName());

		SOUND_TYPE iSound = SOUND_NONE;
		if ( pWeapon )
			iSound = pWeapon->Weapon_GetSoundMiss();
		if ( iSound == SOUND_NONE)
		{
			if ( g_Cfg.IsSkillFlag(skill, SKF_RANGED) )
			{
				static const SOUND_TYPE sm_Snd_Miss_Ranged[] = { 0x233, 0x238 };
				iSound = sm_Snd_Miss_Ranged[Calc_GetRandVal(CountOf(sm_Snd_Miss_Ranged))];
			}
			else
			{
				static const SOUND_TYPE sm_Snd_Miss[] = { 0x238, 0x239, 0x23a };
				iSound = sm_Snd_Miss[Calc_GetRandVal(CountOf(sm_Snd_Miss))];
			}
		}
		Sound(iSound);

		return WAR_SWING_EQUIPPING_NOWAIT;
	}

	// We hit
	if ( !(iDmgType & DAMAGE_GOD) )
	{
		CItem * pItemHit = nullptr;
		if (pCharTarg->Fight_Parry(pItemHit))
		{
			if ( IsPriv(PRIV_DETAIL) )
				SysMessageDefault(DEFMSG_COMBAT_PARRY);
			if ( pItemHit )
				pItemHit->OnTakeDamage(1, this, iDmgType);

			//Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16);		// moved to scripts (@UseQuick on Parrying skill)
			return WAR_SWING_EQUIPPING_NOWAIT;
		}
	}

	// Calculate base damage
	int	iDmg = Fight_CalcDamage(pWeapon);

	CScriptTriggerArgs Args(iDmg, iDmgType, pWeapon);
	Args.m_VarsLocal.SetNum("ItemDamageChance", 40);
	if ( pAmmo )
		Args.m_VarsLocal.SetNum("Arrow", pAmmo->GetUID());

	if ( IsTrigUsed(TRIGGER_SKILLSUCCESS) )
	{
		if ( Skill_OnCharTrigger(skill, CTRIG_SkillSuccess) == TRIGRET_RET_TRUE )
		{
			Skill_Cleanup();
			return WAR_SWING_EQUIPPING;		// ok, so no hit - skill failed. Pah!
		}
	}
	if ( IsTrigUsed(TRIGGER_SUCCESS) )
	{
		if ( Skill_OnTrigger(skill, SKTRIG_SUCCESS) == TRIGRET_RET_TRUE )
		{
			Skill_Cleanup();
			return WAR_SWING_EQUIPPING;		// ok, so no hit - skill failed. Pah!
		}
	}

	if ( IsTrigUsed(TRIGGER_HIT) )
	{
		if ( OnTrigger(CTRIG_Hit, pCharTarg, &Args) == TRIGRET_RET_TRUE )
			return WAR_SWING_EQUIPPING;

		if ( Args.m_VarsLocal.GetKeyNum("ArrowHandled") != 0 )		// if arrow is handled by script, do nothing with it further
			pAmmo = nullptr;

		iDmg = (int)(Args.m_iN1);
        iDmgType = (DAMAGE_TYPE)(Args.m_iN2);
	}

	// BAD BAD Healing fix.. Cant think of something else -- Radiant
	if ( pCharTarg->m_Act_SkillCurrent == SKILL_HEALING )
	{
		pCharTarg->SysMessageDefault(DEFMSG_HEALING_INTERRUPT);
		pCharTarg->Skill_Cleanup();
	}

	if ( pAmmo )
	{
		if ( pCharTarg->m_pNPC && (40 >= Calc_GetRandVal(100)) )
		{
			pAmmo->UnStackSplit(1);
			pCharTarg->ItemBounce(pAmmo, false);
		}
		else
			pAmmo->ConsumeAmount(1);
	}

	// Hit noise (based on weapon type)
	SoundChar(CRESND_HIT);

	if ( pWeapon )
	{
		// Check if the weapon is poisoned
		if ( !IsSetCombatFlags(COMBAT_NOPOISONHIT) && pWeapon->m_itWeapon.m_poison_skill && 
            (pWeapon->m_itWeapon.m_poison_skill > Calc_GetRandVal(100) || pWeapon->m_itWeapon.m_poison_skill < 10))
		{
			byte iPoisonDeliver = (byte)(Calc_GetRandVal(pWeapon->m_itWeapon.m_poison_skill));
			pCharTarg->SetPoison(10 * iPoisonDeliver, iPoisonDeliver / 5, this);

			pWeapon->m_itWeapon.m_poison_skill -= iPoisonDeliver / 2;	// reduce weapon poison charges
			pWeapon->UpdatePropertyFlag();
		}

		// Check if the weapon will be damaged
		int iDamageChance = (int)(Args.m_VarsLocal.GetKeyNum("ItemDamageChance"));
		if ( iDamageChance > Calc_GetRandVal(100) )
			pWeapon->OnTakeDamage(iDmg, pCharTarg);
	}
	else if ( m_pNPC && (m_pNPC->m_Brain == NPCBRAIN_MONSTER) )
	{
		// Base poisoning for NPCs
		if ( !IsSetCombatFlags(COMBAT_NOPOISONHIT) && 50 >= Calc_GetRandVal(100) )
		{
			int iPoisoningSkill = Skill_GetBase(SKILL_POISONING);
			if ( iPoisoningSkill )
				pCharTarg->SetPoison(Calc_GetRandVal(iPoisoningSkill), Calc_GetRandVal(iPoisoningSkill / 50), this);
		}
	}

	// Took my swing. Do Damage !
    const CCPropsChar *pCCPChar = GetCCPropsChar(), *pBaseCCPChar = Base_GetDef()->GetCCPropsChar();
    pCharTarg->OnTakeDamage(
        Fight_CalcDamage(m_uidWeapon.ItemFind()),
        this,
        iDmgType,
        (int)GetPropNum(pCCPChar, PROPCH_DAMPHYSICAL, pBaseCCPChar),
        (int)GetPropNum(pCCPChar, PROPCH_DAMFIRE,     pBaseCCPChar),
        (int)GetPropNum(pCCPChar, PROPCH_DAMCOLD,     pBaseCCPChar),
        (int)GetPropNum(pCCPChar, PROPCH_DAMPOISON,   pBaseCCPChar),
        (int)GetPropNum(pCCPChar, PROPCH_DAMENERGY,   pBaseCCPChar)
    );

	if ( iDmg > 0 )
	{
		CItem *pCurseWeapon = LayerFind(LAYER_SPELL_Curse_Weapon);
		ushort uiHitLifeLeech = (ushort)GetPropNum(pCCPChar, PROPCH_HITLEECHLIFE, pBaseCCPChar);
		if ( pWeapon && pCurseWeapon )
			uiHitLifeLeech += pCurseWeapon->m_itSpell.m_spelllevel;

		bool bMakeLeechSound = false;
		if ( uiHitLifeLeech )
		{
			uiHitLifeLeech = (ushort)(Calc_GetRandVal2(0, (iDmg * uiHitLifeLeech * 30) / 10000));	// leech 0% ~ 30% of damage value
			UpdateStatVal(STAT_STR, uiHitLifeLeech);
			bMakeLeechSound = true;
		}

		ushort uiHitManaLeech = (ushort)GetPropNum(pCCPChar, PROPCH_HITLEECHMANA, pBaseCCPChar);
		if ( uiHitManaLeech )
		{
			uiHitManaLeech = (ushort)(Calc_GetRandVal2(0, (iDmg * uiHitManaLeech * 40) / 10000));	// leech 0% ~ 40% of damage value
			UpdateStatVal(STAT_INT, uiHitManaLeech);
			bMakeLeechSound = true;
		}

		if ( GetPropNum(pCCPChar, PROPCH_HITLEECHSTAM, pBaseCCPChar) > Calc_GetRandLLVal(100) )
		{
			UpdateStatVal(STAT_DEX, (ushort)iDmg);	// leech 100% of damage value
			bMakeLeechSound = true;
		}

		ushort uiManaDrain = 0;
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
		{
			CItem *pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if ( pPoly && pPoly->m_itSpell.m_spell == SPELL_Wraith_Form )
				uiManaDrain += 5 + (15 * Skill_GetBase(SKILL_SPIRITSPEAK) / 1000);
		}
		if ( GetPropNum(pCCPChar, PROPCH_HITMANADRAIN, pBaseCCPChar) > Calc_GetRandLLVal(100) )
			uiManaDrain += (ushort)IMulDivLL(iDmg, 20, 100);		// leech 20% of damage value

		ushort uiTargMana = pCharTarg->Stat_GetVal(STAT_INT);
		if ( uiManaDrain > uiTargMana )
			uiManaDrain = uiTargMana;

		if ( uiManaDrain > 0 )
		{
			pCharTarg->UpdateStatVal(STAT_INT, uiTargMana - uiManaDrain);
			UpdateStatVal(STAT_INT, uiManaDrain);
			bMakeLeechSound = true;
		}

		if ( bMakeLeechSound )
			Sound(0x44d);

		// Make blood effects
		if ( pCharTarg->m_wBloodHue != (HUE_TYPE)(-1) )
		{
			static const ITEMID_TYPE sm_Blood[] = { ITEMID_BLOOD1, ITEMID_BLOOD2, ITEMID_BLOOD3, ITEMID_BLOOD4, ITEMID_BLOOD5, ITEMID_BLOOD6, ITEMID_BLOOD_SPLAT };
			int iBloodQty = (g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE) ? Calc_GetRandVal2(4, 5) : Calc_GetRandVal2(1, 2);

			for ( int i = 0; i < iBloodQty; ++i )
			{
                ITEMID_TYPE iBloodID = sm_Blood[Calc_GetRandVal(CountOf(sm_Blood))];

                CPointMap pt = pCharTarg->GetTopPoint();
                pt.m_x += (short)Calc_GetRandVal2(-1, 1);
                pt.m_y += (short)Calc_GetRandVal2(-1, 1);
                EffectXYZ(EFFECT_XYZ, iBloodID, nullptr, &pt, 50, 0, false, pCharTarg->m_wBloodHue);
			}
		}

		// Check for passive skill gain
		if ( m_pPlayer && !pCharTarg->m_pArea->IsFlag(REGION_FLAG_NO_PVP) )
		{
			Skill_Experience(skill, m_Act_Difficulty);
			Skill_Experience(SKILL_TACTICS, m_Act_Difficulty);
		}
	}

	return WAR_SWING_EQUIPPING_NOWAIT;
}

bool CChar::Fight_Parry(CItem * &pItemParry)
{
	// Check if target will block the hit
	// Legacy pre-SE formula
    bool bCanShield = g_Cfg.m_iCombatParryingEra & PARRYERA_SHIELDBLOCK;
    bool bCanOneHanded = g_Cfg.m_iCombatParryingEra & PARRYERA_ONEHANDBLOCK;
    bool bCanTwoHanded = g_Cfg.m_iCombatParryingEra & PARRYERA_TWOHANDBLOCK;

    int iParrying = Skill_GetBase(SKILL_PARRYING);
    int iParryChance = 0;   // 0-100 difficulty! without the decimal!
    if (g_Cfg.m_iCombatParryingEra & PARRYERA_SEFORMULA)   // Samurai Empire formula
    {
        int iBushido = Skill_GetBase(SKILL_BUSHIDO);
        int iChanceSE = 0, iChanceLegacy = 0;

        if (bCanShield && IsStatFlag(STATF_HASSHIELD))	// parry using shield
        {
            pItemParry = LayerFind(LAYER_HAND2);
            iParryChance = (iParrying - iBushido) / 40;
            if ((iParrying >= 1000) || (iBushido >= 1000))
                iParryChance += 5;
            if (iParryChance < 0)
                iParryChance = 0;
        }
        else if (m_uidWeapon.IsItem())		// parry using weapon
        {
            CItem* pTempItemParry = m_uidWeapon.ItemFind();
            if (bCanOneHanded && (pTempItemParry->GetEquipLayer() == LAYER_HAND1))
            {
                pItemParry = pTempItemParry;

                iChanceSE = iParrying * iBushido / 48000;
                if ((iParrying >= 1000) || (iBushido >= 1000))
                    iChanceSE += 5;

                iChanceLegacy = iParrying / 80;
                if (iParrying >= 1000)
                    iChanceLegacy += 5;

                iParryChance = maximum(iChanceSE, iChanceLegacy);
            }
            else if (bCanTwoHanded && (pTempItemParry->GetEquipLayer() == LAYER_HAND2))
            {
                pItemParry = pTempItemParry;

                iChanceSE = iParrying * iBushido / 41140;
                if ((iParrying >= 1000) || (iBushido >= 1000))
                    iChanceSE += 5;

                iChanceLegacy = iParrying / 80;
                if (iParrying >= 1000)
                    iChanceLegacy += 5;

                iParryChance = maximum(iChanceSE, iChanceLegacy);
            }
        }
    }
    else    // Legacy formula (pre Samurai Empire)
    {
        if (bCanShield && IsStatFlag(STATF_HASSHIELD))	// parry using shield
        {
            pItemParry = LayerFind(LAYER_HAND2);
            iParryChance = iParrying / 40;
        }
        else if (m_uidWeapon.IsItem())		// parry using weapon
        {
            CItem* pTempItemParry =  m_uidWeapon.ItemFind();
            if ( (bCanOneHanded && (pTempItemParry->GetEquipLayer() == LAYER_HAND1)) ||
                 (bCanTwoHanded && (pTempItemParry->GetEquipLayer() == LAYER_HAND2)) )
            {
                pItemParry = pTempItemParry;
                iParryChance = iParrying / 80;
            }
        }

        if ((iParryChance > 0) && (iParrying >= 1000))
            iParryChance += 5;
    }

    if (iParryChance <= 0)
        return false;
	
    int iDex = Stat_GetAdjusted(STAT_DEX);
    if (iDex < 80)
    {
        const float fDexMod = (80 - iDex)/100.0f;
        iParryChance = int((float)iParryChance * (1.0f - fDexMod));
    }
        
    if (Skill_UseQuick(SKILL_PARRYING, iParryChance, true, false))
        return true;

	return false;
}
