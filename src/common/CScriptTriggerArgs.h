/**
* @file CScriptTriggerArgs.h
*/

#ifndef _INC_CSCRIPTTRIGGERARGS_H
#define _INC_CSCRIPTTRIGGERARGS_H

#include "CScriptObj.h"

class CScriptTriggerArgs : public CScriptObj
{
    // All the args an event will need.
    static lpctstr const sm_szLoadKeys[];
public:
    static const char *m_sClassName;
    int64                   m_iN1;		// "ARGN" or "ARGN1" = a modifying numeric arg to the current trigger.
    int64					m_iN2;		// "ARGN2" = a modifying numeric arg to the current trigger.
    int64					m_iN3;		// "ARGN3" = a modifying numeric arg to the current trigger.

    CScriptObj *			m_pO1;		// "ARGO" or "ARGO1" = object 1
                                        // these can go out of date ! get deleted etc.

    CSString				m_s1;			// ""ARGS" or "ARGS1" = string 1
    CSString				m_s1_raw;		// RAW, used to build argv in runtime

    std::vector<lpctstr>	m_v;

    CVarDefMap 				m_VarsLocal;	// "LOCAL.x" = local variable x
    CVarFloat				m_VarsFloat;	// "FLOAT.x" = float local variable x
    CLocalObjMap			m_VarObjs;		// "REFx" = local object x

public:

    CScriptTriggerArgs() :
        m_iN1(0),  m_iN2(0), m_iN3(0)
    {
        m_pO1 = NULL;
    }

    explicit CScriptTriggerArgs( lpctstr pszStr );

    explicit CScriptTriggerArgs( CScriptObj * pObj ) :
        m_iN1(0),  m_iN2(0), m_iN3(0), m_pO1(pObj)
    {
    }

    explicit CScriptTriggerArgs( int64 iVal1 ) :
        m_iN1(iVal1),  m_iN2(0), m_iN3(0)
    {
        m_pO1 = NULL;
    }
    CScriptTriggerArgs( int64 iVal1, int64 iVal2, int64 iVal3 = 0 ) :
        m_iN1(iVal1), m_iN2(iVal2), m_iN3(iVal3)
    {
        m_pO1 = NULL;
    }

    CScriptTriggerArgs( int64 iVal1, int64 iVal2, CScriptObj * pObj ) :
        m_iN1(iVal1), m_iN2(iVal2), m_iN3(0), m_pO1(pObj)
    {
    }

    virtual ~CScriptTriggerArgs()
    {
    };

private:
    CScriptTriggerArgs(const CScriptTriggerArgs& copy);
    CScriptTriggerArgs& operator=(const CScriptTriggerArgs& other);

public:
    void getArgNs( int64 *iVar1 = NULL, int64 *iVar2 = NULL, int64 *iVar3 = NULL) //Puts the ARGN's into the specified variables
    {
        if (iVar1)
            *iVar1 = this->m_iN1;

        if (iVar2)
            *iVar2 = this->m_iN2;

        if (iVar3)
            *iVar3 = this->m_iN3;
    }

    void Init( lpctstr pszStr );
    bool r_Verb( CScript & s, CTextConsole * pSrc );
    bool r_LoadVal( CScript & s );
    bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
    bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc );
    bool r_Copy( CTextConsole * pSrc );
    lpctstr GetName() const
    {
        return "ARG";
    }
};

#endif // _INC_CSCRIPTTRIGGERARGS_H
