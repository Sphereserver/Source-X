
#include "CCPropsItemChar.h"
#include "../items/CItem.h"
#include "../chars/CChar.h"


lpctstr const CCPropsItemChar::_ptcPropertyKeys[PROPITCH_QTY + 1] =
{
    #define ADDPROP(a,b,c) b,
    #include "../../tables/CCPropsItemChar_props.tbl"
    #undef ADDPROP
    nullptr
};
KeyTableDesc_s CCPropsItemChar::GetPropertyKeysData() const {
    return {_ptcPropertyKeys, (int)ARRAY_COUNT(_ptcPropertyKeys)};
}

RESDISPLAY_VERSION CCPropsItemChar::_iPropertyExpansion[PROPITCH_QTY + 1] =
{
    #define ADDPROP(a,b,c) c,
    #include "../../tables/CCPropsItemChar_props.tbl"
    #undef ADDPROP
    RDS_QTY
};

CCPropsItemChar::CCPropsItemChar() : CComponentProps(COMP_PROPS_ITEMCHAR)
{
}


// If a CItem: subscribed in CItemBase::SetType and CItem::SetType
// If a CChar: subscribed in CCharBase::CCharBase and CChar::CChar
/*
bool CCPropsItemChar::CanSubscribe(const CObjBase* pObj) noexcept // static
{
    return (pObj->IsItem() || pObj->IsChar());
}
*/


lpctstr CCPropsItemChar::GetPropertyName(PropertyIndex_t iPropIndex) const
{
    ASSERT(iPropIndex < PROPITCH_QTY);
    return _ptcPropertyKeys[iPropIndex];
}

bool CCPropsItemChar::IsPropertyStr(PropertyIndex_t iPropIndex) const
{
    /*
    switch (iPropIndex)
    {
        case 0:
            return true;
        default:
            return false;
    }
    */
    UnreferencedParameter(iPropIndex);
    return false;
}

bool CCPropsItemChar::GetPropertyNumPtr(PropertyIndex_t iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CCPropsItemChar::GetPropertyNumPtr");
    ASSERT(!IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyNum(&_mPropsNum, iPropIndex, piOutVal);
}

bool CCPropsItemChar::GetPropertyStrPtr(PropertyIndex_t iPropIndex, CSString* psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CCPropsItemChar::GetPropertyStrPtr");
    ASSERT(IsPropertyStr(iPropIndex));
    return BaseCont_GetPropertyStr(&_mPropsStr, iPropIndex, psOutVal, fZero);
}

bool CCPropsItemChar::SetPropertyNum(PropertyIndex_t iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsItemChar::SetPropertyNum");
    ASSERT(!IsPropertyStr(iPropIndex));
    ASSERT((iLimitToExpansion >= RDS_PRET2A) && (iLimitToExpansion < RDS_QTY));

    auto itOldVal = _mPropsNum.find(iPropIndex);

    const bool fOldValExistant = itOldVal != _mPropsNum.end();
    PropertyValNum_t iOldVal = 0;
    if (iPropIndex == PROPITCH_WEIGHTREDUCTION)
    {
        if (pLinkedObj)
            iOldVal = pLinkedObj->GetWeight();
    }
    else if (fOldValExistant)
        iOldVal = itOldVal->second;

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
    switch (iPropIndex)
    {
        case PROPITCH_WEIGHTREDUCTION:
        {
            if (iVal != iOldVal)
            {
                const CItem* pItemLink = static_cast<const CItem*>(pLinkedObj);
                CContainer* pCont = dynamic_cast <CContainer*> (pItemLink->GetParent());
                if (pCont)
                {
                    ASSERT(pItemLink->IsItemEquipped() || pItemLink->IsItemInContainer());
                    pCont->OnWeightChange(pItemLink->GetWeight() - iOldVal);
                }
                pLinkedObj->UpdatePropertyFlag();
            }
            break;
        }

        //default:
        //    pLinkedObj->UpdatePropertyFlag();
        //    break;
    }

    return true;
}

bool CCPropsItemChar::SetPropertyStr(PropertyIndex_t iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero)
{
    ADDTOCALLSTACK("CCPropsItemChar::SetPropertyStr");
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

void CCPropsItemChar::DeletePropertyNum(PropertyIndex_t iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemChar::DeletePropertyNum");
    _mPropsNum.erase(iPropIndex);
}

void CCPropsItemChar::DeletePropertyStr(PropertyIndex_t iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemChar::DeletePropertyStr");
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsItemChar::FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, PropertyIndex_t iPropIndex, bool fPropStr)
{
    ADDTOCALLSTACK("CCPropsItemChar::FindLoadPropVal");
    if (!fPropStr && (*s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(iPropIndex);
        return true;
    }

    BaseProp_LoadPropVal(iPropIndex, fPropStr, s, pLinkedObj, iLimitToExpansion);
    return true;
}

bool CCPropsItemChar::FindWritePropVal(CSString & sVal, PropertyIndex_t iPropIndex, bool fPropStr) const
{
    ADDTOCALLSTACK("CCPropsItemChar::FindWritePropVal");

    return BaseProp_WritePropVal(iPropIndex, fPropStr, sVal);
}

void CCPropsItemChar::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCItemWeapon::r_Write");
    // r_Write isn't called by CItemBase/CCharBase, so we don't get base props saved
    BaseCont_Write_ContNum(&_mPropsNum, _ptcPropertyKeys, s);
    BaseCont_Write_ContStr(&_mPropsStr, _ptcPropertyKeys, s);
}

void CCPropsItemChar::Copy(const CComponentProps * target)
{
    ADDTOCALLSTACK("CCPropsItemChar::Copy");
    const CCPropsItemChar *pTarget = static_cast<const CCPropsItemChar*>(target);
    if (!pTarget)
        return;

    _mPropsNum = pTarget->_mPropsNum;
    _mPropsStr = pTarget->_mPropsStr;
}

void CCPropsItemChar::AddPropsTooltipData(CObjBase* pLinkedObj)
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

        if (pLinkedObj->IsItem())
        {
            // Show tooltips for these props only if the linked obj is an item
            switch (prop)
            {
                case PROPITCH_WEIGHTREDUCTION:
                    ADDTNUM(1072210); // Weight reduction: ~1_PERCENTAGE~%
                    break;
            }
            // End of Item-only tooltips
        }
    }
    // End of Numeric properties

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
