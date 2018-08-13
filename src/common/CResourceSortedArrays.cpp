/**
* @file CResourceSortedArrays.cpp
*
*/

#include "CScriptObj.h"
#include "CResourceSortedArrays.h"


// Sorted array of strings
int CSStringSortArray::CompareKey( tchar* pszID1, tchar* pszID2, bool fNoSpaces ) const
{
    UNREFERENCED_PARAMETER(fNoSpaces);
    ASSERT( pszID2 );
    return strcmpi( pszID1, pszID2);
}

void CSStringSortArray::AddSortString( lpctstr pszText )
{
    ASSERT(pszText);
    size_t len = strlen( pszText );
    tchar * pNew = new tchar [ len + 1 ];
    strcpy( pNew, pszText );
    AddSortKey( pNew, pNew );
}

// Array of CScriptObj. name sorted.
int CObjNameSortArray::CompareKey( lpctstr pszID, CScriptObj* pObj, bool fNoSpaces ) const
{
    ASSERT( pszID );
    ASSERT( pObj );

    lpctstr objStr = pObj->GetName();
    if ( fNoSpaces )
    {
        const char * p = strchr( pszID, ' ' );
        if (p != nullptr)
        {
            size_t iLen = p - pszID;
            // return( strnicmp( pszID, pObj->GetName(), iLen ) );

            size_t objStrLen = strlen( objStr );
            int retval = strnicmp( pszID, objStr, iLen );
            if ( retval == 0 )
            {
                if (objStrLen == iLen )
                    return 0;
                else if ( iLen < objStrLen )
                    return -1;
                else
                    return 1;
            }
            else
            {
                return retval;
            }
        }
    }
    return strcmpi( pszID, objStr);
}
