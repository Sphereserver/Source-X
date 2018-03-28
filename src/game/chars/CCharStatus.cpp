//  CChar is either an NPC or a Player.

#include "../../common/CUIDExtra.h"
#include "../../network/network.h"
#include "../clients/CClient.h"
#include "../../common/CLog.h"
#include "../spheresvr.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

bool CChar::IsResourceMatch( CResourceIDBase rid, dword dwAmount )
{
	ADDTOCALLSTACK("CChar::IsResourceMatch");
	return IsResourceMatch(rid, dwAmount, 0);
}

bool CChar::IsResourceMatch( CResourceIDBase rid, dword dwAmount, dword dwArgResearch )
{
	ADDTOCALLSTACK("CChar::IsResourceMatch (dwArgResearch)");
	// Is the char a match for this test ?
	switch ( rid.GetResType() )
	{
		case RES_SKILL:			// do I have this skill level?
			if ( Skill_GetBase(static_cast<SKILL_TYPE>(rid.GetResIndex())) < dwAmount )
				return false;
			return true;

		case RES_CHARDEF:		// I'm this type of char?
		{
			CCharBase *pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			if ( pCharDef->GetResourceID() == rid )
				return true;
			return false;
		}

		case RES_SPEECH:		// do I have this speech?
			if ( m_pNPC )
			{
				if ( m_pNPC->m_Speech.ContainsResourceID(rid) )
					return true;
				CCharBase *pCharDef = Char_GetDef();
				ASSERT(pCharDef);
				if ( pCharDef->m_Speech.ContainsResourceID(rid) )
					return true;
			}
			return false;

		case RES_EVENTS:		// do I have these events?
		{
			if ( m_OEvents.ContainsResourceID(rid) )
				return true;
			if ( m_pNPC )
			{
				CCharBase *pCharDef = Char_GetDef();
				ASSERT(pCharDef);
				if ( pCharDef->m_TEvents.ContainsResourceID(rid) )
					return true;
			}
			return false;
		}

		case RES_TYPEDEF:		// do I have these in my posession?
			if ( !ContentConsume(rid, dwAmount, true, dwArgResearch) )
				return true;
			return false;

		case RES_ITEMDEF:		// do I have these in my posession?
			if ( !ContentConsume(rid, dwAmount, true) )
				return true;
			return false;
		default:
			return false;
	}
}

bool CChar::IsSpeakAsGhost() const
{
	return( IsStatFlag(STATF_DEAD) &&
		! IsStatFlag( STATF_SPIRITSPEAK ) &&
		! IsPriv( PRIV_GM ));
}

bool CChar::CanUnderstandGhost() const
{
	// Can i understand player ghost speak ?
	if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_HEALER )
		return true;
	if ( Skill_GetBase( SKILL_SPIRITSPEAK ) >= g_Cfg.m_iMediumCanHearGhosts )
		return true;
	return( IsStatFlag( STATF_SPIRITSPEAK|STATF_DEAD ) || IsPriv( PRIV_GM|PRIV_HEARALL ));
}

bool CChar::IsPlayableCharacter() const
{
	return( IsHuman() || IsElf() || IsGargoyle() );
}

bool CChar::IsHuman() const
{
	return( CCharBase::IsHumanID( GetDispID()) );
}

bool CChar::IsElf() const
{
	return( CCharBase::IsElfID( GetDispID()) );
}

bool CChar::IsGargoyle() const
{
	return( CCharBase::IsGargoyleID( GetDispID()) );
}

CItemContainer * CChar::GetPack() const
{
	return( dynamic_cast <CItemContainer *>(LayerFind( LAYER_PACK )) );
}

CItemContainer *CChar::GetBank( LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CChar::GetBank");
	// Get our bank box or vendor box.
	// If we dont have 1 then create it.

	ITEMID_TYPE id;
	switch ( layer )
	{
		case LAYER_PACK:
			id = ITEMID_BACKPACK;
			break;

		case LAYER_VENDOR_STOCK:
		case LAYER_VENDOR_EXTRA:
		case LAYER_VENDOR_BUYS:
			if ( !NPC_IsVendor() )
				return NULL;
			id = ITEMID_VENDOR_BOX;
			break;

		default:
			id = ITEMID_BANK_BOX;
			layer = LAYER_BANKBOX;
			break;
	}
	
	CItem *pItemTest = LayerFind(layer);
	CItemContainer *pBankBox = dynamic_cast<CItemContainer *>(pItemTest);
	if ( pBankBox )
		return pBankBox;

	if ( !g_Serv.IsLoading() )
	{
		if ( pItemTest )
		{
			DEBUG_ERR(("Junk in bank box layer %d!\n", layer));
			pItemTest->Delete();
		}

		// Add new bank box if not found
		pBankBox = dynamic_cast<CItemContainer *>(CItem::CreateScript(id, this));
		ASSERT(pBankBox);
		pBankBox->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
		LayerAdd(pBankBox, layer);
	}
	return pBankBox;
}

CItemContainer * CChar::GetPackSafe()
{
	return GetBank(LAYER_PACK);
}

CItem *CChar::GetBackpackItem(ITEMID_TYPE id)
{
	ADDTOCALLSTACK("CChar::GetBackpackItem");
	CItemContainer *pPack = GetPack();
	if ( pPack )
	{
		for ( CItem *pItem = pPack->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
		{
			if ( pItem->GetID() == id )
				return pItem;
		}
	}
	return NULL;
}

CItem *CChar::LayerFind( LAYER_TYPE layer ) const
{
	ADDTOCALLSTACK("CChar::LayerFind");
	// Find an item i have equipped.

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->GetEquipLayer() == layer )
			return pItem;
	}
	return NULL;
}

TRIGRET_TYPE CChar::OnCharTrigForLayerLoop( CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CSString *pResult, LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CChar::OnCharTrigForLayerLoop");
	CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->GetEquipLayer() == layer )
		{
			TRIGRET_TYPE iRet = pItem->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);
			if ( iRet == TRIGRET_BREAK )
			{
				EndContext = StartContext;
				break;
			}
			if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
				return iRet;
			if ( iRet == TRIGRET_CONTINUE )
				EndContext = StartContext;
			else
				EndContext = s.GetContext();
			s.SeekContext(StartContext);
		}
	}
	if ( EndContext.m_stOffset <= StartContext.m_stOffset )
	{
		// just skip to the end.
		TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult);
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
		s.SeekContext(EndContext);
	return TRIGRET_ENDIF;
}

int CChar::GetWeightLoadPercent( int iWeight ) const
{
	ADDTOCALLSTACK("CChar::GetWeightLoadPercent");
	// Get a percent of load.
	if ( IsPriv(PRIV_GM) )
		return 1;

	int	MaxCarry = g_Cfg.Calc_MaxCarryWeight(this);
	if ( !MaxCarry )
		return 1000;	// suppose self extra-overloaded

	return (int)IMulDivLL( iWeight, 100, MaxCarry );
}

bool CChar::CanCarry( const CItem *pItem ) const
{
	ADDTOCALLSTACK("CChar::CanCarry");
	if ( IsPriv(PRIV_GM) )
		return true;

	int iItemWeight = pItem->GetWeight();
	if ( pItem->GetEquipLayer() == LAYER_DRAGGING )		// if we're dragging the item, its weight is already added on char so don't count it again
		iItemWeight = 0;

	if ( (GetTotalWeight() + iItemWeight) > g_Cfg.Calc_MaxCarryWeight(this) )
		return false;

	return true;
}

void CChar::ContentAdd( CItem * pItem, bool bForceNoStack )
{
	ADDTOCALLSTACK("CChar::ContentAdd");
	UNREFERENCED_PARAMETER(bForceNoStack);
	ItemEquip(pItem);
	//LayerAdd( pItem, LAYER_QTY );
}

bool CChar::CanEquipStr( CItem *pItem ) const
{
	ADDTOCALLSTACK("CChar::CanEquipStr");
	if ( IsPriv(PRIV_GM) )
		return true;

	CItemBase *pItemDef = pItem->Item_GetDef();
	LAYER_TYPE layer = pItemDef->GetEquipLayer();
	if ( !pItemDef->IsTypeEquippable() || !CItemBase::IsVisibleLayer(layer) )
		return true;

	if ( Stat_GetAdjusted(STAT_STR) >= pItemDef->m_ttEquippable.m_iStrReq * (100 - pItem->GetDefNum("LOWERREQ", true, true)) / 100 )
		return true;

	return false;
}

LAYER_TYPE CChar::CanEquipLayer( CItem *pItem, LAYER_TYPE layer, CChar *pCharMsg, bool fTest )
{
	ADDTOCALLSTACK("CChar::CanEquipLayer");
	// This takes care of any conflicting items in the slot !
	// NOTE: Do not check to see if i can pick this up or steal this etc.
	// LAYER_NONE = can't equip this .

	ASSERT(pItem);

	if ( pItem->IsType(IT_SPELL) )	// spell memory conflicts are handled by CChar::Spell_Effect_Create()
		return layer;

	const CItemBase *pItemDef = pItem->Item_GetDef();
	if ( layer >= LAYER_QTY )
	{
		layer = pItemDef->GetEquipLayer();
		if ( pItemDef->IsTypeEquippable() && CItemBase::IsVisibleLayer(layer) )
		{
			// Test only on players or if requested
			if ( (m_pPlayer || fTest) && pItemDef->m_ttEquippable.m_iStrReq && (Stat_GetAdjusted(STAT_STR) < pItemDef->m_ttEquippable.m_iStrReq * (100 - pItem->GetDefNum("LOWERREQ", true, true)) / 100) )
			{
				if ( m_pPlayer )	// message only players
				{
					SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
					if ( pCharMsg && pCharMsg != this )
						pCharMsg->SysMessagef("%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), pItem->GetName());
				}
				return LAYER_NONE;
			}
		}
	}

	if ( pItem->GetParent() == this && pItem->GetEquipLayer() == layer )		// not a visible item LAYER_TYPE
		return layer;

	CItem *pItemPrev = NULL;
	bool fCantEquip = false;

	switch ( layer )
	{
		case LAYER_PACK:
		case LAYER_AUCTION:
			if ( !pItem->IsType(IT_CONTAINER) )
				fCantEquip = true;
			break;
		case LAYER_NONE:
		case LAYER_SPECIAL:
			switch ( pItem->GetType() )
			{
				case IT_EQ_TRADE_WINDOW:
				case IT_EQ_MEMORY_OBJ:
				case IT_EQ_SCRIPT:
					return LAYER_SPECIAL;	// we can have multiple items of these
				default:
					return LAYER_NONE;		// not legal!
			}
		case LAYER_HAIR:
			if ( !pItem->IsType(IT_HAIR) )
				fCantEquip = true;
			break;
		case LAYER_BEARD:
			if ( !pItem->IsType(IT_BEARD) )
				fCantEquip = true;
			break;
		case LAYER_BANKBOX:
			if ( !pItem->IsType(IT_EQ_BANK_BOX) )
				fCantEquip = true;
			break;
		case LAYER_HORSE:
			if ( !pItem->IsType(IT_EQ_HORSE) || !IsMountCapable() )
				fCantEquip = true;
			break;
		case LAYER_HAND1:
		case LAYER_HAND2:
		{
			if ( !pItemDef->IsTypeEquippable() || !Can(CAN_C_USEHANDS) )
			{
				fCantEquip = true;
				break;
			}

			if ( layer == LAYER_HAND2 )
			{
				// If it's a 2 handed weapon, unequip the other hand
				if ( pItem->IsTypeWeapon() || pItem->IsType(IT_FISH_POLE) )
					pItemPrev = LayerFind(LAYER_HAND1);
			}
			else
			{
				// Unequip 2 handed weapons if we must use the other hand
				pItemPrev = LayerFind(LAYER_HAND2);
				if ( pItemPrev && !pItemPrev->IsTypeWeapon() && !pItemPrev->IsType(IT_FISH_POLE) )
					pItemPrev = NULL;
			}
			break;
		}

		case LAYER_FACE:
		case LAYER_COLLAR:
		case LAYER_RING:
		case LAYER_EARS:
		case LAYER_TALISMAN:
			break;

		default:
			if ( CItemBase::IsVisibleLayer(layer) && !Can(CAN_C_EQUIP) )
				fCantEquip = true;
			break;
	}

	if ( fCantEquip )	// some creatures can equip certain special items ??? (orc lord?)
	{
		if ( pCharMsg )
			pCharMsg->SysMessageDefault(DEFMSG_EQUIP_CANNOT);
		return LAYER_NONE;
	}

	// Check for objects already in this slot. Deal with it.
	if ( !pItemPrev )
		pItemPrev = LayerFind(layer);

	if ( pItemPrev )
	{
		switch ( layer )
		{
			case LAYER_AUCTION:
			case LAYER_PACK:
				return LAYER_NONE;	// this should not happen
			case LAYER_HORSE:
			case LAYER_DRAGGING:
				if ( !fTest )
					ItemBounce(pItemPrev);
				break;
			case LAYER_BEARD:
			case LAYER_HAIR:
				if ( !fTest )
					pItemPrev->Delete();
				break;
			default:
			{
				if ( !CanMove(pItemPrev) )
					return LAYER_NONE;
				if ( !fTest )
					ItemBounce(pItemPrev);
				break;
			}
		}
	}

	return layer;
}

int CChar::GetHealthPercent() const
{
	ADDTOCALLSTACK("CChar::GetHealthPercent");
	short str = Stat_GetAdjusted(STAT_STR);
	if ( !str )
		return 0;
	return IMulDiv(Stat_GetVal(STAT_STR), 100, str);
}

CChar* CChar::GetNext() const
{
	return( static_cast <CChar*>( CObjBase::GetNext()) );
}

CObjBaseTemplate * CChar::GetTopLevelObj() const
{
	// Get the object that has a location in the world. (Ground level)
	return const_cast<CChar*>(this);
}

bool CChar::IsSwimming() const
{
	ADDTOCALLSTACK("CChar::IsSwimming");
	// Am i in/over/slightly under the water now ?
	// NOTE: This is a bit more complex because we need to test if we are slightly under water.

	CPointMap ptTop = GetTopPoint();

	CPointMap pt = g_World.FindItemTypeNearby(ptTop, IT_WATER);
	if ( !pt.IsValidPoint() )
		return false;

	char iDistZ = ptTop.m_z - pt.m_z;
	if ( iDistZ < -PLAYER_HEIGHT )	// far under the water somehow
		return false;

	// Is there a solid surface under us?
	dword dwBlockFlags = GetMoveBlockFlags();
	char iSurfaceZ = g_World.GetHeightPoint2(ptTop, dwBlockFlags, true);
	if ( (iSurfaceZ == pt.m_z) && (dwBlockFlags & CAN_I_WATER) )
		return true;

	return false;
}

NPCBRAIN_TYPE CChar::GetNPCBrain(bool fDefault) const
{
	ADDTOCALLSTACK("CChar::GetNPCBrain");
	// Return NPCBRAIN_ANIMAL for animals, _HUMAN for NPC human and PCs, >= _MONSTER for monsters
	//	(can return also _BERSERK and _DRAGON)
	// For tracking and other purposes.

	if (fDefault)
	{
		if (m_pNPC)
		{
			if ((m_pNPC->m_Brain >= NPCBRAIN_HUMAN) && (m_pNPC->m_Brain <= NPCBRAIN_STABLE))
				return NPCBRAIN_HUMAN;

			return m_pNPC->m_Brain;
		}
		else if (m_pPlayer)
			return NPCBRAIN_HUMAN;
	}
	

	// Handle the exceptions (or voluntarily auto-detect the brain, if fDefault == false)
	CREID_TYPE id = GetDispID();
	if ( id >= CREID_IRON_GOLEM )
	{
		switch ( id )
		{
			//TODO: add other dragons
			case CREID_DRAGON_SERPENTINE:
			case CREID_DRAGON_SKELETAL:
			case CREID_REPTILE_LORD:
			case CREID_WYRM_ANCIENT:
			case CREID_SWAMP_DRAGON:
			case CREID_SWAMP_DRAGON_AR:
				return NPCBRAIN_DRAGON;
			default:
				break;
		}
		return NPCBRAIN_MONSTER;
	}

	if ( (id == CREID_ENERGY_VORTEX) || (id == CREID_BLADE_SPIRIT) )
		return NPCBRAIN_BERSERK;

	if ( id >= CREID_MAN )
		return NPCBRAIN_HUMAN;

	if ( id >= CREID_HORSE_TAN )
		return NPCBRAIN_ANIMAL;

	switch ( id )
	{
		case CREID_EAGLE:
		case CREID_BIRD:
		case CREID_GORILLA:
		case CREID_SNAKE:
		case CREID_BULL_FROG:
		case CREID_DOLPHIN:
			return NPCBRAIN_ANIMAL;
		default:
			return NPCBRAIN_MONSTER;
	}
}

lpctstr CChar::GetPronoun() const
{
	ADDTOCALLSTACK("CChar::GetPronoun");
	switch ( GetDispID() )
	{
		case CREID_MAN:
		case CREID_GHOSTMAN:
		case CREID_ELFMAN:
		case CREID_ELFGHOSTMAN:
		case CREID_GARGMAN:
		case CREID_GARGGHOSTMAN:
			return g_Cfg.GetDefaultMsg(DEFMSG_PRONOUN_HE);
		case CREID_WOMAN:
		case CREID_GHOSTWOMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTWOMAN:
		case CREID_GARGWOMAN:
		case CREID_GARGGHOSTWOMAN:
			return g_Cfg.GetDefaultMsg(DEFMSG_PRONOUN_SHE);
		default:
			return g_Cfg.GetDefaultMsg(DEFMSG_PRONOUN_IT);
	}
}

lpctstr CChar::GetPossessPronoun() const
{
	ADDTOCALLSTACK("CChar::GetPossessPronoun");
	switch ( GetDispID() )
	{
		case CREID_MAN:
		case CREID_GHOSTMAN:
		case CREID_ELFMAN:
		case CREID_ELFGHOSTMAN:
		case CREID_GARGMAN:
		case CREID_GARGGHOSTMAN:
			return g_Cfg.GetDefaultMsg(DEFMSG_POSSESSPRONOUN_HIS);
		case CREID_WOMAN:
		case CREID_GHOSTWOMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTWOMAN:
		case CREID_GARGWOMAN:
		case CREID_GARGGHOSTWOMAN:
			return g_Cfg.GetDefaultMsg(DEFMSG_POSSESSPRONOUN_HER);
		default:
			return g_Cfg.GetDefaultMsg(DEFMSG_POSSESSPRONOUN_ITS);
	}
}

byte CChar::GetModeFlag( const CClient *pViewer ) const
{
	ADDTOCALLSTACK("CChar::GetModeFlag");
	CCharBase *pCharDef = Char_GetDef();
	byte mode = 0;

	if ( IsStatFlag(STATF_FREEZE|STATF_STONE) )
		mode |= CHARMODE_FREEZE;
	if ( pCharDef->IsFemale() )
		mode |= CHARMODE_FEMALE;

	if ( pViewer && (pViewer->GetNetState()->isClientVersion(MINCLIVER_SA) || pViewer->GetNetState()->isClientEnhanced()) )
	{
		if ( IsStatFlag(STATF_HOVERING) )
			mode |= CHARMODE_FLYING;
	}
	else
	{
		if ( IsStatFlag(STATF_POISONED) )
			mode |= CHARMODE_POISON;
	}

	if ( IsStatFlag(STATF_INVUL) )
		mode |= CHARMODE_YELLOW;
	if ( GetPrivLevel() > PLEVEL_Player )
		mode |= CHARMODE_IGNOREMOBS;
	if ( IsStatFlag(STATF_WAR) )
		mode |= CHARMODE_WAR;

	uint64 iFlags = STATF_SLEEPING;
	if ( !g_Cfg.m_iColorInvis )	//This is needed for Serv.ColorInvis to work, proper flags must be set
        iFlags |= STATF_INSUBSTANTIAL;
	if ( !g_Cfg.m_iColorHidden )	//serv.ColorHidden
        iFlags |= STATF_HIDDEN;
	if ( !g_Cfg.m_iColorInvisSpell )	//serv.ColorInvisSpell
        iFlags |= STATF_INVISIBLE;
	if ( IsStatFlag(iFlags) )	// Checking if I have any of these settings enabled on the ini and I have any of them, if so ... CHARMODE_INVIS is set and color applied.
		mode |= CHARMODE_INVIS;

	return mode;
}

byte CChar::GetDirFlag(bool fSquelchForwardStep) const
{
	// future: strongly typed enums will remove the need for this cast
	byte dir = (byte)(m_dirFace);
	ASSERT( dir<DIR_QTY );

	if ( fSquelchForwardStep )
	{
		// not so sure this is an intended 'feature' but it seems to work (5.0.2d)
		switch ( dir )
		{
			case DIR_S:
				return 0x2a;	// 0x32; 0x5a; 0x5c; all also work
			case DIR_SW:
				return 0x1d;	// 0x29; 0x5d; 0x65; all also work
			case DIR_W:
				return 0x60;
			default:
				return ( dir | 0x08 );
		}
	}

	if ( IsStatFlag( STATF_FLY ))
		dir |= 0x80; // running/flying ?

	return( dir );
}

dword CChar::GetMoveBlockFlags(bool bIgnoreGM) const
{
	// What things block us ?
	if ( IsPriv(PRIV_GM|PRIV_ALLMOVE) && !bIgnoreGM )	// nothing blocks us.
		return 0xFFFFFFFF;

	dword dwCan = GetCanFlags();
	if ( Can(CAN_C_GHOST) )
		dwCan |= CAN_C_GHOST;

	if ( IsStatFlag(STATF_HOVERING) )
		dwCan |= CAN_C_HOVER;

	// Inversion of MT_INDOORS, so MT_INDOORS should be named MT_NOINDOORS now.
	if (dwCan & CAN_C_INDOORS)
		dwCan &= ~CAN_C_INDOORS;
	else
		dwCan |= CAN_C_INDOORS;

	return ( dwCan & CAN_C_MOVEMASK );
}

byte CChar::GetLightLevel() const
{
	ADDTOCALLSTACK("CChar::GetLightLevel");
	// Get personal light level.

	if ( IsStatFlag(STATF_DEAD|STATF_SLEEPING|STATF_NIGHTSIGHT) || IsPriv(PRIV_DEBUG) )
		return LIGHT_BRIGHT;
	if ( (g_Cfg.m_iRacialFlags & RACIALF_ELF_NIGHTSIGHT) && IsElf() )		// elves always have nightsight enabled (Night Sight racial trait)
		return LIGHT_BRIGHT;
	return GetTopSector()->GetLight();
}

CItem *CChar::GetSpellbook(SPELL_TYPE iSpell) const	// Retrieves a spellbook from the magic school given in iSpell
{
	ADDTOCALLSTACK("CChar::GetSpellbook");
	// Search for suitable book in hands first
	CItem *pReturn = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( !pItem->IsTypeSpellbook() )
			continue;
		CItemBase *pItemDef = pItem->Item_GetDef();
		SPELL_TYPE min = (SPELL_TYPE)pItemDef->m_ttSpellbook.m_iOffset;
		SPELL_TYPE max = (SPELL_TYPE)(pItemDef->m_ttSpellbook.m_iOffset + pItemDef->m_ttSpellbook.m_iMaxSpells);
		if ( (iSpell > min) && (iSpell < max) )
		{
			if ( pItem->IsSpellInBook(iSpell) )	//We found a book with this same spell, nothing more to do.
				return pItem;
			else
				pReturn = pItem;	// We did not find the spell, but this book is of the same school ... we'll return this book if none better is found (NOTE: some book must be returned or the code will think that we don't have a book).
		}
	}

	// Then search in the top level of the pack
	CItemContainer *pPack = GetPack();
	if ( pPack )
	{
		for ( CItem *pItem = pPack->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
		{
			if ( !pItem->IsTypeSpellbook() )
				continue;
			CItemBase *pItemDef = pItem->Item_GetDef();
			SPELL_TYPE min = (SPELL_TYPE)pItemDef->m_ttSpellbook.m_iOffset;
			SPELL_TYPE max = (SPELL_TYPE)(pItemDef->m_ttSpellbook.m_iOffset + pItemDef->m_ttSpellbook.m_iMaxSpells);
			if ( (iSpell > min) && (iSpell < max) )
			{
				if ( pItem->IsSpellInBook(iSpell) )	//We found a book with this same spell, nothing more to do.
					return pItem;
				else
					pReturn = pItem;	// We did not find the spell, but this book is of the same school ... we'll return this book if none better is found (NOTE: some book must be returned or the code will think that we don't have a book).
			}
		}
	}
	return pReturn;
}

short CChar::Food_GetLevelPercent() const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelPercent");
	short max	= Stat_GetMax(STAT_FOOD);
	if ( max == 0 )
		return 100;
	return (short)(IMulDiv(Stat_GetVal(STAT_FOOD), 100, max));
}

lpctstr CChar::Food_GetLevelMessage(bool fPet, bool fHappy) const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelMessage");
	short max = Stat_GetMax(STAT_FOOD);
	if ( max == 0 )
		return g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_UNAFFECTED);

	uint index = (uint)IMulDiv(Stat_GetVal(STAT_FOOD), 8, max);

	if ( fPet )
	{
		static lpctstr const sm_szPetHunger[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_1),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_2),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_3),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_4),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_5),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_6),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_7),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_HAPPY_8)
		};
		static lpctstr const sm_szPetHappy[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_1),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_2),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_3),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_4),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_5),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_6),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_7),
			g_Cfg.GetDefaultMsg(DEFMSG_MSG_PET_FOOD_8)
		};

		if ( index >= (CountOf(sm_szPetHunger) - 1) )
			index = CountOf(sm_szPetHunger) - 1;

		return fHappy ? sm_szPetHappy[index] : sm_szPetHunger[index];
	}

	static lpctstr const sm_szFoodLevel[] =
	{
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_1),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_2),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_3),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_4),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_5),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_6),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_7),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_8)
	};

	if ( index >= (CountOf(sm_szFoodLevel) - 1) )
		index = CountOf(sm_szFoodLevel) - 1;

	return sm_szFoodLevel[index];
}

short CChar::Food_CanEat( CObjBase *pObj ) const
{
	ADDTOCALLSTACK("CChar::Food_CanEat");
	// Would i want to eat this creature ? hehe
	// would i want to eat some of this item ?
	// 0 = not at all.
	// 10 = only if starving.
	// 20 = needs to be prepared.
	// 50 = not bad.
	// 75 = yummy.
	// 100 = my favorite (i will drop other things to get this).

	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	size_t iRet = pCharDef->m_FoodType.FindResourceMatch(pObj);
	if ( iRet != pCharDef->m_FoodType.BadIndex() )
		return (short)(pCharDef->m_FoodType[iRet].GetResQty());	// how bad do i want it?

	return 0;
}

CChar * CChar::GetOwner() const
{
	ADDTOCALLSTACK("CChar::GetOwner");

	if (m_pPlayer)
		return NULL;
	if (m_pNPC)
		return NPC_PetGetOwner();
	return NULL;
}

bool CChar::IsOwnedBy( const CChar * pChar, bool fAllowGM ) const
{
	ADDTOCALLSTACK("CChar::IsOwnedBy");
	// Is pChar my master ?
	// BESERK will not listen to commands tho.
	// fAllowGM = consider GM's to be owners of all NPC's

	if ( !pChar )
		return false;
	if ( pChar == this )
		return true;

	if (m_pPlayer)
		return false;
	if (m_pNPC)
		return NPC_IsOwnedBy(pChar, fAllowGM);

	return false;
}

lpctstr CChar::GetTradeTitle() const // Paperdoll title for character p (2)
{
	ADDTOCALLSTACK("CChar::GetTradeTitle");
	if ( !m_sTitle.IsEmpty() )
		return m_sTitle;

	tchar *pTemp = Str_GetTemp();
	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	// Incognito ?
	// If polymorphed then use the poly name.
	if ( IsStatFlag(STATF_INCOGNITO) || !IsPlayableCharacter() || (m_pNPC && pCharDef->GetTypeName() != pCharDef->GetTradeName()) )
	{
		if ( !IsIndividualName() )
			return "";	// same as type anyhow.
		sprintf(pTemp, "%s %s", pCharDef->IsFemale() ? g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_FEMALE) : g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_MALE), pCharDef->GetTradeName());
		return pTemp;
	}

	// Only players can have skill titles
	if ( !m_pPlayer )
		return pTemp;

	int len;
	SKILL_TYPE skill = Skill_GetBest();
	if ( skill == SKILL_NINJITSU )
	{
		static const CValStr sm_SkillTitles[] =
		{
			{ "", INT32_MIN },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE),	(int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NEOPHYTE", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NOVICE", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_APPRENTICE", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_JOURNEYMAN", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_EXPERT", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ADEPT", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_MASTER", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_GRANDMASTER", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ELDER_NINJITSU), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ELDER", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_LEGENDARY_NINJITSU), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_LEGENDARY", true)) },
			{ NULL, INT32_MAX }
		};
		len = sprintf(pTemp, "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}
	else if ( skill == SKILL_BUSHIDO )
	{
		static const CValStr sm_SkillTitles[] =
		{
			{ "", INT32_MIN },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE),	(int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NEOPHYTE", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NOVICE", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_APPRENTICE", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_JOURNEYMAN", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_EXPERT", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ADEPT", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_MASTER", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_GRANDMASTER", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ELDER_BUSHIDO), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ELDER", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_LEGENDARY_BUSHIDO), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_LEGENDARY", true)) },
			{ NULL, INT32_MAX }
		};
		len = sprintf(pTemp, "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}
	else
	{
		static const CValStr sm_SkillTitles[] =
		{
			{ "", INT32_MIN },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE),	(int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NEOPHYTE", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NOVICE", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_APPRENTICE", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_JOURNEYMAN", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_EXPERT", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ADEPT", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_MASTER", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER),(int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_GRANDMASTER", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ELDER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ELDER", true)) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_LEGENDARY), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_LEGENDARY", true)) },
			{ NULL, INT32_MAX }
		};
		len = sprintf(pTemp, "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}

	sprintf(pTemp + len, g_Cfg.GetSkillDef(skill)->m_sTitle);
	return pTemp;
}

bool CChar::CanDisturb( const CChar *pChar ) const
{
	ADDTOCALLSTACK_INTENSIVE("CChar::CanDisturb");
	// I can see/disturb only players with priv same/less than me.
	if ( !pChar )
		return false;
	if ( GetPrivLevel() < pChar->GetPrivLevel() )
		return !pChar->IsStatFlag(STATF_INSUBSTANTIAL | STATF_INVUL);
	return true;
}

bool CChar::CanSeeAsDead( const CChar *pChar) const
{
	ADDTOCALLSTACK("CChar::CanSeeAsDead");
	int iDeadCannotSee = g_Cfg.m_fDeadCannotSeeLiving;
	if ( iDeadCannotSee && !pChar->IsStatFlag(STATF_DEAD) && !IsPriv(PRIV_GM) )
	{
		if ( pChar->m_pPlayer )
		{
			if ( iDeadCannotSee == 2 )
				return false;
		}
		else
		{
			if ( (pChar->NPC_PetGetOwner() != this) && (pChar->m_pNPC->m_Brain != NPCBRAIN_HEALER) )
				return false;
		}
	}
	return true;
}

bool CChar::CanSeeInContainer( const CItemContainer *pContItem ) const
{
	ADDTOCALLSTACK("CChar::CanSeeInContainer");
	// This is a container of some sort. Can i see inside it ?

	if ( !pContItem || IsPriv(PRIV_GM) )
		return true;

	if ( pContItem->IsSearchable() )	// not a bank box or locked box
		return true;

	// Not normally searchable.
	// Make some special cases for searchable.

	CChar *pChar = static_cast<CChar*>(pContItem->GetTopLevelObj());
	if ( !pChar )
		return false;

	if ( pContItem->IsType(IT_EQ_TRADE_WINDOW) )
	{
		if ( pChar == this )
			return true;

		CItem *pItemTrade = pContItem->m_uidLink.ItemFind();
		if ( pItemTrade )
		{
			CChar *pCharTrade = static_cast<CChar*>(pItemTrade->GetTopLevelObj());
			if ( pCharTrade == this )
				return true;
		}
		return false;
	}

	if ( !pChar->IsOwnedBy(this) )		// pets and player vendors
		return false;

	if ( pContItem->IsType(IT_EQ_VENDOR_BOX) || pContItem->IsType(IT_EQ_BANK_BOX) )
	{
		// must be in the same position i opened it legitimately.
		// see addBankOpen
		if ( pContItem->m_itEqBankBox.m_pntOpen != GetTopPoint() )
			return false;
	}
	return true;
}

bool CChar::CanSee( const CObjBaseTemplate *pObj ) const
{
	ADDTOCALLSTACK("CChar::CanSee");
	// Can I see this object (char or item)?
	if ( !pObj || IsDisconnected() || !pObj->GetTopLevelObj()->GetTopPoint().IsValidPoint() )
		return false;

	if ( pObj->IsItem() )
	{
		const CItem *pItem = static_cast<const CItem*>(pObj);
		if ( !pItem || !CanSeeItem(pItem) )
			return false;

		int iDist = pItem->IsTypeMulti() ? UO_MAP_VIEW_RADAR : GetVisualRange();
		if ( GetTopPoint().GetDistSight(pObj->GetTopLevelObj()->GetTopPoint()) > iDist )
			return false;

		CObjBase *pObjCont = pItem->GetContainer();
		if ( pObjCont )
		{
//			LAYER_TYPE layerCont = pObjCont->GetEquipLayer();
//			if (layerCont < 26 && layerCont > 28)	// you can always see what a vendor can buy or sell
//			{
				if (!CanSeeInContainer(dynamic_cast<const CItemContainer*>(pObjCont)))
					return false;

				if (IsSetEF(EF_FixCanSeeInClosedConts))
				{
					// A client cannot see the contents of someone else's container, unless they have opened it first
					if (IsClient() && pObjCont->IsItem() && pObjCont->GetTopLevelObj() != this)
					{
						CClient *pClient = GetClient();
						if (pClient && (pClient->m_openedContainers.find(pObjCont->GetUID().GetPrivateUID()) == pClient->m_openedContainers.end()))
						{
						/*
						#ifdef _DEBUG
							if ( CanSee(pObjCont) )
							{
							#ifdef THREAD_TRACK_CALLSTACK
								StackDebugInformation::printStackTrace();
							#endif
								g_Log.EventDebug("%x:EF_FixCanSeeInClosedConts prevents %s, (0%x, '%s') from seeing item uid=0%x (%s, '%s') in container uid=0%x (%s, '%s')\n",
									pClient->GetSocketID(), pClient->GetAccount()->GetName(), (dword)GetUID(), GetName(false),
									(dword)pItem->GetUID(), pItem->GetResourceName(), pItem->GetName(),
									(dword)pObjCont->GetUID(), pObjCont->GetResourceName(), pObjCont->GetName());
							}
						#endif
						*/
							return false;
						}
					}
				}

//			}
			return CanSee(pObjCont);
		}
	}
	else
	{
		const CChar *pChar = static_cast<const CChar*>(pObj);
		if ( !pChar )
			return false;
		if ( pChar == this )
			return true;
		if ( GetTopPoint().GetDistSight(pChar->GetTopPoint()) > GetVisualRange() )
			return false;
		if ( IsPriv(PRIV_ALLSHOW) )
			return (GetPrivLevel() < pChar->GetPrivLevel()) ? false : true;

		if ( m_pNPC && pChar->IsStatFlag(STATF_DEAD) )
		{
			if ( m_pNPC->m_Brain != NPCBRAIN_HEALER )
				return false;
		}
		else if ( pChar->IsStatFlag(STATF_INVISIBLE|STATF_INSUBSTANTIAL|STATF_HIDDEN) )
		{
			// Characters can be invisible, but not to GM's (true sight ?)
			// equal level can see each other if they are staff members or they return 1 in @SeeHidden
			if ( pChar->GetPrivLevel() <= PLEVEL_Player )
			{
				if ( IsTrigUsed( TRIGGER_SEEHIDDEN ) )
				{
					CScriptTriggerArgs Args;
					Args.m_iN1 = GetPrivLevel() <= pChar->GetPrivLevel();
					CChar *pChar2 = const_cast< CChar* >( pChar );
					CChar *this2 = const_cast< CChar* >( this );
					this2->OnTrigger( CTRIG_SeeHidden, pChar2, &Args );
					return ( Args.m_iN1 != 1 );
				}
			}
			if ( GetPrivLevel() <= pChar->GetPrivLevel() )
				return false;
		}

		if ( IsStatFlag(STATF_DEAD) && !CanSeeAsDead(pChar) )
			return false;

		if ( pChar->IsDisconnected() )
		{
			if ( pChar->IsStatFlag(STATF_RIDDEN) )
			{
				CChar *pCharRider = Horse_GetMountChar();
				if ( pCharRider )
					return CanSee(pCharRider);
				return false;
			}

			if ( !IsPriv(PRIV_GM) )		// only characters with GM or ALLSHOW priv should be able to see disconnected chars
				return false;
		}
	}

	if ( IsPriv(PRIV_ALLSHOW) && (pObj->IsTopLevel() || pObj->IsDisconnected()) )		// don't exclude for logged out and diff maps
		return (GetTopPoint().GetDistSightBase(pObj->GetTopPoint()) <= GetVisualRange());

	return true;
}

bool CChar::CanSeeItem( const CItem * pItem ) const
{
	ADDTOCALLSTACK("CChar::CanSeeItem");
	ASSERT(pItem);
	if (pItem->IsAttr(ATTR_INVIS))
	{
		if (IsPriv(PRIV_GM))
			return true;
		tchar *uidCheck = Str_GetTemp();
		sprintf(uidCheck, "SeenBy_0%x", (dword)(GetUID()));

		if (!pItem->m_TagDefs.GetKeyNum(uidCheck, false))
			return false;
	}
	return true;
}

bool CChar::CanTouch( const CPointMap &pt ) const
{
	ADDTOCALLSTACK("CChar::CanTouch");
	// Can I reach this from where i am.
	// swords, spears, arms length = x units.
	// Use or Grab.
	// Check for blocking objects.
	// It this in a container we can't get to ?

	return CanSeeLOS(pt, NULL, 6);
}

bool CChar::CanTouch( const CObjBase *pObj ) const
{
	ADDTOCALLSTACK("CChar::CanTouch");
	// Can I reach this from where i am. swords, spears, arms length = x units.
	// Use or Grab. May be in snooped container.
	// Check for blocking objects. Is this in a container we can't get to ?

	if ( !pObj )
		return false;

	const CItem *pItem = NULL;
	const CObjBaseTemplate *pObjTop = pObj->GetTopLevelObj();
	int iDist = GetTopDist3D(pObjTop);

	if ( pObj->IsItem() )	// some objects can be used anytime. (even by the dead.)
	{
		pItem = static_cast<const CItem *>(pObj);
		if ( !pItem )
			return false;

		bool bDeathImmune = IsPriv(PRIV_GM);
		switch ( pItem->GetType() )
		{
		case IT_SIGN_GUMP:	// can be seen from a distance.
			return (iDist <= pObjTop->GetVisualRange());

		case IT_SHRINE:		// We can use shrines when dead !!
		case IT_TELESCOPE:
			bDeathImmune = true;
			break;

		case IT_SHIP_PLANK:
		case IT_SHIP_SIDE:
		case IT_SHIP_SIDE_LOCKED:
		case IT_ROPE:
			if ( IsStatFlag(STATF_SLEEPING|STATF_FREEZE|STATF_STONE) )
				break;
			return (iDist <= g_Cfg.m_iMaxShipPlankTeleport);

		default:
			break;
		}

		if ( !bDeathImmune && IsStatFlag(STATF_DEAD|STATF_SLEEPING|STATF_FREEZE|STATF_STONE) )
			return false;
	}

	//	search up to top level object
	const CChar *pChar = NULL;
	if ( pObjTop && (pObjTop != this) )
	{
		if ( pObjTop->IsChar() )
		{
			pChar = dynamic_cast<const CChar*>(pObjTop);
			if ( !pChar )
				return false;
			if ( pChar == this )
				return true;
			if ( IsPriv(PRIV_GM) )
				return (GetPrivLevel() >= pChar->GetPrivLevel());
			if ( pChar->IsStatFlag(STATF_DEAD|STATF_STONE) )
				return false;
		}

		CObjBase *pObjCont = NULL;
		const CObjBase *pObjTest = pObj;
		for (;;)
		{
			pItem = static_cast<const CItem *>(pObjTest);
			if ( !pItem )
				break;

			// What is this inside of ?
			pObjCont = pItem->GetContainer();
			if ( !pObjCont )
				break;

			pObjTest = pObjCont;
			if ( !CanSeeInContainer(static_cast<const CItemContainer *>(pObjTest)) )
				return false;
		}
	}

	if ( IsPriv(PRIV_GM) )
		return true;

	if ( !CanSeeLOS(pObjTop) )
	{
		if ( GetAbilityFlags() & CAN_C_DCIGNORELOS )
			return true;
		else if ( pObj->IsChar() && (pChar != NULL) && (pChar->GetAbilityFlags() & CAN_C_DCIGNORELOS) )
			return true;
		else if ( pObj->IsItem() && (pItem != NULL) && (pItem->GetAbilityFlags() & CAN_I_DCIGNORELOS) )
			return true;
		else
			return false;
	}
	if ( iDist > 3 )
	{
		if ( GetAbilityFlags() & CAN_C_DCIGNOREDIST )
			return true;
		else if ( pObj->IsChar() && (pChar != NULL) && (pChar->GetAbilityFlags() & CAN_C_DCIGNOREDIST) )
			return true;
		else if ( pObj->IsItem() && (pItem != NULL) && (pItem->GetAbilityFlags() & CAN_I_DCIGNOREDIST) )
			return true;
		else
			return false;
	}
	return true;
}

IT_TYPE CChar::CanTouchStatic( CPointMap &pt, ITEMID_TYPE id, CItem *pItem )
{
	ADDTOCALLSTACK("CChar::CanTouchStatic");
	// Might be a dynamic or a static.
	// RETURN:
	//  IT_JUNK = too far away.
	//  set pt to the top level point.

	if ( pItem )
	{
		if ( !CanTouch(pItem) )
			return IT_JUNK;
		pt = GetTopLevelObj()->GetTopPoint();
		return pItem->GetType();
	}

	// It's a static !
	CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( !pItemDef )
		return IT_NORMAL;
	if ( !CanTouch(pt) )
		return IT_JUNK;

	// Is this static really here ?
	const CServerMapBlock *pMapBlock = g_World.GetMapBlock(pt);
	if ( !pMapBlock )
		return IT_JUNK;

	int x2 = pMapBlock->GetOffsetX(pt.m_x);
	int y2 = pMapBlock->GetOffsetY(pt.m_y);

	size_t iQty = pMapBlock->m_Statics.GetStaticQty();
	for ( size_t i = 0; i < iQty; ++i )
	{
		if ( !pMapBlock->m_Statics.IsStaticPoint(i, x2, y2) )
			continue;
		const CUOStaticItemRec *pStatic = pMapBlock->m_Statics.GetStatic(i);
		if ( id == pStatic->GetDispID() )
			return pItemDef->GetType();
	}

	return IT_NORMAL;
}

bool CChar::CanHear( const CObjBaseTemplate *pSrc, TALKMODE_TYPE mode ) const
{
	ADDTOCALLSTACK("CChar::CanHear");
	// can we hear text or sound. (not necessarily understand it (ghost))
	// Can't hear TALKMODE_SAY through house walls.
	// NOTE: Assume pClient->CanHear() has already been called. (if it matters)

	if ( !pSrc )	// must be broadcast I guess
		return true;

	bool fThrough = false;
	int iHearRange = 0;
	switch ( mode )
	{
		case TALKMODE_YELL:
			if ( g_Cfg.m_iDistanceYell < 0 )
				return false;
			else if ( g_Cfg.m_iDistanceYell == 0 )
			{
				int dist = GetVisualRange();
				if ( dist == 0 )
					return true;
				iHearRange = dist;
				break;
			}
			iHearRange = g_Cfg.m_iDistanceYell;
			fThrough = true;
			break;
		case TALKMODE_BROADCAST:
			return true;
		case TALKMODE_WHISPER:
			if ( g_Cfg.m_iDistanceWhisper < 0 )
				return false;
			else if ( g_Cfg.m_iDistanceWhisper == 0 )
			{
				int dist = GetVisualRange();
				if ( dist == 0 )
					return true;
				iHearRange = dist;
				break;
			}
			iHearRange = g_Cfg.m_iDistanceWhisper;
			break;
		default:	// this is executed also when playing a sound (TALKMODE_OBJ)
			if ( g_Cfg.m_iDistanceTalk < 0 )
				return false;
			else if ( g_Cfg.m_iDistanceTalk == 0 )
			{
				int dist = GetVisualRange();
				if ( dist == 0 )
					return true;
				iHearRange = dist;
				break;
			}
			iHearRange = g_Cfg.m_iDistanceTalk;
			break;
	}

	pSrc = pSrc->GetTopLevelObj();
	int iDist = GetTopDist3D(pSrc);
	if ( iDist > iHearRange )	// too far away
		return false;
	if ( IsSetOF(OF_NoHouseMuteSpeech) )
		return true;
	if ( IsPriv(PRIV_GM) )
		return true;
	if ( fThrough )		// a yell goes through walls..
		return true;

	// sound can be blocked if in house.
	CRegionWorld *pSrcRegion;
	if ( pSrc->IsChar() )
	{
		const CChar *pCharSrc = dynamic_cast<const CChar*>(pSrc);
		ASSERT(pCharSrc);
		pSrcRegion = pCharSrc->GetRegion();
		if ( pCharSrc->IsPriv(PRIV_GM) )
			return true;
	}
	else
		pSrcRegion = dynamic_cast<CRegionWorld *>(pSrc->GetTopPoint().GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));

	if ( m_pArea == pSrcRegion )	// same region is always ok.
		return true;
	if ( !pSrcRegion || !m_pArea ) // should not happen really.
		return false;

	bool fCanSpeech = false;
	CVarDefCont *pValue = pSrcRegion->GetResourceID().IsItem() ? pSrcRegion->GetResourceID().ItemFind()->GetKey("NOMUTESPEECH", false) : NULL;
	if ( pValue && pValue->GetValNum() > 0 )
		fCanSpeech = true;
	if ( pSrcRegion->GetResourceID().IsItem() && !pSrcRegion->IsFlag(REGION_FLAG_SHIP) && !fCanSpeech )
		return false;

	pValue = m_pArea->GetResourceID().IsItem() ? m_pArea->GetResourceID().ItemFind()->GetKey("NOMUTESPEECH", false) : NULL;
	if ( pValue && pValue->GetValNum() > 0 )
		fCanSpeech = true;
	if ( m_pArea->GetResourceID().IsItem() && !m_pArea->IsFlag(REGION_FLAG_SHIP) && !fCanSpeech )
		return false;

	return true;
}

bool CChar::CanMove( CItem *pItem, bool fMsg ) const
{
	ADDTOCALLSTACK("CChar::CanMove");
	// Is it possible that i could move this ?
	// NOT: test if need to steal. IsTakeCrime()
	// NOT: test if close enough. CanTouch()

	if ( IsPriv(PRIV_ALLMOVE|PRIV_DEBUG|PRIV_GM) )
		return true;
	if ( !pItem )
		return false;
	if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_LOCKEDDOWN) && !pItem->IsAttr(ATTR_MOVE_ALWAYS) )
		return false;

	if ( IsStatFlag(STATF_STONE|STATF_FREEZE|STATF_INSUBSTANTIAL|STATF_DEAD|STATF_SLEEPING) )
	{
		if ( fMsg )
			SysMessageDefault(DEFMSG_CANTMOVE_DEAD);
		return false;
	}

	if ( pItem->IsTopLevel() )
	{
		if ( pItem->IsTopLevelMultiLocked() )
		{
			if ( fMsg )
				SysMessageDefault(DEFMSG_CANTMOVE_MULTI);

			return false;
		}
	}
	else	// equipped or in a container.
	{
		// Can't move items from the trade window (client limitation)
		if ( pItem->GetContainer()->IsContainer() )
		{
			CItemContainer *pItemCont = dynamic_cast<CItemContainer *> (pItem->GetContainer());
			if ( pItemCont && pItemCont->IsItemInTrade() )
			{
				SysMessageDefault(DEFMSG_MSG_TRADE_CANTMOVE);
				return false;
			}
		}

		// Can't move equipped cursed items
		if ( pItem->IsAttr(ATTR_CURSED|ATTR_CURSED2) && pItem->IsItemEquipped() )
		{
			pItem->SetAttr(ATTR_IDENTIFIED);
			if ( fMsg )
				SysMessagef("%s %s", static_cast<lpctstr>(pItem->GetName()), g_Cfg.GetDefaultMsg(DEFMSG_CANTMOVE_CURSED));

			return false;
		}

		// Can't steal/move newbie items on another cchar. (even if pet)
		if ( pItem->IsAttr(ATTR_NEWBIE|ATTR_BLESSED2|ATTR_CURSED|ATTR_CURSED2) )
		{
			const CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();
			if ( pObjTop->IsItem() )	// is this a corpse or sleeping person ?
			{
				const CItemCorpse *pCorpse = dynamic_cast<const CItemCorpse *>(pObjTop);
				if ( pCorpse )
				{
					CChar *pChar = pCorpse->IsCorpseSleeping();
					if ( pChar && pChar != this )
						return false;
				}
			}
			else if ( pObjTop->IsChar() && (pObjTop != this) )
			{
				if (( pItem->IsAttr(ATTR_NEWBIE) ) && g_Cfg.m_bAllowNewbTransfer )
				{
					CChar *pPet = dynamic_cast<CChar*>( pItem->GetTopLevelObj() );
					if (pPet && (pPet->GetOwner() == this) )
						return true;
				}
				if ( !pItem->IsItemEquipped() || (pItem->GetEquipLayer() != LAYER_DRAGGING) )
					return false;
			}
		}

		if ( pItem->IsItemEquipped() )
		{
			LAYER_TYPE layer = pItem->GetEquipLayer();
			switch ( layer )
			{
				case LAYER_DRAGGING:
					return true;
				case LAYER_HAIR:
				case LAYER_BEARD:
				case LAYER_PACK:
					if ( !IsPriv(PRIV_GM) )
						return false;
					break;
				default:
					if ( !CItemBase::IsVisibleLayer(layer) && ! IsPriv(PRIV_GM) )
						return false;
			}
		}
	}

	return pItem->IsMovable();
}

bool CChar::IsTakeCrime( const CItem *pItem, CChar ** ppCharMark ) const
{
	ADDTOCALLSTACK("CChar::IsTakeCrime");

	if ( IsPriv(PRIV_GM | PRIV_ALLMOVE) )
		return false;

	CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();
	CChar *pCharMark = dynamic_cast<CChar*>(pObjTop);
	if ( ppCharMark != NULL )
		*ppCharMark = pCharMark;

	if ( static_cast<const CChar*>(pObjTop) == this )
		return false;	// this is yours

	if ( pCharMark == NULL )	// In some (or is) container.
	{
		if ( pItem->IsAttr(ATTR_OWNED) && pItem->m_uidLink != GetUID() )
			return true;

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pObjTop);
		if ( pCont )
		{
			if ( pCont->IsAttr(ATTR_OWNED) )
				return true;
		}

		const CItemCorpse *pCorpseItem = dynamic_cast<const CItemCorpse *>(pObjTop);
		if ( pCorpseItem )		// taking stuff off someones corpse can be a crime
			return const_cast<CChar*>(this)->CheckCorpseCrime(pCorpseItem, true, true);

		return false;	// I guess it's not a crime
	}

	if ( pCharMark->IsOwnedBy(this) || (pCharMark->Memory_FindObjTypes(this, MEMORY_FRIEND) != NULL) )	// he lets you
		return false;

	// Pack animal has no owner ?
	if ( pCharMark->m_pNPC && (pCharMark->GetNPCBrain() == NPCBRAIN_ANIMAL) && !pCharMark->IsStatFlag(STATF_PET) )
		return false;

	return true;
}

bool CChar::CanUse( CItem *pItem, bool fMoveOrConsume ) const
{
	ADDTOCALLSTACK("CChar::CanUse");
	// Can the Char use ( CONSUME )  the item where it is ?
	// NOTE: Although this implies that we pick it up and use it.

	if ( !CanTouch(pItem) )
		return false;

	if ( fMoveOrConsume )
	{
		if ( !CanMove(pItem) )
			return false;	// ? using does not imply moving in all cases ! such as reading ?
		if ( IsTakeCrime(pItem) )
			return false;
	}
	else
	{
		// Just snooping i guess.
		if ( pItem->IsTopLevel() )
			return true;
		// The item is on another character ?
		const CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();
		if ( pObjTop != (static_cast<const CObjBaseTemplate*>(this)) )
		{
			if ( IsPriv(PRIV_GM) || pItem->IsType(IT_CONTAINER) || pItem->IsType(IT_BOOK) )
				return true;
			return false;
		}
	}

	return true;
}

bool CChar::IsMountCapable() const
{
	ADDTOCALLSTACK("CChar::IsMountCapable");
	// Is the character capable of mounting rides?
	// RETURN:
	//  false = incapable of riding

	if ( IsStatFlag(STATF_DEAD) )
		return false;
	if ( IsHuman() || IsElf() || (GetAbilityFlags() & CAN_C_MOUNT) )
		return true;

	return false;
}

bool CChar::IsVerticalSpace( CPointMap ptDest, bool fForceMount )
{
	ADDTOCALLSTACK("CChar::IsVerticalSpace");
	if ( IsPriv(PRIV_GM | PRIV_ALLMOVE) || !ptDest.IsValidPoint() )
		return true;

	dword dwBlockFlags = GetMoveBlockFlags();
	if ( dwBlockFlags & CAN_C_WALK )
		dwBlockFlags |= CAN_I_CLIMB;

	CServerMapBlockState block(dwBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + GetHeightMount(), ptDest.m_z + m_zClimbHeight + 2, GetHeightMount());
	g_World.GetHeightPoint(ptDest, block, true);

	if ( GetHeightMount() + ptDest.m_z + (fForceMount ? 4 : 0) >= block.m_Top.m_z )		// 4 is the mount height
		return false;
	return true;
}

CRegion *CChar::CheckValidMove( CPointBase &ptDest, dword *pdwBlockFlags, DIR_TYPE dir, height_t *pClimbHeight, bool fPathFinding ) const
{
	ADDTOCALLSTACK("CChar::CheckValidMove");
	// Is it ok to move here ? is it blocked ?
	// ignore other characters for now.
	// RETURN:
	//  The new region we may be in.
	//  Fill in the proper ptDest.m_z value for this location. (if walking)
	//  pdwBlockFlags = what is blocking me. (can be null = don't care)

	//	test diagonal dirs by two others *only* when already having a normal location
	if ( GetTopPoint().IsValidPoint() && !fPathFinding && (dir % 2) )
	{
		CPointMap ptTest;
		DIR_TYPE dirTest1 = static_cast<DIR_TYPE>(dir - 1); // get 1st ortogonal
		DIR_TYPE dirTest2 = static_cast<DIR_TYPE>(dir + 1); // get 2nd ortogonal
		if ( dirTest2 == DIR_QTY )		// roll over
			dirTest2 = DIR_N;

		ptTest = GetTopPoint();
		ptTest.Move(dirTest1);
		if ( !CheckValidMove(ptTest, pdwBlockFlags, DIR_QTY, pClimbHeight) )
			return NULL;

		ptTest = GetTopPoint();
		ptTest.Move(dirTest2);
		if ( !CheckValidMove(ptTest, pdwBlockFlags, DIR_QTY, pClimbHeight) )
			return NULL;
	}

	CRegion *pArea = ptDest.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA|REGION_TYPE_ROOM);
	if ( !pArea )
	{
		if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
			g_pLog->EventWarn("Failed to get region\n");
		return NULL;
	}

	dword dwCan = GetMoveBlockFlags();
	if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
		g_pLog->EventWarn("GetMoveBlockFlags() (0x%x)\n", dwCan);
	if ( !(dwCan & (CAN_C_SWIM| CAN_C_WALK|CAN_C_FLY|CAN_C_RUN|CAN_C_HOVER)) )
		return NULL;	// cannot move at all, so WTF?

	dword dwBlockFlags = dwCan;
	if ( dwCan & CAN_C_WALK )
	{
		dwBlockFlags |= CAN_I_CLIMB;		// if we can walk than we can climb. Ignore CAN_C_FLY at all here
		if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
			g_pLog->EventWarn("dwBlockFlags (0%x) dwCan(0%x)\n", dwBlockFlags, dwCan);
	}

	CServerMapBlockState block(dwBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + GetHeightMount(), ptDest.m_z + m_zClimbHeight + 3, GetHeightMount());
	if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
		g_pLog->EventWarn("\t\tCServerMapBlockState block( 0%x, %d, %d, %d );ptDest.m_z(%d) m_zClimbHeight(%d)\n",
					dwBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + GetHeightMount(), ptDest.m_z + m_zClimbHeight + 2, ptDest.m_z, m_zClimbHeight);

	if ( !ptDest.IsValidPoint() )
	{
		DEBUG_ERR(("Character 0%x on %d,%d,%d wants to move into an invalid location %d,%d,%d.\n",
			GetUID().GetObjUID(), GetTopPoint().m_x, GetTopPoint().m_y, GetTopPoint().m_z, ptDest.m_x, ptDest.m_y, ptDest.m_z));
		return NULL;
	}
	g_World.GetHeightPoint(ptDest, block, true);

	// Pass along my results.
	dwBlockFlags = block.m_Bottom.m_dwBlockFlags;

	if ( block.m_Top.m_dwBlockFlags )
	{
		dwBlockFlags |= CAN_I_ROOF;	// we are covered by something.
		if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
			g_pLog->EventWarn("block.m_Top.m_z (%d) > ptDest.m_z (%d) + m_zClimbHeight (%d) + (block.m_Top.m_dwTile (0x%x) > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT/2 )(%d)\n",
				block.m_Top.m_z, ptDest.m_z, m_zClimbHeight, block.m_Top.m_dwTile, ptDest.m_z - (m_zClimbHeight + (block.m_Top.m_dwTile > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT / 2)));
		if ( block.m_Top.m_z < block.m_Bottom.m_z + (m_zClimbHeight + (block.m_Top.m_dwTile > TERRAIN_QTY ? GetHeightMount() : GetHeightMount() / 2)) )
			dwBlockFlags |= CAN_I_BLOCK;		// we can't fit under this!
	}

	if ( (dwCan != 0xFFFFFFFF) && (dwBlockFlags != 0x0) )
	{
		if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
			g_pLog->EventWarn("BOTTOMitemID (0%x) TOPitemID (0%x)\n", (block.m_Bottom.m_dwTile - TERRAIN_QTY), (block.m_Top.m_dwTile - TERRAIN_QTY));

		if ( (dwBlockFlags & CAN_I_DOOR) && !Can(CAN_C_GHOST) )
			dwBlockFlags |= CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_ROOF) && !Can(CAN_C_INDOORS) )
			dwBlockFlags |= CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_WATER) && !Can(CAN_C_SWIM) )
			dwBlockFlags |= CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_HOVER) && !Can(CAN_C_HOVER) && !IsStatFlag(STATF_HOVERING) )
			dwBlockFlags |= CAN_I_BLOCK;

		// If anything blocks us it should not be overridden by this.
		if ( (dwBlockFlags & CAN_I_DOOR) && Can(CAN_C_GHOST) )
			dwBlockFlags &= ~CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_ROOF) && Can(CAN_C_INDOORS) )
			dwBlockFlags &= ~CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_WATER) && Can(CAN_C_SWIM) )
			dwBlockFlags &= ~CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_PLATFORM) && Can(CAN_C_WALK) )
			dwBlockFlags &= ~CAN_I_BLOCK;
		else if ( (dwBlockFlags & CAN_I_HOVER) && (Can(CAN_C_HOVER) || IsStatFlag(STATF_HOVERING)) )
			dwBlockFlags &= ~CAN_I_BLOCK;

		if ( !Can(CAN_C_FLY) )
		{
			if ( !(dwBlockFlags & CAN_I_CLIMB) ) // we can climb anywhere
			{
				if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
					g_pLog->EventWarn("block.m_Lowest.m_z %d  block.m_Bottom.m_z %d  block.m_Top.m_z %d\n", block.m_Lowest.m_z, block.m_Bottom.m_z, block.m_Top.m_z);
				if ( block.m_Bottom.m_dwTile > TERRAIN_QTY )
				{
					if ( block.m_Bottom.m_z > ptDest.m_z + m_zClimbHeight + 2 ) // Too high to climb.
						return NULL;
				}
				else if ( block.m_Bottom.m_z > ptDest.m_z + m_zClimbHeight + GetHeightMount() + 3 )
					return NULL;
			}
		}

		// CAN_I_CLIMB is not releveant for moving as you would need CAN_C_FLY to negate it. All others seem to match
		// and the above uncommented checks are redundant (even dont make sense(?))
		//dword dwMoveBlock = (dwBlockFlags & CAN_I_MOVEMASK) &~ (CAN_I_CLIMB);
		//dword dwMoveBlock = (dwBlockFlags & CAN_I_MOVEMASK) &~ (CAN_I_CLIMB|CAN_I_ROOF);
		//if ( dwMoveBlock &~ dwCan )
		if ( (dwBlockFlags & CAN_I_BLOCK) && !Can(CAN_C_PASSWALLS) )
			return NULL;
		if ( block.m_Bottom.m_z >= UO_SIZE_Z )
			return NULL;
	}

	if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
		g_pLog->EventWarn("GetHeightMount() %d  block.m_Top.m_z  %d ptDest.m_z  %d\n", GetHeightMount(), block.m_Top.m_z, ptDest.m_z);
	if ( (GetHeightMount() + ptDest.m_z >= block.m_Top.m_z) && g_Cfg.m_iMountHeight && !IsPriv(PRIV_GM) && !IsPriv(PRIV_ALLMOVE) )
	{
		SysMessageDefault(DEFMSG_MSG_MOUNT_CEILING);
		return NULL;
	}

	if ( pdwBlockFlags )
		*pdwBlockFlags = dwBlockFlags;
	if ( (dwBlockFlags & CAN_I_CLIMB) && pClimbHeight )
		*pClimbHeight = block.m_zClimbHeight;

	ptDest.m_z = block.m_Bottom.m_z;
	return pArea;
}

void CChar::FixClimbHeight()
{
	ADDTOCALLSTACK("CChar::FixClimbHeight");
	CPointBase pt = GetTopPoint();
	CServerMapBlockState block(CAN_I_CLIMB, pt.m_z, pt.m_z + GetHeightMount() + 3, pt.m_z + 2, GetHeightMount());
	g_World.GetHeightPoint(pt, block, true);

	if ( (block.m_Bottom.m_z == pt.m_z) && (block.m_dwBlockFlags & CAN_I_CLIMB) )	// we are standing on stairs
		m_zClimbHeight = block.m_zClimbHeight;
	else
		m_zClimbHeight = 0;
	m_fClimbUpdated = true;
}
