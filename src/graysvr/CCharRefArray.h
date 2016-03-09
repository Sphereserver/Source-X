#ifndef _INC_CCHARREFARRAY_H
#define _INC_CCHARREFARRAY_H

#include "../common/CScript.h"
#include "../common/CArray.h"
#include "../common/CGrayUID.h"

class CCharRefArray
{
private:
	// List of Players and NPC's involved in the quest/party/account etc..
	CGTypedArray< CGrayUID, CGrayUID> m_uidCharArray;

public:
	static const char *m_sClassName;
	size_t FindChar( const CChar * pChar ) const;
	bool IsCharIn( const CChar * pChar ) const;
	size_t AttachChar( const CChar * pChar );
	size_t InsertChar( const CChar * pChar, size_t i );
	void DetachChar( size_t i );
	size_t DetachChar( const CChar * pChar );
	void DeleteChars();
	size_t GetCharCount() const;
	CGrayUID GetChar( size_t i ) const;
	bool IsValidIndex( size_t i ) const;
	inline size_t BadIndex() const
	{
		return m_uidCharArray.BadIndex();
	}
	void WritePartyChars( CScript & s );

public:
	CCharRefArray() { };

private:
	CCharRefArray(const CCharRefArray& copy);
	CCharRefArray& operator=(const CCharRefArray& other);
};

#endif // _INC_CCHARREFARRAY_H
