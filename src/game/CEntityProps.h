/**
* @file CEntityProps.h
*
*/

#ifndef _INC_CENTITYPROPS_H
#define _INC_CENTITYPROPS_H

#include "../common/flat_containers/flat_map.hpp"
#include "CComponentProps.h"
#include <memory>

class CCPropsChar;
class CCPropsItem;
class CCPropsItemChar;
class CCPropsItemEquippable;
class CCPropsItemWeapon;
class CCPropsItemWeaponRanged;

class CTextConsole;
class CObjBase;
struct CBaseBaseDef;


class CEntityProps
{
    fc::vector_map<COMPPROPS_TYPE, std::unique_ptr<CComponentProps>> _lComponentProps;
    using iterator          = decltype(_lComponentProps)::iterator;
    using const_iterator    = decltype(_lComponentProps)::const_iterator;

    struct CEPLoopRet_t
    {
        CComponentProps::PropertyIndex_t iPropIndex;
        bool fPropStr;
        COMPPROPS_TYPE iCCPType;
    };
    bool CEPLoopLoad (CEPLoopRet_t* pRet, CScript& s, CObjBase* pLinkedObj, const RESDISPLAY_VERSION iLimitToExpansion);
    bool CEPLoopWrite(CEPLoopRet_t* pRet, lpctstr ptcKey, CSString& sVal) const;

public:
    CEntityProps();
    ~CEntityProps();

    /**
    * @brief Removes all contained CComponentProps.
    */
    void ClearPropComponents();

    /**
    * @brief Subscribes a CComponentProps.
    */
    template<class _ComponentType, class... _ArgsType>
    _ComponentType* TrySubscribeComponentProps(_ArgsType&&... args);

    /**
    * @brief Subscribes a CComponentProps, if it can be subscribed.
    */
    template<class _ComponentType, class _SubscriberType, class... _ArgsType>
    _ComponentType* TrySubscribeAllowedComponentProps(const _SubscriberType* pSubscriber, _ArgsType&&... args);

    /**
    * @brief Unsubscribes a CComponentProps.
    *   Never unsubscribe Props Components, because if the type is changed to an unsubscribable type and then again to the previous type, the component will be deleted and created again.
    *   This means that all the properties (base and "dynamic") are lost.
    */
    template<class _ComponentType>
    bool UnsubscribeComponentProps();

    /**
    * @brief Unsubscribes a CComponentProps.
    *
    * @param pComponent the CComponentProps to unsuscribe.
    */
    void UnsubscribeComponentProps(CComponentProps *pCCProp);

    /**
    * @brief Gets a pointer to the given CComponent type.
    *
    * @return a pointer to the CComponentProps, if it is subscribed.
    */
    template<class _ComponentType>
    const _ComponentType* GetComponentProps() const noexcept;

    template<class _ComponentType>
    _ComponentType* GetComponentProps() noexcept;

    /**
    * @brief Gets a pointer to the given CComponent type.
    *
    * @param type the type of the CComponentProps to retrieve.
    * @return a pointer to the CComponentProps, if it is suscribed.
    */
    CComponentProps* GetComponentProps(COMPPROPS_TYPE type);
    const CComponentProps* GetComponentProps(COMPPROPS_TYPE type) const;

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
    * @param ptcKey the property's key to search for (eg: name, ResCold, etc).
    * @param sVal the storage that will contain property's value.
    * @param pObjEntityProps The CObjBase holding the dynamic properties. nullptr if this method is called from a CBaseBaseDef.
    * @param pBaseEntityProps The CBaseBaseDef holding the base properties.
    *
    * @return true if there was a key to retrieve.
    */
    static bool r_WritePropVal(lpctstr ptcKey, CSString & sVal, const CObjBase *pObjEntityProps, const CBaseBaseDef *pBaseEntityProps);

    /**
    * @brief Try to store the property in one of the Prop Components subscribed to this Entity.
    *
    * @param s the container with the keys and values to set.
    * @param pObjEntityProps The CObjBase holding the dynamic properties. nullptr if this method is called from a CBaseBaseDef.
    * @param pBaseEntityProps The CBaseBaseDef holding the base properties. Can't be nullptr.
    *
    * @return true if the prop was stored in a Component
    */
    static bool r_LoadPropVal(CScript & s, CObjBase *pObjEntityProps, CBaseBaseDef *pBaseEntityProps);

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
};


// Template methods definition

template<class _ComponentType>
const _ComponentType* CEntityProps::GetComponentProps() const noexcept
{
    if (_lComponentProps.empty()) {
        return nullptr;
    }
    auto it = _lComponentProps.find(_ComponentType::_kiType);
    return (it == _lComponentProps.end()) ? nullptr : static_cast<_ComponentType*>(it->second.get());
}

template<class _ComponentType>
_ComponentType* CEntityProps::GetComponentProps() noexcept
{
    if (_lComponentProps.empty()) {
        return nullptr;
    }
    auto it = _lComponentProps.find(_ComponentType::_kiType);
    return (it == _lComponentProps.end()) ? nullptr : static_cast<_ComponentType*>(it->second.get());
}

template<class _ComponentType, class... _ArgsType>
_ComponentType* CEntityProps::TrySubscribeComponentProps(_ArgsType&&... args)
{
    auto retPair = _lComponentProps.try_emplace(_ComponentType::_kiType, std::make_unique<_ComponentType>(std::forward<_ArgsType>(args)...));
    return static_cast<_ComponentType*>(retPair.first->second.get());
}

template<class _ComponentType, class _SubscriberType, class... _ArgsType>
_ComponentType* CEntityProps::TrySubscribeAllowedComponentProps(const _SubscriberType *pSubscriber, _ArgsType&&... args)
{
    if (!_ComponentType::CanSubscribe(pSubscriber))
        return nullptr;
    
    return TrySubscribeComponentProps<_ComponentType, _ArgsType...>(std::forward<_ArgsType>(args)...);
}

template<class _ComponentType>
bool CEntityProps::UnsubscribeComponentProps()
{
    auto it = _lComponentProps.find(_ComponentType::_kiType);
    if (it == _lComponentProps.end()) {
        return false;
    }
    _lComponentProps.erase(it);
    return true;
}

#endif // _INC_CENTITYPROPS_H
