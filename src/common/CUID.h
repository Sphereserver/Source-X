/**
* @file CUID.h
*/

#ifndef _INC_CUID_H
#define _INC_CUID_H

#include "common.h"		// for the datatypes


class CObjBase;
class CItem;
class CChar;

#define UID_CLEAR			(dword)0
#define UID_UNUSED			(dword)0xFFFFFFFF	// 0 = not used as well.

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
	inline void InitUID() noexcept {
		m_dwInternalVal = UID_UNUSED;
	}

    // Use ClearUID only if the CUID is not used as a pure UID, but it can assume other kind of values.
    //  Example: m_itFigurine.m_UID, m_itKey.m_UIDLock -> a MORE1/MORE2 == 0 is considered legit, also for many many item types MORE* isn't a UID.
	inline void ClearUID() noexcept {
		m_dwInternalVal = UID_CLEAR;
	}

	inline CUID() noexcept
	{
		InitUID();
	}
	inline CUID(const CUID& uid) noexcept
	{
		SetPrivateUID(uid.GetPrivateUID());
	}
	inline explicit CUID(dword dwPrivateUID) noexcept
	{
		// TODO: directly setting the private UID can led to unexpected results...
		//	it's better to use SetObjUID in order to "filter" the raw value passed.
		//  Though, it needs some testing before, who knows if in the big sea of misuses there's a legit use...
		SetPrivateUID(dwPrivateUID);
	}

    static bool IsValidUID(dword dwPrivateUID) noexcept;
    static bool IsResource(dword dwPrivateUID) noexcept;
    static bool IsValidResource(dword dwPrivateUID) noexcept;
    static bool IsItem(dword dwPrivateUID) noexcept;
    static bool IsChar(dword dwPrivateUID) noexcept;

    inline bool IsValidUID() const noexcept {
        return IsValidUID(m_dwInternalVal);
    }
    inline bool IsResource() const noexcept {
        return IsResource(m_dwInternalVal);
    }
    inline bool IsValidResource() const noexcept {
        return IsResource(m_dwInternalVal);
    }
    inline bool IsItem() const noexcept {
        return IsItem(m_dwInternalVal);
    }
    inline bool IsChar() const noexcept {
        return IsChar(m_dwInternalVal);
    }

	bool IsObjDisconnected() const noexcept;
	bool IsObjTopLevel() const noexcept;

	bool IsItemEquipped() const noexcept;
	bool IsItemInContainer() const noexcept;

	void SetObjContainerFlags(dword dwFlags) noexcept;
    void RemoveObjFlags(dword dwFlags) noexcept;

	inline void SetPrivateUID(dword dwVal) noexcept {
		m_dwInternalVal = dwVal;
	}
	inline dword GetPrivateUID() const noexcept {
		return m_dwInternalVal;
	}

	dword GetObjUID() const noexcept;
	void SetObjUID(dword dwVal) noexcept;

public:
	operator bool() const noexcept = delete;
	inline operator dword () const noexcept {
		return GetObjUID();
	}

	inline bool operator < (CUID const& rhs) const noexcept {	// for std::less
		return m_dwInternalVal < rhs.m_dwInternalVal;
	}

	inline bool operator != (dword index) const noexcept {
		return (GetObjUID() != index);
	}
	inline bool operator == (dword index) const noexcept {
		return (GetObjUID() == index);
	}
	CUID& operator = (const CUID&) noexcept = default;

protected:
	// Be sure that, when doing an assignment directly via a dword (so directly assigning a m_dwInternalVal), we know what we are doing.
	// To enforce that we make this method protected, so that the only public way to assign the m_dwInternalVal is via SetPrivateUID.
	// That's very necessary also because most of the times we want to do the assignment via SetObjUID, not SetPrivateUID.
	CUID& operator = (dword index) noexcept {
		SetPrivateUID(index);
		return *this;
	}

public:
    static CObjBase * ObjFindFromUID(dword dwPrivateUID, bool fInvalidateBeingDeleted = false) noexcept;
    static CItem * ItemFindFromUID(dword dwPrivateUID, bool fInvalidateBeingDeleted = false) noexcept;
    static CChar * CharFindFromUID(dword dwPrivateUID, bool fInvalidateBeingDeleted = false) noexcept;
	inline CObjBase* ObjFind(bool fInvalidateBeingDeleted = false) const noexcept {
		return ObjFindFromUID(m_dwInternalVal, fInvalidateBeingDeleted);
	}
	inline CItem* ItemFind(bool fInvalidateBeingDeleted = false) const noexcept {
		return ItemFindFromUID(m_dwInternalVal, fInvalidateBeingDeleted);
	}
	inline CChar* CharFind(bool fInvalidateBeingDeleted = false) const noexcept{
		return CharFindFromUID(m_dwInternalVal, fInvalidateBeingDeleted);
	}
    
};

#endif // _INC_CUID_H
