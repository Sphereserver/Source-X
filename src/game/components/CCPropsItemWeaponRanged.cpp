#include "../../common/sphere_library/CSString.h"
#include "../items/CItem.h"
#include "CCPropsItemWeaponRanged.h"


lpctstr const CCPropsItemWeaponRanged::_ptcPropertyKeys[PROPIWEAPRNG_QTY + 1] =
{
    #define ADDPROP(a,b,c) b,
    #include "../../tables/CCPropsItemWeaponRanged_props.tbl"
    #undef ADDPROP
    nullptr
};
KeyTableDesc_s CCPropsItemWeaponRanged::GetPropertyKeysData() const {
    return {_ptcPropertyKeys, (PropertyIndex_t)ARRAY_COUNT(_ptcPropertyKeys)};
}

RESDISPLAY_VERSION CCPropsItemWeaponRanged::_iPropertyExpansion[PROPIWEAPRNG_QTY + 1] =
{
#define ADDPROP(a,b,c) c,
#include "../../tables/CCPropsItemWeaponRanged_props.tbl"
#undef ADDPROP
    RDS_QTY
};

CCPropsItemWeaponRanged::CCPropsItemWeaponRanged() : CComponentProps(COMP_PROPS_ITEMWEAPONRANGED)
{
}

static bool CanSubscribeTypeIWR(IT_TYPE type) noexcept
{
    return (type == IT_WEAPON_BOW || type == IT_WEAPON_XBOW);
}

bool CCPropsItemWeaponRanged::CanSubscribe(const CItemBase* pItemBase) noexcept // static
{
    return CanSubscribeTypeIWR(pItemBase->GetType());
}

bool CCPropsItemWeaponRanged::CanSubscribe(const CItem* pItem) noexcept // static
{
    return CanSubscribeTypeIWR(pItem->GetType());
}


lpctstr CCPropsItemWeaponRanged::GetPropertyName(PropertyIndex_t iPropIndex) const
{
    ASSERT(iPropIndex < PROPIWEAPRNG_QTY);
    return _ptcPropertyKeys[iPropIndex];
}

bool CCPropsItemWeaponRanged::IsPropertyStr(PropertyIndex_t iPropIndex) const
{
    switch (iPropIndex)
    {
		case PROPIWEAPRNG_AMMOANIM:
			return true;
		case PROPIWEAPRNG_AMMOCONT:
			return true;
		case PROPIWEAPRNG_AMMOTYPE:
			return true;
        default:
            return false;
    }
}

bool CCPropsItemWeaponRanged::GetPropertyNumPtr(PropertyIndex_t iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::GetPropertyNumPtr");
    ASSERT(!IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyNum(&_mPropsNum, iPropIndex, piOutVal);
}

bool CCPropsItemWeaponRanged::GetPropertyStrPtr(PropertyIndex_t iPropIndex, CSString* psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::GetPropertyStrPtr");
    ASSERT(IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyStr(&_mPropsStr, iPropIndex, psOutVal, fZero);
}

bool CCPropsItemWeaponRanged::SetPropertyNum(PropertyIndex_t iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::SetPropertyNum");
    ASSERT(!IsPropertyStr(iPropIndex));
    ASSERT((iLimitToExpansion >= RDS_PRET2A) && (iLimitToExpansion < RDS_QTY));

    if ((fDeleteZero && (iVal == 0)) || (_iPropertyExpansion[iPropIndex] > iLimitToExpansion))
    {
        if (0 == _mPropsNum.erase(iPropIndex))
            return true; // I didn't have this property, so avoid further processing.
    }
    else
    {
        _mPropsNum[iPropIndex] = iVal;
        //_mPropsNum.container.shrink_to_fit();
    }

    if (!pLinkedObj)
        return true;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
    return true;
}

bool CCPropsItemWeaponRanged::SetPropertyStr(PropertyIndex_t iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::SetPropertyStr");
    ASSERT(ptcVal);
    ASSERT(IsPropertyStr(iPropIndex));
    ASSERT((iLimitToExpansion >= RDS_PRET2A) && (iLimitToExpansion < RDS_QTY));

    if ((fDeleteZero && (*ptcVal == '\0')) || (_iPropertyExpansion[iPropIndex] > iLimitToExpansion))
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

void CCPropsItemWeaponRanged::DeletePropertyNum(PropertyIndex_t iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::DeletePropertyNum");
    _mPropsNum.erase(iPropIndex);
}

void CCPropsItemWeaponRanged::DeletePropertyStr(PropertyIndex_t iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::DeletePropertyStr");
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsItemWeaponRanged::FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, PropertyIndex_t iPropIndex, bool fPropStr)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::FindLoadPropVal");
    if (!fPropStr && (*s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(iPropIndex);
        return true;
    }

    BaseProp_LoadPropVal(iPropIndex, fPropStr, s, pLinkedObj, iLimitToExpansion);
    return true;
}

bool CCPropsItemWeaponRanged::FindWritePropVal(CSString & sVal, PropertyIndex_t iPropIndex, bool fPropStr) const
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::FindWritePropVal");

    return BaseProp_WritePropVal(iPropIndex, fPropStr, sVal);
}

void CCPropsItemWeaponRanged::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::r_Write");
    // r_Write isn't called by CItemBase/CCharBase, so we don't get base props saved
    BaseCont_Write_ContNum(&_mPropsNum, _ptcPropertyKeys, s);
    BaseCont_Write_ContStr(&_mPropsStr, _ptcPropertyKeys, s);
}

void CCPropsItemWeaponRanged::Copy(const CComponentProps * target)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::Copy");
    const CCPropsItemWeaponRanged *pTarget = static_cast<const CCPropsItemWeaponRanged*>(target);
    if (!pTarget)
    {
        return;
    }
    _mPropsNum = pTarget->_mPropsNum;
    _mPropsStr = pTarget->_mPropsStr;
}

void CCPropsItemWeaponRanged::AddPropsTooltipData(CObjBase* pLinkedObj)
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
            default:
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
            default:
                break;
        }
    }
    // End of String properties
*/
}
