/**
* @file CComponentProps.h
*
*/

#ifndef _INC_CCOMPONENTPROPS_H
#define _INC_CCOMPONENTPROPS_H

#include "../common/sphere_library/CSString.h"
#include <map>

class CScript;
class CObjBase;
class CClientTooltip;


enum COMPPROPS_TYPE
{
    COMP_PROPS_CHAR,
    COMP_PROPS_ITEM,
    COMP_PROPS_ITEMCHAR,
    COMP_PROPS_ITEMEQUIPPABLE,
    COMP_PROPS_ITEMWEAPON,
    COMP_PROPS_ITEMWEAPONRANGED,
    COMP_PROPS_QTY
};

class CComponentProps
{
    COMPPROPS_TYPE _iType;

public:
    using PropertyValNum_t  = int;
    using BaseContNum_t = std::map<int, PropertyValNum_t>;  // <PropertyIndex (in the component-specific enum), PropertyVal>
    using BaseContNumPair_t = BaseContNum_t::value_type;
    using BaseContStr_t = std::map<int, CSString>;          // <PropertyIndex (in the component-specific enum), PropertyVal>
    using BaseContStrPair_t = BaseContStr_t::value_type;

    CComponentProps(COMPPROPS_TYPE type);

    virtual lpctstr GetName() const = 0;
    virtual int GetPropsQty() const = 0;

    virtual lpctstr GetPropertyName(int iPropIndex) const = 0;

    /*
    *@brief Is the value for this property a string? Or a number?
    *@param iPropIndex Property
    */
    virtual bool IsPropertyStr(int iPropIndex) const = 0;

    /*
    *@brief Retrieve the numerical value for the given property
    *@param iPropIndex Property index
    *@param piOutval Pointer to the variable that will hold the value
    *@return True if the variable was found, the value was numerical and it was stored in *piOutVal
    */
    virtual bool GetPropertyNumPtr(int iPropIndex, PropertyValNum_t* piOutVal) const = 0;

    /*
    *@brief Retrieve the string value for the given property
    *@param iPropIndex Property index
    *@param psOutVal Pointer to the variable that will hold the value
    *@param fZero If true, if the string value is existant but empty, retrieve "0"
    *@return True if the variable was found, the value was of string type and it value was stored in *pptcOutVal
    */
    virtual bool GetPropertyStrPtr(int iPropIndex, CSString *psOutVal, bool fZero = false) const = 0;

    /*
    *@brief Set the numerical value for the given property
    *@param iPropIndex Property index
    *@param iVal The value for the property
    *@param pLinkedObj The CObjBase-child we are setting the prop on. Use nullptr if you are storing the prop in a BaseDef! (CItemBase, CCharBase...)
    */
    virtual void SetPropertyNum(int iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj) = 0;

    /*
    *@brief Set the string value for the given property
    *@param iPropIndex Property index
    *@param ptcVal The value for the property
    *@param fZero If true, if ptcVal == nullptr or is empty, set "0"
    *@param pLinkedObj The CObjBase-child we are setting the prop on. Use nullptr if you are storing the prop in a BaseDef! (CItemBase, CCharBase...)
    */
    virtual void SetPropertyStr(int iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, bool fZero = false) = 0;

    /*
    @brief Delete the numerical property at the given index
    */
    virtual void DeletePropertyNum(int iPropIndex) = 0;

    /*
    @brief Delete the string property at the given index
    */
    virtual void DeletePropertyStr(int iPropIndex) = 0;

    /*
    @brief Generate and append the tooltip data for the given object
    @param pLinkedObj CObjBase to append the data to
    */
    virtual void AddTooltipData(CObjBase* pLinkedObj) = 0;

    /*
    *@brief Retrieve the numerical value for the given property
    *@param iPropIndex Property index
    *@return The value of the property (0 if not found)
    */
    PropertyValNum_t GetPropertyNum(int iPropIndex) const;

    /*
    *@brief Retrieve the string value for the given property
    *@param iPropIndex Property index
    *@return The string value of the property (empty if not found)
    */
    CSString GetPropertyStr(int iPropIndex, bool fZero = false) const;

protected:
    bool BaseCont_GetPropertyNum(const BaseContNum_t* container, int iPropIndex, PropertyValNum_t* piOutVal) const;
    bool BaseCont_GetPropertyStr(const BaseContStr_t* container, int iPropIndex, CSString *psOutVal, bool fZero = false) const;
    void BaseProp_LoadPropVal(int iPropIndex, bool fPropStr, CScript & s, CObjBase* pLinkedObj);
    bool BaseProp_WritePropVal(int iPropIndex, bool fPropStr, CSString & s);
    static void BaseCont_Write_ContNum(const BaseContNum_t* container, const lpctstr *ptcPropsTable, CScript &s);
    static void BaseCont_Write_ContStr(const BaseContStr_t* container, const lpctstr *ptcPropsTable, CScript &s);

public:
    virtual ~CComponentProps() = default;
    COMPPROPS_TYPE GetType() const;

    virtual bool r_LoadPropVal(CScript & s, CObjBase* pLinkedObj) = 0;      // Use pLinkedObj = nullptr if calling this from CItemBase or CCharBase
    virtual bool r_WritePropVal(lpctstr pszKey, CSString & s) = 0;
    virtual void r_Write(CScript & s) = 0;                                  // Storing data in the worldsave. Must be void, everything must be saved.
    virtual void Copy(const CComponentProps* copy) = 0;                     // Copy the contents to a new object.
};

#endif // _INC_CCOMPONENTPROPS_H
