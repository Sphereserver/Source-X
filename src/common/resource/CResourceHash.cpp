/**
* @file CResourceHash.cpp
*
*/

#include "CResourceDef.h"
#include "CResourceHash.h"


size_t CResourceHash::FindKey(CResourceIDBase rid) const
{
    return m_Array[GetHashArray(rid)].FindKey(rid);
}
CResourceDef* CResourceHash::GetAt(CResourceIDBase rid, size_t index) const
{
    return m_Array[GetHashArray(rid)].at(index);
}
size_t CResourceHash::AddSortKey(CResourceIDBase rid, CResourceDef* pNew)
{
    return m_Array[GetHashArray(rid)].AddSortKey(pNew, rid);
}
void CResourceHash::SetAt(CResourceIDBase rid, size_t index, CResourceDef* pNew)
{
    m_Array[GetHashArray(rid)].assign(index, pNew);
}


int CResourceHashArray::CompareKey( CResourceIDBase rid, CResourceDef * pBase, bool fNoSpaces ) const
{
    UNREFERENCED_PARAMETER(fNoSpaces);
    dword dwID1 = rid.GetPrivateUID();
    ASSERT( pBase );
    dword dwID2 = pBase->GetResourceID().GetPrivateUID();
    if (dwID1 > dwID2 )
        return 1;
    if (dwID1 == dwID2 )
        return 0;
    return -1;
}

