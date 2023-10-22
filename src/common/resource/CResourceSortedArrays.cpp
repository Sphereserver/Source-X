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
    UnreferencedParameter(fNoSpaces);
    ASSERT( pszID2 );
    return strcmpi( pszID1, pszID2);
}

void CSStringSortArray::AddSortString( lpctstr pszText )
{
    ASSERT(pszText);
    size_t len = strlen( pszText );
    tchar * pNew = new tchar [ len + 1 ];
    Str_CopyLimitNull( pNew, pszText, len + 1 );
    AddSortKey( pNew, pNew );
}

void CSStringSortArray::DeleteElements()
{
    for (tchar* elem : *this)
        delete[] elem;
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

int CSkillKeySortArray::CompareKey(lpctstr ptcKey, CValStr * pVal, bool fNoSpaces) const
{
    UnreferencedParameter(fNoSpaces);
    ASSERT(ptcKey);
    ASSERT(pVal->m_pszName);
    return strcmpi(ptcKey, pVal->m_pszName);
}

int CMultiDefArray::CompareKey(MULTI_TYPE id, CUOMulti* pBase, bool fNoSpaces) const
{
    UnreferencedParameter(fNoSpaces);
    ASSERT(pBase);
    return (id - pBase->GetMultiID());
}

/*
int CObjUniquePtrNameSortVector::_compare(const CScriptObj* pObj, lpctstr ptcKey) // static
{
    ASSERT( pObj );
    ASSERT( ptcKey );
  
    // We can use Str_CmpHeadI to ignore whitespaces (args to the function or whatever) in ptcKey, but i'm not totally sure if this is faster than the code below
    //return -Str_CmpHeadI(ptcKey, pObj->GetName());

    const lpctstr ptcObjStr = pObj->GetName();
    const char * const p = strchr( ptcKey, ' ' );
    if (p != nullptr)
    {
        const size_t uiLen = p - ptcKey;
        const size_t uiObjStrLen = strlen(ptcObjStr);
        int retval = strnicmp(ptcKey, ptcObjStr, uiLen);
        if (retval == 0)
        {
            if (uiObjStrLen == uiLen)
                return 0;
            else if (uiLen < uiObjStrLen)
                return -1;
            else
                return 1;
        }
        else
        {
            return retval;
        }
    }
    return strcmpi(ptcObjStr, ptcKey);
};
*/
