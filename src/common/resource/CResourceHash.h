/**
* @file CResourceHash.h
*
*/

#ifndef _INC_CRESOURCEHASH_H
#define _INC_CRESOURCEHASH_H

#include "../sphere_library/CSObjSortArray.h"
#include "CResourceID.h"

class CResourceDef;


struct CResourceHashArraySorter
{
    bool operator()(const CResourceDef* pObjStored, const CResourceDef* pObj) const;
};
class CResourceHashArray : public CSSortedVector< CResourceDef*, CResourceHashArraySorter >
{
    static int _compare(const CResourceDef* pObjStored, CResourceID const& rid);

public:
    // This list OWNS the CResourceDef and CResourceLink objects.
    // Sorted array of RESOURCE_ID
    static const char *m_sClassName;
    CResourceHashArray() = default;
    CResourceHashArray(const CResourceHashArray&) = delete;
    CResourceHashArray& operator=(const CResourceHashArray&) = delete;

    inline size_t find_sorted(CResourceID const& rid) const { return this->find_predicate(rid, _compare);        }
    //inline bool   Contains(CResourceID const& rid) const  { return (SCONT_BADINDEX != this->find_sorted(rid)); }
};

struct CResourceHash
{
    static const char *m_sClassName;
    CResourceHashArray m_Array[16];

    CResourceHash() = default;
    CResourceHash(const CResourceHash&) = delete;
    CResourceHash& operator=(const CResourceHash&) = delete;

private:
    inline int GetHashArray(const CResourceID& rid) const
    {
        return (rid.GetResIndex() & 0x0F);
    }

public:
    inline size_t FindKey(const CResourceID& rid) const
    {
        return m_Array[GetHashArray(rid)].find_sorted(rid);
    }
    inline CResourceDef* GetAt(const CResourceID& rid, size_t index) const
    {
        return m_Array[GetHashArray(rid)][index];
    }

    void AddSortKey(CResourceID const& rid, CResourceDef* pNew);
    void SetAt(CResourceID const& rid, size_t index, CResourceDef* pNew);
    //void ReplaceRid(CResourceID const& ridOld, CResourceDef* pNew);
};


#endif // _INC_CRESOURCEHASH_H
