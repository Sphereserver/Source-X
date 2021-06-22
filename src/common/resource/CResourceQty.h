/**
* @file CResourceQty.h
*
*/

#ifndef _INC_CRESOURCEQTY_H
#define _INC_CRESOURCEQTY_H

#include "../sphere_library/CSTypedArray.h" // Needed for constants
#include "CResourceID.h"


struct CResourceQty
{
private:
    CResourceID m_rid;		// A RES_SKILL, RES_ITEMDEF, or RES_TYPEDEF
    int64 m_iQty;			// How much of this ?
public:
    inline const CResourceID& GetResourceID() const
    {
        return m_rid;
    }
    void SetResourceID(const CResourceID& rid, int iQty)
    {
        m_rid = rid;
        m_iQty = iQty;
    }
    inline RES_TYPE GetResType() const
    {
        return m_rid.GetResType();
    }
    inline int GetResIndex() const
    {
        return m_rid.GetResIndex();
    }
    inline int64 GetResQty() const
    {
        return m_iQty;
    }
    inline void SetResQty(int64 iQuantity)
    {
        m_iQty = iQuantity;
    }
    inline bool Load( lptstr & arg )
    {
        return Load( const_cast<lpctstr&>(arg) );
    }
    bool Load( lpctstr & pszCmds );
    size_t WriteKey( tchar * pszArgs, bool fQtyOnly = false, bool fKeyOnly = false ) const;
    size_t WriteNameSingle( tchar * pszArgs, int iQty = 0 ) const;

public:
    CResourceQty() : m_iQty(0) { };
};

class CResourceQtyArray : public std::vector<CResourceQty>
{
    // Define a list of index id's (not references) to resource objects. (Not owned by the list)
public:
    static const char *m_sClassName;
    CResourceQtyArray();
    explicit CResourceQtyArray(lpctstr pszCmds);
    bool operator == ( const CResourceQtyArray & array ) const;

    CResourceQtyArray& operator=(const CResourceQtyArray& other) = default;
    CResourceQtyArray(const CResourceQtyArray& copy) = default;

public:
    size_t Load( lpctstr pszCmds );
    void WriteKeys( tchar * pszArgs, size_t index = 0, bool fQtyOnly = false, bool fKeyOnly = false ) const;
    void WriteNames( tchar * pszArgs, size_t index = 0 ) const;

    size_t FindResourceID( const CResourceID& rid ) const;
    size_t FindResourceType( RES_TYPE type ) const;
    size_t FindResourceMatch( const CObjBase * pObj ) const;
    bool IsResourceMatchAll( const CChar * pChar ) const;

    inline bool ContainsResourceID( const CResourceID & rid ) const
    {
        return FindResourceID(rid) != SCONT_BADINDEX;
    }
    inline bool ContainsResourceType( RES_TYPE type ) const
    {
        return FindResourceType(type) != SCONT_BADINDEX;
    }
    inline bool ContainsResourceMatch( CObjBase * pObj ) const
    {
        return FindResourceMatch(pObj) != SCONT_BADINDEX;
    }

    void setNoMergeOnLoad();

private:
    bool m_mergeOnLoad;
};


#endif
