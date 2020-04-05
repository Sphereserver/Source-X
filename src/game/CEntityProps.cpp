#include "../CLog.h"
#include "chars/CChar.h"
#include "components/CCPropsChar.h"
#include "components/CCPropsItem.h"
#include "components/CCPropsItemChar.h"
#include "components/CCPropsItemEquippable.h"
#include "components/CCPropsItemWeapon.h"
#include "components/CCPropsItemWeaponRanged.h"
#include "CEntityProps.h"

CEntityProps::CEntityProps()
{
    _List.clear();
}

CEntityProps::~CEntityProps()
{
    ADDTOCALLSTACK("CEntityProps::~CEntityProps");
    ClearPropComponents();
}

void CEntityProps::ClearPropComponents()
{
    ADDTOCALLSTACK("CEntityProps::ClearPropComponents");
    if (_List.empty())
        return;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponentProps *pComponent = it->second;
        ASSERT(pComponent);
        delete pComponent;
    }
    _List.clear();
}

void CEntityProps::SubscribeComponentProps(CComponentProps * pComponent)
{
    ADDTOCALLSTACK("CEntityProps::SubscribeComponentProps");
    const COMPPROPS_TYPE compType = pComponent->GetType();
    const auto pairResult = _List.try_emplace(compType, pComponent);
    if (pairResult.second == false)
    {
        delete pComponent;
        ASSERT(false);  // This should never happen
        //g_Log.EventError("Trying to duplicate prop component (%d) for %s '0x%08x'\n", (int)pComponent->GetType(), pComponent->GetLink()->GetName(), pComponent->GetLink()->GetUID());
        return;
    }
    //_List.container.shrink_to_fit();
}

void CEntityProps::UnsubscribeComponentProps(iterator& it, bool fEraseFromMap)
{
    ADDTOCALLSTACK("CEntityProps::UnsubscribeComponentProps(it)");
    delete it->second;
    if (fEraseFromMap)
    {
        it = _List.erase(it);
    }
    //_List.container.shrink_to_fit();
}

void CEntityProps::UnsubscribeComponentProps(CComponentProps *pComponent)
{
    ADDTOCALLSTACK("CEntityProps::UnsubscribeComponentProps");
    if (_List.empty())
    {
        return;
    }
    const COMPPROPS_TYPE compType = pComponent->GetType();
    auto it = _List.find(compType);
    if (it == _List.end())
    {
        g_Log.EventError("Trying to unsuscribe not suscribed prop component (%d)\n", (int)pComponent->GetType());    // Should never happen?
        delete pComponent;
        return;
    }
    _List.erase(it);  // iterator invalidation!
}

void CEntityProps::CreateSubscribeComponentProps(COMPPROPS_TYPE iComponentPropsType)
{
    ADDTOCALLSTACK("CEntityProps::CreateSubscribeComponentProps");
    switch (iComponentPropsType)
    {
        case COMP_PROPS_CHAR:
            SubscribeComponentProps(new CCPropsChar());
            break;
        case COMP_PROPS_ITEM:
            SubscribeComponentProps(new CCPropsItem());
            break;
        case COMP_PROPS_ITEMCHAR:
            SubscribeComponentProps(new CCPropsItemChar());
            break;
        case COMP_PROPS_ITEMEQUIPPABLE:
            SubscribeComponentProps(new CCPropsItemEquippable());
            break;
        case COMP_PROPS_ITEMWEAPON:
            SubscribeComponentProps(new CCPropsItemWeapon());
            break;
        case COMP_PROPS_ITEMWEAPONRANGED:
            SubscribeComponentProps(new CCPropsItemWeaponRanged());
            break;
        default:
            // This should NEVER happen! Add the new components here if missing, or check why an invalid component type was passed as argument
            PERSISTANT_ASSERT(0);
            break;
    }
}

bool CEntityProps::IsSubscribedComponentProps(CComponentProps *pComponent) const
{
    ADDTOCALLSTACK("CEntityProps::IsSubscribedComponentProps");
    return ( !_List.empty() && (_List.end() != _List.find(pComponent->GetType())) );
}

CComponentProps * CEntityProps::GetComponentProps(COMPPROPS_TYPE type)
{
    ADDTOCALLSTACK("CEntityProps::GetComponentProps");
    ASSERT(type < COMP_PROPS_QTY);
    if (_List.empty())
    {
        return nullptr;
    }
    auto it = _List.find(type);
    return (it != _List.end()) ? it->second : nullptr;
}

const CComponentProps * CEntityProps::GetComponentProps(COMPPROPS_TYPE type) const
{
    ADDTOCALLSTACK("CEntityProps::GetComponentProps(const)");
    if (!_List.empty())
    {
        auto it = _List.find(type);
        return (it == _List.end()) ? nullptr : it->second;
    }
    return nullptr;
}

void CEntityProps::r_Write(CScript & s) // Storing data in the worldsave.
{
    ADDTOCALLSTACK("CEntityProps::r_Write");
    if (_List.empty() && !s.IsWriteMode())
        return;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponentProps *pComponent = it->second;
        ASSERT(pComponent);
        pComponent->r_Write(s);
    }
}

bool CEntityProps::r_LoadPropVal(CScript & s, CObjBase* pObjEntityProps, CBaseBaseDef *pBaseEntityProps) // static
{
    ADDTOCALLSTACK("CEntityProps::r_LoadPropVal");
    // return false: invalid property for any of the subscribed components
    // return true: valid property, whether it has a defined value or not

    ASSERT(pBaseEntityProps);
    const RESDISPLAY_VERSION iLimitToExpansion = pBaseEntityProps->_iEraLimitProps;

    CComponentProps::PropertyIndex_t iPropIndex = (CComponentProps::PropertyIndex_t) -1;
    bool fPropStr = false;
    COMPPROPS_TYPE iCCPType = COMP_PROPS_INVALID;
    auto _CEPLoopLoad = [&s, &iPropIndex, &fPropStr, &iCCPType, iLimitToExpansion](CEntityProps *pEP, CObjBase* pLinkedObj) -> bool
    {
        if (pEP->_List.empty())
            return false;
        const lpctstr ptcKey = s.GetKey();
        for (const_iterator it = pEP->_List.begin(), itEnd = pEP->_List.end(); it != itEnd; ++it)
        {
            if ( CComponentProps *pComponent = it->second )
            {
                const KeyTableDesc_s ktd = pComponent->GetPropertyKeysData();
                iPropIndex = (CComponentProps::PropertyIndex_t) FindTableSorted(ptcKey, ktd.pptcTable, ktd.iTableSize - 1);
                if (iPropIndex == (CComponentProps::PropertyIndex_t) -1)
                {
                    // The key doesn't belong to this CComponentProps.
                    continue;
                }
                fPropStr = pComponent->IsPropertyStr(iPropIndex);
                if (pComponent->FindLoadPropVal(s, pLinkedObj, iLimitToExpansion, iPropIndex, fPropStr))
                {
                    // The property belongs to this CCP and it is set.
                    return true;
                }
                // The property belongs to this CCP but is not set. Remember which CCP this is, so if needed we can search directly for this CCP type instead of looping through all of them again.
                iCCPType = pComponent->GetType();
                return true;
            }
        }
        return false;
    };

    if (pObjEntityProps == nullptr)    // I'm calling it from a base CEntityProps
    {
        return _CEPLoopLoad(pBaseEntityProps, nullptr);  // pEntityProps is already a base cep
    }

    ASSERT(pObjEntityProps);
    if (_CEPLoopLoad(pObjEntityProps, pObjEntityProps)) // Check the object dynamic props first, then the base
    {
        // A subscribed CComponentProps can store this prop...
        if (iCCPType == COMP_PROPS_INVALID)
        {
            // The prop is set. We found it.
            return true;
        }
        else
        {
            // But the prop isn't set. Let's check the base. We already have iPropIndex and fPropStr.
            CComponentProps *pComp = pBaseEntityProps->GetComponentProps(iCCPType);
            if (!pComp)
                return true;    // The base doesn't have this component, but the obj did -> return true
            ASSERT(iPropIndex != (CComponentProps::PropertyIndex_t)-1);
            pComp->FindLoadPropVal(s, nullptr, iLimitToExpansion, (CComponentProps::PropertyIndex_t)iPropIndex, fPropStr);
            return true;        // return true regardlessly of the value being set or not (it's still a valid property)
        }
    }

    return _CEPLoopLoad(pBaseEntityProps, nullptr);
}

bool CEntityProps::r_WritePropVal(lpctstr ptcKey, CSString & sVal, const CObjBase *pObjEntityProps, const CBaseBaseDef *pBaseEntityProps) // static
{
    ADDTOCALLSTACK("CEntityProps::r_WritePropVal");
    // return false: invalid property for any of the subscribed components
    // return true: valid property, whether it has a defined value or not

    CComponentProps::PropertyIndex_t iPropIndex = (CComponentProps::PropertyIndex_t) -1;
    bool fPropStr = false;
    COMPPROPS_TYPE iCCPType = COMP_PROPS_INVALID;
    auto _CEPLoopWrite = [ptcKey, &sVal, &iPropIndex, &fPropStr, &iCCPType](const CEntityProps *pEP) -> bool
    {
        if (pEP->_List.empty())
            return false;
        for (const_iterator it = pEP->_List.begin(), itEnd = pEP->_List.end(); it != itEnd; ++it)
        {
            const CComponentProps *pComponent = it->second;
            if (pComponent)
            {
                const KeyTableDesc_s ktd = pComponent->GetPropertyKeysData();
                iPropIndex = (COMPPROPS_TYPE)FindTableSorted(ptcKey, ktd.pptcTable, ktd.iTableSize - 1);
                if (iPropIndex == (CComponentProps::PropertyIndex_t) -1)
                {
                    // The key doesn't belong to this CComponentProps.
                    continue;
                }
                fPropStr = pComponent->IsPropertyStr(iPropIndex);
                if (pComponent->FindWritePropVal(sVal, iPropIndex, fPropStr))
                {
                    // The property belongs to this CCP and it is set.
                    return true;
                }
                // The property belongs to this CCP but is not set. Remember which CCP this is, so if needed we can search directly for this CCP type instead of looping through all of them again.
                iCCPType = pComponent->GetType();
                return true;
            }
        }
        return false;
    };

    if (pObjEntityProps == nullptr)    // I'm calling it from a base CEntityProps
    {
        ASSERT(pBaseEntityProps);
        return _CEPLoopWrite(pBaseEntityProps);  // pEntityProps is already a base cep
    }

    ASSERT(pObjEntityProps);
    if (_CEPLoopWrite(pObjEntityProps)) // Check the object dynamic props first, then the base
    {
        // A subscribed CComponentProps can store this prop...
        if (iCCPType == COMP_PROPS_INVALID)
        {
            // The prop is set. We found it.
            return true;
        }
        else
        {
            // But the prop isn't set. Let's check the base. We already have iPropIndex and fPropStr.
            ASSERT(pBaseEntityProps);
            const CComponentProps *pComp = pBaseEntityProps->GetComponentProps(iCCPType);
            if (!pComp)
                return true;    // The base doesn't have this component, but the obj did -> return true
            ASSERT(iPropIndex != (CComponentProps::PropertyIndex_t)-1);
            pComp->FindWritePropVal(sVal, (CComponentProps::PropertyIndex_t)iPropIndex, fPropStr);
            return true;        // return true regardlessly of the value being set or not (it's still a valid property)
        }
    }

    if (pBaseEntityProps)
        return _CEPLoopWrite(pBaseEntityProps);
    return false;
}

void CEntityProps::AddPropsTooltipData(CObjBase* pObj)
{
    ADDTOCALLSTACK("CEntityProps::AddPropsTooltipData");
    if (_List.empty())
        return;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponentProps *pComponent = it->second;
        ASSERT(pComponent);
        pComponent->AddPropsTooltipData(pObj);
    }
    return;
}

void CEntityProps::Copy(const CEntityProps *target)
{
    ADDTOCALLSTACK("CEntityProps::Copy");
    if (_List.empty())
        return;
    for (auto it = target->_List.begin(), itEnd = target->_List.end(); it != itEnd; ++it)
    {
        CComponentProps *pTarget = it->second;    // the CComponent to copy from
        ASSERT(pTarget);
        CComponentProps *pCopy = GetComponentProps(pTarget->GetType());    // the CComponent to copy to.
        if (pCopy)
        {
            pCopy->Copy(pTarget);
        }
    }
}

void CEntityProps::DumpComponentProps(CTextConsole *pSrc, lpctstr ptcPrefix) const
{
    ADDTOCALLSTACK("CEntityProps::DumpComponentProps");
    // List out all the keys.
    ASSERT(pSrc);

    if ( ptcPrefix == nullptr )
        ptcPrefix = "";

    bool fIsClient = pSrc->GetChar();
    CSString sPropVal;
    CComponentProps::PropertyValNum_t iPropVal;
    for (CComponentProps::PropertyIndex_t iCCP = 0; iCCP < COMP_PROPS_QTY; ++iCCP)
    {
        const CComponentProps* pCCP = GetComponentProps((COMPPROPS_TYPE)iCCP);
        if (!pCCP)
            continue;
        lpctstr ptcCCPName = pCCP->GetName();
        const CComponentProps::PropertyIndex_t iCCPQty = pCCP->GetPropsQty();
        for (CComponentProps::PropertyIndex_t iPropIndex = 0; iPropIndex < iCCPQty; ++iPropIndex)
        {
            bool fPropFound;
            if (pCCP->IsPropertyStr(iPropIndex))
            {
                fPropFound = pCCP->GetPropertyStrPtr(iPropIndex, &sPropVal, false);
            }
            else
            {
                fPropFound = pCCP->GetPropertyNumPtr(iPropIndex, &iPropVal);
                if (fPropFound)
                    sPropVal.Format("%d", iPropVal);
            }

            if (fPropFound)
            {
                lpctstr ptcPropName = pCCP->GetPropertyName(iPropIndex);
                if (fIsClient)
                    pSrc->SysMessagef("%s[%s]%s=%s", ptcPrefix, ptcCCPName, ptcPropName, sPropVal.GetPtr());
                else
                    g_Log.Event(LOGL_EVENT,"%s[%s]%s=%s\n", ptcPrefix, ptcCCPName, ptcPropName, sPropVal.GetPtr());
            }

        }
    }
}


//--

const CCPropsChar* CEntityProps::GetCCPropsChar() const
{
    return static_cast<const CCPropsChar*>(GetComponentProps(COMP_PROPS_CHAR));
}
CCPropsChar* CEntityProps::GetCCPropsChar()
{
    return static_cast<CCPropsChar*>(GetComponentProps(COMP_PROPS_CHAR));
}

const CCPropsItem* CEntityProps::GetCCPropsItem() const
{
    return static_cast<const CCPropsItem*>(GetComponentProps(COMP_PROPS_ITEM));
}
CCPropsItem* CEntityProps::GetCCPropsItem()
{
    return static_cast<CCPropsItem*>(GetComponentProps(COMP_PROPS_ITEM));
}

const CCPropsItemChar* CEntityProps::GetCCPropsItemChar() const
{
    return static_cast<const CCPropsItemChar*>(GetComponentProps(COMP_PROPS_ITEMCHAR));
}
CCPropsItemChar* CEntityProps::GetCCPropsItemChar()
{
    return static_cast<CCPropsItemChar*>(GetComponentProps(COMP_PROPS_ITEMCHAR));
}

const CCPropsItemEquippable* CEntityProps::GetCCPropsItemEquippable() const
{
    return static_cast<const CCPropsItemEquippable*>(GetComponentProps(COMP_PROPS_ITEMEQUIPPABLE));
}
CCPropsItemEquippable* CEntityProps::GetCCPropsItemEquippable()
{
    return static_cast<CCPropsItemEquippable*>(GetComponentProps(COMP_PROPS_ITEMEQUIPPABLE));
}

const CCPropsItemWeapon* CEntityProps::GetCCPropsItemWeapon() const
{
    return static_cast<const CCPropsItemWeapon*>(GetComponentProps(COMP_PROPS_ITEMWEAPON));
}
CCPropsItemWeapon* CEntityProps::GetCCPropsItemWeapon()
{
    return static_cast<CCPropsItemWeapon*>(GetComponentProps(COMP_PROPS_ITEMWEAPON));
}

const CCPropsItemWeaponRanged* CEntityProps::GetCCPropsItemWeaponRanged() const
{
    return static_cast<const CCPropsItemWeaponRanged*>(GetComponentProps(COMP_PROPS_ITEMWEAPONRANGED));
}
CCPropsItemWeaponRanged* CEntityProps::GetCCPropsItemWeaponRanged()
{
    return static_cast<CCPropsItemWeaponRanged*>(GetComponentProps(COMP_PROPS_ITEMWEAPONRANGED));
}
