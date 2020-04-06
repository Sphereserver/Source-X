#include "CSObjCont.h"
#include "../assertion.h"
#include <algorithm>


CSObjContRec::CSObjContRec()
{
    m_pParent = nullptr;
}

CSObjContRec::~CSObjContRec()
{
    RemoveSelf();
}

void CSObjContRec::RemoveSelf()
{
    if (m_pParent)
        m_pParent->OnRemoveObj(this);	// call any approriate virtuals.
}

//---

// CSObjCont:: Constructors, Destructor, Assign operator.

CSObjCont::CSObjCont()
{
	_fIsClearing = false;
}

CSObjCont::~CSObjCont()
{
	ClearContainer();
}

// CSObjCont:: Modifiers.

void CSObjCont::ClearContainer()
{
	if (empty())
		return;

	// delete all entries.
	_fIsClearing = true;

	// Loop through a copy of the current state of the container, since by deleting other container objects it could happen that
	//	other objects are deleted and appended to this list, thus invalidating the iterators used by the for loop.
	const auto stateCopy = GetIterationSafeContReverse();
	clear();

	for (CSObjContRec* pRec : stateCopy)	// iterate the list.
	{
		ASSERT( pRec->GetParent() == this );
		delete pRec;
	}

	_fIsClearing = false;
}

/*
void CSObjCont::InsertContentHead(CSObjContRec* pNewRec)
{
	pNewRec->RemoveSelf();
	pNewRec->m_pParent = this;
	//emplace(begin(), pNewRec);
    emplace_front(pNewRec);
}
*/

void CSObjCont::InsertContentTail(CSObjContRec* pNewRec)
{
	pNewRec->RemoveSelf();
	pNewRec->m_pParent = this;
	emplace_back(pNewRec);
}

void CSObjCont::OnRemoveObj(CSObjContRec* pObjRec )	// Override this = called when removed from list.
{
	// just remove from list. DON'T delete !
	ASSERT(pObjRec);
	ASSERT(pObjRec->GetParent() == this);

	if (!_fIsClearing)
	{
		iterator itEnd = end();
		iterator itObjRec = std::find(begin(), itEnd, pObjRec);
		ASSERT (itObjRec != itEnd);
		erase(itObjRec);
	}

	pObjRec->m_pParent = nullptr;	// We are now unlinked.
}
