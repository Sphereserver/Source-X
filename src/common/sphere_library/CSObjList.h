/**
* @file  CSObjList.h
* @brief Generic list of objects (not thread safe).
*/

#ifndef _INC_CSOBJLIST_H
#define _INC_CSOBJLIST_H

#include "CSObjListRec.h"

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
    virtual ~CSObjList() {
        Clear();
    }

private:
    /**
    * @brief No copies allowed.
    */
    CSObjList(const CSObjList& copy);
    /**
    * @brief No copies allowed.
    */
    CSObjList& operator=(const CSObjList& other);
    ///@}
    /** @name Capacity:
    */
    ///@{
public:
    /**
    * @brief Get the record count of the list.
    * @return The record count of the list.
    */
    inline size_t GetCount() const {
        return m_iCount;
    }
    /**
    * @brief Check if CSObjList if empty.
    * @return true if CSObjList is empty, false otherwise.
    */
    inline bool IsEmpty() const {
        return !GetCount();
    }
    ///@}
    /** @name Element Access:
    */
    ///@{
    /**
    * @brief Get the nth element of the list.
    * @param index of the element to get.
    * @return nth element if lenght is greater or equal to index, NULL otherwise.
    */
    CSObjListRec * GetAt( size_t index ) const;
    /**
    * @brief Get the first record of the CSObjList.
    * @return The first record of the CSObjList if list is not empty, NULL otherwise.
    */
    inline CSObjListRec * GetHead() const {
        return m_pHead;
    }
    /**
    * @brief Get the last record of the CSObjList.
    * @return The last record of the CSObjList if list is not empty, NULL otherwise.
    */
    inline CSObjListRec * GetTail() const {
        return m_pTail;
    }
    ///@}
    /** @name Modifiers:
    */
    ///@{
    /**
    * @brief Remove all records of the CSObjList.
    */
    void Clear();
    /**
    * @brief Insert a record after the referenced record.
    *
    * If the position referenced is NULL, the record is inserted at head.
    * @param pNewRec record to insert.
    * @param pPrev position to insert after.
    */
    virtual void InsertAfter( CSObjListRec * pNewRec, CSObjListRec * pPrev = NULL );
    /**
    * @brief Insert a record at head.
    * @param pNewRec record to insert.
    */
    inline void InsertHead( CSObjListRec * pNewRec ) {
        InsertAfter(pNewRec, NULL);
    }
    /**
    * @brief Insert a record at tail.
    * @param pNewRec record to insert.
    */
    inline void InsertTail( CSObjListRec * pNewRec ) {
        InsertAfter(pNewRec, GetTail());
    }
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

    CSObjListRec * m_pHead;  // Head of the list.
    CSObjListRec * m_pTail;  // Tail of the list. Do we really care about tail ? (as it applies to lists anyhow)
    size_t m_iCount;		// Count of elements of the CSObjList.
};


#endif //_INC_CSOBJLIST_H
