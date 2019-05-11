
#include "CEntity.h"
#include "CComponent.h"
#include "../CLog.h"
#include "CObjBase.h"
#include "chars/CChar.h"
#include "items/CItem.h"


CEntity::CEntity()
{
    _List.clear();
}

CEntity::~CEntity()
{
    ClearComponents();
}

void CEntity::Delete(bool fForce)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::Delete");
    if (_List.empty())
        return;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        pComponent->Delete(fForce);
    }
    _List.clear();
}

void CEntity::ClearComponents()
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::ClearComponents");
    if (_List.empty())
        return;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        delete pComponent;
    }
    _List.clear();
}

void CEntity::SubscribeComponent(CComponent * pComponent)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::SubscribeComponent");
    COMP_TYPE compType = pComponent->GetType();
    if (_List.end() != _List.find(compType))
    {
        delete pComponent;
        PERSISTANT_ASSERT(0);  // This should never happen
        //g_Log.EventError("Trying to duplicate component (%d) for %s '0x%08x'\n", (int)pComponent->GetType(), pComponent->GetLink()->GetName(), pComponent->GetLink()->GetUID());
        return;
    }
    _List[compType] = pComponent;
}

void CEntity::UnsubscribeComponent(iterator& it, bool fEraseFromMap)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::UnsubscribeComponent(it)");
    delete it->second;
    if (fEraseFromMap)
    {
        it = _List.erase(it);
    }
}

void CEntity::UnsubscribeComponent(CComponent *pComponent)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::UnsubscribeComponent");
    if (_List.empty())
    {
        return;
    }
    COMP_TYPE compType = pComponent->GetType();
    if (_List.end() == _List.find(compType))
    {
        g_Log.EventError("Trying to unsuscribe not suscribed component (%d)\n", (int)pComponent->GetType());    // Should never happen?
        delete pComponent;
        return;
    }
    _List.erase(compType);  // iterator invalidation!
}

bool CEntity::IsComponentSubscribed(CComponent *pComponent) const
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::IsComponentSubscribed");
    return ( !_List.empty() && (_List.end() != _List.find(pComponent->GetType())) );
}

CComponent * CEntity::GetComponent(COMP_TYPE type) const
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::GetComponent");
    ASSERT((type >= 0) && (type < COMP_QTY));
    if (_List.empty())
    {
        return nullptr;
    }
    auto it = _List.find(type);
    return (it != _List.end()) ? it->second : nullptr;
}

bool CEntity::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_GetRef");
    if (_List.empty())
        return false;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        if (pComponent->r_GetRef(pszKey, pRef))   // Returns true if there is a match.
        {
            return true;
        }
    }
    return false;
}

void CEntity::r_Write(CScript & s) // Storing data in the worldsave.
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_Write");
    if (_List.empty() && !s.IsWriteMode())
        return;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        pComponent->r_Write(s);
    }
}

bool CEntity::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_WriteVal");
    if (_List.empty())
        return false;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        if (pComponent->r_WriteVal(pszKey, sVal, pSrc))   // Returns true if there is a match.
        {
            return true;
        }
    }
    return false;
}

bool CEntity::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_LoadVal");
    if (_List.empty())
        return false;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        if (pComponent->r_LoadVal(s))  // Returns true if there is a match.
        {
            return true;
        }
    }
    return false;
}

bool CEntity::r_Verb(CScript & s, CTextConsole * pSrc) // Execute command from script.
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_Verb");
    if (_List.empty())
        return false;
    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        if (pComponent->r_Verb(s, pSrc))   // Returns true if there is a match.
        {
            return true;
        }
    }
    return false;
}

void CEntity::Copy(const CEntity *target)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::Copy");
    if (_List.empty())
        return;
    for (auto it = target->_List.begin(), itEnd = target->_List.end(); it != itEnd; ++it)
    {
        CComponent *pTarget = it->second;    // the CComponent to copy from
        ASSERT(pTarget);
        CComponent *pCopy = GetComponent(pTarget->GetType());    // the CComponent to copy to.
        if (pCopy)
        {
            pCopy->Copy(pTarget);
        }
    }
}

CCRET_TYPE CEntity::OnTick()
{
    ADDTOCALLSTACK("CEntity::OnTick");
    if (_List.empty())
        return CCRET_CONTINUE;

    for (auto it = _List.begin(), itEnd = _List.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        CCRET_TYPE iRet = pComponent->OnTickComponent();
        if (iRet != CCRET_CONTINUE)   
        {
            return iRet;    // Stop the loop and return whatever return is needed.
        }
        // Otherwise, continue the loop.
    }
    return CCRET_CONTINUE;
}
