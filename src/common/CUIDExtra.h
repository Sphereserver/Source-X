/**
* @file CUIDExtra.h
* @brief Inlined CUIDBase methods which need CItem and CChar classes
*/

#ifndef _INC_CUIDEXTRA_H
#define _INC_CUIDEXTRA_H

#include "CUID.h"
#include "../game/chars/CChar.h"


inline CItem * CUIDBase::ItemFind(dword dwPrivateUID)    // static
{
    // Does item still exist or has it been deleted?
    // IsItem() may be faster ?
    return dynamic_cast<CItem *>(ObjFind(dwPrivateUID));
}
inline CChar * CUIDBase::CharFind(dword dwPrivateUID)    // static
{
    // Does character still exists?
    return dynamic_cast<CChar *>(ObjFind(dwPrivateUID));
}

inline CObjBase * CUIDBase::ObjFind() const
{
   return ObjFind(m_dwInternalVal);
}
inline CItem * CUIDBase::ItemFind() const 
{
    return ItemFind(m_dwInternalVal);
}
inline CChar * CUIDBase::CharFind() const
{
    return CharFind(m_dwInternalVal);
}

#endif // _INC_CUIDEXTRA_H
