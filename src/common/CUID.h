/**
* @file CUID.h
*/

#ifndef _INC_CUID_H
#define _INC_CUID_H

#include "common.h"		// for the datatypes


class CObjBase;
class CItem;
class CChar;

#define UID_CLEAR			0
#define UID_UNUSED			0xFFFFFFFF	// 0 = not used as well.

#define UID_F_RESOURCE		0x80000000	// ALSO: pileable or special macro flag passed to client.
#define UID_F_ITEM			0x40000000	// CItem as apposed to CChar based

#define UID_O_EQUIPPED		0x20000000	// This item is equipped.
#define UID_O_CONTAINED		0x10000000	// This item is inside another container
#define UID_O_DISCONNECT	0x30000000	// Not attached yet.
#define UID_O_INDEX_MASK	0x0FFFFFFF	// lose the upper bits.
#define UID_O_INDEX_FREE	0x01000000	// Spellbook needs unused UID's ?

class CUID		// A unique system serial id. 4 bytes long
{
	// This is a ref to a game object. It may or may not be valid.
	// The top few bits are just flags.
protected:
	dword m_dwInternalVal;

public:
	inline void InitUID() {
		m_dwInternalVal = UID_UNUSED;
	}

    // Use ClearUID only if the CUID is not used as a pure UID, but it can assume other kind of values.
    //  Example: m_itFigurine.m_UID, m_itKey.m_UIDLock -> a MORE1/MORE2 == 0 is considered legit, also for many many item types MORE* isn't a UID.
	inline void ClearUID() {
		m_dwInternalVal = UID_CLEAR;
	}

	inline CUID()
	{
		InitUID();
	}
	inline CUID(dword dwPrivateUID)
	{
		SetPrivateUID(dwPrivateUID);
	}

    static bool IsValidUID(dword dwPrivateUID);
    static bool IsResource(dword dwPrivateUID);
    static bool IsValidResource(dword dwPrivateUID);
    static bool IsItem(dword dwPrivateUID);
    static bool IsChar(dword dwPrivateUID);

    inline bool IsValidUID() const {
        return IsValidUID(m_dwInternalVal);
    }
    inline bool IsResource() const {
        return IsResource(m_dwInternalVal);
    }
    inline bool IsValidResource() const {
        return IsResource(m_dwInternalVal);
    }
    inline bool IsItem() const {
        return IsItem(m_dwInternalVal);
    }
    inline bool IsChar() const {
        return IsChar(m_dwInternalVal);
    }

	bool IsObjDisconnected() const;
	bool IsObjTopLevel() const;

	bool IsItemEquipped() const;
	bool IsItemInContainer() const;

	void SetObjContainerFlags(dword dwFlags);
    void RemoveObjFlags(dword dwFlags);

	inline void SetPrivateUID(dword dwVal) {
		m_dwInternalVal = dwVal;
	}
	inline dword GetPrivateUID() const {
		return m_dwInternalVal;
	}

	dword GetObjUID() const;
	void SetObjUID(dword dwVal);

	inline bool operator == (dword index) const {
		return (GetObjUID() == index);
	}
	inline bool operator != (dword index) const {
		return (GetObjUID() != index);
	}
	inline operator dword () const {
		return GetObjUID();
	}
    
    static CObjBase * ObjFind(dword dwPrivateUID);
    static CItem * ItemFind(dword dwPrivateUID);
    static CChar * CharFind(dword dwPrivateUID);
    CObjBase * ObjFind() const;
    CItem * ItemFind() const;
    CChar * CharFind() const;
    
};

#endif // _INC_CUID_H
