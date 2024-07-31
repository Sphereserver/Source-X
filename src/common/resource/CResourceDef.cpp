/**
* @file CResourceDef.cpp
*
*/

#include "../../sphere/threads.h"
#include "../CVarDefMap.h"
#include "../CExpression.h"
#include "../CLog.h"
#include "CResourceDef.h"


bool CResourceDef::SetResourceName( lpctstr pszName )
{
    ADDTOCALLSTACK("CResourceDef::SetResourceName");
    ASSERT(pszName);

    // This is the global def for this item.
    for ( size_t i = 0; pszName[i]; ++i )
    {
        if ( i >= EXPRESSION_MAX_KEY_LEN )
        {
            DEBUG_ERR(( "DEFNAME=%s: too long. Aborting registration of the resource\n", pszName ));
            return false;
        }
        if ( ! _ISCSYM(pszName[i]) )
        {
            DEBUG_ERR(( "DEFNAME=%s: bad characters. Aborting registration of the resource\n", pszName ));
            return false;
        }
    }

    const CVarDefCont * pExistingVarKey = g_Exp.m_VarResDefs.GetKey( pszName );
    const dword dwResPrivateUID = GetResourceID().GetPrivateUID();
    const int iResIndex = GetResourceID().GetResIndex();
	CVarDefContNum* pVarKeyNum = nullptr;
    if ( pExistingVarKey )
    {
        const dword dwKeyVal = (dword)pExistingVarKey->GetValNum();
        if ( dwKeyVal == dwResPrivateUID )
        {
            // DEBUG_WARN(("DEFNAME=%s: redefinition (new value same as previous)\n", pszName));
            // It happens tipically for types pre-defined in sphere_defs.scp and other things. We want this behaviour.
            return true;
        }

        const int iKeyIndex = (int)ResGetIndex(dwKeyVal);
        if ( iKeyIndex == iResIndex)
            DEBUG_WARN(( "DEFNAME=%s: redefinition with a strange type mismatch? (0%" PRIx32 "!=0%" PRIx32 ")\n", pszName, dwKeyVal, dwResPrivateUID ));
        else
            DEBUG_WARN(( "DEFNAME=%s: redefinition (0%x!=0%x)\n", pszName, iKeyIndex, iResIndex ));

        pVarKeyNum = g_Exp.m_VarResDefs.SetNum( pszName, dwResPrivateUID );
    }
    else
    {
        pVarKeyNum = g_Exp.m_VarResDefs.SetNumNew( pszName, dwResPrivateUID );
    }

    if ( pVarKeyNum == nullptr )
        return false;

    SetResourceVar( pVarKeyNum );
    return true;
}

lpctstr CResourceDef::GetResourceName() const
{
    ADDTOCALLSTACK("CResourceDef::GetResourceName");
    if (m_pDefName)
    {
        return m_pDefName->GetKey();
    }

    tchar * ptcTemp = Str_GetTemp();
    snprintf(ptcTemp, Str_TempLength(), "0%x", GetResourceID().GetResIndex());
    return ptcTemp;
}

bool CResourceDef::HasResourceName()
{
    ADDTOCALLSTACK("CResourceDef::HasResourceName");
    if ( m_pDefName )
        return true;
    return false;
}

// Unused
/*
bool CResourceDef::MakeResourceName()
{
    ADDTOCALLSTACK("CResourceDef::MakeResourceName");
    if ( m_pDefName )
        return true;
    lpctstr pszName = GetName();

    GETNONWHITESPACE( pszName );
    tchar * pbuf = Str_GetTemp();
    tchar ch;
    tchar * pszDef;

    strcpy(pbuf, "auto_");

    lpctstr ptcKey = nullptr;	// auxiliary, the key of a similar CVarDef, if any found
    pszDef = pbuf + 2;

    for ( ; *pszName; ++pszName )
    {
        ch	= *pszName;
        if ( ch == ' ' || ch == '\t' || ch == '-' )
            ch	= '_';
        else if ( !IsAlnum( ch ) )
            continue;
        // collapse multiple spaces together
        if ( ch == '_' && *(pszDef-1) == '_' )
            continue;
        *pszDef	= ch;
        pszDef++;
    }
    *pszDef	= '_';
    *(++pszDef)	= '\0';


    size_t uiVar = 1;
    size_t uiLen = strlen( pbuf );

    for ( const CVarDefCont *pVarDef : g_Exp.m_VarResDefs )
    {
        // Is this a similar key?
        ptcKey	= pVarDef->GetKey();
        if ( strnicmp( pbuf, ptcKey, uiLen ) != 0 )
            continue;

        // skip underscores
        ptcKey = ptcKey + uiLen;
        while ( *ptcKey	== '_' )
            ++ptcKey;

        // Is this is subsequent key with a number? Get the highest (plus one)
        if ( IsStrNumericDec( ptcKey ) )
        {
            size_t uiVarThis = Str_ToUI( ptcKey );
            if ( uiVarThis >= uiVar )
                uiVar = uiVarThis + 1;
        }
        else
            ++uiVar;
    }

    // add an extra _, hopefully won't conflict with named areas
    sprintf( pszDef, "_%" PRIuSIZE_T, uiVar );
    SetResourceName( pbuf );
    // Assign name
    return true;
}
*/
