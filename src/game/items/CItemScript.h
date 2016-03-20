
#pragma once
#ifndef CITEMSCRIPT_H
#define CITEMSCRIPT_H

#include "CItemVendable.h"
//#include "graysvr.h" Removed to test.


class CItemScript : public CItemVendable	// A message for a bboard or book text.
{
    // IT_SCRIPT, IT_EQ_SCRIPT
public:
    static const char *m_sClassName;
    static LPCTSTR const sm_szLoadKeys[];
    static LPCTSTR const sm_szVerbKeys[];
public:
    CItemScript( ITEMID_TYPE id, CItemBase * pItemDef );
    virtual ~CItemScript();

private:
    CItemScript(const CItemScript& copy);
    CItemScript& operator=(const CItemScript& other);

public:
    virtual bool r_Verb( CScript & s, CTextConsole * pSrc );	// some command on this object as a target
    virtual void r_Write( CScript & s );
    virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc = NULL );
    virtual bool r_LoadVal( CScript & s );
    virtual void DupeCopy( const CItem * pItem );
};


#endif // CITEMSCRIPT_H
