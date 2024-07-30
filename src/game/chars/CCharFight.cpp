
// Fight/Criminal actions/Noto.

#include "../../network/send.h"
#include "../clients/CClient.h"
#include "../components/CCPropsChar.h"
#include "../components/CCPropsItemWeapon.h"
#include "../CWorldGameTime.h"
#include "../CWorldSearch.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharNPC.h"


// 1.0 seconds is the minimum animation duration ("delay")
static constexpr int16 kiMinSwingAnimationDelay = 10; // in tenths of second

// I noticed a crime.
void CChar::OnNoticeCrime( CChar * pCriminal, CChar * pCharMark )
{
	ADDTOCALLSTACK("CChar::OnNoticeCrime");
	if ( !pCriminal || pCriminal == this || pCriminal == pCharMark || pCriminal->IsPriv(PRIV_GM) || (pCriminal->m_pNPC && pCriminal->GetNPCBrain() == NPCBRAIN_GUARD) )
		return;

	if (pCharMark)
	{
		const NOTO_TYPE iNoto = pCharMark->Noto_GetFlag(pCriminal);
		if (iNoto == NOTO_CRIMINAL || iNoto == NOTO_EVIL)
			return;
	}

	// Make my owner criminal too (if I have one)
	/*CChar * pOwner = pCriminal->GetOwner();
	if ( pOwner != nullptr && pOwner != this )
		OnNoticeCrime( pOwner, pCharMark );*/

	if ( m_pPlayer )
	{
		// I have the option of attacking the criminal. or calling the guards.
		bool fMakeCriminal = false; //We don't need to call guards automatically in default.
		if (IsTrigUsed(TRIGGER_SEECRIME))
		{
			CScriptTriggerArgs Args;
			Args.m_iN1 = fMakeCriminal;
			Args.m_pO1 = pCharMark;
			OnTrigger(CTRIG_SeeCrime, pCriminal, &Args);
            fMakeCriminal = Args.m_iN1 ? true : false;
		}
        Memory_AddObjTypes(pCriminal, MEMORY_SAWCRIME); //Memory should always be added to the player.
        if (fMakeCriminal) //We call guards automatically if ARGN1 set to 1 (true) in trigger.
        {
            pCriminal->Noto_Criminal(pCharMark, true);
        }
		return;
	}

	// NPC's can take other actions.

	ASSERT(m_pNPC);
	bool fMyMaster = NPC_IsOwnedBy( pCriminal );

	if ( this != pCharMark )	// it's not me.
	{
		if ( fMyMaster )	// I won't rat you out.
			return;
        Memory_AddObjTypes( pCriminal, MEMORY_SAWCRIME );
	}
	else
	{
		Memory_AddObjTypes( pCriminal, MEMORY_SAWCRIME );
		// I being the victim can retaliate.
		OnHarmedBy( pCriminal );
	}

	if ( !NPC_CanSpeak() )
		return;	// I can't talk anyhow.

    pCriminal->Noto_Criminal(pCharMark);

    ASSERT(m_pArea);
    if (m_pArea->IsGuarded())
    {
        if (m_pNPC->m_Brain != NPCBRAIN_GUARD)
            Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_CRIM));
        CallGuards( pCriminal );
    }
}

// I am commiting a crime.
// Did others see me commit or try to commit the crime.
//  SkillToSee = NONE = everyone can notice this.
//	pCharMark = offended char.
// RETURN:
//  true = somebody saw me.
bool CChar::CheckCrimeSeen( SKILL_TYPE SkillToSee, CChar * pCharMark, const CObjBase * pItem, lpctstr pAction )
{
	ADDTOCALLSTACK("CChar::CheckCrimeSeen");
    // Who notices ?
	bool fSeen = false;

	if (m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD) // guards only fight for justice, they can't commit a crime!!?
		return false;

	auto AreaChars = CWorldSearchHolder::GetInstance( GetTopPoint(), g_Cfg.m_iMapViewSize );
	for (;;)
	{
		CChar * pChar = AreaChars->GetChar();
		if ( pChar == nullptr )
			break;
        if (this == pChar) // Ignore the player himself.
            continue;
        if (pChar == pCharMark) // Attacked player should be ignored.
            continue;   
		if (pChar->IsPriv(PRIV_GM)) // GMs also should be ignored.
			continue;
		if ( ! pChar->CanSeeLOS( this, LOS_NB_WINDOWS )) // What if I was standing behind a window when I saw a crime? :)
			continue;

        const bool fYour = (pCharMark && ( pCharMark == pChar ));
        if (!g_Cfg.Calc_CrimeSeen(this, pChar, SkillToSee, fYour))
            continue;
		
		tchar *z = Str_GetTemp();
		if ( pItem && pAction )
		{
			if ( pCharMark )
				snprintf(z, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_2), GetName(), pAction, fYour ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_YOUR) : pCharMark->GetName(), fYour ? "" : g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_S), pItem->GetName());
			else
				snprintf(z, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_1), GetName(), pAction, pItem->GetName());
		}
		
		// They are not a criminal til someone calls the guards !!!
		if ( SkillToSee == SKILL_SNOOPING )
		{
			if (IsTrigUsed(TRIGGER_SEESNOOP))
			{
				CScriptTriggerArgs Args(pAction);
				Args.m_iN1 = SkillToSee;
				Args.m_iN2 = pItem ? (dword)pItem->GetUID() : 0;    // here i can modify pItem via scripts, so it isn't really const
				Args.m_pO1 = pCharMark;
				TRIGRET_TYPE iRet = pChar->OnTrigger(CTRIG_SeeSnoop, this, &Args);

				if (iRet == TRIGRET_RET_TRUE)
					continue;
			}

            fSeen = true;
            
			// Off chance of being a criminal. (hehe)
			if ( g_Rand.GetVal(100) < g_Cfg.m_iSnoopCriminal )
				pChar->OnNoticeCrime( this, pCharMark );

            if ( pChar->m_pNPC )
                pChar->NPC_OnNoticeSnoop( this, pCharMark );

            if (fYour)
                pCharMark->Memory_AddObjTypes(this, MEMORY_IRRITATEDBY);

            pChar->ObjMessage(z, this);
		}
		else
		{
            fSeen = true;
			pChar->OnNoticeCrime( this, pCharMark );

            pChar->ObjMessage(z, this);
		}
	}
	return fSeen;
}

void CChar::CallGuards()
{
	ADDTOCALLSTACK("CChar::CallGuards");
    if (!m_pPlayer && (!m_pArea || !m_pArea->IsGuarded()))
        return;
	if (IsStatFlag(STATF_DEAD|STATF_STONE))
		return;

    // Spam check, not calling this more than once per second, which will cause an excess of calls and checks on crowded areas because of the 2 CWorldSearch.
	if (CWorldGameTime::GetCurrentTime().GetTimeDiff(_iTimeLastCallGuards + (1 * MSECS_PER_SEC)) <= 0)
		return;

	// We don't have any target yet, let's check everyone nearby
	CChar * pCriminal;
	auto AreaCrime = CWorldSearchHolder::GetInstance(GetTopPoint(), g_Cfg.m_iMapViewSize);
	while ((pCriminal = AreaCrime->GetChar()) != nullptr)
	{
		if (pCriminal == this)
			continue;
		if (!CanDisturb(pCriminal))	// don't allow guards to be called on someone we can't disturb
			continue;

		// Mark person as criminal if I saw him criming
		// Only players call guards this way. NPC's flag criminal instantly
        if (m_pPlayer && Memory_FindObjTypes(pCriminal, MEMORY_SAWCRIME))
        {
            pCriminal->Noto_Criminal(this, true);
        }
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
    ASSERT(pCriminal);
	if ( !m_pArea || (pCriminal == this) )
		return false;
	if (IsStatFlag(STATF_DEAD) ||
        (pCriminal->IsStatFlag(STATF_DEAD) || pCriminal->Can(CAN_C_STATUE|CAN_C_NONSELECTABLE) || pCriminal->IsPriv(PRIV_GM) || !pCriminal->m_pArea->IsGuarded()))
    {
		return false;
    }

    if (CWorldGameTime::GetCurrentTime().GetTimeDiff(_iTimeLastCallGuards + (25 * MSECS_PER_TENTH)) <= 0)	// Spam check
        return false;

    _iTimeLastCallGuards = CWorldGameTime::GetCurrentTime().GetTimeRaw();

	CChar *pGuard = nullptr;
	if (m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD)
	{
		// I'm a guard, why summon someone else to do my work? :)
		pGuard = this;
	}
	else
	{
		// Search for a free guards nearby
		auto AreaGuard = CWorldSearchHolder::GetInstance(GetTopPoint(), UO_MAP_VIEW_SIGHT);
		CChar *pGuardFound = nullptr;
		while ((pGuardFound = AreaGuard->GetChar()) != nullptr)
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

		if ( (uint)Args.m_iN1 != rid.GetResIndex())
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
		pGuard->Spell_Effect_Create(SPELL_Summon, LAYER_SPELL_Summon, g_Cfg.GetSpellEffect(SPELL_Summon, 1000), (g_Cfg.m_iGuardLingerTime/MSECS_PER_TENTH));
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
	if ( m_pNPC)
	{
		if (Skill_GetActive() == NPCACT_RIDDEN) //prevent action 111 to be changed.
			return;
	}
	if (fFightActive && m_Fight_Targ_UID.CharFind())
	{
		// In war mode already
		if ( m_pPlayer )
			return;
		if ( g_Rand.Get16ValFast( 10 ))
			return;
		// NPC will Change targets.
	}

	if (!IsSetCombatFlags(COMBAT_NOPETDESERT) && m_pNPC && NPC_IsOwnedBy(pCharSrc, false))
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
	if (IsStatFlag(STATF_DEAD|STATF_STONE))
		return false;

	if (fShouldReveal)
		pCharSrc->Reveal();	// fix invis exploit

	// Am i already attacking the source anyhow
	if ( Fight_IsActive() && (m_Fight_Targ_UID == pCharSrc->GetUID()) )
		return true;

    bool fAggreived = false;
    word wMemTypes = MEMORY_HARMEDBY | MEMORY_IRRITATEDBY;
    if (!pCharSrc->Memory_FindObjTypes(this, MEMORY_AGGREIVED))
    {
        // I'm the one being attacked first
        fAggreived = true;
        wMemTypes |= MEMORY_AGGREIVED;
    }
    Memory_AddObjTypes(pCharSrc, wMemTypes);
	Attacker_Add(pCharSrc);

	if ((Noto_GetFlag(pCharSrc) == NOTO_GOOD) && fAggreived )
	{
		if (IsClientActive())	// I decide if this is a crime.
		{
            OnNoticeCrime(pCharSrc, this);
            CChar* pCharMark = pCharSrc->IsStatFlag(STATF_PET) ? pCharSrc->NPC_PetGetOwner() : nullptr;
            if (pCharMark != nullptr)
                OnNoticeCrime(pCharMark, this);
		}
		else
		{
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

	for (CSObjContRec* pObjRec : *this)
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
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
					Displayed AR = ((Parrying Skill * Base AR of Shield) ÷ 200) + 1
					For all shields the maximum AR will be reached before your parry skill reaches GM level.
					Also note that the maximum displayed AR for a shield cannot exceed Base AR ÷ 2.
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

 int CChar::CalcPercentArmorDefense(LAYER_TYPE layer) //static
{
	 ADDTOCALLSTACK("CChar::CalcPercentArmorDefense");
	 int iPercentArmorDefence = 0;
	 switch (layer)
	 {
	 case LAYER_HELM:	//15% from head location.
		 iPercentArmorDefence = sm_ArmorLayers[0].m_iCoverage;
		 break;
	 case LAYER_COLLAR: //7% from neck location.
		 iPercentArmorDefence = sm_ArmorLayers[1].m_iCoverage;
		 break;
	 case LAYER_SHIRT: //LAYER_SHIRT, LAYER_CHEST and LAYER_TUNIC get the 35% of AR  from chest location.
	 case LAYER_CHEST:
	 case LAYER_TUNIC: 
		 iPercentArmorDefence = sm_ArmorLayers[3].m_iCoverage;
		 break;
	 case LAYER_ROBE:	//35% from chest, 22% from legs and 14% from arms locations.
		 iPercentArmorDefence = sm_ArmorLayers[3].m_iCoverage + sm_ArmorLayers[4].m_iCoverage + sm_ArmorLayers[6].m_iCoverage;
		 break;
	 case LAYER_CAPE: //Both get 14% from arms location.
	 case LAYER_ARMS:
		 iPercentArmorDefence = sm_ArmorLayers[4].m_iCoverage;
		 break;
	 case LAYER_GLOVES: //Gloves get 7% from hands location.
		 iPercentArmorDefence = sm_ArmorLayers[5].m_iCoverage;
		 break;
	 case LAYER_PANTS: //LAYER_PANTS, LAYER_SKIRT, LAYER_HALF_APRON and LAYER_LEGS get a 22% of AR from legs location.
	 case LAYER_SKIRT:
	 case LAYER_HALF_APRON:
	 case LAYER_LEGS:
		 iPercentArmorDefence = sm_ArmorLayers[6].m_iCoverage;
		 break;
	 case LAYER_HAND2:	//By default Shields get a 7% armor coverage, if PARRYERA_ARSCALING is enabled they get a 100% armor coverage.
		 iPercentArmorDefence = sm_ArmorLayers[5].m_iCoverage;
		 if (g_Cfg.m_iCombatParryingEra & PARRYERA_ARSCALING)
		 {
			 iPercentArmorDefence = sm_ArmorLayers[8].m_iCoverage;
		 }
		 break;
	 default:	//Back and Feet location provide no protections (0%).
		 break;
	 }
	 return iPercentArmorDefence;
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
int CChar::OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType, int iDmgPhysical, int iDmgFire, int iDmgCold, int iDmgPoison, int iDmgEnergy, SPELL_TYPE spell)
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
		//if ( pSrc->m_pNPC && (pSrc->NPC_PetGetOwner() == this) && (pSrc->m_pNPC->m_Brain != NPCBRAIN_BERSERK) )
			//goto effect_bounce;
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
		if ( pEvilOmen && !g_Cfg.GetSpellDef(SPELL_Evil_Omen)->IsSpellType(SPELLFLAG_SCRIPTED))
		{
			iDmg += iDmg / 4;
			pEvilOmen->Delete();
		}

		CItem * pBloodOath = LayerFind(LAYER_SPELL_Blood_Oath);
		if ( pBloodOath && pBloodOath->m_uidLink == pSrc->GetUID() && !(uType & DAMAGE_FIXED) && !g_Cfg.GetSpellDef(SPELL_Blood_Oath)->IsSpellType(SPELLFLAG_SCRIPTED))	// if DAMAGE_FIXED is set we are already receiving a reflected damage, so we must stop here to avoid an infinite loop.
		{
			iDmg += iDmg / 10;
			pSrc->OnTakeDamage(iDmg * (100 - pBloodOath->m_itSpell.m_spelllevel) / 100, this, DAMAGE_MAGIC|DAMAGE_FIXED,0,0,0,0,0,SPELL_Blood_Oath);
		}
	}

	const CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	const CCPropsChar* pCCPChar = GetComponentProps<CCPropsChar>();
	const CCPropsChar* pBaseCCPChar = pCharDef->GetComponentProps<CCPropsChar>();

	// MAGICF_IGNOREAR bypasses defense completely
	if ( (uType & DAMAGE_MAGIC) && IsSetMagicFlags(MAGICF_IGNOREAR) )
		uType |= DAMAGE_FIXED;
	
	bool fElemental = IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE);
	// Apply armor calculation
	if ( !(uType & (DAMAGE_GOD|DAMAGE_FIXED)) )
	{
		if ( fElemental )
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

			int iArMax = iArmorRating * g_Rand.Get16Val2Fast(7,35) / 100;
			int iArMin = iArMax / 2;

			int iDef = g_Rand.GetVal2Fast( iArMin, (iArMax - iArMin) + 1 );
			if ( uType & DAMAGE_MAGIC )		// magical damage halves effectiveness of defense
				iDef /= 2;

			iDmg -= iDef;
			if (iDmg <= 0)
				iDmg = 0;
		}
	}

	CScriptTriggerArgs Args( iDmg, uType, (int64)(0) );
	Args.m_VarsLocal.SetNum("ItemDamageLayer", sm_ArmorDamageLayers[(size_t)g_Rand.Get16ValFast(ARRAY_COUNT(sm_ArmorDamageLayers))]);
	Args.m_VarsLocal.SetNum("ItemDamageChance", 25);
	Args.m_VarsLocal.SetNum("Spell", (int)spell);

	if ( fElemental )
	{
		Args.m_VarsLocal.SetNum("DamagePercentPhysical", iDmgPhysical);
		Args.m_VarsLocal.SetNum("DamagePercentFire", iDmgFire);
		Args.m_VarsLocal.SetNum("DamagePercentCold", iDmgCold);
		Args.m_VarsLocal.SetNum("DamagePercentPoison", iDmgPoison);
		Args.m_VarsLocal.SetNum("DamagePercentEnergy", iDmgEnergy);
	}

    CItem* pItemHit = nullptr;
	if ( IsTrigUsed(TRIGGER_GETHIT) )
	{
		if ( OnTrigger( CTRIG_GetHit, pSrc, &Args ) == TRIGRET_RET_TRUE )
			return 0;
		iDmg = (int)(Args.m_iN1);
		uType = (DAMAGE_TYPE)(Args.m_iN2);

        LAYER_TYPE iHitLayer = (LAYER_TYPE)(Args.m_VarsLocal.GetKeyNum("ItemDamageLayer"));
        pItemHit = LayerFind(iHitLayer);
        if (pItemHit)
        {
            Args.m_pO1 = this;
            // "ItemDamageLayer" will only be readable.
            if (pItemHit->OnTrigger(ITRIG_GetHit, pSrc, &Args) == TRIGRET_RET_TRUE)
                return 0;
            iDmg = (int)(Args.m_iN1); //Update damage amount and type again after @Hit trigger under item.
            uType = (DAMAGE_TYPE)(Args.m_iN2);
            // We don't need to update iHitLayer as it's already called on item
        }
	}

	int iItemDamageChance = (int)(Args.m_VarsLocal.GetKeyNum("ItemDamageChance"));
	if ( (iItemDamageChance > g_Rand.GetVal(100)) && !Can(CAN_C_NONHUMANOID) )
	{
		if ( pItemHit )
			pItemHit->OnTakeDamage(iDmg, pSrc, uType);
	}

    CSpellDef* pSpellDef = nullptr;
	// Remove stuck/paralyze effect
	if (!(uType & DAMAGE_NOUNPARALYZE))
	{
        if (spell)
            pSpellDef = g_Cfg.GetSpellDef(spell);

        if (!pSpellDef || (pSpellDef && !pSpellDef->IsSpellType(SPELLFLAG_NOUNPARALYZE))) // Block spells with SPELLFLAG_NOUNPARALYZE flag, unparalyze the target.
        {
            CItem* pParalyze = LayerFind(LAYER_SPELL_Paralyze);
            if (pParalyze)
                pParalyze->Delete();
        }

		CItem * pStuck = LayerFind(LAYER_FLAG_Stuck);
		if ( pStuck )
			pStuck->Delete();

		if ( IsStatFlag(STATF_FREEZE) )
		{
			StatFlag_Clear(STATF_FREEZE);
			UpdateModeFlag(); //We are going to update char flags, not the sight...
		}
	}

    if (IsSetCombatFlags(COMBAT_SLAYER))
    {
		CItem *pWeapon = nullptr;
		if (uType & DAMAGE_MAGIC)	// If the damage is magic, we are probably using a spell or a weapon that causes also magical damage.
		{
			pWeapon = pSrc->GetSpellbookLayer();	// Search for an equipped spellbook
			if ( !pWeapon ) //No spellbook, so it's a weapon causing magical damage.
				pWeapon = pSrc->m_uidWeapon.ItemFind();	// then force a weapon find.
		}
		else //Other types of damage.
		{
			pWeapon = pSrc->m_uidWeapon.ItemFind();	//  force a weapon find.
		}
        int iDmgBonus = 1;
        const CCFaction *pSlayer = nullptr;
        const CCFaction *pFaction = GetFaction();
        //const CCFaction *pSrcFaction = pSrc->GetFaction();
        if (pWeapon)
        {
            pSlayer = pWeapon->GetSlayer();
            if (pSlayer && pSlayer->GetFactionID() != FACTION_NONE)
            {
                if (m_pNPC) // I'm an NPC attacked (Should the attacker be a player to get the bonus?).
                {
                    if (pFaction && pFaction->GetFactionID() != FACTION_NONE)
                    {
                        iDmgBonus = pSlayer->GetSlayerDamageBonus(pFaction);
                    }
                }
                else if (m_pPlayer && pSrc->m_pNPC) // Wielding a slayer type against its opposite will cause the attacker to take more damage
                {
                    if (pFaction && pFaction->GetFactionID() != FACTION_NONE)
                    {
                        iDmgBonus = pSlayer->GetSlayerDamagePenalty(pFaction);
                    }
                }
            }
        }
        if (iDmgBonus == 1) // Couldn't find a weapon, a Slayer flag or a suitable flag for the target...
        {
            const CItem *pTalisman = pSrc->LayerFind(LAYER_TALISMAN); // then lets try with a Talisman
            if (pTalisman)
            {
                pSlayer = pTalisman->GetSlayer();
                if (pSlayer && pSlayer->GetFactionID() != FACTION_NONE)
                {
                    if (m_pNPC) // I'm an NPC attacked (Should the attacker be a player to get the bonus?).
                    {
                        if (pFaction && pFaction->GetFactionID() != FACTION_NONE)
                        {
                            iDmgBonus = pSlayer->GetSlayerDamageBonus(pFaction);
                        }
                    }
                    else if (m_pPlayer && pSrc->m_pNPC) // Wielding a slayer type against its opposite will cause the attacker to take more damage
                    {
                        if (pFaction && pFaction->GetFactionID() != FACTION_NONE)
                        {
                            iDmgBonus = pSlayer->GetSlayerDamagePenalty(pFaction);
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
	
	// Disturb magic spells (only players can be disturbed if NpCCanFizzleOnHit is false in sphere.ini)
	if ( (m_pPlayer || g_Cfg.m_fNPCCanFizzleOnHit) && (pSrc != this) && !(uType & DAMAGE_NODISTURB) && g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_MAGIC) )
	{
		// Check if my spell can be interrupted
		int iDisturbChance = 0;
		int iSpellSkill = -1;
		pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_iSpell);
		if ( pSpellDef && pSpellDef->GetPrimarySkill(&iSpellSkill) )
			iDisturbChance = pSpellDef->m_Interrupt.GetLinear(Skill_GetBase((SKILL_TYPE)iSpellSkill));

		if ( iDisturbChance && fElemental && !pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) ) //If Protection spell has SPELLFLAG_SCRIPTED don't make this check.
		{
			// Protection spell can cancel the disturb
			CItem *pProtectionSpell = LayerFind(LAYER_SPELL_Protection);
			if ( pProtectionSpell )
			{
				const int iChance = pProtectionSpell->m_itSpell.m_spelllevel;
				if ( iChance > g_Rand.GetVal(1000) )
					iDisturbChance = 0;
			}
		}

		if ( iDisturbChance > g_Rand.GetVal(1000) )
		{
			bool bInterrupt = true;
			if (IsTrigUsed(TRIGGER_SPELLINTERRUPT))
			{
				CScriptTriggerArgs ArgsInterrupt(m_atMagery.m_iSpell, iDisturbChance);
				if (pSrc->OnTrigger(CTRIG_SpellInterrupt, this, &ArgsInterrupt) == TRIGRET_RET_TRUE)
					bInterrupt = false;
			}

			if (bInterrupt)
				Skill_Fail();
		}
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
			attacker.ignore = false;
			m_lastAttackers.emplace_back(std::move(attacker));
		}

		// A physical blow of some sort.
		if (uType & (DAMAGE_HIT_BLUNT|DAMAGE_HIT_PIERCE|DAMAGE_HIT_SLASH))
		{
			// Check if Reactive Armor will reflect some damage back.
			// Preventing recurrent reflection with DAMAGE_REACTIVE.
			if ( IsStatFlag(STATF_REACTIVE) && !((uType & DAMAGE_GOD) || (uType & DAMAGE_REACTIVE)) )
			{
				if (GetTopDist3D(pSrc) <= 2)
				{
					CItem* pReactive = LayerFind(LAYER_SPELL_Reactive);
					
					if (pReactive)
					{
						int iReactiveDamage = (iDmg * pReactive->m_itSpell.m_PolyStr) / 100;
						if (iReactiveDamage < 1)
						{
							iReactiveDamage = 1;
						}

						iDmg -= iReactiveDamage;
						pSrc->OnTakeDamage(iReactiveDamage, this, (DAMAGE_TYPE)(DAMAGE_FIXED | DAMAGE_REACTIVE), iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy,(SPELL_TYPE)pReactive->m_itSpell.m_spell);
						pSrc->Sound(0x1F1);
						pSrc->Effect(EFFECT_OBJ, ITEMID_FX_CURSE_EFFECT, this, 10, 16);
					}
                }
            }
            // Check if REFLECTPHYSICALDAM will reflect some damage back.
            // Preventing recurrent reflection with DAMAGE_REACTIVE.
            if (!(uType & DAMAGE_REACTIVE))
            {
                int iReflectPhysical = (ushort)std::min(GetPropNum(pCCPChar, PROPCH_REFLECTPHYSICALDAM, pBaseCCPChar),250); //Capped to 250

                if (iReflectPhysical)
                {
                    int iReflectPhysicalDam = (iDmg * iReflectPhysical) / 100;
                    pSrc->OnTakeDamage(iReflectPhysicalDam, this, (DAMAGE_TYPE)(DAMAGE_FIXED | DAMAGE_REACTIVE), iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy);
                }
            }
			
		}
	}
	
	if (iDmg <= 0)
		return 0;

	// Apply damage
	SoundChar(CRESND_GETHIT);
	UpdateStatVal( STAT_STR, -iDmg);
	if ( pSrc->IsClientActive() )
		pSrc->GetClientActive()->addHitsUpdate( this );	// always send updates to src

	if ( IsAosFlagEnabled( FEATURE_AOS_DAMAGE ) )
	{
		if ( IsClientActive() )
			m_pClient->addShowDamage( iDmg, (dword)(GetUID()) );
		if ( pSrc->IsClientActive() && (pSrc != this) )
			pSrc->m_pClient->addShowDamage( iDmg, (dword)(GetUID()) );
		else
		{
			CChar * pSrcOwner = pSrc->GetOwner(); 
			if ( pSrcOwner != nullptr && pSrcOwner != this ) //If my pet damages somebody display the pop-up damage unless it's damaging me because i already received the pop-up damage on before.
			{
				if ( pSrcOwner->IsClientActive() )
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

	if (m_atFight.m_iWarSwingState != WAR_SWING_SWINGING)	// don't interrupt my swing animation
		UpdateAnimate(ANIM_GET_HIT);

	return iDmg;
}

void CChar::OnTakeDamageInflictArea(int iDmg, CChar* pSrc, DAMAGE_TYPE uType, int iDmgPhysical, int iDmgFire, int iDmgCold, int iDmgPoison, int iDmgEnergy, HUE_TYPE effectHue, SOUND_TYPE effectSound)
{
    ADDTOCALLSTACK("CChar::OnTakeDamageInflictArea");

    bool fMakeSound = false;
    
    int iDistance = 5;
    if (IsAosFlagEnabled(FEATURE_AOS_DAMAGE))
        iDistance=10; // 5 for ML and 10 for aos

    auto AreaChars = CWorldSearchHolder::GetInstance(GetTopPoint(), iDistance);
    for (;;)
        //pSrc = Char make the attack
        //pChar = Char scanned on the loop iteration
        //this = Char get the initial hit
    {
        CChar* pChar = AreaChars->GetChar();
        if (!pChar)
            break;
        if ((pChar == this) || (pChar == pSrc))                     //This char already receive the base hit. Damage already done
            continue;
        if (pChar->Fight_CanHit(pSrc,true) == WAR_SWING_INVALID)    //Check if target can be hit (I am invul, stone etc. Target is Disconnected,safe zone etc)
            continue;
        if (!pChar->m_pClient && pChar->NPC_IsOwnedBy(pSrc,false))	// it's my pet?
            continue;
        if (pChar->Noto_CalcFlag(pSrc) == NOTO_GOOD)                //Avoid to hit someone we can't legally attack (same guild, same party, Vendor etc)
            continue;
        if (!pChar->CanSeeLOS(pSrc))                                //Avoid hit someone in nearby house
            continue;

        /* On servUo they modify the damage depending of the distance with this formula
           There no info about this on UO Wiki 
           damage *= ( 11 - from.GetDistanceToSqrt( m ) ) / 10; */

        pChar->OnTakeDamage(iDmg, pSrc, uType, iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy);
        pChar->Effect(EFFECT_OBJ, ITEMID_FX_SPARKLE_2, this, 1, 15, false, effectHue);
        fMakeSound = true;
    }
    if (fMakeSound && (effectSound != SOUND_NONE))
        Sound(effectSound);
}

//*******************************************************************************
// Fight specific memories.

//********************************************************

byte CChar::GetRangeL() const
{
    if (_uiRange == 0)
        return Char_GetDef()->GetRangeL();
    return (byte)(RANGE_GET_LO(_uiRange));
}

byte CChar::GetRangeH() const
{
    if (_uiRange == 0)
        return Char_GetDef()->GetRangeH();
    return (byte)(RANGE_GET_HI(_uiRange));
}

// What sort of weapon am i using?
SKILL_TYPE CChar::Fight_GetWeaponSkill() const
{
	ADDTOCALLSTACK_DEBUG("CChar::Fight_GetWeaponSkill");
	const CItem * pWeapon = m_uidWeapon.ItemFind();
	if ( pWeapon == nullptr )
		return SKILL_WRESTLING;
	return pWeapon->Weapon_GetSkill();
}

DAMAGE_TYPE CChar::Fight_GetWeaponDamType(const CItem* pWeapon) const
{
    ADDTOCALLSTACK_DEBUG("CChar::Fight_GetWeaponDamType");
    DAMAGE_TYPE iDmgType = DAMAGE_HIT_BLUNT;
    if ( pWeapon )
    {
        const CVarDefCont *pDamTypeOverride = pWeapon->GetKey("OVERRIDE.DAMAGETYPE", true);
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

	const SKILL_TYPE iSkillActive = Skill_GetActive();
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
		return UINT16_MAX;	// swing made.

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

	if ( m_pPlayer || IsSetCombatFlags(COMBAT_NPC_BONUSDAMAGE))	
	{
		int iIncreaseDam = (int)GetPropNum(COMP_PROPS_CHAR, PROPCH_INCREASEDAM, true);
		int iDmgBonus = maximum(-100, minimum(iIncreaseDam, 100));		// Damage Increase is capped at +-100%

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

				if ( !iStatBonus )
					iStatBonus = STAT_STR;
				if ( !iStatBonusPercent )
					iStatBonusPercent = 30;
				if (Stat_GetAdjusted(iStatBonus) >= 100)
					iDmgBonus += 5;

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
		return( g_Rand.GetVal2(iDmgMin, iDmgMax) );
}

bool CChar::Fight_IsAttackableState()
{
	ADDTOCALLSTACK("CChar::IsAttackable");
	return !IsDisconnected() && !IsStatFlag(STATF_DEAD|STATF_STONE|STATF_INVISIBLE|STATF_INSUBSTANTIAL|STATF_HIDDEN|STATF_INVUL);
}

// Clear all my active targets. Toggle out of war mode.
void CChar::Fight_ClearAll()
{
	ADDTOCALLSTACK("CChar::Fight_ClearAll");
	if ( Fight_IsActive() )
	{
		Skill_Start(SKILL_NONE);
		m_Fight_Targ_UID.InitUID();
	}
	
    Attacker_Clear();
	m_atFight.m_iWarSwingState = WAR_SWING_EQUIPPING;
	m_atFight.m_iRecoilDelay = 0;
	m_atFight.m_iSwingAnimationDelay = 0;
	m_atFight.m_iSwingAnimation = 0;
	m_atFight.m_iSwingIgnoreLastHitTag = 0;

	SetKeyStr("LastHit", "");
	StatFlag_Clear(STATF_WAR);
	UpdateModeFlag();
}

// I no longer want to attack this char.
bool CChar::Fight_Clear(CChar *pChar, bool bForced)
{
	ADDTOCALLSTACK("CChar::Fight_Clear");
	if ( !pChar || !Attacker_Delete(pChar, bForced, ATTACKER_CLEAR_FORCED) )
		return false;

    m_atFight.m_iWarSwingState = WAR_SWING_EQUIPPING;
    m_atFight.m_iRecoilDelay = 0;
    m_atFight.m_iSwingAnimationDelay = 0;
    m_atFight.m_iSwingAnimation = 0;
    m_atFight.m_iSwingIgnoreLastHitTag = 0;

	CItemMemory* pMemoryFight =  Memory_FindObj(m_Fight_Targ_UID);
	if ( pMemoryFight && ( pMemoryFight->IsMemoryTypes(MEMORY_FIGHT) || pMemoryFight->IsMemoryTypes(MEMORY_IRRITATEDBY) ) )
		pMemoryFight->Delete();

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
bool CChar::Fight_Attack( CChar *pCharTarg, bool fToldByMaster )
{
	ADDTOCALLSTACK("CChar::Fight_Attack");

	if ( !pCharTarg || pCharTarg == this || pCharTarg->IsStatFlag(STATF_DEAD|STATF_INVUL|STATF_STONE) || IsStatFlag(STATF_DEAD) )
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

	int threat = 0;
	if (fToldByMaster)
	{
		threat = ATTACKER_THREAT_TOLDBYMASTER + Attacker_GetHighestThreat();
	}

    CChar *pTarget = pCharTarg;
	bool ignored = Attacker_GetIgnore(pTarget);
	if ( ((IsTrigUsed(TRIGGER_ATTACK)) || (IsTrigUsed(TRIGGER_CHARATTACK))) && m_Fight_Targ_UID != pCharTarg->GetUID() )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = threat;
		Args.m_iN2 = (int)ignored;
		if ( OnTrigger(CTRIG_Attack, pTarget, &Args) == TRIGRET_RET_TRUE )
			return false;
		threat = (int)Args.m_iN1;
		ignored = (bool)Args.m_iN2;		
	}
	Attacker_SetIgnore(pTarget, ignored);

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
		if ( IsClientActive() )
			GetClientActive()->addPlayerWarMode();
	}

	const SKILL_TYPE skillWeapon = Fight_GetWeaponSkill();
	const SKILL_TYPE skillActive = Skill_GetActive();

    if ((skillActive == skillWeapon) && (m_Fight_Targ_UID == pCharTarg->GetUID()))	// already attacking this same target using the same skill
    {
        return true;
    }
    if (g_Cfg.IsSkillFlag(skillActive, SKF_MAGIC))	// don't start another fight skill when already casting spells
    {
        return true;
    }

    pCharTarg->Memory_AddObjTypes(this, MEMORY_IRRITATEDBY);
    // Looking for MEMORY_AGGREIVED|MEMORY_HARMEDBY because in this case it won't be a crime, but most importantly to avoid infinite recursion
	if (g_Cfg.m_fAttackingIsACrime && (pCharTarg->Noto_GetFlag(this) == NOTO_GOOD) && !pCharTarg->Memory_FindObjTypes(this, MEMORY_AGGREIVED | MEMORY_HARMEDBY))
	{
		CheckCrimeSeen(SKILL_NONE, pTarget, nullptr, nullptr);
	}

    if (m_pNPC && !fToldByMaster)		// call FindBestTarget when this CChar is a NPC and was not commanded to attack, otherwise it attack directly
    {
        pTarget = NPC_FightFindBestTarget();
    }

	m_Fight_Targ_UID = pTarget ? pTarget->GetUID() : CUID();
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
	/*
	  We still need to check if player is hidden/invisible but do not need to make anything if the attacker is player, 
	  So we only check Statf_Dead,stone,insub and invul for players to avoid continue attack the not attackable targets.
	*/
	if ( !pCharTarg || (pCharTarg && pCharTarg->IsStatFlag(STATF_DEAD|STATF_STONE|STATF_INSUBSTANTIAL|STATF_INVUL)) || (pCharTarg->IsStatFlag(STATF_HIDDEN|STATF_INVISIBLE) && m_pNPC) )
	{
		// I can't hit this target, try switch to another one
		if (m_pNPC)
		{
			std::vector<CChar*> vExcludeTargets { pCharTarg };	// Ignore the current target, i want other npcs
			if (!Fight_Attack(NPC_FightFindBestTarget(&vExcludeTargets)))
			{
				Skill_Start(SKILL_NONE);
				m_Fight_Targ_UID.InitUID();
				if ( m_pNPC )
					StatFlag_Clear(STATF_WAR);
			}
		}
		else
		{
				Skill_Start(SKILL_NONE);
				m_Fight_Targ_UID.InitUID();
		}
		return;
	}
	
    bool fIH_ShouldInstaHit = false, fIH_LastHitTag_Newer = false;
    int64 iIH_LastHitTag_FullHit = 0;  // Time required to perform a normal hit, without the PreHit delay reduction.
    if (m_atFight.m_iWarSwingState == WAR_SWING_EQUIPPING)
    {
        if (IsSetCombatFlags(COMBAT_FIRSTHIT_INSTANT))
        {
            fIH_ShouldInstaHit = (!m_atFight.m_iRecoilDelay && !m_atFight.m_iSwingAnimationDelay);
            Fight_SetDefaultSwingDelays();
            const int64 iTimeCur = CWorldGameTime::GetCurrentTime().GetTimeRaw() / MSECS_PER_TENTH;
            // Time required to perform the previous normal hit, without the InstaHit delay reduction.
            const int64 iIH_LastHitTag_FullHit_Prev = GetKeyNum("LastHit");   // TAG.LastHit is in tenths of second
            // Time required to perform the shortened hit with InstaHit.
            const int64 iIH_LastHitTag_InstaHit = iTimeCur + m_atFight.m_iSwingAnimationDelay;   // it's the new m_iRecoilDelay (0) + the new m_iSwingAnimationDelay (COMBAT_MIN_SWING_ANIMATION_DELAY)
            iIH_LastHitTag_FullHit = iTimeCur + m_atFight.m_iRecoilDelay + m_atFight.m_iSwingAnimationDelay;
            if (iIH_LastHitTag_InstaHit > iIH_LastHitTag_FullHit_Prev)
            {
                fIH_LastHitTag_Newer = true;
                if (fIH_ShouldInstaHit && !iIH_LastHitTag_FullHit_Prev)
                {
                    // First hit with FirstHit_Instant -> no recoil, only the minimum swing animation delay
                    m_atFight.m_iSwingIgnoreLastHitTag = 1;
                    m_atFight.m_iRecoilDelay = 0;
                    if (IsSetCombatFlags(COMBAT_PREHIT))
                        m_atFight.m_iSwingAnimationDelay = 1;
                }
            }
        }
        else
        {
            Fight_SetDefaultSwingDelays();
        }
    }

    WAR_SWING_TYPE retHit = Fight_Hit(pCharTarg);

    if (IsSetCombatFlags(COMBAT_FIRSTHIT_INSTANT))
    {
        // This needs to be set after the @HitTry call, because we may want to change the default iRecoilDelay and iSwingAnimationDelay in that trigger.
        if ((m_atFight.m_iWarSwingState == WAR_SWING_READY) && (retHit == WAR_SWING_READY))
        {
            if (fIH_LastHitTag_Newer)
            {
                // This protects against allowing shortened hits every time a char stops and starts attacking again, independently of this being the first, second, third or whatever hit.
                SetKeyNum("LastHit", iIH_LastHitTag_FullHit);
            }
            if (!fIH_ShouldInstaHit)
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
            if ((m_atFight.m_iWarSwingState == WAR_SWING_EQUIPPING)
                || (IsSetCombatFlags(COMBAT_SWING_NORANGE) && (m_atFight.m_iWarSwingState == WAR_SWING_READY)) ) // Ready to start a new swing, check swingTypeHold in Fight_Hit
            {
                if (retHit == WAR_SWING_EQUIPPING_NOWAIT)
                {
                    // Reactivate as soon as possible (without waiting for a new tick) the fighting routines, which are normally called by _OnTick(), which in turn calls
                    //  OnTickSkill() -> Skill_Done() -> Skill_Stage() -> Skill_Fighting() ->
                    //  -> Fight_HitTry() (which is this method) -> Fight_Hit() (which sets the recoil and swing delays and more) ...
                    // If i use _SetTimeout(1), i will lose a tick, since i'll start to set the timers for the new swing only on the next tick, not on the current.
                    OnTickSkill();
                }
                else
                {
                    // Wait a bit, then check again if i can hit. If i don't wait, the condition that leaded to this point will always be the same,
                    //  and the combat code and this function will be called recursively.
                    _SetTimeoutD(1);
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
            if (!_IsTimerSet())	// If i haven't landed the hit yet...
            {
                // Player & NPC: wait some time and check again if i can land the hit
                // NPC: also keeps its AI alive, so that in NPCActFight the NPC can further approach his target.
                _SetTimeoutD(1);
            }
			return;
		}
		case WAR_SWING_SWINGING:	// must come back here again to complete
            if (!_IsTimerSet())
            {
                // This happens (only with both PreHit and Swing_NoRange on) if i can't land the hit right now, otherwise retHit
                //  should be WAR_SWING_EQUIPPING. If this isn't the case, there's something wrong (asserts are placed to intercept this situations).
                // Though, consider the case of custom combat systems, in that case the asserts may be invalid.
                _SetTimeoutD(1);
                //ASSERT(IsSetCombatFlags(COMBAT_FIRSTHIT_INSTANT) && IsSetCombatFlags(COMBAT_SWING_NORANGE|COMBAT_PREHIT));
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

	const int iCharRange = GetRangeH();
	const int iWeaponRange = pWeapon ? pWeapon->GetRangeH() : 0;
	return ( maximum(iCharRange , iWeaponRange) );
}

void CChar::Fight_SetDefaultSwingDelays()
{
    ADDTOCALLSTACK("CChar::Fight_SetDefaultSwingDelays");
    // Be wary that with the new animation packet the anim delay ("speed") is always 1.0 s, no matter the value we send, and then the char will keep waiting the remaining time.
    // With the old packet, the minimum anim delay is 1.0s, it doesn't matter if you send 0.

    int16 iAttackSpeed = int16(g_Cfg.Calc_CombatAttackSpeed(this, m_uidWeapon.ItemFind()));
    if (iAttackSpeed < kiMinSwingAnimationDelay)
        iAttackSpeed = kiMinSwingAnimationDelay;
    if (IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH))
    {
        m_atFight.m_iRecoilDelay = 0;    // We don't have an actual recoil: the hit animation has the duration of the delay between hits, so the char is always doing a smooth, slow attack animation
        m_atFight.m_iSwingAnimationDelay = iAttackSpeed;
    }
    else 
    {
        m_atFight.m_iRecoilDelay = (iAttackSpeed - kiMinSwingAnimationDelay);
        m_atFight.m_iSwingAnimationDelay = kiMinSwingAnimationDelay;
    }
    if (IsSetCombatFlags(COMBAT_PREHIT))
    {
        m_atFight.m_iRecoilDelay += m_atFight.m_iSwingAnimationDelay;
        m_atFight.m_iSwingAnimationDelay = 0;
    }
}

WAR_SWING_TYPE CChar::Fight_CanHit(CChar * pCharSrc, bool fSwingNoRange)
{
	ADDTOCALLSTACK("CChar::Fight_CanHit");
	//	Very basic check on possibility to hit
	//	return:
	//  WAR_SWING_INVALID	= target is invalid
	//	WAR_SWING_EQUIPPING	= recoiling weapon / swing made
	//  WAR_SWING_READY		= Ready to hit, will switch to WAR_SWING_SWINGING ASAP.
	//  WAR_SWING_SWINGING	= taking my swing now
  
	// We can't hit them. Char deleted? Target deleted? Am I dead or stoned? or Is target Dead, stone, invul, insub or slept?
	if (IsDisconnected() || pCharSrc->IsDisconnected() || IsStatFlag(STATF_DEAD | STATF_STONE) || (pCharSrc->IsStatFlag(STATF_DEAD | STATF_STONE | STATF_INVUL | STATF_INSUBSTANTIAL)) || (pCharSrc->IsSleeping()))
	{
		return WAR_SWING_INVALID;
	}
	// We can't hit them right now. Because we can't see them or reach them (invis/hidden).
	// Why the target is freeze we are change the attack type to swinging? Player can still attack paralyzed or sleeping characters.
	// We make sure that the target is freeze or sleeping must wait ready for attack!
	else if ( (pCharSrc->IsStatFlag(STATF_HIDDEN | STATF_INVISIBLE | STATF_SLEEPING)) || (IsStatFlag(STATF_FREEZE) && (!IsSetCombatFlags(COMBAT_PARALYZE_CANSWING))) || (IsStatFlag(STATF_SLEEPING)) ) // STATF_FREEZE | STATF_SLEEPING
	{
		return WAR_SWING_SWINGING;
	}
	if (pCharSrc->m_pArea && pCharSrc->m_pArea->IsFlag(REGION_FLAG_SAFE)) //Is area safe zone?
		return WAR_SWING_INVALID;

    // Ignore the distance and the line of sight if fSwingNoRange is true, but only if i'm starting the swing. To land the hit i need to be in range.
    if (!fSwingNoRange ||
        (IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH) && (m_atFight.m_iWarSwingState == WAR_SWING_SWINGING)) || 
        (!IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH) && (m_atFight.m_iWarSwingState == WAR_SWING_READY)))
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
            return WAR_SWING_SWINGING;
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
		pArgs.m_iN1 = m_atFight.m_iWarSwingState;
		pArgs.m_iN2 = iDmgType;
        pArgs.m_VarsLocal.SetNum("Recoil_NoRange", (int)fSwingNoRange);
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_HitCheck, pCharTarg, &pArgs);
		if ( tRet == TRIGRET_RET_TRUE )
			return (WAR_SWING_TYPE)pArgs.m_iN1;
		if ( tRet == -1 )
			return WAR_SWING_INVALID;

		m_atFight.m_iWarSwingState = (WAR_SWING_TYPE)(pArgs.m_iN1);
        iDmgType = (DAMAGE_TYPE)(pArgs.m_iN2);
        fSwingNoRange = (bool)pArgs.m_VarsLocal.GetKeyNum("Recoil_NoRange");

        if (tRet != -2)     // if @HitCheck returns -2, just continue with the hardcoded stuff
        {
            if ( (m_atFight.m_iWarSwingState == WAR_SWING_SWINGING) && (iDmgType & DAMAGE_FIXED) )
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

				const CCPropsChar* pCCPChar = GetComponentProps<CCPropsChar>();
				const CCPropsChar* pBaseCCPChar = Base_GetDef()->GetComponentProps<CCPropsChar>();

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
    if ( IsClientActive() && IsSetCombatFlags(COMBAT_FACECOMBAT) )
    {
        DIR_TYPE dirOpponent = GetDir(pCharTarg, m_dirFace);
        if ( (dirOpponent != m_dirFace) && (dirOpponent != GetDirTurn(m_dirFace, -1)) && (dirOpponent != GetDirTurn(m_dirFace, 1)) )
            return WAR_SWING_READY;
    }

    const WAR_SWING_TYPE iStageToSuspend = (IsSetCombatFlags(COMBAT_PREHIT) ? WAR_SWING_SWINGING : WAR_SWING_EQUIPPING);
    if ( IsSetCombatFlags(COMBAT_FIRSTHIT_INSTANT) && (!m_atFight.m_iSwingIgnoreLastHitTag) && (m_atFight.m_iWarSwingState == iStageToSuspend) )
    {
        const int64 iTimeDiff = ((CWorldGameTime::GetCurrentTime().GetTimeRaw() / MSECS_PER_TENTH) - GetKeyNum("LastHit"));
        if (iTimeDiff < 0)
        {
            return iStageToSuspend;
        }
    }    
	
	CItem *pAmmo = nullptr;
	const SKILL_TYPE skill = Skill_GetActive();
	const int dist = GetTopDist3D(pCharTarg);
	const bool fSkillRanged = g_Cfg.IsSkillFlag(skill, SKF_RANGED);

	if (fSkillRanged)
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
			if ( m_pClient && ( (CWorldGameTime::GetCurrentTime().GetTimeDiff(m_pClient->m_timeLastEventWalk) / MSECS_PER_TENTH) < g_Cfg.m_iCombatArcheryMovementDelay) )
				return WAR_SWING_EQUIPPING;
		}

        if ( pWeapon )
        {
            const CResourceID ridAmmo(pWeapon->Weapon_GetRangedAmmoRes());
	
			if (ridAmmo.IsValidUID() && ridAmmo.GetObjUID() > 0 ) 
            {
                pAmmo = pWeapon->Weapon_FindRangedAmmo(ridAmmo);
                if ( !pAmmo && m_pPlayer )
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
            if ( !iMaxDist || (iMinDist == 0 && iMaxDist == 1) )
                iMaxDist = g_Cfg.m_iArcheryMaxDist;
            if ( !iMinDist )
                iMinDist = g_Cfg.m_iArcheryMinDist;

            if ( dist < iMinDist )
            {
                SysMessageDefault(DEFMSG_COMBAT_ARCH_TOOCLOSE);
                if ( !IsSetCombatFlags(COMBAT_STAYINRANGE) || (m_atFight.m_iWarSwingState != WAR_SWING_SWINGING) )
                    return swingTypeHold;
                return WAR_SWING_EQUIPPING;
            }
            else if ( dist > iMaxDist )
            {
                if ( !IsSetCombatFlags(COMBAT_STAYINRANGE) || (m_atFight.m_iWarSwingState != WAR_SWING_SWINGING) )
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
		    if ( (dist < iMinDist) || (dist > iMaxDist) )
		    {
			    if ( !IsSetCombatFlags(COMBAT_STAYINRANGE) || (m_atFight.m_iWarSwingState != WAR_SWING_SWINGING) )
				    return swingTypeHold;
			    return WAR_SWING_EQUIPPING;
		    }
        }
	}

    // Do i have to wait for the recoil time?
    if (m_atFight.m_iWarSwingState == WAR_SWING_EQUIPPING)
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
            //if (m_atFight.m_iSwingAnimation < (ANIM_TYPE)-1)  // -1 is a valid value
            //    m_atFight.m_iSwingAnimation = (int16)animSwingDefault;
            if ( m_atFight.m_iRecoilDelay < 1 )
                m_atFight.m_iRecoilDelay = 1;
            if ( m_atFight.m_iSwingAnimationDelay < 0 )
                m_atFight.m_iSwingAnimationDelay = 0;
        }

        _SetTimeoutD(m_atFight.m_iRecoilDelay);   // Wait for the recoil time.
        m_atFight.m_iWarSwingState = WAR_SWING_READY;
        return WAR_SWING_READY;
    }

	// I have waited for the recoil time, then i can start the swing
	if ( m_atFight.m_iWarSwingState == WAR_SWING_READY )
	{
		if (fSwingNoRange) // We don't want the animation to display if we are outside of range.
		{
			int	iMinDist = pWeapon ? pWeapon->GetRangeL() : 0;
			int	iMaxDist = Fight_CalcRange(pWeapon);
			if (dist <  iMinDist || dist > iMaxDist)
				return WAR_SWING_READY;
		}
		m_atFight.m_iWarSwingState = WAR_SWING_SWINGING;
		Reveal();

		if ( !IsSetCombatFlags(COMBAT_NODIRCHANGE) && CanSee(pCharTarg) )
        {
			UpdateDir(pCharTarg);
        }
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
            //if ((m_atFight.m_iSwingAnimationDelay % 10) >= 5)
            //    iSwingAnimationDelayInSeconds += 1; // round up

        }
		UpdateAnimate((ANIM_TYPE)m_atFight.m_iSwingAnimation, false, false, iSwingAnimationDelayInSeconds );

        // Now that i have waited the recoil time, start the hit animation and wait for it to end
		/*
		// If COMBAT_ANIM_SMOOTH is set we can't set m_iSwingAnimationDelay as the timeout value because otherwise 
		there will be a delay between the end of the animation and the damage display. This is very noticeable when
		the m_iSwingAnimationDelay property is near the next digit. (If m_iSwingAnimationDelay is 3.8 the iSwingAnimationDelayInSeconds will be 3 and the damage will be displayed around a 0.8 second later!
		*/
		if (!IsSetCombatFlags(COMBAT_ANIM_HIT_SMOOTH))
			_SetTimeoutD(m_atFight.m_iSwingAnimationDelay);
		else 
			_SetTimeoutD(iSwingAnimationDelayInSeconds * TENTHS_PER_SEC);
		return WAR_SWING_SWINGING;
	}

	if ( fSkillRanged && pWeapon )
	{
		// Post-swing behavior
		ITEMID_TYPE AnimID = ITEMID_NOTHING;
		dword AnimHue = 0, AnimRender = 0;
		pWeapon->Weapon_GetRangedAmmoAnim(AnimID, AnimHue, AnimRender);
		pCharTarg->Effect(EFFECT_BOLT, AnimID, this, 18, 1, false, AnimHue, AnimRender);

		if ( m_pClient && (skill == SKILL_THROWING) )		// throwing weapons also have anim of the weapon returning after throw it
		{
			m_pClient->m_timeLastSkillThrowing = CWorldGameTime::GetCurrentTime().GetTimeRaw();
			m_pClient->m_pSkillThrowingTarg = pCharTarg;
			m_pClient->m_SkillThrowingAnimID = AnimID;
			m_pClient->m_SkillThrowingAnimHue = AnimHue;
			m_pClient->m_SkillThrowingAnimRender = AnimRender;
		}
	}

	// We made our swing. Apply the damage and start the recoil.
	m_atFight.m_iWarSwingState = WAR_SWING_EQUIPPING;

	// We missed
	if ( m_Act_Difficulty < 0 )
	{
		if ( IsTrigUsed(TRIGGER_HITMISS) )
		{
			CScriptTriggerArgs Args(0, 0, pWeapon);
			if ( pAmmo && pAmmo->GetUID().IsValidUID())
				Args.m_VarsLocal.SetNum("Arrow", (dword)pAmmo->GetUID());
			if ( OnTrigger(CTRIG_HitMiss, pCharTarg, &Args) == TRIGRET_RET_TRUE )
				return WAR_SWING_EQUIPPING_NOWAIT;

			if ( Args.m_VarsLocal.GetKeyNum("ArrowHandled") != 0 )		// if arrow is handled by script, do nothing with it further!
				pAmmo = nullptr;
		}

		if ( pAmmo && m_pPlayer  )
		{
			if (40 >= g_Rand.Get16ValFast(100))
			{
				pAmmo->UnStackSplit(1);
				pAmmo->MoveToDecay(pCharTarg->GetTopPoint(), g_Cfg.m_iDecay_Item);
			}
			else
				pAmmo->ConsumeAmount(1);
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
				static constexpr SOUND_TYPE sm_Snd_Miss_Ranged[] = { 0x233, 0x238 };
				iSound = sm_Snd_Miss_Ranged[(size_t)g_Rand.Get16ValFast(ARRAY_COUNT(sm_Snd_Miss_Ranged))];
			}
			else
			{
				static constexpr SOUND_TYPE sm_Snd_Miss[] = { 0x238, 0x239, 0x23a };
				iSound = sm_Snd_Miss[(size_t)g_Rand.Get16ValFast(ARRAY_COUNT(sm_Snd_Miss))];
			}
		}
		Sound(iSound);

		return WAR_SWING_EQUIPPING_NOWAIT;
	}

	// We hit
	// Calculate the damage and check for parrying
	int	iDmg = Fight_CalcDamage(pWeapon);
	int iParryReduction = 100;

	if ( !(iDmgType & DAMAGE_GOD) )
	{
		CItem* pItemHit = nullptr;
		SKILL_TYPE ParrySkill = SKILL_PARRYING;
		const CSkillDef* pSkillDef = g_Cfg.GetSkillDef(ParrySkill);
		int iParryChance = g_Cfg.Calc_CombatChanceToParry(pCharTarg, pItemHit);

		// If Effect property is defined on the Parrying skill use it instead of the hardcoded value of 100.
		if (!pSkillDef->m_vcEffect.m_aiValues.empty())
			iParryReduction = pSkillDef->m_vcEffect.GetLinear(pCharTarg->Skill_GetAdjusted(ParrySkill));
	
		/*
			ARGN1 = Percent of damage that will be reduced.
			ARGN2 =  Damage type.
			ARGO  = The weapon/shield used for parry, if any.
			local.ParryChance = The chance to parry, will be used in SkillUseQuick check, default 100.
			local.ParrySkill = The skill used for parrying, default is Parrying.
			local.ItemParryDamage = The chance that the parrying item will be damaged.
			local.Damage = The amount of damage (raw) before parrying reduction.
		*/
		CScriptTriggerArgs Args(iParryReduction, iDmgType, pItemHit);
		Args.m_VarsLocal.SetNum("ParryChance", iParryChance);
		Args.m_VarsLocal.SetNum("ParrySkillID", ParrySkill);
		Args.m_VarsLocal.SetNum("ItemParryDamageChance", 100);
		Args.m_VarsLocal.SetNum("Damage", iDmg);

		if (IsTrigUsed(TRIGGER_HITPARRY))
		{
			if (pCharTarg->OnTrigger(CTRIG_HitParry, this, &Args) == TRIGRET_RET_TRUE)
				return WAR_SWING_EQUIPPING_NOWAIT;

			iParryReduction = (int)(Args.m_iN1);
			iDmgType = (DAMAGE_TYPE)(Args.m_iN2);
			iDmg = (int)Args.m_VarsLocal.GetKeyNum("Damage");
			iParryChance = (int)Args.m_VarsLocal.GetKeyNum("ParryChance");
			ParrySkill = (SKILL_TYPE)Args.m_VarsLocal.GetKeyNum("ParrySkillID");
		}

		if (iParryChance > 0 && pCharTarg->Skill_UseQuick(ParrySkill, iParryChance, true, false))
		{
			if ( IsPriv(PRIV_DETAIL) )
				SysMessageDefault(DEFMSG_COMBAT_PARRY);

			//If we are using the Samurai Empire Formula and we are not wearing a shield also raise Bushido.
			if (g_Cfg.m_iFeatureSE & FEATURE_SE_NINJASAM && g_Cfg.m_iCombatParryingEra & PARRYERA_SEFORMULA && !pCharTarg->IsStatFlag(STATF_HASSHIELD))
				pCharTarg->Skill_Experience(SKILL_BUSHIDO, iParryChance);

			int iParryDamageChance = (int)(Args.m_VarsLocal.GetKeyNum("ItemParryDamageChance"));
			if ( pItemHit && (iParryDamageChance > g_Rand.GetVal(100)) )
				pItemHit->OnTakeDamage(1, this, iDmgType);

			//Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16);		// moved to scripts (@UseQuick on Parrying skill)
			if (iParryReduction >= 100)
				return WAR_SWING_EQUIPPING_NOWAIT;

			// Apply parrying reduction (if there's any)
			if (iParryReduction > 0)
				iDmg -= IMulDiv(iDmg, iParryReduction, 100);
		}
	}

	

	CScriptTriggerArgs Args(iDmg, iDmgType, pWeapon);
	Args.m_VarsLocal.SetNum("ItemDamageChance", 25);
    Args.m_VarsLocal.SetNum("ItemPoisonReductionChance", 100);
    Args.m_VarsLocal.SetNum("ItemPoisonReductionAmount", 1);
    int32 iPoison = 0;
    if (pWeapon)
    {
        iPoison = g_Rand.GetVal(pWeapon->m_itWeapon.m_poison_skill);
        Args.m_VarsLocal.SetNum("ItemPoisonReductionAmount", iPoison / 2);
    }

	if ( pAmmo && pAmmo->GetUID().IsValidUID() )
		Args.m_VarsLocal.SetNum("Arrow",(dword)pAmmo->GetUID());

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

        if (pWeapon)
        {
            Args.m_pO1 = this;
            if (pWeapon->OnTrigger(ITRIG_Hit, pCharTarg, &Args) == TRIGRET_RET_TRUE)
                return WAR_SWING_EQUIPPING;

            if (Args.m_VarsLocal.GetKeyNum("ArrowHandled") != 0)		// if arrow is handled by script, do nothing with it further
                pAmmo = nullptr;

            iDmg = (int)(Args.m_iN1);
            iDmgType = (DAMAGE_TYPE)(Args.m_iN2);
        }
	}

	// BAD BAD Healing fix.. Cant think of something else -- Radiant
	if ( pCharTarg->m_Act_SkillCurrent == SKILL_HEALING )
	{
		pCharTarg->SysMessageDefault(DEFMSG_HEALING_INTERRUPT);
		pCharTarg->Skill_Cleanup();
	}

	if ( pAmmo )
	{
		if ( pCharTarg->m_pNPC && (40 >= g_Rand.Get16ValFast(100)) )
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
            (pWeapon->m_itWeapon.m_poison_skill > g_Rand.GetVal(100) || pWeapon->m_itWeapon.m_poison_skill < 10))
		{
			byte iPoisonDeliver = (byte)(iPoison);
			pCharTarg->SetPoison(10 * iPoisonDeliver, iPoisonDeliver / 5, this);

            if (Args.m_VarsLocal.GetKeyNum("ItemPoisonReductionChance") > g_Rand.GetVal(100))
            {
                pWeapon->m_itWeapon.m_poison_skill -= (byte)(Args.m_VarsLocal.GetKeyNum("ItemPoisonReductionAmount"));	// reduce weapon poison charges
                pWeapon->UpdatePropertyFlag();
            }
		}

		// Check if the weapon will be damaged
		int iDamageChance = (int)(Args.m_VarsLocal.GetKeyNum("ItemDamageChance"));
		if ( iDamageChance > g_Rand.GetVal(100) )
			pWeapon->OnTakeDamage(iDmg, pCharTarg);
	}
	else if ( m_pNPC )
	{
		// Base poisoning for NPCs
		if ( !IsSetCombatFlags(COMBAT_NOPOISONHIT) && 50 >= g_Rand.GetVal(100) )
		{
			int iPoisoningSkill = Skill_GetBase(SKILL_POISONING);
			if ( iPoisoningSkill )
				pCharTarg->SetPoison(g_Rand.GetVal(iPoisoningSkill), g_Rand.GetVal(iPoisoningSkill / 50), this);
		}
	}

	// Took my swing. Do Damage !
	const CCPropsChar* pCCPChar = GetComponentProps<CCPropsChar>();
	const CCPropsChar* pBaseCCPChar = Base_GetDef()->GetComponentProps<CCPropsChar>();
	pCharTarg->OnTakeDamage(
        iDmg,
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

		bool fMakeLeechSound = false;
		if ( uiHitLifeLeech )
		{
			uiHitLifeLeech = (ushort)(g_Rand.GetVal2(0, (iDmg * uiHitLifeLeech * 30) / 10000));	// leech 0% ~ 30% of damage value
			UpdateStatVal(STAT_STR, uiHitLifeLeech);
            fMakeLeechSound = true;
		}

		ushort uiHitManaLeech = (ushort)GetPropNum(pCCPChar, PROPCH_HITLEECHMANA, pBaseCCPChar);
		if ( uiHitManaLeech )
		{
			uiHitManaLeech = (ushort)(g_Rand.GetVal2(0, (iDmg * uiHitManaLeech * 40) / 10000));	// leech 0% ~ 40% of damage value
			UpdateStatVal(STAT_INT, uiHitManaLeech);
            fMakeLeechSound = true;
		}

		if ( GetPropNum(pCCPChar, PROPCH_HITLEECHSTAM, pBaseCCPChar) > g_Rand.GetLLVal(100) )
		{
			UpdateStatVal(STAT_DEX, (ushort)iDmg);	// leech 100% of damage value
            fMakeLeechSound = true;
		}

		ushort uiManaDrain = 0;
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
		{
			CItem *pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if ( pPoly && pPoly->m_itSpell.m_spell == SPELL_Wraith_Form )
				uiManaDrain += 5 + (15 * Skill_GetBase(SKILL_SPIRITSPEAK) / 1000);
		}
		if ( GetPropNum(pCCPChar, PROPCH_HITMANADRAIN, pBaseCCPChar) > g_Rand.GetVal(100) )
			uiManaDrain += (ushort)IMulDivLL(iDmg, 20, 100);		// leech 20% of damage value

		ushort uiTargMana = pCharTarg->Stat_GetVal(STAT_INT);
		if ( uiManaDrain > uiTargMana )
			uiManaDrain = uiTargMana;

		if ( uiManaDrain > 0 )
		{
			pCharTarg->UpdateStatVal(STAT_INT, uiTargMana - uiManaDrain);
			UpdateStatVal(STAT_INT, uiManaDrain);
            fMakeLeechSound = true;
		}

		if ( fMakeLeechSound )
			Sound(0x44d);

        if (pWeapon)
        {
	     
            if (GetPropNum(pCCPChar, PROPCH_HITAREAPHYSICAL, pBaseCCPChar) > g_Rand.GetVal(100))
                pCharTarg->OnTakeDamageInflictArea(iDmg / 2, this, DAMAGE_HIT_BLUNT, 100, 0, 0, 0, 0, static_cast<HUE_TYPE>(0x32), static_cast<SOUND_TYPE>(0x10E));

            bool fElemental = IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE);
            if (fElemental)
	        {
				
		        if (GetPropNum(pCCPChar, PROPCH_HITAREAFIRE, pBaseCCPChar) > g_Rand.GetVal(100))
			        pCharTarg->OnTakeDamageInflictArea(iDmg / 2, this, DAMAGE_FIRE, 0, 100, 0, 0, 0, static_cast<HUE_TYPE>(0x488), static_cast<SOUND_TYPE>(0x11D));

		        if (GetPropNum(pCCPChar, PROPCH_HITAREACOLD, pBaseCCPChar) > g_Rand.GetVal(100))
			        pCharTarg->OnTakeDamageInflictArea(iDmg / 2, this, DAMAGE_COLD, 0, 0, 100, 0, 0, static_cast<HUE_TYPE>(0x834), static_cast<SOUND_TYPE>(0xFC));

		        if (GetPropNum(pCCPChar, PROPCH_HITAREAPOISON, pBaseCCPChar) > g_Rand.GetVal(100))
			        pCharTarg->OnTakeDamageInflictArea(iDmg / 2, this, DAMAGE_POISON, 0, 0, 0, 100, 0, static_cast<HUE_TYPE>(0x48E), static_cast<SOUND_TYPE>(0x205));

		        if (GetPropNum(pCCPChar, PROPCH_HITAREAENERGY, pBaseCCPChar) > g_Rand.GetVal(100))
			        pCharTarg->OnTakeDamageInflictArea(iDmg / 2, this, DAMAGE_ENERGY, 0, 0, 0, 0, 100, static_cast<HUE_TYPE>(0x78), static_cast<SOUND_TYPE>(0x1F1));
			
	        }

	        if (GetPropNum(pCCPChar, PROPCH_HITDISPEL, pBaseCCPChar) > g_Rand.GetVal(100))
		        pCharTarg->OnSpellEffect(SPELL_Dispel, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);
						
	        if (GetPropNum(pCCPChar, PROPCH_HITFIREBALL, pBaseCCPChar) > g_Rand.GetVal(100))
		        pCharTarg->OnSpellEffect(SPELL_Fireball, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);
			
	        if (GetPropNum(pCCPChar, PROPCH_HITHARM, pBaseCCPChar) > g_Rand.GetVal(100))
		        pCharTarg->OnSpellEffect(SPELL_Harm, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);
			
	        if (GetPropNum(pCCPChar, PROPCH_HITLIGHTNING, pBaseCCPChar) > g_Rand.GetVal(100))
		        pCharTarg->OnSpellEffect(SPELL_Lightning, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);
			
	        if (GetPropNum(pCCPChar, PROPCH_HITMAGICARROW, pBaseCCPChar) > g_Rand.GetVal(100))
		        pCharTarg->OnSpellEffect(SPELL_Magic_Arrow, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);
        }

		// Make blood effects
		if ( pCharTarg->_wBloodHue != (HUE_TYPE)(-1) )
		{
			static constexpr ITEMID_TYPE sm_Blood[] = { ITEMID_BLOOD1, ITEMID_BLOOD2, ITEMID_BLOOD3, ITEMID_BLOOD4, ITEMID_BLOOD5, ITEMID_BLOOD6, ITEMID_BLOOD_SPLAT };
			const int iBloodQty = (g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE) ? g_Rand.Get16Val2Fast(4, 5) : g_Rand.Get16Val2Fast(1, 2);

			for ( int i = 0; i < iBloodQty; ++i )
			{
                const ITEMID_TYPE iBloodID = sm_Blood[(size_t)g_Rand.Get16ValFast(ARRAY_COUNT(sm_Blood))];

                CItem *pBlood = CItem::CreateBase(iBloodID);
                ASSERT(pBlood);
                pBlood->SetHue(pCharTarg->_wBloodHue);
                pBlood->MoveNear(pCharTarg->GetTopPoint(), 1);
                pBlood->Update();
                pBlood->SetDecayTimeS(5);

                // Looks like the hues with index >= 1000 cause the blood to be black, instead of the right color
                /*
                CPointMap pt = pCharTarg->GetTopPoint();
                pt.m_x += (short)g_Rand.GetVal2(-1, 1);
                pt.m_y += (short)g_Rand.GetVal2(-1, 1);
                EffectLocation(EFFECT_XYZ, iBloodID, nullptr, &pt, 50, 0, false, pCharTarg->_wBloodHue);
                */
			}
		}

		// Check for passive skill gain
		if ( m_pPlayer &&  (!pCharTarg->m_pArea->IsFlag(REGION_FLAG_NO_PVP) || !pCharTarg->IsPlayer()) )
		{
			Skill_Experience(skill, m_Act_Difficulty);
			Skill_Experience(SKILL_TACTICS, m_Act_Difficulty);
		}
	}

	return WAR_SWING_EQUIPPING_NOWAIT;
}

