/**
* @file CCPropsItemWeaponRanged.h
*
*/

#ifndef _INC_CCPROPSITEMWEAPONRANGED_H
#define _INC_CCPROPSITEMWEAPONRANGED_H

#include "../CComponentProps.h"

class CObjBase;


enum PROPIWEAPRNG_TYPE : CComponentProps::PropertyIndex_t
{
    #define ADDPROP(a,b,c) PROPIWEAPRNG_##a,
    #include "../../tables/CCPropsItemWeaponRanged_props.tbl"
    #undef ADDPROP
    PROPIWEAPRNG_QTY
};

class CCPropsItemWeaponRanged : public CComponentProps
{
    static lpctstr const        _ptcPropertyKeys[];
    static RESDISPLAY_VERSION   _iPropertyExpansion[];

public:
    static constexpr COMPPROPS_TYPE _kiType = COMP_PROPS_ITEMWEAPONRANGED;

    CCPropsItemWeaponRanged();
    virtual ~CCPropsItemWeaponRanged() = default;

    static bool CanSubscribe(const CItemBase* pItemBase);
    static bool CanSubscribe(const CItem* pItem);


    virtual lpctstr GetName() const override {
        return "ItemWeaponRanged";
    }
    virtual PropertyIndex_t GetPropsQty() const override {
        return PROPIWEAPRNG_QTY;
    }
    virtual KeyTableDesc_s GetPropertyKeysData() const override;
    virtual lpctstr GetPropertyName(PropertyIndex_t iPropIndex) const override;
    virtual bool IsPropertyStr(PropertyIndex_t iPropIndex) const override;
    virtual bool GetPropertyNumPtr(PropertyIndex_t iPropIndex, PropertyValNum_t* piOutVal) const override;
    virtual bool GetPropertyStrPtr(PropertyIndex_t iPropIndex, CSString *psOutVal, bool fZero = false) const override;
    virtual void SetPropertyNum(PropertyIndex_t iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion = RDS_QTY, bool fDeleteZero = true) override;
    virtual void SetPropertyStr(PropertyIndex_t iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion = RDS_QTY, bool fDeleteZero = true) override;
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


#endif //_INC_CCPROPSITEMWEAPONRANGED_H