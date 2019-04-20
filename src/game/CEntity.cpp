
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
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            pComponent->Delete(fForce);
        }
        UnsubscribeComponent(it, false);
    }
    _List.clear();
}

void CEntity::ClearComponents()
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::ClearComponents");
    if (_List.empty())
        return;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            delete _List[pComponent->GetType()];
        }
    }
    _List.clear();
}

void CEntity::SubscribeComponent(CComponent * pComponent)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::SubscribeComponent");
    COMP_TYPE compType = pComponent->GetType();
    if (_List.count(compType))
    {
        delete pComponent;
        PERSISTANT_ASSERT(0);  // This should never happen
        //g_Log.EventError("Trying to duplicate component (%d) for %s '0x%08x'\n", (int)pComponent->GetType(), pComponent->GetLink()->GetName(), pComponent->GetLink()->GetUID());
        return;
    }
    _List[compType] = pComponent;
}

void CEntity::UnsubscribeComponent(std::map<COMP_TYPE, CComponent*>::iterator& it, bool fEraseFromMap)
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
    if (!_List.count(compType))
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
    return (!_List.empty() && _List.count(pComponent->GetType()));
}

CComponent * CEntity::GetComponent(COMP_TYPE type)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::GetComponent");
    ASSERT((type >= 0) && (type < COMP_QTY));
    if (!_List.empty() && _List.count(type))
    {
        return _List[type];
    }
    return nullptr;
}

bool CEntity::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_GetRef");
    if (_List.empty())
        return false;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            if (pComponent->r_GetRef(pszKey, pRef))   // Returns true if there is a match.
            {
                return true;
            }
        }
    }
    return false;
}

void CEntity::r_Write(CScript & s) // Storing data in the worldsave.
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_Write");
    if (_List.empty() && !s.IsWriteMode())
        return;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            pComponent->r_Write(s);
        }
    }
}

bool CEntity::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_WriteVal");
    if (_List.empty())
        return false;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            if (pComponent->r_WriteVal(pszKey, sVal, pSrc))   // Returns true if there is a match.
            {
                return true;
            }
        }
    }
    return false;
}

bool CEntity::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_LoadVal");
    if (_List.empty())
        return false;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            if (pComponent->r_LoadVal(s))  // Returns true if there is a match.
            {
                return true;
            }
        }
    }
    return false;
}

bool CEntity::r_Verb(CScript & s, CTextConsole * pSrc) // Execute command from script.
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::r_Verb");
    if (_List.empty())
        return false;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            if (pComponent->r_Verb(s, pSrc))   // Returns true if there is a match.
            {
                return true;
            }
        }
    }
    return false;
}

void CEntity::Copy(const CEntity *target)
{
    ADDTOCALLSTACK_INTENSIVE("CEntity::Copy");
    if (_List.empty())
        return;
    for (std::map<COMP_TYPE, CComponent*>::const_iterator it = target->_List.begin(); it != target->_List.end(); ++it)
    {
        CComponent *pTarget = it->second;    // the CComponent to copy from
        if (pTarget)
        {
            CComponent *pCopy = GetComponent(pTarget->GetType());    // the CComponent to copy to.
            if (pCopy)
            {
                pCopy->Copy(pTarget);
            }
        }
    }
}

CCRET_TYPE CEntity::OnTick()
{
    ADDTOCALLSTACK("CEntity::OnTick");
    if (_List.empty())
        return CCRET_CONTINUE;

    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            CCRET_TYPE iRet = pComponent->OnTickComponent();
            if (iRet == CCRET_CONTINUE)   // Continue the loop.
            {
                continue;
            }
            else    // Stop the loop and return whatever return is needed.
            {
                return iRet;
            }
        }
    }
    return CCRET_CONTINUE;
}
