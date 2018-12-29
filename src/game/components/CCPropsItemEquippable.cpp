
#include "CCPropsItemEquippable.h"
#include "../items/CItem.h"
#include "../chars/CChar.h"


lpctstr const CCPropsItemEquippable::_ptcPropertyKeys[PROPIEQUIP_QTY + 1] =
{
    #define ADD(a,b) b,
    #include "../../tables/CCPropsItemEquippable_props.tbl"
    #undef ADD
    nullptr
};
KeyTableDesc_s CCPropsItemEquippable::GetPropertyKeysData() const {
    return {_ptcPropertyKeys, (int)CountOf(_ptcPropertyKeys)};
}

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


lpctstr CCPropsItemEquippable::GetPropertyName(int iPropIndex) const
{
    ASSERT(iPropIndex < PROPIEQUIP_QTY);
    return _ptcPropertyKeys[iPropIndex];
}

bool CCPropsItemEquippable::IsPropertyStr(int iPropIndex) const
{
    switch (iPropIndex)
    {
        case PROPIEQUIP_HITSPELL:
            return true;
        default:
            return false;
    }
}

bool CCPropsItemEquippable::GetPropertyNumPtr(int iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CCPropsItemChar::GetPropertyNumPtr");
    ASSERT(!IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyNum(&_mPropsNum, iPropIndex, piOutVal);
}

bool CCPropsItemEquippable::GetPropertyStrPtr(int iPropIndex, CSString* psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CCPropsItemEquippable::GetPropertyStrPtr");
    ASSERT(IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyStr(&_mPropsStr, iPropIndex, psOutVal, fZero);
}

void CCPropsItemEquippable::SetPropertyNum(int iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::SetPropertyNum");

    ASSERT(!IsPropertyStr(iPropIndex));
    _mPropsNum[iPropIndex] = iVal;

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
}

void CCPropsItemEquippable::SetPropertyStr(int iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, bool fZero)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::SetPropertyStr");
    ASSERT(ptcVal);
    if (fZero && (*ptcVal == '\0'))
        ptcVal = "0";

    ASSERT(IsPropertyStr(iPropIndex));
    _mPropsStr[iPropIndex] = ptcVal;

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
}

void CCPropsItemEquippable::DeletePropertyNum(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::DeletePropertyNum");
    ASSERT(_mPropsNum.count(iPropIndex));
    _mPropsNum.erase(iPropIndex);
}

void CCPropsItemEquippable::DeletePropertyStr(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::DeletePropertyStr");
    ASSERT(_mPropsStr.count(iPropIndex));
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsItemEquippable::FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, int iPropIndex, bool fPropStr)
{
    ADDTOCALLSTACK("CCPropsItemEquippable::FindLoadPropVal");
    if (!fPropStr && (*s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(iPropIndex);
        return true;
    }

    BaseProp_LoadPropVal(iPropIndex, fPropStr, s, pLinkedObj);
    return true;
}

bool CCPropsItemEquippable::FindWritePropVal(CSString & sVal, int iPropIndex, bool fPropStr) const
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
        int prop = propPair.first;
        PropertyValNum_t iVal = propPair.second;

        if (iVal == 0)
            continue;

        switch (prop)
        {
            case PROPIEQUIP_BONUSDEX:
                ADDTNUM(1060409); // dexterity bonus ~1_val~
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
            case PROPIEQUIP_CASTINGFOCUS: // unimplemented
                // Missing cliloc id
                break;
            case PROPIEQUIP_COMBATBONUSPERCENT: // unimplemented -> it should raise char's prop
                //Missing cliloc id
                break;
            case PROPIEQUIP_COMBATBONUSSTAT: // unimplemented -> it should raise char's prop
                //Missing cliloc id
                break;
            case PROPIEQUIP_DAMCHAOS: // unimplemented
                ADDTNUM(1072846); // chaos damage ~1_val~%
                break;
            case PROPIEQUIP_DAMCOLD:
                ADDTNUM(1060404); // cold damage ~1_val~%
                break;
            case PROPIEQUIP_DAMDIRECT:
                ADDTNUM(1079978); // direct damage: ~1_PERCENT~%
                break;
            case PROPIEQUIP_DAMENERGY:
                ADDTNUM(1060407); // energy damage ~1_val~%
                break;
            case PROPIEQUIP_DAMFIRE:
                ADDTNUM(1060405); // fire damage ~1_val~%
                break;
            case PROPIEQUIP_DAMMODIFIER: // unimplemented
                // Missing cliloc id
                break;
            case PROPIEQUIP_DAMPHYSICAL:
                ADDTNUM(1060403); // physical damage ~1_val~%
                break;
            case PROPIEQUIP_DAMPOISON:
                ADDTNUM(1060406); // poison damage ~1_val~%
                break;
            case PROPIEQUIP_DECREASEHITCHANCE: // unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_EATERCOLD: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_EATERDAM: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_EATERENERGY: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_EATERFIRE: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_EATERKINETIC: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_EATERPOISON: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_ENHANCEPOTIONS:
                ADDTNUM(1060411); // enhance potions ~1_val~%
                break;
            case PROPIEQUIP_EPHEMERAL: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_FASTERCASTING:
                ADDTNUM(1060413); // faster casting ~1_val~
                break;
            case PROPIEQUIP_FASTERCASTRECOVERY: // unimplemented
                ADDTNUM(1060412); // faster cast recovery ~1_val~
                break;
            case PROPIEQUIP_HITAREACOLD: // unimplemented
                ADDTNUM(1060416); // hit cold area ~1_val~%
                break;
            case PROPIEQUIP_HITAREAENERGY: // unimplemented
                ADDTNUM(1060418); // hit energy area ~1_val~%
                break;
            case PROPIEQUIP_HITAREAFIRE: // unimplemented
                ADDTNUM(1060419); // hit fire area ~1_val~%
                break;
            case PROPIEQUIP_HITAREAPHYSICAL: // unimplemented
                ADDTNUM(1060428); // hit physical area ~1_val~%
                break;
            case PROPIEQUIP_HITAREAPOISON: // unimplemented
                ADDTNUM(1060429); // hit poison area ~1_val~%
                break;
            case PROPIEQUIP_HITCURSE: // unimplemented
                // Missing cliloc id
                break;
            case PROPIEQUIP_HITDISPEL: // unimplemented
                ADDTNUM(1060417); // hit dispel ~1_val~%
                break;
            case PROPIEQUIP_HITFATIGUE: // unimplemented
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
            case PROPIEQUIP_HITMANADRAIN: // unimplemented -> it should raise char's prop
                // Missing cliloc id
                break;
            case PROPIEQUIP_HITSPELLSTR: // unimplemented
                // Missing cliloc id
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
            case PROPIEQUIP_INCREASEGOLD: // Unimplemented
                //Missing cliloc id
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
                //Missing cliloc id
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
            case PROPIEQUIP_MANABURSTEVIL: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_MANABURSTGOOD: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_MANABURSTFREQUENCY: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_MANABURSTKARMA: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_MANAPHASE: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_MASSIVE: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_NIGHTSIGHT:
                ADDT(1060441); // night sight
                break;
            case PROPIEQUIP_RAGEFOCUS: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_REACTIVEPARALYZE: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_REFLECTPHYSICALDAM: // Unimplemented
                ADDTNUM(1060442); // reflect physical damage ~1_val~%
                break;
            case PROPIEQUIP_REGENFOOD: // unimplemented
                // Missing cliloc id
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
                // Missing cliloc id?
                break;
            case PROPIEQUIP_REGENVALHITS: // unimplemented
                // Missing cliloc id?
                break;
            case PROPIEQUIP_REGENVALMANA: // unimplemented
                // Missing cliloc id?
                break;
            case PROPIEQUIP_REGENVALSTAM: // unimplemented
                // Missing cliloc id?
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
                //Missing cliloc id
                break;
            case PROPIEQUIP_SOULCHARGECOLD: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_SOULCHARGEENERGY: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_SOULCHARGEFIRE: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_SOULCHARGEKINETIC: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_SOULCHARGEPOISON: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_SPELLCHANNELING:
                ADDT(1060482); // spell channeling
                break;
            case PROPIEQUIP_SPELLCONSUMPTION: // Unimplemented
                //Missing cliloc id
                break;
            case PROPIEQUIP_SPELLFOCUSING: // Unimplemented
                //Missing cliloc id
                break;
        }
    }
    // End of Numeric properties

/*
    // String properties
    for (const BaseContStrPair_t& propPair : _mPropsStr)
    {
        int prop = propPair.first;
        lpctstr ptcVal = propPair.second.GetPtr();

        switch (prop)
        {
            case PROPIEQUIP_HITSPELL: // unimplemented
                //Missing cliloc id
                break;
        }
    }
    // End of String properties
*/
}
