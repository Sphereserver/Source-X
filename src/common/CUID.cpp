
#include "../game/chars/CChar.h"
#include "../game/CWorld.h"
#include "CUID.h"


// -----------------------------
//	CUID
// -----------------------------

CObjBase * CUID::ObjFind(dword dwPrivateUID)   // static
{
    if ( IsResource(dwPrivateUID) || !IsValidUID(dwPrivateUID) )
        return nullptr;
    return g_World.FindUID( dwPrivateUID & UID_O_INDEX_MASK );
}

CItem * CUID::ItemFind(dword dwPrivateUID)    // static
{
    // Does item still exist or has it been deleted?
    // IsItem() may be faster ?
    return dynamic_cast<CItem *>(ObjFind(dwPrivateUID));
}
CChar * CUID::CharFind(dword dwPrivateUID)    // static
{
    // Does character still exists?
    return dynamic_cast<CChar *>(ObjFind(dwPrivateUID));
}

CObjBase * CUID::ObjFind() const
{
    return ObjFind(m_dwInternalVal);
}
CItem * CUID::ItemFind() const 
{
    return ItemFind(m_dwInternalVal);
}
CChar * CUID::CharFind() const
{
    return CharFind(m_dwInternalVal);
}


bool CUID::IsValidUID(dword dwPrivateUID) // static
{
	return ( dwPrivateUID && ( dwPrivateUID & UID_O_INDEX_MASK ) != UID_O_INDEX_MASK );
}

bool CUID::IsResource(dword dwPrivateUID) // static
{
    return (dwPrivateUID & UID_F_RESOURCE);
}

bool CUID::IsValidResource(dword dwPrivateUID) // static
{
    return (IsResource(dwPrivateUID) && IsValidUID(dwPrivateUID));
}

bool CUID::IsItem(dword dwPrivateUID) 	// static
{
	if ( (dwPrivateUID & (UID_F_RESOURCE|UID_F_ITEM)) == UID_F_ITEM )    // It's NOT a resource, and it's an item
		return true;	// might be static in client ?
	return false;
}

bool CUID::IsChar(dword dwPrivateUID) // static
{
	if ( ( dwPrivateUID & (UID_F_RESOURCE|UID_F_ITEM)) == 0 )    // It's NOT a resource, and it's not an item
		return IsValidUID(dwPrivateUID);
	return false;
}


bool CUID::IsObjDisconnected() const	// Not in the game world for some reason.
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == UID_O_DISCONNECT )
		return true;
	return false;
}

bool CUID::IsObjTopLevel() const	// on the ground in the world.
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == 0 )
		return true;	// might be static in client ?
	return false;
}

bool CUID::IsItemEquipped() const
{
	if ( (m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_EQUIPPED))
		return IsValidUID();
	return false ;
}

bool CUID::IsItemInContainer() const
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_CONTAINED) )
		return IsValidUID();
	return false;
}

void CUID::SetObjContainerFlags( dword dwFlags )
{
	m_dwInternalVal = ( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM) ) | dwFlags;
}

void CUID::RemoveObjFlags( dword dwFlags )
{
    m_dwInternalVal &= ~dwFlags;
}

dword CUID::GetObjUID() const
{
	return ( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM) );
}

void CUID::SetObjUID( dword dwVal )
{
	// can be set to -1 by the client.
	m_dwInternalVal = ( dwVal & (UID_O_INDEX_MASK|UID_F_ITEM) ) | UID_O_DISCONNECT;
}
