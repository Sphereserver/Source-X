
#include "CCPropsChar.h"
#include "../chars/CChar.h"


lpctstr const CCPropsChar::_ptcPropertyKeys[PROPCH_QTY + 1] =
{
    #define ADD(a,b) b,
    #include "../../tables/CCPropsChar_props.tbl"
    #undef ADD
    nullptr
};

CCPropsChar::CCPropsChar() : CComponentProps(COMP_PROPS_CHAR)
{
}

// If a CItem: subscribed in CItemBase::SetType and CItem::SetType
// If a CChar: subscribed in CCharBase::CCharBase and CChar::CChar
/*
bool CCPropsChar::CanSubscribe(const CObjBase* pObj) // static
{
    return (pObj->IsItem() || pObj->IsChar());
}
*/


lpctstr CCPropsChar::GetPropertyName(int iPropIndex) const
{
    ASSERT(iPropIndex < PROPCH_QTY);
    return _ptcPropertyKeys[iPropIndex];
}

bool CCPropsChar::IsPropertyStr(int iPropIndex) const
{
/*
    switch (iPropIndex)
    {
        default:
            return false;
    }
    */
    UNREFERENCED_PARAMETER(iPropIndex);
    return false;
}

bool CCPropsChar::GetPropertyNumPtr(int iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CCPropsChar::GetPropertyNumPtr");
    return BaseCont_GetPropertyNum(&_mPropsNum, iPropIndex, piOutVal);
}

bool CCPropsChar::GetPropertyStrPtr(int iPropIndex, CSString* psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CCPropsChar::GetPropertyStrPtr");
    return BaseCont_GetPropertyStr(&_mPropsStr, iPropIndex, psOutVal, fZero);
}

void CCPropsChar::SetPropertyNum(int iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj)
{
    ADDTOCALLSTACK("CCPropsChar::SetPropertyNum");
    _mPropsNum[iPropIndex] = iVal;

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    switch (iPropIndex)
    {
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
            if (IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE))
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
            pLinkedObj->ResendTooltip();
            break;
    }
}

void CCPropsChar::SetPropertyStr(int iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, bool fZero)
{
    ADDTOCALLSTACK("CCPropsChar::SetPropertyStr");
    ASSERT(ptcVal);
    if (fZero && (ptcVal == '\0'))
        ptcVal = "0";
    _mPropsStr[iPropIndex] = ptcVal;

    if (!pLinkedObj)
        return;

    // Do stuff to the pLinkedObj
    pLinkedObj->ResendTooltip();
}

void CCPropsChar::DeletePropertyNum(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsChar::DeletePropertyNum");
    ASSERT(_mPropsNum.count(iPropIndex));
    _mPropsNum.erase(iPropIndex);
}

void CCPropsChar::DeletePropertyStr(int iPropIndex)
{
    ADDTOCALLSTACK("CCPropsChar::DeletePropertyStr");
    ASSERT(_mPropsStr.count(iPropIndex));
    _mPropsStr.erase(iPropIndex);
}

bool CCPropsChar::r_LoadPropVal(CScript & s, CObjBase* pLinkedObj)
{
    ADDTOCALLSTACK("CCPropsChar::r_LoadPropVal");
    int i = FindTableSorted(s.GetKey(), _ptcPropertyKeys, CountOf(_ptcPropertyKeys)-1);
    if (i == -1)
        return false;
    if (i == PROPCH_NIGHTSIGHT)
        return false;   // handle it in CChar::r_LoadVal
    
    bool fPropStr = IsPropertyStr(i);
    if (!fPropStr && (s.GetArgRaw() == '\0'))
    {
        DeletePropertyNum(i);
        return true;
    }

    BaseCont_LoadPropVal(i, fPropStr, s, pLinkedObj);
    return true;
}

bool CCPropsChar::r_WritePropVal(lpctstr pszKey, CSString & s)
{
    ADDTOCALLSTACK("CCPropsChar::r_WritePropVal");
    int i = FindTableSorted(pszKey, _ptcPropertyKeys, CountOf(_ptcPropertyKeys)-1);
    if (i == -1)
        return false;
    if (i == PROPCH_NIGHTSIGHT)
        return false;   // handle it in CChar::r_LoadVal

    BaseCont_WritePropVal(i, IsPropertyStr(i), s);
    return true;
}

void CCPropsChar::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCItemWeapon::r_Write");
    // r_Write isn't called by CItemBase/CCharBase, so we don't get base props saved
    BaseCont_Write_ContNum(&_mPropsNum, _ptcPropertyKeys, s);
    BaseCont_Write_ContStr(&_mPropsStr, _ptcPropertyKeys, s);
}

void CCPropsChar::Copy(const CComponentProps * target)
{
    ADDTOCALLSTACK("CCPropsChar::Copy");
    const CCPropsChar *pTarget = static_cast<const CCPropsChar*>(target);
    if (!pTarget)
        return;

    _mPropsNum = pTarget->_mPropsNum;
    _mPropsStr = pTarget->_mPropsStr;
}

void CCPropsChar::AddTooltipData(CObjBase* pLinkedObj)
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
        int prop = propPair.first;
        lpctstr ptcVal = propPair.second.GetPtr();

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
