/**
* @file CResourceRef.h
*
*/

#ifndef _INC_CRESOURCEREF_H
#define _INC_CRESOURCEREF_H

#include "CResourceLink.h"
#include "CResourceID.h"
#include <vector>

class CSString;
class CResourceLink;
class CScript;


class CResourceRef
{
private:
    CResourceLink* m_pLink;

public:
    static const char *m_sClassName;

    CResourceRef();
    CResourceRef(CResourceLink* pLink);
    CResourceRef(const CResourceRef& copy);

    ~CResourceRef();

    CResourceRef& operator=(const CResourceRef& other);

public:
    void SetRef(CResourceLink* pLink);
    inline CResourceLink* GetRef() const noexcept
    {
        return m_pLink;
    }
    inline bool operator==(const CResourceRef& comp) const noexcept
    {
        return (GetRef() == comp.GetRef());
    }
};

class CResourceRefArray : public std::vector<CResourceRef>
{
    // Define a list of pointer references to resource. (Not owned by the list)
    // An indexed list of CResourceLink s.

private:
    lpctstr GetResourceName( size_t iIndex ) const;

public:
    static const char *m_sClassName;
    CResourceRefArray() = default;
    size_t FindResourceType( RES_TYPE type ) const;
    size_t FindResourceID( const CResourceID & rid ) const;
    size_t FindResourceName( RES_TYPE restype, lpctstr ptcKey ) const;

    void WriteResourceRefList( CSString & sVal ) const;
    bool r_LoadVal( CScript & s, RES_TYPE restype );
    void r_Write( CScript & s, lpctstr ptcKey ) const;

    inline bool ContainsResourceID( const CResourceID & rid ) const
    {
        return FindResourceID(rid) != sl::scont_bad_index();
    }
    inline bool ContainsResourceName( RES_TYPE restype, lpctstr & ptcKey ) const
    {
        return FindResourceName(restype, ptcKey) != sl::scont_bad_index();
    }
};


#endif // _INC_CRESOURCEREF_H
