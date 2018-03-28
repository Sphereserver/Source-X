
#include "CUID.h"

// -----------------------------
//	CUIDBase
// -----------------------------

bool CUIDBase::IsValidUID() const
{
	return ( m_dwInternalVal && ( m_dwInternalVal & UID_O_INDEX_MASK ) != UID_O_INDEX_MASK );
}

bool CUIDBase::IsResource() const
{
	if ( m_dwInternalVal & UID_F_RESOURCE )
		return IsValidUID();
	return false ;
}

bool CUIDBase::IsItem() const	// Item vs. Char
{
	if ( (m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM)) == UID_F_ITEM )
		return true;	// might be static in client ?
	return false;
}

bool CUIDBase::IsChar() const	// Item vs. Char
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM)) == 0 )
		return IsValidUID();
	return false;
}

bool CUIDBase::IsObjDisconnected() const	// Not in the game world for some reason.
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == UID_O_DISCONNECT )
		return true;
	return false;
}

bool CUIDBase::IsObjTopLevel() const	// on the ground in the world.
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == 0 )
		return true;	// might be static in client ?
	return false;
}

bool CUIDBase::IsItemEquipped() const
{
	if ( (m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_EQUIPPED))
		return IsValidUID();
	return false ;
}

bool CUIDBase::IsItemInContainer() const
{
	if ( ( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_CONTAINED) )
		return IsValidUID();
	return false;
}

void CUIDBase::SetObjContainerFlags( dword dwFlags )
{
	m_dwInternalVal = ( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM) ) | dwFlags;
}

dword CUIDBase::GetObjUID() const
{
	return ( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM) );
}

void CUIDBase::SetObjUID( dword dwVal )
{
	// can be set to -1 by the client.
	m_dwInternalVal = ( dwVal & (UID_O_INDEX_MASK|UID_F_ITEM) ) | UID_O_DISCONNECT;
}



