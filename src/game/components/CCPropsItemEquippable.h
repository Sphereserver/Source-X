/**
* @file CCPropsItemEquippable.h
*
*/

#ifndef _INC_CCPROPSITEMEQUIPPABLE_H
#define _INC_CCPROPSITEMEQUIPPABLE_H

#include "../CComponentProps.h"

class CItemBase;
class CObjBase;
class CItem;


enum PROPIEQUIP_TYPE
{
    #define ADDPROP(a,b,c) PROPIEQUIP_##a,
    #include "../../tables/CCPropsItemEquippable_props.tbl"
    #undef ADDPROP
    PROPIEQUIP_QTY
};

class CCPropsItemEquippable : public CComponentProps
{
    static lpctstr const        _ptcPropertyKeys[];
    static RESDISPLAY_VERSION   _iPropertyExpansion[];

public:
    CCPropsItemEquippable();
    virtual ~CCPropsItemEquippable() = default;

    static bool CanSubscribe(const CItemBase* pItemBase);
    static bool CanSubscribe(const CItem* pItem);

    static bool IgnoreElementalProperty(int iPropIndex);

    virtual lpctstr GetName() const override {
        return "ItemEquippable";
    }
    virtual int GetPropsQty() const override {
        return PROPIEQUIP_QTY;
    }
    virtual KeyTableDesc_s GetPropertyKeysData() const override;
    virtual lpctstr GetPropertyName(int iPropIndex) const override;
    virtual bool IsPropertyStr(int iPropIndex) const override;
    virtual bool GetPropertyNumPtr(int iPropIndex, PropertyValNum_t* piOutVal) const override;
    virtual bool GetPropertyStrPtr(int iPropIndex, CSString *psOutVal, bool fZero = false) const override;
    virtual void SetPropertyNum(int iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion = RDS_QTY, bool fDeleteZero = false) override;
    virtual void SetPropertyStr(int iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion = RDS_QTY, bool fDeleteZero = false) override;
    virtual void DeletePropertyNum(int iPropIndex) override;
    virtual void DeletePropertyStr(int iPropIndex) override;

    virtual bool FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, int iPropIndex, bool fPropStr) override; // Use pLinkedObj = nullptr if calling this from CItemBase or CCharBase
    virtual bool FindWritePropVal(CSString & sVal, int iPropIndex, bool fPropStr) const override;
    virtual void r_Write(CScript & s) override;
    virtual void Copy(const CComponentProps *target) override;

    virtual void AddPropsTooltipData(CObjBase* pLinkedObj) override;

private:
    BaseContNum_t _mPropsNum;
    BaseContStr_t _mPropsStr;
};


#endif //_INC_CCPROPSITEMEQUIPPABLE_H
