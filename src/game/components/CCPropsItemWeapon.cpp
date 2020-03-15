#include "../../common/sphere_library/CSString.h"
#include "../items/CItem.h"
#include "CCPropsItemWeapon.h"


lpctstr const CCPropsItemWeapon::_ptcPropertyKeys[PROPIWEAP_QTY + 1] =
{
    #define ADDPROP(a,b,c) b,
    #include "../../tables/CCPropsItemWeapon_props.tbl"
    #undef ADDPROP
    nullptr
};
KeyTableDesc_s CCPropsItemWeapon::GetPropertyKeysData() const {
    return {_ptcPropertyKeys, (int)CountOf(_ptcPropertyKeys)};
}

RESDISPLAY_VERSION CCPropsItemWeapon::_iPropertyExpansion[PROPIWEAP_QTY + 1] =
{
#define ADDPROP(a,b,c) c,
#include "../../tables/CCPropsItemWeapon_props.tbl"
#undef ADDPROP
    RDS_QTY
};

CCPropsItemWeapon::CCPropsItemWeapon() : CComponentProps(COMP_PROPS_ITEMWEAPON)
{
    // All the unset properties have to be 0
    //#define PROP_RANGE_DEFAULT          RANGE_MAKE(1, 0)    // minimum value for Range, even if not set: RangeH = 1; RangeL = 0
    _iRange = 0;
}

static bool CanSubscribeTypeIW(IT_TYPE type)
{
    return (type == IT_WEAPON_AXE || type == IT_WEAPON_BOW || type == IT_WEAPON_FENCE || type == IT_WEAPON_MACE_CROOK || type == IT_WEAPON_MACE_PICK || type == IT_WEAPON_MACE_SHARP ||
        type == IT_WEAPON_MACE_SMITH || type == IT_WEAPON_MACE_STAFF || type == IT_WEAPON_SWORD || type == IT_WEAPON_THROWING || type == IT_WEAPON_WHIP || type == IT_WEAPON_XBOW ||
        type == IT_FISH_POLE);
}

bool CCPropsItemWeapon::CanSubscribe(const CItemBase* pItemBase) // static
{
    return CanSubscribeTypeIW(pItemBase->GetType());
}

bool CCPropsItemWeapon::CanSubscribe(const CItem* pItem) // static
{
    return CanSubscribeTypeIW(pItem->GetType());
}


lpctstr CCPropsItemWeapon::GetPropertyName(int iPropIndex) const
{
    ASSERT(iPropIndex < PROPIWEAP_QTY);
    return _ptcPropertyKeys[iPropIndex];
}

bool CCPropsItemWeapon::IsPropertyStr(int iPropIndex) const
{
    switch (iPropIndex)
    {
        case PROPIWEAP_RANGE:   // returns a string that contains two numbers
            return true;
        default:
            return false;
    }
}

bool CCPropsItemWeapon::GetPropertyNumPtr(int iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CCPropsItemWeapon::GetPropertyNumPtr");
    ASSERT(!IsPropertyStr(iPropIndex));
    switch (iPropIndex)
    {
        case PROPIWEAP_RANGEL:
        case PROPIWEAP_RANGEH:
            if (_iRange == 0)
            {
                *piOutVal = 1;
                return false;   // not set
            }
            if (iPropIndex == PROPIWEAP_RANGEL)
            {
                *piOutVal = RANGE_GET_LO(_iRange);
                return true;
            }
            else
            {
                *piOutVal = RANGE_GET_HI(_iRange);
                return true;
            }

        default:
            return BaseCont_GetPropertyNum(&_mPropsNum, iPropIndex, piOutVal);
    }
}

bool CCPropsItemWeapon::GetPropertyStrPtr(int iPropIndex, CSString* psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CCPropsItemWeapon::GetPropertyStrPtr");
    ASSERT(IsPropertyStr(iPropIndex));
    switch (iPropIndex)
    {
        case PROPIWEAP_RANGE:
        {
            if (_iRange == 0)
            {
                psOutVal->FormatVal(1);
                return false;   // not set
            }
            int iRangeH = RANGE_GET_HI(_iRange);
            int iRangeL = RANGE_GET_LO(_iRange);
            if ( iRangeL == 0 )
                psOutVal->Format( "%d", iRangeH );
            else
                psOutVal->Format( "%d,%d", iRangeL, iRangeH );
        }
        return true;

        default:
            return BaseCont_GetPropertyStr(&_mPropsStr, iPropIndex, psOutVal, fZero);
    }
}

void CCPropsItemWeapon::SetPropertyNum(int iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::SetPropertyNum");
    
    switch (iPropIndex)
    {
        case PROPIWEAP_RANGEH:
            ASSERT(0); // should never happen: we set only the whole range value
            break;
        case PROPIWEAP_RANGEL:
            ASSERT(0); // should never happen: we set only the whole range value
            break;
        /*
        case PROPIWEAP_RANGE:
            _iRange = iVal;
            break;
        */
        default:
            ASSERT(!IsPropertyStr(iPropIndex));
            ASSERT((iLimitToExpansion >= RDS_PRET2A) && (iLimitToExpansion < RDS_QTY));

            if ((fDeleteZero && (iVal == 0)) || (_iPropertyExpansion[iPropIndex] > iLimitToExpansion))
            {
                if (0 == _mPropsNum.erase(iPropIndex))
                    return; // I didn't have this property, so avoid further processing.
            }
            else
                _mPropsNum[iPropIndex] = iVal;
            break;
    }

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
}

void CCPropsItemWeapon::SetPropertyStr(int iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::SetPropertyStr");
    ASSERT(ptcVal);

    switch (iPropIndex)
    {
        case PROPIWEAP_RANGE:
        {
            _iRange = CBaseBaseDef::ConvertRangeStr(ptcVal);
            break;
        }
        
        default:
            ASSERT(IsPropertyStr(iPropIndex));
            ASSERT((iLimitToExpansion >= RDS_PRET2A) && (iLimitToExpansion < RDS_QTY));

            if ((fDeleteZero && (*ptcVal == '\0')) || (_iPropertyExpansion[iPropIndex] > iLimitToExpansion))
            {
                if (0 == _mPropsNum.erase(iPropIndex))
                    return; // I didn't have this property, so avoid further processing.
            }
            else
                _mPropsStr[iPropIndex] = ptcVal;
            break;
    }

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
}

void CCPropsItemWeapon::DeletePropertyNum(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::DeletePropertyNum");
    _mPropsNum.erase(iPropIndex);
}

void CCPropsItemWeapon::DeletePropertyStr(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::DeletePropertyStr");
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsItemWeapon::FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, int iPropIndex, bool fPropStr)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::FindLoadPropVal");
    if (!fPropStr && (*s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(iPropIndex);
        return true;
    }

    BaseProp_LoadPropVal(iPropIndex, fPropStr, s, pLinkedObj, iLimitToExpansion);
    return true;
}

bool CCPropsItemWeapon::FindWritePropVal(CSString & sVal, int iPropIndex, bool fPropStr) const
{
    ADDTOCALLSTACK("CCPropsItemWeapon::FindWritePropVal");

    return BaseProp_WritePropVal(iPropIndex, fPropStr, sVal);
}

void CCPropsItemWeapon::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::Write");
    // r_Write isn't called by CItemBase/CCharBase, so we don't get base props saved

    if (_iRange != 0)
        s.WriteKeyFormat("RANGE", "%d,%d", RANGE_GET_HI(_iRange), RANGE_GET_LO(_iRange)); // RANGE = hi, lo
    BaseCont_Write_ContNum(&_mPropsNum, _ptcPropertyKeys, s);
    BaseCont_Write_ContStr(&_mPropsStr, _ptcPropertyKeys, s);
}

void CCPropsItemWeapon::Copy(const CComponentProps * target)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::Copy");
    const CCPropsItemWeapon *pTarget = static_cast<const CCPropsItemWeapon*>(target);
    if (!pTarget)
    {
        return;
    }
    _mPropsNum = pTarget->_mPropsNum;
    _mPropsStr = pTarget->_mPropsStr;
}

void CCPropsItemWeapon::AddPropsTooltipData(CObjBase* pLinkedObj)
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
            case PROPIWEAP_ASSASSINHONED: // unimplemented
                ADDT(1152206);  // Assassin Honed
                break;
            case PROPIWEAP_BALANCED: // unimplemented (for two-handed weapons only)
                ADDT(1072792); // Balanced
                break;
            case PROPIWEAP_BANE: // unimplemented
                ADDT(1154671); // Bane
                break;
            case PROPIWEAP_BATTLELUST: // unimplemented
                ADDT(1113710); // Battle Lust
                break;
            case PROPIWEAP_BLOODDRINKER: // unimplemented
                ADDT(1113591); // Blood Drinker
                break;
            case PROPIWEAP_BONEBREAKER: // unimplemented
                ADDT(1157318); // Bone Breaker
                break;
            case PROPIWEAP_MAGEWEAPON: // unimplemented
                ADDTNUM(1060438); // mage weapon -~1_val~ skill
                break;
            case PROPIWEAP_SEARINGWEAPON: // unimplemented
                ADDT(1151183); // (1151318, String.Format("#{0}", LabelNumber)); ?
                break;
            case PROPIWEAP_SPLINTERINGWEAPON: // unimplemented
                ADDTNUM(1112857); //splintering weapon ~1_val~%
                break;
            case PROPIWEAP_USEBESTWEAPONSKILL: // unimplemented
                ADDT(1060400); // use best weapon skill
                break;
            default:
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
            default:
                break;
        }
    }
    // End of String properties
*/
}
