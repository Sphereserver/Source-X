/**
* @file CQueue.cpp
*/

#include "CQueue.h"
#include "../sphere/threads.h"

// CQueueBytes:: Constructors, Destructor, Asign operator.

CQueueBytes::CQueueBytes()
{
	m_iDataQty = 0;
}

CQueueBytes::~CQueueBytes()
{
}

// CQueueBytes:: Capacity.

size_t CQueueBytes::GetDataQty() const
{
	return m_iDataQty;
}

// CQueueBytes:: Modifiers.

void CQueueBytes::AddNewData( const byte * pData, size_t iLen )
{
	ADDTOCALLSTACK("CQueueBytes::AddNewData");
	// Add new data to the end of the queue.
	memcpy( AddNewDataLock(iLen), pData, iLen );
	AddNewDataFinish( iLen );
}

void CQueueBytes::AddNewDataFinish( size_t iLen )
{
	// The data is now ready.
	m_iDataQty += iLen;
}

byte * CQueueBytes::AddNewDataLock( size_t iLen )
{
	ADDTOCALLSTACK("CQueueBytes::AddNewDataLock");
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

void CQueueBytes::Empty()
{
	m_iDataQty = 0;
}

void CQueueBytes::RemoveDataAmount( size_t iSize )
{
	ADDTOCALLSTACK("CQueueBytes::RemoveDataAmount");
	// use up this read data. (from the start)
	if ( iSize > m_iDataQty )
		iSize = m_iDataQty;
	m_iDataQty -= iSize;
	memmove( m_Mem.GetData(), m_Mem.GetData()+iSize, m_iDataQty );
}

const byte * CQueueBytes::RemoveDataLock() const
{
	return m_Mem.GetData();
}
