#include "../../common/sphere_library/CSString.h"
#include "../items/CItem.h"
#include "CCPropsItemWeaponRanged.h"


lpctstr const CCPropsItemWeaponRanged::_ptcPropertyKeys[PROPIWEAPRNG_QTY + 1] =
{
    #define ADD(a,b) b,
    #include "../../tables/CCPropsItemWeaponRanged_props.tbl"
    #undef ADD
    nullptr
};
KeyTableDesc_s CCPropsItemWeaponRanged::GetPropertyKeysData() const {
    return {_ptcPropertyKeys, (int)CountOf(_ptcPropertyKeys)};
}

CCPropsItemWeaponRanged::CCPropsItemWeaponRanged() : CComponentProps(COMP_PROPS_ITEMWEAPONRANGED)
{
}

bool CanSubscribeTypeIWR(IT_TYPE type)
{
    return (type == IT_WEAPON_BOW || type == IT_WEAPON_XBOW);
}

bool CCPropsItemWeaponRanged::CanSubscribe(const CItemBase* pItemBase) // static
{
    return CanSubscribeTypeIWR(pItemBase->GetType());
}

bool CCPropsItemWeaponRanged::CanSubscribe(const CItem* pItem) // static
{
    return CanSubscribeTypeIWR(pItem->GetType());
}


lpctstr CCPropsItemWeaponRanged::GetPropertyName(int iPropIndex) const
{
    ASSERT(iPropIndex < PROPIWEAPRNG_QTY);
    return _ptcPropertyKeys[iPropIndex];
}

bool CCPropsItemWeaponRanged::IsPropertyStr(int iPropIndex) const
{
    switch (iPropIndex)
    {
        case PROPIWEAPRNG_AMMOTYPE:
            return true;
        default:
            return false;
    }
}

bool CCPropsItemWeaponRanged::GetPropertyNumPtr(int iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::GetPropertyNumPtr");
    ASSERT(!IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyNum(&_mPropsNum, iPropIndex, piOutVal);
}

bool CCPropsItemWeaponRanged::GetPropertyStrPtr(int iPropIndex, CSString* psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::GetPropertyStrPtr");
    ASSERT(IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyStr(&_mPropsStr, iPropIndex, psOutVal, fZero);
}

void CCPropsItemWeaponRanged::SetPropertyNum(int iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::SetPropertyNum");
    
    ASSERT(!IsPropertyStr(iPropIndex));
    _mPropsNum[iPropIndex] = iVal;

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
}

void CCPropsItemWeaponRanged::SetPropertyStr(int iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, bool fZero)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::SetPropertyStr");
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

void CCPropsItemWeaponRanged::DeletePropertyNum(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::DeletePropertyNum");
    _mPropsNum.erase(iPropIndex);
}

void CCPropsItemWeaponRanged::DeletePropertyStr(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::DeletePropertyStr");
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsItemWeaponRanged::FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, int iPropIndex, bool fPropStr)
{
    ADDTOCALLSTACK("CCPropsItemWeaponRanged::FindLoadPropVal");
    if (!fPropStr && (*s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(iPropIndex);
        return true;
    }

    BaseProp_LoadPropVal(iPropIndex, fPropStr, s, pLinkedObj);
    return true;
}

bool CCPropsItemWeaponRanged::FindWritePropVal(CSString & sVal, int iPropIndex, bool fPropStr) const
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

    UNREFERENCED_PARAMETER(pLinkedObj);
    /* Tooltips for "dynamic" properties (stored in the BaseConts: _mPropsNum and _mPropsStr) */
/*
    // Numeric properties
    for (const BaseContNumPair_t& propPair : _mPropsNum)
    {
        int prop = propPair.first;
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
