/**
* @file CResourceQty.h
*
*/

#ifndef _INC_CRESOURCEQTY_H
#define _INC_CRESOURCEQTY_H

#include "../sphere_library/CSTypedArray.h"
#include "CResourceID.h"


struct CResourceQty
{
private:
    CResourceID m_rid;		// A RES_SKILL, RES_ITEMDEF, or RES_TYPEDEF
    int64 m_iQty;			// How much of this ?
public:
    inline CResourceID GetResourceID() const
    {
        return m_rid;
    }
    void SetResourceID(CResourceID rid, int iQty)
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
    inline void SetResQty(int64 wQty)
    {
        m_iQty = wQty;
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

class CResourceQtyArray : public CSTypedArray<CResourceQty>
{
    // Define a list of index id's (not references) to resource objects. (Not owned by the list)
public:
    static const char *m_sClassName;
    CResourceQtyArray();
    explicit CResourceQtyArray(lpctstr pszCmds);
    bool operator == ( const CResourceQtyArray & array ) const;
    //CResourceQtyArray& operator=(const CResourceQtyArray& other);

private:
    CResourceQtyArray(const CResourceQtyArray& copy);

public:
    size_t Load( lpctstr pszCmds );
    void WriteKeys( tchar * pszArgs, size_t index = 0, bool fQtyOnly = false, bool fKeyOnly = false ) const;
    void WriteNames( tchar * pszArgs, size_t index = 0 ) const;

    size_t FindResourceID( CResourceIDBase rid ) const;
    size_t FindResourceType( RES_TYPE type ) const;
    size_t FindResourceMatch( CObjBase * pObj ) const;
    bool IsResourceMatchAll( CChar * pChar ) const;

    inline bool ContainsResourceID( CResourceIDBase & rid ) const
    {
        return FindResourceID(rid) != BadIndex();
    }
    inline bool ContainsResourceMatch( CObjBase * pObj ) const
    {
        return FindResourceMatch(pObj) != BadIndex();
    }

    void setNoMergeOnLoad();

private:
    bool m_mergeOnLoad;
};


#endif
