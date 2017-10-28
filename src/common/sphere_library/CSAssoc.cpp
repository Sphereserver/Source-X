
#include "CSAssoc.h"
#include "../CExpression.h"
#include "../common.h"

//***************************************************************************
// -CValStr

lpctstr CValStr::FindName( int iVal ) const
{
	size_t i = 0;
	ASSERT(this[i].m_pszName != NULL);
	for ( ; this[i].m_pszName; i++ )
	{
		if ( iVal < this[i + 1].m_iVal )
			return ( this[i].m_pszName );
	}
	return this[i - 1].m_pszName;
}

//***************************************************************************
// -CElementDef
// Describe the elements of a structure/class

const int CElementDef::sm_Lengths[ELEM_QTY] =
{
	0,	// ELEM_VOID:
	-1,	// ELEM_CSTRING,
	-1, // ELEM_STRING,	// Assume max size of REG_SIZE
	sizeof(bool),	// ELEM_BOOL
	sizeof(byte), // ELEM_BYTE,		// 1 byte.
	sizeof(byte), // ELEM_MASK_BYTE,	// bits in a byte
	sizeof(word), // ELEM_WORD,		// 2 bytes
	sizeof(word), // ELEM_MASK_WORD,	// bits in a word
	sizeof(int),  // ELEM_INT,		// Whatever the int size is. 4 i assume
	sizeof(int),  // ELEM_MASK_INT,
	sizeof(dword), // ELEM_DWORD,		// 4 bytes.
	sizeof(dword), // ELEM_MASK_DWORD,	// bits in a dword
};

bool CElementDef::SetValStr( void * pBase, lpctstr pszVal ) const
{
	// Set the element value as a string.
	dword dwVal = 0;
	//ASSERT(m_offset>=0);
	void * pValPtr = GetValPtr(pBase);
	switch ( m_type )
	{
		case ELEM_VOID:
			return false;
		case ELEM_STRING:
			strcpylen(static_cast<tchar *>(pValPtr), pszVal, GetValLength() - 1);
			return true;
		case ELEM_CSTRING:
			*static_cast<CSString *>(pValPtr) = pszVal;
			return true;
		case ELEM_BOOL:
		case ELEM_BYTE:
		case ELEM_WORD:
		case ELEM_INT: // signed ?
		case ELEM_DWORD:
			dwVal = Exp_GetVal( pszVal );
			memcpy( pValPtr, &dwVal, GetValLength());
			return true;
		case ELEM_MASK_BYTE:	// bits in a byte
		case ELEM_MASK_WORD:	// bits in a word
		case ELEM_MASK_INT:
		case ELEM_MASK_DWORD:	// bits in a dword
			return false;
		default:
			break;
	}
	return false;
}

void * CElementDef::GetValPtr( const void * pBaseInst ) const
{
	return ( (byte *)pBaseInst + m_offset );
}

int CElementDef::GetValLength() const
{
	ASSERT(m_type<ELEM_QTY);
	if ( m_type == ELEM_STRING )
		return m_extra;
	return sm_Lengths[m_type];
}

bool CElementDef::GetValStr( const void * pBase, CSString & sVal ) const
{
	// Get the element value as a string.

	dword dwVal = 0;
	//ASSERT(m_offset>=0);
	void * pValPtr = GetValPtr(pBase);
	switch ( m_type )
	{
		case ELEM_VOID:
			return false;
		case ELEM_STRING:
			sVal = static_cast<tchar *>(pValPtr);
			return true;
		case ELEM_CSTRING:
			sVal = *static_cast<CSString *>(pValPtr);
			return true;
		case ELEM_BOOL:
		case ELEM_BYTE:
		case ELEM_WORD:
		case ELEM_INT: // signed ?
		case ELEM_DWORD:
			memcpy( &dwVal, pValPtr, GetValLength());
			sVal.Format("%u", dwVal);
			return true;
		case ELEM_MASK_BYTE:	// bits in a byte
		case ELEM_MASK_WORD:	// bits in a word
		case ELEM_MASK_INT:
		case ELEM_MASK_DWORD:	// bits in a dword
			return false;
		default:
			break;
	}
	return false;
}
