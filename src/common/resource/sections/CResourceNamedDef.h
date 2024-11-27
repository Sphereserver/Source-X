#ifndef _INC_CRESOURCENAMEDDEF_H
#define _INC_CRESOURCENAMEDDEF_H


#include "../../sphere_library/CSString.h"
#include "../CResourceLink.h"

class CResourceNamedDef : public CResourceLink
{
    // Private name pool. (does not use DEFNAME) RES_FUNCTION
public:
    static const char* m_sClassName;
private:
    const CSString m_sName;

public:
    CResourceNamedDef(const CResourceID& rid, lpctstr pszName);
    virtual ~CResourceNamedDef() = default;

    CResourceNamedDef(const CResourceNamedDef& copy) = delete;
    CResourceNamedDef& operator=(const CResourceNamedDef& other) = delete;

public:
    virtual lpctstr GetName() const override;

};


#endif //_INC_CRESOURCENAMEDDEF_H
