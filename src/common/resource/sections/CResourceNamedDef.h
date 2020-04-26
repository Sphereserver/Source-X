#include "../../sphere_library/CSString.h"
#include "../CResourceLink.h"

class CResourceNamedDef : public CResourceLink
{
    // Private name pool. (does not use DEFNAME) RES_FUNCTION
public:
    static const char *m_sClassName;
    const CSString m_sName;
public:
    CResourceNamedDef(CResourceID rid, lpctstr pszName) : CResourceLink(rid), m_sName(pszName)
    {
    }
    virtual ~CResourceNamedDef()
    {
    }

private:
    CResourceNamedDef(const CResourceNamedDef& copy);
    CResourceNamedDef& operator=(const CResourceNamedDef& other);

public:
    lpctstr GetName() const
    {
        return m_sName;
    }
};

