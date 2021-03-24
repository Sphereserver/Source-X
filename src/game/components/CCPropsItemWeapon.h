/**
* @file CCPropsItemWeapon.h
*
*/

#ifndef _INC_CCPROPSITEMWEAPON_H
#define _INC_CCPROPSITEMWEAPON_H

#include "../CComponentProps.h"

class CItemBase;
class CObjBase;
class CItem;


enum PROPIWEAP_TYPE : CComponentProps::PropertyIndex_t
{
    #define ADDPROP(a,b,c) PROPIWEAP_##a,
    #include "../../tables/CCPropsItemWeapon_props.tbl"
    #undef ADDPROP
    PROPIWEAP_QTY
};

class CCPropsItemWeapon : public CComponentProps
{
    static lpctstr const        _ptcPropertyKeys[];
    static RESDISPLAY_VERSION   _iPropertyExpansion[];

public:
    static constexpr COMPPROPS_TYPE _kiType = COMP_PROPS_ITEMWEAPON;

    CCPropsItemWeapon();
    virtual ~CCPropsItemWeapon() = default;

    static bool CanSubscribe(const CItemBase* pItemBase);
    static bool CanSubscribe(const CItem* pItem);


    virtual lpctstr GetName() const override {
        return "ItemWeapon";
    }
    virtual PropertyIndex_t GetPropsQty() const override {
        return PROPIWEAP_QTY;
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

    ushort _uiRange;
};


#endif //_INC_CCPROPSITEMWEAPON_H