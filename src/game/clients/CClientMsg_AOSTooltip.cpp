#include "../triggers.h"
#include "../chars/CChar.h"
#include "../chars/CCharNPC.h"
#include "../items/CItem.h"
#include "../items/CItemSpawn.h"
#include "../items/CItemStone.h"
#include "CClient.h"


// Simple string hashing algorithm function
// Founded by D. J. Bernstein
// Original code found at: http://www.cse.yorku.ca/~oz/hash.html
uint HashString(lpctstr str, size_t length)
{
	uint hash = 5381;
	for (size_t i = 0; i < length; i++)
		hash = ((hash << 5) + hash) + *str++;

	return hash;
}


void CClient::addAOSTooltip(const CObjBase * pObj, bool bRequested, bool bShop)
{
	ADDTOCALLSTACK("CClient::addAOSTooltip");
	if (!pObj)
		return;

	if (PacketPropertyList::CanSendTo(GetNetState()) == false)
		return;

	bool bNameOnly = false;
	if (!IsResClient(RDS_AOS) || !IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))
	{
		if (!bShop)
			return;

		// shop items use tooltips whether they're disabled or not,
		// so we can just send a basic tooltip with the item name
		bNameOnly = true;
	}

	// we do not need to send tooltips for items not in LOS (multis/ships)
	//DEBUG_MSG(("(( m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()) (%x) > UO_MAP_VIEW_SIZE (%x) ) && ( !bShop ) (%x) )", m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()), UO_MAP_VIEW_SIZE, ( !bShop )));
	if ((m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()) > UO_MAP_VIEW_SIZE) && (m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()) <= UO_MAP_VIEW_RADAR) && (!bShop))
		return;

	// We check here if we are sending a tooltip for a static/non-movable items
	// (client doesn't expect us to) but only in the world
	if (pObj->IsItem())
	{
		const CItem * pItem = dynamic_cast<const CItem *>(pObj);

		if (!pItem->GetContainer() && pItem->IsAttr(/*ATTR_MOVE_NEVER|*/ATTR_STATIC))
		{
			if ((!this->GetChar()->IsPriv(PRIV_GM)) && (!this->GetChar()->IsPriv(PRIV_ALLMOVE)))
				return;
		}
	}

	PacketPropertyList* propertyList = pObj->GetPropertyList();

	if (propertyList == NULL || propertyList->hasExpired(g_Cfg.m_iTooltipCache))
	{
		CItem *pItem = pObj->IsItem() ? const_cast<CItem *>(static_cast<const CItem *>(pObj)) : NULL;
		CChar *pChar = pObj->IsChar() ? const_cast<CChar *>(static_cast<const CChar *>(pObj)) : NULL;

		if (pItem != NULL)
			pItem->FreePropertyList();
		else if (pChar != NULL)
			pChar->FreePropertyList();

		CClientTooltip* t = NULL;
		this->m_TooltipData.Clean(true);

		//DEBUG_MSG(("Preparing tooltip for 0%x (%s)\n", (dword)pObj->GetUID(), pObj->GetName()));
		if (bNameOnly) // if we only want to display the name (FEATURE_AOS_UPDATE_B disabled)
		{
			dword ClilocName = (dword)(pObj->GetDefNum("NAMELOC", false, true));

			if (ClilocName)
				m_TooltipData.InsertAt(0, new CClientTooltip(ClilocName));
			else
			{
				m_TooltipData.InsertAt(0, t = new CClientTooltip(1042971)); // ~1_NOTHING~
				t->FormatArgs("%s", pObj->GetName());
			}
		}
		else // we have FEATURE_AOS_UPDATE_B enabled
		{
			TRIGRET_TYPE iRet = TRIGRET_RET_FALSE;

			if ((IsTrigUsed(TRIGGER_CLIENTTOOLTIP)) || ((IsTrigUsed(TRIGGER_ITEMCLIENTTOOLTIP)) && (pItem)) || ((IsTrigUsed(TRIGGER_CHARCLIENTTOOLTIP)) && (pChar)))
			{
				CScriptTriggerArgs args(const_cast<CObjBase *>(pObj));
				args.m_iN1 = bRequested;
				iRet = const_cast<CObjBase *>(pObj)->OnTrigger("@ClientTooltip", this->GetChar(), &args); //ITRIG_CLIENTTOOLTIP , CTRIG_ClientTooltip
			}

			if (iRet != TRIGRET_RET_TRUE)
			{
				// First add the name tooltip entry

				dword ClilocName = (dword)(pObj->GetDefNum("NAMELOC", true, true));

				if (pItem)
				{
					if (ClilocName)
						m_TooltipData.InsertAt(0, new CClientTooltip(ClilocName));
					else
					{
						m_TooltipData.InsertAt(0, t = new CClientTooltip(1050045)); // ~1_PREFIX~~2_NAME~~3_SUFFIX~
						if ((pItem->GetAmount() != 1) && (pItem->GetType() != IT_CORPSE))
							t->FormatArgs("%d \t%s\t ", pItem->GetAmount(), pObj->GetName());
						else
							t->FormatArgs(" \t%s\t ", pObj->GetName());
					}
				}
				else if (pChar)
				{
					lpctstr lpPrefix = pChar->GetKeyStr("NAME.PREFIX");
					// HUE_TYPE wHue = m_pChar->Noto_GetHue( pChar, true );

					if (!*lpPrefix)
						lpPrefix = pChar->Noto_GetFameTitle();

					if (!*lpPrefix)
						lpPrefix = " ";

					tchar * lpSuffix = Str_GetTemp();
					strcpy(lpSuffix, pChar->GetKeyStr("NAME.SUFFIX"));

					const CStoneMember * pGuildMember = pChar->Guild_FindMember(MEMORY_GUILD);
					if (pGuildMember && (!pChar->IsStatFlag(STATF_Incognito) || GetPrivLevel() > pChar->GetPrivLevel()))
					{
						const CItemStone * pParentStone = pGuildMember->GetParentStone();
						ASSERT(pParentStone != NULL);

						if (pGuildMember->IsAbbrevOn() && pParentStone->GetAbbrev()[0])
						{
							strcat(lpSuffix, " [");
							strcat(lpSuffix, pParentStone->GetAbbrev());
							strcat(lpSuffix, "]");
						}
					}

					if (*lpSuffix == '\0')
						strcpy(lpSuffix, " ");

					// The name
					m_TooltipData.InsertAt(0, t = new CClientTooltip(1050045)); // ~1_PREFIX~~2_NAME~~3_SUFFIX~
					if (ClilocName)
						t->FormatArgs("%s\t%u\t%s", lpPrefix, ClilocName, lpSuffix);
					else
						t->FormatArgs("%s\t%s\t%s", lpPrefix, pObj->GetName(), lpSuffix);

					// Need to find a way to get the ushort inside hues.mul for index wHue to get this working.
					// t->FormatArgs("<basefont color=\"#%02x%02x%02x\">%s\t%s\t%s</basefont>",
					//	(byte)((((int)wHue) & 0x7C00) >> 7), (byte)((((int)wHue) & 0x3E0) >> 2),
					//	(byte)((((int)wHue) & 0x1F) << 3),lpPrefix, pObj->GetName(), lpSuffix); // ~1_PREFIX~~2_NAME~~3_SUFFIX~

					if (!pChar->IsStatFlag(STATF_Incognito) || (GetPrivLevel() > pChar->GetPrivLevel()))
					{
						if (pGuildMember && pGuildMember->IsAbbrevOn())
						{
							if (pGuildMember->GetTitle()[0])
							{
								this->m_TooltipData.Add(t = new CClientTooltip(1060776)); // ~1_val~, ~2_val~
								t->FormatArgs("%s\t%s", pGuildMember->GetTitle(), pGuildMember->GetParentStone()->GetName());
							}
							else
							{
								this->m_TooltipData.Add(new CClientTooltip(1070722, pGuildMember->GetParentStone()->GetName())); // ~1_NOTHING~
							}
						}
					}
				}


				// Then add some default tooltip entries, if RETURN 0 or no script

				if (pChar)		// Character specific stuff
					AOSTooltip_addDefaultCharData(pChar);
				else if (pItem)	// Item specific stuff
					AOSTooltip_addDefaultItemData(pItem);
			}
		}

#define DOHASH( value ) hash ^= ((value) & 0x3FFFFFF); \
						hash ^= ((value) >> 26) & 0x3F;

		// build a hash value from the tooltip entries
		dword hash = 0;
		dword argumentHash = 0;
		for (size_t i = 0; i < m_TooltipData.GetCount(); i++)
		{
			CClientTooltip* tipEntry = m_TooltipData.GetAt(i);
			argumentHash = HashString(tipEntry->m_args, strlen(tipEntry->m_args));

			DOHASH(tipEntry->m_clilocid);
			DOHASH(argumentHash);
		}
		hash |= UID_F_ITEM;

#undef DOHASH

		// clients actually expect to use an incremental revision number and not a
		// hash to check if a tooltip needs updating - the client will not request
		// updated tooltip data if the hash happens to be less than the previous one
		//
		// we still want to generate a hash though, so we don't have to increment
		// the revision number if the tooltip hasn't actually been changed
		dword revision = 0;
		if (pItem != NULL)
			revision = pItem->UpdatePropertyRevision(hash);
		else if (pChar != NULL)
			revision = pChar->UpdatePropertyRevision(hash);

		propertyList = new PacketPropertyList(pObj, revision, &m_TooltipData);

		// cache the property list for next time, unless property list is
		// incomplete (name only) or caching is disabled
		if (bNameOnly == false && g_Cfg.m_iTooltipCache > 0)
		{
			if (pItem != NULL)
				pItem->SetPropertyList(propertyList);
			else if (pChar != NULL)
				pChar->SetPropertyList(propertyList);
		}
	}

	if (propertyList->isEmpty() == false)
	{
		switch (g_Cfg.m_iTooltipMode)
		{
		case TOOLTIPMODE_SENDVERSION:
			if (!bRequested)
			{
				// send property list version (client will send a request for the full tooltip if needed)
				if (PacketPropertyListVersion::CanSendTo(GetNetState()) == false)
					new PacketPropertyListVersionOld(this, pObj, propertyList->getVersion());
				else
					new PacketPropertyListVersion(this, pObj, propertyList->getVersion());

				break;
			}

			// fall through to send full list

		case TOOLTIPMODE_SENDFULL:
		default:
			// send full property list
			new PacketPropertyList(this, propertyList);
			break;
		}
	}

	// delete the original packet, as long as it doesn't belong
	// to the object (i.e. wasn't cached)
	if (propertyList != pObj->GetPropertyList())
		delete propertyList;
}

void CClient::AOSTooltip_addDefaultCharData(const CChar * pChar)
{
	CClientTooltip* t = NULL;

	if (pChar->m_pPlayer)
	{
		if (pChar->IsPriv(PRIV_GM) && !pChar->IsPriv(PRIV_PRIV_NOSHOW))
			this->m_TooltipData.Add(new CClientTooltip(1018085)); // Game Master
	}
	else if (pChar->m_pNPC)
	{
		if (g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE)
		{
			CREID_TYPE id = pChar->GetID();
			if (id == CREID_LLAMA_PACK || id == CREID_HORSE_PACK || id == CREID_GIANT_BEETLE)
			{
				int iWeight = pChar->GetWeight() / WEIGHT_UNITS;
				this->m_TooltipData.Add(t = new CClientTooltip(iWeight == 1 ? 1072788 : 1072789)); // Weight: ~1_WEIGHT~ stone / Weight: ~1_WEIGHT~ stones
				t->FormatArgs("%d", iWeight);
			}

			if (pChar->Skill_GetActive() == NPCACT_GUARD_TARG)
				this->m_TooltipData.Add(new CClientTooltip(1080078)); // guarding
		}

		if (pChar->IsStatFlag(STATF_Conjured))
			this->m_TooltipData.Add(new CClientTooltip(1049646)); // (summoned)
		else if (pChar->IsStatFlag(STATF_Pet))
			this->m_TooltipData.Add(new CClientTooltip(pChar->m_pNPC->m_bonded ? 1049608 : 502006)); // (bonded) / (tame)
	}
}

void CClient::AOSTooltip_addDefaultItemData(const CItem * pItem)
{
	CClientTooltip* t = NULL;

	if (pItem->IsAttr(ATTR_LOCKEDDOWN))
		this->m_TooltipData.Add(new CClientTooltip(501643)); // Locked Down
	if (pItem->IsAttr(ATTR_SECURE))
		this->m_TooltipData.Add(new CClientTooltip(501644)); // Locked Down & Secured
	if (pItem->IsAttr(ATTR_BLESSED))
		this->m_TooltipData.Add(new CClientTooltip(1038021)); // Blessed
	if (pItem->IsAttr(ATTR_CURSED))
		this->m_TooltipData.Add(new CClientTooltip(1049643)); // Cursed
	if (pItem->IsAttr(ATTR_INSURED))
		this->m_TooltipData.Add(new CClientTooltip(1061682)); // <b>Insured</b>
	if (pItem->IsAttr(ATTR_QUESTITEM))
		this->m_TooltipData.Add(new CClientTooltip(1072351)); // Quest Item
	if (pItem->IsAttr(ATTR_MAGIC))
		this->m_TooltipData.Add(new CClientTooltip(3010064)); // Magic
	if (pItem->IsAttr(ATTR_NEWBIE))
		this->m_TooltipData.Add(new CClientTooltip(1070722, g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_NEWBIE))); // ~1_NOTHING~

	if (g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE)
	{
		if (pItem->IsMovable())
		{
			int iWeight = pItem->GetWeight() / WEIGHT_UNITS;
			this->m_TooltipData.Add(t = new CClientTooltip(iWeight == 1 ? 1072788 : 1072789)); // Weight: ~1_WEIGHT~ stone / Weight: ~1_WEIGHT~ stones
			t->FormatArgs("%d", iWeight);
		}
	}

	CUID uid = static_cast<CUID>((dword)(pItem->GetDefNum("CRAFTEDBY")));
	CChar *pCraftsman = uid.CharFind();
	if (pCraftsman)
	{
		this->m_TooltipData.Add(t = new CClientTooltip(1050043)); // crafted by ~1_NAME~
		t->FormatArgs("%s", pCraftsman->GetName());
	}

	if (pItem->IsAttr(ATTR_EXCEPTIONAL))
		this->m_TooltipData.Add(new CClientTooltip(1060636)); // exceptional

	int64 ArtifactRarity = pItem->GetDefNum("RARITY", true, true);
	if (ArtifactRarity > 0)
	{
		this->m_TooltipData.Add(t = new CClientTooltip(1061078)); // artifact rarity ~1_val~
		t->FormatArgs("%" PRId64, ArtifactRarity);
	}

	int64 UsesRemaining = pItem->GetDefNum("USESCUR", true, true);
	if (UsesRemaining > 0)
	{
		this->m_TooltipData.Add(t = new CClientTooltip(1060584)); // uses remaining: ~1_val~
		t->FormatArgs("%" PRId64, UsesRemaining);
	}

	if (pItem->IsTypeArmorWeapon())
	{
		if (pItem->GetDefNum("BALANCED", true, true))
			this->m_TooltipData.Add(new CClientTooltip(1072792)); // balanced

		int64 DamageIncrease = pItem->GetDefNum("INCREASEDAM", true, true);
		if (DamageIncrease != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060401)); // damage increase ~1_val~%
			t->FormatArgs("%" PRId64, DamageIncrease);
		}

		int64 DefenceChanceIncrease = pItem->GetDefNum("INCREASEDEFCHANCE", true, true);
		if (DefenceChanceIncrease != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060408)); // defense chance increase ~1_val~%
			t->FormatArgs("%" PRId64, DefenceChanceIncrease);
		}

		int64 DexterityBonus = pItem->GetDefNum("BONUSDEX", true, true);
		if (DexterityBonus != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060409)); // dexterity bonus ~1_val~
			t->FormatArgs("%" PRId64, DexterityBonus);
		}

		int64 EnhancePotions = pItem->GetDefNum("ENHANCEPOTIONS", true, true);
		if (EnhancePotions != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060411)); // enhance potions ~1_val~%
			t->FormatArgs("%" PRId64, EnhancePotions);
		}

		int64 FasterCastRecovery = pItem->GetDefNum("FASTERCASTRECOVERY", true, true);
		if (FasterCastRecovery != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060412)); // faster cast recovery ~1_val~
			t->FormatArgs("%" PRId64, FasterCastRecovery);
		}

		int64 FasterCasting = pItem->GetDefNum("FASTERCASTING", true, true);
		if (FasterCasting != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060413)); // faster casting ~1_val~
			t->FormatArgs("%" PRId64, FasterCasting);
		}

		int64 HitChanceIncrease = pItem->GetDefNum("INCREASEHITCHANCE", true, true);
		if (HitChanceIncrease != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060415)); // hit chance increase ~1_val~%
			t->FormatArgs("%" PRId64, HitChanceIncrease);
		}

		int64 HitPointIncrease = pItem->GetDefNum("BONUSHITS", true, true);
		if (HitPointIncrease != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060431)); // hit point increase ~1_val~%
			t->FormatArgs("%" PRId64, HitPointIncrease);
		}

		int64 IntelligenceBonus = pItem->GetDefNum("BONUSINT", true, true);
		if (IntelligenceBonus != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060432)); // intelligence bonus ~1_val~
			t->FormatArgs("%" PRId64, IntelligenceBonus);
		}

		int64 LowerManaCost = pItem->GetDefNum("LOWERMANACOST", true, true);
		if (LowerManaCost != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060433)); // lower mana cost ~1_val~%
			t->FormatArgs("%" PRId64, LowerManaCost);
		}

		int64 LowerReagentCost = pItem->GetDefNum("LOWERREAGENTCOST", true, true);
		if (LowerReagentCost != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060434)); // lower reagent cost ~1_val~%
			t->FormatArgs("%" PRId64, LowerReagentCost);
		}

		int64 LowerRequirements = pItem->GetDefNum("LOWERREQ", true, true);
		if (LowerRequirements != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060435)); // lower requirements ~1_val~%
			t->FormatArgs("%" PRId64, LowerRequirements);
		}

		int64 Luck = pItem->GetDefNum("LUCK", true, true);
		if (Luck != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060436)); // luck ~1_val~
			t->FormatArgs("%" PRId64, Luck);
		}

		if (pItem->GetDefNum("MAGEARMOR", true, true))
			this->m_TooltipData.Add(new CClientTooltip(1060437)); // mage armor

		int64 MageWeapon = pItem->GetDefNum("MAGEWEAPON", true, true);
		if (MageWeapon != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060438)); // mage weapon -~1_val~ skill
			t->FormatArgs("%" PRId64, MageWeapon);
		}

		int64 ManaIncrease = pItem->GetDefNum("BONUSMANA", true, true);
		if (ManaIncrease != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060439)); // mana increase ~1_val~
			t->FormatArgs("%" PRId64, ManaIncrease);
		}

		int64 ManaRegeneration = pItem->GetDefNum("REGENMANA", true, true);
		if (ManaRegeneration != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060440)); // mana regeneration ~1_val~
			t->FormatArgs("%" PRId64, ManaRegeneration);
		}

		if (pItem->GetDefNum("NIGHTSIGHT", true, true))
			this->m_TooltipData.Add(new CClientTooltip(1060441)); // night sight

		int64 ReflectPhysicalDamage = pItem->GetDefNum("REFLECTPHYSICALDAM", true, true);
		if (ReflectPhysicalDamage != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060442)); // reflect physical damage ~1_val~%
			t->FormatArgs("%" PRId64, ReflectPhysicalDamage);
		}

		int64 StaminaRegeneration = pItem->GetDefNum("REGENSTAM", true, true);
		if (StaminaRegeneration != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060443)); // stamina regeneration ~1_val~
			t->FormatArgs("%" PRId64, StaminaRegeneration);
		}

		int64 HitPointRegeneration = pItem->GetDefNum("REGENHITS", true, true);
		if (HitPointRegeneration != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060444)); // hit point regeneration ~1_val~
			t->FormatArgs("%" PRId64, HitPointRegeneration);
		}

		int64 SelfRepair = pItem->GetDefNum("SELFREPAIR", true, true);
		if (SelfRepair != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060450)); // self repair ~1_val~
			t->FormatArgs("%" PRId64, SelfRepair);
		}

		if (pItem->GetDefNum("SPELLCHANNELING", true, true))
			this->m_TooltipData.Add(new CClientTooltip(1060482)); // spell channeling

		int64 SpellDamageIncrease = pItem->GetDefNum("INCREASESPELLDAM", true, true);
		if (SpellDamageIncrease != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060483)); // spell damage increase ~1_val~%
			t->FormatArgs("%" PRId64, SpellDamageIncrease);
		}

		int64 StaminaIncrease = pItem->GetDefNum("BONUSSTAM", true, true);
		if (StaminaIncrease != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060484)); // stamina increase ~1_val~
			t->FormatArgs("%" PRId64, StaminaIncrease);
		}

		int64 StrengthBonus = pItem->GetDefNum("BONUSSTR", true, true);
		if (StrengthBonus != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060485)); // strength bonus ~1_val~
			t->FormatArgs("%" PRId64, StrengthBonus);
		}

		int64 SwingSpeedIncrease = pItem->GetDefNum("INCREASESWINGSPEED", true, true);
		if (SwingSpeedIncrease != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060486)); // swing speed increase ~1_val~%
			t->FormatArgs("%" PRId64, SwingSpeedIncrease);
		}

		int64 IncreasedKarmaLoss = pItem->GetDefNum("INCREASEKARMALOSS", true, true);
		if (IncreasedKarmaLoss != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1075210)); // increased karma loss ~1val~%
			t->FormatArgs("%" PRId64, IncreasedKarmaLoss);
		}
	}


	// Some type specific default stuff

	switch (pItem->GetType())
	{
	case IT_CONTAINER_LOCKED:
		this->m_TooltipData.Add(new CClientTooltip(3005142)); // Locked
	case IT_CONTAINER:
	case IT_TRASH_CAN:
		if (pItem->IsContainer())
		{
			const CContainer * pContainer = dynamic_cast <const CContainer *> (pItem);
			this->m_TooltipData.Add(t = new CClientTooltip(1050044));
			t->FormatArgs("%" PRIuSIZE_T "\t%d", pContainer->GetCount(), pContainer->GetTotalWeight() / WEIGHT_UNITS); // ~1_COUNT~ items, ~2_WEIGHT~ stones
		}
		break;

	case IT_ARMOR_LEATHER:
	case IT_ARMOR:
	case IT_CLOTHING:
	case IT_SHIELD:
	{
		if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
		{
			int64 PhysicalResist = pItem->GetDefNum("RESPHYSICAL", true, true);
			if (PhysicalResist != 0)
			{
				this->m_TooltipData.Add(t = new CClientTooltip(1060448)); // physical resist ~1_val~%
				t->FormatArgs("%" PRId64, PhysicalResist);
			}

			int64 FireResist = pItem->GetDefNum("RESFIRE", true, true);
			if (FireResist != 0)
			{
				this->m_TooltipData.Add(t = new CClientTooltip(1060447)); // fire resist ~1_val~%
				t->FormatArgs("%" PRId64, FireResist);
			}

			int64 ColdResist = pItem->GetDefNum("RESCOLD", true, true);
			if (ColdResist != 0)
			{
				this->m_TooltipData.Add(t = new CClientTooltip(1060445)); // cold resist ~1_val~%
				t->FormatArgs("%" PRId64, ColdResist);
			}

			int64 PoisonResist = pItem->GetDefNum("RESPOISON", true, true);
			if (PoisonResist != 0)
			{
				this->m_TooltipData.Add(t = new CClientTooltip(1060449)); // poison resist ~1_val~%
				t->FormatArgs("%" PRId64, PoisonResist);
			}

			int64 EnergyResist = pItem->GetDefNum("RESENERGY", true, true);
			if (EnergyResist != 0)
			{
				this->m_TooltipData.Add(t = new CClientTooltip(1060446)); // energy resist ~1_val~%
				t->FormatArgs("%" PRId64, EnergyResist);
			}
		}

		int ArmorRating = pItem->Armor_GetDefense();
		if (ArmorRating != 0)
		{
			// Obsolete AR was replaced by physical/fire/cold/poison/energy resist since AOS
			// and doesn't even have proper tooltips. It's just there for backward compatibility
			this->m_TooltipData.Add(t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
			t->FormatArgs("%s\t%d", g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_ARMOR), ArmorRating);
		}

		int64 StrengthRequirement = pItem->Item_GetDef()->m_ttEquippable.m_iStrReq - pItem->GetDefNum("LOWERREQ", true, true);
		if (StrengthRequirement > 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1061170)); // strength requirement ~1_val~
			t->FormatArgs("%" PRId64, StrengthRequirement);
		}

		this->m_TooltipData.Add(t = new CClientTooltip(1060639)); // durability ~1_val~ / ~2_val~
		t->FormatArgs("%hu\t%hu", pItem->m_itArmor.m_Hits_Cur, pItem->m_itArmor.m_Hits_Max);
	}
	break;

	case IT_WEAPON_MACE_SMITH:
	case IT_WEAPON_MACE_SHARP:
	case IT_WEAPON_MACE_STAFF:
	case IT_WEAPON_MACE_CROOK:
	case IT_WEAPON_MACE_PICK:
	case IT_WEAPON_SWORD:
	case IT_WEAPON_FENCE:
	case IT_WEAPON_BOW:
	case IT_WEAPON_AXE:
	case IT_WEAPON_XBOW:
	case IT_WEAPON_THROWING:
	{
		if (pItem->m_itWeapon.m_poison_skill)
			this->m_TooltipData.Add(new CClientTooltip(1017383)); // poisoned

		if (pItem->GetDefNum("USEBESTWEAPONSKILL", true, true))
			this->m_TooltipData.Add(new CClientTooltip(1060400)); // use best weapon skill

		int64 HitColdArea = pItem->GetDefNum("HITAREACOLD", true, true);
		if (HitColdArea != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060416)); // hit cold area ~1_val~%
			t->FormatArgs("%" PRId64, HitColdArea);
		}

		int64 HitDispel = pItem->GetDefNum("HITDISPEL", true, true);
		if (HitDispel != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060417)); // hit dispel ~1_val~%
			t->FormatArgs("%" PRId64, HitDispel);
		}

		int64 HitEnergyArea = pItem->GetDefNum("HITAREAENERGY", true, true);
		if (HitEnergyArea != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060418)); // hit energy area ~1_val~%
			t->FormatArgs("%" PRId64, HitEnergyArea);
		}

		int64 HitFireArea = pItem->GetDefNum("HITAREAFIRE", true, true);
		if (HitFireArea != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060419)); // hit fire area ~1_val~%
			t->FormatArgs("%" PRId64, HitFireArea);
		}

		int64 HitFireball = pItem->GetDefNum("HITFIREBALL", true, true);
		if (HitFireball != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060420)); // hit fireball ~1_val~%
			t->FormatArgs("%" PRId64, HitFireball);
		}

		int64 HitHarm = pItem->GetDefNum("HITHARM", true, true);
		if (HitHarm != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060421)); // hit harm ~1_val~%
			t->FormatArgs("%" PRId64, HitHarm);
		}

		int64 HitLifeLeech = pItem->GetDefNum("HITLEECHLIFE", true, true);
		if (HitLifeLeech != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060422)); // hit life leech ~1_val~%
			t->FormatArgs("%" PRId64, HitLifeLeech);
		}

		int64 HitLightning = pItem->GetDefNum("HITLIGHTNING", true, true);
		if (HitLightning != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060422)); // hit lightning ~1_val~%
			t->FormatArgs("%" PRId64, HitLightning);
		}

		int64 HitLowerAttack = pItem->GetDefNum("HITLOWERATK", true, true);
		if (HitLowerAttack != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060424)); // hit lower attack ~1_val~%
			t->FormatArgs("%" PRId64, HitLowerAttack);
		}

		int64 HitLowerDefense = pItem->GetDefNum("HITLOWERDEF", true, true);
		if (HitLowerDefense != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060425)); // hit lower defense ~1_val~%
			t->FormatArgs("%" PRId64, HitLowerDefense);
		}

		int64 HitMagicArrow = pItem->GetDefNum("HITMAGICARROW", true, true);
		if (HitMagicArrow != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060426)); // hit magic arrow ~1_val~%
			t->FormatArgs("%" PRId64, HitMagicArrow);
		}

		int64 HitManaLeech = pItem->GetDefNum("HITLEECHMANA", true, true);
		if (HitManaLeech != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060427)); // hit mana leech ~1_val~%
			t->FormatArgs("%" PRId64, HitManaLeech);
		}

		int64 HitPhysicalArea = pItem->GetDefNum("HITAREAPHYSICAL", true, true);
		if (HitPhysicalArea != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060428)); // hit physical area ~1_val~%
			t->FormatArgs("%" PRId64, HitPhysicalArea);
		}

		int64 HitPoisonArea = pItem->GetDefNum("HITAREAPOISON", true, true);
		if (HitPoisonArea != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060429)); // hit poison area ~1_val~%
			t->FormatArgs("%" PRId64, HitPoisonArea);
		}

		int64 HitStaminaLeech = pItem->GetDefNum("HITLEECHSTAM", true, true);
		if (HitStaminaLeech != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060430)); // hit stamina leech ~1_val~%
			t->FormatArgs("%" PRId64, HitStaminaLeech);
		}

		int64 PhysicalDamage = pItem->GetDefNum("DAMPHYSICAL", true, true);
		if (PhysicalDamage != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060403)); // physical damage ~1_val~%
			t->FormatArgs("%" PRId64, PhysicalDamage);
		}

		int64 FireDamage = pItem->GetDefNum("DAMFIRE", true, true);
		if (FireDamage != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060405)); // fire damage ~1_val~%
			t->FormatArgs("%" PRId64, FireDamage);
		}

		int64 ColdDamage = pItem->GetDefNum("DAMCOLD", true, true);
		if (ColdDamage != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060404)); // cold damage ~1_val~%
			t->FormatArgs("%" PRId64, ColdDamage);
		}

		int64 PoisonDamage = pItem->GetDefNum("DAMPOISON", true, true);
		if (PoisonDamage != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060406)); // poison damage ~1_val~%
			t->FormatArgs("%" PRId64, PoisonDamage);
		}

		int64 EnergyDamage = pItem->GetDefNum("DAMENERGY", true, true);
		if (EnergyDamage != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060407)); // energy damage ~1_val~%
			t->FormatArgs("%" PRId64, EnergyDamage);
		}

		int64 ChaosDamage = pItem->GetDefNum("DAMCHAOS", true, true);
		if (ChaosDamage != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1072846)); // chaos damage ~1_val~%
			t->FormatArgs("%" PRId64, ChaosDamage);
		}

		int64 DirectDamage = pItem->GetDefNum("DAMDIRECT", true, true);
		if (DirectDamage != 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1079978)); // direct damage: ~1_PERCENT~%
			t->FormatArgs("%" PRId64, DirectDamage);
		}

		this->m_TooltipData.Add(t = new CClientTooltip(1061168)); // weapon damage ~1_val~ - ~2_val~
		t->FormatArgs("%d\t%d", pItem->m_attackBase + pItem->m_ModAr, pItem->Weapon_GetAttack(true));

		this->m_TooltipData.Add(t = new CClientTooltip(1061167)); // weapon speed ~1_val~
		t->FormatArgs("%hhu", pItem->GetSpeed());

		byte Range = pItem->RangeL();
		if (Range > 1)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1061169)); // range ~1_val~
			t->FormatArgs("%hhu", Range);
		}

		int64 StrengthRequirement = pItem->Item_GetDef()->m_ttEquippable.m_iStrReq - pItem->GetDefNum("LOWERREQ", true, true);
		if (StrengthRequirement > 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1061170)); // strength requirement ~1_val~
			t->FormatArgs("%" PRId64, StrengthRequirement);
		}

		if (pItem->Item_GetDef()->GetEquipLayer() == LAYER_HAND2)
			this->m_TooltipData.Add(new CClientTooltip(1061171)); // two-handed weapon
		else
			this->m_TooltipData.Add(new CClientTooltip(1061824)); // one-handed weapon

		if (!pItem->GetDefNum("USEBESTWEAPONSKILL", true, true))
		{
			switch (pItem->Item_GetDef()->m_iSkill)
			{
			case SKILL_SWORDSMANSHIP:	this->m_TooltipData.Add(new CClientTooltip(1061172));	break; // skill required: swordsmanship
			case SKILL_MACEFIGHTING:	this->m_TooltipData.Add(new CClientTooltip(1061173));	break; // skill required: mace fighting
			case SKILL_FENCING:			this->m_TooltipData.Add(new CClientTooltip(1061174));	break; // skill required: fencing
			case SKILL_ARCHERY:			this->m_TooltipData.Add(new CClientTooltip(1061175));	break; // skill required: archery
			case SKILL_THROWING:		this->m_TooltipData.Add(new CClientTooltip(1112075));	break; // skill required: throwing
			default:					break;
			}
		}

		this->m_TooltipData.Add(t = new CClientTooltip(1060639)); // durability ~1_val~ / ~2_val~
		t->FormatArgs("%hu\t%hu", pItem->m_itWeapon.m_Hits_Cur, pItem->m_itWeapon.m_Hits_Max);
	}
	break;

	case IT_WAND:
		this->m_TooltipData.Add(t = new CClientTooltip(1054132)); // [charges: ~1_charges~]
		t->FormatArgs("%d", pItem->m_itWeapon.m_spellcharges);
		break;

	case IT_TELEPAD:
	case IT_MOONGATE:
		if (this->IsPriv(PRIV_GM))
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
			t->FormatArgs("%s\t%s", g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_DESTINATION), pItem->m_itTelepad.m_pntMark.WriteUsed());
		}
		break;

	case IT_RUNE:
	{
		const CPointMap pt = pItem->m_itTelepad.m_pntMark;
		if (!pt.IsValidPoint())
			break;

		lpctstr regionName = g_Cfg.GetDefaultMsg(DEFMSG_RUNE_LOCATION_UNK);
		if (pt.GetRegion(REGION_TYPE_AREA))
			regionName = pt.GetRegion(REGION_TYPE_AREA)->GetName();
		bool regionMulti = (pt.GetRegion(REGION_TYPE_MULTI) != NULL);

		if (pt.m_map == 0)
			this->m_TooltipData.Add(t = new CClientTooltip(regionMulti ? 1062452 : 1060805)); // ~1_val~ (Felucca)[(House)]
		else if (pt.m_map == 1)
			this->m_TooltipData.Add(t = new CClientTooltip(regionMulti ? 1062453 : 1060806)); // ~1_val~ (Trammel)[(House)]
		else if (pt.m_map == 3)
			this->m_TooltipData.Add(t = new CClientTooltip(regionMulti ? 1062454 : 1060804)); // ~1_val~ (Malas)[(House)]
		else if (pt.m_map == 4)
			this->m_TooltipData.Add(t = new CClientTooltip(regionMulti ? 1063260 : 1063259)); // ~1_val~ (Tokuno Islands)[(House)]
		else if (pt.m_map == 5)
			this->m_TooltipData.Add(t = new CClientTooltip(regionMulti ? 1113206 : 1113205)); // ~1_val~ (Ter Mur)[(House)]
		else
			// There's no proper clilocs for Ilshenar (map2) and custom facets (map > 5), so let's use a generic cliloc
			this->m_TooltipData.Add(t = new CClientTooltip(1042971)); // ~1_NOTHING~

		t->FormatArgs("%s %s", g_Cfg.GetDefaultMsg(DEFMSG_RUNE_TO), regionName);
	}
	break;

	case IT_SPELLBOOK:
	case IT_SPELLBOOK_NECRO:
	case IT_SPELLBOOK_PALA:
	case IT_SPELLBOOK_BUSHIDO:
	case IT_SPELLBOOK_NINJITSU:
	case IT_SPELLBOOK_ARCANIST:
	case IT_SPELLBOOK_MYSTIC:
	case IT_SPELLBOOK_BARD:
	{
		int count = pItem->GetSpellcountInBook();
		if (count > 0)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1042886)); // ~1_NUMBERS_OF_SPELLS~ Spells
			t->FormatArgs("%d", count);
		}
	} break;

	case IT_SPAWN_CHAR:
	{
		CResourceDef * pSpawnCharDef = g_Cfg.ResourceGetDef(pItem->m_itSpawnChar.m_CharID);
		lpctstr pszName = NULL;
		if (pSpawnCharDef)
		{
			CCharBase *pCharBase = dynamic_cast<CCharBase*>(pSpawnCharDef);
			if (pCharBase)
				pszName = pCharBase->GetTradeName();
			else
				pszName = pSpawnCharDef->GetName();

			while (*pszName == '#')
				pszName++;
		}

		this->m_TooltipData.Add(t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
		t->FormatArgs("Character\t%s", pszName ? pszName : "none");
		this->m_TooltipData.Add(t = new CClientTooltip(1061169)); // range ~1_val~
		t->FormatArgs("%hhu", pItem->m_itSpawnChar.m_DistMax);
		this->m_TooltipData.Add(t = new CClientTooltip(1074247)); // Live Creatures: ~1_NUM~ / ~2_MAX~
		t->FormatArgs("%hhu\t%hu", static_cast<const CItemSpawn *>(pItem)->m_currentSpawned, pItem->GetAmount());
		this->m_TooltipData.Add(t = new CClientTooltip(1060659)); // ~1_val~: ~2_val~
		t->FormatArgs("Time range\t%hu min / %hu max", pItem->m_itSpawnChar.m_TimeLoMin, pItem->m_itSpawnChar.m_TimeHiMin);
		this->m_TooltipData.Add(t = new CClientTooltip(1060660)); // ~1_val~: ~2_val~
		t->FormatArgs("Time until next spawn\t%" PRId64 " sec", pItem->GetTimerAdjusted());
	} break;

	case IT_SPAWN_ITEM:
	{
		CResourceDef * pSpawnItemDef = g_Cfg.ResourceGetDef(pItem->m_itSpawnItem.m_ItemID);

		this->m_TooltipData.Add(t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
		t->FormatArgs("Item\t%u %s", maximum(1, pItem->m_itSpawnItem.m_pile), pSpawnItemDef ? pSpawnItemDef->GetName() : "none");
		this->m_TooltipData.Add(t = new CClientTooltip(1061169)); // range ~1_val~
		t->FormatArgs("%hhu", pItem->m_itSpawnItem.m_DistMax);
		this->m_TooltipData.Add(t = new CClientTooltip(1074247)); // Live Creatures: ~1_NUM~ / ~2_MAX~
		t->FormatArgs("%hhu\t%hu", static_cast<const CItemSpawn *>(pItem)->m_currentSpawned, pItem->GetAmount());
		this->m_TooltipData.Add(t = new CClientTooltip(1060659)); // ~1_val~: ~2_val~
		t->FormatArgs("Time range\t%hu min / %hu max", pItem->m_itSpawnItem.m_TimeLoMin, pItem->m_itSpawnItem.m_TimeHiMin);
		this->m_TooltipData.Add(t = new CClientTooltip(1060660)); // ~1_val~: ~2_val~
		t->FormatArgs("Time until next spawn\t%" PRId64 " sec", pItem->GetTimerAdjusted());
	} break;

	case IT_COMM_CRYSTAL:
	{
		CItem *pLink = pItem->m_uidLink.ItemFind();
		this->m_TooltipData.Add(new CClientTooltip((pLink && pLink->IsType(IT_COMM_CRYSTAL)) ? 1060742 : 1060743)); // active / inactive
		this->m_TooltipData.Add(new CClientTooltip(1060745)); // broadcast
	} break;

	case IT_STONE_GUILD:
	{
		this->m_TooltipData.Clean(true);
		this->m_TooltipData.Add(t = new CClientTooltip(1041429)); // a guildstone
		const CItemStone *thisStone = static_cast<const CItemStone *>(pItem);
		if (thisStone)
		{
			this->m_TooltipData.Add(t = new CClientTooltip(1060802)); // Guild name: ~1_val~
			if (thisStone->GetAbbrev()[0])
				t->FormatArgs("%s [%s]", thisStone->GetName(), thisStone->GetAbbrev());
			else
				t->FormatArgs("%s", thisStone->GetName());
		}
	} break;

	default:
		break;
	}

}
