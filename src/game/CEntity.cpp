#include "../common/CException.h"
#include "../common/CLog.h"
#include "CEntity.h"
#include "CComponent.h"
#include "chars/CChar.h"
#include "items/CItem.h"
#include "CObjBase.h"


CEntity::CEntity()
{
    _lComponents.clear();
}

CEntity::~CEntity()
{
    ClearComponents();
}

void CEntity::Delete(bool fForce)
{
    ADDTOCALLSTACK_DEBUG("CEntity::Delete");
    if (_lComponents.empty())
        return;
    for (auto it = _lComponents.begin(), itEnd = _lComponents.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        pComponent->Delete(fForce);
    }
    _lComponents.clear();
}

void CEntity::ClearComponents()
{
    ADDTOCALLSTACK_DEBUG("CEntity::ClearComponents");
    if (_lComponents.empty())
        return;
    for (auto it = _lComponents.begin(), itEnd = _lComponents.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        delete pComponent;
    }
    _lComponents.clear();
}

void CEntity::SubscribeComponent(CComponent * pComponent)
{
    ADDTOCALLSTACK_DEBUG("CEntity::SubscribeComponent");
    const COMP_TYPE compType = pComponent->GetType();
    const auto pairResult = _lComponents.try_emplace(compType, pComponent);
    if (pairResult.second == false)
    {
        delete pComponent;
        ASSERT(false);  // This should never happen
        //g_Log.EventError("Trying to duplicate component (%d) for %s '0x%08x'\n", (int)pComponent->GetType(), pComponent->GetLink()->GetName(), pComponent->GetLink()->GetUID());
        return;
    }
    //_List.container.shrink_to_fit();
}

void CEntity::UnsubscribeComponent(CComponent *pComponent)
{
    ADDTOCALLSTACK_DEBUG("CEntity::UnsubscribeComponent");
    if (_lComponents.empty())
    {
        return;
    }
    const COMP_TYPE compType = pComponent->GetType();
    auto it = _lComponents.find(compType);
    if (it == _lComponents.end())
    {
        g_Log.EventError("Trying to unsuscribe not suscribed component (%d)\n", (int)pComponent->GetType());    // Should never happen?
        delete pComponent;
        return;
    }
    _lComponents.erase(it);  // iterator invalidation!
    //_List.container.shrink_to_fit();
}

bool CEntity::IsComponentSubscribed(CComponent *pComponent) const
{
    ADDTOCALLSTACK_DEBUG("CEntity::IsComponentSubscribed");
    return ( !_lComponents.empty() && (_lComponents.end() != _lComponents.find(pComponent->GetType())) );
}

CComponent * CEntity::GetComponent(COMP_TYPE type) const
{
    ADDTOCALLSTACK_DEBUG("CEntity::GetComponent");
    ASSERT(type < COMP_QTY);
    if (_lComponents.empty())
    {
        return nullptr;
    }
    auto it = _lComponents.find(type);
    return (it != _lComponents.end()) ? it->second : nullptr;
}

bool CEntity::r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK_DEBUG("CEntity::r_GetRef");
    if (_lComponents.empty())
        return false;
    for (auto it = _lComponents.begin(), itEnd = _lComponents.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        if (pComponent->r_GetRef(ptcKey, pRef))   // Returns true if there is a match.
        {
            return true;
        }
    }
    return false;
}

void CEntity::r_Write(CScript & s) // Storing data in the worldsave.
{
    ADDTOCALLSTACK_DEBUG("CEntity::r_Write");
    if (_lComponents.empty() && !s.IsWriteMode())
        return;
    for (auto it = _lComponents.begin(), itEnd = _lComponents.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        pComponent->r_Write(s);
    }
}

bool CEntity::r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK_DEBUG("CEntity::r_WriteVal");
    if (_lComponents.empty())
        return false;
    for (auto it = _lComponents.begin(), itEnd = _lComponents.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        if (pComponent->r_WriteVal(ptcKey, sVal, pSrc))   // Returns true if there is a match.
        {
            return true;
        }
    }
    return false;
}

bool CEntity::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK_DEBUG("CEntity::r_LoadVal");
    if (_lComponents.empty())
        return false;
    for (auto it = _lComponents.begin(), itEnd = _lComponents.end(); it != itEnd; ++it)
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
    ADDTOCALLSTACK_DEBUG("CEntity::r_Verb");
    EXC_TRY("Verb");

    if (_lComponents.empty())
        return false;
    for (auto it = _lComponents.begin(), itEnd = _lComponents.end(); it != itEnd; ++it)
    {
        CComponent *pComponent = it->second;
        ASSERT(pComponent);
        if (pComponent->r_Verb(s, pSrc))   // Returns true if there is a match.
        {
            return true;
        }
    }

    EXC_CATCH;
    return false;
}

void CEntity::Copy(const CEntity *target)
{
    ADDTOCALLSTACK_DEBUG("CEntity::Copy");
    if (_lComponents.empty())
        return;
    for (auto it = target->_lComponents.begin(), itEnd = target->_lComponents.end(); it != itEnd; ++it)
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

CCRET_TYPE CEntity::_OnTick()
{
    ADDTOCALLSTACK_DEBUG("CEntity::_OnTick");
    if (_lComponents.empty())
        return CCRET_CONTINUE;

    for (auto it = _lComponents.begin(), itEnd = _lComponents.end(); it != itEnd; ++it)
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
