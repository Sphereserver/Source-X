/**
* @file CResourceSortedArrays.h
*
*/

#ifndef _INC_CRESOURCESORTEDARRAYS_H
#define _INC_CRESOURCESORTEDARRAYS_H

#include "../../common/sphere_library/sptr_containers.h"
#include "../CServerMap.h"

struct CValStr;


struct CSStringSortArray final : public CSObjSortArray< tchar*, tchar* >
{
    CSStringSortArray() noexcept {
        _fBaseDestructorShouldDeleteElements = false;
    }
    virtual ~CSStringSortArray() noexcept;

    void clear() noexcept = delete;
    void Clear() {
        this->CSStringSortArray::DeleteElements();
        this->std::vector<tchar*>::clear();
    }

    CSStringSortArray(const CSStringSortArray& copy) = delete;
    CSStringSortArray& operator=(const CSStringSortArray& other) = delete;

    // Sorted array of strings
    virtual int CompareKey( tchar* pszID1, tchar* pszID2, bool fNoSpaces ) const override;
    void AddSortString( lpctstr pszText );

protected:
    virtual void DeleteElements() noexcept override;
};

struct CObjNameSortArray final : public CSObjSortArray< CScriptObj*, lpctstr >
{
    static const char *m_sClassName;
    CObjNameSortArray() = default;
    virtual ~CObjNameSortArray() = default;

    CObjNameSortArray(const CObjNameSortArray& copy) = delete;
    CObjNameSortArray& operator=(const CObjNameSortArray& other) = delete;

    // Array of CScriptObj. name sorted.
    int CompareKey( lpctstr pszID, CScriptObj* pObj, bool fNoSpaces ) const;
};

class CSkillKeySortArray final : public CSObjSortArray< CValStr*, lpctstr >
{
    CSkillKeySortArray() = default;

    CSkillKeySortArray(const CSkillKeySortArray& copy) = delete;
    CSkillKeySortArray& operator=(const CSkillKeySortArray& other) = delete;

    int CompareKey( lpctstr ptcKey, CValStr * pVal, bool fNoSpaces ) const;
};

struct CMultiDefArray final : public CSObjSortArray< CUOMulti*, MULTI_TYPE >
{
    // store the static components of a IT_MULTI
    // Sorted array
    int CompareKey( MULTI_TYPE id, CUOMulti* pBase, bool fNoSpaces ) const;
};


template <typename _ObjType>
struct CObjUniquePtrNameVectorSorter
{
    inline bool operator()(std::unique_ptr<_ObjType> const& s1, std::unique_ptr<_ObjType> const& s2) const noexcept
    {
        //return (Str_CmpHeadI(s1->GetName(), s2->GetName()) < 0); // not needed, since the compared strings don't contain whitespaces (arguments or whatsoever)
        // Current strcmpi implementation internally converts to lowerCASE the strings, so this will work until Str_CmpHeadI checks with tolower, instead of toupper
        return (strcmpi(s1->GetName(), s2->GetName()) < 0);
    }
    inline static int _compare(std::unique_ptr<_ObjType> const& pObj, lpctstr ptcKey)
    {
        return -Str_CmpHeadI(ptcKey, pObj->GetName());  // We use Str_CmpHeadI to ignore '_' and whitespaces (args to the function or whatever) in ptcKey
    }
};
template <typename _ObjType>
class CObjUniquePtrNameSortVector final : public sl::unique_ptr_sorted_vector<_ObjType, CObjUniquePtrNameVectorSorter<_ObjType>>
{
public:
    //static const char *m_sClassName;  
    inline size_t find_sorted(lpctstr ptcKey) const noexcept { return this->find_predicate(ptcKey, CObjUniquePtrNameVectorSorter<_ObjType>::_compare);        }
    inline bool   ContainsKey(lpctstr ptcKey) const noexcept { return (sl::scont_bad_index() != this->find_sorted(ptcKey)); }
};

#endif // _INC_CRESOURCESORTEDARRAYS_H
