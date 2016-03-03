/**
* @file CQueue.h
*/

#ifndef _INC_CGRAYUID_H
#define _INC_CGRAYUID_H
#include "graycom.h"

class CObjBase;
class CChar;
class CItem;

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

	bool IsValidUID() const
	{
		return( m_dwInternalVal && ( m_dwInternalVal & UID_O_INDEX_MASK ) != UID_O_INDEX_MASK );
	}
	void InitUID()
	{
		m_dwInternalVal = UID_UNUSED;
	}
	void ClearUID()
	{
		m_dwInternalVal = UID_CLEAR;
	}

	bool IsResource() const
	{
		if ( m_dwInternalVal & UID_F_RESOURCE )
			return( IsValidUID() );
		return( false );
	}
	bool IsItem() const	// Item vs. Char
	{
		if (( m_dwInternalVal & (UID_F_ITEM|UID_F_RESOURCE)) == UID_F_ITEM )
			return( true );	// might be static in client ?
		return( false );
	}
	bool IsChar() const	// Item vs. Char
	{
		if (( m_dwInternalVal & (UID_F_ITEM|UID_F_RESOURCE)) == 0 )
			return( IsValidUID());
		return( false );
	}

	bool IsObjDisconnected() const	// Not in the game world for some reason.
	{
		if (( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == UID_O_DISCONNECT )
			return( true );
		return( false );
	}
	bool IsObjTopLevel() const	// on the ground in the world.
	{
		if (( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == 0 )
			return( true );	// might be static in client ?
		return( false );
	}

	bool IsItemEquipped() const
	{
		if (( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_EQUIPPED))
			return( IsValidUID() );
		return( false );
	}
	bool IsItemInContainer() const
	{
		if (( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_CONTAINED))
			return( IsValidUID() );
		return( false );
	}

	void SetObjContainerFlags( DWORD dwFlags = 0 )
	{
		m_dwInternalVal = ( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM )) | dwFlags;
	}

	void SetPrivateUID( DWORD dwVal )
	{
		m_dwInternalVal = dwVal;
	}
	DWORD GetPrivateUID() const
	{
		return m_dwInternalVal;
	}

	DWORD GetObjUID() const
	{
		return( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM) );
	}
	void SetObjUID( DWORD dwVal )
	{
		// can be set to -1 by the client.
		m_dwInternalVal = ( dwVal & (UID_O_INDEX_MASK|UID_F_ITEM)) | UID_O_DISCONNECT;
	}

	bool operator == ( DWORD index ) const
	{
		return( GetObjUID() == index );
	}
	bool operator != ( DWORD index ) const
	{
		return( GetObjUID() != index );
	}
	operator DWORD () const
	{
		return( GetObjUID());
	}

	CObjBase * ObjFind() const;
	CItem * ItemFind() const; // Does item still exist or has it been deleted
	CChar * CharFind() const; // Does character still exist
};

struct CGrayUID : public CGrayUIDBase
{
	CGrayUID()
	{
		InitUID();
	}
	CGrayUID( DWORD dw )
	{
		SetPrivateUID( dw );
	}
};

#endif
