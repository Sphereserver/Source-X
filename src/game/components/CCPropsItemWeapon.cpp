#include "../../common/sphere_library/CSString.h"
#include "../items/CItem.h"
#include "CCPropsItemWeapon.h"


lpctstr const CCPropsItemWeapon::_ptcPropertyKeys[PROPIWEAP_QTY + 1] =
{
    #define ADD(a,b) b,
    #include "../../tables/CCPropsItemWeapon_props.tbl"
    #undef ADD
    nullptr
};

CCPropsItemWeapon::CCPropsItemWeapon() : CComponentProps(COMP_PROPS_ITEMWEAPON)
{
    // All the unset properties have to be 0
    _iRange = 0;
}

bool CanSubscribeTypeIW(IT_TYPE type)
{
    return (type == IT_WEAPON_AXE || type == IT_WEAPON_BOW || type == IT_WEAPON_FENCE || type == IT_WEAPON_MACE_CROOK || type == IT_WEAPON_MACE_PICK || type == IT_WEAPON_MACE_SHARP ||
        type == IT_WEAPON_MACE_SMITH || type == IT_WEAPON_MACE_STAFF || type == IT_WEAPON_SWORD || type == IT_WEAPON_THROWING || type == IT_WEAPON_WHIP || type == IT_WEAPON_XBOW);
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
                *piOutVal = 0;
                return false;   // not set
            }
            if (iPropIndex == PROPIWEAP_RANGEL)
            {
                *piOutVal = (_iRange & 0xFF);
                return true;
            }
            else
            {
                *piOutVal = ((_iRange >> 8) & 0xFF);
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
                return false;   // not set
            int iRangeH = (_iRange >> 8) & 0xFF;
            int iRangeL = _iRange & 0xFF;
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

void CCPropsItemWeapon::SetPropertyNum(int iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::SetPropertyNum");
    
    switch (iPropIndex)
    {
        case PROPIWEAP_RANGEH:  // internally: rangeh seems to be the Range Lowest value.
        /*{
            //iVal = maximum(iVal, 1);
            int iRange = GetPropertyNum(iPropIndex);
            if (iRange == 0)
                iRange = iVal;
            _iRange = (iRange & 0xFF00) | (iVal & 0xFF);
            break;
        }*/
            ASSERT(0); // should never happen: we set only the whole range value
            break;
        case PROPIWEAP_RANGEL: // rangel seems to be Range Highest value
        /*{
            //iVal = maximum(iVal, 1);
            int iRange = GetPropertyNum(iPropIndex);
            if (iRange == 0)
                iRange = iVal << 8;
            _iRange = ((iVal & 0xFF) << 8) | (iRange & 0xFF);
            break;
        }*/
            ASSERT(0); // should never happen: we set only the whole range value
            break;
        /*
        case PROPIWEAP_RANGE:
            _iRange = iVal;
            break;
        */
        default:
            ASSERT(!IsPropertyStr(iPropIndex));
            _mPropsNum[iPropIndex] = iVal;
            break;
    }

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
}

void CCPropsItemWeapon::SetPropertyStr(int iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, bool fZero)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::SetPropertyStr");
    ASSERT(ptcVal);
    if (fZero && (*ptcVal == '\0'))
        ptcVal = "0";

    switch (iPropIndex)
    {
        case PROPIWEAP_RANGE:
        {
            int64 piVal[2];
            tchar *ptcTmp = Str_GetTemp();
            strncpy(ptcTmp, ptcVal, STR_TEMPLENGTH);
            int iQty = Str_ParseCmds( ptcTmp, piVal, CountOf(piVal));
            int iRange;
            if ( iQty > 1 )
            {
                iRange = (int)((piVal[1] & 0xff) << 8); // highest byte contains the highest value
                iRange |= (int)(piVal[0] & 0xff);            // lowest byte contains the lowest value
            }
            else
            {
                iRange = (int)((piVal[0] & 0xff) << 8);
            }
            _iRange = iRange;
            break;
        }
        
        default:
            ASSERT(IsPropertyStr(iPropIndex));
            _mPropsStr[iPropIndex] = ptcVal;
    }

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->UpdatePropertyFlag();
}

void CCPropsItemWeapon::DeletePropertyNum(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::DeletePropertyNum");
    ASSERT(_mPropsNum.count(iPropIndex));
    _mPropsNum.erase(iPropIndex);
}

void CCPropsItemWeapon::DeletePropertyStr(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::DeletePropertyStr");
    ASSERT(_mPropsStr.count(iPropIndex));
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsItemWeapon::r_LoadPropVal(CScript & s, CObjBase* pLinkedObj)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::r_LoadPropVal");
    int i = FindTableSorted(s.GetKey(), _ptcPropertyKeys, CountOf(_ptcPropertyKeys)-1);
    if (i == -1)
        return false;

    bool fPropStr = IsPropertyStr(i);
    if (!fPropStr && (*s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(i);
        return true;
    }

    BaseProp_LoadPropVal(i, fPropStr, s, pLinkedObj);
    return true;
}

bool CCPropsItemWeapon::r_WritePropVal(lpctstr pszKey, CSString & s)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::r_WritePropVal");
    int i = FindTableSorted(pszKey, _ptcPropertyKeys, CountOf(_ptcPropertyKeys)-1);
    if (i == -1)
        return false;

    return BaseProp_WritePropVal(i, IsPropertyStr(i), s);
}

void CCPropsItemWeapon::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCPropsItemWeapon::r_Write");
    // r_Write isn't called by CItemBase/CCharBase, so we don't get base props saved

    if (_iRange != 0)
        s.WriteKeyVal(s.GetKey(), _iRange);
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
            case PROPIWEAP_BALANCED: // unimplemented
                ADDT(1072792); // balanced
                break;
            case PROPIWEAP_BANE: // unimplemented
                // Missing cliloc id
                break;
            case PROPIWEAP_BATTLELUST: // unimplemented
                // Missing cliloc id
                break;
            case PROPIWEAP_BLOODDRINKER: // unimplemented
                // Missing cliloc id
                break;
            case PROPIWEAP_MAGEWEAPON: // unimplemented
                ADDTNUM(1060438); // mage weapon -~1_val~ skill
                break;
            case PROPIWEAP_SEARINGWEAPON: // unimplemented
                // Missing cliloc id
                break;
            case PROPIWEAP_SPLINTERINGWEAPON: // unimplemented
                // Missing cliloc id
                break;
            case PROPIWEAP_USEBESTWEAPONSKILL: // unimplemented
                // Missing cliloc id
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
