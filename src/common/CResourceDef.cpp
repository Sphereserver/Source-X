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
    for ( size_t i = 0; pszName[i]; i++ )
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

    int iVarNum;

    CVarDefCont * pVarKey = g_Exp.m_VarDefs.GetKey( pszName );
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

        iVarNum = g_Exp.m_VarDefs.SetNum( pszName, GetResourceID().GetPrivateUID() );
    }
    else
    {
        iVarNum = g_Exp.m_VarDefs.SetNumNew( pszName, GetResourceID().GetPrivateUID() );
    }

    if ( iVarNum < 0 )
        return false;

    SetResourceVar( dynamic_cast <const CVarDefContNum*>( g_Exp.m_VarDefs.GetAt( iVarNum )) );
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


    size_t iMax = g_Exp.m_VarDefs.GetCount();
    int iVar = 1;
    size_t iLen = strlen( pbuf );

    for ( size_t i = 0; i < iMax; i++ )
    {
        // Is this a similar key?
        pszKey	= g_Exp.m_VarDefs.GetAt(i)->GetKey();
        if ( strnicmp( pbuf, pszKey, iLen ) != 0 )
            continue;

        // skip underscores
        pszKey = pszKey + iLen;
        while ( *pszKey	== '_' )
            pszKey++;

        // Is this is subsequent key with a number? Get the highest (plus one)
        if ( IsStrNumericDec( pszKey ) )
        {
            int iVarThis = ATOI( pszKey );
            if ( iVarThis >= iVar )
                iVar = iVarThis + 1;
        }
        else
            iVar++;
    }

    // add an extra _, hopefully won't conflict with named areas
    sprintf( pszDef, "_%i", iVar );
    SetResourceName( pbuf );
    // Assign name
    return true;
}

