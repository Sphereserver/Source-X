#include "CSObjCont.h"
#include "../assertion.h"
#include "../CException.h"
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
	if (_Contents.empty())
		return;

	// delete all entries.
	ASSERT(!_fIsClearing);
	_fIsClearing = true;

	// Loop through a copy of the current state of the container, since by deleting other container objects it could happen that
	//	other objects are deleted and appended to this list, thus invalidating the iterators used by the for loop.
	const auto stateCopy = GetIterationSafeContReverse();
	_Contents.clear();

	for (CSObjContRec* pRec : stateCopy)	// iterate the list.
	{
		EXC_TRY("Deleting objects scheduled for deletion");

		if (pRec->GetParent() == this)
		{
			// I still haven't figured why sometimes, when force closing sectors, an item is stored in both the g_World.m_ObjDelete and the sector object lists
			OnRemoveObj(pRec);
			delete pRec;
		}

		EXC_CATCH;
	}

	_fIsClearing = false;
}

/*
void CSObjCont::InsertContentHead(CSObjContRec* pNewRec)
{
	pNewRec->RemoveSelf();
	pNewRec->m_pParent = this;

	if (itObjRec == itEnd)
	{
		// Avoid duplicates, thus delete-ing objects multiple times
		//_Contents.emplace(begin(), pNewRec);
		_Contents.emplace_front(pNewRec);
	}
}
*/

void CSObjCont::InsertContentTail(CSObjContRec* pNewRec)
{
    pNewRec->RemoveSelf();
    pNewRec->m_pParent = this;

#ifdef _DEBUG
	const_iterator itEnd = cend();
	const_iterator itObjRec = std::find(cbegin(), itEnd, pNewRec);
	ASSERT(itObjRec == itEnd);
	// Avoid duplicates, thus delete-ing objects multiple times
#endif

    _Contents.emplace_back(pNewRec);
}

void CSObjCont::OnRemoveObj(CSObjContRec* pObjRec)	// Override this = called when removed from list.
{
	// just remove from list. DON'T delete !
	ASSERT(pObjRec);
	ASSERT(pObjRec->GetParent() == this);

	pObjRec->m_pParent = nullptr;	// We are now unlinked.

	if (!_fIsClearing)
	{
		iterator itEnd = end();
		iterator itObjRec = std::find(begin(), itEnd, pObjRec);
		ASSERT(itObjRec != itEnd);
		_Contents.erase(itObjRec);
	}
}
