/**
* @file CResourceSortedArrays.h
*
*/

#ifndef _INC_CRESOURCESORTEDARRAYS_H
#define _INC_CRESOURCESORTEDARRAYS_H

#include "../sphere_library/CSObjSortArray.h"

class CSCriptObj;


struct CSStringSortArray : public CSObjSortArray< tchar*, tchar* >
{
public:
    CSStringSortArray() { }
    virtual ~CSStringSortArray() { }
private:
    CSStringSortArray(const CSStringSortArray& copy);
    CSStringSortArray& operator=(const CSStringSortArray& other);
public:
    // Sorted array of strings
    int CompareKey( tchar* pszID1, tchar* pszID2, bool fNoSpaces ) const;
    void AddSortString( lpctstr pszText );
};

class CObjNameSortArray : public CSObjSortArray< CScriptObj*, lpctstr >
{
public:
    static const char *m_sClassName;
    CObjNameSortArray() {}
    virtual ~CObjNameSortArray() { }
private:
    CObjNameSortArray(const CObjNameSortArray& copy);
    CObjNameSortArray& operator=(const CObjNameSortArray& other);

public:
    // Array of CScriptObj. name sorted.
    int CompareKey( lpctstr pszID, CScriptObj* pObj, bool fNoSpaces ) const;
};


#endif // _INC_CRESOURCESORTEDARRAYS_H
