/**
* @file CCPropsItemEquippable.h
*
*/

#ifndef _INC_CCPROPSITEMEQUIPPABLE_H
#define _INC_CCPROPSITEMEQUIPPABLE_H

#include "subcomponents/CFactionDef.h"
#include "../CComponentProps.h"

class CItemBase;
class CObjBase;
class CItem;


enum PROPIEQUIP_TYPE : CComponentProps::PropertyIndex_t
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
    static constexpr COMPPROPS_TYPE _kiType = COMP_PROPS_ITEMEQUIPPABLE;

    CCPropsItemEquippable();
    virtual ~CCPropsItemEquippable() = default;

    static bool CanSubscribe(const CItemBase* pItemBase) noexcept;
    static bool CanSubscribe(const CItem* pItem) noexcept;

    static bool IgnoreElementalProperty(PropertyIndex_t iPropIndex);

    virtual lpctstr GetName() const override {
        return "ItemEquippable";
    }
    virtual PropertyIndex_t GetPropsQty() const override {
        return PROPIEQUIP_QTY;
    }
    virtual KeyTableDesc_s GetPropertyKeysData() const override;
    virtual lpctstr GetPropertyName(PropertyIndex_t iPropIndex) const override;
    virtual bool IsPropertyStr(PropertyIndex_t iPropIndex) const override;
    virtual bool GetPropertyNumPtr(PropertyIndex_t iPropIndex, PropertyValNum_t* piOutVal) const override;
    virtual bool GetPropertyStrPtr(PropertyIndex_t iPropIndex, CSString *psOutVal, bool fZero = false) const override;
    virtual bool SetPropertyNum(PropertyIndex_t iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion = RDS_QTY, bool fDeleteZero = true) override;
    virtual bool SetPropertyStr(PropertyIndex_t iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion = RDS_QTY, bool fDeleteZero = true) override;
    virtual void DeletePropertyNum(PropertyIndex_t iPropIndex) override;
    virtual void DeletePropertyStr(PropertyIndex_t iPropIndex) override;

    virtual bool FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, PropertyIndex_t iPropIndex, bool fPropStr) override; // Use pLinkedObj = nullptr if calling this from CItemBase or CCharBase
    virtual bool FindWritePropVal(CSString & sVal, PropertyIndex_t iPropIndex, bool fPropStr) const override;
    virtual void r_Write(CScript & s) override;
    virtual void Copy(const CComponentProps *target) override;

    virtual void AddPropsTooltipData(CObjBase* pLinkedObj) override;

    inline const CFactionDef* GetFaction() const noexcept;
    inline CFactionDef* GetFaction() noexcept;

private:
    BaseContNum_t _mPropsNum;
    BaseContStr_t _mPropsStr;

    CFactionDef _faction;
};


const CFactionDef* CCPropsItemEquippable::GetFaction() const noexcept {
    return &_faction;
}
CFactionDef* CCPropsItemEquippable::GetFaction() noexcept {
    return &_faction;
}

#endif //_INC_CCPROPSITEMEQUIPPABLE_H
