/**
* @file CEntity.h
*
*/

#ifndef _INC_CENTITY_H
#define _INC_CENTITY_H

//#include "../common/sphere_library/smap.h"
#include "CComponent.h"
#include <map>

class CEntity
{
    //tsdynamicmap<COMP_TYPE, CComponent*> _List;
    std::map<COMP_TYPE, CComponent*> _List;

public:
    CEntity();
    ~CEntity();
    /**
    * @brief Calls Delete on contained CComponents.
    *
    * @param fForce.
    */
    void Delete(bool fForce = false);

    /**
    * @brief Removes all contained CComponents.
    */
    void ClearComponents();

    /**
    * @brief Suscribes a CComponent.
    *
    * @param pComponent the CComponent to suscribe.
    */
    void SubscribeComponent(CComponent *pComponent);

    /**
    * @brief Unsuscribes a CComponent. Use this if looping through the components with an iterator!
    *
    * @param it Iterator to the component to unsubscribe.
    * @param fEraseFromMap Should i erase this component from the internal map? Use false if you're going to erase it manually later
    */
    void UnsubscribeComponent(std::map<COMP_TYPE, CComponent*>::iterator& it, bool fEraseFromMap = true);

    /**
    * @brief Unsuscribes a CComponent.
    *
    * @param pComponent the CComponent to unsuscribe.
    */
    void UnsubscribeComponent(CComponent *pComponent);

    /**
    * @brief Checks if a CComponent is actually suscribed.
    *
    * @param pComponent the CComponent to check.
    * @return true if the pComponent is suscribed.
    */
    bool IsComponentSubscribed(CComponent *pComponent) const;

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
    bool r_GetRef(lpctstr & pszKey, CScriptObj * & pRef);

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
    bool r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc);
    /**
    * @brief Wrapper of base method.
    *
    * Sets a value from scripts name=xx or ingame '.xname = xx /.set name = xx'
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

    /**
    * @brief Copies contents of the components from the target
    */
    void Copy(const CEntity *base);

    /**
    * @brief Calls OnTick on all components.
    *
    * This should commonly return CCRET_CONTINUE to allow other CComponents
    * and default behaviour to control another basic aspects, but on some cases
    * CCRET_TRUE can be used to skip the rest of the code or CCRET_FALSE to stop
    * the ticking and remove the object on the main OnTick().
    *
    * @return The return type:
    * -CCRET_TRUE,     // True: code done, stop the loop.
    * -CCRET_FALSE,    // False: code done, stop the loop.
    * -CCRET_CONTINUE  // Continue: just continue the loop.
    */
    CCRET_TYPE OnTick();

};

#endif // _INC_CENTITY_H
