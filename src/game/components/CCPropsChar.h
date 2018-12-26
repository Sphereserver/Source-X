/**
* @file CCPropsChar.h
*
*/

#ifndef _INC_CCPROPSCHAR_H
#define _INC_CCPROPSCHAR_H

#include "../CComponentProps.h"


enum PROPCH_TYPE
{
#define ADD(a,b) PROPCH_##a,
#include "../../tables/CCPropsChar_props.tbl"
#undef ADD
    PROPCH_QTY
};

class CCPropsChar : public CComponentProps
{
    //CBaseBase* _pLink;
    static lpctstr const _ptcPropertyKeys[];

public:
    CCPropsChar();
    virtual ~CCPropsChar() = default;

    //static bool CanSubscribe(const CObjBase* pObj);

    virtual lpctstr GetName() const override {
        return "Char";
    }
    virtual int GetPropsQty() const override {
        return PROPCH_QTY;
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
};


#endif //_INC_CCPROPSCHAR_H
