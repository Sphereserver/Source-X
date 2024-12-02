
#include "CCPropsChar.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"


lpctstr const CCPropsChar::_ptcPropertyKeys[PROPCH_QTY + 1] =
{
    #define ADDPROP(a,b,c) b,
    #include "../../tables/CCPropsChar_props.tbl"
    #undef ADDPROP
    nullptr
};
KeyTableDesc_s CCPropsChar::GetPropertyKeysData() const {
    return {_ptcPropertyKeys, (PropertyIndex_t)ARRAY_COUNT(_ptcPropertyKeys)};
}

RESDISPLAY_VERSION CCPropsChar::_iPropertyExpansion[PROPCH_QTY + 1] =
{
    #define ADDPROP(a,b,c) c,
    #include "../../tables/CCPropsChar_props.tbl"
    #undef ADDPROP
    RDS_QTY
};

CCPropsChar::CCPropsChar() : CComponentProps(COMP_PROPS_CHAR)
{
}

// If a CItem: subscribed in CItemBase::SetType and CItem::SetType
// If a CChar: subscribed in CCharBase::CCharBase and CChar::CChar
/*
bool CCPropsChar::CanSubscribe(const CObjBase* pObj) noexcept // static
{
    return (pObj->IsItem() || pObj->IsChar());
}
*/


bool CCPropsChar::IgnoreElementalProperty(PropertyIndex_t iPropIndex) // static
{
    if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
        return false;
    switch ( PROPCH_TYPE(iPropIndex) )
    {
        case PROPCH_DAMCHAOS:
        case PROPCH_DAMCOLD:
        case PROPCH_DAMDIRECT:
        case PROPCH_DAMENERGY:
        case PROPCH_DAMFIRE:
        case PROPCH_DAMPHYSICAL:
        case PROPCH_DAMPOISON:
        case PROPCH_HITAREACOLD:
        case PROPCH_HITAREAENERGY:
        case PROPCH_HITAREAFIRE:
        case PROPCH_HITAREAPHYSICAL:
        case PROPCH_HITAREAPOISON:
        case PROPCH_RESCOLD:
        case PROPCH_RESCOLDMAX:
        case PROPCH_RESENERGY:
        case PROPCH_RESENERGYMAX:
        case PROPCH_RESFIRE:
        case PROPCH_RESFIREMAX:
            return true;
    }
    return false;
}


lpctstr CCPropsChar::GetPropertyName(PropertyIndex_t iPropIndex) const
{
    ASSERT(iPropIndex < PROPCH_QTY);
    return _ptcPropertyKeys[iPropIndex];
}

bool CCPropsChar::IsPropertyStr(PropertyIndex_t iPropIndex) const
{
/*
    switch (iPropIndex)
    {
        default:
            return false;
    }
    */
    UnreferencedParameter(iPropIndex);
    return false;
}

bool CCPropsChar::GetPropertyNumPtr(PropertyIndex_t iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CCPropsChar::GetPropertyNumPtr");
    ASSERT(!IsPropertyStr(iPropIndex));

    if (iPropIndex == PROPCH_FACTION_GROUP)
    {
        auto group = _faction.GetGroup();
        if (group == CFactionDef::Group::NONE)
            return false;
        *piOutVal = (int32)enum_alias_cast<uint32>(group);
        return true;
    }
    else if (iPropIndex == PROPCH_FACTION_SPECIES)
    {
        auto species = _faction.GetSpecies();
        if (species == CFactionDef::Species::NONE)
            return false;
        *piOutVal = (int32)enum_alias_cast<uint32>(species);
        return true;
    }
    return BaseCont_GetPropertyNum(&_mPropsNum, iPropIndex, piOutVal);
}

bool CCPropsChar::GetPropertyStrPtr(PropertyIndex_t iPropIndex, CSString* psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CCPropsChar::GetPropertyStrPtr");
    ASSERT(IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyStr(&_mPropsStr, iPropIndex, psOutVal, fZero);
}

bool CCPropsChar::SetPropertyNum(PropertyIndex_t iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsChar::SetPropertyNum");
    ASSERT(!IsPropertyStr(iPropIndex));
    ASSERT((iLimitToExpansion >= RDS_PRET2A) && (iLimitToExpansion < RDS_QTY));

    if ((fDeleteZero && (iVal == 0)) || (_iPropertyExpansion[iPropIndex] > iLimitToExpansion) /*|| IgnoreElementalProperty(iPropIndex)*/)
    {
        if (0 == _mPropsNum.erase(iPropIndex))
            return true; // I didn't have this property, so avoid further processing.
    }

    else if (iPropIndex == PROPCH_FACTION_GROUP)
    {
        return _faction.SetGroup(enum_alias_cast<CFactionDef::Group>(uint32(iVal)));
    }
    else if (iPropIndex == PROPCH_FACTION_SPECIES)
    {
        return _faction.SetSpecies(enum_alias_cast<CFactionDef::Species>(uint32(iVal)));
    }
    else
    {
        _mPropsNum[iPropIndex] = iVal;
        //_mPropsNum.container.shrink_to_fit();
    }

    if (!pLinkedObj)
        return true;

    // Do stuff to the pLinkedObj
    switch (iPropIndex)
    {
        case PROPCH_NIGHTSIGHT:
        {
            CChar * pChar = static_cast <CChar*>(pLinkedObj);
            pChar->StatFlag_Mod( STATF_NIGHTSIGHT, (iVal > 0) ? true : false );
            if ( pChar->IsClientActive() )
                pChar->GetClientActive()->addLight();
            break;
        }

        case PROPCH_RESCOLDMAX:
        case PROPCH_RESFIREMAX:
        case PROPCH_RESENERGYMAX:
        case PROPCH_RESPOISONMAX:
        case PROPCH_RESPHYSICAL:
        case PROPCH_RESPHYSICALMAX:
        case PROPCH_RESFIRE:
        case PROPCH_RESCOLD:
        case PROPCH_RESPOISON:
        case PROPCH_RESENERGY:
        {
            // This should be used in case items with these properties updates the character in the moment without any script to make status reflect the update.
            // Maybe too a cliver check to not send update if not needed.
            if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) || g_Cfg.m_fDisplayElementalResistance)
            {
                CChar * pChar = static_cast <CChar*>(pLinkedObj);
                pChar->UpdateStatsFlag();
            }
            break;
        }

        case PROPCH_COMBATBONUSSTAT:
        case PROPCH_COMBATBONUSPERCENT:
        case PROPCH_FASTERCASTRECOVERY:
        case PROPCH_FASTERCASTING:
        case PROPCH_INCREASESWINGSPEED:
        case PROPCH_INCREASEDAM:
        case PROPCH_INCREASEDEFCHANCE:
        case PROPCH_INCREASEDEFCHANCEMAX:
        case PROPCH_INCREASESPELLDAM:
        case PROPCH_LUCK:
        case PROPCH_LOWERREAGENTCOST:
        case PROPCH_LOWERMANACOST:
        {
            CChar * pChar = static_cast <CChar*>(pLinkedObj);
            pChar->UpdateStatsFlag();
            break;
        }

        default:
            //pLinkedObj->UpdatePropertyFlag();
            break;
    }

    return true;
}

bool CCPropsChar::SetPropertyStr(PropertyIndex_t iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsChar::SetPropertyStr");
    ASSERT(ptcVal);
    ASSERT(IsPropertyStr(iPropIndex));
    ASSERT((iLimitToExpansion >= RDS_PRET2A) && (iLimitToExpansion < RDS_QTY));

    if ((fDeleteZero && (*ptcVal == '\0')) || (_iPropertyExpansion[iPropIndex] > iLimitToExpansion) /*|| IgnoreElementalProperty(iPropIndex)*/)
    {
        if (0 == _mPropsNum.erase(iPropIndex))
            return true; // I didn't have this property, so avoid further processing.
    }
    else
    {
        _mPropsStr[iPropIndex] = ptcVal;
        //_mPropsStr.container.shrink_to_fit();
    }

    if (!pLinkedObj)
        return true;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();

    return true;
}

void CCPropsChar::DeletePropertyNum(PropertyIndex_t iPropIndex)
{
    ADDTOCALLSTACK("CCPropsChar::DeletePropertyNum");
    _mPropsNum.erase(iPropIndex);
}

void CCPropsChar::DeletePropertyStr(PropertyIndex_t iPropIndex)
{
    ADDTOCALLSTACK("CCPropsChar::DeletePropertyStr");
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsChar::FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, PropertyIndex_t iPropIndex, bool fPropStr)
{
    ADDTOCALLSTACK("CCPropsChar::FindLoadPropVal");

    if (iPropIndex == PROPCH_NIGHTSIGHT)
    {
        // Special case if it has empty args: Keep old 'switch' from 0 to 1 and viceversa behaviour
        if (!s.HasArgs())
        {
            int iVal = ! static_cast <CChar*>(pLinkedObj)->IsStatFlag(STATF_NIGHTSIGHT);
            SetPropertyNum(iPropIndex, iVal, pLinkedObj);
            return true;
        }
    }

    if (!fPropStr && (*s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(fPropStr);
        return true;
    }

    BaseProp_LoadPropVal(iPropIndex, fPropStr, s, pLinkedObj, iLimitToExpansion);
    return true;
}

bool CCPropsChar::FindWritePropVal(CSString & sVal, PropertyIndex_t iPropIndex, bool fPropStr) const
{
    ADDTOCALLSTACK("CCPropsChar::FindWritePropVal");
    return BaseProp_WritePropVal(iPropIndex, fPropStr, sVal);
}

void CCPropsChar::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCItemWeapon::r_Write");
    // r_Write isn't called by CItemBase/CCharBase, so we don't get base props saved
    BaseCont_Write_ContNum(&_mPropsNum, _ptcPropertyKeys, s);
    BaseCont_Write_ContStr(&_mPropsStr, _ptcPropertyKeys, s);

    if (_faction.GetGroup() != CFactionDef::Group::NONE)
        s.WriteKeyVal(_ptcPropertyKeys[PROPCH_FACTION_GROUP],   (int64)enum_alias_cast<uint32>(_faction.GetGroup()));
    if (_faction.GetSpecies() != CFactionDef::Species::NONE)
        s.WriteKeyVal(_ptcPropertyKeys[PROPCH_FACTION_SPECIES], (int64)enum_alias_cast<uint32>(_faction.GetSpecies()));
}

void CCPropsChar::Copy(const CComponentProps * target)
{
    ADDTOCALLSTACK("CCPropsChar::Copy");
    const CCPropsChar *pTarget = static_cast<const CCPropsChar*>(target);
    if (!pTarget)
        return;

    _mPropsNum = pTarget->_mPropsNum;
    _mPropsStr = pTarget->_mPropsStr;

    _faction = pTarget->_faction;
}

void CCPropsChar::AddPropsTooltipData(CObjBase* pLinkedObj)
{
#define TOOLTIP_APPEND(t)   pLinkedObj->m_TooltipData.emplace_back(t)
#define ADDT(tooltipID)     TOOLTIP_APPEND(new CClientTooltip(tooltipID))
#define ADDTNUM(tooltipID)  TOOLTIP_APPEND(new CClientTooltip(tooltipID, iVal))
#define ADDTSTR(tooltipID)  TOOLTIP_APPEND(new CClientTooltip(tooltipID, ptcVal))

    UnreferencedParameter(pLinkedObj);

    /* Tooltips for "dynamic" properties (stored in the BaseConts: _mPropsNum and _mPropsStr) */

/*
    // Numeric properties
    for (const BaseContNumPair_t& propPair : _mPropsNum)
    {
        PropertyIndex_t prop = propPair.first;
        PropertyValNum_t iVal = propPair.second;

        if (iVal == 0)
            continue;

        switch (prop)
        {
            default: // unimplemented
                     //Missing cliloc id
                break;
        }
    }
    // End of Numeric properties
*/

/*
    // String properties
    for (const BaseContStrPair_t& propPair : _mPropsStr)
    {
        PropertyIndex_t prop = propPair.first;
        lpctstr ptcVal = propPair.second.GetBuffer();

        switch (prop)
        {
            default: // unimplemented
                //Missing cliloc id
                break;
        }
    }
    // End of String properties
*/
}
