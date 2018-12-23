
#include "CCPropsItemChar.h"
#include "../items/CItem.h"
#include "../chars/CChar.h"


lpctstr const CCPropsItemChar::_ptcPropertyKeys[PROPITCH_QTY + 1] =
{
    #define ADD(a,b) b,
    #include "../../tables/CCPropsItemChar_props.tbl"
    #undef ADD
    nullptr
};

CCPropsItemChar::CCPropsItemChar() : CComponentProps(COMP_PROPS_ITEMCHAR)
{
}

// If a CItem: subscribed in CItemBase::SetType and CItem::SetType
// If a CChar: subscribed in CCharBase::CCharBase and CChar::CChar
/*
bool CCPropsItemChar::CanSubscribe(const CObjBase* pObj) // static
{
    return (pObj->IsItem() || pObj->IsChar());
}
*/


lpctstr CCPropsItemChar::GetPropertyName(int iPropIndex) const
{
    ASSERT(iPropIndex < PROPITCH_QTY);
    return _ptcPropertyKeys[iPropIndex];
}

bool CCPropsItemChar::IsPropertyStr(int iPropIndex) const
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
    UNREFERENCED_PARAMETER(iPropIndex);
    return false;
}

bool CCPropsItemChar::GetPropertyNumPtr(int iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CCPropsItemChar::GetPropertyNumPtr");
    return BaseCont_GetPropertyNum(&_mPropsNum, iPropIndex, piOutVal);
}

bool CCPropsItemChar::GetPropertyStrPtr(int iPropIndex, CSString* psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CCPropsItemChar::GetPropertyStrPtr");
    return BaseCont_GetPropertyStr(&_mPropsStr, iPropIndex, psOutVal, fZero);
}

void CCPropsItemChar::SetPropertyNum(int iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj)
{
    ADDTOCALLSTACK("CCPropsItemChar::SetPropertyNum");
    _mPropsNum[iPropIndex] = iVal;

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    switch (iPropIndex)
    {
        case PROPITCH_WEIGHTREDUCTION:
        {
            CItem *pItemLink = static_cast<CItem*>(pLinkedObj);
            int oldweight = pItemLink->GetWeight();
            CContainer * pCont = dynamic_cast <CContainer*> (pItemLink->GetParent());
            if (pCont)
            {
                ASSERT(pItemLink->IsItemEquipped() || pItemLink->IsItemInContainer());
                pCont->OnWeightChange(pItemLink->GetWeight() - oldweight);
                pLinkedObj->ResendTooltip();
            }
            break;
        }

        //default:
        //    pLinkedObj->ResendTooltip();
        //    break;
    }
}

void CCPropsItemChar::SetPropertyStr(int iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, bool fZero)
{
    ADDTOCALLSTACK("CCPropsItemChar::SetPropertyStr");
    ASSERT(ptcVal);
    if (fZero && (*ptcVal == '\0'))
        ptcVal = "0";
    _mPropsStr[iPropIndex] = ptcVal;

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->ResendTooltip();
}

void CCPropsItemChar::DeletePropertyNum(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemChar::DeletePropertyNum");
    ASSERT(_mPropsNum.count(iPropIndex));
    _mPropsNum.erase(iPropIndex);
}

void CCPropsItemChar::DeletePropertyStr(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsItemChar::DeletePropertyStr");
    ASSERT(_mPropsStr.count(iPropIndex));
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsItemChar::r_LoadPropVal(CScript & s, CObjBase* pLinkedObj)
{
    ADDTOCALLSTACK("CCPropsItemChar::r_LoadPropVal");
    int i = FindTableSorted(s.GetKey(), _ptcPropertyKeys, CountOf(_ptcPropertyKeys)-1);
    if (i == -1)
        return false;
    
    bool fPropStr = IsPropertyStr(i);
    if (!fPropStr && (*s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(i);
        return true;
    }

    BaseCont_LoadPropVal(i, fPropStr, s, pLinkedObj);
    return true;
}

bool CCPropsItemChar::r_WritePropVal(lpctstr pszKey, CSString & s)
{
    ADDTOCALLSTACK("CCPropsItemChar::r_WritePropVal");
    int i = FindTableSorted(pszKey, _ptcPropertyKeys, CountOf(_ptcPropertyKeys)-1);
    if (i == -1)
        return false;

    BaseCont_WritePropVal(i, IsPropertyStr(i), s);
    return true;
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

void CCPropsItemChar::AddTooltipData(CObjBase* pLinkedObj)
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
        
        if (pLinkedObj->IsItem())
        {
            // Show tooltips for these props only if the linked obj is an item
            switch (prop)
            {
                case PROPITCH_WEIGHTREDUCTION:
                    // Missing cliloc id
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
