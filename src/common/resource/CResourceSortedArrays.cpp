/**
* @file CResourceSortedArrays.cpp
*
*/

#include "../sphere_library/CSAssoc.h"
#include "../sphere_library/sstring.h"
#include "../CScriptObj.h"
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

    const lpctstr objStr = pObj->GetName();
    if ( fNoSpaces )
    {
        const char * const p = strchr( pszID, ' ' );
        if (p != nullptr)
        {
            return strnicmp( pszID, objStr, (p - pszID) );

			/*
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
			*/
		}
    }
    return strcmpi( pszID, objStr);
}

int CSkillKeySortArray::CompareKey(lpctstr pszKey, CValStr * pVal, bool fNoSpaces) const
{
    UNREFERENCED_PARAMETER(fNoSpaces);
    ASSERT(pszKey);
    ASSERT(pVal->m_pszName);
    return strcmpi(pszKey, pVal->m_pszName);
}

int CMultiDefArray::CompareKey(MULTI_TYPE id, CSphereMulti* pBase, bool fNoSpaces) const
{
    UNREFERENCED_PARAMETER(fNoSpaces);
    ASSERT(pBase);
    return (id - pBase->GetMultiID());
}


bool CObjNameSorter::operator()(const CScriptObj * s1, const CScriptObj * s2) const
{
    ASSERT( s1 );
    ASSERT( s2 );

    /*
    const lpctstr ptc1 = s1->GetName();
    const lpctstr ptc2 = s2->GetName();
    const char * const p = strchr( ptc1, ' ' );
    if (p != nullptr)
    {
        const size_t iLen = p - ptc1;
        return (strnicmp(ptc1, ptc2, iLen) < 0);
    }
    return (strcmpi(ptc1, ptc2) < 0);
    */
    //return (Str_CmpHeadI(s1->GetName(), s2->GetName()) < 0); // not needed, since the compared strings don't contain whitespaces (arguments or whatsoever)
    return (strcmpi(s1->GetName(), s2->GetName()) < 0);
};

int CObjNameSortVector::_compare(const CScriptObj* pObj, lpctstr ptcKey) // static
{
    ASSERT( pObj );
    ASSERT( ptcKey );
  
    /*
    const lpctstr ptcObjStr = pObj->GetName();
    const char * const p = strchr( ptcKey, ' ' );
    if (p != nullptr)
    {
        const size_t iLen = p - ptcKey;
        return strnicmp(ptcObjStr, ptcKey, iLen);
    }
    return strcmpi(ptcObjStr, ptcKey);
    */
    return -Str_CmpHeadI(ptcKey, pObj->GetName());  // use Str_CmpHeadI to ignore whitespaces (args to the function or whatever) in ptcKey
};
