#include "../sphere/threads.h"
#include "../common/CScript.h"
#include "CComponentProps.h"

CComponentProps::CComponentProps(COMPPROPS_TYPE type)
{
    _iType = type;
}

COMPPROPS_TYPE CComponentProps::GetType() const
{
    return _iType;
}

bool CComponentProps::BaseCont_GetPropertyNum(const BaseContNum_t* container, int iPropIndex, PropertyValNum_t* piOutVal) const
{
    ADDTOCALLSTACK("CComponentProps::GetPropertyNum");
    BaseContNum_t::const_iterator it = container->find(iPropIndex);
    if (it != container->end())
    {
        *piOutVal = it->second;
        return true;
    }
    *piOutVal = 0;
    return false;
}

bool CComponentProps::BaseCont_GetPropertyStr(const BaseContStr_t* container, int iPropIndex, CSString *psOutVal, bool fZero) const
{
    ADDTOCALLSTACK("CComponentProps::GetPropertyStr");
    BaseContStr_t::const_iterator it = container->find(iPropIndex);
    if (it != container->end())
    {
        lpctstr val = it->second.GetPtr();
        ASSERT(val);
        if (val[0] == '\0')
        {
            *psOutVal = fZero ? "0" : "";
            return true;
        }
        *psOutVal = it->second;
        return true;
    }
    return false;
}

void CComponentProps::BaseProp_LoadPropVal(int iPropIndex, bool fPropStr, CScript & s, CObjBase* pLinkedObj)
{
    ADDTOCALLSTACK("CComponentProps::BaseProp_LoadPropVal");
    if (fPropStr)
        SetPropertyStr(iPropIndex, s.GetArgStr(), pLinkedObj, true);
    else
        SetPropertyNum(iPropIndex, s.GetArgVal(), pLinkedObj, true);
}

bool CComponentProps::BaseProp_WritePropVal(int iPropIndex, bool fPropStr, CSString & sVal) const
{
    ADDTOCALLSTACK("CComponentProps::BaseProp_WritePropVal");
    if (fPropStr)
    {
        return GetPropertyStrPtr(iPropIndex, &sVal);
    }
    else
    {
        PropertyValNum_t iVal = 0;
        bool fRet = GetPropertyNumPtr(iPropIndex, &iVal);
        sVal.FormatLLVal(iVal);
        return fRet;
    }
}

void CComponentProps::BaseCont_Write_ContNum(const BaseContNum_t* container, const lpctstr *ptcPropsTable, CScript &s) // static
{
    for (const BaseContNumPair_t& propPair : *container)
    {
        if (propPair.second == 0)
            continue;
        s.WriteKeyVal(ptcPropsTable[propPair.first], propPair.second);
    }
}

void CComponentProps::BaseCont_Write_ContStr(const BaseContStr_t* container, const lpctstr *ptcPropsTable, CScript &s) // static
{
    for (const BaseContStrPair_t& propPair : *container)
    {
        lpctstr ptcVal = propPair.second.GetPtr();
        ASSERT(ptcVal);
        if (ptcVal[0] == '\0')
            continue;
        s.WriteKey(ptcPropsTable[propPair.first], ptcVal);
    }
}

CComponentProps::PropertyValNum_t CComponentProps::GetPropertyNum(int iPropIndex) const
{
    ADDTOCALLSTACK("CComponentProps::GetPropertyNum");
    // Basically a wrapper for GetPropertyNumPtr, when you don't care if the property is present or not
    PropertyValNum_t iRet = 0;
    GetPropertyNumPtr(iPropIndex, &iRet);
    return iRet;
}

CSString CComponentProps::GetPropertyStr(int iPropIndex, bool fZero) const
{
    ADDTOCALLSTACK("CComponentProps::GetPropertyStr");
    // Basically a wrapper for GetPropertyStrPtr, when you don't care if the property is present or not
    CSString sRet;
    GetPropertyStrPtr(iPropIndex, &sRet, fZero);
    return sRet;
}