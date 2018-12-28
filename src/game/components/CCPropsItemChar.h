/**
* @file CCPropsItemChar.h
*
*/

#ifndef _INC_CCPROPSITEMCHAR_H
#define _INC_CCPROPSITEMCHAR_H

#include "../CComponentProps.h"


enum PROPITCH_TYPE
{
#define ADD(a,b) PROPITCH_##a,
#include "../../tables/CCPropsItemChar_props.tbl"
#undef ADD
    PROPITCH_QTY
};

class CCPropsItemChar : public CComponentProps
{
    //CBaseBase* _pLink;
    static lpctstr const _ptcPropertyKeys[];

public:
    CCPropsItemChar();
    virtual ~CCPropsItemChar() = default;

    //static bool CanSubscribe(const CObjBase* pObj);

    virtual lpctstr GetName() const override {
        return "ItemChar";
    }
    virtual int GetPropsQty() const override {
        return PROPITCH_QTY;
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

    virtual void AddPropsTooltipData(CObjBase* pLinkedObj) override;

private:
    BaseContNum_t _mPropsNum;
    BaseContStr_t _mPropsStr;
};


#endif //_INC_CCPROPSITEMCHAR_H
