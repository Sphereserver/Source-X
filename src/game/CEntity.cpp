
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
    if (_List.empty())
        return;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            pComponent->Delete(fForce);
        }
        Unsuscribe(it, false);
    }
    _List.clear();
}

void CEntity::ClearComponents()
{
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

void CEntity::Suscribe(CComponent * pComponent)
{
    COMP_TYPE compType = pComponent->GetType();
    if (_List.count(compType))
    {
        g_Log.EventError("Trying to duplicate component (%d) for %s '0x%08x'\n", (int)pComponent->GetType(), pComponent->GetLink()->GetName(), pComponent->GetLink()->GetUID());
        delete pComponent;
        return;
    }
    _List[compType] = pComponent;
}

void CEntity::Unsuscribe(std::map<COMP_TYPE, CComponent*>::iterator& it, bool fEraseFromMap)
{
    delete it->second;
    if (fEraseFromMap)
    {
        it = _List.erase(it);
    }
}

void CEntity::Unsuscribe(CComponent *pComponent)
{
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

bool CEntity::IsSuscribed(CComponent *pComponent) const
{
    if (!_List.empty() && _List.count(pComponent->GetType()))
    {
        return true;
    }
    return false;
}

CComponent * CEntity::GetComponent(COMP_TYPE type)
{
    if (!_List.empty() && _List.count(type))
    {
        return _List[type];
    }
    return nullptr;
}

bool CEntity::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
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

void CEntity::r_Write(CScript & s) ///< Storing data in the worldsave.
{
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

bool CEntity::r_Verb(CScript & s, CTextConsole * pSrc) ///< Execute command from script.
{
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

void CEntity::Copy(CEntity *target)
{
    if (_List.empty())
        return;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = target->_List.begin(); it != target->_List.end(); ++it)
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
    if (_List.empty())
        return CCRET_CONTINUE;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pComponent = it->second;
        if (pComponent)
        {
            CCRET_TYPE iRet = pComponent->OnTick();
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
