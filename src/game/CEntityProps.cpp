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
    _lComponentProps.clear();
}

CEntityProps::~CEntityProps()
{
    ADDTOCALLSTACK("CEntityProps::~CEntityProps");
    ClearPropComponents();
}

void CEntityProps::ClearPropComponents()
{
    ADDTOCALLSTACK("CEntityProps::ClearPropComponents");
    if (_lComponentProps.empty())
        return;
    for (auto it = _lComponentProps.begin(), itEnd = _lComponentProps.end(); it != itEnd; ++it)
    {
        CComponentProps *pComponent = it->second;
        ASSERT(pComponent);
        delete pComponent;
    }
    _lComponentProps.clear();
}

void CEntityProps::SubscribeComponentProps(CComponentProps * pComponent)
{
    ADDTOCALLSTACK("CEntityProps::SubscribeComponentProps");
    const COMPPROPS_TYPE compType = pComponent->GetType();
    const auto pairResult = _lComponentProps.try_emplace(compType, pComponent);
    if (pairResult.second == false)
    {
        delete pComponent;
        ASSERT(false);  // This should never happen
        //g_Log.EventError("Trying to duplicate prop component (%d) for %s '0x%08x'\n", (int)pComponent->GetType(), pComponent->GetLink()->GetName(), pComponent->GetLink()->GetUID());
        return;
    }
    //_lComponentProps.container.shrink_to_fit();
}

void CEntityProps::UnsubscribeComponentProps(iterator& it, bool fEraseFromMap)
{
    ADDTOCALLSTACK("CEntityProps::UnsubscribeComponentProps(it)");
    delete it->second;
    if (fEraseFromMap)
    {
        it = _lComponentProps.erase(it);
    }
    //_lComponentProps.container.shrink_to_fit();
}

void CEntityProps::UnsubscribeComponentProps(CComponentProps *pComponent)
{
    ADDTOCALLSTACK("CEntityProps::UnsubscribeComponentProps");
    if (_lComponentProps.empty())
    {
        return;
    }
    const COMPPROPS_TYPE compType = pComponent->GetType();
    auto it = _lComponentProps.find(compType);
    if (it == _lComponentProps.end())
    {
        g_Log.EventError("Trying to unsuscribe not suscribed prop component (%d)\n", (int)pComponent->GetType());    // Should never happen?
        delete pComponent;
        return;
    }
    _lComponentProps.erase(it);  // iterator invalidation!
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
    return ( !_lComponentProps.empty() && (_lComponentProps.end() != _lComponentProps.find(pComponent->GetType())) );
}

CComponentProps * CEntityProps::GetComponentProps(COMPPROPS_TYPE type)
{
    ADDTOCALLSTACK("CEntityProps::GetComponentProps");
    ASSERT(type < COMP_PROPS_QTY);
    if (_lComponentProps.empty())
    {
        return nullptr;
    }
    auto it = _lComponentProps.find(type);
    return (it != _lComponentProps.end()) ? it->second : nullptr;
}

const CComponentProps * CEntityProps::GetComponentProps(COMPPROPS_TYPE type) const
{
    ADDTOCALLSTACK("CEntityProps::GetComponentProps(const)");
    if (!_lComponentProps.empty())
    {
        auto it = _lComponentProps.find(type);
        return (it == _lComponentProps.end()) ? nullptr : it->second;
    }
    return nullptr;
}

void CEntityProps::r_Write(CScript & s) // Storing data in the worldsave.
{
    ADDTOCALLSTACK("CEntityProps::r_Write");
    if (_lComponentProps.empty() && !s.IsWriteMode())
        return;
    for (auto it = _lComponentProps.begin(), itEnd = _lComponentProps.end(); it != itEnd; ++it)
    {
        CComponentProps *pComponent = it->second;
        ASSERT(pComponent);
        pComponent->r_Write(s);
    }
}


bool CEntityProps::CEPLoopLoad(CEPLoopRet_t *pRet, CScript& s, CObjBase* pLinkedObj, const RESDISPLAY_VERSION iLimitToExpansion)
{
    const lpctstr ptcKey = s.GetKey();
    for (const auto& pairElem : _lComponentProps)
    {
        if (CComponentProps* pComponent = pairElem.second)
        {
            const KeyTableDesc_s ktd = pComponent->GetPropertyKeysData();
            pRet->iPropIndex = (CComponentProps::PropertyIndex_t) FindTableSorted(ptcKey, ktd.pptcTable, ktd.iTableSize - 1);
            if (pRet->iPropIndex == (CComponentProps::PropertyIndex_t) - 1)
            {
                // The key doesn't belong to this CComponentProps.
                continue;
            }
            pRet->fPropStr = pComponent->IsPropertyStr(pRet->iPropIndex);
            if (pComponent->FindLoadPropVal(s, pLinkedObj, iLimitToExpansion, pRet->iPropIndex, pRet->fPropStr))
            {
                // The property belongs to this CCP and it is set.
                return true;
            }
            // The property belongs to this CCP but is not set. Remember which CCP this is, so if needed we can search directly for this CCP type instead of looping through all of them again.
            pRet->iCCPType = pComponent->GetType();
            return true;
        }
    }
    return false;
}

bool CEntityProps::r_LoadPropVal(CScript & s, CObjBase* pObjEntityProps, CBaseBaseDef *pBaseEntityProps) // static
{
    ADDTOCALLSTACK("CEntityProps::r_LoadPropVal");
    // return false: invalid property for any of the subscribed components
    // return true: valid property, whether it has a defined value or not

    ASSERT(pBaseEntityProps);
    const RESDISPLAY_VERSION iLimitToExpansion = pBaseEntityProps->_iEraLimitProps;
    CEPLoopRet_t loopRet{ (CComponentProps::PropertyIndex_t) - 1, false, COMP_PROPS_INVALID };

    if (pObjEntityProps == nullptr)    // I'm calling it from a base CEntityProps
    {
        if (pBaseEntityProps->_lComponentProps.empty())
            return false;
        return pBaseEntityProps->CEPLoopLoad(&loopRet, s, nullptr, iLimitToExpansion);  // pEntityProps is already a base cep
    }

    ASSERT(pObjEntityProps);
    if (!pObjEntityProps->_lComponentProps.empty() && pObjEntityProps->CEPLoopLoad(&loopRet, s, pObjEntityProps, iLimitToExpansion)) // Check the object dynamic props first, then the base
    {
        // A subscribed CComponentProps can store this prop...
        if (loopRet.iCCPType == COMP_PROPS_INVALID)
        {
            // The prop is set. We found it.
            return true;
        }
        else
        {
            // But the prop isn't set. Let's check the base. We already have iPropIndex and fPropStr.
            CComponentProps *pComp = pBaseEntityProps->GetComponentProps(loopRet.iCCPType);
            if (!pComp)
                return true;    // The base doesn't have this component, but the obj did -> return true
            ASSERT(loopRet.iPropIndex != (CComponentProps::PropertyIndex_t)-1);
            pComp->FindLoadPropVal(s, nullptr, iLimitToExpansion, (CComponentProps::PropertyIndex_t)loopRet.iPropIndex, loopRet.fPropStr);
            return true;        // return true regardlessly of the value being set or not (it's still a valid property)
        }
    }

    return ((pBaseEntityProps->_lComponentProps.empty() == false) && pBaseEntityProps->CEPLoopLoad(&loopRet, s, nullptr, iLimitToExpansion));
}


bool CEntityProps::CEPLoopWrite(CEPLoopRet_t* pRet, lpctstr ptcKey, CSString& sVal) const
{
    for (const auto& pairElem : _lComponentProps)
    {
        if (CComponentProps* pComponent = pairElem.second)
        {
            const KeyTableDesc_s ktd = pComponent->GetPropertyKeysData();
            pRet->iPropIndex = (COMPPROPS_TYPE)FindTableSorted(ptcKey, ktd.pptcTable, ktd.iTableSize - 1);
            if (pRet->iPropIndex == (CComponentProps::PropertyIndex_t) - 1)
            {
                // The key doesn't belong to this CComponentProps.
                continue;
            }
            pRet->fPropStr = pComponent->IsPropertyStr(pRet->iPropIndex);
            if (pComponent->FindWritePropVal(sVal, pRet->iPropIndex, pRet->fPropStr))
            {
                // The property belongs to this CCP and it is set.
                return true;
            }
            // The property belongs to this CCP but is not set. Remember which CCP this is, so if needed we can search directly for this CCP type instead of looping through all of them again.
            pRet->iCCPType = pComponent->GetType();
            return true;
        }
    }
    return false;
}

bool CEntityProps::r_WritePropVal(lpctstr ptcKey, CSString & sVal, const CObjBase *pObjEntityProps, const CBaseBaseDef *pBaseEntityProps) // static
{
    ADDTOCALLSTACK("CEntityProps::r_WritePropVal");
    // return false: invalid property for any of the subscribed components
    // return true: valid property, whether it has a defined value or not

    CEPLoopRet_t loopRet{ (CComponentProps::PropertyIndex_t) - 1, false, COMP_PROPS_INVALID };

    if (pObjEntityProps == nullptr)    // I'm calling it from a base CEntityProps
    {
        ASSERT(pBaseEntityProps);
        if (pBaseEntityProps->_lComponentProps.empty())
            return false;

        return pBaseEntityProps->CEPLoopWrite(&loopRet, ptcKey, sVal);  // pEntityProps is already a base cep
    }

    ASSERT(pObjEntityProps);
    if (!pObjEntityProps->_lComponentProps.empty() && pObjEntityProps->CEPLoopWrite(&loopRet, ptcKey, sVal)) // Check the object dynamic props first, then the base
    {
        // A subscribed CComponentProps can store this prop...
        if (loopRet.iCCPType == COMP_PROPS_INVALID)
        {
            // The prop is set. We found it.
            return true;
        }
        else
        {
            // But the prop isn't set. Let's check the base. We already have iPropIndex and fPropStr.
            ASSERT(pBaseEntityProps);
            const CComponentProps *pComp = pBaseEntityProps->GetComponentProps(loopRet.iCCPType);
            if (!pComp)
                return true;    // The base doesn't have this component, but the obj did -> return true
            ASSERT(loopRet.iPropIndex != (CComponentProps::PropertyIndex_t)-1);
            pComp->FindWritePropVal(sVal, (CComponentProps::PropertyIndex_t)loopRet.iPropIndex, loopRet.fPropStr);
            return true;        // return true regardlessly of the value being set or not (it's still a valid property)
        }
    }

    return (pBaseEntityProps && (pBaseEntityProps->_lComponentProps.empty() == false) && pBaseEntityProps->CEPLoopWrite(&loopRet, ptcKey, sVal));
}

void CEntityProps::AddPropsTooltipData(CObjBase* pObj)
{
    ADDTOCALLSTACK("CEntityProps::AddPropsTooltipData");
    if (_lComponentProps.empty())
        return;
    for (auto it = _lComponentProps.begin(), itEnd = _lComponentProps.end(); it != itEnd; ++it)
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
    if (_lComponentProps.empty())
        return;
    for (auto it = target->_lComponentProps.begin(), itEnd = target->_lComponentProps.end(); it != itEnd; ++it)
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

    bool fIsClient = false;
    if (const CChar* pChar = pSrc->GetChar())
    {
        fIsClient = pChar->IsClient();
    }

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
                    pSrc->SysMessagef("%s[%s]%s=%s", ptcPrefix, ptcCCPName, ptcPropName, sPropVal.GetBuffer());
                else
                    g_Log.Event(LOGL_EVENT,"%s[%s]%s=%s\n", ptcPrefix, ptcCCPName, ptcPropName, sPropVal.GetBuffer());
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
