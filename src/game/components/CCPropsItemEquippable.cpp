
#include "CCPropsItemEquippable.h"
#include "../items/CItem.h"
#include "../chars/CChar.h"


lpctstr const CCPropsItemEquippable::_ptcPropertyKeys[PROPIEQUIP_QTY + 1] =
{
    #define ADDPROP(a,b,c) b,
    #include "../../tables/CCPropsItemEquippable_props.tbl"
    #undef ADDPROP
    nullptr
};
KeyTableDesc_s CCPropsItemEquippable::GetPropertyKeysData() const {
    return {_ptcPropertyKeys, (PropertyIndex_t)CountOf(_ptcPropertyKeys)};
}

RESDISPLAY_VERSION CCPropsItemEquippable::_iPropertyExpansion[PROPIEQUIP_QTY + 1] =
{
#define ADDPROP(a,b,c) c,
#include "../../tables/CCPropsItemEquippable_props.tbl"
#undef ADDPROP
    RDS_QTY
};

CCPropsItemEquippable::CCPropsItemEquippable() : CComponentProps(COMP_PROPS_ITEMEQUIPPABLE)
{
}

bool CCPropsItemEquippable::CanSubscribe(const CItemBase* pItemBase) // static
{
    return pItemBase->IsTypeEquippable();
}

bool CCPropsItemEquippable::CanSubscribe(const CItem* pItem) // static
{
    return pItem->IsTypeEquippable();
}


lpctstr CCPropsItemEquippable::GetPropertyName(PropertyIndex_t iPropIndex) const
{
    ASSERT(iPropIndex < PROPIEQUIP_QTY);
    return _ptcPropertyKeys[iPropIndex];
}

bool CCPropsItemEquippable::IsPropertyStr(PropertyIndex_t iPropIndex) const
{
    switch (iPropIndex)
    {
        case PROPIEQUIP_HITSPELL:
            return true;
        default:
            return false;
    }
}


/* The idea is: 
    - Changed: Elemental damages and resistances (RESFIRE, DAMFIRE...) are now loaded only if COMBAT_ELEMENTAL_ENGINE is on, otherwise those keywords will be IGNORED. Bear in mind that if you have created
        equippable items when that flag was off and you decide to turn it on later, you'll need to RECREATE all of those ITEMS in order to make them load the elemental properties.
    We'll use this method when we have figured how to avoid possible bugs when equipping and unequipping items and switching COMBAT_ELEMENTAL_ENGINE on and off.
*/
bool CCPropsItemEquippable::IgnoreElementalProperty(PropertyIndex_t iPropIndex) // static
{
    if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
        return false;
    switch ( PROPIEQUIP_TYPE(iPropIndex) )
    {
        case PROPIEQUIP_DAMCHAOS:
        case PROPIEQUIP_DAMCOLD:
        case PROPIEQUIP_DAMDIRECT:
        case PROPIEQUIP_DAMENERGY:
        case PROPIEQUIP_DAMFIRE:
        case PROPIEQUIP_DAMPHYSICAL:
        case PROPIEQUIP_DAMPOISON:
        case PROPIEQUIP_HITAREACOLD:
        case PROPIEQUIP_HITAREAENERGY:
        case PROPIEQUIP_HITAREAFIRE:
        case PROPIEQUIP_HITAREAPHYSICAL:
        case PROPIEQUIP_HITAREAPOISON:
        case PROPIEQUIP_RESCOLD:
        case PROPIEQUIP_RESCOLDMAX:
        case PROPIEQUIP_RESENERGY:
        case PROPIEQUIP_RESENERGYMAX:
        case PROPIEQUIP_RESFIRE:
        case PROPIEQUIP_RESFIREMAX:
            return true;
    }
    return false;
}


bool CCPropsItemEquippable::GetPropertyNumPtr(PropertyIndex_t iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CCPropsItemChar::GetPropertyNumPtr");
    ASSERT(!IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyNum(&_mPropsNum, iPropIndex, piOutVal);
}

bool CCPropsItemEquippable::GetPropertyStrPtr(PropertyIndex_t iPropIndex, CSString* psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CCPropsItemEquippable::GetPropertyStrPtr");
    ASSERT(IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyStr(&_mPropsStr, iPropIndex, psOutVal, fZero);
}

void CCPropsItemEquippable::SetPropertyNum(PropertyIndex_t iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::SetPropertyNum");
    ASSERT(!IsPropertyStr(iPropIndex));
    ASSERT((iLimitToExpansion >= RDS_PRET2A) && (iLimitToExpansion < RDS_QTY));

    if ((fDeleteZero && (iVal == 0)) || (_iPropertyExpansion[iPropIndex] > iLimitToExpansion) /*|| IgnoreElementalProperty(iPropIndex)*/)
    {
        if (0 == _mPropsNum.erase(iPropIndex))
            return; // I didn't have this property, so avoid further processing.
    }
    else
    {
        _mPropsNum[iPropIndex] = iVal;
        //_mPropsNum.container.shrink_to_fit();
    }

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
}

void CCPropsItemEquippable::SetPropertyStr(PropertyIndex_t iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::SetPropertyStr");
    ASSERT(ptcVal);
    ASSERT(IsPropertyStr(iPropIndex));
    ASSERT((iLimitToExpansion >= RDS_PRET2A) && (iLimitToExpansion < RDS_QTY));

    if ((fDeleteZero && (*ptcVal == '\0')) || (_iPropertyExpansion[iPropIndex] > iLimitToExpansion) /*|| IgnoreElementalProperty(iPropIndex)*/)
    {
        if (0 == _mPropsNum.erase(iPropIndex))
            return; // I didn't have this property, so avoid further processing.
    }
    else
    {
        _mPropsStr[iPropIndex] = ptcVal;
        //_mPropsStr.container.shrink_to_fit();
    }

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
}

void CCPropsItemEquippable::DeletePropertyNum(PropertyIndex_t iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::DeletePropertyNum");
    _mPropsNum.erase(iPropIndex);
}

void CCPropsItemEquippable::DeletePropertyStr(PropertyIndex_t iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::DeletePropertyStr");
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsItemEquippable::FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, PropertyIndex_t iPropIndex, bool fPropStr)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::FindLoadPropVal");
    if (!fPropStr && (*s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(iPropIndex);
        return true;
    }

    BaseProp_LoadPropVal(iPropIndex, fPropStr, s, pLinkedObj, iLimitToExpansion);
    return true;
}

bool CCPropsItemEquippable::FindWritePropVal(CSString & sVal, PropertyIndex_t iPropIndex, bool fPropStr) const
{
    ADDTOCALLSTACK("CCPropsItemEquippable::FindWritePropVal");

    return BaseProp_WritePropVal(iPropIndex, fPropStr, sVal);
}

void CCPropsItemEquippable::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::r_Write");
    // r_Write isn't called by CItemBase/CCharBase, so we don't get base props saved
    BaseCont_Write_ContNum(&_mPropsNum, _ptcPropertyKeys, s);
    BaseCont_Write_ContStr(&_mPropsStr, _ptcPropertyKeys, s);
}

void CCPropsItemEquippable::Copy(const CComponentProps * target)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::Copy");
    const CCPropsItemEquippable *pTarget = static_cast<const CCPropsItemEquippable*>(target);
    if (!pTarget)
    {
        return;
    }
    _mPropsNum = pTarget->_mPropsNum;
    _mPropsStr = pTarget->_mPropsStr;
}

void CCPropsItemEquippable::AddPropsTooltipData(CObjBase* pLinkedObj)
{
#define TOOLTIP_APPEND(t)   pLinkedObj->m_TooltipData.emplace_back(t)
#define ADDT(tooltipID)     TOOLTIP_APPEND(new CClientTooltip(tooltipID))
#define ADDTNUM(tooltipID)  TOOLTIP_APPEND(new CClientTooltip(tooltipID, iVal))
#define ADDTSTR(tooltipID)  TOOLTIP_APPEND(new CClientTooltip(tooltipID, ptcVal))

    /* Tooltips for "dynamic" properties (stored in the BaseConts: _mPropsNum and _mPropsStr) */

    // Numeric properties
    for (const BaseContNumPair_t& propPair : _mPropsNum)
    {
        PropertyIndex_t prop = propPair.first;
        PropertyValNum_t iVal = propPair.second;

        if (iVal == 0)
            continue;

        switch (prop)
        {
            case PROPIEQUIP_ALTERED:
                ADDT(1111880); // Altered
                break;
            case PROPIEQUIP_ANTIQUE:
                ADDT(1152714); // Antique
                break;
            case PROPIEQUIP_BONUSBERSERK: // unimplemented
                ADDTNUM(1151541); // Berserk ~1_VAL~
                break;
            case PROPIEQUIP_BONUSDEX:
                ADDTNUM(1060409); // dexterity bonus ~1_val~
                break;
            case PROPIEQUIP_BONUSDURABILITY:
                ADDTNUM(1060410); // durability ~1_val~%
                break;
            case PROPIEQUIP_BONUSHITSMAX:
                ADDTNUM(1060431); // hit point increase ~1_val~%
                break;
            case PROPIEQUIP_BONUSINT:
                ADDTNUM(1060432); // intelligence bonus ~1_val~
                break;
            case PROPIEQUIP_BONUSMANAMAX:
                ADDTNUM(1060439); // mana increase ~1_val~
                break;
            case PROPIEQUIP_BONUSSTAMMAX:
                ADDTNUM(1060484); // stamina increase ~1_val~
                break;
            case PROPIEQUIP_BONUSSTR:
                ADDTNUM(1060485); // strength bonus ~1_val~
                break;
            case PROPIEQUIP_BRITTLE:
                ADDT(1116209); // Brittle
                break;
            case PROPIEQUIP_CASTINGFOCUS: // unimplemented
                ADDTNUM(1113696);
                break;
            case PROPIEQUIP_COMBATBONUSPERCENT: // Implemented the 15/03/2015 by Xun (Sphere 56c), it determines the % of the stat used for bonus combat damage.
                //Missing cliloc id
                break;
            case PROPIEQUIP_COMBATBONUSSTAT: // Implemented the 15/03/2015 by Xun (Sphere 56c), it determines what stat (Str, Dex, or Int) is used for bonus combat damage.
                //Missing cliloc id
                break;
            case PROPIEQUIP_DAMCHAOS: // unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1072846); // chaos damage ~1_val~%
                break;
            case PROPIEQUIP_DAMCOLD:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060404); // cold damage ~1_val~%
                break;
            case PROPIEQUIP_DAMDIRECT: // unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE)) // ?
                    ADDTNUM(1079978); // direct damage: ~1_PERCENT~%
                break;
            case PROPIEQUIP_DAMENERGY:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060407); // energy damage ~1_val~%
                break;
            case PROPIEQUIP_DAMFIRE:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060405); // fire damage ~1_val~%
                break;
            case PROPIEQUIP_DAMPHYSICAL:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060403); // physical damage ~1_val~%
                break;
            case PROPIEQUIP_DAMPOISON:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060406); // poison damage ~1_val~%
                break;
            case PROPIEQUIP_EATERCOLD: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1113594); // Cold Eater ~1_Val~%
                break;
            case PROPIEQUIP_EATERDAM: // Unimplemented (what's this for?)
                // Missing cliloc id
                break;
            case PROPIEQUIP_EATERENERGY: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1113596); // Energy Eater ~1_Val~%
                break;
            case PROPIEQUIP_EATERFIRE: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1113593); // Fire Eater ~1_Val~%
                break;
            case PROPIEQUIP_EATERKINETIC: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))   
                    ADDTNUM(1113597); // Kinetic Eater ~1_Val~%
                break;
            case PROPIEQUIP_EATERPOISON: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1113595); // Poison Eater ~1_Val~%
                break;
            case PROPIEQUIP_ENHANCEPOTIONS:
                ADDTNUM(1060411); // enhance potions ~1_val~%
                break;
            case PROPIEQUIP_EPHEMERAL: // Unimplemented
                ADDT(1153085); // Moonbound: Ephemeral
                break;
            case PROPIEQUIP_FASTERCASTING:
                ADDTNUM(1060413); // faster casting ~1_val~
                break;
            case PROPIEQUIP_FASTERCASTRECOVERY: // unimplemented
                ADDTNUM(1060412); // faster cast recovery ~1_val~
                break;
            case PROPIEQUIP_HITAREACOLD: // unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060416); // hit cold area ~1_val~%
                break;
            case PROPIEQUIP_HITAREAENERGY: // unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060418); // hit energy area ~1_val~%
                break;
            case PROPIEQUIP_HITAREAFIRE: // unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060419); // hit fire area ~1_val~%
                break;
            case PROPIEQUIP_HITAREAPHYSICAL: // unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060428); // hit physical area ~1_val~%
                break;
            case PROPIEQUIP_HITAREAPOISON: // unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060429); // hit poison area ~1_val~%
                break;
            case PROPIEQUIP_HITCURSE: // unimplemented
                ADDTNUM(1113712); // Hit Curse ~1_val~%
                break;
            case PROPIEQUIP_HITDISPEL: // unimplemented
                ADDTNUM(1060417); // hit dispel ~1_val~%
                break;
            case PROPIEQUIP_HITFATIGUE: // unimplemented
                ADDTNUM(1113700); // // Hit Fatigue ~1_val~%
                break;
            case PROPIEQUIP_HITFIREBALL: // unimplemented
                ADDTNUM(1060420); // hit fireball ~1_val~%
                break;
            case PROPIEQUIP_HITHARM: // unimplemented
                ADDTNUM(1060421); // hit harm ~1_val~%
                break;
            case PROPIEQUIP_HITLEECHLIFE:
                ADDTNUM(1060422); // hit life leech ~1_val~%
                break;
            case PROPIEQUIP_HITLEECHMANA:
                ADDTNUM(1060427); // hit mana leech ~1_val~%
                break;
            case PROPIEQUIP_HITLEECHSTAM:
                ADDTNUM(1060430); // hit stamina leech ~1_val~%
                break;
            case PROPIEQUIP_HITLIGHTNING: // unimplemented
                ADDTNUM(1060422); // hit lightning ~1_val~%
                break;
            case PROPIEQUIP_HITLOWERATK: // unimplemented
                ADDTNUM(1060424); // hit lower attack ~1_val~%
                break;
            case PROPIEQUIP_HITLOWERDEF: // unimplemented
                ADDTNUM(1060425); // hit lower defense ~1_val~%
                break;
            case PROPIEQUIP_HITMAGICARROW: // unimplemented
                ADDTNUM(1060426); // hit magic arrow ~1_val~%
                break;
            case PROPIEQUIP_HITMANADRAIN: // unimplemented
                ADDTNUM(1113699); // Hit Mana Drain ~1_val~%
                break;
            case PROPIEQUIP_HITSPARKS: // unimplemented
                ADDTNUM(1157326); // Sparks ~1_val~%
                break;
            case PROPIEQUIP_HITSPELLSTR: // unimplemented
                // Missing cliloc id
                break;
            case PROPIEQUIP_HITSWARM: // unimplemented
                ADDTNUM(1157325); // Swarm ~1_val~%
                break;
            case PROPIEQUIP_IMBUED: // unimplemented
                ADDT(1080418); // (Imbued)
                break;
            case PROPIEQUIP_INCREASEDAM:
                ADDTNUM(1060401); // damage increase ~1_val~%
                break;
            case PROPIEQUIP_INCREASEDEFCHANCE:
                ADDTNUM(1060408); // defense chance increase ~1_val~%
                break;
            case PROPIEQUIP_INCREASEDEFCHANCEMAX: // unimplemented -> it should raise char's prop
                //Missing cliloc id
                break;
            case PROPIEQUIP_INCREASEGOLD:
                ADDTNUM(1060414); // gold increase ~1_val~%
                break;
            case PROPIEQUIP_INCREASEHITCHANCE:
                ADDTNUM(1060415); // hit chance increase ~1_val~%
                break;
            case PROPIEQUIP_INCREASEKARMALOSS: // Unimplemented
                ADDTNUM(1075210); // increased karma loss ~1val~%
                break;
            case PROPIEQUIP_INCREASESPELLDAM:
                ADDTNUM(1060483); // spell damage increase ~1_val~%
                break;
            case PROPIEQUIP_INCREASESWINGSPEED:
                ADDTNUM(1060486); // swing speed increase ~1_val~%
                break;
            case PROPIEQUIP_LOWERAMMOCOST:    // Unimplemented
                ADDTNUM(1075208); // Lower Ammo Cost ~1_Percentage~%
                break;
            case PROPIEQUIP_LOWERMANACOST:
                ADDTNUM(1060433); // lower mana cost ~1_val~%
                break;
            case PROPIEQUIP_LOWERREAGENTCOST:
                ADDTNUM(1060434); // lower reagent cost ~1_val~%
                break;
            case PROPIEQUIP_LOWERREQ:
                ADDTNUM(1060435); // lower requirements ~1_val~%
                break;
            case PROPIEQUIP_LUCK:
                ADDTNUM(1060436); // luck ~1_val~
                break;
            case PROPIEQUIP_MAGEARMOR: // unimplemented
                ADDT(1060437); // mage armor
                break;
            case PROPIEQUIP_MASSIVE: // Unimplemented
                ADDTNUM(1038003);
                break;
            case PROPIEQUIP_NIGHTSIGHT:
                ADDT(1060441); // night sight
                break;
            case PROPIEQUIP_PRIZED: // unimplemented
                ADDT(1154910); // Prized
                break;
            case PROPIEQUIP_RAGEFOCUS: // Unimplemented
                //Missing cliloc id 1153788? Rage Attack
                break;
            case PROPIEQUIP_REACTIVEPARALYZE: // Unimplemented
                ADDT(1112364); // reactive paralyze
                break;
            case PROPIEQUIP_REFLECTPHYSICALDAM: // Unimplemented
                ADDTNUM(1060442); // reflect physical damage ~1_val~%
                break;
            case PROPIEQUIP_REGENFOOD: // unimplemented
                // Missing cliloc id (does not exist)
                break;
            case PROPIEQUIP_REGENHITS: // unimplemented
                ADDTNUM(1060444); // hit point regeneration ~1_val~
                break;
            case PROPIEQUIP_REGENMANA: // unimplemented
                ADDTNUM(1060440); // mana regeneration ~1_val~
                break;
            case PROPIEQUIP_REGENSTAM: // unimplemented
                ADDTNUM(1060443); // stamina regeneration ~1_val~
                break;
            case PROPIEQUIP_REGENVALFOOD: // unimplemented
                // Missing cliloc id? (doesn't exist)
                break;
            case PROPIEQUIP_REGENVALHITS: // unimplemented
                // Missing cliloc id? (doesn't exist)
                break;
            case PROPIEQUIP_REGENVALMANA: // unimplemented
                // Missing cliloc id? (doesn't exist)
                break;
            case PROPIEQUIP_REGENVALSTAM: // unimplemented
                // Missing cliloc id? (doesn't exist)
                break;
            case PROPIEQUIP_RESCOLD:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060445); // cold resist ~1_val~%
                break;
            case PROPIEQUIP_RESCOLDMAX: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_RESENERGY:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060446); // energy resist ~1_val~%
                break;
            case PROPIEQUIP_RESENERGYMAX: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_RESFIRE:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060447); // fire resist ~1_val~%
                break;
            case PROPIEQUIP_RESFIREMAX: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_RESONANCECOLD: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_RESONANCEENERGY: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_RESONANCEFIRE: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_RESONANCEKINETIC: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_RESONANCEPOISON: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_RESPHYSICAL:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060448); // physical resist ~1_val~%
                break;
            case PROPIEQUIP_RESPHYSICALMAX: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_RESPOISON:
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1060449); // poison resist ~1_val~%
                break;
            case PROPIEQUIP_RESPOISONMAX: // Unimplemented
                //if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    //Missing cliloc id
                break;
            case PROPIEQUIP_SOULCHARGE: // Unimplemented
                    ADDTNUM(1113630); // Soul Charge ~1_val~%
                break;
            case PROPIEQUIP_SOULCHARGECOLD: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1113632); // Soul Charge : Cold : ~1_val~%
                break;
            case PROPIEQUIP_SOULCHARGEENERGY: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1113634); // Soul Charge : Energy : ~1_val~%
                break;
            case PROPIEQUIP_SOULCHARGEFIRE: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1113631); // Soul Charge : Fire : ~1_val~%
                break;
            case PROPIEQUIP_SOULCHARGEKINETIC: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1113635); // Soul Charge : Kinetic : ~1_val~%
                break;
            case PROPIEQUIP_SOULCHARGEPOISON: // Unimplemented
                if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
                    ADDTNUM(1113633); // Soul Charge : Poison : ~1_val~%
                break;
            case PROPIEQUIP_SPELLCHANNELING:
                ADDT(1060482); // spell channeling
                break;
            case PROPIEQUIP_SPELLCONSUMPTION: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_SPELLFOCUSING: // Unimplemented
                ADDT(1151391); // Spell Focusing
                break;
            case PROPIEQUIP_UNWIELDLY: // Unimplemented
                ADDT(1154909); // Unwieldly
                break;
        }
    }
    // End of Numeric properties


    // String properties
    for (const BaseContStrPair_t& propPair : _mPropsStr)
    {
        PropertyIndex_t prop = propPair.first;
        lpctstr ptcVal = propPair.second.GetBuffer();

        switch (prop)
        {
            case PROPIEQUIP_HITSPELL: // unimplemented
            {
                SPELL_TYPE iSpellNumber = SPELL_TYPE(g_Cfg.ResourceGetID(RES_SPELL, ptcVal, 0).GetResIndex());
                const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(iSpellNumber);
                if (!pSpellDef)
                    break;
                ptcVal = pSpellDef->GetName();
                ADDTSTR(1080129); // +~1_HIT_SPELL_EFFECT~% Hit Spell Effect.
                break;
            }
        }
    }
    // End of String properties
}
