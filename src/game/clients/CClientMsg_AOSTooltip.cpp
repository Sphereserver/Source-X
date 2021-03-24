#include "../chars/CChar.h"
#include "../chars/CCharNPC.h"
#include "../items/CItem.h"
#include "../items/CItemStone.h"
#include "../components/CCSpawn.h"
//#include "../components/CCPropsItemChar.h"
#include "../components/CCPropsItemEquippable.h"
#include "../components/CCPropsItemWeapon.h"
#include "../triggers.h"
#include "CClient.h"


// Simple string hashing algorithm function by D. J. Bernstein
// Original code found at: http://www.cse.yorku.ca/~oz/hash.html
uint HashString(lpctstr str, size_t length)
{
	uint hash = 5381;
	for (size_t i = 0; i < length; ++i)
		hash = ((hash << 5) + hash) + *(str++);

	return hash;
}

#define PUSH_FRONT_TOOLTIP(pObj, t) pObj->m_TooltipData.emplace(pObj->m_TooltipData.begin(),t)
#define PUSH_BACK_TOOLTIP(pObj, t) pObj->m_TooltipData.emplace_back(t)

bool CClient::addAOSTooltip(CObjBase * pObj, bool fRequested, bool fShop)
{
	ADDTOCALLSTACK("CClient::addAOSTooltip");
	if (!pObj)
		return false;

	if (PacketPropertyList::CanSendTo(GetNetState()) == false)
		return false;

	// Enhanced and KR clients always need the tooltips (but they can't be enabled without FEATURE_AOS_UPDATE_B, since this has to be sent to the client via the packet 0xB9).
	// Shop items use tooltips whether they're disabled or not,
	//  so we can just send a basic tooltip with the item name
	bool fNameOnly = false;
	if (!IsResClient(RDS_AOS) || !IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))
	{
		if (!fShop)
			return false;

		fNameOnly = true;
	}

	// we do not need to send tooltips for items not in LOS (multis/ships)
	//DEBUG_MSG(("(( m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()) (%x) > UO_MAP_VIEW_SIZE_DEFAULT (%x) ) && ( !bShop ) (%x) )", m_pChar->GetTopPoint().GetDistSight(pObj->GetTopPoint()), UO_MAP_VIEW_SIZE_DEFAULT, ( !bShop )));
	int iDist = GetChar()->GetTopPoint().GetDistSight(pObj->GetTopPoint());
	if ( (iDist > GetChar()->GetVisualRange()) && (iDist <= UO_MAP_VIEW_RADAR) && !fShop ) //(iDist <= UO_MAP_VIEW_RADAR) fShop is needed because items equipped or in a container have invalid GetTopPoint (and a very high iDist)
		return false;

	// We check here if we are sending a tooltip for a static/non-movable items
	// (client doesn't expect us to) but only in the world
	if (pObj->IsItem())
	{
		const CItem * pItem = static_cast<const CItem *>(pObj);
		if (!pItem->GetContainer() && pItem->IsAttr(/*ATTR_MOVE_NEVER|*/ATTR_STATIC))
		{
			if ((!GetChar()->IsPriv(PRIV_GM)) && (!GetChar()->IsPriv(PRIV_ALLMOVE)))
				return false;
		}
	}

	PacketPropertyList* propertyList = pObj->GetPropertyList();

	if (propertyList == nullptr || propertyList->hasExpired(g_Cfg.m_iTooltipCache))
	{
        pObj->m_TooltipData.clear();
		pObj->FreePropertyList();

        CClientTooltip* t = nullptr;
        CItem *pItem = pObj->IsItem() ? static_cast<CItem *>(pObj) : nullptr;
        CChar *pChar = pObj->IsChar() ? static_cast<CChar *>(pObj) : nullptr;

		//DEBUG_MSG(("Preparing tooltip for 0%x (%s)\n", (dword)pObj->GetUID(), pObj->GetName()));
		if (fNameOnly) // if we only want to display the name
		{
			dword ClilocName = (dword)(pObj->GetDefNum("NAMELOC", false));

			if (ClilocName)
                PUSH_FRONT_TOOLTIP(pObj, new CClientTooltip(ClilocName));
			else
			{
                PUSH_FRONT_TOOLTIP(pObj, t = new CClientTooltip(1042971)); // ~1_NOTHING~
				t->FormatArgs("%s", pObj->GetName());
			}
		}
		else
		{
			TRIGRET_TYPE iRet = TRIGRET_RET_FALSE;

			if (IsTrigUsed(TRIGGER_CLIENTTOOLTIP) || (pItem && IsTrigUsed(TRIGGER_ITEMCLIENTTOOLTIP)) || (pChar && IsTrigUsed(TRIGGER_CHARCLIENTTOOLTIP)))
			{
				CScriptTriggerArgs args(pObj);
				args.m_iN1 = fRequested;
				iRet = pObj->OnTrigger("@ClientTooltip", this->GetChar(), &args); //ITRIG_CLIENTTOOLTIP , CTRIG_ClientTooltip
			}

			if (iRet != TRIGRET_RET_TRUE)
			{
                // First add the name tooltip entry
                AOSTooltip_addName(pObj);

				// Then add some default tooltip entries, if RETURN 0 or no script
				if (pChar)		// Character specific stuff
					AOSTooltip_addDefaultCharData(pChar);
				else if (pItem)	// Item specific stuff
					AOSTooltip_addDefaultItemData(pItem);

                pObj->AddPropsTooltipData(pObj);
			}
			
			if (IsTrigUsed(TRIGGER_CLIENTTOOLTIP_AFTERDEFAULT) || (pItem && IsTrigUsed(TRIGGER_ITEMCLIENTTOOLTIP_AFTERDEFAULT)) || (pChar && IsTrigUsed(TRIGGER_CHARCLIENTTOOLTIP_AFTERDEFAULT)))
			{
				CScriptTriggerArgs args(pObj);
				args.m_iN1 = fRequested;
				iRet = pObj->OnTrigger("@ClientTooltip_AfterDefault", this->GetChar(), &args); //Save to return on iRet to make sure return value doesn't stuck the boolean.
			}
		}

#define DOHASH( value ) dwHash ^= ((value) & 0x3FFFFFF); \
						dwHash ^= ((value) >> 26) & 0x3F;

		// build a hash value from the tooltip entries
		dword dwHash = 0;
		dword dwArgumentHash = 0;
		for (size_t i = 0; i < pObj->m_TooltipData.size(); ++i)
		{
			CClientTooltip* tipEntry = pObj->m_TooltipData[i].get();
            dwArgumentHash = HashString(tipEntry->m_args, strlen(tipEntry->m_args));

			DOHASH(tipEntry->m_clilocid);
			DOHASH(dwArgumentHash);
		}
        dwHash |= UID_F_ITEM;

#undef DOHASH

        if ( g_Cfg.m_iTooltipMode == TOOLTIPMODE_SENDVERSION )
        {
            // If client receive an tooltip with given obj UID / revision, and for some reason
            // the server delete this obj and create another one with the same UID / revision,
            // the client will show the previous cached tooltip on the new obj. To avoid this,
            // compare both tooltip hashes to check if it really got changed, and if positive,
            // send the full tooltip instead just the revision number
            if ( pObj->GetPropertyHash() != dwHash )
                fRequested = true;
        }

		// Clients actually expect to use an incremental revision number and not a
		// hash to check if a tooltip needs updating - the client will not request
		// updated tooltip data if the hash happens to be less than the previous one
		//
		// we still want to generate a hash though, so we don't have to increment
		// the revision number if the tooltip hasn't actually been changed
		dword revision = pObj->UpdatePropertyRevision(dwHash);
		propertyList = new PacketPropertyList(pObj, revision, pObj->m_TooltipData);

		// cache the property list for next time, unless property list is
		// incomplete (name only) or caching is disabled
		if ((fNameOnly == false) && (g_Cfg.m_iTooltipCache > 0))
		{
			pObj->SetPropertyList(propertyList);
		}
	}
	
	if (propertyList->isEmpty() == false)
	{
		switch (g_Cfg.m_iTooltipMode)
		{
		case TOOLTIPMODE_SENDVERSION:
            // If a full tooltip was requested (fRequested), or this object is in a shop window (fNameOnly), we need to
            // send the full tooltip and not the version. In the fRequested case, we may have been asked for the full tooltip via scripts,
            // or forcing in the source the full tooltip or because the client asked it because we sent him a newer tooltip version.
            // In the shop window case, if we send the property version instead of the list, sometimes wrong names are shown. This
            // happens especially when using a client localization other than english.
			if (!fRequested && !fNameOnly)
			{
				// send property list version (client will send a request for the full tooltip if needed)
				if (PacketPropertyListVersion::CanSendTo(GetNetState()) == false)
					new PacketPropertyListVersionOld(this, pObj, propertyList->getVersion());
				else
					new PacketPropertyListVersion(this, pObj, propertyList->getVersion());

				break;
			}

			// fall through to send full list
			FALLTHROUGH;

		default:
			FALLTHROUGH;
		case TOOLTIPMODE_SENDFULL:
			// send full property list
			new PacketPropertyList(this, propertyList);
			break;
		}
	}

	// delete the original packet, as long as it doesn't belong
	// to the object (i.e. wasn't cached)
	if (propertyList != pObj->GetPropertyList())
		delete propertyList;

    return true;
}

void CClient::AOSTooltip_addName(CObjBase* pObj)
{
	CItem *pItem = pObj->IsItem() ? static_cast<CItem *>(pObj) : nullptr;
	CChar *pChar = pObj->IsChar() ? static_cast<CChar *>(pObj) : nullptr;
	CClientTooltip* t = nullptr;

	dword dwClilocName = (dword)(pObj->GetDefNum("NAMELOC", true));

	if (pItem)
	{
		if ( dwClilocName )
		{
            PUSH_FRONT_TOOLTIP(pItem, new CClientTooltip(dwClilocName));
		}
		else if ( (pItem->GetAmount() > 1) && (pItem->GetType() != IT_CORPSE) )
		{
            PUSH_FRONT_TOOLTIP(pItem, t = new CClientTooltip(1050039)); // ~1_NUMBER~ ~2_ITEMNAME~
			t->FormatArgs("%" PRIu16 "\t%s", pItem->GetAmount(), pObj->GetName());
		}
		else
		{
            PUSH_FRONT_TOOLTIP(pItem, t = new CClientTooltip(1042971)); // ~1_NOTHING~
            t->FormatArgs("%s", pObj->GetName());
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
        Str_CopyLimitNull(lpSuffix, pChar->GetKeyStr("NAME.SUFFIX"), STR_TEMPLENGTH);

		const CStoneMember * pGuildMember = pChar->Guild_FindMember(MEMORY_GUILD);
		if (pGuildMember && (!pChar->IsStatFlag(STATF_INCOGNITO) || GetPrivLevel() > pChar->GetPrivLevel()))
		{
			const CItemStone * pParentStone = pGuildMember->GetParentStone();
			ASSERT(pParentStone != nullptr);

			if (pGuildMember->IsAbbrevOn())
			{
				lpctstr ptcAbbrev = pParentStone->GetAbbrev();
				if (ptcAbbrev[0])
				{
					Str_ConcatLimitNull(lpSuffix, " [", STR_TEMPLENGTH);
					Str_ConcatLimitNull(lpSuffix, ptcAbbrev, STR_TEMPLENGTH);
					Str_ConcatLimitNull(lpSuffix, "]", STR_TEMPLENGTH);
				}
			}
		}

		if (*lpSuffix == '\0')
            Str_CopyLimitNull(lpSuffix, " ", STR_TEMPLENGTH);

		// The name
        PUSH_FRONT_TOOLTIP(pChar, t = new CClientTooltip(1050045)); // ~1_PREFIX~~2_NAME~~3_SUFFIX~
		if (dwClilocName)
			t->FormatArgs("%s\t%u\t%s", lpPrefix, dwClilocName, lpSuffix);
		else
			t->FormatArgs("%s\t%s\t%s", lpPrefix, pObj->GetName(), lpSuffix);

		// Need to find a way to get the ushort inside hues.mul for index wHue to get this working.
		// t->FormatArgs("<basefont color=\"#%02x%02x%02x\">%s\t%s\t%s</basefont>",
		//	(byte)((((int)wHue) & 0x7C00) >> 7), (byte)((((int)wHue) & 0x3E0) >> 2),
		//	(byte)((((int)wHue) & 0x1F) << 3),lpPrefix, pObj->GetName(), lpSuffix); // ~1_PREFIX~~2_NAME~~3_SUFFIX~

		if (!pChar->IsStatFlag(STATF_INCOGNITO) || (GetPrivLevel() > pChar->GetPrivLevel()))
		{
			if (pGuildMember && pGuildMember->IsAbbrevOn())
			{
				if (pGuildMember->GetTitle()[0])
				{
                    PUSH_FRONT_TOOLTIP(pChar, t = new CClientTooltip(1060776)); // ~1_val~, ~2_val~
					t->FormatArgs("%s\t%s", pGuildMember->GetTitle(), pGuildMember->GetParentStone()->GetName());
				}
				else
				{
                    PUSH_FRONT_TOOLTIP(pChar, new CClientTooltip(1070722, pGuildMember->GetParentStone()->GetName())); // ~1_NOTHING~
				}
			}
		}
	}
}

void CClient::AOSTooltip_addDefaultCharData(CChar * pChar)
{
	CClientTooltip* t = nullptr;

	if (pChar->m_pPlayer)
	{
		if (pChar->IsPriv(PRIV_GM) && !pChar->IsPriv(PRIV_PRIV_NOSHOW))
            PUSH_BACK_TOOLTIP(pChar, new CClientTooltip(1018085)); // Game Master
	}
	else if (pChar->m_pNPC)
	{
		if (g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE)
		{
			CREID_TYPE id = pChar->GetID();
			if (id == CREID_LLAMA_PACK || id == CREID_HORSE_PACK || id == CREID_GIANT_BEETLE)
			{
				int iWeight = pChar->GetWeight() / WEIGHT_UNITS;
                PUSH_BACK_TOOLTIP(pChar, t = new CClientTooltip(iWeight == 1 ? 1072788 : 1072789)); // Weight: ~1_WEIGHT~ stone / Weight: ~1_WEIGHT~ stones
				t->FormatArgs("%d", iWeight);
			}

			if (pChar->Skill_GetActive() == NPCACT_GUARD_TARG)
                PUSH_BACK_TOOLTIP(pChar, new CClientTooltip(1080078)); // guarding
		}

		if (pChar->IsStatFlag(STATF_CONJURED))
            PUSH_BACK_TOOLTIP(pChar, new CClientTooltip(1049646)); // (summoned)
		else if (pChar->IsStatFlag(STATF_PET))
            PUSH_BACK_TOOLTIP(pChar, new CClientTooltip(pChar->m_pNPC->m_bonded ? 1049608 : 502006)); // (bonded) / (tame)
	}
}

void CClient::AOSTooltip_addDefaultItemData(CItem * pItem)
{
	// TODO: add check to ATTR_IDENTIFIED, then add tooltips: stolen, BonusSkill1/2/3/4/5, minlevel/maxlevel, shurikencount

    //const CCPropsItemChar *pCCPItemChar = pItem->GetCCPropsItemChar(), *pBaseCCPItemChar = pItem->Base_GetDef()->GetCCPropsItemChar
	const auto pCCPItemEquip = pItem->GetComponentProps<CCPropsItemEquippable>();
	const auto pBaseCCPItemEquip = pItem->Base_GetDef()->GetComponentProps<CCPropsItemEquippable>();
	CClientTooltip* t = nullptr;

	if (pItem->IsAttr(ATTR_LOCKEDDOWN))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(501643)); // Locked Down
	if (pItem->IsAttr(ATTR_SECURE))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(501644)); // Locked Down & Secured
	if (pItem->IsAttr(ATTR_BLESSED))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1038021)); // Blessed
	if (pItem->IsAttr(ATTR_CURSED))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1049643)); // Cursed
	if (pItem->IsAttr(ATTR_INSURED))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1061682)); // <b>Insured</b>
	if (pItem->IsAttr(ATTR_QUESTITEM))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1072351)); // Quest Item
	if (pItem->IsAttr(ATTR_MAGIC))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(3010064)); // Magic
	if (pItem->IsAttr(ATTR_NEWBIE))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1070722, g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_NEWBIE))); // ~1_NOTHING~
    if (pItem->IsAttr(ATTR_NODROP))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1076253)); // NO-DROP
    if (pItem->IsAttr(ATTR_NOTRADE))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1076255)); // NO-TRADE

    if (pItem->IsCanUse(CAN_U_ELF) && !pItem->IsCanUse(CAN_U_HUMAN|CAN_U_GARGOYLE))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1154650)); // Elves Only
    else if (pItem->IsCanUse(CAN_U_GARGOYLE) && !pItem->IsCanUse(CAN_U_HUMAN|CAN_U_ELF))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1111709)); // Gargoyles Only
    /* // Not used by OSI?
    else if (pItem->IsCanUse(CAN_U_HUMAN) && pItem->IsCanUse(CAN_U_GARGOYLE) && !pItem->IsCanUse(CAN_U_ELF))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1154651)); // Exclude Elves Only
    else if (pItem->IsCanUse(CAN_U_HUMAN) && pItem->IsCanUse(CAN_U_ELF) && !pItem->IsCanUse(CAN_U_GARGOYLE))
        PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1154649)); // Exclude Gargoyles Only
    */

	if (g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE)
	{
		if (pItem->IsMovable())
		{
			int iWeight = pItem->GetWeight() / WEIGHT_UNITS;
            PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(iWeight == 1 ? 1072788 : 1072789)); // Weight: ~1_WEIGHT~ stone / Weight: ~1_WEIGHT~ stones
			t->FormatArgs("%d", iWeight);
		}
	}

	const CChar *pCraftsman = CUID::CharFindFromUID(dword(pItem->GetDefNum("CRAFTEDBY")));
	if (pCraftsman)
	{
        PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1050043)); // crafted by ~1_NAME~
		t->FormatArgs("%s", pCraftsman->GetName());
	}

	if (pItem->IsAttr(ATTR_EXCEPTIONAL))
		PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1060636)); // exceptional

	int64 ArtifactRarity = pItem->GetDefNum("RARITY", true);
	if (ArtifactRarity > 0)
	{
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1061078)); // artifact rarity ~1_val~
		t->FormatArgs("%" PRId64, ArtifactRarity);
	}

	int64 UsesRemaining = pItem->GetDefNum("USESCUR", true);
	if (UsesRemaining > 0)
	{
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060584)); // uses remaining: ~1_val~
		t->FormatArgs("%" PRId64, UsesRemaining);
	}

	if (pItem->IsTypeArmorWeapon())
	{
		int64 SelfRepair = pItem->GetDefNum("SELFREPAIR", true);
		if (SelfRepair != 0)
		{
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060450)); // self repair ~1_val~
			t->FormatArgs("%" PRId64, SelfRepair);
		}
	}


	// Some type specific default stuff

	switch (pItem->GetType())
	{
	case IT_CONTAINER_LOCKED:
		PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(3005142)); // Locked
		break;
	case IT_CONTAINER:
	case IT_TRASH_CAN:
		if (pItem->IsContainer())
		{
			const CContainer * pContainer = dynamic_cast <const CContainer *> (pItem);
			ASSERT(pContainer);
			if ( g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE )
			{
				if ( pItem->m_ModMaxWeight )
				{
                    PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1072241)); // Contents: ~1_COUNT~/~2_MAXCOUNT~ items, ~3_WEIGHT~/~4_MAXWEIGHT~ stones
					t->FormatArgs("%" PRIuSIZE_T "\t%d\t%d\t%d", pContainer->GetContentCount(), g_Cfg.m_iContainerMaxItems, pContainer->GetTotalWeight() / WEIGHT_UNITS, pItem->m_ModMaxWeight / WEIGHT_UNITS);
				}
				else
				{
                    PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1073841)); // Contents: ~1_COUNT~/~2_MAXCOUNT~ items, ~3_WEIGHT~ stones
					t->FormatArgs("%" PRIuSIZE_T "\t%d\t%d", pContainer->GetContentCount(), g_Cfg.m_iContainerMaxItems, pContainer->GetTotalWeight() / WEIGHT_UNITS);
				}
			}
			else
			{
                PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1050044));
				t->FormatArgs("%" PRIuSIZE_T "\t%d", pContainer->GetContentCount(), pContainer->GetTotalWeight() / WEIGHT_UNITS); // ~1_COUNT~ items, ~2_WEIGHT~ stones
			}
		}
		break;

	case IT_ARMOR_LEATHER:
	case IT_ARMOR:
	case IT_CLOTHING:
	case IT_SHIELD:
	{
        if (!IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
        {
            int iArmorRating = pItem->Armor_GetDefense();
			int iPercentArmorRating = 0;
			if (g_Cfg.m_fDisplayPercentAr)
			{
				iPercentArmorRating = CChar::CalcPercentArmorDefense(pItem->Item_GetDef()->GetEquipLayer());
				iArmorRating = IMulDivDown(iArmorRating, iPercentArmorRating, 100);
			}
            if (iArmorRating != 0)
            {
                // Obsolete AR was replaced by physical/fire/cold/poison/energy resist since AOS
                // and doesn't even have proper tooltips. It's just there for backward compatibility
                PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
				t->FormatArgs("%s\t%d", g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_ARMOR), iArmorRating);
            }
        }

		int64 StrengthRequirement = pItem->Item_GetDef()->m_ttEquippable.m_iStrReq;
        if (pCCPItemEquip || pBaseCCPItemEquip)
            StrengthRequirement -= pItem->GetPropNum(pCCPItemEquip, PROPIEQUIP_LOWERREQ, pBaseCCPItemEquip);
		if (StrengthRequirement > 0)
		{
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1061170)); // strength requirement ~1_val~
			t->FormatArgs("%" PRId64, StrengthRequirement);
		}

        if ( pItem->m_itArmor.m_wHitsMax > 0 )
        {
		    PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060639)); // durability ~1_val~ / ~2_val~
		    t->FormatArgs("%hu\t%hu", pItem->m_itArmor.m_dwHitsCur, pItem->m_itArmor.m_wHitsMax);
        }
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
    case IT_WEAPON_WHIP:
	{
		if (pItem->m_itWeapon.m_poison_skill)
			PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1017383)); // poisoned

		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1061168)); // weapon damage ~1_val~ - ~2_val~
		t->FormatArgs("%d\t%d", pItem->m_attackBase + pItem->m_ModAr, pItem->Weapon_GetAttack(true));

		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1061167)); // weapon speed ~1_val~
		t->FormatArgs("%hhu", pItem->GetSpeed());

		uchar Range = pItem->GetRangeH();
		if (Range > 1)
		{
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1061169, Range)); // range ~1_val~
		}

		int64 StrengthRequirement = (int64)(pItem->Item_GetDef()->m_ttEquippable.m_iStrReq) - pItem->GetPropNum(pCCPItemEquip, PROPIEQUIP_LOWERREQ, pBaseCCPItemEquip);
		if (StrengthRequirement > 0)
		{
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1061170, StrengthRequirement)); // strength requirement ~1_val~
		}

		if (pItem->Item_GetDef()->GetEquipLayer() == LAYER_HAND2)
			PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1061171)); // two-handed weapon
		else
			PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1061824)); // one-handed weapon

		if (!pItem->GetPropNum(COMP_PROPS_ITEMWEAPON, PROPIWEAP_USEBESTWEAPONSKILL, true))
		{
			switch (pItem->Weapon_GetSkill())
			{
			case SKILL_SWORDSMANSHIP:	PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1061172));	break; // skill required: swordsmanship
			case SKILL_MACEFIGHTING:	PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1061173));	break; // skill required: mace fighting
			case SKILL_FENCING:			PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1061174));	break; // skill required: fencing
			case SKILL_ARCHERY:			PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1061175));	break; // skill required: archery
			case SKILL_THROWING:		PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1112075));	break; // skill required: throwing
			default:					break;
			}
		}

        if ( pItem->m_itWeapon.m_wHitsMax > 0 )
        {
		    PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060639)); // durability ~1_val~ / ~2_val~
		    t->FormatArgs("%hu\t%hu", pItem->m_itWeapon.m_dwHitsCur, pItem->m_itWeapon.m_wHitsMax);
        }
	}
	break;

	case IT_WAND:
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1054132)); // [charges: ~1_charges~]
		t->FormatArgs("%d", pItem->m_itWeapon.m_spellcharges);
		break;

	case IT_TELEPAD:
	case IT_MOONGATE:
		if (this->IsPriv(PRIV_GM))
		{
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
			t->FormatArgs("%s\t%s", g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_DESTINATION), pItem->m_itTelepad.m_ptMark.WriteUsed());
		}
		break;

	case IT_RUNE:
	{
		const CPointMap& pt = pItem->m_itTelepad.m_ptMark;
		if (!pt.IsValidPoint())
			break;

		lpctstr regionName = g_Cfg.GetDefaultMsg(DEFMSG_RUNE_LOCATION_UNK);
		if (pt.GetRegion(REGION_TYPE_AREA))
			regionName = pt.GetRegion(REGION_TYPE_AREA)->GetName();
		bool regionMulti = (pt.GetRegion(REGION_TYPE_MULTI) != nullptr);

		if (pt.m_map == 0)
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(regionMulti ? 1062452 : 1060805)); // ~1_val~ (Felucca)[(House)]
		else if (pt.m_map == 1)
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(regionMulti ? 1062453 : 1060806)); // ~1_val~ (Trammel)[(House)]
		else if (pt.m_map == 3)
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(regionMulti ? 1062454 : 1060804)); // ~1_val~ (Malas)[(House)]
		else if (pt.m_map == 4)
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(regionMulti ? 1063260 : 1063259)); // ~1_val~ (Tokuno Islands)[(House)]
		else if (pt.m_map == 5)
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(regionMulti ? 1113206 : 1113205)); // ~1_val~ (Ter Mur)[(House)]
		else
			// There's no proper clilocs for Ilshenar (map2) and custom facets (map > 5), so let's use a generic cliloc
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1042971)); // ~1_NOTHING~

		t->FormatArgs("%s %s", g_Cfg.GetDefaultMsg(DEFMSG_RUNE_TO), regionName);
	}
	break;

	case IT_SPELLBOOK:
	case IT_SPELLBOOK_EXTRA:
	case IT_SPELLBOOK_NECRO:
	case IT_SPELLBOOK_PALA:
	case IT_SPELLBOOK_BUSHIDO:
	case IT_SPELLBOOK_NINJITSU:
	case IT_SPELLBOOK_ARCANIST:
	case IT_SPELLBOOK_MYSTIC:
	case IT_SPELLBOOK_MASTERY:
	{
		int count = pItem->GetSpellcountInBook();
		if (count > 0)
		{
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1042886)); // ~1_NUMBERS_OF_SPELLS~ Spells
			t->FormatArgs("%d", count);
		}
	} break;

	case IT_SPAWN_CHAR:
	{

		CCSpawn* pSpawn = static_cast<CCSpawn*>(pItem->GetComponent(COMP_SPAWN));
		if (!pSpawn)
            break;
        CResourceDef * pSpawnCharDef = g_Cfg.ResourceGetDef(pSpawn->GetSpawnID());
		lpctstr pszName = nullptr;
		if (pSpawnCharDef)
		{
			CCharBase *pCharBase = dynamic_cast<CCharBase*>(pSpawnCharDef);
			if (pCharBase)
				pszName = pCharBase->GetTradeName();
			else
				pszName = pSpawnCharDef->GetName();

			while (*pszName == '#')
				++pszName;
		}

		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
		t->FormatArgs("Character\t%s", pszName ? pszName : "none");
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1061169)); // range ~1_val~
		t->FormatArgs("%hhu", pSpawn->GetDistanceMax());
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1074247)); // Live Creatures: ~1_NUM~ / ~2_MAX~
		t->FormatArgs("%hhu\t%hu", pSpawn->GetCurrentSpawned(), pSpawn->GetAmount());
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060659)); // ~1_val~: ~2_val~
		t->FormatArgs("Time range\t%hu min / %hu max", pSpawn->GetTimeLo(), pSpawn->GetTimeHi());
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060660)); // ~1_val~: ~2_val~
		t->FormatArgs("Time until next spawn\t%" PRId64 " sec", pItem->GetTimerSAdjusted());
		
	} break;

	case IT_SPAWN_ITEM:
	{
		CCSpawn* pSpawn = static_cast<CCSpawn*>(pItem->GetComponent(COMP_SPAWN));
        if (!pSpawn)
            break;
		CResourceDef * pSpawnItemDef = g_Cfg.ResourceGetDef(pSpawn->GetSpawnID());

		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
		t->FormatArgs("Item\t%u %s", pSpawn->GetPile(), pSpawnItemDef ? pSpawnItemDef->GetName() : "none");
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1061169)); // range ~1_val~
		t->FormatArgs("%hhu", pSpawn->GetDistanceMax());
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1074247)); // Live Creatures: ~1_NUM~ / ~2_MAX~
		t->FormatArgs("%hhu\t%hu", pSpawn->GetCurrentSpawned(), pSpawn->GetAmount());
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060659)); // ~1_val~: ~2_val~
		t->FormatArgs("Time range\t%hu min / %hu max", pSpawn->GetTimeLo(), pSpawn->GetTimeHi());
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060660)); // ~1_val~: ~2_val~
		t->FormatArgs("Time until next spawn\t%" PRId64 " sec", pItem->GetTimerSAdjusted());
	} break;

	case IT_COMM_CRYSTAL:
	{
		CItem *pLink = pItem->m_uidLink.ItemFind();
		PUSH_BACK_TOOLTIP(pItem, new CClientTooltip((pLink && pLink->IsType(IT_COMM_CRYSTAL)) ? 1060742 : 1060743)); // active / inactive
		PUSH_BACK_TOOLTIP(pItem, new CClientTooltip(1060745)); // broadcast
	} break;

	case IT_STONE_GUILD:
	{
		pItem->m_TooltipData.clear();
		PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1041429)); // a guildstone
		const CItemStone *thisStone = static_cast<const CItemStone *>(pItem);
		if (thisStone)
		{
			PUSH_BACK_TOOLTIP(pItem, t = new CClientTooltip(1060802)); // Guild name: ~1_val~
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
