/**
* @file CComponentProps.h
*
*/

#ifndef _INC_CCOMPONENTPROPS_H
#define _INC_CCOMPONENTPROPS_H

#include "../common/flat_containers/flat_map.hpp"
#include "../common/sphere_library/CSString.h"
#include "game_enums.h"

class CScript;
class CObjBase;
class CClientTooltip;


enum COMPPROPS_TYPE : uchar
{
    COMP_PROPS_ITEMCHAR,
    COMP_PROPS_CHAR,
    COMP_PROPS_ITEM,
    COMP_PROPS_ITEMEQUIPPABLE,
    COMP_PROPS_ITEMWEAPON,
    COMP_PROPS_ITEMWEAPONRANGED,
    COMP_PROPS_QTY,

    COMP_PROPS_INVALID = 255
};

class CComponentProps
{
    static lpctstr const _ptcPropertyKeys[];
    COMPPROPS_TYPE _iType;

public:
    using PropertyIndex_t   = uchar;
    //static constexpr PropertyIndex_t kiPropertyIndexInvalid = (PropertyIndex_t)-1;

    using PropertyValNum_t  = int;
    using BaseContNum_t = fc::vector_map<PropertyIndex_t, PropertyValNum_t>;  // <PropertyIndex (in the component-specific enum), PropertyVal>
    using BaseContNumPair_t = BaseContNum_t::value_type;

    using BaseContStr_t = fc::vector_map<PropertyIndex_t, CSString>;          // <PropertyIndex (in the component-specific enum), PropertyVal>
    using BaseContStrPair_t = BaseContStr_t::value_type;


    CComponentProps(COMPPROPS_TYPE type) noexcept {
        _iType = type;
    }

    virtual lpctstr GetName() const = 0;
    virtual PropertyIndex_t GetPropsQty() const = 0;
    virtual KeyTableDesc_s GetPropertyKeysData() const = 0;

    virtual lpctstr GetPropertyName(PropertyIndex_t iPropIndex) const = 0;

    /*
    *@brief Is the value for this property a string? Or a number?
    *@param iPropIndex Property
    */
    virtual bool IsPropertyStr(PropertyIndex_t iPropIndex) const = 0;

    /*
    *@brief Retrieve the numerical value for the given property
    *@param iPropIndex Property index
    *@param piOutval Pointer to the variable that will hold the value
    *@return True if the variable was found, the value was numerical and it was stored in *piOutVal
    */
    virtual bool GetPropertyNumPtr(PropertyIndex_t iPropIndex, PropertyValNum_t* piOutVal) const = 0;

    /*
    *@brief Retrieve the string value for the given property
    *@param iPropIndex Property index
    *@param psOutVal Pointer to the variable that will hold the value
    *@param fZero If true, if the string value is existant but empty, retrieve "0"
    *@return True if the variable was found, the value was of string type and it value was stored in *pptcOutVal
    */
    virtual bool GetPropertyStrPtr(PropertyIndex_t iPropIndex, CSString *psOutVal, bool fZero = false) const = 0;

    /*
    *@brief Set the numerical value for the given property
    *@param iPropIndex Property index
    *@param iVal The value for the property
    *@param pLinkedObj The CObjBase-child we are setting the prop on. Use nullptr if you are storing the prop in a BaseDef! (CItemBase, CCharBase...)
    *@param iLimitToExpansion This EntityProps accepts/stores/uses properties only up to this expansion.
    *@param fDeleteZero If true, if iVal == 0 or is empty, delete the prop (if already existing)
    */
    virtual void SetPropertyNum(PropertyIndex_t iPropIndex, PropertyValNum_t iVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero = true) = 0;

    /*
    *@brief Set the string value for the given property
    *@param iPropIndex Property index
    *@param ptcVal The value for the property
    *@param pLinkedObj The CObjBase-child we are setting the prop on. Use nullptr if you are storing the prop in a BaseDef! (CItemBase, CCharBase...)
    *@param iLimitToExpansion This EntityProps accepts/stores/uses properties only up to this expansion.
    *@param fDeleteZero If true, if ptcVal == nullptr or is empty, delete the prop (if already existing)
    */
    virtual void SetPropertyStr(PropertyIndex_t iPropIndex, lpctstr ptcVal, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, bool fDeleteZero = true) = 0;

    /*
    @brief Delete the numerical property at the given index
    */
    virtual void DeletePropertyNum(PropertyIndex_t iPropIndex) = 0;

    /*
    @brief Delete the string property at the given index
    */
    virtual void DeletePropertyStr(PropertyIndex_t iPropIndex) = 0;

    /*
    @brief Generate and append the tooltip data for the given object
    @param pLinkedObj CObjBase to append the data to
    */
    virtual void AddPropsTooltipData(CObjBase* pLinkedObj) = 0;

    /*
    *@brief Retrieve the numerical value for the given property
    *@param iPropIndex Property index
    *@return The value of the property (0 if not found)
    */
    PropertyValNum_t GetPropertyNum(PropertyIndex_t iPropIndex) const;

    /*
    *@brief Retrieve the string value for the given property
    *@param iPropIndex Property index
    *@return The string value of the property (empty if not found)
    */
    CSString GetPropertyStr(PropertyIndex_t iPropIndex, bool fZero = false) const;

protected:
    bool BaseCont_GetPropertyNum(const BaseContNum_t* container, PropertyIndex_t iPropIndex, PropertyValNum_t* piOutVal) const;
    bool BaseCont_GetPropertyStr(const BaseContStr_t* container, PropertyIndex_t iPropIndex, CSString *psOutVal, bool fZero = false) const;
    void BaseProp_LoadPropVal(PropertyIndex_t iPropIndex, bool fPropStr, CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion);
    bool BaseProp_WritePropVal(PropertyIndex_t iPropIndex, bool fPropStr, CSString & sVal) const;
    static void BaseCont_Write_ContNum(const BaseContNum_t* container, const lpctstr *ptcPropsTable, CScript &s);
    static void BaseCont_Write_ContStr(const BaseContStr_t* container, const lpctstr *ptcPropsTable, CScript &s);

public:
    virtual ~CComponentProps() noexcept = default;
    
    inline COMPPROPS_TYPE GetType() const noexcept {
        return _iType;
    }

    /**
    *@brief Check if a property can be stored inside this CComponentProps, if it can, store it.
    *@param s The input script
    *@param pLinkedObj The CObjBase which is attached this CEntityProps. Use pLinkedObj = nullptr if calling this from CItemBase or CCharBase instead
    *@param iLimitToExpansion This EntityProps accepts/stores/uses properties only up to this expansion.
    *@param iPropIndex The index of the property in this CComponentProps specialization-specific key table
    *@param fPropStr If the property is string-type
    *@return true if the property could be stored inside this component
    */
    virtual bool FindLoadPropVal(CScript & s, CObjBase* pLinkedObj, RESDISPLAY_VERSION iLimitToExpansion, PropertyIndex_t iPropIndex, bool fPropStr) = 0;

    /**
    *@brief Retrieve a property from this CComponentProps.
    *@param s The output string. Contains the value of the property, if it could be retrieved.
    *@param iPropIndex The index of the property in this CComponentProps specialization-specific key table
    *@param fPropStr If the property is string-type
    *@return true if the property is present in this component. The value is stored in the param s.
    */
    virtual bool FindWritePropVal(CSString & s, PropertyIndex_t iPropIndex, bool fPropStr) const = 0;

    virtual void r_Write(CScript & s) = 0;                                  // Storing data in the worldsave. Must be void, everything must be saved.
    virtual void Copy(const CComponentProps* copy) = 0;                     // Copy the contents to a new object.
};

#endif // _INC_CCOMPONENTPROPS_H
