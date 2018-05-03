
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
            Unsuscribe(pComponent);
            pComponent->Delete(fForce);
        }
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
            _List[pComponent->GetType()] = nullptr;
        }
    }
    _List.clear();
}

void CEntity::Suscribe(CComponent * pComponent)
{
    if (_List[pComponent->GetType()])
    {
        g_Log.EventError("Trying to duplicate component (%d) for %s '0x%08x'\n", (int)pComponent->GetType(), pComponent->GetLink()->GetName(), pComponent->GetLink()->GetUID());
        delete pComponent;
        return;
    }
    _List[pComponent->GetType()] = pComponent;
}

void CEntity::Unsuscribe(CComponent *pComponent)
{
    if (!_List.size())
    {
        return;
    }
    if (!_List[pComponent->GetType()])
    {
        g_Log.EventError("Trying to unsuscribe not suscribed component (%d)\n", (int)pComponent->GetType());    // Should never happen?
        delete pComponent;
        return;
    }
    _List[pComponent->GetType()];
}

bool CEntity::IsSuscribed(CComponent *pComponent)
{
    if (_List.size() && _List[pComponent->GetType()])
    {
        return true;
    }
    return false;
}

CComponent * CEntity::GetComponent(COMP_TYPE type)
{
    if (_List.size() && _List[type])
    {
        return _List[type];
    }
    return nullptr;
}

bool CEntity::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
    if (!_List.size())
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
    if (!_List.size() && !s.IsWriteMode())
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
    if (!_List.size())
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
    if (!_List.size())
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
    if (!_List.size())
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
    if (!_List.size())
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
