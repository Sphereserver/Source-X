// Test things to judge what an NPC might be thinking. (want to do)
// But take no actions here.

#include "../items/CItemVendable.h"
#include "../spheresvr.h"
#include "CChar.h"
#include "CCharNPC.h"

CREID_TYPE CChar::NPC_GetAllyGroupType(CREID_TYPE idTest) // static
{
	ADDTOCALLSTACK("CChar::NPC_GetAllyGroupType");
	switch ( idTest )
	{
		case CREID_MAN:
		case CREID_WOMAN:
		case CREID_GHOSTMAN:
		case CREID_GHOSTWOMAN:
			return( CREID_MAN );
		case CREID_ELFMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTMAN:
		case CREID_ELFGHOSTWOMAN:
			return( CREID_ELFMAN );
		case CREID_GARGMAN:
		case CREID_GARGWOMAN:
		case CREID_GARGGHOSTMAN:
		case CREID_GARGGHOSTWOMAN:
			return( CREID_GARGMAN );

		case CREID_OGRE:
		case CREID_OGRE_LORD:
		case CREID_OGRE_LORD_ARCTIC:
			return CREID_OGRE;
		case CREID_ETTIN:
		case CREID_ETTIN_AXE:
			return( CREID_ETTIN );
		case CREID_TROLL_SWORD:
		case CREID_TROLL:
		case CREID_TROLL_MACE:
			return( CREID_TROLL );
		case CREID_ORC:
		case CREID_ORC_CLUB:
		case CREID_ORC_CAPTAIN:
		case CREID_ORC_LORD:
		case CREID_ORC_CHOPPER:
		case CREID_ORC_MAGE:
		case CREID_ORC_BOMBER:
		case CREID_ORC_SCOUT:
		case CREID_ORC_BRUTE:
			return( CREID_ORC );
		case CREID_SNAKE:
		case CREID_GIANT_SNAKE:
		case CREID_SERPENT_ICE:
		case CREID_SERPENT_LAVA:
		case CREID_SERPENT_COIL:
		case CREID_SERPENT_ARROW:
			return CREID_SNAKE;
		case CREID_DEMON:
		case CREID_DEMON_SWORD:
		case CREID_ICE_FIEND:
		case CREID_DEMON_EXODUS:
			return( CREID_DEMON );
		case CREID_DRAKE_GREY:
		case CREID_DRAKE_RED:
		case CREID_DRAGON_GREY:
		case CREID_DRAGON_RED:
		case CREID_DRAGON_SERPENTINE:
		//case CREID_DRAGON_SKELETAL:
		//case CREID_DRAGON_GREATER:
		//case CREID_WYVERN:
			return CREID_DRAKE_GREY;
		case CREID_WYRM_SHADOW:
		case CREID_WYRM_WHITE_UNK:
		case CREID_WYRM_ANCIENT:
			return CREID_WYRM_SHADOW;
		case CREID_DRAGON_CRIMSON:
		case CREID_DRAKE_CRIMSON:
		case CREID_DRAKE_CRIMSON_ELEM:
			return CREID_DRAGON_CRIMSON;
		case CREID_DRAGON_PLATINUM:
		case CREID_DRAKE_PLATINUM:
		case CREID_DRAKE_PLATINUM_ELEM:
			return CREID_DRAGON_PLATINUM;
		case CREID_DRAGON_STYGIAN:
		case CREID_DRAKE_STYGIAN:
		case CREID_DRAKE_STYGIAN_ELEM:
			return CREID_DRAGON_STYGIAN;
		case CREID_SWAMP_DRAGON:
		case CREID_SWAMP_DRAGON_AR:
			return CREID_SWAMP_DRAGON;
		case CREID_DRAGON_TURTLE:
		case CREID_DRAGON_TURTLE_HATCHL:
			return CREID_DRAGON_TURTLE;
		case CREID_GARGOYLE:
		case CREID_GARG_STONE:
		case CREID_GARG_FIRE:
		case CREID_GARG_UNDEAD:
		case CREID_GARG_ENSLAVED:
			return CREID_GARGOYLE;
		case CREID_GAZER:
		case CREID_GAZER2:
		case CREID_GAZER_ELDER:
		case CREID_GAZER_LARVA:
			return CREID_GAZER;
		case CREID_LICH:
		case CREID_LICH_LORD:
		case CREID_LICH_ANCIENT:
		case CREID_LICH_PRIMEVAL:
			return CREID_LICH;
		case CREID_LIZMAN:
		case CREID_LIZMAN_SPEAR:
		case CREID_LIZMAN_MACE:
			return( CREID_LIZMAN );
		case CREID_RATMAN:
		case CREID_RATMAN_CLUB:
		case CREID_RATMAN_SWORD:
		case CREID_RATMAN_ARCHER:
		case CREID_RATMAN_MAGE:
			return( CREID_RATMAN );
		case CREID_HARPY:
		case CREID_HARPY_STONE:
			return CREID_HARPY;
		case CREID_SKELETON:
		case CREID_SKELETON_AXE:
		case CREID_SKELETON_SW_SH:
		case CREID_SKELETON_KNIGHT:
		case CREID_SKELETON_MAGE:
			return( CREID_SKELETON );
		case CREID_ZOMBIE:
		case CREID_MUMMY:
		case CREID_GHOUL:
			return CREID_ZOMBIE;
		case CREID_BANSHEE:
		case CREID_SPECTRE:
			return CREID_BANSHEE;
		case CREID_SLIME:
		case CREID_SLIME_CORROSIVE:
		case CREID_OOZE_FROST:
			return CREID_SLIME;
		case CREID_SEA_CREATURE:
		case CREID_SEA_SERPENT:
		case CREID_SEA_SERPENT_DEEP:
		case CREID_HORSE_SEA:
			return CREID_SEA_CREATURE;
		case CREID_GIANT_TOAD:
		case CREID_BULL_FROG:
			return CREID_GIANT_TOAD;
		case CREID_TERA_WARRIOR:
		case CREID_TERA_DRONE:
		case CREID_TERA_MATRIARCH:
			return( CREID_TERA_DRONE );
		case CREID_OPHID_SHAMAN:
		case CREID_OPHID_WARRIOR:
		case CREID_OPHID_QUEEN:
		case CREID_OPHID_JUSTICAR:
		case CREID_OPHID_AVENGER:
			return( CREID_OPHID_WARRIOR );
		case CREID_JUKA_WARRIOR:
		case CREID_JUKA_MAGE:
		case CREID_JUKA_LORD:
			return CREID_JUKA_WARRIOR;
		case CREID_MEER_MAGE:
		case CREID_MEER_WARRIOR:
		case CREID_MEER_ETERNAL:
		case CREID_MEER_CAPTAIN:
			return CREID_MEER_MAGE;
		case CREID_ELEM_ORE_AGA:
		case CREID_ELEM_ORE_BRONZ:
		case CREID_ELEM_ORE_COPP:
		case CREID_ELEM_ORE_DCOPP:
		case CREID_ELEM_ORE_SHAD:
		case CREID_ELEM_ORE_VALO:
		case CREID_ELEM_ORE_VERI:
		case CREID_ELEM_ORE_GOLD:
			return CREID_ELEM_ORE_AGA;
		case CREID_HELLHOUND:
		case CREID_HELLHOUND_LEAD:
		case CREID_HELLHOUND_ANCIENT:
			return CREID_HELLHOUND;
		case CREID_EVILMAGE:
		case CREID_EVILMAGE_LORD:
			return CREID_EVILMAGE;
		case CREID_BOGLING:
		case CREID_BOG_THING:
			return CREID_BOGLING;
		case CREID_SOLEN_WORKER:
		case CREID_SOLEN_WARRIOR:
		case CREID_SOLEN_QUEEN:
		case CREID_SOLEN_MATRIARCH:
			return CREID_SOLEN_WORKER;
		case CREID_SOLEN_WORKER_BK:
		case CREID_SOLEN_WARRIOR_BK:
		case CREID_SOLEN_QUEEN_BK:
		case CREID_SOLEN_MATRIARCH_BK:
			return CREID_SOLEN_WORKER_BK;
		case CREID_HORDE_DEMON_L:
		case CREID_HORDE_DEMON_MULTIC:
		case CREID_HORDE_DEMON_MINION:
			return CREID_HORDE_DEMON_L;
		case CREID_MINOTAUR:
		case CREID_MINOTAUR_CAPTAIN:
		case CREID_MINOTAUR_LORD:
		case CREID_MINOTAUR_SCOUT:
		case CREID_MINOTAUR_TORM:
			return CREID_MINOTAUR;
		case CREID_SHIMMERING_EFF:
		case CREID_MANTRA_EFF:
		case CREID_FETID_ESSENCE:
		case CREID_CORPOREAL_BRUME:
			return CREID_SHIMMERING_EFF;
		case CREID_YOMOTSU_WARRIOR:
		case CREID_YOMOTSU_PRIEST:
		case CREID_YOMOTSU_ELDER:
			return CREID_YOMOTSU_WARRIOR;
		case CREID_GIANT_BEETLE:
		case CREID_FIRE_BEETLE:
			return CREID_GIANT_BEETLE;
		case CREID_GOBLIN_GREEN:
		case CREID_GOBLIN_GREEN_MAGE:
			return CREID_GOBLIN_GREEN;
		case CREID_WILD_TIGER:
		case CREID_TIGER_CUB:
		case CREID_SABERTOOTHED_TIGER:
			return CREID_WILD_TIGER;
		case CREID_BOURA:
		case CREID_BOURA_AR:
			return CREID_BOURA;
		case CREID_KEPETCH:
		case CREID_KEPETCH_SHORN:
			return CREID_KEPETCH;
		case CREID_SPIDER_WOLF:
		case CREID_SPIDER_TRAPDOOR:
			return CREID_SPIDER_WOLF;
		case CREID_VAMPIRE:
		case CREID_VAMPIRE_FEMALE:
			return CREID_VAMPIRE;
		case CREID_CHARYBDIS:
		case CREID_CHARYBDIS_TENTACLE:
			return CREID_CHARYBDIS;
		case CREID_PUMPKIN_DEMON:
		case CREID_PUMPKIN_FRIGHT:
			return CREID_PUMPKIN_DEMON;
		case CREID_DIMETROSAUR:
		case CREID_GALLUSAURUS:
		case CREID_ARCHAEOSAURUS:
		case CREID_NAJASAURUS:
		case CREID_ALLOSAURUS:
		case CREID_ANCHISAUR:
		case CREID_TYRANNOSAURUS_REX:
			return CREID_DIMETROSAUR;
		case CREID_MYRMIDEX_LARVAE:
		case CREID_MYRMIDEX_DRONE:
		case CREID_MYRMIDEX_WARRIOR:
		case CREID_MYRMIDEX_QUEEN:
			return CREID_MYRMIDEX_LARVAE;
		case CREID_KOTL_AUTOMATON:
		case CREID_ZIPACTRIOTL:
			return CREID_KOTL_AUTOMATON;

		case CREID_EAGLE:
		case CREID_BIRD:
		case CREID_BIRD_CROW:
		case CREID_PARROT:
			return CREID_BIRD;
		case CREID_HORSE_TAN:
		case CREID_HORSE_BROWN_DK:
		case CREID_HORSE_GRAY:
		case CREID_HORSE_BROWN_LT:
		case CREID_HORSE_PACK:
		case CREID_STEED_SILVER:
		case CREID_STEED_DARK:
		case CREID_WARHORSE_BRIT:
		case CREID_WARHORSE_MAGE:
		case CREID_WARHORSE_MINAX:
		case CREID_WARHORSE_SHAD:
		case CREID_WARHORSE_DREAD:
		case CREID_MARE_UNK1:
		case CREID_MARE_UNK2:
		case CREID_HORSE_PALOMINO:
			return( CREID_HORSE_TAN );
		case CREID_BEAR_BROWN:
		case CREID_BEAR_GRIZZLY:
		case CREID_BEAR_POLAR:
			return( CREID_BEAR_BROWN );
		case CREID_COW_BW:
		case CREID_COW_BROWN:
		case CREID_BULL_BROWN_LT:
		case CREID_BULL_BROWN_DK:
			return( CREID_BULL_BROWN_LT );
		case CREID_OSTARD_DESERT:
		case CREID_OSTARD_FRENZ:
		case CREID_OSTARD_FOREST:
			return( CREID_OSTARD_FOREST );
		case CREID_SHEEP:
		case CREID_SHEEP_SHORN:
			return( CREID_SHEEP );
		case CREID_HART:
		case CREID_DEER:
			return( CREID_DEER );
		case CREID_PIG:
		case CREID_BOAR:
			return( CREID_PIG );
		case CREID_LLAMA:
		case CREID_LLAMA_PACK:
			return( CREID_LLAMA );
		case CREID_RAT:
		case CREID_RAT_GIANT:
			return CREID_RAT;
		case CREID_WOLF_TIMBER:
		case CREID_WOLF_SILVER:
		case CREID_WOLF_DARK:
			return CREID_WOLF_SILVER;
		case CREID_ALLIGATOR_SM:
		case CREID_DRAGON_KOMODO:
			return CREID_ALLIGATOR_SM;

		case CREID_RIDGEBACK:
		case CREID_RIDGEBACK_SAVAGE:
			return CREID_RIDGEBACK;
		default:
			return( idTest );
	}
}

int CChar::NPC_GetVendorMarkup() const
{
	ADDTOCALLSTACK("CChar::NPC_GetVendorMarkup");
	// This vendor marks stuff up/down this percentage.
	// When vendor sells to players this is the markup value.
	//
	// RETURN:
	//  +100% = double price
	//  0% = default price
	//  -100% = free

	if ( IsStatFlag(STATF_Pet) )	// not on a hired vendor
		return 0;

	// Use char value
	CVarDefCont	*pVar = NULL;
	pVar = m_TagDefs.GetKey("VENDORMARKUP");
	if ( pVar )
		return static_cast<int>(pVar->GetValNum());

	// Use region value
	pVar = GetRegion()->m_TagDefs.GetKey("VENDORMARKUP");
	if ( pVar )
		return static_cast<int>(pVar->GetValNum());

	// Use chardef value
	CCharBase *pCharDef = Char_GetDef();
	if ( pCharDef )
		pVar = pCharDef->m_TagDefs.GetKey("VENDORMARKUP");
	if ( pVar )
		return static_cast<int>(pVar->GetValNum());

	// Use default value
	return 15;
}

size_t CChar::NPC_OnHearName( lpctstr pszText ) const
{
	ADDTOCALLSTACK("CChar::NPC_OnHearName");
	// Did I just hear my name in this text ?
	// should be able to deal with "hi Dennis" in the future.
	// RETURN:
	//  index to skip past the name.

	lpctstr pszName = GetName();

	size_t i = FindStrWord( pszText, pszName );
	if ( i )
		return( i );

	if ( m_pNPC )
	{
		// My title ?
		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
		{
			if ( ! strnicmp( pszText, "GUARD ", 6 ))
				return 6;
		}
		else if ( NPC_IsVendor())
		{
			if ( ! strnicmp( pszText, "VENDOR ", 7 ))
				return 7;
		}
	}

	CCharBase * pCharDef = Char_GetDef();

	// Named the chars type ? (must come first !)
	pszName = pCharDef->GetTradeName();
	for ( i = 0; pszText[i] != '\0'; i++ )
	{
		if ( pszName[i] == '\0' )
		{
			// found name.
			while ( ISWHITESPACE( pszText[i] ))
				i++;
			return( i );	// Char name found
		}
		if ( toupper( pszName[i] ) != toupper( pszText[i] ))	// not the name.
			break;
	}

	return 0;
}

bool CChar::NPC_CanSpeak() const
{
	ADDTOCALLSTACK("CChar::NPC_CanSpeak");
	//	players and chars with speech can
	if ( m_pNPC == NULL || m_pNPC->m_Speech.GetCount() > 0 )
		return true;

	CCharBase * pCharDef = Char_GetDef();
	return( pCharDef != NULL && pCharDef->m_Speech.GetCount() > 0 );
}

bool CChar::NPC_FightMayCast(bool fCheckSkill) const
{
	ADDTOCALLSTACK("CChar::NPC_FightMayCast");
	// This NPC could cast spells if they wanted to ?
	// check mana and anti-magic
	// Dont check for skill if !fCheckSkill
	if (!m_pNPC)
		return false;
	if (fCheckSkill && !const_cast<CChar*>(this)->Skill_GetMagicRandom(300))
		return false;
	if ( m_pArea && m_pArea->IsFlag(REGION_ANTIMAGIC_DAMAGE|REGION_FLAG_SAFE) )
		return false;
	if ( Stat_GetVal(STAT_INT) < 5 )
		return false;

	return true;
}

bool CChar::NPC_IsOwnedBy( const CChar * pChar, bool fAllowGM ) const
{
	ADDTOCALLSTACK("CChar::NPC_IsOwnedBy");
	// Is pChar my master ?
	// BESERK will not listen to commands tho.
	// fAllowGM = consider GM's to be owners of all NPC's

	if ( pChar == NULL )
		return false;
	if ( this == pChar )
		return true;

	if ( fAllowGM && pChar->IsPriv( PRIV_GM ))
		return( pChar->GetPrivLevel() > GetPrivLevel());

	if ( ! IsStatFlag( STATF_Pet ) || m_pPlayer )	// shortcut - i'm not a pet.
		return false;
	if ( m_pNPC == NULL )
		return false;
	if ( m_pNPC->m_Brain == NPCBRAIN_BERSERK )	// i cannot be commanded.
		return false;

	return( Memory_FindObjTypes( pChar, MEMORY_IPET ) != NULL );
}

CChar * CChar::NPC_PetGetOwner() const
{
	ADDTOCALLSTACK("CChar::NPC_PetGetOwner");
	// Assume i am a pet. Get my first (primary) owner. not just friends. used for blame .

	if ( !IsStatFlag(STATF_Pet) )
		return NULL;

	CItemMemory	*pMemory = Memory_FindTypes(MEMORY_IPET);
	if ( !pMemory )
		return NULL;

	return pMemory->m_uidLink.CharFind();
}

int CChar::NPC_GetTrainMax( const CChar * pStudent, SKILL_TYPE Skill ) const
{
	ADDTOCALLSTACK("CChar::NPC_GetTrainMax");
	// What is the max I can train to ?
	int iMax;
	int iMaxAllowed;

	CVarDefCont * pValue = GetKey("OVERRIDE.TRAINSKILLMAXPERCENT",true);
	if ( pValue )
		iMax = (int)MulDivLL( pValue->GetValNum(), Skill_GetBase(Skill), 100 );
	else
		iMax = (int)MulDivLL( g_Cfg.m_iTrainSkillPercent, Skill_GetBase(Skill), 100 );

	pValue = GetKey("OVERRIDE.TRAINSKILLMAX",true);
	if ( pValue )
		iMaxAllowed = (int)(pValue->GetValNum());
	else
		iMaxAllowed = g_Cfg.m_iTrainSkillMax;

	if ( iMax > iMaxAllowed )
		return minimum(iMaxAllowed, pStudent->Skill_GetMax(Skill));

	// Is this more that the student can take ?
	return minimum(iMax, pStudent->Skill_GetMax(Skill));
}

bool CChar::NPC_CheckWalkHere( const CPointBase & pt, const CRegionBase * pArea, word wBlockFlags ) const
{
	ADDTOCALLSTACK("CChar::NPC_CheckWalkHere");
	UNREFERENCED_PARAMETER(wBlockFlags);
	// Does the NPC want to walk here ? step on this item ?
	if ( !m_pNPC )
		return false;
	if ( !pt.IsValidXY() )
		return true;

	if ( m_pArea != NULL )
	{
		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD && !IsStatFlag(STATF_War) )	// guards will want to stay in guard range
		{
			if ( m_pArea->IsGuarded() && !pArea->IsGuarded() )
				return false;
		}

		if ( Noto_IsCriminal() )
		{
			if ( !m_pArea->IsGuarded() && pArea->IsGuarded() )
				return false;
		}
	}

	// Is there a nasty object here that will hurt us ?
	CWorldSearch AreaItems(pt);
	for (;;)
	{
		CItem * pItem = AreaItems.GetItem();
		if ( pItem == NULL )
			break;

		if ( abs(pItem->GetTopZ() - pt.m_z) > 5 )
			continue;

		switch ( pItem->GetType() )
		{
			case IT_WEB:
				return (GetDispID() == CREID_GIANT_SPIDER) ? true : false;
			case IT_FIRE:
				return Can(CAN_C_FIRE_IMMUNE);
			case IT_TRAP:
			case IT_TRAP_ACTIVE:
			case IT_MOONGATE:
			case IT_TELEPAD:
				return false;
			default:
				break;
		}
	}
	return true;
}

CItemVendable * CChar::NPC_FindVendableItem( CItemVendable * pVendItem, CItemContainer * pContBuy, CItemContainer * pContStock ) // static
{
	ADDTOCALLSTACK("CChar::NPC_FindVendableItem");
	UNREFERENCED_PARAMETER(pContStock);
	// Does the NPC want to buy this item?
	if ( !pVendItem || !pContBuy || !pVendItem->IsValidSaleItem(false) )
		return NULL;

	CItem * pItemTest = pContBuy->ContentFind(CResourceID(RES_ITEMDEF, pVendItem->GetID()));
	if ( pItemTest == NULL )
		return NULL;

	CItemVendable * pItemSell = dynamic_cast<CItemVendable *>(pItemTest);
	if ( pItemSell == NULL )	// the item is not vendable
		return NULL;
	if ( pVendItem->GetType() != pItemSell->GetType())	// sanity check
		return NULL;

	return pItemSell;
}

////////////////////////////////////////////////////////////////////
// This stuff is still questionable.


int CChar::NPC_WantThisItem( CItem * pItem ) const
{
	ADDTOCALLSTACK("CChar::NPC_WantThisItem");
	//  This should be the ULTIMATE place to check if the NPC wants this in any way.
	//  May not want to use it but rather just put it in my pack.
	//
	// NOTE:
	//  Don't check if i can see this or i can reach it.
	//  Don't check if i can carry this ?
	//
	// RETURN:
	//  0-100 percent = how bad do we want it ?

	if ( !CanMove(pItem, false) )
		return 0;

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef != NULL);
	size_t iRet = pCharDef->m_Desires.FindResourceMatch(pItem);
	if ( iRet != pCharDef->m_Desires.BadIndex() )
		return (int)(pCharDef->m_Desires[iRet].GetResQty());

	// I'm hungry and this is food ?
	int iFoodLevel = Food_GetLevelPercent();
	if ( Food_CanEat(pItem) && iFoodLevel < 100 )
		return 100 - iFoodLevel;

	if ( NPC_IsVendor() )
	{
		// Vendors always want money
		if ( pItem->IsType(IT_GOLD) )
			return 100;

		// Is it something I would buy?
		CItemVendable * pItemSell = NPC_FindVendableItem(dynamic_cast<CItemVendable *>(pItem), const_cast<CChar *>(this)->GetBank(LAYER_VENDOR_BUYS), const_cast<CChar *>(this)->GetBank(LAYER_VENDOR_STOCK));
		if ( pItemSell )
			return pItemSell->GetVendorPrice(0);
	}

	return 0;
}

int CChar::NPC_GetWeaponUseScore( CItem * pWeapon )
{
	ADDTOCALLSTACK("CChar::NPC_GetWeaponUseScore");
	// How good would i be at this weapon ?

	SKILL_TYPE skill;

	if ( !pWeapon )
		skill = SKILL_WRESTLING;
	else
	{
		// Is it a weapon ?
		skill = pWeapon->Weapon_GetSkill();
		if ( skill == SKILL_WRESTLING )
			return 0;

		// I can't equip this anyhow.
		if ( CanEquipLayer( pWeapon, LAYER_QTY, NULL, true ) == LAYER_NONE )
			return 0;
		// How much damage could i do with this ?
	}

	int iDmg = Fight_CalcDamage( pWeapon );
	int iSkillLevel = Skill_GetAdjusted( skill );

	return( iSkillLevel + iDmg * 50 );
}

int CChar::NPC_GetHostilityLevelToward( const CChar * pCharTarg ) const
{
	ADDTOCALLSTACK("CChar::NPC_GetHostilityLevelToward");
	// What is my general hostility level toward this type of creature ?
	//
	// based on:
	//  npc vs player, (evil npc's don't like players regurdless of align, xcept in town)
	//  karma (we are of different alignments)
	//  creature body type. (allie groups)
	//  hunger, (they could be food)
	//  memories of this creature.
	//
	// DO NOT consider:
	//   strength, he is far stronger or waeker than me.
	//	 health, i may be near death.
	//   location (guarded area), (xcept in the case that evil people like other evils in town)
	//   loot, etc.
	//
	// RETURN:
	//   100 = extreme hatred.
	//   0 = neutral.
	//   -100 = love them
	//


	if ( !pCharTarg || !m_pNPC )
		return 0;

	int iHostility = 0;

	// if it is a pet - register it the same as it's master.
	CChar * pCharOwn = pCharTarg->NPC_PetGetOwner();
	if ( pCharOwn != NULL && pCharOwn != this )
	{
		static int sm_iReentrant = 0;
		if (sm_iReentrant > 32)
		{
			DEBUG_ERR(("Too many owners (circular ownership?) to continue acquiring hostility level towards %s uid=0%x\n", pCharOwn->GetName(), pCharOwn->GetUID().GetPrivateUID()));
			return 0;
		}

		++sm_iReentrant;
		iHostility = NPC_GetHostilityLevelToward( pCharOwn );
		--sm_iReentrant;

		return iHostility;
	}

	int iKarma = Stat_GetAdjusted(STAT_KARMA);

	bool fDoMemBase = false;

	if ( Noto_IsEvil() &&	// i am evil.
		(m_pArea && !m_pArea->IsGuarded()) &&	// we are not in an evil town.
		pCharTarg->m_pPlayer )	// my target is a player.
	{
		// If i'm evil i give no benefit to players with bad karma.
		// I hate all players.
		// Unless i'm in a guarded area. then they are cool.
		iHostility = 51;
	}
	else if ( m_pNPC->m_Brain == NPCBRAIN_BERSERK )	// i'm beserk.
	{
		// beserks just hate everyone all the time.
		iHostility = 100;
	}
	else if ( pCharTarg->m_pNPC &&	// my target is an NPC
		pCharTarg->m_pNPC->m_Brain != NPCBRAIN_BERSERK &&	// ok to hate beserks.
		! g_Cfg.m_fMonsterFight )		// monsters are not supposed to fight other monsters !
	{
		iHostility = -50;
		fDoMemBase = true;	// set this low in case we are defending ourselves. but not attack for hunger.
	}
	else
	{
		// base hostillity on karma diff.

		int iKarmaTarg = pCharTarg->Stat_GetAdjusted(STAT_KARMA);

		if ( Noto_IsEvil())
		{
			// I'm evil.
			if ( iKarmaTarg > 0 )
			{
				iHostility += ( iKarmaTarg ) / 1024;
			}
		}
		else if ( iKarma > 300 )
		{
			// I'm good and my target is evil.
			if ( iKarmaTarg < -100 )
			{
				iHostility += ( -iKarmaTarg ) / 1024;
			}
		}
	}

	// Based on just creature type.

	if ( ! fDoMemBase )
	{
		if ( pCharTarg->m_pNPC )
		{
			// Human NPC's will attack humans .

			if ( GetDispID() == pCharTarg->GetDispID())
			{
				// I will never attack those of my own kind...even if starving
				iHostility -= 100;
			}
			else if ( NPC_GetAllyGroupType( GetDispID()) == NPC_GetAllyGroupType(pCharTarg->GetDispID()))
			{
				iHostility -= 50;
			}
			else if ( pCharTarg->m_pNPC->m_Brain == m_pNPC->m_Brain )	// My basic kind
			{
				// Won't attack other monsters. (unless very hungry)
				iHostility -= 30;
			}
		}
		else
		{
			// Not immediately hostile if looks the same as me.
			if ( ! IsPlayableCharacter() && NPC_GetAllyGroupType( GetDispID()) == NPC_GetAllyGroupType(pCharTarg->GetDispID()))
			{
				iHostility -= 51;
			}
		}
	}

	// I have been attacked/angered by this creature before ?
	CItemMemory * pMemory = Memory_FindObjTypes( pCharTarg, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_SAWCRIME|MEMORY_AGGREIVED );
	if ( pMemory )
	{
		iHostility += 50;
	}
	return( iHostility );
}

int CChar::NPC_GetAttackContinueMotivation( CChar * pChar, int iMotivation ) const
{
	ADDTOCALLSTACK("CChar::NPC_GetAttackContinueMotivation");
	// I have seen fit to attack them.
	// How much do i want to continue an existing fight ? cowardice ?
	// ARGS:
	//  iMotivation = My base motivation toward this creature.
	//
	// RETURN:
	// -101 = ? dead meat. (run away)
	//
	// 0 = I'm have no interest.
	// 50 = even match.
	// 100 = he's a push over.
	if ( !m_pNPC || !pChar )
		return 0;

	if ( !pChar->Fight_IsAttackable() )
		return( -100 );
	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
		return( 100 );
	if ( m_pNPC->m_Brain == NPCBRAIN_BERSERK )
		return( iMotivation + 80 - GetDist( pChar ));	// less interested the further away they are

	// Try to stay on one target.
	if ( Fight_IsActive() && m_Act_UID == pChar->GetUID())
		iMotivation += 8;

	// Less interested the further away they are.
	iMotivation -= GetDist( pChar );

	if ( !g_Cfg.m_fMonsterFear )
		return( iMotivation );

	// I'm just plain stronger.
	iMotivation += ( Stat_GetAdjusted(STAT_STR) - pChar->Stat_GetAdjusted(STAT_STR));

	// I'm healthy.
	int iTmp = GetHealthPercent() - pChar->GetHealthPercent();
	if ( iTmp < -50 )
		iMotivation -= 50;
	else if ( iTmp > 50 )
		iMotivation += 50;

	// I'm smart and therefore more cowardly. (if injured)
	iMotivation -= Stat_GetAdjusted(STAT_INT) / 16;

	return( iMotivation );
}

int CChar::NPC_GetAttackMotivation( CChar * pChar, int iMotivation ) const
{
	ADDTOCALLSTACK("CChar::NPC_GetAttackMotivation");
	// Some sort of monster.
	// Am I stronger than he is ? Should I continue fighting ?
	// Take into consideration AC, health, skills, etc..
	// RETURN:
	//   < 0 = dead meat. (run away)
	//   0 = I'm have no interest.
	//   50 = even match.
	//   100 = he's a push over.

	if ( !m_pNPC || !pChar || !pChar->m_pArea )
		return 0;
	if ( IsStatFlag(STATF_DEAD) || pChar->IsStatFlag(STATF_DEAD) )
		return 0;
	if ( pChar->m_pArea->IsFlag(REGION_FLAG_SAFE) )
		return 0;

	iMotivation += NPC_GetHostilityLevelToward(pChar);
	if ( iMotivation > 0 )
		iMotivation = NPC_GetAttackContinueMotivation(pChar, iMotivation);		// Am i injured etc ?

	return iMotivation;
}

