/**
* @file CItemMap.h
*
*/

#pragma once
#ifndef _INC_CITEMMAP_H
#define _INC_CITEMMAP_H

#include "CItemVendable.h"


struct CMapPinRec // Pin on a map
{
	short m_x;
	short m_y;

public:
	CMapPinRec( short x, short y )
		: m_x(x), m_y(y)
	{
	}
};

class CItemMap : public CItemVendable
{
    // IT_MAP
public:
    static const char *m_sClassName;
    enum
    {
        MAX_PINS = 128,
        DEFAULT_SIZE = 200
    };

    bool m_fPlotMode;	// should really be per-client based but oh well.
    CSTypedArray<CMapPinRec,CMapPinRec&> m_Pins;

public:
    CItemMap( ITEMID_TYPE id, CItemBase * pItemDef );
    virtual ~CItemMap();

private:
    CItemMap(const CItemMap& copy);
    CItemMap& operator=(const CItemMap& other);

public:
    virtual bool IsSameType( const CObjBase * pObj ) const;
    virtual void r_Write( CScript & s );
    virtual bool r_WriteVal( lpctstr pszKey, CSString &sVal, CTextConsole * pSrc = NULL );
    virtual bool r_LoadVal( CScript & s );
    virtual void DupeCopy( const CItem * pItem );
};

#endif // _INC_CITEMMAP_H
