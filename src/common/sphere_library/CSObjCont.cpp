#include "CSObjCont.h"
#include "../assertion.h"
#include "../CException.h"
#include <algorithm>


CSObjContRec::CSObjContRec() :
    m_pParent(nullptr)
{
}

CSObjContRec::~CSObjContRec()
{
    RemoveSelf();
}

void CSObjContRec::RemoveSelf()
{
    if (m_pParent) {
        m_pParent->OnRemoveObj(this);	// call any approriate virtuals.
    }
}

//---

// CSObjCont:: Constructors, Destructor, Assign operator.

CSObjCont::CSObjCont() :
    _fClearingContainer(false)
{
}

CSObjCont::~CSObjCont()
{
    // Do not virtually call this method, since i'm in the destructor of the base-most class.
	CSObjCont::ClearContainer(true);
}

// CSObjCont:: Modifiers.

void CSObjCont::ClearContainer(bool fClosingWorld)
{
	if (_Contents.empty())
		return;

	// delete all entries.

	// Loop through a copy of the current state of the container, since by deleting other container objects it could happen that
	//	other objects are deleted and appended to this list, thus invalidating the iterators used by the for loop.
	const auto stateCopy = GetIterationSafeContReverse();
	_Contents.clear();
    _fClearingContainer = true;

    EXC_TRY("Deleting objects");

    if (fClosingWorld)
    {
        EXC_SET_BLOCK("Closing world cleanup.");
        // Do not notify the parent element.
        for (CSObjContRec* pRec : stateCopy)
        {
            // This might not be true for some containers.
            //  Example: sectors. We might want to delete the object here just after it's been detatched from its sector.
            //ASSERT(pRec->GetParent() == this);

            //pRec->m_pParent->_ContentsAlreadyDeleted.emplace_back(pRec);
            pRec->m_pParent = nullptr;
            delete pRec;
        }
    }
    else
    {
        EXC_SET_BLOCK("Standard cleanup.");
        	for (CSObjContRec* pRec : stateCopy)
        {
            // This might not be true for some containers.
            //  Example: sectors. We might want to delete the object here just after it's been detatched from its sector.
            //ASSERT(pRec->GetParent() == this);

            OnRemoveObj(pRec);
            delete pRec;
        }
    }

    _fClearingContainer = false;
	EXC_CATCH;
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
#ifdef _DEBUG
	const_iterator itEnd = cend();
	const_iterator itObjRec = std::find(cbegin(), itEnd, pNewRec);
	ASSERT(itObjRec == itEnd);
	// Avoid duplicates, thus delete-ing objects multiple times
#endif

    pNewRec->RemoveSelf();
    pNewRec->m_pParent = this;

    _Contents.emplace_back(pNewRec);
}

void CSObjCont::OnRemoveObj(CSObjContRec* pObjRec)	// Override this = called when removed from list.
{
	// just remove from list. DON'T delete !
	ASSERT(pObjRec);

	ASSERT(pObjRec->GetParent() == this);

	pObjRec->m_pParent = nullptr;	// We are now unlinked.

	iterator itObjRec = std::find(begin(), end(), pObjRec);
    if (itObjRec == end())
        return;

	/*
	if (!_fClearingContainer)
    {
        // _fCleaning == true happens when a CSObjCont deletes its CSObjContRec.
        //  CSObjContRec::RemoveSelf calls its CSObjCont::OnRemoveObj, so we are here, but in this case _Contents is empty,
        //  so it's expected not to find the object here..
        iterator itObjRec = std::find(begin(), end(), pObjRec);
        ASSERT(itObjRec != end());

        _Contents.erase(itObjRec);
    }
    */

    _Contents.erase(itObjRec);
}
