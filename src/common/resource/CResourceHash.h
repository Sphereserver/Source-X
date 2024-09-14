/**
* @file CResourceHash.h
*
*/

#ifndef _INC_CRESOURCEHASH_H
#define _INC_CRESOURCEHASH_H

#include "../sphere_library/sptr_containers.h"
#include "CResourceID.h"

class CResourceDef;


struct CResourceHashArraySorter
{
    bool operator()(std::unique_ptr<CResourceDef> const& pObjStored, std::unique_ptr<CResourceDef> const& pObj) const;
};
class CResourceHashArray : public sl::unique_ptr_sorted_vector<CResourceDef, CResourceHashArraySorter>
{
    static int _compare(std::unique_ptr<CResourceDef> const& pObjStored, CResourceID const& rid);

public:
    // This list OWNS the CResourceDef and CResourceLink objects.
    // Sorted array of RESOURCE_ID
    static const char *m_sClassName;
    CResourceHashArray() = default;
    CResourceHashArray(const CResourceHashArray&) = delete;
    CResourceHashArray& operator=(const CResourceHashArray&) = delete;

    inline size_t find_sorted(CResourceID const& rid) const { return this->find_predicate(rid, _compare);        }
    //inline bool   Contains(CResourceID const& rid) const  { return (sl::scont_bad_index() != this->find_sorted(rid)); }
};

struct CResourceHash
{
    static const char *m_sClassName;
    CResourceHashArray m_Array[16];

    CResourceHash() = default;
    CResourceHash(const CResourceHash&) = delete;
    CResourceHash& operator=(const CResourceHash&) = delete;

private:
    inline uint GetHashArray(const CResourceID& rid) const
    {
        return (rid.GetResIndex() & 0x0F);
    }

public:
    inline size_t FindKey(const CResourceID& rid) const
    {
        return m_Array[GetHashArray(rid)].find_sorted(rid);
    }
    inline sl::smart_ptr_view<CResourceDef> GetSmartPtrViewAt(const CResourceID& rid, size_t index) const
    {
        return sl::smart_ptr_view<CResourceDef>(m_Array[GetHashArray(rid)][index]);
    }
    inline CResourceDef* GetBarePtrAt(const CResourceID& rid, size_t index) const
    {
        return m_Array[GetHashArray(rid)][index].get();
    }

    void AddSortKey(CResourceID const& rid, CResourceDef* pNew);
    void SetAt(CResourceID const& rid, size_t index, CResourceDef* pNew);
    //void ReplaceRid(CResourceID const& ridOld, CResourceDef* pNew);
};


#endif // _INC_CRESOURCEHASH_H
