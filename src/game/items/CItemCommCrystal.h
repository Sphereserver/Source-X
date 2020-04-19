/**
* @file CItemCommCrystal.h
*
*/

#ifndef _INC_CITEMCOMMCRYSTAL_H
#define _INC_CITEMCOMMCRYSTAL_H

#include "CItemVendable.h"


class CItemCommCrystal : public CItemVendable
{
    // STATF_COMM_CRYSTAL and IT_COMM_CRYSTAL
    // What speech blocks does it like ?
protected:
    static lpctstr const sm_szLoadKeys[];
    CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)
public:
    static const char *m_sClassName;
    CItemCommCrystal( ITEMID_TYPE id, CItemBase * pItemDef );
    virtual ~CItemCommCrystal();

private:
    CItemCommCrystal(const CItemCommCrystal& copy);
    CItemCommCrystal& operator=(const CItemCommCrystal& other);

public:
    virtual void OnMoveFrom() override;
    virtual bool MoveTo(const CPointMap& pt, bool fForceFix = false) override;

    virtual void OnHear( lpctstr pszCmd, CChar * pSrc );
    virtual void  r_Write( CScript & s );
    virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false );
    virtual bool  r_LoadVal( CScript & s  );
    virtual void DupeCopy( const CItem * pItem ) override;  // overriding CItem::DupeCopy
};


#endif // _INC_CITEMCOMMCRYSTAL_H
