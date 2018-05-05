#include "CSObjList.h"
#include "../assertion.h"


// CSObjListRec:: Capacity.

void CSObjListRec::RemoveSelf()
{
    if (GetParent())
        m_pParent->OnRemoveObj(this);	// call any approriate virtuals.
}

//---

// CSObjList:: Constructors, Destructor, Assign operator.

CSObjList::CSObjList()
{
	m_pHead = m_pTail = NULL;
	m_iCount = 0;
}

// CSObjList:: Element access.

CSObjListRec * CSObjList::GetAt( size_t index ) const
{
	CSObjListRec * pRec = GetHead();
	while ( index > 0 && pRec != NULL )
	{
		pRec = pRec->GetNext();
		--index;
	}
	return pRec;
}

// CSObjList:: Modifiers.

void CSObjList::Clear()
{
	// delete all entries.
	for (;;)	// iterate the list.
	{
		CSObjListRec * pRec = GetHead();
		if ( pRec == NULL )
			break;
		ASSERT( pRec->GetParent() == this );
		delete pRec;
	}
	m_iCount = 0;
	m_pHead = NULL;
	m_pTail = NULL;
}

void CSObjList::InsertAfter( CSObjListRec * pNewRec, CSObjListRec * pPrev )
{
	// Add after pPrev.
	// pPrev = NULL == add to the start.
	ASSERT( pNewRec != NULL );
	pNewRec->RemoveSelf();	// Get out of previous list first.
	ASSERT( pPrev != pNewRec );
	ASSERT( pNewRec->GetParent() == NULL );

	pNewRec->m_pParent = this;

	CSObjListRec * pNext;
	if ( pPrev != NULL )		// Its the first.
	{
		ASSERT( pPrev->GetParent() == this );
		pNext = pPrev->GetNext();
		pPrev->m_pNext = pNewRec;
	}
	else
	{
		pNext = GetHead();
		m_pHead = pNewRec;
	}

	pNewRec->m_pPrev = pPrev;

	if ( pNext != NULL )
	{
		ASSERT( pNext->GetParent() == this );
		pNext->m_pPrev = pNewRec;
	}
	else
	{
		m_pTail = pNewRec;
	}

	pNewRec->m_pNext = pNext;
	++m_iCount;
}

void CSObjList::OnRemoveObj( CSObjListRec* pObRec )	// Override this = called when removed from list.
{
	// just remove from list. DON'T delete !
	if ( pObRec == NULL )
		return;
	ASSERT( pObRec->GetParent() == this );

	CSObjListRec * pNext = pObRec->GetNext();
	CSObjListRec * pPrev = pObRec->GetPrev();

	if ( pNext != NULL )
		pNext->m_pPrev = pPrev;
	else
		m_pTail = pPrev;
	if ( pPrev != NULL )
		pPrev->m_pNext = pNext;
	else
		m_pHead = pNext;

	pObRec->m_pNext = NULL;	// this should not really be necessary.
	pObRec->m_pPrev = NULL;
	pObRec->m_pParent = NULL;	// We are now unlinked.
	--m_iCount;
}

