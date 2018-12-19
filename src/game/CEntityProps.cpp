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
    ClearPropComponents();
}

void CEntityProps::ClearPropComponents()
{
    if (_List.empty())
        return;
    for (auto it = _List.begin(); it != _List.end(); ++it)
    {
        CComponentProps *pComponent = it->second;
        if (pComponent)
        {
            delete _List[pComponent->GetType()];
        }
    }
    _List.clear();
}

void CEntityProps::SubscribeComponentProps(CComponentProps * pComponent)
{
    COMPPROPS_TYPE compType = pComponent->GetType();
    if (_List.count(compType))
    {
        delete pComponent;
        ASSERT(0);  // This should never happen
        //g_Log.EventError("Trying to duplicate prop component (%d) for %s '0x%08x'\n", (int)pComponent->GetType(), pComponent->GetLink()->GetName(), pComponent->GetLink()->GetUID());
        return;
    }
    _List[compType] = pComponent;
}

void CEntityProps::UnsubscribeComponentProps(std::map<COMPPROPS_TYPE, CComponentProps*>::iterator& it, bool fEraseFromMap)
{
    delete it->second;
    if (fEraseFromMap)
    {
        it = _List.erase(it);
    }
}

void CEntityProps::UnsubscribeComponentProps(CComponentProps *pComponent)
{
    if (_List.empty())
    {
        return;
    }
    COMPPROPS_TYPE compType = pComponent->GetType();
    if (!_List.count(compType))
    {
        g_Log.EventError("Trying to unsuscribe not suscribed prop component (%d)\n", (int)pComponent->GetType());    // Should never happen?
        delete pComponent;
        return;
    }
    _List.erase(compType);  // iterator invalidation!
}

void CEntityProps::CreateSubscribeComponentProps(COMPPROPS_TYPE iComponentPropsType)
{
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
            ASSERT(0);
            break;
    }
}

bool CEntityProps::IsSubscribedComponentProps(CComponentProps *pComponent) const
{
    return (!_List.empty() && _List.count(pComponent->GetType()));
}

CComponentProps * CEntityProps::GetComponentProps(COMPPROPS_TYPE type)
{
    if (!_List.empty() && _List.count(type))
    {
        return _List.at(type);
    }
    return nullptr;
}

const CComponentProps * CEntityProps::GetComponentProps(COMPPROPS_TYPE type) const
{
    if (!_List.empty() && _List.count(type))
    {
        return _List.at(type);
    }
    return nullptr;
}

void CEntityProps::r_Write(CScript & s) // Storing data in the worldsave.
{
    if (_List.empty() && !s.IsWriteMode())
        return;
    for (auto it = _List.begin(); it != _List.end(); ++it)
    {
        CComponentProps *pComponent = it->second;
        if (pComponent)
        {
            pComponent->r_Write(s);
        }
    }
}

bool CEntityProps::r_LoadPropVal(CScript & s, CObjBase* pLinkedObj)
{
    if (_List.empty())
        return false;
    for (auto it = _List.begin(); it != _List.end(); ++it)
    {
        CComponentProps *pComponent = it->second;
        if (pComponent)
        {
            if (pComponent->r_LoadPropVal(s, pLinkedObj))   // Returns true if there is a match.
            {
                return true;
            }
        }
    }
    return false;
}

bool CEntityProps::r_WritePropVal(lpctstr pszKey, CSString & s)
{
    if (_List.empty())
        return false;
    for (auto it = _List.begin(); it != _List.end(); ++it)
    {
        CComponentProps *pComponent = it->second;
        if (pComponent)
        {
            if (pComponent->r_WritePropVal(pszKey, s))  // Returns true if there is a match.
            {
                return true;
            }
        }
    }
    return false;
}

void CEntityProps::AddTooltipData(CObjBase* pObj)
{
    if (_List.empty())
        return;
    for (auto it = _List.begin(); it != _List.end(); ++it)
    {
        CComponentProps *pComponent = it->second;
        if (pComponent)
        {
            pComponent->AddTooltipData(pObj);
        }
    }
    return;
}

void CEntityProps::Copy(const CEntityProps *target)
{
    if (_List.empty())
        return;
    for (auto it = target->_List.begin(); it != target->_List.end(); ++it)
    {
        CComponentProps *pTarget = it->second;    // the CComponent to copy from
        if (pTarget)
        {
            CComponentProps *pCopy = GetComponentProps(pTarget->GetType());    // the CComponent to copy to.
            if (pCopy)
            {
                pCopy->Copy(pTarget);
            }
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
                    pSrc->SysMessagef(pSrc->GetChar() ? "[%s]%s%s=%s" : "[%s]%s%s=%s\n", ptcCCPName, ptcPrefix, ptcPropName, sPropVal.GetPtr());
                else
                    g_Log.Event(LOGL_EVENT, pSrc->GetChar() ? "[%s]%s%s=%s" : "[%s]%s%s=%s\n", ptcCCPName, ptcPrefix, ptcPropName, sPropVal.GetPtr());
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
