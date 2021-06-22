/**
* @file CCharRefArray.h
*
*/

#ifndef _INC_CCHARREFARRAY_H
#define _INC_CCHARREFARRAY_H

#include "../../common/sphere_library/CSTypedArray.h"
#include "../../common/CUID.h"


class CChar;
class CScript;

class CCharRefArray
{
private:
	// List of Players and NPC's involved in the quest/party/account etc..
	CSTypedArray<CUID> m_uidCharArray;

public:
	static const char *m_sClassName;
	size_t FindChar( const CChar * pChar ) const;
	bool IsCharIn( const CChar * pChar ) const;
	size_t AttachChar( const CChar * pChar );
	size_t InsertChar( const CChar * pChar, size_t i );
	void DetachChar( size_t i );
	size_t DetachChar( const CChar * pChar );
	void DeleteChars();
	inline size_t GetCharCount() const
	{
		return m_uidCharArray.size();
	}
	inline const CUID& GetChar( size_t i ) const
	{
		return m_uidCharArray[i];
	}
	inline bool IsValidIndex( size_t i ) const
	{
		return m_uidCharArray.IsValidIndex(i);
	}
	void WritePartyChars( CScript & s );

public:
	CCharRefArray() = default;

private:
	CCharRefArray(const CCharRefArray& copy);
	CCharRefArray& operator=(const CCharRefArray& other);
};


#endif // CCHARREFARRAY_H
