
#include "../game/chars/CChar.h"
#include "../game/CWorld.h"
#include "CUID.h"


// -----------------------------
//	CUID
// -----------------------------

CObjBase * CUID::ObjFindFromUID(dword dwPrivateUID, bool fInvalidateBeingDeleted) noexcept    // static
{
    if ( IsResource(dwPrivateUID) || !IsValidUID(dwPrivateUID) )
        return nullptr;

	CObjBase *pObj = g_World.FindUID(dwPrivateUID & UID_O_INDEX_MASK);

	if (fInvalidateBeingDeleted && (!pObj || pObj->IsBeingDeleted()))
		return nullptr;
	return pObj;
}

CItem * CUID::ItemFindFromUID(dword dwPrivateUID, bool fInvalidateBeingDeleted) noexcept     // static
{
    // Does item still exist or has it been deleted?
    // IsItem() may be faster ?
    return dynamic_cast<CItem *>(ObjFindFromUID(dwPrivateUID, fInvalidateBeingDeleted));
}
CChar * CUID::CharFindFromUID(dword dwPrivateUID, bool fInvalidateBeingDeleted) noexcept    // static
{
    // Does character still exists?
    return dynamic_cast<CChar *>(ObjFindFromUID(dwPrivateUID, fInvalidateBeingDeleted));
}


bool CUID::IsValidUID(dword dwPrivateUID) noexcept // static
{
	return ( dwPrivateUID && ( dwPrivateUID & UID_O_INDEX_MASK ) != UID_O_INDEX_MASK );
}

bool CUID::IsResource(dword dwPrivateUID) noexcept  // static
{
    return (dwPrivateUID & UID_F_RESOURCE);
}

bool CUID::IsValidResource(dword dwPrivateUID) noexcept  // static
{
    return (IsResource(dwPrivateUID) && IsValidUID(dwPrivateUID));
}

bool CUID::IsItem(dword dwPrivateUID) noexcept 	// static
{
	if ( (dwPrivateUID & (UID_F_RESOURCE|UID_F_ITEM)) == UID_F_ITEM )    // It's NOT a resource, and it's an item
		return true;	// might be static in client ?
	return false;
}

bool CUID::IsChar(dword dwPrivateUID) noexcept // static
{
	if ( ( dwPrivateUID & (UID_F_RESOURCE|UID_F_ITEM)) == 0 )    // It's NOT a resource, and it's not an item
		return IsValidUID(dwPrivateUID);
	return false;
}


bool CUID::IsObjDisconnected() const noexcept 	// Not in the game world for some reason.
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == UID_O_DISCONNECT )
		return true;
	return false;
}

bool CUID::IsObjTopLevel() const noexcept 	// on the ground in the world.
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == 0 )
		return true;	// might be static in client ?
	return false;
}

bool CUID::IsItemEquipped() const noexcept
{
	if ( (m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_EQUIPPED))
		return IsValidUID();
	return false ;
}

bool CUID::IsItemInContainer() const noexcept
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_CONTAINED) )
		return IsValidUID();
	return false;
}

void CUID::SetObjContainerFlags( dword dwFlags ) noexcept
{
	m_dwInternalVal = ( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM) ) | dwFlags;
}

void CUID::RemoveObjFlags( dword dwFlags ) noexcept
{
    m_dwInternalVal &= ~dwFlags;
}

dword CUID::GetObjUID() const noexcept
{
	return ( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM) );
}

void CUID::SetObjUID( dword dwVal ) noexcept
{
	// can be set to -1 by the client.
	m_dwInternalVal = ( dwVal & (UID_O_INDEX_MASK|UID_F_ITEM) ) | UID_O_DISCONNECT;
}
