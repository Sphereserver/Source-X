/**
* @file CResourceSortedArrays.h
*
*/

#ifndef _INC_CRESOURCESORTEDARRAYS_H
#define _INC_CRESOURCESORTEDARRAYS_H

#include "../CServerMap.h"

struct CValStr;


struct CSStringSortArray : public CSObjSortArray< tchar*, tchar* >
{
    CSStringSortArray() = default;
    virtual ~CSStringSortArray() = default;

    CSStringSortArray(const CSStringSortArray& copy) = delete;
    CSStringSortArray& operator=(const CSStringSortArray& other) = delete;

    // Sorted array of strings
    int CompareKey( tchar* pszID1, tchar* pszID2, bool fNoSpaces ) const;
    void AddSortString( lpctstr pszText );
};

struct CObjNameSortArray : public CSObjSortArray< CScriptObj*, lpctstr >
{
    static const char *m_sClassName;
    CObjNameSortArray() = default;
    virtual ~CObjNameSortArray() = default;

    CObjNameSortArray(const CObjNameSortArray& copy) = delete;
    CObjNameSortArray& operator=(const CObjNameSortArray& other) = delete;

    // Array of CScriptObj. name sorted.
    int CompareKey( lpctstr pszID, CScriptObj* pObj, bool fNoSpaces ) const;
};

class CSkillKeySortArray : public CSObjSortArray< CValStr*, lpctstr >
{
    CSkillKeySortArray() = default;

    CSkillKeySortArray(const CSkillKeySortArray& copy) = delete;
    CSkillKeySortArray& operator=(const CSkillKeySortArray& other) = delete;

    int CompareKey( lpctstr pszKey, CValStr * pVal, bool fNoSpaces ) const;
};

struct CMultiDefArray : public CSObjSortArray< CSphereMulti*, MULTI_TYPE >
{
    // store the static components of a IT_MULTI
    // Sorted array
    int CompareKey( MULTI_TYPE id, CSphereMulti* pBase, bool fNoSpaces ) const;
};


struct CObjNameSorter
{
    bool operator()(const CScriptObj * s1, const CScriptObj * s2) const;
};
class CObjNameSortVector : public CSSortedVector< CScriptObj*, CObjNameSorter >
{
    static int _compare(const CScriptObj* pObj, lpctstr ptcKey);

public:
    //static const char *m_sClassName;  

    inline size_t find_sorted(lpctstr ptcKey) const { return this->find_predicate(ptcKey, _compare);        }
    inline bool   ContainsKey(lpctstr ptcKey) const { return (SCONT_BADINDEX != this->find_sorted(ptcKey)); }
};

#endif // _INC_CRESOURCESORTEDARRAYS_H
