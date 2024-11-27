#include "CSObjList.h"
#include "../assertion.h"
#include "../CException.h"


// CSObjListRec:: Constructors, Destructor, Assign operator.

CSObjListRec::CSObjListRec() noexcept :
    m_pParent(nullptr), m_pNext(nullptr), m_pPrev(nullptr)
    {}

CSObjListRec::~CSObjListRec()
{
     RemoveSelf();
}


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
	m_pHead = m_pTail = nullptr;
	m_uiCount = 0;
}

CSObjList::~CSObjList()
{
	ClearContainer();
}

// CSObjList:: Element access.

CSObjListRec * CSObjList::GetContentAt( size_t index ) const
{
	CSObjListRec * pRec = GetContainerHead();
	while ( index > 0 && pRec != nullptr )
	{
		pRec = pRec->GetNext();
		--index;
	}
	return pRec;
}

// CSObjList:: Modifiers.

void CSObjList::ClearContainer()
{
	// delete all entries.
    bool fSuccess = false;
	EXC_TRY("Deleting objects scheduled for deletion");
	for (;;)	// iterate the list.
	{
		CSObjListRec * pRec = GetContainerHead();
        if ( pRec == nullptr ) {
            fSuccess = true;
            break;
        }
		ASSERT( pRec->GetParent() == this );
		delete pRec;
	}
	EXC_CATCH;

    if (fSuccess) {
        ASSERT(m_uiCount == 0);
    }
    else {
        m_uiCount = 0;
    }
	m_pHead = nullptr;
	m_pTail = nullptr;
}

void CSObjList::InsertContentAfter( CSObjListRec * pNewRec, CSObjListRec * pPrev )
{
	// Add after pPrev.
	// pPrev = nullptr == add to the start.
	ASSERT( pNewRec != nullptr );
	pNewRec->RemoveSelf();	// Get out of previous list first.
	ASSERT( pPrev != pNewRec );
	ASSERT( pNewRec->GetParent() == nullptr );

	pNewRec->m_pParent = this;

	CSObjListRec * pNext;
	if ( pPrev != nullptr )		// Its the first.
	{
		ASSERT( pPrev->GetParent() == this );
		pNext = pPrev->GetNext();
		pPrev->m_pNext = pNewRec;
	}
	else
	{
		pNext = GetContainerHead();
		m_pHead = pNewRec;
	}

	pNewRec->m_pPrev = pPrev;

	if ( pNext != nullptr )
	{
		ASSERT( pNext->GetParent() == this );
		pNext->m_pPrev = pNewRec;
	}
	else
	{
		m_pTail = pNewRec;
	}

	pNewRec->m_pNext = pNext;
	++m_uiCount;
}

void CSObjList::OnRemoveObj( CSObjListRec* pObRec )	// Override this = called when removed from list.
{
	// just remove from list. DON'T delete !
	if ( pObRec == nullptr )
		return;
	ASSERT( pObRec->GetParent() == this );

	CSObjListRec * pNext = pObRec->GetNext();
	CSObjListRec * pPrev = pObRec->GetPrev();

	if ( pNext != nullptr )
		pNext->m_pPrev = pPrev;
	else
		m_pTail = pPrev;
	if ( pPrev != nullptr )
		pPrev->m_pNext = pNext;
	else
		m_pHead = pNext;

	pObRec->m_pNext = nullptr;	// this should not really be necessary.
	pObRec->m_pPrev = nullptr;
	pObRec->m_pParent = nullptr;	// We are now unlinked.
	--m_uiCount;
}

