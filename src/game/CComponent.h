/**
* @file CEntityComponent.h
*
*/

#ifndef _INC_ENTITYCOMPONENT_H
#define _INC_ENTITYCOMPONENT_H

#include "../common/CScript.h"
#include "../common/CScriptObj.h"
#include "../common/sphere_library/CSString.h"

enum COMP_TYPE
{
    COMP_CHAMPION,
    COMP_SPAWN,
    COMP_MULTI,
    COM_QTY
};

class CObjBase;

class CComponent
{
    COMP_TYPE _iType;
    const CObjBase *_pLink;

protected:
    CComponent(COMP_TYPE type, const CObjBase *pLink);

public:
    COMP_TYPE GetType();
    const CObjBase *GetLink();

    // static LPCTSTR const sm_szLoadKeys[]; // This must be added on child classes 
    virtual bool r_GetRef(LPCTSTR & pszKey, CScriptObj * & pRef) = 0;                   ///< Gets a REF to be used on scripts.
    virtual void r_Write(CScript & s) = 0;                                              ///< Storing data in the worldsave.
    virtual bool r_WriteVal(LPCTSTR pszKey, CSString & sVal, CTextConsole * pSrc) = 0;  ///< Returns some value to the scripts or game commands '.xshow '.
    virtual bool r_LoadVal(CScript & s) = 0;                                            ///< Sets a value from scripts or game commands.
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) = 0;                          ///< Execute command from script.
};

#endif // _INC_ENTITYCOMPONENT_H
