/**
* @file CResourceDef.cpp
*
*/

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

    const CVarDefCont * pVarKey = g_Exp.m_VarResDefs.GetKey( pszName );
	CVarDefContNum* pVarKeyNum = nullptr;
    if ( pVarKey )
    {
        dword keyVal = (dword)pVarKey->GetValNum();
        if ( keyVal == GetResourceID().GetPrivateUID() )
        {
            // DEBUG_WARN(("DEFNAME=%s: redefinition (new value same as previous)\n", pszName));
            // It happens tipically for types pre-defined in sphere_defs.scp and other things. Wanted behaviour.
            return true;
        }


        if ( RES_GET_INDEX(keyVal) == (dword)GetResourceID().GetResIndex())
            DEBUG_WARN(( "DEFNAME=%s: redefinition with a strange type mismatch? (0%" PRIx32 "!=0%" PRIx32 ")\n", pszName, keyVal, GetResourceID().GetPrivateUID() ));
        else
            DEBUG_WARN(( "DEFNAME=%s: redefinition (0%" PRIx32 "!=0%" PRIx32 ")\n", pszName, RES_GET_INDEX(keyVal), GetResourceID().GetResIndex() ));

        pVarKeyNum = g_Exp.m_VarResDefs.SetNum( pszName, GetResourceID().GetPrivateUID() );
    }
    else
    {
        pVarKeyNum = g_Exp.m_VarResDefs.SetNumNew( pszName, GetResourceID().GetPrivateUID() );
    }

    if ( pVarKeyNum == nullptr )
        return false;

    SetResourceVar( pVarKeyNum );
    return true;
}

lpctstr CResourceDef::GetResourceName() const
{
    ADDTOCALLSTACK("CResourceDef::GetResourceName");
    if ( m_pDefName )
        return m_pDefName->GetKey();

    TemporaryString tsTemp;
    tchar* pszTemp = static_cast<tchar *>(tsTemp);
    sprintf(pszTemp, "0%x", GetResourceID().GetResIndex());
    return pszTemp;
}

bool CResourceDef::HasResourceName()
{
    ADDTOCALLSTACK("CResourceDef::HasResourceName");
    if ( m_pDefName )
        return true;
    return false;
}

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

    strcpy(pbuf, "a_");

    lpctstr pszKey = nullptr;	// auxiliary, the key of a similar CVarDef, if any found
    pszDef = pbuf + 2;

    for ( ; *pszName; ++pszName )
    {
        ch	= *pszName;
        if ( ch == ' ' || ch == '\t' || ch == '-' )
            ch	= '_';
        else if ( !iswalnum( ch ) )
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
        pszKey	= pVarDef->GetKey();
        if ( strnicmp( pbuf, pszKey, uiLen ) != 0 )
            continue;

        // skip underscores
        pszKey = pszKey + uiLen;
        while ( *pszKey	== '_' )
            pszKey++;

        // Is this is subsequent key with a number? Get the highest (plus one)
        if ( IsStrNumericDec( pszKey ) )
        {
            size_t uiVarThis = Str_ToULL( pszKey );
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

