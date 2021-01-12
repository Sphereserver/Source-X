/**
* @file CScriptTriggerArgs.h
*/

#ifndef _INC_CSCRIPTTRIGGERARGS_H
#define _INC_CSCRIPTTRIGGERARGS_H

#include "CScriptObj.h"
#include "CVarDefMap.h"
#include "CLocalVarsExtra.h"
#include <vector>

class CScriptTriggerArgs : public CScriptObj
{
    // All the args an event will need.
    static lpctstr const sm_szLoadKeys[];

public:
    static const char *m_sClassName;
    int64                   m_iN1;      // "ARGN" or "ARGN1" = a modifying numeric arg to the current trigger.
    int64					m_iN2;      // "ARGN2" = a modifying numeric arg to the current trigger.
    int64					m_iN3;      // "ARGN3" = a modifying numeric arg to the current trigger.

    CScriptObj *			m_pO1;      // "ARGO" or "ARGO1" = object 1
                                        // these can go out of date ! get deleted etc.

    CSString				m_s1;           // ""ARGS" or "ARGS1" = string 1
    CSString				m_s1_buf_vec;   // RAW, used to build argv in runtime

    std::vector<lpctstr>	m_v;

    CVarDefMap 				m_VarsLocal;    // "LOCAL.x" = local variable x
    CLocalFloatVars			m_VarsFloat;    // "FLOAT.x" = float local variable x
    CLocalObjMap			m_VarObjs;      // "REFx" = local object x

public:
    CScriptTriggerArgs();
    explicit CScriptTriggerArgs(lpctstr pszStr);
    explicit CScriptTriggerArgs(CScriptObj* pObj);
    explicit CScriptTriggerArgs(int64 iVal1);
    CScriptTriggerArgs(int64 iVal1, int64 iVal2, int64 iVal3 = 0);
    CScriptTriggerArgs(int64 iVal1, int64 iVal2, CScriptObj* pObj);

    virtual ~CScriptTriggerArgs() = default;

private:
    CScriptTriggerArgs(const CScriptTriggerArgs& copy);
    CScriptTriggerArgs& operator=(const CScriptTriggerArgs& other);

public:
    //Puts the ARGN's into the specified variables
    void GetArgNs(int64* iVar1 = nullptr, int64* iVar2 = nullptr, int64* iVar3 = nullptr);

    void Clear();
    void Init( lpctstr pszStr );
    bool r_Verb( CScript & s, CTextConsole * pSrc ) override;
    bool r_LoadVal( CScript & s ) override;
    bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
    bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
    //bool r_Copy( CTextConsole * pSrc );

    virtual lpctstr GetName() const override
    {
        return "ARG";
    }
};

#endif // _INC_CSCRIPTTRIGGERARGS_H
