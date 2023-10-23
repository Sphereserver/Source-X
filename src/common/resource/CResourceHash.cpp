/**
* @file CResourceHash.cpp
*
*/

#include "CResourceDef.h"
#include "CResourceHash.h"

bool CResourceHashArraySorter::operator()(std::unique_ptr<CResourceDef> const& pObjStored, std::unique_ptr<CResourceDef> const& pObj) const
{
    // <  : true
    // >= : false
    ASSERT( pObjStored );
    ASSERT( pObj );
    CResourceID const& rid = pObj->GetResourceID();
    //ASSERT(rid.GetResPage != RES_PAGE_ANY); // RES_PAGE_ANY can be used only for search, you can't insert a rid with this special page
    CResourceID const& ridStored = pObjStored->GetResourceID();
    const uint dwID1 = ridStored.GetPrivateUID();
    const uint dwID2 = rid.GetPrivateUID();
    if (dwID1 > dwID2)
        return false;
    if (dwID1 == dwID2)
        return (ridStored.GetResPage() < rid.GetResPage());
    return true;
}

int CResourceHashArray::_compare(std::unique_ptr<CResourceDef> const& pObjStored, CResourceID const& rid) // static
{
    ASSERT( pObjStored );
    CResourceID const& ridStored = pObjStored->GetResourceID();
    const uint dwID1 = ridStored.GetPrivateUID();
    const uint dwID2 = rid.GetPrivateUID();
    if (dwID1 > dwID2)
        return 1;
    if (dwID1 == dwID2)
    {
        const ushort uiPage2 = rid.GetResPage();
        if (uiPage2 == RES_PAGE_ANY)  //rid page == RES_PAGE_ANY: search independently from the page
            return 0;
        const ushort uiPage1 = ridStored.GetResPage();
        if (uiPage1 > uiPage2)
            return 1;
        else if (uiPage1 == uiPage2)
            return 0;
        return -1;
    }
    return -1;
}

void CResourceHash::AddSortKey(CResourceID const& rid, CResourceDef* pNew)
{
    ASSERT(rid.GetResPage() <= RES_PAGE_MAX); // RES_PAGE_ANY can be used only for search, you can't insert a rid with this special page
    m_Array[GetHashArray(rid)].emplace(pNew);
}

void CResourceHash::SetAt(CResourceID const& rid, size_t index, CResourceDef* pNew)
{
    ASSERT(rid.GetResPage() <= RES_PAGE_MAX); // RES_PAGE_ANY can be used only for search, you can't insert a rid with this special page
    m_Array[GetHashArray(rid)][index].reset(pNew);
}

/*
void CResourceHash::ReplaceRid(CResourceID const& ridOld, CResourceDef* pNew)
{
CResourceHashArray & arrOld = m_Array[GetHashArray(ridOld)];
size_t index = arrOld.find_sorted(ridOld);
ASSERT (index != sl::scont_bad_index());
arrOld.erase(arrOld.begin() + index);

m_Array[GetHashArray(pNew->GetResourceID())].emplace(pNew);
}
*/