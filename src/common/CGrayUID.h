/**
* @file CGrayUID.h
*/

#pragma once
#ifndef _INC_CGRAYUID_H
#define _INC_CGRAYUID_H

#include "common.h"
#include "graycom.h"

class CObjBase;
class CItem;
class CChar;

struct CGrayUIDBase		// A unique system serial id. 4 bytes long
{
	// This is a ref to a game object. It may or may not be valid.
	// The top few bits are just flags.
#define UID_CLEAR			0
#define UID_UNUSED			0xFFFFFFFF	// 0 = not used as well.

#define UID_F_RESOURCE		0x80000000	// ALSO: pileable or special macro flag passed to client.
#define UID_F_ITEM			0x40000000	// CItem as apposed to CChar based

#define UID_O_EQUIPPED		0x20000000	// This item is equipped.
#define UID_O_CONTAINED		0x10000000	// This item is inside another container
#define UID_O_DISCONNECT	0x30000000	// Not attached yet.
#define UID_O_INDEX_MASK	0x0FFFFFFF	// lose the upper bits.
#define UID_O_INDEX_FREE	0x01000000	// Spellbook needs unused UID's ?

protected:
	DWORD m_dwInternalVal;
public:

	bool IsValidUID() const;
	void InitUID();
	void ClearUID();

	bool IsResource() const;
	bool IsItem() const;
	bool IsChar() const;

	bool IsObjDisconnected() const;
	bool IsObjTopLevel() const;

	bool IsItemEquipped() const;
	bool IsItemInContainer() const;

	void SetObjContainerFlags( DWORD dwFlags = 0 );

	void SetPrivateUID( DWORD dwVal );
	DWORD GetPrivateUID() const;

	DWORD GetObjUID() const;
	void SetObjUID( DWORD dwVal );

	bool operator == ( DWORD index ) const;
	bool operator != ( DWORD index ) const;
	operator DWORD () const;
	CObjBase * ObjFind() const;
	CItem * ItemFind() const;
	CChar * CharFind() const;
};

struct CGrayUID : public CGrayUIDBase
{
	CGrayUID();
	CGrayUID( DWORD dw );
};

#endif //_INC_CGRAYUID_H
