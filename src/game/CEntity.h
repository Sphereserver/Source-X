/**
* @file CEntityManager.h
*
*/

#ifndef _INC_ENTITYMANAGER_H
#define _INC_ENTITYMANAGER_H

#include "../common/sphere_library/smap.h"
#include "CComponent.h"

class CEntity
{
    //tsdynamicmap<COMP_TYPE, CComponent*> _List;
    std::map<COMP_TYPE, CComponent*> _List;

public:
    ~CEntity();

    /**
    * @brief Suscribes a CComponent.
    *
    * @param pComponent the CComponent to suscribe.
    */
    void Suscribe(CComponent *pComponent);

    /**
    * @brief Unsuscribes a CComponent.
    *
    * @param pComponent the CComponent to unsuscribe.
    */
    void Unsuscribe(CComponent *pComponent);

    /**
    * @brief Checks if a CComponent is actually suscribed.
    *
    * @param pComponent the CComponent to check.
    * @return true if the pComponent is suscribed.
    */
    bool IsSuscribed(CComponent *pComponent);


    /**
    * @brief Gets a pointer to the given CComponent type.
    *
    * @param type the type of the CComponent to retrieve.
    * @return a pointer to the CComponent, if it is suscribed.
    */
    CComponent *GetComponent(COMP_TYPE type);


    /**
    * @brief Wrapper of base method.
    *
    * Searchs an object that can be used as reference for the whole key
    * (eg: uid.04001.name, will set the game object with uid '04001' as
    * reference, the rest of the code will be executed on it.
    *
    * @param pszKey the key applied on the search.
    * @param pRef a pointer to the object found.
    */
    bool r_GetRef(LPCTSTR & pszKey, CScriptObj * & pRef);

    /**
    * @brief Wrapper of base method.
    *
    * Will save important data on worldsave files.
    *
    * @param s the save file.
    * @param pRef a pointer to the object found.
    */
    void r_Write(CScript & s);
    /**
    * @brief Wrapper of base method.
    *
    * Returns a value to a script '<name>' or ingame '.show/.xshow name'
    *
    * @param pszKey the key to search for (eg: name, ResCold, etc).
    * @param sVal the storage that will return the value.
    * @param pSrc is this requested by someone?.
    *
    * @return true if there was a key to retrieve.
    */
    bool r_WriteVal(LPCTSTR pszKey, CSString & sVal, CTextConsole * pSrc);
    /**
    * @brief Wrapper of base method.
    *
    * Sets a value from script name=xx or ingame '.xname = xx /.set name = xx'
    *
    * @param s the container with the keys and values to set.
    * @return true if there was a key to change.
    */
    bool r_LoadVal(CScript & s);
    /**
    * @brief Wrapper of base method.
    *
    * Executes a command (eg: dupe, bounce...)
    *
    * @param s the container with the keys and values to execute.
    * @return true if there was a key which could be executed.
    */
    bool r_Verb(CScript & s, CTextConsole * pSrc);                          ///< Execute command from script.

};

#endif // _INC_ENTITYMANAGER_H
