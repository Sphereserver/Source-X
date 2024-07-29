/**
* @file CComponent.h
*
*/

#ifndef _INC_CCOMPONENT_H
#define _INC_CCOMPONENT_H

#include "../common/common.h"

class CSString;
class CScript;
class CScriptObj;
class CTextConsole;
class CObjBase;


enum COMP_TYPE : ushort
{
    COMP_CHAMPION,
    COMP_SPAWN,
    COMP_MULTI,
    COMP_FACTION,
    COMP_ITEMDAMAGEABLE,
    COMP_QTY
};

enum CCRET_TYPE
{
    CCRET_TRUE,     // True: code done, stop the loop.
    CCRET_FALSE,    // False: code done, stop the loop.
    CCRET_CONTINUE  // Continue: just continue the loop.
};


class CComponent
{
    COMP_TYPE _iType;


protected:
    CComponent(COMP_TYPE type);

public:
    virtual ~CComponent() = default;
    COMP_TYPE GetType() const;

    /* Script's compatibility
    * All methods here are meant to be proccessed from CEntity so they may behave a little different
    * than the methods they are emulating, almost all of them are run through a loop and since attributes
    * or keywords cannot be duplicated (it won't make sense to have MORE1 on more than one CComponent because
    * there will be problems understanding code among other ones) a bool return is placed so each time a
    * CComponent finds a match to the given keyword it returns true and the loop stops and if return is false
    * the iteration continues to the next CComponent and if all fails then CItem/CChar, or whatever CEntity they
    * are being held in, will continue the check on their own.
    */

    virtual void Delete(bool fForce = false) = 0;                                       // Called before destruction to avoid nullptrs
    virtual bool r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef) = 0;                   // Gets a REF to be used on scripts.
    virtual void r_Write(CScript & s) = 0;                                              // Storing data in the worldsave. Must be void, everything must be saved.
    virtual bool r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc) = 0;  // Returns some value to the scripts or game commands '.xshow '.
    virtual bool r_LoadVal(CScript & s) = 0;                                            // Sets a value from scripts or game commands.
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) = 0;                          // Execute command from script.
    virtual void Copy(const CComponent* copy) = 0;                                      // Copy the contents to a new object.
    virtual CCRET_TYPE OnTickComponent() = 0;                                           // Custom implementation of _OnTick.
};

#endif // _INC_CCOMPONENT_H
