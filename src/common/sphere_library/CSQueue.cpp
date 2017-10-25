/**
* @file CQueue.cpp
*/

#include <cstring>
#include "CSQueue.h"
#include "../../sphere/threads.h"

// CSQueueBytes:: Constructors, Destructor, Asign operator.

CSQueueBytes::CSQueueBytes()
{
	m_iDataQty = 0;
}

CSQueueBytes::~CSQueueBytes()
{
}

// CSQueueBytes:: Capacity.

size_t CSQueueBytes::GetDataQty() const
{
	return m_iDataQty;
}

// CSQueueBytes:: Modifiers.

void CSQueueBytes::AddNewData( const byte * pData, size_t iLen )
{
	//ADDTOCALLSTACK("CSQueueBytes::AddNewData");
	// Add new data to the end of the queue.
	memcpy( AddNewDataLock(iLen), pData, iLen );
	AddNewDataFinish( iLen );
}

void CSQueueBytes::AddNewDataFinish( size_t iLen )
{
	// The data is now ready.
	m_iDataQty += iLen;
}

byte * CSQueueBytes::AddNewDataLock( size_t iLen )
{
	//ADDTOCALLSTACK("CSQueueBytes::AddNewDataLock");
	// lock the queue to place this data in it.

	size_t iLenNew = m_iDataQty + iLen;
	if ( iLenNew > m_Mem.GetDataLength() )
	{
		// re-alloc a bigger buffer. as needed.

		ASSERT(m_iDataQty<=m_Mem.GetDataLength());
		m_Mem.Resize( ( iLenNew + 0x1000 ) &~ 0xFFF );
	}

	return( m_Mem.GetData() + m_iDataQty );
}

void CSQueueBytes::Empty()
{
	m_iDataQty = 0;
}

void CSQueueBytes::RemoveDataAmount( size_t iSize )
{
	//ADDTOCALLSTACK("CSQueueBytes::RemoveDataAmount");
	// use up this read data. (from the start)
	if ( iSize > m_iDataQty )
		iSize = m_iDataQty;
	m_iDataQty -= iSize;
	memmove( m_Mem.GetData(), m_Mem.GetData()+iSize, m_iDataQty );
}

const byte * CSQueueBytes::RemoveDataLock() const
{
	return m_Mem.GetData();
}
