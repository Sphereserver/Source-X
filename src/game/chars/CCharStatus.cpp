//  CChar is either an NPC or a Player.

#include "../../common/CLog.h"
#include "../components/CCPropsItemEquippable.h"
#include "../components/CCPropsItemWeapon.h"
#include "../clients/CClient.h"
#include "../items/CItemMulti.h"
#include "../CServer.h"
#include "../CWorldMap.h"
#include "../spheresvr.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

bool CChar::IsResourceMatch( const CResourceID& rid, dword dwAmount ) const
{
	ADDTOCALLSTACK("CChar::IsResourceMatch");
	return IsResourceMatch(rid, dwAmount, 0);
}

bool CChar::IsResourceMatch( const CResourceID& rid, dword dwAmount, dword dwArgResearch ) const
{
	ADDTOCALLSTACK("CChar::IsResourceMatch (dwArgResearch)");
	// Is the char a match for this test ?
	switch ( rid.GetResType() )
	{
		case RES_SKILL:			// do I have this skill level?
			if ( Skill_GetBase((SKILL_TYPE)(rid.GetResIndex())) < dwAmount )
				return false;
			return true;

		case RES_CHARDEF:		// I'm this type of char?
		{
            const CCharBase *pCharDef = Char_GetDef();
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
                const CCharBase *pCharDef = Char_GetDef();
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
                const CCharBase *pCharDef = Char_GetDef();
				ASSERT(pCharDef);
				if ( pCharDef->m_TEvents.ContainsResourceID(rid) )
					return true;
			}
			return false;
		}

		case RES_TYPEDEF:		// do I have these in my posession?
			return (ContentConsumeTest(rid, dwAmount, dwArgResearch) == 0);

		case RES_ITEMDEF:		// do I have these in my posession?
			return (ContentConsumeTest(rid, dwAmount) == 0);
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

        case LAYER_STABLE:
            id = ITEMID_POUCH;
            break;

		case LAYER_VENDOR_STOCK:
		case LAYER_VENDOR_EXTRA:
		case LAYER_VENDOR_BUYS:
			if ( !NPC_IsVendor() )
				return nullptr;
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
		for (CSObjContRec* pObjRec : *pPack)
		{
			CItem* pItem = static_cast<CItem*>(pObjRec);
			if ( pItem->GetID() == id )
				return pItem;
		}
	}
	return nullptr;
}

CItem *CChar::LayerFind( LAYER_TYPE layer ) const
{
	ADDTOCALLSTACK("CChar::LayerFind");
	// Find an item i have equipped.

	for (CSObjContRec* pObjRec : *this)
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if ( pItem->GetEquipLayer() == layer )
			return pItem;
	}
	return nullptr;
}

TRIGRET_TYPE CChar::OnCharTrigForLayerLoop( CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CSString *pResult, LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CChar::OnCharTrigForLayerLoop");
	const CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;

	for (CSObjContRec* pObjRec : GetIterationSafeCont())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
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
	if ( EndContext.m_iOffset <= StartContext.m_iOffset )
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

int CChar::GetWeightLoadPercent(int iWeight) const
{
	ADDTOCALLSTACK("CChar::GetWeightLoadPercent");
	// Get a percent of load.
	if (IsPriv(PRIV_GM))
		return 1;

	int	MaxCarry = g_Cfg.Calc_MaxCarryWeight(this);
	if (!MaxCarry)
		return 1000;	// suppose self extra-overloaded

	return (int)IMulDivLL(iWeight, 100, MaxCarry);
}

bool CChar::CanCarry( const CItem *pItem ) const
{
	ADDTOCALLSTACK("CChar::CanCarry");
	if ( IsPriv(PRIV_GM) )
		return true;

	int iItemWeight = 0;
    if (IsSetOF(OF_OWNoDropCarriedItem))
    {
        const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
        if (this != pObjTop)    // Aren't we already carrying it ?
            iItemWeight = pItem->GetWeight();
    }
    else if (pItem->GetEquipLayer() != LAYER_DRAGGING && !pItem->IsItemEquipped())
    {
        /*
		If we're dragging the item, its weight is already added on char so don't count it again.
		Same if we are already wearing the item, don't count the item weight again.
		*/
        iItemWeight = pItem->GetWeight();
    }

    return (GetTotalWeight() + iItemWeight <= g_Cfg.Calc_MaxCarryWeight(this));
}

void CChar::ContentAdd( CItem * pItem, bool fForceNoStack )
{
	ADDTOCALLSTACK("CChar::ContentAdd");
	UnreferencedParameter(fForceNoStack);
	ItemEquip(pItem);
	//LayerAdd( pItem, LAYER_QTY );
}

bool CChar::CanEquipStr( CItem *pItem ) const
{
	ADDTOCALLSTACK("CChar::CanEquipStr");
	if ( IsPriv(PRIV_GM) )
		return true;

    const CItemBase *pItemDef = pItem->Item_GetDef();
	LAYER_TYPE layer = pItemDef->GetEquipLayer();
	if ( !pItemDef->IsTypeEquippable() || !CItemBase::IsVisibleLayer(layer) )
		return true;

	if ( Stat_GetAdjusted(STAT_STR) >= pItemDef->m_ttEquippable.m_iStrReq * (100 - pItem->GetPropNum(COMP_PROPS_ITEMEQUIPPABLE, PROPIEQUIP_LOWERREQ, true)) / 100 )
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
			if ( (m_pPlayer || fTest) && !CanEquipStr(pItem) )
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

	if ( (pItem->GetParent() == this) && (pItem->GetEquipLayer() == layer) ) // not a visible item LAYER_TYPE
		return layer;

	CItem *pItemPrev = nullptr;
	bool fCantEquip = false;

	switch ( layer )
	{
		case LAYER_PACK:
		case LAYER_AUCTION:
        case LAYER_STABLE:
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
        case LAYER_STORAGE:
            fCantEquip = true;
            switch (pItem->GetType())
            {
                case IT_CONTAINER:
                case IT_CONTAINER_LOCKED:
                    fCantEquip = false;
                    break;
            }
            break;
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
                if ( pItemDef->GetDispID() != ITEMID_LIGHT_SRC )	// this light source item is a memory equipped on LAYER_HAND2, so it's ok to equip it even without proper TYPE/CAN
                {
                    fCantEquip = true;
                    break;
                }
			}

			if ( layer == LAYER_HAND2 )
			{
				// If it's a 2 handed weapon, unequip the other hand
				if ( CCPropsItemWeapon::CanSubscribe(pItem) )
					pItemPrev = LayerFind(LAYER_HAND1);
			}
			else
			{
				// Unequip 2 handed weapons if we must use the other hand
				pItemPrev = LayerFind(LAYER_HAND2);
				if ( pItemPrev && !CCPropsItemWeapon::CanSubscribe(pItemPrev) ) //If the item in Layer 2 is not a weapon, don't unequip it.
					pItemPrev = nullptr;
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
            case LAYER_STABLE:
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
				if ( !CanMoveItem(pItemPrev) )
					return LAYER_NONE;
				if (!fTest && !ItemBounce(pItemPrev) )
					return LAYER_NONE;
				break;
			}
		}
	}

	return layer;
}

int CChar::GetStatPercent(STAT_TYPE i) const
{
	ADDTOCALLSTACK("CChar::GetStatPercent");
	ushort maxval = Stat_GetMaxAdjusted(i);
	if (!maxval)
		return 0;
	return IMulDiv(Stat_GetVal(i), 100, maxval);
}


const CObjBaseTemplate* CChar::GetTopLevelObj() const
{
	// Get the object that has a location in the world. (Ground level)
	return this;
}

CObjBaseTemplate* CChar::GetTopLevelObj()
{
	// Get the object that has a location in the world. (Ground level)
	return this;
}

bool CChar::IsSwimming() const
{
	ADDTOCALLSTACK("CChar::IsSwimming");
	// Am i in/over/slightly under the water now ?
	// NOTE: This is a bit more complex because we need to test if we are slightly under water.

	const CPointMap& ptTop = GetTopPoint();

	const CPointMap pt = CWorldMap::FindItemTypeNearby(ptTop, IT_WATER);
	if ( !pt.IsValidPoint() )
		return false;

	short iDistZ = ptTop.m_z - pt.m_z;
	if ( iDistZ < -PLAYER_HEIGHT )	// far under the water somehow
		return false;

	// Is there a solid surface under us?
	uint64 uiBlockFlags = GetCanMoveFlags(GetCanFlags());
	char iSurfaceZ = CWorldMap::GetHeightPoint2(ptTop, uiBlockFlags, true);
	if ((iSurfaceZ == pt.m_z) && (uiBlockFlags & CAN_I_WATER))
		return true;

	return false;
}

bool CChar::IsNPC() const
{
    return (m_pNPC != nullptr);
}

NPCBRAIN_TYPE CChar::GetNPCBrain() const
{
    ASSERT(m_pNPC);
    return m_pNPC->m_Brain;
}

NPCBRAIN_TYPE CChar::GetNPCBrainGroup() const noexcept
{
    //ADDTOCALLSTACK("CChar::GetNPCBrainGroup");
    // Return NPCBRAIN_ANIMAL for animals, _HUMAN for NPC human and PCs, >= _MONSTER for monsters
    //	(can return also _BERSERK and _DRAGON)
    // For tracking and other purposes.

    if (m_pNPC)
    {
        if ((m_pNPC->m_Brain >= NPCBRAIN_HUMAN) && (m_pNPC->m_Brain <= NPCBRAIN_STABLE))
            return NPCBRAIN_HUMAN;

        if ((m_pNPC->m_Brain <= NPCBRAIN_NONE) || (m_pNPC->m_Brain >= NPCBRAIN_QTY))
            return GetNPCBrainAuto();
        return m_pNPC->m_Brain;
    }
    else if (m_pPlayer)
        return NPCBRAIN_HUMAN;
    //ASSERT(0);
    return NPCBRAIN_NONE;
}

NPCBRAIN_TYPE CChar::GetNPCBrainAuto() const noexcept
{
	//ADDTOCALLSTACK("CChar::GetNPCBrainAuto");
	// Auto-detect the brain
	const CREID_TYPE id = GetDispID();
	
	switch (id)
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

	if ((id == CREID_ENERGY_VORTEX) || (id == CREID_BLADE_SPIRIT))
		return NPCBRAIN_BERSERK;

	switch ( id )
	{
		case CREID_EAGLE:
		case CREID_BIRD:
		case CREID_GORILLA:
		case CREID_SNAKE:
		case CREID_BULL_FROG:
		case CREID_DOLPHIN:
			return NPCBRAIN_ANIMAL;
	}

	if (id >= CREID_IRON_GOLEM)
		return NPCBRAIN_MONSTER;

	if ( id >= CREID_MAN )
		return NPCBRAIN_HUMAN;

	if ( id >= CREID_HORSE_TAN )
		return NPCBRAIN_ANIMAL;

	return NPCBRAIN_MONSTER;
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
    const CCharBase *pCharDef = Char_GetDef();
	byte mode = 0;

	if ( IsStatFlag(STATF_FREEZE|STATF_STONE) )
		mode |= CHARMODE_FREEZE;
	if ( pCharDef->IsFemale() )
		mode |= CHARMODE_FEMALE;

	if ( pViewer && (pViewer->GetNetState()->isClientVersionNumber(MINCLIVER_SA) || pViewer->GetNetState()->isClientEnhanced()) )
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
	
	//When you want change the color of character anim, you must evitate to send CHARMODE_INVIS because the anim will automaticly be grey
	//Here we check if it's define on the ini that you need override the color
	if ( !g_Cfg.m_iColorInvis )			//Serv.ColorInvis
        iFlags |= STATF_INSUBSTANTIAL;
	if ( !g_Cfg.m_iColorHidden )		//serv.ColorHidden
        iFlags |= STATF_HIDDEN;
	if ( !g_Cfg.m_iColorInvisSpell )	//serv.ColorInvisSpell
        iFlags |= STATF_INVISIBLE;
	
	if ( IsStatFlag(iFlags) )	// Checking if I have any of these settings enabled on the ini and I have any of them, if so ... CHARMODE_INVIS is set and color applied.
	mode |= CHARMODE_INVIS; //When sending CHARMODE_INVIS state to client, your character anim are grey

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

	return dir;
}

uint64 CChar::GetCanMoveFlags(uint64 uiCanFlags, bool fIgnoreGM) const
{
	// What things do not block us ?
	if ( IsPriv(PRIV_GM|PRIV_ALLMOVE) && !fIgnoreGM )
		return UINT64_MAX;	// nothing blocks us: we are able to move onto everything.

	if ( IsStatFlag(STATF_DEAD) )
		uiCanFlags |= CAN_C_GHOST;

	if ( IsStatFlag(STATF_HOVERING) )
		uiCanFlags |= CAN_C_HOVER;

    // if we can walk than we can CLIMB. The char flag for doing that is FLY.
    if (uiCanFlags & CAN_C_WALK)
        uiCanFlags |= CAN_C_FLY;

	return ( uiCanFlags & CAN_C_MOVEMASK );
}

byte CChar::GetLightLevel() const
{
	ADDTOCALLSTACK("CChar::GetLightLevel");
	// Get personal default light level.

	if ( IsStatFlag(STATF_DEAD|STATF_SLEEPING|STATF_NIGHTSIGHT) || IsPriv(PRIV_DEBUG) )
		return LIGHT_BRIGHT;
	if ( (g_Cfg.m_iRacialFlags & RACIALF_ELF_NIGHTSIGHT) && IsElf() )		// elves always have nightsight enabled (Night Sight racial trait)
		return LIGHT_BRIGHT;
	const CSector* pSector = GetTopSector();
	ASSERT(pSector);
	return pSector->GetLight();
}

CItem *CChar::GetSpellbook(SPELL_TYPE iSpell) const	// Retrieves a spellbook from the magic school given in iSpell
{
	ADDTOCALLSTACK("CChar::GetSpellbook");
	// Search for suitable book in hands first
	CItem* pReturn = nullptr;
	CItem* pItem = GetSpellbookLayer();
	if ( pItem )
	{
		const CItemBase *pItemDef = pItem->Item_GetDef();
		const SPELL_TYPE min = (SPELL_TYPE)pItemDef->m_ttSpellbook.m_iOffset;
		const SPELL_TYPE max = (SPELL_TYPE)(pItemDef->m_ttSpellbook.m_iOffset + pItemDef->m_ttSpellbook.m_iMaxSpells);
	    if ( (iSpell > min) && (iSpell <= max) ) //Had to replace < with <= otherwise the spell would not be considered a valid one.
	    {
		    if (pItem->IsSpellInBook(iSpell) )	//We found a book with this same spell, nothing more to do.
			    return pItem;
			else // I did not find the spell, but this book is of the same school ... so i'll return this book if none better is found (NOTE: some book must be returned or the code will think that i don't have any book).
		    	pReturn = pItem;
		}
    }
	// No book found in layer 1 or 2 or found one which doesn't have the spell I am going to cast, then let's search in the top level of the backpack.
	CItemContainer *pPack = GetPack();
	if ( pPack )
	{
		for (CSObjContRec* pObjRec : *pPack)
		{
			pItem = static_cast<CItem*>(pObjRec);
			if ( !pItem->IsTypeSpellbook() )
				continue;
            // Found a book, let's find each magic school's offsets to search for the desired spell.
			const CItemBase *pItemDef = pItem->Item_GetDef();
			const SPELL_TYPE min = (SPELL_TYPE)pItemDef->m_ttSpellbook.m_iOffset;
			const SPELL_TYPE max = (SPELL_TYPE)(pItemDef->m_ttSpellbook.m_iOffset + pItemDef->m_ttSpellbook.m_iMaxSpells);
			if ( (iSpell > min) && (iSpell <= max) ) // and check now the spell is within the spells that this book can hold. Had to replace < with <= otherwise the spell would not be considered a valid one.
			{
				if ( pItem->IsSpellInBook(iSpell) )	//I found a book with this spell, nothing more to do.
					return pItem;
				else // I did not find the spell, but this book is of the same school ... so i'll return this book if none better is found (NOTE: some book must be returned or the code will think that i don't have any book).
					pReturn = pItem;
			}
		}
	}
	return pReturn;
}

CItem * CChar::GetSpellbookLayer() const //Retrieve a Spellbook from Layer 1 or Layer 2
{
	CItem* pItem = LayerFind(LAYER_HAND1);    // Let's do first a direct search for any book in hand layer 1.
	if (pItem && pItem->IsTypeSpellbook())
		return pItem;
	pItem = LayerFind(LAYER_HAND2); // on custom freeshard, it's possible to have book on layer 2
	if (pItem && pItem->IsTypeSpellbook())
		return pItem;
	return nullptr;
}

short CChar::Food_GetLevelPercent() const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelPercent");
	ushort max = Stat_GetMaxAdjusted(STAT_FOOD);
	if ( max == 0 )
		return 100;
	return (short)(IMulDiv(Stat_GetVal(STAT_FOOD), 100, max));
}

lpctstr CChar::Food_GetLevelMessage(bool fPet, bool fHappy) const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelMessage");
	ushort max = Stat_GetMaxAdjusted(STAT_FOOD);
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

		if ( index >= (ARRAY_COUNT(sm_szPetHunger) - 1) )
			index = ARRAY_COUNT(sm_szPetHunger) - 1;

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

	if ( index >= (ARRAY_COUNT(sm_szFoodLevel) - 1) )
		index = ARRAY_COUNT(sm_szFoodLevel) - 1;

	return sm_szFoodLevel[index];
}

ushort CChar::Food_CanEat( CObjBase *pObj ) const
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

    const CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	size_t iRet = pCharDef->m_FoodType.FindResourceMatch(pObj);
	if ( iRet != sl::scont_bad_index() )
		return (ushort)(pCharDef->m_FoodType[iRet].GetResQty());	// how bad do i want it?

	return 0;
}

CChar * CChar::GetOwner() const
{
	ADDTOCALLSTACK("CChar::GetOwner");

	if (m_pPlayer)
		return nullptr;
	if (m_pNPC)
		return NPC_PetGetOwner();
	return nullptr;
}

bool CChar::CanDress(const CChar* pChar) const
{
    if (IsPriv(PRIV_GM) && (GetPrivLevel() > pChar->GetPrivLevel() || GetPrivLevel() == PLEVEL_Owner))
        return true;
    else if (g_Cfg.m_fCanUndressPets && pChar->IsOwnedBy(this))
        return true;
    return false;
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
		return m_sTitle.GetBuffer();

	tchar *pTemp = Str_GetTemp();
    const CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	// Incognito ?
	// If polymorphed then use the poly name.
	if ( IsStatFlag(STATF_INCOGNITO) || !IsPlayableCharacter() || (m_pNPC && (pCharDef->GetTypeName() != pCharDef->GetTradeName())) )
	{
		if ( !IsIndividualName() )
			return "";	// same as type anyhow.
		lpctstr ptcArticle = pCharDef->IsFemale() ? g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_FEMALE) : g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_MALE);
		snprintf(pTemp, Str_TempLength(), "%s %s", ptcArticle, pCharDef->GetTradeName());
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
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE),	(int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NEOPHYTE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NOVICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_APPRENTICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_JOURNEYMAN")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_EXPERT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ADEPT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_MASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_GRANDMASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ELDER_NINJITSU), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ELDER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_LEGENDARY_NINJITSU), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_LEGENDARY")) },
			{ nullptr, INT32_MAX }
		};
		len = snprintf(pTemp, Str_TempLength(), "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}
	else if ( skill == SKILL_BUSHIDO )
	{
		static const CValStr sm_SkillTitles[] =
		{
			{ "", INT32_MIN },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE),	(int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NEOPHYTE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NOVICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_APPRENTICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_JOURNEYMAN")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_EXPERT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ADEPT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_MASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_GRANDMASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ELDER_BUSHIDO), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ELDER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_LEGENDARY_BUSHIDO), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_LEGENDARY")) },
			{ nullptr, INT32_MAX }
		};
		len = snprintf(pTemp, Str_TempLength(), "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}
	else
	{
		static const CValStr sm_SkillTitles[] =
		{
			{ "", INT32_MIN },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE),	(int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NEOPHYTE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_NOVICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_APPRENTICE")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_JOURNEYMAN")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_EXPERT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ADEPT")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_MASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER),(int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_GRANDMASTER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ELDER), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_ELDER")) },
			{ g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_LEGENDARY), (int)(g_Exp.m_VarDefs.GetKeyNum("SKILLTITLE_LEGENDARY")) },
			{ nullptr, INT32_MAX }
		};
		len = snprintf(pTemp, Str_TempLength(), "%s ", sm_SkillTitles->FindName(Skill_GetBase(skill)));
	}

	snprintf(pTemp + len, Str_TempLength() - len, "%s", g_Cfg.GetSkillDef(skill)->m_sTitle.GetBuffer());
	return pTemp;
}

bool CChar::CanDisturb( const CChar *pChar ) const
{
	ADDTOCALLSTACK_DEBUG("CChar::CanDisturb");
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
            if (IsNPC() && NPC_PetGetOwner() != pChar) //NPCs should see their owners even if iDeadCannotSee equals to 2.
            {
                if (iDeadCannotSee == 2)
                    return false;
            }
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

    const CChar *pChar = static_cast<const CChar*>(pContItem->GetTopLevelObj());
	if ( !pChar )
		return false;

	if ( pContItem->IsType(IT_EQ_TRADE_WINDOW) )
	{
		if ( pChar == this )
			return true;

        const CItem *pItemTrade = pContItem->m_uidLink.ItemFind();
		if ( pItemTrade )
		{
            const CChar *pCharTrade = static_cast<const CChar*>(pItemTrade->GetTopLevelObj());
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
//true = client can see the invisble target
{
	ADDTOCALLSTACK("CChar::CanSee");
	// Can I see this object (char or item)?
	if ( !pObj || IsDisconnected() )
		return false;

    const CPointMap& ptObjTop = pObj->GetTopLevelObj()->GetTopPoint();
    if (!ptObjTop.IsValidPoint())
        return false;

    const CPointMap& ptTop = GetTopPoint();
    int iDistSight = GetVisualRange();

    /*
    const CRegion* pRegionHouse = ptTop.GetRegion(REGION_TYPE_HOUSE);
    if (pRegionHouse)
    {
        const CItemMulti* pMultiRegion = static_cast<const CItemMulti*>(pRegionHouse->GetResourceID().ItemFindFromResource());
        // If it's a public house
        {
            const bool fShowPublicHouseContent = IsClientActive() ? GetClientActive()->_fShowPublicHouseContent : false;
            if (!fShowPublicHouseContent)
                return false;
        }
    }
    */

	if ( pObj->IsItem() )
	{
		const CItem *pItem = static_cast<const CItem*>(pObj);
		if ( !pItem || !CanSeeItem(pItem) )
			return false;

        if (pItem->IsTypeMulti())
        {
            const DIR_TYPE dirFace = GetDir(pObj);
            const CItemMulti *pMulti = static_cast<const CItemMulti*>(pItem);
            iDistSight += pMulti->GetSideDistanceFromCenter(dirFace);
        }
		if ( ptTop.GetDistSight(ptObjTop) > iDistSight )
			return false;

		const CObjBase *pObjCont = pItem->GetContainer();
		if ( pObjCont )
		{
			if (IsSetEF(EF_FixCanSeeInClosedConts))
			{
                // the bSkip check is mainly needed when a trade window is created script-side
                bool fSkip = false;
                if (const CItemContainer* pItemContainer = dynamic_cast<const CItemContainer*>(pObjCont))
                {
                    if (pItemContainer->IsType(IT_EQ_TRADE_WINDOW))
                    {
                        // Both of the chars in a trade can see the items being traded
                        if (this == dynamic_cast<const CChar*>(pItemContainer->GetContainer()))
                        {
                            fSkip = true;
                        }
                        else
                        {
                            if (const CItem* pLinkedTradeWindow = pItemContainer->m_uidLink.ItemFind())
                                if (const CChar* pSecondClientInTrade = dynamic_cast<const CChar*>(pLinkedTradeWindow->GetContainer()))
                                    if (this == pSecondClientInTrade)
                                        fSkip = true;
                        }
                    }
                }

				// A client cannot see the contents of someone else's container, unless they have opened it first
				if (!fSkip && IsClientActive() && pObjCont->IsItem() && pObjCont->GetTopLevelObj() != this)
				{
					const CClient *pClient = GetClientActive();
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
            return CanSee(pObjCont);
		}
	}
	else
	{
		const CChar *pChar = static_cast<const CChar*>(pObj);
		if ( pChar == this )
			return true;
		if ( ptTop.GetDistSight(pChar->GetTopPoint()) > iDistSight )
			return false;

        const PLEVEL_TYPE plevelMe = GetPrivLevel();
        const PLEVEL_TYPE plevelChar = pChar->GetPrivLevel();
		if ( IsPriv(PRIV_ALLSHOW) )
			return (plevelMe < plevelChar) ? false : true;

		if ( m_pNPC && pChar->IsStatFlag(STATF_DEAD) )
		{
			if ( m_pNPC->m_Brain != NPCBRAIN_HEALER )
				return false;
		}
		else if ( pChar->IsStatFlag(STATF_INVISIBLE|STATF_INSUBSTANTIAL|STATF_HIDDEN) )
		{
			// Characters can be invisible, but not to GM's (true sight ?)
			// equal plevel can see each other if they are staff members or they return 1 in @SeeHidden
			if ( pChar->GetPrivLevel() <= PLEVEL_Player )
			{
				if ( IsTrigUsed( TRIGGER_SEEHIDDEN ) )
				{
					CScriptTriggerArgs Args;
					Args.m_iN1 = (plevelMe <= plevelChar);
					CChar *pChar2 = const_cast< CChar* >( pChar );
					CChar *this2 = const_cast< CChar* >( this );
					this2->OnTrigger( CTRIG_SeeHidden, pChar2, &Args );
					return ( Args.m_iN1 != 1 );
				}
			}
			//Here we analyse how GM can see other player/GM when they are hide.
			if (plevelMe < 2) //only Plevel over 2 have the possibility to see other
			{
				return false;
			}
			else
			{
				switch (g_Cfg.m_iCanSeeSamePLevel) //Evaluate the .ini setting
				{
				case 0: //GM see all
					if (plevelMe < plevelChar)
					{
						return false;
					}
					break;
				case 1: //Gm dont see same plevel
					if (plevelMe <= plevelChar)
					{
						return false;
					}
					break;
				case 2: //Plevel x and more see all
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					if (plevelMe < g_Cfg.m_iCanSeeSamePLevel)
					{
						return false;
					}
					break;
				}
			}
		}

		if ( IsStatFlag(STATF_DEAD) && !CanSeeAsDead(pChar) )
			return false;

		if ( pChar->IsDisconnected() )
		{
			if ( pChar->IsStatFlag(STATF_RIDDEN) )
			{
				const CChar *pCharRider = Horse_GetMountChar();
				if ( pCharRider )
					return CanSee(pCharRider);
				return false;
			}

			if ( !IsPriv(PRIV_GM) )		// only characters with GM or ALLSHOW priv should be able to see disconnected chars
				return false;
		}
	}

	if ( IsPriv(PRIV_ALLSHOW) && (pObj->IsTopLevel() || pObj->IsDisconnected()) )		// don't exclude for logged out and diff maps
		return (ptTop.GetDistSightBase(ptObjTop) <= iDistSight);

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
		if (!pItem->m_TagDefs.GetKeyNum(uidCheck))
			return false;
	}
	return true;
}

bool CChar::CanTouch( const CPointMap &pt ) const
{
	ADDTOCALLSTACK("CChar::CanTouch(pt)");
	// Can I reach this from where i am.
	// swords, spears, arms length = x units.
	// Use or Grab.
	// Check for blocking objects.
	// It this in a container we can't get to ?

	return CanSeeLOS(pt, nullptr, 6);
}

bool CChar::CanTouch( const CObjBase *pObj ) const
{
	ADDTOCALLSTACK("CChar::CanTouch");
	// Can I reach this from where i am. swords, spears, arms length = x units.
	// Use or Grab. May be in snooped container.
	// Check for blocking objects. Is this in a container we can't get to ?

	if ( !pObj )
		return false;

    const CItem *pItem = nullptr;
	const CObjBaseTemplate *pObjTop = pObj->GetTopLevelObj();
	int iDist = GetTopDist3D(pObjTop);

	if ( pObj->IsItem() )	// some objects can be used anytime. (even by the dead.)
	{
        pItem = static_cast<const CItem *>(pObj);
		bool fDeathImmune = IsPriv(PRIV_GM);
		bool fFreezeImmune = false;
		switch ( pItem->GetType() )
		{
		case IT_SIGN_GUMP:	// can be seen from a distance.
			return (iDist <= pObjTop->GetVisualRange());

		case IT_SHRINE:		// We can use shrines when dead !!
		case IT_TELESCOPE:
            fDeathImmune = true;
			break;
        case IT_CONTAINER:
        case IT_CONTAINER_LOCKED:
        {
            if ( pObjTop && pObjTop == this ) //This is default OSI behaviour, you can look through your backpack while frozen.
                fFreezeImmune = true;
            break;
        }

        case IT_ARCHERY_BUTTE:
        {
            const CItem * pWeapon = m_uidWeapon.ItemFind();
            if (pWeapon)
            {
                IT_TYPE iType = pWeapon->GetType();
                if ((iType == IT_WEAPON_BOW) || (iType == IT_WEAPON_XBOW) || (iType == IT_WEAPON_THROWING))
                    return (iDist <= pWeapon->GetRangeH());
            }
            break;
        }

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
        
        if ( !fDeathImmune && IsStatFlag(STATF_FREEZE) )
        {
            if ( !fFreezeImmune && !pItem->IsAttr(ATTR_CANUSE_PARALYZED) )
                return false;
        }

		if ( !fDeathImmune && IsStatFlag(STATF_DEAD|STATF_SLEEPING|STATF_STONE) )
			return false;
	}

	//	search up to top level object
	const CChar *pChar = nullptr;
	if ( pObjTop && (pObjTop != this) )
	{
		if ( pObjTop->IsChar() )
		{
			pChar = static_cast<const CChar*>(pObjTop);
			if ( pChar == this )
				return true;
			if ( IsPriv(PRIV_GM) )
				return (GetPrivLevel() > pChar->GetPrivLevel() || GetPrivLevel() == PLEVEL_Owner);
			//The check below is needed otherwise, you cannot resurrect a player ghost by using bandages (unless you are a GM), maybe there is a better way?
			if (pChar->IsStatFlag(STATF_DEAD) && (Skill_GetActive() == SKILL_HEALING || Skill_GetActive() == SKILL_VETERINARY))
				return true;
			if ( pChar->IsStatFlag(STATF_DEAD|STATF_STONE) || Can(CAN_C_STATUE) )
				return false;
		}

        const CObjBase *pObjCont = nullptr;
		const CObjBase *pObjTest = pObj;
		for (;;)
		{
			pItem = dynamic_cast<const CItem *>(pObjTest);
			if ( !pItem )
				break;

			// What is this inside of ?
			pObjCont = pItem->GetContainer();
			if ( !pObjCont )
				break;

			pObjTest = pObjCont;
			if ( !CanSeeInContainer(dynamic_cast<const CItemContainer *>(pObjTest)) )
				return false;
		}
	}
	else if (pObjTop == this) //Top container is the player (bank or backpack)
	{
		// Check if the item is in my bankbox, and i'm not in the same position from which I opened it the last time.
		const CPointMap& ptTop = GetTopPoint();
		CItemContainer* pBank = GetChar()->GetBank();
		bool fItemContIsInsideBankBox = pBank->IsItemInside(pObj->GetUID().ItemFind());
		if (fItemContIsInsideBankBox && (pBank->m_itEqBankBox.m_pntOpen != ptTop))
			return false;
	}


	if ( IsPriv(PRIV_GM) )
		return true;

    bool fCanTouch = true;
	if ( !CanSeeLOS(pObjTop) )
	{
        fCanTouch = false;
        if ((Can(CAN_C_DCIGNORELOS)) || (pChar && pChar->Can(CAN_C_DCIGNORELOS)) || (pItem && pItem->Can(CAN_I_DCIGNORELOS)))
        {
            fCanTouch = true;
        }
	}
	if (( iDist > 2 ) && fCanTouch)
	{
        fCanTouch = false;
        if ((Can(CAN_C_DCIGNOREDIST)) || (pChar && pChar->Can(CAN_C_DCIGNOREDIST)) || (pItem && pItem->Can(CAN_I_DCIGNOREDIST)))
        {
            fCanTouch = true;
        }
	}
	return fCanTouch;
}

IT_TYPE CChar::CanTouchStatic( CPointMap *pPt, ITEMID_TYPE id, const CItem *pItem ) const
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
		*pPt = pItem->GetTopLevelObj()->GetTopPoint(); //While item is available, set the target p as the item top point, not the player.
		return pItem->GetType();
	}

	// It's a static !
    const CItemBase *pItemDef = CItemBase::FindItemBase(id);
	if ( !pItemDef )
		return IT_NORMAL;
	if ( !CanTouch(*pPt) )
		return IT_JUNK;

	// Is this static really here ?
	const CServerMapBlock *pMapBlock = CWorldMap::GetMapBlock(*pPt);
	if ( !pMapBlock )
		return IT_JUNK;

	int x2 = pMapBlock->GetOffsetX(pPt->m_x);
	int y2 = pMapBlock->GetOffsetY(pPt->m_y);

	uint iQty = pMapBlock->m_Statics.GetStaticQty();
	for ( uint i = 0; i < iQty; ++i )
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

	// sound can be blocked if in house.
    pSrc = pSrc->GetTopLevelObj();
    const CRegionWorld *pSrcRegion;
	if ( pSrc->IsChar() )
	{
		const CChar *pCharSrc = dynamic_cast<const CChar*>(pSrc);
		ASSERT(pCharSrc);
		pSrcRegion = pCharSrc->GetRegion();
	}
	else
    {
		pSrcRegion = dynamic_cast<CRegionWorld *>(pSrc->GetTopPoint().GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
    }	
	if ( !pSrcRegion || !m_pArea )  // should not happen really.
		return false;

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
        //case TALKMODE_SOUND:
        //case TALKMODE_SAY:
        default:
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

    int iDist = GetTopDist3D(pSrc);
    if ( iDist > iHearRange )	// too far away
        return false;

    if ( IsPriv(PRIV_GM) )
        return true;

    // Check if i can hear from a different region
    if ( fThrough )		        // a yell goes through walls..
        return true;
    if ( m_pArea == pSrcRegion )// same region is always ok.
       return true;

    // Different region (which can be a multi or an areadef). 
    if ( IsSetOF(OF_NoHouseMuteSpeech) )
        return true;

    auto _RegionBlocksSpeech = [](const CRegion* pRegion) -> bool
    {
        const CResourceID& ridRegion = pRegion->GetResourceID();
        const bool fRegionFromItem = ridRegion.IsUIDItem();
        bool fCanSpeech = false;
        const CVarDefCont *pValue = fRegionFromItem ? ridRegion.ItemFindFromResource()->GetKey("NOMUTESPEECH", false) : nullptr;
        if ( pValue && pValue->GetValNum() > 0 )
            fCanSpeech = true;
        if ( fRegionFromItem && !pRegion->IsFlag(REGION_FLAG_SHIP) && !fCanSpeech )
            return false;
        return true;
    };
    if (!_RegionBlocksSpeech(pSrcRegion) || !_RegionBlocksSpeech(m_pArea))
        return false;
    return true;
}

bool CChar::CanMoveItem( const CItem *pItem, bool fMsg ) const
{
	ADDTOCALLSTACK("CChar::CanMoveItem");
	// Is it possible that i could move this ?
	// NOT: test if need to steal. IsTakeCrime()
	// NOT: test if close enough. CanTouch()

	if ( IsPriv(PRIV_ALLMOVE|PRIV_DEBUG|PRIV_GM) )
		return true;
	if ( !pItem )
		return false;
	if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_LOCKEDDOWN) && !pItem->IsAttr(ATTR_MOVE_ALWAYS) )
		return false;

	if ( IsStatFlag(STATF_STONE|STATF_FREEZE|STATF_INSUBSTANTIAL|STATF_DEAD|STATF_SLEEPING) || Can(CAN_C_STATUE) )
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
            const CItemContainer *pItemCont = dynamic_cast<CItemContainer *> (pItem->GetContainer());
			if ( pItemCont && pItemCont->IsItemInTrade() )
			{
				SysMessageDefault(DEFMSG_MSG_TRADE_CANTMOVE);
				return false;
			}
		}

		// Can't move equipped cursed items
		if ( pItem->IsAttr(ATTR_CURSED|ATTR_CURSED2) && pItem->IsItemEquipped() )
		{
			//pItem->SetAttr(ATTR_IDENTIFIED);
			if ( fMsg )
				SysMessagef("%s %s", pItem->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_CANTMOVE_CURSED));

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
                    const CChar *pChar = pCorpse->IsCorpseSleeping();
					if ( pChar && pChar != this )
						return false;
				}
			}
			else if ( pObjTop->IsChar() && (pObjTop != this) )
			{
				if (( pItem->IsAttr(ATTR_NEWBIE) ) && g_Cfg.m_fAllowNewbTransfer )
				{
                    const CChar *pPet = dynamic_cast<const CChar*>( pItem->GetTopLevelObj() );
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

	CObjBaseTemplate *pObjTop = const_cast<CObjBaseTemplate*>(pItem->GetTopLevelObj());
	CChar *pCharMark = dynamic_cast<CChar*>(pObjTop);
	if ( ppCharMark != nullptr )
		*ppCharMark = pCharMark;

	if ( static_cast<const CChar*>(pObjTop) == this )
		return false;	// this is yours

	if ( pCharMark == nullptr )	// In some (or is) container.
	{
		if ( pItem->IsAttr(ATTR_OWNED) && pItem->m_uidLink != GetUID() )
			return true;

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pObjTop);
		if ( pCont )
		{
			if ( pCont->IsAttr(ATTR_OWNED) )
				return true;
		}

		CItemCorpse *pCorpseItem = dynamic_cast<CItemCorpse *>(pObjTop);
		if ( pCorpseItem )		// taking stuff off someones corpse can be a crime
			return const_cast<CChar*>(this)->CheckCorpseCrime(pCorpseItem, true, true);     // const_cast is BAD!

		return false;	// I guess it's not a crime
	}

	if ( pCharMark->IsOwnedBy(this) || (pCharMark->Memory_FindObjTypes(this, MEMORY_FRIEND) != nullptr) )	// he lets you
		return false;

	// Pack animal has no owner ?
	if ( pCharMark->m_pNPC && (pCharMark->GetNPCBrain() == NPCBRAIN_ANIMAL) && !pCharMark->IsStatFlag(STATF_PET) )
		return false;

	return true;
}

bool CChar::CanUse( const CItem *pItem, bool fMoveOrConsume ) const
{
	ADDTOCALLSTACK("CChar::CanUse");
	// Can the Char use ( CONSUME )  the item where it is ?
	// NOTE: Although this implies that we pick it up and use it.

	if ( !CanTouch(pItem) )
		return false;

	if ( fMoveOrConsume )
	{
		if ( !CanMoveItem(pItem) )
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
	if ( IsHuman() || IsElf() || (GetCanFlags() & CAN_C_MOUNT) )
		return true;

	return false;
}

bool CChar::IsVerticalSpace( const CPointMap& ptDest, bool fForceMount ) const
{
	ADDTOCALLSTACK("CChar::IsVerticalSpace");
	if ( IsPriv(PRIV_GM | PRIV_ALLMOVE) || !ptDest.IsValidPoint() )
		return true;

	uint64 uiBlockFlags = GetCanMoveFlags(GetCanFlags());
	if (uiBlockFlags & CAN_C_WALK)
		uiBlockFlags |= CAN_I_CLIMB;

    const height_t iHeightMount = GetHeightMount();
	CServerMapBlockingState block(uiBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + iHeightMount, ptDest.m_z + m_zClimbHeight + 2, iHeightMount);
	CWorldMap::GetHeightPoint(ptDest, block, true);

	if ( iHeightMount + ptDest.m_z + (fForceMount ? 4 : 0) >= block.m_Top.m_z )		// 4 is the mount height
		return false;
	return true;
}

bool CChar::CanStandAt(CPointMap *ptDest, const CRegion* pArea, uint64 uiMyMovementFlags, height_t uiMyHeight, CServerMapBlockingState* blockingState, bool fPathfinding) const
{
    ADDTOCALLSTACK("CChar::CanStandAt");
    ASSERT(ptDest);
    ASSERT(pArea);
    ASSERT(blockingState);

    uint64 uiMapPointMovementFlags = uiMyMovementFlags;
    CWorldMap::GetHeightPoint(*ptDest, *blockingState, true);

    // Pass along my results, blockingState got modified.
    uiMapPointMovementFlags = blockingState->m_Bottom.m_uiBlockFlags;

    uint uiBlockedBy = 0;
    // need to check also for UFLAG1_FLOOR?
    if (blockingState->m_Top.m_uiBlockFlags)
    {
        const bool fTopLandTile = (blockingState->m_Top.m_dwTile <= TERRAIN_QTY);
        if (!fTopLandTile && (blockingState->m_Top.m_uiBlockFlags & (CAN_I_ROOF|CAN_I_PLATFORM|CAN_I_BLOCK)) && Can(CAN_C_NOINDOORS))
            return false;

        const short iHeightDiff = (blockingState->m_Top.m_z - blockingState->m_Bottom.m_z);
        const height_t uiHeightReq = fTopLandTile ? (uiMyHeight / 2) : uiMyHeight;

        if (!fPathfinding && (g_Cfg.m_iDebugFlags & DEBUGF_WALK))
        {
            g_Log.EventWarn("block.m_Top.m_z (%hhd) - block.m_Bottom.m_z (%hhd) < m_zClimbHeight (%hhu) + (block.m_Top.m_dwTile (0x%" PRIx32 ") > TERRAIN_QTY ? iHeightMount : iHeightMount/2 )(%hhu).\n",
                blockingState->m_Top.m_z, blockingState->m_Bottom.m_z, m_zClimbHeight, blockingState->m_Top.m_dwTile, (height_t)(m_zClimbHeight + uiHeightReq));
        }
        if ((iHeightDiff < uiHeightReq) && !Can(CAN_C_NOBLOCKHEIGHT) && !pArea->IsFlag(REGION_FLAG_WALK_NOBLOCKHEIGHT))
        {
            // Two cases possible:
            // 1) On the dest P we would be covered by something and we wouldn't fit under this!
            // 2) On the dest P there's an item but we can pass through it (this special case will be handled with fPassTrough late
            uiMapPointMovementFlags |= CAN_I_BLOCK;
            uiBlockedBy |= CAN_I_ROOF;
        }
        else if (iHeightDiff < (m_zClimbHeight + uiHeightReq))
        {
            // i'm trying to walk on a point over my head, it's possible to climb it but there isn't enough room for me to fit between Top and Bottom tile
            // (i'd bang my head against the ceiling!)
            uiMapPointMovementFlags |= CAN_I_BLOCK;
            uiBlockedBy |= CAN_I_CLIMB;
        }
    }

    const bool fLandTile = (blockingState->m_Bottom.m_dwTile <= TERRAIN_QTY);
    bool fPassTrough = false;
    if ((uiMyMovementFlags != UINT64_MAX) && (uiMapPointMovementFlags != 0x0))
    {
        // It IS in my way and HAS a flag set, check further
        if (!fPathfinding && (g_Cfg.m_iDebugFlags & DEBUGF_WALK))
            g_Log.EventWarn("BOTTOMitemID (0%" PRIx32 ") TOPitemID (0%" PRIx32 ").\n", (blockingState->m_Bottom.m_dwTile - TERRAIN_QTY), (blockingState->m_Top.m_dwTile - TERRAIN_QTY));

        if (uiMapPointMovementFlags & CAN_I_WATER)
        {
            if (Can(CAN_C_SWIM, uiMyMovementFlags))
            {
                // I can swim, and water tiles have the impassable flag, so let's remove it
                uiMapPointMovementFlags &= ~CAN_I_BLOCK;
            }
            else
            {
                // Item is water and i can't swim
                uiMapPointMovementFlags |= CAN_I_BLOCK; // it should be already added in the tiledata, but we better make that sure
                uiBlockedBy |= CAN_I_WATER;
            }
        }
        if ((uiMapPointMovementFlags & CAN_I_PLATFORM) && !Can(CAN_C_WALK, uiMyMovementFlags))
        {
            // Item is walkable (land, not water) and i can't walk
            uiMapPointMovementFlags |= CAN_I_BLOCK;
            uiBlockedBy |= CAN_I_PLATFORM;
        }
        if ((uiMapPointMovementFlags & CAN_I_DOOR))
        {
            if (Can(CAN_C_GHOST, uiMyMovementFlags))
            {
                fPassTrough = true;
            }
            else
            {
                // Item is a door and i'm not a ghost
                uiMapPointMovementFlags |= CAN_I_BLOCK;
                uiBlockedBy |= CAN_I_DOOR;
            }
        }
        if ((uiMapPointMovementFlags & CAN_I_HOVER))
        {
            if (Can(CAN_C_HOVER, uiMyMovementFlags) || IsStatFlag(STATF_HOVERING))
            {
                ; //fPassTrough = true;
            }
            else
            {
                uiMapPointMovementFlags |= CAN_I_BLOCK;
                uiBlockedBy |= CAN_I_HOVER;
            }
        }

        if (!fPathfinding && (g_Cfg.m_iDebugFlags & DEBUGF_WALK))
            g_Log.EventWarn("block.m_Lowest.m_z %hhd, block.m_Bottom.m_z %hhd, block.m_Top.m_z %hhd.\n", blockingState->m_Lowest.m_z, blockingState->m_Bottom.m_z, blockingState->m_Top.m_z);

        if (blockingState->m_Bottom.m_z >= UO_SIZE_Z)
            return false;

        if (fLandTile)
        {
            // It's a land tile
            if ((uiMapPointMovementFlags & CAN_I_BLOCK) && !(uiBlockedBy & CAN_I_CLIMB))
                return false;
            if (blockingState->m_Bottom.m_z > ptDest->m_z + m_zClimbHeight + uiMyHeight + 3)
                return false;
        }
        else
        {
            // It's an item
            if (!fPassTrough && (uiMapPointMovementFlags & CAN_I_BLOCK))
            {
                // It's a blocking item. I should need special capabilities to pass through (or over) it.
                if (!(uiBlockedBy & CAN_I_CLIMB))
                    return false;
                if (!Can(CAN_C_PASSWALLS, uiMyMovementFlags))
                {
                    // I can't pass through it, but can i climb or fly on it?
                    if (Can(CAN_C_FLY, uiMyMovementFlags))
                    {
                        if (blockingState->m_Top.m_uiBlockFlags & CAN_I_ROOF)
                        {
                            // Roof tiles usually don't have the impassable/block tiledata flag, but i don't want flying chars to pass over the wall (bottom tile)
                            //  and through roof (top tile) and enter a building in this way
                            return false;
                        }
                    }
                    else if (uiMapPointMovementFlags & CAN_I_CLIMB)
                    {
                        // If dwBlockFlags & CAN_I_CLIMB, then it's a "climbable" item (and i can climb it, 
                        //  since i don't have CAN_I_CLIMB in uiBlockedBy)
                    }
                    else
                    {
                        // Standard check
                        // Keep in mind that m_z, when an item is encountered in the mapblockstate, is equal to the item z + its height
                        if (blockingState->m_Bottom.m_z > ptDest->m_z + m_zClimbHeight + 2) // Too high to climb.
                            return false;
                    }
                }
            }
        }
    }

    if (!fPathfinding && (g_Cfg.m_iDebugFlags & DEBUGF_WALK))
        g_Log.EventWarn("GetHeightMount() %hhu, block.m_Top.m_z %hhd, ptDest.m_z %hhd.\n", uiMyHeight, blockingState->m_Top.m_z, ptDest->m_z);

    if ((uiMyHeight + ptDest->m_z >= blockingState->m_Top.m_z) && g_Cfg.m_iMountHeight && !IsPriv(PRIV_GM) && !IsPriv(PRIV_ALLMOVE))
    {
        if (!fPathfinding)
            SysMessageDefault(DEFMSG_MSG_MOUNT_CEILING);
        return false;
    }

    // Be wary that now block.m_Bottom isn't just the "tile with lowest (p.z + height)", because if you are stepping on an item with height != 0,
    //  the bottom tile would always be the terrain, and the item above that (but at the same z) flags would be ignored.
    // We need to consider the item flags instead of the terrain's.
    // So, now block.m_Bottom is the item with lowest Z, ignoring the height, but block.m_Bottom.m_z is that item's (p.z + height).
    if (!fPassTrough)
    {
        // If i pass through a door and set our new Z to block.m_Bottom.m_z, we'll be on the top of the door
        ptDest->m_z = blockingState->m_Bottom.m_z;
    }

    return true;
}

CRegion *CChar::CheckValidMove( CPointMap &ptDest, uint64 *uiBlockFlags, DIR_TYPE dir, height_t *pClimbHeight, bool fPathFinding ) const
{
	ADDTOCALLSTACK("CChar::CheckValidMove");
	// Is it ok to move here ? is it blocked ?
	// ignore other characters for now.
	// RETURN:
	//  The new region we may be in.
	//  Fill in the proper ptDest.m_z value for this location. (if walking)
	//  pdwBlockFlags = what is blocking me. (can be null = don't care)

	//	test diagonal dirs by two others *only* when already having a normal location
    {
        const CPointMap ptOld(GetTopPoint());
        if (!fPathFinding && ptOld.IsValidPoint() && (dir % 2))
        {
            DIR_TYPE dirTest1 = (DIR_TYPE)(dir - 1); // get 1st ortogonal
            DIR_TYPE dirTest2 = (DIR_TYPE)(dir + 1); // get 2nd ortogonal
            if (dirTest2 == DIR_QTY)		// roll over
                dirTest2 = DIR_N;

            CPointMap ptTest(ptOld);
            ptTest.Move(dirTest1);
            if (!CheckValidMove(ptTest, uiBlockFlags, DIR_QTY, pClimbHeight))
                return nullptr;

            ptTest = ptOld;
            ptTest.Move(dirTest2);
            if (!CheckValidMove(ptTest, uiBlockFlags, DIR_QTY, pClimbHeight))
                return nullptr;
        }

        if (!ptDest.IsValidPoint())
        {
            if (!ptOld.IsValidPoint())
            {
                g_Log.EventWarn("Character with UID=0%x on invalid location %d,%d,%d,%d wants to move into another invalid location %d,%d,%d,%d. Is pathfinding check: %d\n",
                    GetUID().GetObjUID(), ptOld.m_x, ptOld.m_y, ptOld.m_z, ptOld.m_map, ptDest.m_x, ptDest.m_y, ptDest.m_z, ptDest.m_map, int(fPathFinding));
            }
            return nullptr;
        }
    }

    CRegion *pArea = ptDest.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA|REGION_TYPE_ROOM);
	if ( !pArea )
	{
		//if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
		g_Log.EventWarn("WalkCheck: failed to get the destination region at P=%d,%d,%d,%d (UID: 0%x, name: %s).\n",
            ptDest.m_x, ptDest.m_y, ptDest.m_z, ptDest.m_map, (dword)GetUID(), GetName());
		return nullptr;
	}

    // Now check if i can stand on that tile.

	const uint64 uiCanFlags = GetCanFlags();
	const uint64 uiMovementCan = GetCanMoveFlags(uiCanFlags);  // actions i can perform to step on a tile (some tiles require a specific ability, like to swim for the water)

	if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
		g_Log.EventWarn("GetCanMoveFlags() (0x%" PRIx64 ").\n", uiMovementCan);
	if (!(uiMovementCan & CAN_C_MOVEMENTCAPABLEMASK))
		return nullptr;	// cannot move at all, so WTF?


    const height_t uiHeight = IsSetEF(EF_WalkCheckHeightMounted) ? GetHeightMount() : GetHeight();
    CServerMapBlockingState blockingState(uiMovementCan, ptDest.m_z, ptDest.m_z + m_zClimbHeight + uiHeight, ptDest.m_z + m_zClimbHeight + 3, uiHeight);
    if (g_Cfg.m_iDebugFlags & DEBUGF_WALK)
    {
        g_Log.EventWarn("\t\tCServerMapBlockState block( 0%" PRIx64 ", %d, %d, %d ); ptDest.m_z(%d) m_zClimbHeight(%d).\n",
            uiMovementCan, ptDest.m_z, ptDest.m_z + m_zClimbHeight + uiHeight, ptDest.m_z + m_zClimbHeight + 2, ptDest.m_z, m_zClimbHeight);
    }

    const bool fCanStand = CanStandAt(&ptDest, pArea, uiMovementCan, uiHeight, &blockingState, fPathFinding);
    if (!fCanStand)
        return nullptr;

    if (uiBlockFlags)
        *uiBlockFlags = blockingState.m_Bottom.m_uiBlockFlags;
    if (pClimbHeight && (blockingState.m_Bottom.m_uiBlockFlags & CAN_I_CLIMB))
        *pClimbHeight = blockingState.m_zClimbHeight;

	return pArea;
}

void CChar::FixClimbHeight()
{
	ADDTOCALLSTACK("CChar::FixClimbHeight");
	const CPointMap& pt = GetTopPoint();
    const height_t iHeightMount = GetHeightMount();
	CServerMapBlockingState block(CAN_I_CLIMB, pt.m_z, pt.m_z + iHeightMount + 3, pt.m_z + 2, iHeightMount);
	CWorldMap::GetHeightPoint(pt, block, true);

	if ( (block.m_Bottom.m_z == pt.m_z) && (block.m_uiBlockFlags & CAN_I_CLIMB) )	// we are standing on stairs
		m_zClimbHeight = block.m_zClimbHeight;
	else
		m_zClimbHeight = 0;
	m_fClimbUpdated = true;
}
