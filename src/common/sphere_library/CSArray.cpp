
#include "../../sphere/threads.h"
#include "CSArray.h"

// CObListRec:: Constructors, Destructor, Asign operator.

CSObjListRec::CSObjListRec()
{
	m_pParent = NULL;
	m_pNext = m_pPrev = NULL;
}

CSObjListRec::~CSObjListRec()
{
	RemoveSelf();
}

// CSObjList:: Constructors, Destructor, Asign operator.

CSObjList::CSObjList()
{
	m_pHead = m_pTail = NULL;
	m_iCount = 0;
}

CSObjList::~CSObjList()
{
	DeleteAll();
}

// CSObjList:: Capacity.

size_t CSObjList::GetCount() const
{
	ADDTOCALLSTACK("CSObjList::GetCount");
	return m_iCount;
}

bool CSObjList::IsEmpty() const
{
	ADDTOCALLSTACK("CSObjList::IsEmpty");
	return !GetCount();
}

// CSObjList:: Element access.

CSObjListRec * CSObjList::GetAt( size_t index ) const
{
	ADDTOCALLSTACK("CSObjList::GetAt");
	CSObjListRec * pRec = GetHead();
	while ( index-- > 0 && pRec != NULL )
	{
		pRec = pRec->GetNext();
	}
	return pRec;
}

CSObjListRec * CSObjList::GetHead() const
{
	ADDTOCALLSTACK("CSObjList::GetHead");
	return m_pHead;
}

CSObjListRec * CSObjList::GetTail() const
{
	ADDTOCALLSTACK("CSObjList::GetTail");
	return m_pTail;
}

// CSObjList:: Modifiers.

void CSObjList::DeleteAll()
{
	ADDTOCALLSTACK("CSObjList::DeleteAll");
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

void CSObjList::Empty()
{
	ADDTOCALLSTACK("CSObjList::Empty");
	DeleteAll();
}

void CSObjList::InsertAfter( CSObjListRec * pNewRec, CSObjListRec * pPrev )
{
	ADDTOCALLSTACK("CSObjList::InsertAfter");
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
	m_iCount ++;
}

void CSObjList::InsertHead( CSObjListRec * pNewRec )
{
	ADDTOCALLSTACK("CSObjList::InsertHead");
	InsertAfter(pNewRec, NULL);
}

void CSObjList::InsertTail( CSObjListRec * pNewRec )
{
	ADDTOCALLSTACK("CSObjList::InsertTail");
	InsertAfter(pNewRec, GetTail());
}

void CSObjList::OnRemoveObj( CSObjListRec* pObRec )	// Override this = called when removed from list.
{
	ADDTOCALLSTACK("CSObjList::OnRemoveObj");
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
	m_iCount --;
}

void CSObjList::RemoveAtSpecial( CSObjListRec * pObRec )
{
	ADDTOCALLSTACK("CSObjList::RemoveAtSpecial");
	// only called by pObRec->RemoveSelf()
	OnRemoveObj(pObRec);	// call any approriate virtuals.
}
