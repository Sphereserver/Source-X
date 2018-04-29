
#include "CEntity.h"
#include "CComponent.h"
#include "../CLog.h"


CEntity::~CEntity()
{
    if (!_List.size())
        return;
    _List.clear();  // Clearing the map will also destruct it's CComponents.
}

void CEntity::Suscribe(CComponent * pEntity)
{
    if (_List[pEntity->GetType()])
    {
        g_Log.EventError("Trying to duplicate component (%d)", (int)pEntity->GetType());
        return;
    }
    _List[pEntity->GetType()] = pEntity;
}

void CEntity::Unsuscribe(CComponent *pEntity)
{
    if (!_List.size())
    {
        return;
    }
    if (!_List[pEntity->GetType()])
    {
        g_Log.EventError("Trying to unsuscribe not suscribed component (%d)", (int)pEntity->GetType());
        return;
    }
    _List[pEntity->GetType()];
}

bool CEntity::IsSuscribed(CComponent *pEntity)
{
    if (_List.size() && _List[pEntity->GetType()])
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
    if (!_List.size())
        return;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pEntity = it->second;
        if (pEntity)
        {
            pEntity->r_Write(s);
        }
    }
}

bool CEntity::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    if (!_List.size())
        return false;
    for (std::map<COMP_TYPE, CComponent*>::iterator it = _List.begin(); it != _List.end(); ++it)
    {
        CComponent *pEntity = it->second;
        if (pEntity)
        {
            if (pEntity->r_WriteVal(pszKey, sVal, pSrc))   // Returns true if there is a match.
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
        CComponent *pEntity = it->second;
        if (pEntity)
        {
            if (pEntity->r_LoadVal(s))  // Returns true if there is a match.
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
        CComponent *pEntity = it->second;
        if (pEntity)
        {
            if (pEntity->r_Verb(s, pSrc))   // Returns true if there is a match.
            {
                return true;
            }
        }
    }
    return false;
}
