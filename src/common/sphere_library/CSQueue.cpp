/**
* @file CQueue.cpp
*/

#include <cstring>
#include "CSQueue.h"

// CSQueueBytes:: Constructors, Destructor, Asign operator.

CSQueueBytes::CSQueueBytes() noexcept :
    m_iDataQty(0)
{}

// CSQueueBytes:: Modifiers.

void CSQueueBytes::AddNewData( const byte * pData, size_t iLen )
{
	memcpy( AddNewDataLock(iLen), pData, iLen );	// Add new data to the end of the queue.
	m_iDataQty += iLen;								// The data is now ready.
}

byte * CSQueueBytes::AddNewDataLock( size_t iLen )
{
	// lock the queue to place this data in it.

	size_t iLenNew = m_iDataQty + iLen;
	if ( iLenNew > m_Mem.GetDataLength() )
	{
		// re-alloc a bigger buffer. as needed.

		ASSERT(m_iDataQty<=m_Mem.GetDataLength());
		m_Mem.Resize( ( iLenNew + 0x1000u ) & (size_t)~0xFFF );
	}

	return ( m_Mem.GetData() + m_iDataQty );
}

void CSQueueBytes::RemoveDataAmount( size_t iSize )
{
	// use up this read data. (from the start)
	if ( iSize > m_iDataQty )
		iSize = m_iDataQty;
	m_iDataQty -= iSize;
	memmove( m_Mem.GetData(), m_Mem.GetData()+iSize, m_iDataQty );
}

