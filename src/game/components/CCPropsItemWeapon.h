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


enum PROPIWEAP_TYPE
{
    #define ADD(a,b) PROPIWEAP_##a,
    #include "../../tables/CCPropsItemWeapon_props.tbl"
    #undef ADD
    PROPIWEAP_QTY
};

class CCPropsItemWeapon : public CComponentProps
{
    static lpctstr const _ptcPropertyKeys[];

public:
    CCPropsItemWeapon();
    virtual ~CCPropsItemWeapon() = default;

    static bool CanSubscribe(const CItemBase* pItemBase);
    static bool CanSubscribe(const CItem* pItem);


    virtual lpctstr GetName() const override {
        return "ItemWeapon";
    }
    virtual int GetPropsQty() const override {
        return PROPIWEAP_QTY;
    }
    virtual lpctstr GetPropertyName(int iPropIndex) const override;
    virtual bool IsPropertyStr(int iPropIndex) const override;
    virtual bool GetPropertyNumPtr(int iPropIndex, PropertyValNum_t* piOutVal) const override;
    virtual bool GetPropertyStrPtr(int iPropIndex, CSString *psOutVal, bool fZero = false) const override;
    virtual void SetPropertyNum(int iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj) override;
    virtual void SetPropertyStr(int iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, bool fZero = false) override;
    virtual void DeletePropertyNum(int iPropIndex) override;
    virtual void DeletePropertyStr(int iPropIndex) override;

    virtual bool r_LoadPropVal(CScript & s, CObjBase* pLinkedObj) override; // Use pLinkedObj = nullptr if calling this from CItemBase or CCharBase
    virtual bool r_WritePropVal(lpctstr pszKey, CSString & s) override;
    virtual void r_Write(CScript & s) override;
    virtual void Copy(const CComponentProps *target) override;

    virtual void AddTooltipData(CObjBase* pLinkedObj) override;

private:
    BaseContNum_t _mPropsNum;
    BaseContStr_t _mPropsStr;

    int _iRange;
};


#endif //_INC_CCPROPSITEMWEAPON_H