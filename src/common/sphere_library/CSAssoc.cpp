
#include "CSAssoc.h"
#include "../CExpression.h"
#include "../common.h"

//***************************************************************************
// -CValStr

lpctstr CValStr::FindName( int iVal ) const
{
	size_t i = 0;
	ASSERT(this[i].m_pszName != nullptr);
	for ( ; this[i].m_pszName; ++i )
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
	sizeof(bool),	// ELEM_BOOL
	sizeof(byte), // ELEM_BYTE,			// 1 byte.
	sizeof(byte), // ELEM_MASK_BYTE,	// bits in a byte
	sizeof(word), // ELEM_WORD,			// 2 bytes
	sizeof(word), // ELEM_MASK_WORD,	// bits in a word
	sizeof(int),  // ELEM_INT,			// Whatever the int size is. 4 i assume
	sizeof(uint), // ELEM_MASK_INT,		// bits in a int
	sizeof(int64),  // ELEM_INT64,		// 8 bytes
	sizeof(uint64), // ELEM_MASK_INT64,	// 8 bytes.
	sizeof(dword), // ELEM_DWORD,		// 4 bytes.
	sizeof(dword), // ELEM_MASK_DWORD,	// bits in a dword
};

bool CElementDef::SetValStr( void * pBase, lpctstr ptcVal ) const
{
	// Set the element value as a string.
	uint64 qwVal = 0;
	//ASSERT(m_offset>=0);
	void * pValPtr = GetValPtr(pBase);
	switch ( m_type )
	{
		case ELEM_VOID:
			return false;
		case ELEM_CSTRING:
			*static_cast<CSString *>(pValPtr) = ptcVal;
			return true;
		case ELEM_BOOL:
		case ELEM_INT:
		case ELEM_INT64:
			qwVal = Exp_Get64Val(ptcVal);
			memcpy( pValPtr, &qwVal, GetValLength() );
			return true;
		case ELEM_BYTE:
		case ELEM_WORD:
		case ELEM_DWORD:
		case ELEM_MASK_BYTE:	// bits in a byte
		case ELEM_MASK_WORD:	// bits in a word
		case ELEM_MASK_DWORD:	// bits in a dword
		case ELEM_MASK_INT:		// bits in an (unsigned) int
		case ELEM_MASK_INT64:	// bits in an (unsigned) int64
			qwVal = Exp_GetU64Val(ptcVal);
			memcpy( pValPtr, &qwVal, GetValLength());
			return true;
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
	ASSERT((m_type >= ELEM_VOID) && (m_type < ELEM_QTY));
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
		case ELEM_CSTRING:
			sVal = *static_cast<CSString *>(pValPtr);
			return true;
		case ELEM_BOOL:
		case ELEM_INT:
			memcpy( &dwVal, pValPtr, GetValLength() );
			sVal.Format("%d", dwVal);
			return true;
		case ELEM_BYTE:
		case ELEM_WORD:
		case ELEM_DWORD:
		case ELEM_MASK_BYTE:	// bits in a byte
		case ELEM_MASK_WORD:	// bits in a word
		case ELEM_MASK_DWORD:	// bits in a dword
		case ELEM_MASK_INT:		// bits in an (unsigned) int
			memcpy( &dwVal, pValPtr, GetValLength() );
			sVal.Format("%u", dwVal);
			return true;
		default:
			break;
	}
	return false;
}
