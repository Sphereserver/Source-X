#ifndef _INC_CGRAYUIDEXTRA_H
#define _INC_CGRAYUIDEXTRA_H

#include "CGrayUID.h"
#include "../game/CWorld.h"
#include "../game/Chars/CChar.h"
#include "../game/CObjBase.h"

inline CObjBase * CGrayUIDBase::ObjFind() const
{
	if ( IsResource() )
		return( NULL );
	return( g_World.FindUID( m_dwInternalVal & UID_O_INDEX_MASK ) );
}

inline CItem * CGrayUIDBase::ItemFind() const // Does item still exist or has it been deleted
{
    // IsItem() may be faster ?
    return dynamic_cast<CItem *>(ObjFind());
}

inline CChar * CGrayUIDBase::CharFind() const // Does character still exist
{
	return dynamic_cast<CChar *>(ObjFind());
}

#endif //_INC_CGRAYUIDEXTRA_H
