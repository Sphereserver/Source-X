/**
* @file CEntityProps.h
*
*/

#ifndef _INC_CENTITYPROPS_H
#define _INC_CENTITYPROPS_H

//#include "../common/sphere_library/smap.h"
#include "CComponentProps.h"
#include <map>

class CCPropsChar;
class CCPropsItem;
class CCPropsItemChar;
class CCPropsItemEquippable;
class CCPropsItemWeapon;
class CCPropsItemWeaponRanged;
class CTextConsole;


class CEntityProps
{
    //tsdynamicmap<COMPPROPS_TYPE, CComponentProps*> _List;
    std::map<COMPPROPS_TYPE, CComponentProps*> _List;

public:
    CEntityProps();
    ~CEntityProps();

    /**
    * @brief Removes all contained CComponentProps.
    */
    void ClearPropComponents();

    /**
    * @brief Subscribes a CComponentProps.
    *
    * @param pComponent the CComponentProps to suscribe.
    */
    void SubscribeComponentProps(CComponentProps *pCCProp);

    /**
    * @brief Unsubscribes a CComponentProps. Use this if looping through the components with an iterator!
    *
    * @param it Iterator to the component to unsubscribe.
    * @param fEraseFromMap Should i erase this component from the internal map? Use false if you're going to erase it manually later
    */
    void UnsubscribeComponentProps(std::map<COMPPROPS_TYPE, CComponentProps*>::iterator& it, bool fEraseFromMap = true);

    /**
    * @brief Unsubscribes a CComponentProps.
    *
    * @param pComponent the CComponentProps to unsuscribe.
    */
    void UnsubscribeComponentProps(CComponentProps *pCCProp);

    /**
    * @brief Checks if a CComponentProps is actually susbcribed.
    *
    * @param pComponent the CComponentProps to check.
    * @return true if the CComponentProps is suscribed.
    */
    bool IsSubscribedComponentProps(CComponentProps *pCCProp) const;

    /**
    * @brief Creates and subscribes a CComponentProps to this CEntityProps.
    *
    * @param iComponentPropsType The CComponentProps type to create suscribe.
    */
    void CreateSubscribeComponentProps(COMPPROPS_TYPE iComponentPropsType);

    /**
    * @brief Gets a pointer to the given CComponent type.
    *
    * @param type the type of the CComponentProps to retrieve.
    * @return a pointer to the CComponentProps, if it is suscribed.
    */
    CComponentProps *GetComponentProps(COMPPROPS_TYPE type);
    const CComponentProps *GetComponentProps(COMPPROPS_TYPE type) const;

    /**
    * @brief Wrapper of base method.
    *
    * Will save important data on worldsave files.
    *
    * @param s the save file.
    * @param pRef a pointer to the object found.
    */
    void r_Write(CScript & s);

    /**
     * @brief Try to retrieve the property in one of the Prop Components subscribed to this Entity.
    *
    * @param pszKey the property's key to search for (eg: name, ResCold, etc).
    * @param sVal the storage that will contain property's value.
    *
    * @return true if there was a key to retrieve.
    */
    bool r_WritePropVal(lpctstr pszKey, CSString & s);

    /**
    * @brief Try to store the property in one of the Prop Components subscribed to this Entity.
    *
    * @param s the container with the keys and values to set.
    * @param pLinkedObj CObjBase from which the method is called. Use nullptr if calling from CItemBase or CCharBase.
    *
    * @return true if the prop was stored in a Component
    */
    bool r_LoadPropVal(CScript & s, CObjBase* pLinkedObj);

    /**
    *@brief Generate and the tooltip data for the properties inside this CEntityProps to pObj
    */
    void AddPropsTooltipData(CObjBase* pObj);

    /**
    * @brief Copies contents of the components from the target
    */
    void Copy(const CEntityProps *base);

    /**
    * @brief Copies contents of the components from the target
    */
    void DumpComponentProps(CTextConsole *pSrc, lpctstr ptcPrefix = nullptr) const;


    // Get the pointer for a given component, if found. Basically a wrapper, to avoid writing long lines of code
    const CCPropsChar* GetCCPropsChar() const;
    CCPropsChar* GetCCPropsChar();

    const CCPropsItem* GetCCPropsItem() const;
    CCPropsItem* GetCCPropsItem();

    const CCPropsItemChar* GetCCPropsItemChar() const;
    CCPropsItemChar* GetCCPropsItemChar();

    const CCPropsItemEquippable* GetCCPropsItemEquippable() const;
    CCPropsItemEquippable* GetCCPropsItemEquippable();

    const CCPropsItemWeapon* GetCCPropsItemWeapon() const;
    CCPropsItemWeapon* GetCCPropsItemWeapon();

    const CCPropsItemWeaponRanged* GetCCPropsItemWeaponRanged() const;
    CCPropsItemWeaponRanged* GetCCPropsItemWeaponRanged();
};

#endif // _INC_CENTITYPROPS_H
