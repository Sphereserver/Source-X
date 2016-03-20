
#pragma once
#ifndef CITEMCOMMCRYSTAL_H
#define CITEMCOMMCRYSTAL_H

#include "CItemVendable.h"
//#include "graysvr.h" Removed to test.


class CItemCommCrystal : public CItemVendable
{
    // STATF_COMM_CRYSTAL and IT_COMM_CRYSTAL
    // What speech blocks does it like ?
protected:
    static LPCTSTR const sm_szLoadKeys[];
    CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)
public:
    static const char *m_sClassName;
    CItemCommCrystal( ITEMID_TYPE id, CItemBase * pItemDef );
    virtual ~CItemCommCrystal();

private:
    CItemCommCrystal(const CItemCommCrystal& copy);
    CItemCommCrystal& operator=(const CItemCommCrystal& other);

public:
    virtual void OnMoveFrom();
    virtual bool MoveTo(CPointMap pt, bool bForceFix = false);

    virtual void OnHear( LPCTSTR pszCmd, CChar * pSrc );
    virtual void  r_Write( CScript & s );
    virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
    virtual bool  r_LoadVal( CScript & s  );
    virtual void DupeCopy( const CItem * pItem );
};


#endif // CITEMCOMMCRYSTAL_H
