#include "CResourceNamedDef.h"

CResourceNamedDef::CResourceNamedDef(const CResourceID& rid, lpctstr pszName) :
    CResourceLink(rid), m_sName(pszName)
{
}

lpctstr CResourceNamedDef::GetName() const
{
    return m_sName.GetBuffer();
}
