/**
* @file CUIDExtra.h
* @brief Inlined CUIDBase methods which need CItem and CChar classes
*/

#ifndef _INC_CUIDEXTRA_H
#define _INC_CUIDEXTRA_H

#include "CUID.h"
#include "../game/CWorld.h"
#include "../game/chars/CChar.h"

inline CObjBase * CUIDBase::ObjFind() const
{
	if ( IsResource() || !IsValidUID() )
		return nullptr;
	return g_World.FindUID( m_dwInternalVal & UID_O_INDEX_MASK );
}

inline CItem * CUIDBase::ItemFind() const // Does item still exist or has it been deleted
{
    // IsItem() may be faster ?
    return dynamic_cast<CItem *>(ObjFind());
}

inline CChar * CUIDBase::CharFind() const // Does character still exist
{
	return dynamic_cast<CChar *>(ObjFind());
}

#endif // _INC_CUIDEXTRA_H
