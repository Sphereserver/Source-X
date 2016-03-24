// Fight/Criminal actions/Noto.
#include "../clients/CClient.h"
#include "../common/CGrayUIDextra.h"
#include "../network/send.h"
#include "../CServTime.h"
#include "../Triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

// I noticed a crime.
void CChar::OnNoticeCrime( CChar * pCriminal, const CChar * pCharMark )
{
	ADDTOCALLSTACK("CChar::OnNoticeCrime");
	if ( !pCriminal || pCriminal == this || pCriminal == pCharMark || pCriminal->IsPriv(PRIV_GM) || pCriminal->GetNPCBrain() == NPCBRAIN_GUARD )
		return;
	if ( pCriminal->Noto_Criminal(this) )
		return;

	// Make my owner criminal too (if I have one)
	CChar * pOwner = pCriminal->NPC_PetGetOwner();
	if ( pOwner != NULL && pOwner != this )
		OnNoticeCrime( pOwner, pCharMark );

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

	if ( ! NPC_CanSpeak())
		return;	// I can't talk anyhow.

	if (GetNPCBrain() != NPCBRAIN_HUMAN)
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
bool CChar::CheckCrimeSeen( SKILL_TYPE SkillToSee, CChar * pCharMark, const CObjBase * pItem, LPCTSTR pAction )
{
	ADDTOCALLSTACK("CChar::CheckCrimeSeen");

	bool fSeen = false;

	// Who notices ?

	if (m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD) // guards only fight for justice, they can't commit a crime!!?
		return false;

	CWorldSearch AreaChars( GetTopPoint(), UO_MAP_VIEW_SIZE );
	for (;;)
	{
		CChar * pChar = AreaChars.GetChar();
		if ( pChar == NULL )
			break;
		if ( this == pChar )
			continue;	// I saw myself before.
		if ( ! pChar->CanSeeLOS( this, LOS_NB_WINDOWS )) //what if I was standing behind a window when I saw a crime? :)
			continue;

		bool fYour = ( pCharMark == pChar );


		char *z = Str_GetTemp();
		if ( pItem && pAction )
		{
			if ( pCharMark )
				sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_2), GetName(), pAction, fYour ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_YOUR) : pCharMark->GetName(), fYour ? "" : g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_S), pItem->GetName());
			else
				sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_1), GetName(), pAction, pItem->GetName());
			pChar->ObjMessage(z, this);
		}

		// If a GM sees you it it not a crime.
		if ( pChar->GetPrivLevel() > GetPrivLevel())
			continue;
		fSeen = true;

		// They are not a criminal til someone calls the guards !!!
		if ( SkillToSee == SKILL_SNOOPING )
		{
			if (IsTrigUsed(TRIGGER_SEESNOOP))
			{
				CScriptTriggerArgs Args(pAction);
				Args.m_iN1 = SkillToSee ? SkillToSee : pCharMark->Skill_GetActive();;
				Args.m_iN2 = pItem ? (dword)pItem->GetUID() : 0;
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
	return( fSeen );
}

// Assume the container is not locked.
// return: true = snoop or can't open at all.
//  false = instant open.
bool CChar::Skill_Snoop_Check( const CItemContainer * pItem )
{
	ADDTOCALLSTACK("CChar::Skill_Snoop_Check");

	if ( pItem == NULL )
		return( true );

	ASSERT( pItem->IsItem());
	if ( pItem->IsContainer() )
	{
		CItemContainer * pItemCont = dynamic_cast <CItemContainer *> (pItem->GetContainer());
			if  ( ( pItemCont->IsItemInTrade() == true )  && ( g_Cfg.m_iTradeWindowSnooping == false ) )
				return( false );
	}

	if ( ! IsPriv(PRIV_GM))
	switch ( pItem->GetType())
	{
		case IT_SHIP_HOLD_LOCK:
		case IT_SHIP_HOLD:
			// Must be on board a ship to open the hatch.
			ASSERT(m_pArea);
			if ( m_pArea->GetResourceID() != pItem->m_uidLink )
			{
				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_HATCH_FAIL));
				return( true );
			}
			break;
		case IT_EQ_BANK_BOX:
			// Some sort of cheater.
			return( false );
		default:
			break;
	}

	CChar * pCharMark;
	if ( ! IsTakeCrime( pItem, &pCharMark ) || pCharMark == NULL )
		return( false );

	if ( Skill_Wait(SKILL_SNOOPING))
		return( true );

	m_Act_Targ = pItem->GetUID();
	Skill_Start( SKILL_SNOOPING );
	return( true );
}

// SKILL_SNOOPING
// m_Act_Targ = object to snoop into.
// RETURN:
// -SKTRIG_QTY = no chance. and not a crime
// -SKTRIG_FAIL = no chance and caught.
// 0-100 = difficulty = percent chance of failure.
int CChar::Skill_Snooping( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Snooping");

	if ( stage == SKTRIG_STROKE )
		return( 0 );

	// Assume the container is not locked.
	CItemContainer * pCont = dynamic_cast <CItemContainer *>(m_Act_Targ.ItemFind());
	if ( pCont == NULL )
		return( -SKTRIG_QTY );

	CChar * pCharMark;
	if ( ! IsTakeCrime( pCont, &pCharMark ) || pCharMark == NULL )
		return( 0 );	// Not a crime really.

	if ( GetTopDist3D( pCharMark ) > 1 )
	{
		SysMessageDefault( DEFMSG_SNOOPING_REACH );
		return( -SKTRIG_QTY );
	}

	if ( !CanTouch( pCont ))
	{
		SysMessageDefault( DEFMSG_SNOOPING_CANT );
		return( -SKTRIG_QTY );
	}

	if ( stage == SKTRIG_START )
	{
		PLEVEL_TYPE plevel = GetPrivLevel();
		if ( plevel < pCharMark->GetPrivLevel())
		{
			SysMessageDefault( DEFMSG_SNOOPING_CANT );
			return( -SKTRIG_QTY );
		}

		// return the difficulty.
		return( (Skill_GetAdjusted(SKILL_SNOOPING) < Calc_GetRandVal(1000))? 100 : 0 );
	}

	// did anyone see this ?
	CheckCrimeSeen( SKILL_SNOOPING, pCharMark, pCont, g_Cfg.GetDefaultMsg( DEFMSG_SNOOPING_ATTEMPTING ) );
	Noto_Karma( -4, INT32_MIN, true );

	if ( stage == SKTRIG_FAIL )
	{
		SysMessageDefault( DEFMSG_SNOOPING_FAILED );
		if ( Skill_GetAdjusted(SKILL_HIDING) / 2 < Calc_GetRandVal(1000) )
			Reveal();
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( IsClient())
			m_pClient->addContainerSetup( pCont );	// open the container
	}
	return( 0 );
}

// m_Act_Targ = object to steal.
// RETURN:
// -SKTRIG_QTY = no chance. and not a crime
// -SKTRIG_FAIL = no chance and caught.
// 0-100 = difficulty = percent chance of failure.
int CChar::Skill_Stealing( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Stealing");
	if ( stage == SKTRIG_STROKE )
		return( 0 );

	CItem * pItem = m_Act_Targ.ItemFind();
	CChar * pCharMark = NULL;
	if ( pItem == NULL )	// on a chars head ? = random steal.
	{
		pCharMark = m_Act_Targ.CharFind();
		if ( pCharMark == NULL )
		{
			SysMessageDefault( DEFMSG_STEALING_NOTHING );
			return( -SKTRIG_QTY );
		}
		CItemContainer * pPack = pCharMark->GetPack();
		if ( pPack == NULL )
		{
cantsteal:
			SysMessageDefault( DEFMSG_STEALING_EMPTY );
			return( -SKTRIG_QTY );
		}
		pItem = pPack->ContentFindRandom();
		if ( pItem == NULL )
		{
			goto cantsteal;
		}
		m_Act_Targ = pItem->GetUID();
	}

	// Special cases.
	CContainer * pContainer = dynamic_cast <CContainer *> (pItem->GetContainer());
	if ( pContainer )
	{
		CItemCorpse * pCorpse = dynamic_cast <CItemCorpse *> (pContainer);
		if ( pCorpse )
		{
			SysMessageDefault( DEFMSG_STEALING_CORPSE );
			return( -SKTRIG_ABORT );
		}
	}
   CItem * pCItem = dynamic_cast <CItem *> (pItem->GetContainer());
   if ( pCItem )
   {
	   if ( pCItem->GetType() == IT_GAME_BOARD )
	   {
		   SysMessageDefault( DEFMSG_STEALING_GAMEBOARD );
		   return( -SKTRIG_ABORT );
	   }
	   if ( pCItem->GetType() == IT_EQ_TRADE_WINDOW )
	   {
		   SysMessageDefault( DEFMSG_STEALING_TRADE );
		   return( -SKTRIG_ABORT );
	   }
   }
	CItemCorpse * pCorpse = dynamic_cast <CItemCorpse *> (pItem);
	if ( pCorpse )
	{
		SysMessageDefault( DEFMSG_STEALING_CORPSE );
		return( -SKTRIG_ABORT );
	}
	if ( pItem->IsType(IT_TRAIN_PICKPOCKET))
	{
		SysMessageDefault( DEFMSG_STEALING_PICKPOCKET );
		return -SKTRIG_QTY;
	}
	if ( pItem->IsType( IT_GAME_PIECE ))
	{
		return -SKTRIG_QTY;
	}
	if ( ! CanTouch( pItem ))
	{
		SysMessageDefault( DEFMSG_STEALING_REACH );
		return( -SKTRIG_ABORT );
	}
	if ( ! CanMove( pItem ) || ! CanCarry( pItem ))
	{
		SysMessageDefault( DEFMSG_STEALING_HEAVY );
		return( -SKTRIG_ABORT );
	}
	if ( ! IsTakeCrime( pItem, & pCharMark ))
	{
		SysMessageDefault( DEFMSG_STEALING_NONEED );

		// Just pick it up ?
		return( -SKTRIG_QTY );
	}
	if ( m_pArea->IsFlag(REGION_FLAG_SAFE))
	{
		SysMessageDefault( DEFMSG_STEALING_STOP );
		return( -SKTRIG_QTY );
	}

	Reveal();	// If we take an item off the ground we are revealed.

	bool fGround = false;
	if ( pCharMark != NULL )
	{
		if ( GetTopDist3D( pCharMark ) > 2 )
		{
			SysMessageDefault( DEFMSG_STEALING_MARK );
			return -SKTRIG_QTY;
		}
		if ( m_pArea->IsFlag(REGION_FLAG_NO_PVP) && pCharMark->m_pPlayer && ! IsPriv(PRIV_GM))
		{
			SysMessageDefault( DEFMSG_STEALING_SAFE );
			return( -1 );
		}
		if ( GetPrivLevel() < pCharMark->GetPrivLevel())
		{
			return -SKTRIG_FAIL;
		}
		if ( stage == SKTRIG_START )
		{
			return g_Cfg.Calc_StealingItem( this, pItem, pCharMark );
		}
	}
	else
	{
		// stealing off the ground should always succeed.
		// it's just a matter of getting caught.
		if ( stage == SKTRIG_START )
			return( 1 );	// town stuff on the ground is too easy.

		fGround = true;
	}

	// Deliver the goods.

	if ( stage == SKTRIG_SUCCESS || fGround )
	{
		pItem->ClrAttr(ATTR_OWNED);	// Now it's mine
		CItemContainer * pPack = GetPack();
		if ( pItem->GetParent() != pPack && pPack )
		{
			pItem->RemoveFromView();
			// Put in my invent.
			pPack->ContentAdd( pItem );
		}
	}

	if ( m_Act_Difficulty == 0 )
		return( 0 );	// Too easy to be bad. hehe

	// You should only be able to go down to -1000 karma by stealing.
	if ( CheckCrimeSeen( SKILL_STEALING, pCharMark, pItem, (stage == SKTRIG_FAIL)? g_Cfg.GetDefaultMsg( DEFMSG_STEALING_YOUR ) : g_Cfg.GetDefaultMsg( DEFMSG_STEALING_SOMEONE ) ))
		Noto_Karma( -100, -1000, true );

	return( 0 );
}

// I just yelled for guards.
void CChar::CallGuards( CChar * pCriminal )
{
	ADDTOCALLSTACK("CChar::CallGuards");
	if ( !m_pArea || (pCriminal == this) )
		return;
	if ( IsStatFlag(STATF_DEAD) || (pCriminal && (pCriminal->IsStatFlag(STATF_DEAD) || pCriminal->IsPriv(PRIV_GM))) )
		return;

	CChar *pGuard = NULL;
	if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD )
	{
		// I'm a guard, why summon someone else to do my work? :)
		pGuard = this;
	}
	else
	{
		// Search for a free guards nearby
		CWorldSearch AreaGuard(GetTopPoint(), UO_MAP_VIEW_SIGHT);
		CChar *pGuardFound = NULL;
		while ( (pGuardFound = AreaGuard.GetChar()) != NULL )
		{
			if ( pGuardFound->m_pNPC && (pGuardFound->m_pNPC->m_Brain == NPCBRAIN_GUARD) && !pGuardFound->IsStatFlag(STATF_War) )
			{
				pGuard = pGuardFound;
				break;
			}
		}
	}
	
	if ( pCriminal )
	{
		if ( !pCriminal->m_pArea->IsGuarded() )
			return;

		CVarDefCont *pVarDef = pCriminal->m_pArea->m_TagDefs.GetKey("OVERRIDE.GUARDS");
		RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_CHARDEF, (pVarDef ? pVarDef->GetValStr() : "GUARDS"));
		if ( IsTrigUsed(TRIGGER_CALLGUARDS) )
		{
			CScriptTriggerArgs Args(pGuard);
			Args.m_iN1 = rid.GetResIndex();
			Args.m_iN2 = 0;
			Args.m_VarObjs.Insert(1, pCriminal, true);

			if ( OnTrigger(CTRIG_CallGuards, pCriminal, &Args) == TRIGRET_RET_TRUE )
				return;

			if ( static_cast<int>(Args.m_iN1) != rid.GetResIndex() )
				rid = RESOURCE_ID(RES_CHARDEF, static_cast<int>(Args.m_iN1));
			if ( Args.m_iN2 > 0 )
				pGuard = NULL;
		}
		if ( !pGuard )		//	spawn a new guard
		{
			if ( !rid.IsValidUID() )
				return;
			pGuard = CChar::CreateNPC(static_cast<CREID_TYPE>(rid.GetResIndex()));
			if ( !pGuard )
				return;

			if ( pCriminal->m_pArea->m_TagDefs.GetKeyNum("RED", true) )
				pGuard->m_TagDefs.SetNum("NAME.HUE", g_Cfg.m_iColorNotoEvil, true);
			pGuard->Spell_Effect_Create(SPELL_Summon, LAYER_SPELL_Summon, 1000, g_Cfg.m_iGuardLingerTime);
			pGuard->Spell_Teleport(pCriminal->GetTopPoint(), false, false);
		}
		pGuard->NPC_LookAtCharGuard(pCriminal, true);
	}
	else
	{
		// We don't have any target yet, let's check everyone nearby
		CWorldSearch AreaCrime(GetTopPoint(), UO_MAP_VIEW_SIZE);
		while ( (pCriminal = AreaCrime.GetChar()) != NULL )
		{
			if ( pCriminal == this )
				continue;
			if ( !pCriminal->m_pArea->IsGuarded() )
				continue;
			if ( !CanDisturb(pCriminal) )	// don't allow guards to be called on someone we can't disturb
				continue;

			// Mark person as criminal if I saw him criming
			// Only players call guards this way. NPC's flag criminal instantly
			if ( m_pPlayer && Memory_FindObjTypes(pCriminal, MEMORY_SAWCRIME) )
				pCriminal->Noto_Criminal();

			bool bCriminal = (pCriminal->IsStatFlag(STATF_Criminal) || (pCriminal->Noto_IsEvil() && g_Cfg.m_fGuardsOnMurderers));
			if ( !bCriminal )
				continue;

			CVarDefCont *pVarDef = pCriminal->m_pArea->m_TagDefs.GetKey("OVERRIDE.GUARDS");
			RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_CHARDEF, (pVarDef ? pVarDef->GetValStr() : "GUARDS"));
			if ( IsTrigUsed(TRIGGER_CALLGUARDS) )
			{
				CScriptTriggerArgs Args(pGuard);
				Args.m_iN1 = rid.GetResIndex();
				Args.m_iN2 = 0;
				Args.m_VarObjs.Insert(1, pCriminal, true);

				if ( OnTrigger(CTRIG_CallGuards, pCriminal, &Args) == TRIGRET_RET_TRUE )
					return;

				if ( static_cast<int>(Args.m_iN1) != rid.GetResIndex() )
					rid = RESOURCE_ID(RES_CHARDEF, static_cast<int>(Args.m_iN1));
				if ( Args.m_iN2 > 0 )
					pGuard = NULL;
			}
			if ( !pGuard )		//	spawn a new guard
			{
				if ( !rid.IsValidUID() )
					return;
				pGuard = CChar::CreateNPC(static_cast<CREID_TYPE>(rid.GetResIndex()));
				if ( !pGuard )
					return;

				if ( pCriminal->m_pArea->m_TagDefs.GetKeyNum("RED", true) )
					pGuard->m_TagDefs.SetNum("NAME.HUE", g_Cfg.m_iColorNotoEvil, true);
				pGuard->Spell_Effect_Create(SPELL_Summon, LAYER_SPELL_Summon, 1000, g_Cfg.m_iGuardLingerTime);
				pGuard->Spell_Teleport(pCriminal->GetTopPoint(), false, false);
			}
			pGuard->NPC_LookAtCharGuard(pCriminal, true);
			pGuard = NULL;	// we can't still using the same guard when calling guards on many targets at once
		}
	}
}

// i notice a Crime or attack against me ..
// Actual harm has taken place.
// Attack back.
void CChar::OnHarmedBy( CChar * pCharSrc )
{
	ADDTOCALLSTACK("CChar::OnHarmedBy");

	bool fFightActive = Fight_IsActive();
	Memory_AddObjTypes(pCharSrc, MEMORY_HARMEDBY);

	if (fFightActive && m_Fight_Targ.CharFind())
	{
		// In war mode already
		if ( m_pPlayer )
			return;
		if ( Calc_GetRandVal( 10 ))
			return;
		// NPC will Change targets.
	}

	if ( NPC_IsOwnedBy(pCharSrc, false) )
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
bool CChar::OnAttackedBy( CChar * pCharSrc, int iHarmQty, bool fCommandPet, bool fShouldReveal)
{
	ADDTOCALLSTACK("CChar::OnAttackedBy");
	UNREFERENCED_PARAMETER(iHarmQty);

	if ( pCharSrc == NULL )
		return true;	// field spell ?
	if ( pCharSrc == this )
		return true;	// self induced
	if ( IsStatFlag( STATF_DEAD ) )
		return false;

	if (fShouldReveal)
		pCharSrc->Reveal();	// fix invis exploit

	// Am i already attacking the source anyhow
	if (Fight_IsActive() && m_Fight_Targ == pCharSrc->GetUID())
		return true;

	Memory_AddObjTypes( pCharSrc, MEMORY_HARMEDBY|MEMORY_IRRITATEDBY );
	Attacker_Add(pCharSrc);

	// Are they a criminal for it ? Is attacking me a crime ?
	if ( Noto_GetFlag(pCharSrc) == NOTO_GOOD )
	{
		/*if ( IsClient())
		{
			// I decide if this is a crime.
			OnNoticeCrime( pCharSrc, this );
		}
		else
		{*/
			// If it is a pet then this a crime others can report.
			CChar * pCharMark = IsStatFlag(STATF_Pet) ? NPC_PetGetOwner() : this;
			pCharSrc->CheckCrimeSeen(Skill_GetActive(), pCharMark, NULL, NULL);
		//}
	}

	if ( ! fCommandPet )
	{
		// possibly retaliate. (auto defend)
		OnHarmedBy( pCharSrc );
	}

	return( true );
}

static const LAYER_TYPE sm_ArmorLayerHead[] = { LAYER_HELM };															// ARMOR_HEAD
static const LAYER_TYPE sm_ArmorLayerNeck[] = { LAYER_COLLAR };															// ARMOR_NECK
static const LAYER_TYPE sm_ArmorLayerBack[] = { LAYER_SHIRT, LAYER_CHEST, LAYER_TUNIC, LAYER_CAPE, LAYER_ROBE };		// ARMOR_BACK
static const LAYER_TYPE sm_ArmorLayerChest[] = { LAYER_SHIRT, LAYER_CHEST, LAYER_TUNIC, LAYER_ROBE };					// ARMOR_CHEST
static const LAYER_TYPE sm_ArmorLayerArms[] = { LAYER_ARMS, LAYER_CAPE, LAYER_ROBE };									// ARMOR_ARMS
static const LAYER_TYPE sm_ArmorLayerHands[] = { LAYER_GLOVES };														// ARMOR_HANDS
static const LAYER_TYPE sm_ArmorLayerLegs[] = { LAYER_PANTS, LAYER_SKIRT, LAYER_HALF_APRON, LAYER_ROBE, LAYER_LEGS };	// ARMOR_LEGS
static const LAYER_TYPE sm_ArmorLayerFeet[] = { LAYER_SHOES, LAYER_LEGS };												// ARMOR_FEET

struct CArmorLayerType
{
	word m_wCoverage;	// Percentage of humanoid body area
	const LAYER_TYPE * m_pLayers;
};

// OSI doesn't damage ARMOR_BACK and ARMOR_FEET at all.
// But for backward compatibility, I decreased ARMOR_CHEST (-10%) and increased
// ARMOR_BACK (+5%) and ARMOR_FEET (+5%) just to keep them getting some damage
static const CArmorLayerType sm_ArmorLayers[ARMOR_QTY] =	// layers covering the armor zone.
{
	{ 15,	sm_ArmorLayerHead },	// ARMOR_HEAD
	{ 7,	sm_ArmorLayerNeck },	// ARMOR_NECK
	{ 5,	sm_ArmorLayerBack },	// ARMOR_BACK
	{ 25,	sm_ArmorLayerChest },	// ARMOR_CHEST
	{ 14,	sm_ArmorLayerArms },	// ARMOR_ARMS
	{ 7,	sm_ArmorLayerHands },	// ARMOR_HANDS
	{ 22,	sm_ArmorLayerLegs },	// ARMOR_LEGS
	{ 5,	sm_ArmorLayerFeet }		// ARMOR_FEET
};

// When armor is added or subtracted check this.
// This is the general AC number printed.
// Tho not really used to compute damage.
int CChar::CalcArmorDefense() const
{
	ADDTOCALLSTACK("CChar::CalcArmorDefense");

	int iDefenseTotal = 0;
	int iArmorCount = 0;
	word ArmorRegionMax[ARMOR_QTY];
#ifndef _WIN32
	for (int i_tmpN=0; i_tmpN < ARMOR_QTY; i_tmpN++)
	{
		ArmorRegionMax[i_tmpN]=0;
	}
#else
	memset( ArmorRegionMax, 0, sizeof(ArmorRegionMax));
#endif
	for ( CItem* pItem=GetContentHead(); pItem!=NULL; pItem=pItem->GetNext())
	{
		word iDefense = static_cast<word>(pItem->Armor_GetDefense());

		// reverse of sm_ArmorLayers
		switch ( pItem->GetEquipLayer())
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
				// Shield effect.
				if ( pItem->IsType( IT_SHIELD ))
				{
					iDefenseTotal += (iDefense) * ( Skill_GetAdjusted(SKILL_PARRYING) / 10 );
				}
				continue;
			default:
				continue;
		}

		iArmorCount ++;
	}
	
	if ( iArmorCount )
	{
		for ( int i=0; i<ARMOR_QTY; i++ )
		{
			iDefenseTotal += sm_ArmorLayers[i].m_wCoverage * ArmorRegionMax[i];
		}
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

	if ( pSrc == NULL )
		pSrc = this;

	if ( IsStatFlag(STATF_DEAD) )	// already dead
		return( -1 );
	if ( !(uType & DAMAGE_GOD) )
	{
		if ( IsStatFlag(STATF_INVUL|STATF_Stone) )
		{
effect_bounce:
			Effect( EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16 );
			return( 0 );
		}
		if ( (uType & DAMAGE_FIRE) && Can(CAN_C_FIRE_IMMUNE) )
			goto effect_bounce;
		if ( m_pArea )
		{
			if ( m_pArea->IsFlag(REGION_FLAG_SAFE) )
				goto effect_bounce;
			if ( m_pArea->IsFlag(REGION_FLAG_NO_PVP) && pSrc && ((IsStatFlag(STATF_Pet) && NPC_PetGetOwner() == pSrc) || (m_pPlayer && (pSrc->m_pPlayer || pSrc->IsStatFlag(STATF_Pet)))) )
				goto effect_bounce;
		}
	}

	// Make some notoriety checks
	// Don't reveal attacker if the damage has DAMAGE_NOREVEAL flag set (this is set by default for poison and spell damage)
	if ( !OnAttackedBy(pSrc, iDmg, false, !(uType & DAMAGE_NOREVEAL)) )
		return( 0 );

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

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);

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

			int iPhysicalDamage = iDmg * iDmgPhysical * (100 - static_cast<int>(GetDefNum("RESPHYSICAL", true)));
			int iFireDamage = iDmg * iDmgFire * (100 - static_cast<int>(GetDefNum("RESFIRE", true)));
			int iColdDamage = iDmg * iDmgCold * (100 - static_cast<int>(GetDefNum("RESCOLD", true)));
			int iPoisonDamage = iDmg * iDmgPoison * (100 - static_cast<int>(GetDefNum("RESPOISON", true)));
			int iEnergyDamage = iDmg * iDmgEnergy * (100 - static_cast<int>(GetDefNum("RESENERGY", true)));

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

	CScriptTriggerArgs Args( iDmg, uType, static_cast<INT64>(0) );
	Args.m_VarsLocal.SetNum("ItemDamageChance", 40);

	if ( IsTrigUsed(TRIGGER_GETHIT) )
	{
		if ( OnTrigger( CTRIG_GetHit, pSrc, &Args ) == TRIGRET_RET_TRUE )
			return( 0 );
		iDmg = static_cast<int>(Args.m_iN1);
		uType = static_cast<DAMAGE_TYPE>(Args.m_iN2);
	}

	int iItemDamageChance = static_cast<int>(Args.m_VarsLocal.GetKeyNum("ItemDamageChance", true));
	if ( (iItemDamageChance > Calc_GetRandVal(100)) && !pCharDef->Can(CAN_C_NONHUMANOID) )
	{
		int iHitRoll = Calc_GetRandVal(100);
		BODYPART_TYPE iHitArea = ARMOR_HEAD;
		while ( iHitArea < (ARMOR_QTY - 1) )
		{
			iHitRoll -= sm_ArmorLayers[iHitArea].m_wCoverage;
			if ( iHitRoll < 0 )
				break;
			iHitArea = static_cast<BODYPART_TYPE>( iHitArea + 1 );
		}

		LAYER_TYPE iHitLayer = LAYER_NONE;
		switch ( iHitArea )
		{
			case ARMOR_HEAD:	iHitLayer = sm_ArmorLayerHead[Calc_GetRandVal(COUNTOF(sm_ArmorLayerHead))];		break;
			case ARMOR_NECK:	iHitLayer = sm_ArmorLayerNeck[Calc_GetRandVal(COUNTOF(sm_ArmorLayerNeck))];		break;
			case ARMOR_BACK:	iHitLayer = sm_ArmorLayerBack[Calc_GetRandVal(COUNTOF(sm_ArmorLayerBack))];		break;
			case ARMOR_CHEST:	iHitLayer = sm_ArmorLayerChest[Calc_GetRandVal(COUNTOF(sm_ArmorLayerChest))];	break;
			case ARMOR_ARMS:	iHitLayer = sm_ArmorLayerArms[Calc_GetRandVal(COUNTOF(sm_ArmorLayerArms))];		break;
			case ARMOR_HANDS:	iHitLayer = sm_ArmorLayerHands[Calc_GetRandVal(COUNTOF(sm_ArmorLayerHands))];	break;
			case ARMOR_LEGS:	iHitLayer = sm_ArmorLayerLegs[Calc_GetRandVal(COUNTOF(sm_ArmorLayerLegs))];		break;
			case ARMOR_FEET:	iHitLayer = sm_ArmorLayerFeet[Calc_GetRandVal(COUNTOF(sm_ArmorLayerFeet))];		break;
			default:			break;
		}

		CItem * pItemHit = LayerFind( iHitLayer );
		if ( pItemHit != NULL )
			pItemHit->OnTakeDamage( iDmg, pSrc, uType );
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

		if ( IsStatFlag(STATF_Freeze) )
		{
			StatFlag_Clear(STATF_Freeze);
			UpdateMode();
		}
	}

	// Disturb magic spells (only players can be disturbed)
	if ( m_pPlayer && pSrc != this && !(uType & DAMAGE_NODISTURB) && g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_MAGIC) )
	{
		// Check if my spell can be interrupted
		int iDisturbChance = 0;
		int iSpellSkill;
		const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
		if ( pSpellDef && pSpellDef->GetPrimarySkill(&iSpellSkill) )
			iDisturbChance = pSpellDef->m_Interrupt.GetLinear(Skill_GetBase(static_cast<SKILL_TYPE>(iSpellSkill)));

		if ( iDisturbChance && IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
		{
			// Protection spell can cancel the disturb
			CItem *pProtectionSpell = LayerFind(LAYER_SPELL_Protection);
			if ( pProtectionSpell )
			{
				int iChance = pProtectionSpell->m_itSpell.m_spelllevel;
				if ( iChance > Calc_GetRandVal(1000) )
					iDisturbChance = 0;
			}
		}

		if ( iDisturbChance > Calc_GetRandVal(1000) )
			Skill_Fail();
	}

	if ( pSrc && pSrc != this )
	{
		// Update attacker list
		bool bAttackerExists = false;
		for (std::vector<LastAttackers>::iterator it = m_lastAttackers.begin(); it != m_lastAttackers.end(); ++it)
		{
			LastAttackers & refAttacker = *it;
			if ( refAttacker.charUID == pSrc->GetUID().GetPrivateUID() )
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
			attacker.charUID = pSrc->GetUID().GetPrivateUID();
			attacker.elapsed = 0;
			attacker.amountDone = maximum( 0, iDmg );
			attacker.threat = maximum( 0, iDmg );
			m_lastAttackers.push_back(attacker);
		}

		// A physical blow of some sort.
		if (uType & (DAMAGE_HIT_BLUNT|DAMAGE_HIT_PIERCE|DAMAGE_HIT_SLASH))
		{
			// Check if Reactive Armor will reflect some damage back
			if ( IsStatFlag(STATF_Reactive) && !(uType & DAMAGE_GOD) )
			{
				if ( GetTopDist3D(pSrc) < 2 )
				{
					int iReactiveDamage = iDmg / 5;
					if ( iReactiveDamage < 1 )
						iReactiveDamage = 1;

					iDmg -= iReactiveDamage;
					pSrc->OnTakeDamage( iReactiveDamage, this, static_cast<DAMAGE_TYPE>(DAMAGE_FIXED), iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy );
					pSrc->Sound( 0x1F1 );
					pSrc->Effect( EFFECT_OBJ, ITEMID_FX_CURSE_EFFECT, this, 10, 16 );
				}
			}
		}
	}

	if (iDmg <= 0)
		return(0);

	// Apply damage
	SoundChar(CRESND_GETHIT);
	UpdateStatVal( STAT_STR, static_cast<short>(-iDmg));
	if ( pSrc->IsClient() )
		pSrc->GetClient()->addHitsUpdate( GetUID() );	// always send updates to src

	if ( IsAosFlagEnabled( FEATURE_AOS_DAMAGE ) )
	{
		if ( IsClient() )
			m_pClient->addShowDamage( iDmg, static_cast<dword>(GetUID()) );
		if ( pSrc->IsClient() && (pSrc != this) )
			pSrc->m_pClient->addShowDamage( iDmg, static_cast<dword>(GetUID()) );
		else
		{
			CChar * pSrcOwner = pSrc->NPC_PetGetOwner();
			if ( pSrcOwner != NULL )
			{
				if ( pSrcOwner->IsClient() )
					pSrcOwner->m_pClient->addShowDamage( iDmg, static_cast<dword>(GetUID()) );
			}
		}
	}

	if ( Stat_GetVal(STAT_STR) <= 0 )
	{
		// We will die from this. Make sure the killer is set correctly, otherwise the person we are currently attacking will get credit for killing us.
		m_Fight_Targ = pSrc->GetUID();
		return( iDmg );
	}

	if (m_atFight.m_War_Swing_State != WAR_SWING_SWINGING)	// don't interrupt my swing animation
		UpdateAnimate(ANIM_GET_HIT);

	return( iDmg );
}

//*******************************************************************************
// Fight specific memories.

//********************************************************

// What sort of weapon am i using?
SKILL_TYPE CChar::Fight_GetWeaponSkill() const
{
	ADDTOCALLSTACK("CChar::Fight_GetWeaponSkill");
	CItem * pWeapon = m_uidWeapon.ItemFind();
	if ( pWeapon == NULL )
		return( SKILL_WRESTLING );
	return( pWeapon->Weapon_GetSkill());
}

// Am i in an active fight mode ?
bool CChar::Fight_IsActive() const
{
	ADDTOCALLSTACK("CChar::Fight_IsActive");
	if ( ! IsStatFlag(STATF_War))
		return( false );

	SKILL_TYPE iSkillActive = Skill_GetActive();
	switch ( iSkillActive )
	{
		case SKILL_ARCHERY:
		case SKILL_FENCING:
		case SKILL_MACEFIGHTING:
		case SKILL_SWORDSMANSHIP:
		case SKILL_WRESTLING:
		case SKILL_THROWING:
			return( true );

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
	STAT_TYPE iStatBonus = static_cast<STAT_TYPE>(GetDefNum("COMBATBONUSSTAT"));
	int iStatBonusPercent = static_cast<int>(GetDefNum("COMBATBONUSPERCENT"));
	if ( pWeapon != NULL )
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
		int iDmgBonus = minimum(static_cast<int>(GetDefNum("INCREASEDAM", true, true)), 100);		// Damage Increase is capped at 100%

		// Racial Bonus (Berserk), gargoyles gains +15% Damage Increase per each 20 HP lost
		if ((g_Cfg.m_iRacialFlags & RACIALF_GARG_BERSERK) && IsGargoyle())
			iDmgBonus += minimum(15 * ((Stat_GetMax(STAT_STR) - Stat_GetVal(STAT_STR)) / 20), 60);		// value is capped at 60%

		// Horrific Beast (necro spell) add +25% Damage Increase
		if (g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B)
		{
			CItem * pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if (pPoly && pPoly->m_itSpell.m_spell == SPELL_Horrific_Beast)
				iDmgBonus += 25;
		}

		if (IsSetCombatFlags(COMBAT_OSIDAMAGEMOD))
		{
			// AOS damage bonus
			iDmgBonus += Skill_GetBase(SKILL_TACTICS) / 16;
			if (Skill_GetBase(SKILL_TACTICS) >= 1000)
				iDmgBonus += 6;	//6.25

			iDmgBonus += Skill_GetBase(SKILL_ANATOMY) / 20;
			if (Skill_GetBase(SKILL_ANATOMY) >= 1000)
				iDmgBonus += 5;

			if (pWeapon != NULL && pWeapon->IsType(IT_WEAPON_AXE))
			{
				iDmgBonus += Skill_GetBase(SKILL_LUMBERJACKING) / 50;
				if (Skill_GetBase(SKILL_LUMBERJACKING) >= 1000)
					iDmgBonus += 10;
			}

			if (Stat_GetAdjusted(STAT_STR) >= 100)
				iDmgBonus += 5;

			if (!iStatBonus)
				iStatBonus = static_cast<STAT_TYPE>(STAT_STR);
			if (!iStatBonusPercent)
				iStatBonusPercent = 30;
			iDmgBonus += Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;


			// pre-AOS damage bonus
			/*iDmgBonus += (Skill_GetBase(SKILL_TACTICS) - 500) / 10;

			iDmgBonus += Skill_GetBase(SKILL_ANATOMY) / 50;
			if (Skill_GetBase(SKILL_ANATOMY) >= 1000)
				iDmgBonus += 10;

			if (pWeapon != NULL && pWeapon->IsType(IT_WEAPON_AXE))
			{
				iDmgBonus += Skill_GetBase(SKILL_LUMBERJACKING) / 50;
				if (Skill_GetBase(SKILL_LUMBERJACKING) >= 1000)
					iDmgBonus += 10;
			}

			if (!iStatBonus)
				iStatBonus = static_cast<STAT_TYPE>(STAT_STR);
			if (!iStatBonusPercent)
				iStatBonusPercent = 20;
			iDmgBonus += Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;*/
		}
		else
		{
			// Sphere damage bonus (custom)
			if (!iStatBonus)
				iStatBonus = static_cast<STAT_TYPE>(STAT_STR);
			if (!iStatBonusPercent)
				iStatBonusPercent = 10;
			iDmgBonus = Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;

			iDmgMin += iDmgBonus;
			iDmgMax += iDmgBonus;
		}
		iDmgMin += iDmgMin * iDmgBonus / 100;
		iDmgMax += iDmgMax * iDmgBonus / 100;
	}

	if ( bNoRandom )
		return( bGetMax ? iDmgMax : iDmgMin );
	else
		return( Calc_GetRandVal2(iDmgMin, iDmgMax) );
}

// Clear all my active targets. Toggle out of war mode.
// Should I add @CombatEnd trigger here too?
void CChar::Fight_ClearAll()
{
	ADDTOCALLSTACK("CChar::Fight_ClearAll");
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsMemoryTypes(MEMORY_WAR_TARG) )
			Memory_ClearTypes(static_cast<CItemMemory *>(pItem), MEMORY_WAR_TARG);
	}

	Attacker_Clear();
	if ( Fight_IsActive() )
	{
		Skill_Start(SKILL_NONE);
		m_Fight_Targ.InitUID();
	}

	UpdateModeFlag();
}

// I no longer want to attack this char.
bool CChar::Fight_Clear(const CChar *pChar, bool bForced)
{
	ADDTOCALLSTACK("CChar::Fight_Clear");
	if ( !pChar || !Attacker_Delete(const_cast<CChar*>(pChar), bForced, ATTACKER_CLEAR_FORCED) )
		return false;

	// Go to my next target.
	if (m_Fight_Targ == pChar->GetUID())
		m_Fight_Targ.InitUID();

	if ( m_pNPC )
	{
		pChar = NPC_FightFindBestTarget();
		if ( pChar )
			Fight_Attack(pChar);
		else
			Fight_ClearAll();
	}

	return (pChar != NULL);	// I did not know about this ?
}

// We want to attack some one.
// But they won't notice til we actually hit them.
// This is just my intent.
// RETURN:
//  true = new attack is accepted.
bool CChar::Fight_Attack( const CChar *pCharTarg, bool btoldByMaster )
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
		Skill_Start(SKILL_NONE);
		return false;
	}

	CChar *pTarget = const_cast<CChar *>(pCharTarg);

	if ( g_Cfg.m_fAttackingIsACrime )
	{
		if ( pCharTarg->Noto_GetFlag(this) == NOTO_GOOD )
			CheckCrimeSeen(SKILL_NONE, pTarget, NULL, NULL);
	}

	INT64 threat = 0;
	if ( btoldByMaster )
		threat = 1000 + Attacker_GetHighestThreat();

	if ( ((IsTrigUsed(TRIGGER_ATTACK)) || (IsTrigUsed(TRIGGER_CHARATTACK))) && m_Fight_Targ != pCharTarg->GetUID() )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = threat;
		if ( OnTrigger(CTRIG_Attack, pTarget, &Args) == TRIGRET_RET_TRUE )
			return false;
		threat = Args.m_iN1;
	}

	if ( !Attacker_Add(pTarget, threat) )
		return false;
	if ( Attacker_GetIgnore(pTarget) )
		return false;

	// I'm attacking (or defending)
	if ( !IsStatFlag(STATF_War) )
	{
		StatFlag_Set(STATF_War);
		UpdateModeFlag();
		if ( IsClient() )
			GetClient()->addPlayerWarMode();
	}

	SKILL_TYPE skillWeapon = Fight_GetWeaponSkill();
	SKILL_TYPE skillActive = Skill_GetActive();

	if ( skillActive == skillWeapon && m_Fight_Targ == pCharTarg->GetUID() )		// already attacking this same target using the same skill
		return true;

	if ( m_pNPC && !btoldByMaster )		// call FindBestTarget when this CChar is a NPC and was not commanded to attack, otherwise it attack directly
		pTarget = NPC_FightFindBestTarget();

	m_Fight_Targ = pTarget ? pTarget->GetUID() : static_cast<CGrayUID>(UID_UNUSED);
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
	ASSERT( m_atFight.m_War_Swing_State == (WAR_SWING_READY|WAR_SWING_SWINGING) );

	CChar *pCharTarg = m_Fight_Targ.CharFind();
	if ( !pCharTarg || pCharTarg->IsStatFlag(STATF_DEAD) )	// dead players and dead bonded pets can't be attacked
	{
		if ( !Fight_Attack(NPC_FightFindBestTarget()) )
		{
			Skill_Start(SKILL_NONE);
			m_Fight_Targ.InitUID();
			if ( m_pNPC )
				StatFlag_Clear(STATF_War);
		}
		return;
	}

	switch ( Fight_Hit(pCharTarg) )
	{
		case WAR_SWING_INVALID:		// target is invalid
		{
			Fight_Clear(pCharTarg);
			if ( m_pNPC )
				Fight_Attack(NPC_FightFindBestTarget());
			return;
		}
		case WAR_SWING_EQUIPPING:	// keep hitting the same target
		{
			Skill_Start(Skill_GetActive());
			return;
		}
		case WAR_SWING_READY:		// probably too far away, can't take my swing right now
		{
			if ( m_pNPC )
				Fight_Attack(NPC_FightFindBestTarget());
			return;
		}
		case WAR_SWING_SWINGING:	// must come back here again to complete
			return;
		default:
			break;
	}

	ASSERT(0);
}

// Distance from which I can hit
int CChar::CalcFightRange( CItem * pWeapon )
{
	ADDTOCALLSTACK("CChar::CalcFightRange");

	int iCharRange = RangeL();
	int iWeaponRange = pWeapon ? pWeapon->RangeL() : 0;

	return ( maximum(iCharRange , iWeaponRange) );
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

	if ( !pCharTarg || pCharTarg == this )
		return WAR_SWING_INVALID;

	DAMAGE_TYPE iTyp = DAMAGE_HIT_BLUNT;

	if ( IsTrigUsed(TRIGGER_HITCHECK) )
	{
		CScriptTriggerArgs pArgs;
		pArgs.m_iN1 = m_atFight.m_War_Swing_State;
		pArgs.m_iN2 = iTyp;
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_HitCheck, pCharTarg, &pArgs);
		if ( tRet == TRIGRET_RET_TRUE )
			return (WAR_SWING_TYPE)pArgs.m_iN1;
		if ( tRet == -1 )
			return WAR_SWING_INVALID;

		m_atFight.m_War_Swing_State = static_cast<WAR_SWING_TYPE>(pArgs.m_iN1);
		iTyp = static_cast<DAMAGE_TYPE>(pArgs.m_iN2);

		if ( (m_atFight.m_War_Swing_State == WAR_SWING_SWINGING) && (iTyp & DAMAGE_FIXED) )
		{
			if ( tRet == TRIGRET_RET_DEFAULT )
				return WAR_SWING_EQUIPPING;

			if ( iTyp == DAMAGE_HIT_BLUNT )		// if type did not change in the trigger, default iTyp is set
			{
				CItem *pWeapon = m_uidWeapon.ItemFind();
				if ( pWeapon )
				{
					CVarDefCont *pDamTypeOverride = pWeapon->GetKey("OVERRIDE.DAMAGETYPE", true);
					if ( pDamTypeOverride )
						iTyp = static_cast<DAMAGE_TYPE>(pDamTypeOverride->GetValNum());
					else
					{
						CItemBase *pWeaponDef = pWeapon->Item_GetDef();
						switch ( pWeaponDef->GetType() )
						{
							case IT_WEAPON_SWORD:
							case IT_WEAPON_AXE:
							case IT_WEAPON_THROWING:
								iTyp |= DAMAGE_HIT_SLASH;
								break;
							case IT_WEAPON_FENCE:
							case IT_WEAPON_BOW:
							case IT_WEAPON_XBOW:
								iTyp |= DAMAGE_HIT_PIERCE;
								break;
							default:
								break;
						}
					}
				}
			}
			if ( iTyp & DAMAGE_FIXED )
				iTyp &= ~DAMAGE_FIXED;

			pCharTarg->OnTakeDamage(
				Fight_CalcDamage(m_uidWeapon.ItemFind()),
				this,
				iTyp,
				static_cast<int>(GetDefNum("DAMPHYSICAL", true)),
				static_cast<int>(GetDefNum("DAMFIRE", true)),
				static_cast<int>(GetDefNum("DAMCOLD", true)),
				static_cast<int>(GetDefNum("DAMPOISON", true)),
				static_cast<int>(GetDefNum("DAMENERGY", true))
				);

			return WAR_SWING_EQUIPPING;
		}
	}

	// Very basic check on possibility to hit
	if ( IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_Freeze|STATF_Stone) || pCharTarg->IsStatFlag(STATF_DEAD|STATF_INVUL|STATF_Stone|STATF_Ridden) )
		return WAR_SWING_INVALID;
	if ( pCharTarg->m_pArea && pCharTarg->m_pArea->IsFlag(REGION_FLAG_SAFE) )
		return WAR_SWING_INVALID;

	int dist = GetTopDist3D(pCharTarg);
	if ( dist > UO_MAP_VIEW_RADAR )
	{
		if ( IsSetCombatFlags(COMBAT_STAYINRANGE) )
			return WAR_SWING_EQUIPPING;

		return WAR_SWING_INVALID;
	}

	// I am on ship. Should be able to combat only inside the ship to avoid free sea and ground characters hunting
	if ( (m_pArea != pCharTarg->m_pArea) && !IsSetCombatFlags(COMBAT_ALLOWHITFROMSHIP) )
	{
		if ( m_pArea && m_pArea->IsFlag(REGION_FLAG_SHIP) )
		{
			SysMessageDefault(DEFMSG_COMBAT_OUTSIDESHIP);
			return WAR_SWING_INVALID;
		}
		if ( pCharTarg->m_pArea && pCharTarg->m_pArea->IsFlag(REGION_FLAG_SHIP) )
		{
			SysMessageDefault(DEFMSG_COMBAT_INSIDESHIP);
			return WAR_SWING_INVALID;
		}
	}

	// Guards should remove conjured NPCs
	if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD && pCharTarg->m_pNPC && pCharTarg->IsStatFlag(STATF_Conjured) )
	{
		pCharTarg->Delete();
		return WAR_SWING_EQUIPPING;
	}

	// Fix of the bounce back effect with dir update for clients to be able to run in combat easily
	if ( IsClient() && IsSetCombatFlags(COMBAT_FACECOMBAT) )
	{
		DIR_TYPE dirOpponent = GetDir(pCharTarg, m_dirFace);
		if ( (dirOpponent != m_dirFace) && (dirOpponent != GetDirTurn(m_dirFace, -1)) && (dirOpponent != GetDirTurn(m_dirFace, 1)) )
			return WAR_SWING_READY;
	}

	if ( IsSetCombatFlags(COMBAT_PREHIT) && (m_atFight.m_War_Swing_State == WAR_SWING_READY) )
	{
		INT64 diff = GetKeyNum("LastHit", true) - g_World.GetCurrentTime().GetTimeRaw();
		if ( diff > 0 )
		{
			diff = (diff > 50) ? 50 : diff;
			SetTimeout(diff);
			return WAR_SWING_READY;
		}
	}

	CItem *pWeapon = m_uidWeapon.ItemFind();
	CItem *pAmmo = NULL;
	CItemBase *pWeaponDef = NULL;
	CVarDefCont *pType = NULL;
	CVarDefCont *pCont = NULL;
	CVarDefCont *pAnim = NULL;
	CVarDefCont *pColor = NULL;
	CVarDefCont *pRender = NULL;
	if ( pWeapon )
	{
		pType = pWeapon->GetDefKey("AMMOTYPE", true);
		pCont = pWeapon->GetDefKey("AMMOCONT", true);
		pAnim = pWeapon->GetDefKey("AMMOANIM", true);
		pColor = pWeapon->GetDefKey("AMMOANIMHUE", true);
		pRender = pWeapon->GetDefKey("AMMOANIMRENDER", true);
		CVarDefCont *pDamTypeOverride = pWeapon->GetKey("OVERRIDE.DAMAGETYPE", true);
		if ( pDamTypeOverride )
			iTyp = static_cast<DAMAGE_TYPE>(pDamTypeOverride->GetValNum());
		else
		{
			pWeaponDef = pWeapon->Item_GetDef();
			switch ( pWeaponDef->GetType() )
			{
				case IT_WEAPON_SWORD:
				case IT_WEAPON_AXE:
				case IT_WEAPON_THROWING:
					iTyp |= DAMAGE_HIT_SLASH;
					break;
				case IT_WEAPON_FENCE:
				case IT_WEAPON_BOW:
				case IT_WEAPON_XBOW:
					iTyp |= DAMAGE_HIT_PIERCE;
					break;
				default:
					break;
			}
		}
	}

	SKILL_TYPE skill = Skill_GetActive();
	RESOURCE_ID_BASE rid;
	LPCTSTR t_Str;

	if ( g_Cfg.IsSkillFlag(skill, SKF_RANGED) )
	{
		if ( IsStatFlag(STATF_HasShield) )		// this should never happen
		{
			SysMessageDefault(DEFMSG_ITEMUSE_BOW_SHIELD);
			return WAR_SWING_INVALID;
		}

		int	iMinDist = pWeapon ? pWeapon->RangeH() : g_Cfg.m_iArcheryMinDist;
		int	iMaxDist = pWeapon ? pWeapon->RangeL() : g_Cfg.m_iArcheryMaxDist;
		if ( !iMaxDist || (iMinDist == 0 && iMaxDist == 1) )
			iMaxDist = g_Cfg.m_iArcheryMaxDist;
		if ( !iMinDist )
			iMinDist = g_Cfg.m_iArcheryMinDist;

		if ( dist < iMinDist )
		{
			SysMessageDefault(DEFMSG_COMBAT_ARCH_TOOCLOSE);
			if ( !IsSetCombatFlags(COMBAT_STAYINRANGE) || m_atFight.m_War_Swing_State != WAR_SWING_SWINGING )
				return(WAR_SWING_READY);
			return WAR_SWING_EQUIPPING;
		}
		else if ( dist > iMaxDist )
		{
			if ( !IsSetCombatFlags(COMBAT_STAYINRANGE) || m_atFight.m_War_Swing_State != WAR_SWING_SWINGING )
				return(WAR_SWING_READY);
			return WAR_SWING_EQUIPPING;
		}
		if ( pType )
		{
			t_Str = pType->GetValStr();
			rid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID(RES_ITEMDEF, t_Str));
		}
		else
			rid = pWeaponDef->m_ttWeaponBow.m_idAmmo;

		ITEMID_TYPE AmmoID = static_cast<ITEMID_TYPE>(rid.GetResIndex());
		if ( AmmoID )
		{
			if ( pCont )
			{
				CGrayUID uidCont = static_cast<CGrayUID>(static_cast<dword>(pCont->GetValNum()));
				CItemContainer *pNewCont = dynamic_cast<CItemContainer*>(uidCont.ItemFind());
				if ( !pNewCont )	//if no UID, check for ITEMID_TYPE
				{
					t_Str = pCont->GetValStr();
					RESOURCE_ID_BASE rContid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID(RES_ITEMDEF, t_Str));
					ITEMID_TYPE ContID = static_cast<ITEMID_TYPE>(rContid.GetResIndex());
					if ( ContID )
						pNewCont = dynamic_cast<CItemContainer*>(ContentFind(rContid));
				}

				pAmmo = (pNewCont) ? pNewCont->ContentFind(rid) : ContentFind(rid);
			}
			else
				pAmmo = ContentFind(rid);

			if ( !pAmmo && m_pPlayer )
			{
				SysMessageDefault(DEFMSG_COMBAT_ARCH_NOAMMO);
				return WAR_SWING_INVALID;
			}
		}
	}
	else
	{
		int	iMinDist = pWeapon ? pWeapon->RangeH() : 0;
		int	iMaxDist = CalcFightRange(pWeapon);
		if ( dist < iMinDist || dist > iMaxDist )
		{
			if ( !IsSetCombatFlags(COMBAT_STAYINRANGE) || m_atFight.m_War_Swing_State != WAR_SWING_SWINGING )
				return(WAR_SWING_READY);
			return WAR_SWING_EQUIPPING;
		}
	}

	// Start the swing
	if ( m_atFight.m_War_Swing_State == WAR_SWING_READY )
	{
		if ( !CanSeeLOS(pCharTarg) )
			return WAR_SWING_EQUIPPING;

		ANIM_TYPE anim = GenerateAnimate(ANIM_ATTACK_WEAPON);
		int animDelay = 7;		// attack speed is always 7ms and then the char keep waiting the remaining time
		int iSwingDelay = g_Cfg.Calc_CombatAttackSpeed(this, pWeapon) - 1;	// swings are started only on the next tick, so substract -1 to compensate that

		if ( IsTrigUsed(TRIGGER_HITTRY) )
		{
			CScriptTriggerArgs Args(iSwingDelay, 0, pWeapon);
			Args.m_VarsLocal.SetNum("Anim", (int)anim);
			Args.m_VarsLocal.SetNum("AnimDelay", animDelay);
			if ( OnTrigger(CTRIG_HitTry, pCharTarg, &Args) == TRIGRET_RET_TRUE )
				return WAR_SWING_READY;

			iSwingDelay = static_cast<int>(Args.m_iN1);
			anim = static_cast<ANIM_TYPE>(Args.m_VarsLocal.GetKeyNum("Anim", false));
			animDelay = static_cast<int>(Args.m_VarsLocal.GetKeyNum("AnimDelay", true));
			if ( iSwingDelay < 0 )
				iSwingDelay = 0;
			if ( animDelay < 0 )
				animDelay = 0;
		}

		m_atFight.m_War_Swing_State = WAR_SWING_SWINGING;
		m_atFight.m_timeNextCombatSwing = CServTime::GetCurrentTime() + iSwingDelay;

		if ( IsSetCombatFlags(COMBAT_PREHIT) )
		{
			SetKeyNum("LastHit", g_World.GetCurrentTime().GetTimeRaw() + iSwingDelay);
			SetTimeout(0);
		}
		else
			SetTimeout(animDelay);

		Reveal();
		if ( !IsSetCombatFlags(COMBAT_NODIRCHANGE) )
			UpdateDir(pCharTarg);
		UpdateAnimate(anim, false, false, static_cast<byte>(animDelay / TICK_PER_SEC));
		return WAR_SWING_SWINGING;
	}

	if ( g_Cfg.IsSkillFlag(skill, SKF_RANGED) )
	{
		// Post-swing behavior
		ITEMID_TYPE AmmoAnim;
		if ( pAnim )
		{
			t_Str = pAnim->GetValStr();
			rid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID(RES_ITEMDEF, t_Str));
			AmmoAnim = static_cast<ITEMID_TYPE>(rid.GetResIndex());
		}
		else
			AmmoAnim = static_cast<ITEMID_TYPE>(pWeaponDef->m_ttWeaponBow.m_idAmmoX.GetResIndex());

		dword AmmoHue = 0;
		if ( pColor )
			AmmoHue = static_cast<dword>(pColor->GetValNum());

		dword AmmoRender = 0;
		if ( pRender )
			AmmoRender = static_cast<dword>(pRender->GetValNum());

		pCharTarg->Effect(EFFECT_BOLT, AmmoAnim, this, 18, 1, false, AmmoHue, AmmoRender);
	}

	// We made our swing. so we must recoil.
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
				return WAR_SWING_EQUIPPING;

			if ( Args.m_VarsLocal.GetKeyNum("ArrowHandled") != 0 )		// if arrow is handled by script, do nothing with it further!
				pAmmo = NULL;
		}

		if ( pAmmo && m_pPlayer && (40 >= Calc_GetRandVal(100)) )
		{
			pAmmo->UnStackSplit(1);
			pAmmo->MoveToDecay(pCharTarg->GetTopPoint(), g_Cfg.m_iDecay_Item);
		}

		SOUND_TYPE iSound = 0;
		if ( pWeapon )
			iSound = static_cast<SOUND_TYPE>(pWeapon->GetDefNum("AMMOSOUNDMISS", true));
		if ( !iSound )
		{
			if ( g_Cfg.IsSkillFlag(skill, SKF_RANGED) )
			{
				static const SOUND_TYPE sm_Snd_Miss[] = { 0x233, 0x238 };
				iSound = sm_Snd_Miss[Calc_GetRandVal(COUNTOF(sm_Snd_Miss))];
			}
			else
			{
				static const SOUND_TYPE sm_Snd_Miss[] = { 0x238, 0x239, 0x23a };
				iSound = sm_Snd_Miss[Calc_GetRandVal(COUNTOF(sm_Snd_Miss))];
			}
		}

		if ( IsPriv(PRIV_DETAIL) )
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_MISSS), pCharTarg->GetName());
		if ( pCharTarg->IsPriv(PRIV_DETAIL) )
			pCharTarg->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_MISSO), GetName());

		Sound(iSound);
		return WAR_SWING_EQUIPPING;
	}

	// We hit
	if ( !(iTyp & DAMAGE_GOD) )
	{
		// Check if target will block the hit
		// Legacy pre-SE formula
		CItem *pItemHit = NULL;
		int ParryChance = 0;
		if ( pCharTarg->IsStatFlag(STATF_HasShield) )	// parry using shield
		{
			pItemHit = pCharTarg->LayerFind(LAYER_HAND2);
			ParryChance = pCharTarg->Skill_GetBase(SKILL_PARRYING) / 40;
		}
		else if ( pCharTarg->m_uidWeapon.IsItem() )		// parry using weapon
		{
			pItemHit = pCharTarg->m_uidWeapon.ItemFind();
			ParryChance = pCharTarg->Skill_GetBase(SKILL_PARRYING) / 80;
		}

		if ( pCharTarg->Skill_GetBase(SKILL_PARRYING) >= 1000 )
			ParryChance += 5;

		int Dex = pCharTarg->Stat_GetAdjusted(STAT_DEX);
		if ( Dex < 80 )
			ParryChance = ParryChance * (20 + Dex) / 100;

		if ( pCharTarg->Skill_UseQuick(SKILL_PARRYING, ParryChance, true, false) )
		{
			if ( IsPriv(PRIV_DETAIL) )
				SysMessageDefault(DEFMSG_COMBAT_PARRY);
			if ( pItemHit )
				pItemHit->OnTakeDamage(1, this, iTyp);

			//Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16);		// moved to scripts (@UseQuick on Parrying skill)
			return WAR_SWING_EQUIPPING;
		}
	}

	// Calculate base damage
	int	iDmg = Fight_CalcDamage(pWeapon);
	
	CScriptTriggerArgs Args(iDmg, iTyp, pWeapon);
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
			pAmmo = NULL;

		iDmg = static_cast<int>(Args.m_iN1);
		iTyp = static_cast<DAMAGE_TYPE>(Args.m_iN2);
	}

	// BAD BAD Healing fix.. Cant think of something else -- Radiant
	if ( pCharTarg->m_Act_SkillCurrent == SKILL_HEALING )
	{
		pCharTarg->SysMessageDefault(DEFMSG_HEALING_INTERRUPT);
		pCharTarg->Skill_Cleanup();
	}

	if ( pAmmo )
	{
		pAmmo->UnStackSplit(1);
		if ( pCharTarg->m_pNPC && (40 >= Calc_GetRandVal(100)) )
			pCharTarg->ItemBounce(pAmmo, false);
		else
			pAmmo->Delete();
	}

	// Hit noise (based on weapon type)
	SoundChar(CRESND_HIT);

	if ( pWeapon )
	{
		// Check if the weapon is poisoned
		if ( !IsSetCombatFlags(COMBAT_NOPOISONHIT) && pWeapon->m_itWeapon.m_poison_skill && pWeapon->m_itWeapon.m_poison_skill > Calc_GetRandVal(100) )
		{
			byte iPoisonDeliver = static_cast<byte>(Calc_GetRandVal(pWeapon->m_itWeapon.m_poison_skill));
			pCharTarg->SetPoison(10 * iPoisonDeliver, iPoisonDeliver / 5, this);

			pWeapon->m_itWeapon.m_poison_skill -= iPoisonDeliver / 2;	// reduce weapon poison charges
			pWeapon->UpdatePropertyFlag(AUTOTOOLTIP_FLAG_POISON);
		}

		// Check if the weapon will be damaged
		int iDamageChance = static_cast<int>(Args.m_VarsLocal.GetKeyNum("ItemDamageChance", true));
		if ( iDamageChance > Calc_GetRandVal(100) )
			pWeapon->OnTakeDamage(iDmg, pCharTarg);
	}
	else if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_MONSTER )
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
	iDmg = pCharTarg->OnTakeDamage(
		iDmg,
		this,
		iTyp,
		static_cast<int>(GetDefNum("DAMPHYSICAL", true, true)),
		static_cast<int>(GetDefNum("DAMFIRE", true, true)),
		static_cast<int>(GetDefNum("DAMCOLD", true, true)),
		static_cast<int>(GetDefNum("DAMPOISON", true, true)),
		static_cast<int>(GetDefNum("DAMENERGY", true, true))
		);

	if ( iDmg > 0 )
	{
		CItem *pCurseWeapon = LayerFind(LAYER_SPELL_Curse_Weapon);
		short iHitLifeLeech = static_cast<short>(GetDefNum("HitLeechLife", true));
		if ( pWeapon && pCurseWeapon )
			iHitLifeLeech += pCurseWeapon->m_itSpell.m_spelllevel;

		bool bMakeLeechSound = false;
		if ( iHitLifeLeech )
		{
			iHitLifeLeech = static_cast<short>(Calc_GetRandVal2(0, (iDmg * iHitLifeLeech * 30) / 10000));	// leech 0% ~ 30% of damage value
			UpdateStatVal(STAT_STR, iHitLifeLeech, Stat_GetMax(STAT_STR));
			bMakeLeechSound = true;
		}

		short iHitManaLeech = static_cast<short>(GetDefNum("HitLeechMana", true));
		if ( iHitManaLeech )
		{
			iHitManaLeech = static_cast<short>(Calc_GetRandVal2(0, (iDmg * iHitManaLeech * 40) / 10000));	// leech 0% ~ 40% of damage value
			UpdateStatVal(STAT_INT, iHitManaLeech, Stat_GetMax(STAT_INT));
			bMakeLeechSound = true;
		}

		if ( GetDefNum("HitLeechStam", true) > Calc_GetRandLLVal(100) )
		{
			UpdateStatVal(STAT_DEX, static_cast<short>(iDmg), Stat_GetMax(STAT_DEX));	// leech 100% of damage value
			bMakeLeechSound = true;
		}

		short iManaDrain = 0;
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
		{
			CItem *pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if ( pPoly && pPoly->m_itSpell.m_spell == SPELL_Wraith_Form )
				iManaDrain += 5 + (15 * Skill_GetBase(SKILL_SPIRITSPEAK) / 1000);
		}
		if ( GetDefNum("HitManaDrain", true) > Calc_GetRandLLVal(100) )
			iManaDrain += IMULDIV(static_cast<short>(iDmg), 20, 100);	// leech 20% of damage value

		short iTargMana = pCharTarg->Stat_GetVal(STAT_INT);
		if ( iManaDrain > iTargMana )
			iManaDrain = iTargMana;

		if ( iManaDrain > 0 )
		{
			pCharTarg->UpdateStatVal(STAT_INT, iTargMana - iManaDrain);
			UpdateStatVal(STAT_INT, iManaDrain, Stat_GetMax(STAT_INT));
			bMakeLeechSound = true;
		}

		if ( bMakeLeechSound )
			Sound(0x44d);

		// Make blood effects
		if ( m_wBloodHue != static_cast<HUE_TYPE>(-1) )
		{
			static const ITEMID_TYPE sm_Blood[] = { ITEMID_BLOOD1, ITEMID_BLOOD2, ITEMID_BLOOD3, ITEMID_BLOOD4, ITEMID_BLOOD5, ITEMID_BLOOD6, ITEMID_BLOOD_SPLAT };
			int iBloodQty = (g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE) ? Calc_GetRandVal2(4, 5) : Calc_GetRandVal2(1, 2);

			for ( int i = 0; i < iBloodQty; i++ )
			{
				ITEMID_TYPE iBloodID = sm_Blood[Calc_GetRandVal(COUNTOF(sm_Blood))];
				CItem *pBlood = CItem::CreateBase(iBloodID);
				ASSERT(pBlood);
				pBlood->SetHue(pCharTarg->m_wBloodHue);
				pBlood->MoveNear(pCharTarg->GetTopPoint(), 1);
				pBlood->SetDecayTime(5 * TICK_PER_SEC);
			}
		}

		// Check for passive skill gain
		if ( m_pPlayer && !pCharTarg->m_pArea->IsFlag(REGION_FLAG_NO_PVP) )
		{
			Skill_Experience(skill, m_Act_Difficulty);
			Skill_Experience(SKILL_TACTICS, m_Act_Difficulty);
		}
	}

	return WAR_SWING_EQUIPPING;
}
