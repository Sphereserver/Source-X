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
	// delete all entries.
	_fIsClearing = true;

	for (CSObjContRec* pRec : *this)	// iterate the list.
	{
		ASSERT( pRec->GetParent() == this );
		delete pRec;
	}

	_fIsClearing = false;
	clear();
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
