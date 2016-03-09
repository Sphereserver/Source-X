#include "CGrayUID.h"

bool CGrayUIDBase::IsValidUID() const
{
	return( m_dwInternalVal && ( m_dwInternalVal & UID_O_INDEX_MASK ) != UID_O_INDEX_MASK );
}

void CGrayUIDBase::InitUID()
{
	m_dwInternalVal = UID_UNUSED;
}

void CGrayUIDBase::ClearUID()
{
	m_dwInternalVal = UID_CLEAR;
}

bool CGrayUIDBase::IsResource() const
{
	if ( m_dwInternalVal & UID_F_RESOURCE )
		return( IsValidUID() );
	return( false );
}

bool CGrayUIDBase::IsItem() const	// Item vs. Char
{
	if (( m_dwInternalVal & (UID_F_ITEM|UID_F_RESOURCE)) == UID_F_ITEM )
		return( true );	// might be static in client ?
	return( false );
}

bool CGrayUIDBase::IsChar() const	// Item vs. Char
{
	if (( m_dwInternalVal & (UID_F_ITEM|UID_F_RESOURCE)) == 0 )
		return( IsValidUID());
	return( false );
}

bool CGrayUIDBase::IsObjDisconnected() const	// Not in the game world for some reason.
{
	if (( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == UID_O_DISCONNECT )
		return( true );
	return( false );
}

bool CGrayUIDBase::IsObjTopLevel() const	// on the ground in the world.
{
	if (( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == 0 )
		return( true );	// might be static in client ?
	return( false );
}

bool CGrayUIDBase::IsItemEquipped() const
{
	if (( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_EQUIPPED))
		return( IsValidUID() );
	return( false );
}

bool CGrayUIDBase::IsItemInContainer() const
{
	if (( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_CONTAINED))
		return( IsValidUID() );
	return( false );
}

void CGrayUIDBase::SetObjContainerFlags( DWORD dwFlags )
{
	m_dwInternalVal = ( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM )) | dwFlags;
}

void CGrayUIDBase::SetPrivateUID( DWORD dwVal )
{
	m_dwInternalVal = dwVal;
}

DWORD CGrayUIDBase::GetPrivateUID() const
{
	return m_dwInternalVal;
}

DWORD CGrayUIDBase::GetObjUID() const
{
	return( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM) );
}

void CGrayUIDBase::SetObjUID( DWORD dwVal )
{
	// can be set to -1 by the client.
	m_dwInternalVal = ( dwVal & (UID_O_INDEX_MASK|UID_F_ITEM)) | UID_O_DISCONNECT;
}

bool CGrayUIDBase::operator == ( DWORD index ) const
{
	return( GetObjUID() == index );
}

bool CGrayUIDBase::operator != ( DWORD index ) const
{
	return( GetObjUID() != index );
}

CGrayUIDBase::operator DWORD () const
{
	return( GetObjUID());
}

CGrayUID::CGrayUID()
{
	InitUID();
}

CGrayUID::CGrayUID( DWORD dw )
{
	SetPrivateUID( dw );
}

