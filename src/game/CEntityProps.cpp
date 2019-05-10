#include "../CLog.h"
#include "chars/CChar.h"
#include "items/CItem.h"
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
    COMPPROPS_TYPE compType = pComponent->GetType();
    if (_List.end() != _List.find(compType))
    {
        delete pComponent;
        PERSISTANT_ASSERT(0);  // This should never happen
        //g_Log.EventError("Trying to duplicate prop component (%d) for %s '0x%08x'\n", (int)pComponent->GetType(), pComponent->GetLink()->GetName(), pComponent->GetLink()->GetUID());
        return;
    }
    _List[compType] = pComponent;
}

void CEntityProps::UnsubscribeComponentProps(std::map<COMPPROPS_TYPE, CComponentProps*>::iterator& it, bool fEraseFromMap)
{
    ADDTOCALLSTACK("CEntityProps::UnsubscribeComponentProps(it)");
    delete it->second;
    if (fEraseFromMap)
    {
        it = _List.erase(it);
    }
}

void CEntityProps::UnsubscribeComponentProps(CComponentProps *pComponent)
{
    ADDTOCALLSTACK("CEntityProps::UnsubscribeComponentProps");
    if (_List.empty())
    {
        return;
    }
    COMPPROPS_TYPE compType = pComponent->GetType();
    if (_List.end() == _List.find(compType))
    {
        g_Log.EventError("Trying to unsuscribe not suscribed prop component (%d)\n", (int)pComponent->GetType());    // Should never happen?
        delete pComponent;
        return;
    }
    _List.erase(compType);  // iterator invalidation!
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
    ASSERT((type >= 0) && (type < COMP_PROPS_QTY));
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
    if ( !_List.empty() && (_List.end() != _List.find(type)) )
    {
        return _List.at(type);
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
    RESDISPLAY_VERSION iLimitToExpansion = RDS_PRET2A;
    if (const CCharBase *pCharBase = dynamic_cast<CCharBase*>(pBaseEntityProps))
    {
        iLimitToExpansion = pCharBase->_iEraLimitProps;
    }

    int iPropIndex = -1;
    bool fPropStr = false;
    COMPPROPS_TYPE iCCPType = (COMPPROPS_TYPE)-1;
    auto _CEPLoopLoad = [&s, &iPropIndex, &fPropStr, &iCCPType, iLimitToExpansion](CEntityProps *pEP, CObjBase* pLinkedObj) -> bool
    {
        if (pEP->_List.empty())
            return false;
        const lpctstr ptcKey = s.GetKey();
        for (decltype(_List)::const_iterator it = pEP->_List.begin(), itEnd = pEP->_List.end(); it != itEnd; ++it)
        {
            if ( CComponentProps *pComponent = it->second )
            {
                const KeyTableDesc_s ktd = pComponent->GetPropertyKeysData();
                iPropIndex = FindTableSorted(ptcKey, ktd.pptcTable, ktd.iTableSize - 1);
                if (iPropIndex == -1)
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
        if (iCCPType == (COMPPROPS_TYPE)-1)
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
            ASSERT(iPropIndex != -1);
            pComp->FindLoadPropVal(s, nullptr, iLimitToExpansion, iPropIndex, fPropStr);
            return true;        // return true regardlessly of the value being set or not (it's still a valid property)
        }
    }

    return _CEPLoopLoad(pBaseEntityProps, nullptr);
}

bool CEntityProps::r_WritePropVal(lpctstr pszKey, CSString & sVal, const CObjBase *pObjEntityProps, const CBaseBaseDef *pBaseEntityProps) // static
{
    ADDTOCALLSTACK("CEntityProps::r_WritePropVal");
    // return false: invalid property for any of the subscribed components
    // return true: valid property, whether it has a defined value or not
   
    int iPropIndex = -1;
    bool fPropStr = false;
    COMPPROPS_TYPE iCCPType = (COMPPROPS_TYPE)-1;
    auto _CEPLoopWrite = [pszKey, &sVal, &iPropIndex, &fPropStr, &iCCPType](const CEntityProps *pEP) -> bool
    {
        if (pEP->_List.empty())
            return false;
        for (decltype(_List)::const_iterator it = pEP->_List.begin(), itEnd = pEP->_List.end(); it != itEnd; ++it)
        {
            const CComponentProps *pComponent = it->second;
            if (pComponent)
            {
                const KeyTableDesc_s ktd = pComponent->GetPropertyKeysData();
                iPropIndex = FindTableSorted(pszKey, ktd.pptcTable, ktd.iTableSize - 1);
                if (iPropIndex == -1)
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
        if (iCCPType == (COMPPROPS_TYPE)-1)
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
            ASSERT(iPropIndex != -1);
            pComp->FindWritePropVal(sVal, iPropIndex, fPropStr);
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
    for (int iCCP = 0; iCCP < COMP_PROPS_QTY; ++iCCP)
    {
        const CComponentProps* pCCP = GetComponentProps((COMPPROPS_TYPE)iCCP);
        if (!pCCP)
            continue;
        lpctstr ptcCCPName = pCCP->GetName();
        int iCCPQty = pCCP->GetPropsQty();
        for (int iPropIndex = 0; iPropIndex < iCCPQty; ++iPropIndex)
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
