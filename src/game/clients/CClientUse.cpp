
#include "../../common/resource/CResourceLock.h"
#include "../../common/CLog.h"
#include "../../network/send.h"
#include "../chars/CChar.h"
#include "../items/CItemMap.h"
#include "../items/CItemShip.h"
#include "../components/CCSpawn.h"
#include "../CWorldMap.h"
#include "../CWorldSearch.h"
#include "../triggers.h"
#include "CClient.h"

bool CClient::Cmd_Use_Item( CItem *pItem, bool fTestTouch, bool fScript )
{
	ADDTOCALLSTACK("CClient::Cmd_Use_Item");
	// Assume we can see the target.
	// called from Event_DoubleClick

	if ( !pItem )
		return false;

	if ( pItem->Can( CAN_I_FORCEDC ) )
		fTestTouch = false;

	const CObjBaseTemplate *pObjTop = pItem->GetTopLevelObj();

	if ( fTestTouch )
	{
		if ( !fScript )
		{
			CItemContainer *pContainer = dynamic_cast<CItemContainer *>(pItem->GetParent());
			if ( pContainer )
			{
				// protect from ,snoop - disallow picking from not opened containers
				CItemContainer* pTopContainer = dynamic_cast<CItemContainer*>(pItem->GetTopContainer());
				bool isInOpenedContainer = false;
				if ( pContainer->IsType(IT_EQ_TRADE_WINDOW) )
				{
					isInOpenedContainer = true;
				}
				else if (pTopContainer && (pTopContainer->IsType(IT_EQ_VENDOR_BOX) || pTopContainer->IsType(IT_EQ_BANK_BOX)))
				{
					if ( pTopContainer->m_itEqBankBox.m_pntOpen == GetChar()->GetTopPoint() )
						isInOpenedContainer = true;
				}
				else
				{
					auto itContainerFound = m_openedContainers.find(pContainer->GetUID().GetPrivateUID());
					if ( itContainerFound != m_openedContainers.cend() )
					{
						dword dwTopContainerUID = ((itContainerFound->second).first).first;
						dword dwTopMostContainerUID = ((itContainerFound->second).first).second;
						CPointMap ptOpenedContainerPosition = (itContainerFound->second).second;

						dword dwTopContainerUID_ToCheck = 0;
						if ( pContainer->GetContainer() )
							dwTopContainerUID_ToCheck = pContainer->GetContainer()->GetUID().GetPrivateUID();
						else
							dwTopContainerUID_ToCheck = pObjTop->GetUID().GetPrivateUID();

						if ( (dwTopMostContainerUID == pObjTop->GetUID().GetPrivateUID()) && (dwTopContainerUID == dwTopContainerUID_ToCheck) )
						{
							if ( pObjTop->IsChar() )
							{
								// probably a pickup check from pack if pCharTop != this?
								isInOpenedContainer = true;
							}
							else
							{
								const CItem *pItemTop = static_cast<const CItem *>(pObjTop);
								if ( pItemTop && (pItemTop->IsType(IT_SHIP_HOLD) || pItemTop->IsType(IT_SHIP_HOLD_LOCK)) && (pItemTop->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == m_pChar->GetTopPoint().GetRegion(REGION_TYPE_MULTI)) )
									isInOpenedContainer = true;
								else if ( ptOpenedContainerPosition.GetDist(pObjTop->GetTopPoint()) <= 3 )
									isInOpenedContainer = true;
							}
						}
					}
				}

				if ( !isInOpenedContainer )
				{
					SysMessageDefault(DEFMSG_REACH_UNABLE);
					return false;
				}
			}
		}

		// CanTouch handles priv level compares for chars
		if ( !m_pChar->CanUse(pItem, false) )
		{
			if ( !m_pChar->CanTouch(pItem) )
				SysMessage((m_pChar->IsStatFlag(STATF_DEAD)) ? g_Cfg.GetDefaultMsg(DEFMSG_REACH_GHOST) : g_Cfg.GetDefaultMsg(DEFMSG_REACH_FAIL));
			else
				SysMessageDefault(DEFMSG_REACH_UNABLE);

			return false;
		}
	}

	if ( IsTrigUsed(TRIGGER_DCLICK) || IsTrigUsed(TRIGGER_ITEMDCLICK) )
	{
		if ( pItem->OnTrigger(ITRIG_DCLICK, m_pChar) == TRIGRET_RET_TRUE )
			return true;
	}

	CItemBase *pItemDef = pItem->Item_GetDef();
	bool bIsEquipped = pItem->IsItemEquipped();
	if (pItemDef->IsTypeEquippable() && !bIsEquipped && pItemDef->GetEquipLayer())
	{
		bool fMustEquip = true;
		if (pItem->IsTypeSpellbook())
			fMustEquip = false;
		else if ((pItem->IsType(IT_LIGHT_OUT) || pItem->IsType(IT_LIGHT_LIT)) && !pItem->IsItemInContainer())
			fMustEquip = false;

		if (fMustEquip)
		{
			if (!m_pChar->CanMoveItem(pItem))
				return false;
			/*Before weight behavior rework we had this check too :
			if ( (pObjTop != m_pChar) && !m_pChar->CanCarry(pItem) )
			{
				SysMessageDefault(DEFMSG_MSG_HEAVY);
				return false;
			}*/
			if (!m_pChar->ItemEquip(pItem, nullptr, true))
				return false;
		}
	}

	CCSpawn *pSpawn = pItem->GetSpawn();	// remove this item from its spawn when players DClick it from ground, no other way to take it out.
	if ( pSpawn )
		pSpawn->DelObj(pItem->GetUID());

	SetTargMode();
	m_Targ_Prv_UID = m_Targ_UID;
	m_Targ_UID = pItem->GetUID();
	m_tmUseItem.m_pParent = pItem->GetParent();		// store item location to check later if it was not moved

	// Use types of items (specific to client)
	switch ( pItem->GetType() )
	{
		case IT_TRACKER:
		{
			DIR_TYPE dir = static_cast<DIR_TYPE>(DIR_QTY + 1); // invalid value.
			if ( !m_pChar->Skill_Tracking(pItem->m_uidLink, dir) )
			{
				if ( pItem->m_uidLink.IsValidUID() )
					SysMessageDefault(DEFMSG_TRACKING_UNABLE);
				addTarget(CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_TRACKER_ATTUNE));
			}
			return true;
		}

		case IT_SHAFT:
		case IT_FEATHER:
			return Skill_Menu(SKILL_BOWCRAFT, "sm_bolts", pItem->GetID());

		case IT_FISH_POLE:	// Just be near water ?
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_FISHING_PROMT), true);
			return true;

		case IT_DEED:
			addTargetDeed(pItem);
			return true;

		case IT_EQ_BANK_BOX:
		case IT_EQ_VENDOR_BOX:
			if ( !fScript )
				g_Log.Event(LOGL_WARN|LOGM_CHEAT, "%x:Cheater '%s' is using 3rd party tools to open bank box\n", GetSocketID(), GetAccount()->GetName());
			return false;

		case IT_CONTAINER_LOCKED:
			SysMessageDefault(DEFMSG_ITEMUSE_LOCKED);
			if (!m_pChar->GetPackSafe()->ContentFindKeyFor(pItem)) // I don't have the container key
			{
				SysMessageDefault(DEFMSG_ITEMUSE_LOCKED);
				SysMessageDefault(DEFMSG_LOCK_CONT_NO_KEY);
				if (!IsPriv(PRIV_GM))
					return false;
			}
			else // I have the key but i need to use it to unlock the container.
			{
				SysMessageDefault(DEFMSG_LOCK_HAS_KEY); 
				return false;
			}
			break;

		case IT_SHIP_HOLD_LOCK:
			SysMessageDefault(DEFMSG_ITEMUSE_LOCKED);
			if ( !m_pChar->GetPackSafe()->ContentFindKeyFor(pItem) ) // I don't have the hold key
			{
				
				SysMessageDefault(DEFMSG_LOCK_HOLD_NO_KEY);
				if ( !IsPriv(PRIV_GM) )
					return false;
			}
			else // I have the key but i need to use it to unlock the container.
			{
				SysMessageDefault(DEFMSG_LOCK_HAS_KEY);
				return false;
			}
			break;

		case IT_CORPSE:
		case IT_SHIP_HOLD:
		case IT_CONTAINER:
		case IT_TRASH_CAN:
		{
			CItemContainer *pPack = static_cast<CItemContainer *>(pItem);

			if ( !m_pChar->Skill_Snoop_Check(pPack) )
			{
				if ( !addContainerSetup(pPack) )
					return false;
			}

            if (pItem->GetType() == IT_CORPSE)
            {
                CItemCorpse *pCorpseItem = static_cast<CItemCorpse *>(pPack);
                if ( m_pChar->CheckCorpseCrime(pCorpseItem, true, true) )
                    SysMessageDefault(DEFMSG_LOOT_CRIMINAL_ACT);
            }
			
			return true;
		}

		case IT_GAME_BOARD:
		{
			if ( !pItem->IsTopLevel() )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_GAMEBOARD_FAIL);
				return false;
			}
			CItemContainer *pBoard = static_cast<CItemContainer *>(pItem);
			ASSERT(pBoard);
			pBoard->Game_Create();
			addContainerSetup(pBoard);
			return true;
		}

		case IT_BBOARD:
			addBulletinBoard(static_cast<CItemContainer *>(pItem));
			return true;

		case IT_SIGN_GUMP:
		{
			GUMP_TYPE gumpid = pItemDef->m_ttContainer.m_idGump;
			if ( !gumpid )
				return false;
			addGumpTextDisp(pItem, gumpid, pItem->GetName(), pItem->IsIndividualName() ? pItem->GetName() : nullptr);
			return true;
		}

		case IT_BOOK:
		case IT_MESSAGE:
		{
			if ( !addBookOpen(pItem) )
				SysMessageDefault(DEFMSG_ITEMUSE_BOOK_FAIL);
			return true;
		}

		case IT_STONE_GUILD:
		case IT_STONE_TOWN:
			return true;

		case IT_POTION:
		{
			if ( !m_pChar->CanMoveItem(pItem) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_POTION_FAIL);
				return false;
			}
			if ( ResGetIndex(pItem->m_itPotion.m_Type) == SPELL_Explosion )
			{
				// Throw explosion potion
				if ( m_pChar->ItemPickup(pItem, 1) == -1 )	// put the potion in our hand
					return false;

				if (pItem->m_itPotion.m_tick <= 0) //Set default countdown for explosion if morex is not  set.
					pItem->m_itPotion.m_tick = 4;
				pItem->m_itPotion.m_ignited = 1;	// ignite it
				pItem->m_uidLink = m_pChar->GetUID();
				pItem->SetTimeoutS(1);
				m_tmUseItem.m_pParent = pItem->GetParent();
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_SELECT_POTION_TARGET), true, true, pItem->m_itPotion.m_tick * MSECS_PER_SEC);
				return true;
			}
			m_pChar->Use_Drink(pItem);
			return true;
		}

		case IT_ANIM_ACTIVE:
			SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_ITEM_IN_USE));
			return false;

		case IT_CLOCK:
			addObjMessage(m_pChar->GetTopSector()->GetLocalGameTime(), pItem);
			return true;

		case IT_SPAWN_ITEM:
		case IT_SPAWN_CHAR:
		{
			pSpawn = dynamic_cast<CCSpawn*>(pItem->GetComponent(COMP_SPAWN));
			if ( !pSpawn )
				return false;

			if ( pSpawn->GetCurrentSpawned() > 0 )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_SPAWN_NEG);
				pSpawn->KillChildren();		// Removing existing objects spawned from it ( RESET ).
			}
			else
			{
				SysMessageDefault(DEFMSG_ITEMUSE_SPAWN_RESET);
				pSpawn->OnTickComponent();		// Forcing the spawn to work and create some objects ( START ).
			}
			return true;
		}

		case IT_SHRINE:
		{
			if ( m_pChar->OnSpellEffect(SPELL_Resurrection, m_pChar, 1000, pItem) )
				return true;
			SysMessageDefault(DEFMSG_ITEMUSE_SHRINE);
			return true;
		}

		case IT_SHIP_TILLER:
		{
			if (m_net->isClientVersionNumber(MINCLIVER_HS))
			{
				CItemShip* pShip = dynamic_cast<CItemShip*>(pItem->m_uidLink.ItemFind());
				if (pShip)
				{
					if (m_pChar->ContentFindKeyFor(pItem) || pShip->GetOwner() == m_pChar->GetUID())
						pShip->CCMultiMovable::SetPilot(m_pChar);
					else
						pItem->Speak(g_Cfg.GetDefaultMsg(DEFMSG_TILLER_NOTYOURSHIP));
					return true;
				}
			}
			pItem->Speak(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_TILLERMAN), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL);
			return true;
		}
		case IT_WAND:
		case IT_SCROLL:
		{
			const SPELL_TYPE spell = (SPELL_TYPE)(ResGetIndex(pItem->m_itWeapon.m_spell));
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
			if ( !pSpellDef )
				return false;

			if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
			{
				int skill;
				if ( !pSpellDef->GetPrimarySkill(&skill, nullptr) )
					return false;

				m_tmSkillMagery.m_iSpell = spell;	// m_atMagery.m_iSpell
				m_pChar->m_atMagery.m_iSpell = spell;
				m_pChar->Skill_Start((SKILL_TYPE)skill);
				return true;
			}
			return Cmd_Skill_Magery(spell, pItem);
		}

		case IT_RUNE:
		{
			if ( !m_pChar->CanMoveItem(pItem, true) )
				return false;
			addPromptConsole(CLIMODE_PROMPT_NAME_RUNE, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_RUNE_NAME), pItem->GetUID());
			return true;
		}

		case IT_CARPENTRY:
			return Skill_Menu(SKILL_CARPENTRY, "sm_carpentry", pItem->GetID());

		case IT_FORGE:
			// Solve for the combination of this item with another.
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_FORGE));
			return true;

		case IT_ORE:
			return m_pChar->Skill_Mining_Smelt(pItem, nullptr);

		case IT_INGOT:
			return Cmd_Skill_Smith(pItem);

		case IT_KEY:
		case IT_KEYRING:
		{
			if ( pItem->GetTopLevelObj() != m_pChar && !m_pChar->IsPriv(PRIV_GM) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_KEY_FAIL);
				return false;
			}
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_KEY_PROMT), false, true);
			return true;
		}

		case IT_BANDAGE:		// SKILL_HEALING, or SKILL_VETERINARY
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_BANDAGE_PROMT), false, false);
			return true;

		case IT_BANDAGE_BLOOD:	// Clean the bandages.
		case IT_COTTON:			// use on a spinning wheel.
		case IT_WOOL:			// use on a spinning wheel.
		case IT_YARN:			// Use this on a loom.
		case IT_THREAD: 		// Use this on a loom.
		case IT_COMM_CRYSTAL:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_TARGET_PROMT), false, false);
			return true;

		case IT_CARPENTRY_CHOP:
		case IT_LOCKPICK:		// Use on a locked thing.
		case IT_SCISSORS:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_TARGET_PROMT), false, true);
			return true;

		case IT_WEAPON_MACE_PICK:
			if ( bIsEquipped || !IsSetOF(OF_NoDClickTarget) )
			{
				// Mine at the location
				tchar *pszTemp = Str_GetTemp();
				snprintf(pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_MACEPICK_TARG), pItem->GetName());
				addTarget(CLIMODE_TARG_USE_ITEM, pszTemp, true, true);
			}
			return true;

		case IT_WEAPON_SWORD:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_AXE:
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_SMITH:
		{
			if ( bIsEquipped || !IsSetOF(OF_NoDClickTarget) )
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_WEAPON_PROMT), false, true);
			return true;
		}

		case IT_WEAPON_MACE_CROOK:
			if ( bIsEquipped || !IsSetOF(OF_NoDClickTarget) )
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CROOK_PROMT), false, true);
			return true;

		case IT_FISH:
			SysMessageDefault(DEFMSG_ITEMUSE_FISH_FAIL);
			return true;

		case IT_TELESCOPE:
			SysMessageDefault(DEFMSG_ITEMUSE_TELESCOPE);
			return true;

		case IT_MAP:
			addDrawMap(static_cast<CItemMap *>(pItem));
			return true;

		case IT_CANNON_BALL:
		{
			tchar *pszTemp = Str_GetTemp();
			snprintf(pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CBALL_PROMT), pItem->GetName());
			addTarget(CLIMODE_TARG_USE_ITEM, pszTemp);
			return true;
		}

		case IT_CANNON_MUZZLE:
		{
			if ( !m_pChar->CanUse(pItem, false) )
				return false;

			// Make sure the cannon is loaded.
			if ( !(pItem->m_itCannon.m_Load & 1) )
			{
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CANNON_POWDER));
				return true;
			}
			if ( !(pItem->m_itCannon.m_Load & 2) )
			{
				addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CANNON_SHOT));
				return true;
			}
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CANNON_TARG), false, true);
			return true;
		}

		case IT_CRYSTAL_BALL:
			// Gaze into the crystal ball.
			return true;

		case IT_SEED:
		case IT_PITCHER_EMPTY:
		{
			tchar *pszTemp = Str_GetTemp();
			snprintf(pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_PITCHER_TARG), pItem->GetName());
			addTarget(CLIMODE_TARG_USE_ITEM, pszTemp, true);
			return true;
		}

		case IT_SPELLBOOK:
		case IT_SPELLBOOK_NECRO:
		case IT_SPELLBOOK_PALA:
		case IT_SPELLBOOK_BUSHIDO:
		case IT_SPELLBOOK_NINJITSU:
		case IT_SPELLBOOK_ARCANIST:
		case IT_SPELLBOOK_MYSTIC:
		case IT_SPELLBOOK_MASTERY:
        {
            //addItem(m_pChar->GetPackSafe());
            addSpellbookOpen(pItem);
            return true;
        }

		case IT_HAIR_DYE:
		{
			if ( !m_pChar->LayerFind(LAYER_BEARD) && !m_pChar->LayerFind(LAYER_HAIR) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_DYE_NOHAIR);
				return true;
			}
			Dialog_Setup(CLIMODE_DIALOG, g_Cfg.ResourceGetIDType(RES_DIALOG, "d_hair_dye"), 0, pItem);
			return true;
		}

		case IT_DYE:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_DYE_VAT));
			return true;

		case IT_DYE_VAT:
			addTarget(CLIMODE_TARG_USE_ITEM, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_DYE_TARG), false, true);
			return true;

		case IT_MORTAR:
			return Skill_Menu(SKILL_ALCHEMY, "sm_alchemy", pItem->GetID());

		case IT_CARTOGRAPHY:
			return Skill_Menu(SKILL_CARTOGRAPHY, "sm_cartography", pItem->GetID());

		case IT_COOKING:
			return Skill_Menu(SKILL_COOKING, "sm_cooking", pItem->GetID());

		case IT_TINKER_TOOLS:
			return Skill_Menu(SKILL_TINKERING, "sm_tinker", pItem->GetID());

		case IT_SEWING_KIT:
		{
			tchar *pszTemp = Str_GetTemp();
			snprintf(pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SEWKIT_PROMT), pItem->GetName());
			addTarget(CLIMODE_TARG_USE_ITEM, pszTemp);
			return true;
		}

		case IT_SCROLL_BLANK:
			Cmd_Skill_Inscription();
			return true;

		default:
		{
			// An NPC could use it this way.
			if ( m_pChar->Use_Item(pItem) )
				return true;
			break;
		}
	}

	SysMessageDefault(DEFMSG_ITEMUSE_CANTTHINK);
	return false;
}

void CClient::Cmd_EditItem( CObjBase *pObj, int iSelect )
{
	ADDTOCALLSTACK("CClient::Cmd_EditItem");
	// ARGS:
	//   iSelect == -1 = setup.
	//   m_Targ_Text = what are we doing to it ?
	//
	if ( !pObj )
		return;

	CContainer *pContainer = dynamic_cast<CContainer *>(pObj);
	if ( !pContainer )
	{
		addGumpDialogProps(pObj->GetUID());
		return;
	}

	if ( iSelect == 0 )		// cancelled
		return;

    ASSERT(ARRAY_COUNT(m_tmMenu.m_Item) == MAX_MENU_ITEMS);
	if ( iSelect > 0 )		// we selected an item
	{
		if ( iSelect >= MAX_MENU_ITEMS )
			return;

		if ( m_Targ_Text.IsEmpty() )
			addGumpDialogProps(CUID(m_tmMenu.m_Item[iSelect]));
		else
			OnTarg_Obj_Set( CUID::ObjFindFromUID(m_tmMenu.m_Item[iSelect]) );
		return;
	}

	CMenuItem item[MAX_MENU_ITEMS];	    // Most as we want to display at one time.
	item[0].m_sText.Format("Contents of %s", pObj->GetName());

	uint count = 0;
	for (const CSObjContRec* pObjRec : *pContainer)
	{
		const CItem* pItem = static_cast<const CItem*>(pObjRec);
		++count;
		m_tmMenu.m_Item[count] = pItem->GetUID();
		item[count].m_sText = pItem->GetName();
		ITEMID_TYPE idi = pItem->GetDispID();
		item[count].m_id = (word)(idi);
		item[count].m_color = 0;

		if ( !pItem->IsType(IT_EQ_MEMORY_OBJ) )
		{
			HUE_TYPE wHue = pItem->GetHue();
			if ( wHue != 0 )
			{
				wHue = (wHue == 1 ? 0x7FF : wHue - 1);
				item[count].m_color = wHue;
			}
		}

		if ( count >= (ARRAY_COUNT(item) - 1) )
			break;
	}

	ASSERT(count < ARRAY_COUNT(item));
	addItemMenu(CLIMODE_MENU_EDIT, item, count, pObj);
}


bool CClient::Skill_Menu(SKILL_TYPE skill, lpctstr skillmenu, ITEMID_TYPE itemused)
{
	// Default menu is d_craft_menu
	// Open in page 0, args is skill used.
	// LPCTSTR dSkillMenu = "d_CraftingMenu";
	CScriptTriggerArgs Args;
	Args.m_VarsLocal.SetStrNew("SkillMenu", skillmenu);
	Args.m_VarsLocal.SetNumNew("Skill", skill);
	Args.m_VarsLocal.SetNumNew("ItemUsed", itemused);
	if (IsTrigUsed(TRIGGER_SKILLMENU))
	{
		if (m_pChar->OnTrigger(CTRIG_SkillMenu, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return true;

		skillmenu = Args.m_VarsLocal.GetKeyStr("Skillmenu", false);
	}

	lpctstr SkillUsed = g_Cfg.GetSkillKey(skill);
	CResourceID ridDialog = g_Cfg.ResourceGetIDType(RES_DIALOG, skillmenu);
	if (ridDialog.IsValidUID()) {
		return Dialog_Setup(CLIMODE_DIALOG, g_Cfg.ResourceGetIDType(RES_DIALOG, skillmenu), 0, m_pChar, SkillUsed);
	} else {
		CResourceID ridMenu = g_Cfg.ResourceGetIDType(RES_SKILLMENU, skillmenu);
		if (ridMenu.IsValidUID()) {
			return Cmd_Skill_Menu(ridMenu);
		} else {
			g_Log.EventError("CClient::Skill_Menu - Not valid dialog or skillmenu %s \n", skillmenu);
		}
	}

	return false;
}

bool CClient::Cmd_Skill_Menu( const CResourceID& rid, int iSelect )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Menu");
	// Build the skill menu for the curent active skill.
	// Only list the things we have skill and ingrediants to make.
	//
	// ARGS:
	//	m_Targ_UID = the object used to get us here.
	//  rid = which menu ?
	//	iSelect = -2 = Just a test of the whole menu.,
	//	iSelect = -1 = 1st setup.
	//	iSelect = 0 = cancel
	//	iSelect = x = execute the selection.
	//
	// RETURN: false = fail/cancel the skill.
	//   m_tmMenu.m_Item = the menu entries.

	ASSERT(m_pChar);
	if ( rid.GetResType() != RES_SKILLMENU )
    {
        DEBUG_ERR(("Invalid RES_SKILLMENU (error type=1).\n"));
		return false;
    }

	bool fShowMenu = false;
	bool fLimitReached = false;

	if ( iSelect == 0 )		// menu cancelled
		return (Cmd_Skill_Menu_Build(rid, iSelect, nullptr, 0, &fShowMenu, &fLimitReached) > 0);

    ASSERT(ARRAY_COUNT(m_tmMenu.m_Item) == MAX_MENU_ITEMS);
	CMenuItem item[MAX_MENU_ITEMS];
	int iShowCount = Cmd_Skill_Menu_Build(rid, iSelect, item, MAX_MENU_ITEMS, &fShowMenu, &fLimitReached);

	if ( iSelect < -1 )		// just a test
		return iShowCount ? true : false;

	if ( iSelect > 0 )		// seems our resources disappeared.
	{
		if ( iShowCount <= 0 )
			SysMessageDefault(DEFMSG_CANT_MAKE);
		return iShowCount > 0 ? true : false;
	}

	if ( iShowCount <= 0 )
	{
		SysMessageDefault(DEFMSG_CANT_MAKE_RES);
		return false;
	}

	if ( (iShowCount == 1) && fShowMenu && !fLimitReached )
	{
		static int sm_iReentrant = 0;
		if ( sm_iReentrant < 12 )
		{
			++sm_iReentrant;

			// If there is just one menu then select it.
			bool fSuccess = Cmd_Skill_Menu(rid, m_tmMenu.m_Item[1]); // first entry

			--sm_iReentrant;
			return fSuccess;
		}

		if ( g_Cfg.m_iDebugFlags & DEBUGF_SCRIPTS )
			g_Log.EventDebug("[DEBUG_SCRIPTS] Too many empty skill menus to continue seeking through menu '%s'\n",
				g_Cfg.RegisteredResourceGetDef(rid)->GetResourceName());
	}

	ASSERT(iShowCount < (int)ARRAY_COUNT(item));
	addItemMenu(CLIMODE_MENU_SKILL, item, iShowCount);
	return true;
}

int CClient::Cmd_Skill_Menu_Build( const CResourceID& rid, int iSelect, CMenuItem * item, int iMaxSize, bool *fShowMenu, bool *fLimitReached )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Menu_Build");
	// Build the skill menu for the curent active skill.
	// Only list the things we have skill and ingrediants to make.
	//
	// ARGS:
	//	m_Targ_UID = the object used to get us here.
	//  rid = which menu ?
	//	iSelect = -2 = Just a test of the whole menu.,
	//	iSelect = -1 = 1st setup.
	//	iSelect = 0 = cancel
	//	iSelect = x = execute the selection.
	//  fShowMenu = whether or not menus can be shown
	//  item = pointer to entries list
	//  iMaxSize = maximum number of entries
	//
	// RETURN: number of entries in menu
	//  m_tmMenu.m_Item[shownIndex] = ON= index
    //  Index 0 is a special index, used only to store the top text on the skillmenu.

	ASSERT(m_pChar);
	if ( rid.GetResType() != RES_SKILLMENU )
		return 0;

	// Find section.
	CResourceLock s;
	if ( !g_Cfg.ResourceLock(s, rid) )
    {
        DEBUG_ERR(("Invalid RES_SKILLMENU (error type=2).\n"));
		return 0;
    }

	// Get title line
	if ( !s.ReadKey() )
		return 0;

	if ( iSelect == 0 )		// cancelled
	{
		while ( s.ReadKeyParse() )
		{
			if ( !s.IsKey("ON") || (*s.GetArgStr() != '@') )
				continue;

			if ( strcmpi(s.GetArgStr(), "@Cancel") )
				continue;

			if ( m_pChar->OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, m_pChar, nullptr) == TRIGRET_RET_TRUE )
				return 0;

			break;
		}
		return 1;
	}

    if ( iSelect == -1 )
    {
        ASSERT(item != nullptr);
        item[0].m_sText = s.GetKey();   // Set the title
		m_tmMenu.m_ResourceID = rid;
    }

    bool fSkip = false;		// skip this if we lack resources or skill.
    bool fSkipNeedCleanup = false;
	int iOnCount = 0;
	int iShowCount = 0;
	CScriptTriggerArgs Args;

	while ( s.ReadKeyParse() )
	{
        if (fSkipNeedCleanup)
        {
            fSkip = true;
            fSkipNeedCleanup = false;
            if (iSelect != -2)
            {
                ASSERT(item != nullptr);
                item[iShowCount] = {};
                m_tmMenu.m_Item[iShowCount] = 0;
            }
            --iShowCount;
        }
		if ( s.IsKeyHead("ON", 2) )
		{
			if ( !strcmpi(s.GetArgStr(), "@Cancel") )
			{
				fSkip = true;
				continue;
			}
			else if ( *s.GetArgStr() == '@' )
            {
                ++iShowCount;
                fSkip = true;
				continue;
            }

			// a new option to look at.
            fSkip = false;
			++iOnCount;

			if ( iSelect < 0 )	// building up the list.
			{
				if ( (iSelect < -1) && (iShowCount >= 1) )		// just a test. so we are done.
					return 1;
				
                CMenuItem miTest;
				if ( !miTest.ParseLine(s.GetArgRaw(), nullptr, m_pChar) )
				{
					// remove if the item is invalid.
                    fSkip = true;
					continue;
				}
                ++iShowCount;
                // I have increased iShowCount, now i need fSkipNeedCleanup

				if ( iSelect == -1 )
                {
                    // If iSelect == -2 "item" is not an array! And we don't even need to set a value, since with -2 it would just be a test!
                    ASSERT(item != nullptr);
                    item[iShowCount] = miTest;
					m_tmMenu.m_Item[iShowCount] = (dword)iOnCount;
                    ASSERT(m_tmMenu.m_Item[iShowCount] < INT32_MAX);    // iOnCount is int but m_tmMenu.m_Item[x] is uint, don't overflow
                }

				if ( iShowCount >= (iMaxSize - 1) )
					break;
			}
			else
			{
				if ( iOnCount > iSelect )	// we are done.
					break;
			}
			continue;
		}

        if ( fSkip )	// we have decided we cant do the option indicated by the previous (ON, TEST, TESTIF, MAKEITEM, SKILLMENU...) line.
            continue;
		if ( (iSelect > 0) && (iOnCount != iSelect) )	// only interested in the selected option
			continue;

		// Check for a skill / non-consumables required.
		if ( s.IsKey("TEST") )
		{
			m_pChar->ParseScriptText(s.GetArgRaw(), m_pChar);
			CResourceQtyArray skills(s.GetArgStr());
			if ( !skills.IsResourceMatchAll(m_pChar) )
			{
                fSkipNeedCleanup = true;
			}
			continue;
		}

		if ( s.IsKey("TESTIF") )
		{
			m_pChar->ParseScriptText(s.GetArgRaw(), m_pChar);
			if ( !s.GetArgVal() )
			{
                fSkipNeedCleanup = true;
			}
			continue;
		}

		// select to execute any entries here ?
		if ( iOnCount == iSelect )
		{
			// Execute command from script
			TRIGRET_TYPE tRet = m_pChar->OnTriggerRunVal(s, TRIGRUN_SINGLE_EXEC, m_pChar, &Args);
			if ( tRet != TRIGRET_RET_DEFAULT )
				return (tRet == TRIGRET_RET_TRUE) ? 0 : 1;

			++iShowCount;	// we are good. but continue til the end
		}
		else
		{
			ASSERT(iSelect < 0);
			if ( s.IsKey("SKILLMENU") )
			{
				static int sm_iReentrant = 0;
				if ( sm_iReentrant > 1024 )
				{
					if ( g_Cfg.m_iDebugFlags & DEBUGF_SCRIPTS )
					{
						g_Log.EventDebug("[DEBUG_SCRIPTS] Too many skill menus (circular menus?) to continue searching in menu '%s'\n",
							g_Cfg.RegisteredResourceGetDef(rid)->GetResourceName());
					}
					*fLimitReached = true;
				}
				else
				{
					// Test if there is anything in this skillmenu we can do.
					++sm_iReentrant;
					if ( !Cmd_Skill_Menu_Build(g_Cfg.ResourceGetIDType(RES_SKILLMENU, s.GetArgStr()), -2, nullptr, iMaxSize, fShowMenu, fLimitReached) )
					{
                        fSkipNeedCleanup = true;
					}
					else
                    {
			            *fShowMenu = true;
                    }
					--sm_iReentrant;
				}
				continue;
			}

			if ( s.IsKey("MAKEITEM") )
			{
				// test if i can make this item using m_Targ_UID.
				// There should ALWAYS be a valid id here.
				if ( !m_pChar->Skill_MakeItem((ITEMID_TYPE)(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, s.GetArgStr())), m_Targ_UID, SKTRIG_SELECT) )
				{
                    fSkipNeedCleanup = true;
				}
				continue;
			}
		}
	}
    if (fSkipNeedCleanup)
    {
        if (iSelect != -2)
        {
            ASSERT(item != nullptr);
            item[iShowCount] = {};
            m_tmMenu.m_Item[iShowCount] = 0;
        }
        --iShowCount;
    }
	return iShowCount;
}

bool CClient::Cmd_Skill_Magery( SPELL_TYPE iSpell, CObjBase *pSrc )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Magery");
	// Start casting a spell. Prompt for target.
	// ARGS:
	//	pSrc = you the char.
	//	pSrc = magic object is source ?

	ASSERT(m_pChar);

	const CSpellDef *pSpellDef;
	if ( IsSetMagicFlags(MAGICF_PRECAST) && iSpell == m_tmSkillMagery.m_iSpell )
	{
		pSpellDef = g_Cfg.GetSpellDef(m_tmSkillMagery.m_iSpell);
		if ( pSpellDef != nullptr && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
			iSpell = m_tmSkillMagery.m_iSpell;
	}
	else
		pSpellDef = g_Cfg.GetSpellDef(iSpell);

	if ( !pSpellDef )
		return false;

	// Do we have the regs? Etc.
	if ( !m_pChar->Spell_CanCast(iSpell, true, pSrc, true) )
		return false;

	SetTargMode();
	m_tmSkillMagery.m_iSpell = iSpell;	// m_atMagery.m_iSpell
	m_Targ_UID = m_pChar->GetUID();		// Default target.
	m_Targ_Prv_UID = pSrc->GetUID();		// Source of the spell.

	switch ( iSpell )
	{
		case SPELL_Polymorph:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_polymorph");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_polymorph"));
		}

		case SPELL_Summon:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_summon");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_summon"));
		}

		case SPELL_Summon_Familiar:
		{
			if ( IsTrigUsed(TRIGGER_SKILLMENU) )
			{
				CScriptTriggerArgs args("sm_summon_familiar");
				if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
					return true;
			}
			return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_summon_familiar"));
		}

		default:
			break;
	}

	// Targeted spells
	if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ|SPELLFLAG_TARG_XYZ) )
	{
		lpctstr pPrompt = g_Cfg.GetDefaultMsg(DEFMSG_SELECT_MAGIC_TARGET);
		if ( !pSpellDef->m_sTargetPrompt.IsEmpty() )
			pPrompt = pSpellDef->m_sTargetPrompt;

		int64 SpellTimeout = g_Cfg.m_iSpellTimeout;
		if ( m_pChar->GetDefNum("SPELLTIMEOUT", true) )
			SpellTimeout = m_pChar->GetDefNum("SPELLTIMEOUT", true) * MSECS_PER_SEC; // tenths to msec

		addTarget(CLIMODE_TARG_SKILL_MAGERY, pPrompt, pSpellDef->IsSpellType(SPELLFLAG_TARG_XYZ), pSpellDef->IsSpellType(SPELLFLAG_HARM), SpellTimeout);
		return true;
	}

	// Non-targeted spells
	m_pChar->m_Act_p = m_pChar->GetTopPoint();
	m_pChar->m_Act_UID = m_Targ_UID;
	m_pChar->m_Act_Prv_UID = m_Targ_Prv_UID;
	m_pChar->m_atMagery.m_iSpell = iSpell;
	m_Targ_p = m_pChar->GetTopPoint();

	if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
	{
		m_pChar->Spell_CastDone();
		return true;
	}
	else
	{
		int skill;
		if ( !pSpellDef->GetPrimarySkill(&skill, nullptr) )
			return false;

		return m_pChar->Skill_Start((SKILL_TYPE)(skill));
	}
}

bool CClient::Cmd_Skill_Tracking( uint track_sel, bool fExec )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Tracking");
	// look around for stuff.

	ASSERT(m_pChar);
	if ( track_sel == UINT32_MAX )
	{
		// Tracking (unlike other skills) is used during menu setup.
		m_pChar->Skill_Cleanup();	// clean up current skill.

		CMenuItem item[6];
		item[0].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_TITLE);

		item[1].m_id = ITEMID_TRACK_HORSE;
		item[1].m_color = 0;
		item[1].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_ANIMALS);
		item[2].m_id = ITEMID_TRACK_OGRE;
		item[2].m_color = 0;
		item[2].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_MONSTERS);
		item[3].m_id = ITEMID_TRACK_MAN;
		item[3].m_color = 0;
		item[3].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_HUMANS);
		item[4].m_id = ITEMID_TRACK_WOMAN;
		item[4].m_color = 0;
		item[4].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_PLAYERS);

		m_tmMenu.m_Item[0] = 0;
		addItemMenu(CLIMODE_MENU_SKILL_TRACK_SETUP, item, 4);
		return true;
	}

	if ( track_sel > 0 ) // Not Cancelled
	{
		ASSERT(track_sel < ARRAY_COUNT(m_tmMenu.m_Item));
		if ( fExec )
		{
			// Tracking menu got us here. Start tracking the selected creature.
			m_pChar->SetTimeoutS(1);
			m_pChar->m_Act_UID.SetObjUID(m_tmMenu.m_Item[track_sel]);	// selected UID
			m_pChar->Skill_Start(SKILL_TRACKING);
			return true;
		}

		static const NPCBRAIN_TYPE sm_Track_Brain[] =
		{
			NPCBRAIN_QTY,	// not used here.
			NPCBRAIN_ANIMAL,
			NPCBRAIN_MONSTER,
			NPCBRAIN_HUMAN,
			NPCBRAIN_NONE	// players
		};

		if ( track_sel >= ARRAY_COUNT(sm_Track_Brain) )
			track_sel = ARRAY_COUNT(sm_Track_Brain) - 1;

		NPCBRAIN_TYPE track_type = sm_Track_Brain[track_sel];
        ASSERT(ARRAY_COUNT(m_tmMenu.m_Item) == MAX_MENU_ITEMS);
        CMenuItem item[MAX_MENU_ITEMS];
		uint count = 0;

		item[0].m_sText = g_Cfg.GetDefaultMsg(DEFMSG_TRACKING_SKILLMENU_TITLE);
		m_tmMenu.m_Item[0] = track_sel;

		/*
		When the Tracking skill starts and the Effect property is defined on the Tracking skill use it
		instead of the hardcoded formula for the maximum distance.
		*/
		
		if (m_pChar->m_Act_Effect >= 0)
			m_pChar->m_atTracking.m_dwDistMax = (dword)m_pChar->m_Act_Effect;
		else //This is default Sphere maximum tracking distance.
		{
			int iSkillLevel = m_pChar->Skill_GetAdjusted(SKILL_TRACKING);
			if ((g_Cfg.m_iRacialFlags & RACIALF_HUMAN_JACKOFTRADES) && m_pChar->IsHuman())
				iSkillLevel = maximum(iSkillLevel, 200);			// humans always have a 20.0 minimum skill (racial traits)
			m_pChar->m_atTracking.m_dwDistMax = (dword)(iSkillLevel / 10 + 10);
		}
		auto AreaChars = CWorldSearchHolder::GetInstance(m_pChar->GetTopPoint(), m_pChar->m_atTracking.m_dwDistMax);
		for (;;)
		{
			CChar *pChar = AreaChars->GetChar();
			if ( !pChar )
				break;
			if ( m_pChar == pChar )
				continue;

			if ( GetPrivLevel() < pChar->GetPrivLevel() && pChar->IsStatFlag(STATF_INSUBSTANTIAL) )
				continue;

			const CCharBase *pCharDef = pChar->Char_GetDef();
			NPCBRAIN_TYPE basic_type = pChar->GetNPCBrainGroup();
			if ( basic_type == NPCBRAIN_DRAGON || basic_type == NPCBRAIN_BERSERK )
				basic_type = NPCBRAIN_MONSTER;

			if ( track_type != basic_type && track_type != NPCBRAIN_QTY )
			{
				if ( track_type != NPCBRAIN_NONE )		// no match.
					continue;
				if ( pChar->IsStatFlag(STATF_DEAD) )	// can't track ghosts
					continue;
				if ( !pChar->m_pPlayer )
					continue;

				// Check action difficulty when trying to track players
				int tracking = m_pChar->Skill_GetBase(SKILL_TRACKING);
				int detectHidden = m_pChar->Skill_GetBase(SKILL_DETECTINGHIDDEN);
				if ( (g_Cfg.m_iRacialFlags & RACIALF_ELF_DIFFTRACK) && pChar->IsElf() )
					tracking /= 2;			// elves are more difficult to track (Difficult to Track racial trait)

				int hiding = pChar->Skill_GetBase(SKILL_HIDING);
				int stealth = pChar->Skill_GetBase(SKILL_STEALTH);
				int divisor = maximum(hiding + stealth, 1);

				int chance;
				if ( g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE )
					chance = 50 * (tracking * 2 + detectHidden) / divisor;
				else
					chance = 50 * (tracking + detectHidden + 10 * g_Rand.GetVal(20)) / divisor;

				if ( g_Rand.GetVal(100) > chance )
					continue;
			}

			++count;
			item[count].m_id = (word)(pCharDef->m_trackID);
			item[count].m_color = 0;
			item[count].m_sText = pChar->GetName();
			m_tmMenu.m_Item[count] = pChar->GetUID();

			if ( count >= (ARRAY_COUNT(item) - 1) )
				break;
		}

		// Some credit for trying.
		if ( count > 0 )
		{
			m_pChar->Skill_UseQuick(SKILL_TRACKING, 20 + g_Rand.GetLLVal(30));
			ASSERT(count < ARRAY_COUNT(item));
			addItemMenu(CLIMODE_MENU_SKILL_TRACK, item, count);
			return true;
		}
		else
		{
			m_pChar->Skill_UseQuick(SKILL_TRACKING, 10 + g_Rand.GetLLVal(30));
		}
	}

	// Tracking failed or was cancelled.
	static lpctstr const sm_Track_FailMsg[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_CANCEL ),
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_ANIMAL ),
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_MONSTER ),
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_HUMAN ),
		g_Cfg.GetDefaultMsg( DEFMSG_TRACKING_FAIL_HUMAN )
	};

	if ( track_sel >= ARRAY_COUNT(sm_Track_FailMsg) )
		track_sel = ARRAY_COUNT(sm_Track_FailMsg) - 1;

	SysMessage(sm_Track_FailMsg[track_sel]);
	return false;
}

bool CClient::Cmd_Skill_Smith( CItem *pIngots )
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Smith");
	ASSERT(m_pChar);
	if ( !pIngots || !pIngots->IsType(IT_INGOT) )
	{
		SysMessageDefault(DEFMSG_SMITHING_FAIL);
		return false;
	}

	ASSERT(m_Targ_UID == pIngots->GetUID());
	if ( pIngots->GetTopLevelObj() != m_pChar )
	{
		SysMessageDefault(DEFMSG_SMITHING_REACH);
		return false;
	}

	// Must have smith hammer equipped
	CItem *pSmithHammer = m_pChar->LayerFind(LAYER_HAND1);
	if ( !pSmithHammer || !pSmithHammer->IsType(IT_WEAPON_MACE_SMITH) )
	{
		SysMessageDefault(DEFMSG_SMITHING_HAMMER);
		return false;
	}

	// Select the blacksmith item type.
	// repair items or make type of items.
	if ( !CWorldMap::IsItemTypeNear(m_pChar->GetTopPoint(), IT_FORGE, 3, false) )
	{
		SysMessageDefault(DEFMSG_SMITHING_FORGE);
		return false;
	}

	// Select the blacksmith item type.
	// repair items or make type of items.
	return Skill_Menu(SKILL_BLACKSMITHING, "sm_blacksmith", pIngots->GetID());
}

bool CClient::Cmd_Skill_Inscription()
{
	ADDTOCALLSTACK("CClient::Cmd_Skill_Inscription");
	// Select the scroll type to make.
	// iSelect = -1 = 1st setup.
	// iSelect = 0 = cancel
	// iSelect = x = execute the selection.
	// we should already be in inscription skill mode.

	ASSERT(m_pChar);

	CItem *pBlankScroll = m_pChar->ContentFind(CResourceID(RES_TYPEDEF, IT_SCROLL_BLANK));
	if ( !pBlankScroll )
	{
		SysMessageDefault(DEFMSG_INSCRIPTION_FAIL);
		return false;
	}

	return Skill_Menu(SKILL_INSCRIPTION, "sm_inscription");
}

bool CClient::Cmd_SecureTrade( CChar *pChar, CItem *pItem )
{
	ADDTOCALLSTACK("CClient::Cmd_SecureTrade");
	// Begin secure trading with a char. (Make the initial offer)
	if ( !pChar || pChar == m_pChar )
		return false;

	// Make sure both clients can see each other, because trade window is an container
	// and containers can be opened only after the object is already loaded on screen
	if ( !m_pChar->CanSee(pChar) || !pChar->CanSee(m_pChar) )
		return false;

	if ( pItem && (IsTrigUsed(TRIGGER_DROPON_CHAR) || IsTrigUsed(TRIGGER_ITEMDROPON_CHAR)) )
	{
		CScriptTriggerArgs Args(pChar);
		if ( pItem->OnTrigger(ITRIG_DROPON_CHAR, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	if ( pChar->m_pNPC )		// NPC's can't use trade windows
		return pItem ? pChar->NPC_OnItemGive(m_pChar, pItem) : false;
	if ( !pChar->IsClientActive() )	// and also offline players
		return false;

	if ( pChar->GetDefNum("REFUSETRADES", true) )
	{
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_REFUSE), pChar->GetName());
		return false;
	}

	// Check if the trade window is already open
	for (CSObjContRec* pObjRec : m_pChar->GetIterationSafeContReverse())
	{
		CItem* pItemCont = static_cast<CItem*>(pObjRec);
		if ( !pItemCont->IsType(IT_EQ_TRADE_WINDOW) )
			continue;

		CItem *pItemPartner = pItemCont->m_uidLink.ItemFind();
		if ( !pItemPartner )
			continue;

		CChar *pCharPartner = dynamic_cast<CChar *>(pItemPartner->GetParent());
		if ( pCharPartner != pChar )
			continue;

		if ( pItem )
		{
			if ( IsTrigUsed(TRIGGER_DROPON_TRADE) )
			{
				CScriptTriggerArgs Args1(pChar);
				if ( pItem->OnTrigger(ITRIG_DROPON_TRADE, this, &Args1) == TRIGRET_RET_TRUE )
					return false;
			}
			CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItemCont);
			if ( pCont )
				pCont->ContentAdd(pItem);
		}
		return true;
	}

	// Open new trade window
	if ( IsTrigUsed(TRIGGER_TRADECREATE) )
	{
		CScriptTriggerArgs Args(pItem);
		if ( (m_pChar->OnTrigger(CTRIG_TradeCreate, pChar, &Args) == TRIGRET_RET_TRUE) || (pChar->OnTrigger(CTRIG_TradeCreate, m_pChar, &Args) == TRIGRET_RET_TRUE) )
			return false;
	}

	if ( IsTrigUsed(TRIGGER_DROPON_TRADE) && pItem )
	{
		CScriptTriggerArgs Args1(pChar);
		if ( pItem->OnTrigger(ITRIG_DROPON_TRADE, this, &Args1) == TRIGRET_RET_TRUE )
			return false;
	}

	CItem *pItem1 = CItem::CreateBase(ITEMID_Bulletin1);
	if ( !pItem1 )
		return false;

	CItemContainer *pCont1 = dynamic_cast<CItemContainer *>(pItem1);
	if ( !pCont1 )
	{
		DEBUG_ERR(("Item 0%x must be a container type to enable player trading.\n", ITEMID_Bulletin1));
		pItem1->Delete();
		return false;
	}

	CItemContainer *pCont2 = static_cast<CItemContainer *>(CItem::CreateBase(ITEMID_Bulletin1));
	ASSERT(pCont2);

	pCont1->SetName("Trade Window");
	pCont1->SetType(IT_EQ_TRADE_WINDOW);
	pCont1->m_itEqTradeWindow.m_bCheck = 0;
	pCont1->m_uidLink = pCont2->GetUID();
	m_pChar->LayerAdd(pCont1, LAYER_SPECIAL);

	pCont2->SetName("Trade Window");
	pCont2->SetType(IT_EQ_TRADE_WINDOW);
	pCont2->m_itEqTradeWindow.m_bCheck = 0;
	pCont2->m_uidLink = pCont1->GetUID();
	pChar->LayerAdd(pCont2, LAYER_SPECIAL);

	PacketTradeAction cmd(SECURE_TRADE_OPEN);
	cmd.prepareContainerOpen(pChar, pCont1, pCont2);
	cmd.send(this);
	cmd.prepareContainerOpen(m_pChar, pCont2, pCont1);
	cmd.send(pChar->GetClientActive());

	if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD )
	{
		PacketTradeAction cmd2(SECURE_TRADE_UPDATELEDGER);
		if ( GetNetState()->isClientVersionNumber(MINCLIVER_NEWSECURETRADE) )
		{
			cmd2.prepareUpdateLedger(pCont1, (dword)(m_pChar->m_virtualGold % 1000000000), (dword)(m_pChar->m_virtualGold / 1000000000));
			cmd2.send(this);
		}
		if ( pChar->GetClientActive()->GetNetState()->isClientVersionNumber(MINCLIVER_NEWSECURETRADE) )
		{
			cmd2.prepareUpdateLedger(pCont2, (dword)(pChar->m_virtualGold % 1000000000), (dword)(pChar->m_virtualGold / 1000000000));
			cmd2.send(pChar->GetClientActive());
		}
	}

	LogOpenedContainer(pCont2);
	pChar->GetClientActive()->LogOpenedContainer(pCont1);

	if ( pItem )
	{
		if ( IsTrigUsed(TRIGGER_DROPON_TRADE) )
		{
			CScriptTriggerArgs Args1(pChar);
			if ( pItem->OnTrigger(ITRIG_DROPON_TRADE, this, &Args1) == TRIGRET_RET_TRUE )
			{
				pCont1->Delete();
				pCont2->Delete();
				return false;
			}
		}
		pCont1->ContentAdd(pItem, pCont1->GetUnkPoint());
	}
	return true;
}
