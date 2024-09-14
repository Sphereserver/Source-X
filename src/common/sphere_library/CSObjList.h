/**
* @file  CSObjList.h
* @brief Generic list of objects (not thread safe).
*/

#ifndef _INC_CSOBJLIST_H
#define _INC_CSOBJLIST_H

#include "CSObjListRec.h"
#include <cstddef>

class CSObjList
{
public:
    friend class CSObjListRec;
    static const char * m_sClassName;

    /** @name Constructors, Destructor, Asign operator:
    */
    ///@{
    /**
    * @brief Sets head, tail and count.
    */
    CSObjList();
    virtual ~CSObjList();

    /**
    * @brief No copies allowed.
    */
    CSObjList(const CSObjList& copy) = delete;

    /**
    * @brief No copies allowed.
    */
    CSObjList& operator=(const CSObjList& other) = delete;
    ///@}

    /** @name Capacity:
    */
    ///@{
public:

    /**
    * @brief Get the record count of the list.
    * @return The record count of the list.
    */
    inline size_t GetContentCount() const noexcept;

    /**
    * @brief Check if CSObjList if empty.
    * @return true if CSObjList is empty, false otherwise.
    */
    inline bool IsContainerEmpty() const noexcept;

    ///@}
    /** @name Element Access:
    */
    ///@{

    /**
    * @brief Get the nth element of the list.
    * @param index of the element to get.
    * @return nth element if lenght is greater or equal to index, nullptr otherwise.
    */
    CSObjListRec * GetContentAt( size_t index ) const;

    /**
    * @brief Get the first record of the CSObjList.
    * @return The first record of the CSObjList if list is not empty, nullptr otherwise.
    */
    inline CSObjListRec* GetContainerHead() const noexcept;

    /**
    * @brief Get the last record of the CSObjList.
    * @return The last record of the CSObjList if list is not empty, nullptr otherwise.
    */
    inline CSObjListRec* GetContainerTail() const noexcept;
    ///@}

    /** @name Modifiers:
    */
    ///@{

    /**
    * @brief Remove all records of the CSObjList.
    */
    void ClearContainer();

    /**
    * @brief Insert a record after the referenced record.
    *
    * If the position referenced is nullptr, the record is inserted at head.
    * @param pNewRec record to insert.
    * @param pPrev position to insert after.
    */
    void InsertContentAfter( CSObjListRec * pNewRec, CSObjListRec * pPrev = nullptr );

    /**
    * @brief Insert a record at head.
    * @param pNewRec record to insert.
    */
    inline void InsertContentHead(CSObjListRec* pNewRec);

    /**
    * @brief Insert a record at tail.
    * @param pNewRec record to insert.
    */
    inline void InsertContentTail(CSObjListRec* pNewRec);

protected:
    /**
    * @brief Trigger that fires when a record if removed.
    *
    * Override this to get called when an item is removed from this list.
    * Never called directly. Called CSObjListRec::RemoveSelf()
    * @see CSObjListRec::RemoveSelf()
    * @param pObRec removed record.
    */
    virtual void OnRemoveObj( CSObjListRec* pObRec );
    ///@}

private:
    CSObjListRec * m_pHead; // Head of the list.
    CSObjListRec * m_pTail; // Tail of the list.
    size_t m_uiCount;	    // Count of elements of the CSObjList.
};


/* Inlined methods are defined here */

size_t CSObjList::GetContentCount() const noexcept
{
    return m_uiCount;
}

bool CSObjList::IsContainerEmpty() const noexcept
{
    return !GetContentCount();
}

CSObjListRec* CSObjList::GetContainerHead() const noexcept
{
    return m_pHead;
}

CSObjListRec* CSObjList::GetContainerTail() const noexcept
{
    return m_pTail;
}

void CSObjList::InsertContentHead(CSObjListRec* pNewRec)
{
    InsertContentAfter(pNewRec, nullptr);
}

void CSObjList::InsertContentTail(CSObjListRec* pNewRec)
{
    InsertContentAfter(pNewRec, GetContainerTail());
}

#endif //_INC_CSOBJLIST_H
