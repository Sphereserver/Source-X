/**
* @file CResourceDef.h
*
*/

#ifndef _INC_CRESOURCEDEF_H
#define _INC_CRESOURCEDEF_H

#include "../CScriptObj.h"
#include "CResourceID.h"

class CVarDefContNum;


class CResourceDef : public CScriptObj
{
    // Define a generic resource section in the scripts.
    // Now the scripts can be modular. resources can be defined any place.
    // NOTE: This may be loaded fully into memory or just an Index to a file.

private:
    CResourceID m_rid;		// the true resource id. (must be unique for the RES_TYPE)

protected:
    const CVarDefContNum * m_pDefName;	// The name of the resource. (optional)

public:
    static const char *m_sClassName;
    CResourceDef(const CResourceID& rid, lpctstr pszDefName) :
        m_rid(rid), m_pDefName(nullptr)
    {
        SetResourceName(pszDefName);
    }
    CResourceDef(const CResourceID& rid, const CVarDefContNum * pDefName = nullptr) :
        m_rid(rid), m_pDefName(pDefName)
    {
    }
    virtual ~CResourceDef()
    {
        // need a virtual for the dynamic_cast to work.
        // ?? Attempt to remove m_pDefName ?
    }

private:
    CResourceDef(const CResourceDef& copy);
    CResourceDef& operator=(const CResourceDef& other);

public:
    inline const CResourceID& GetResourceID() const noexcept
    {
        return m_rid;
    }
    inline RES_TYPE GetResType() const noexcept
    {
        return m_rid.GetResType();
    }
    inline int GetResPage() const noexcept
    {
        return m_rid.GetResPage();
    }

    inline void CopyDef(const CResourceDef * pLink) noexcept
    {
        m_pDefName = pLink->m_pDefName;
    }

    // Get the name of the resource item. (Used for saving) may be number or name
    lpctstr GetResourceName() const;
    virtual lpctstr GetName() const	// default to same as the DEFNAME name.
    {
        return GetResourceName();
    }

    // Give it another DEFNAME= even if it already has one. it's ok to have multiple names.
    bool SetResourceName( lpctstr pszName );
    void SetResourceVar( const CVarDefContNum* pVarNum ) noexcept
    {
        if (pVarNum != nullptr && m_pDefName == nullptr)
            m_pDefName = pVarNum;
    }

    // Unlink all this data (tho don't delete the def as the pointer might still be used !)
    virtual void UnLink()
    {
        // This does nothing in the CResourceDef case, only in the case of CBaseBaseDef/CItemDef/CCharDef.
    }

    bool HasResourceName();
    //bool MakeResourceName();  // unused
};


#endif // _INC_CRESOURCEDEF_H

