/**
* @file CCPropsChar.h
*
*/

#ifndef _INC_CCPROPSCHAR_H
#define _INC_CCPROPSCHAR_H

#include "../CComponentProps.h"


enum PROPCH_TYPE : CComponentProps::PropertyIndex_t
{
    #define ADDPROP(a,b,c) PROPCH_##a,
    #include "../../tables/CCPropsChar_props.tbl"
    #undef ADDPROP
    PROPCH_QTY
};

class CCPropsChar : public CComponentProps
{
    static lpctstr const        _ptcPropertyKeys[];
    static RESDISPLAY_VERSION   _iPropertyExpansion[];

public:
    static constexpr COMPPROPS_TYPE _kiType = COMP_PROPS_CHAR;

    CCPropsChar();
    virtual ~CCPropsChar() = default;

    //static bool CanSubscribe(const CObjBase* pObj);
    static bool IgnoreElementalProperty(PropertyIndex_t iPropIndex);

    virtual lpctstr GetName() const override {
        return "Char";
    }
    virtual PropertyIndex_t GetPropsQty() const override {
        return PROPCH_QTY;
    }
    virtual KeyTableDesc_s GetPropertyKeysData() const override;
    virtual lpctstr GetPropertyName(PropertyIndex_t iPropIndex) const override;
    virtual bool IsPropertyStr(PropertyIndex_t iPropIndex) const override;
    virtual bool GetPropertyNumPtr(PropertyIndex_t iPropIndex, PropertyValNum_t* piOutVal) const override;
    virtual bool GetPropertyStrPtr(PropertyIndex_t iPropIndex, CSString *psOutVal, bool fZero = false) const override;
    virtual void SetPropertyNum(PropertyIndex_t iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion = RDS_PRET2A, bool fDeleteZero = true) override;
    virtual void SetPropertyStr(PropertyIndex_t iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion = RDS_PRET2A, bool fDeleteZero = true) override;
    virtual void DeletePropertyNum(PropertyIndex_t iPropIndex) override;
    virtual void DeletePropertyStr(PropertyIndex_t iPropIndex) override;

    virtual bool FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, PropertyIndex_t iPropIndex, bool fPropStr) override; // Use pLinkedObj = nullptr if calling this from CItemBase or CCharBase
    virtual bool FindWritePropVal(CSString & sVal, PropertyIndex_t iPropIndex, bool fPropStr) const override;
    virtual void r_Write(CScript & s) override;
    virtual void Copy(const CComponentProps *target) override;

    virtual void AddPropsTooltipData(CObjBase* pLinkedObj) override;

private:
    BaseContNum_t _mPropsNum;
    BaseContStr_t _mPropsStr;
};


#endif //_INC_CCPROPSCHAR_H
